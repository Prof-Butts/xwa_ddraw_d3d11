/*
 * Denoise filter for GTAO (GTAOCommon.h). Licensed under the MIT license. See LICENSE.txt
 *
 * The GTAO pass offsets its samples with a pattern that repeats every 4x4 pixels, so the
 * average of any 4x4 block uses all 16 offsets. This pass averages the 4x4 block around
 * each pixel, skipping the pixels at a different depth, so the occlusion doesn't bleed
 * across the edges of objects.
 *
 * Input: t0 = GTAO buffer (bent normal, AO), t1 = position buffer. Output: the same as the
 * GTAO buffer. At half resolution (gtao_scale = 2) it works on the top-left quarter of the
 * GTAO buffers, and the depths come from the top-left pixel of each 2x2 block of the
 * position buffer, like in the GTAO pass.
 *
 * GTAO_GI_DENOISE (GIDenoise.hlsl) filters the indirect light instead (GTAOGI.hlsl, at a
 * quarter of the resolution): the same average, but the input is a color (rgb, a = 1; a = 0
 * for the pixels the gather skipped), so it isn't normalized like a bent normal. Its depths
 * come from the positions and normals with mips (t1, GIGeomMipsCommon.h), at the level of
 * its resolution (gtao_geom_level): the same values as the position buffer's top-left pixels,
 * but contiguous. Shader model 5.0 (f16tof32).
 *
 * The AO's denoise does the same when gtao_geom_level >= 0 (pbr_gtao_mips, with the indirect
 * light on: the mips only exist then), with t2; otherwise it reads the position buffer (t1).
 */
#include "GTAOCBuffer.h"

#ifdef GTAO_GI_DENOISE
#define DENOISE_SKIPPED(value) ((value).a < 0.5)
#else
// The pixels that GTAO skipped have AO = 2
#define DENOISE_SKIPPED(value) ((value).a > 1.5)
#endif

#ifdef GTAO_GI_DENOISE
#ifdef INSTANCED_RENDERING
Texture2DArray texGTAO         : register(t0);
Texture2DArray<uint4> texGeom  : register(t1);
#define DENOISE_DEPTH(p) f16tof32(texGeom.Load(int4(p, viewId, (int)gtao_geom_level)).z)
#else
Texture2D texGTAO         : register(t0);
Texture2D<uint4> texGeom  : register(t1);
#define DENOISE_DEPTH(p) f16tof32(texGeom.Load(int3(p, (int)gtao_geom_level)).z)
#endif
#else
#define GEOM_PACKING_SM4 1
#include "GIGeomPacking.h"
#ifdef INSTANCED_RENDERING
Texture2DArray texGTAO        : register(t0);
Texture2DArray texPos         : register(t1);
Texture2DArray<uint4> texGeom : register(t2);
#define DENOISE_DEPTH(p) DenoiseDepth(p, viewId, scale, geomLevel, useGeom)
#else
Texture2D texGTAO        : register(t0);
Texture2D texPos         : register(t1);
Texture2D<uint4> texGeom : register(t2);
#define DENOISE_DEPTH(p) DenoiseDepth(p, 0, scale, geomLevel, useGeom)
#endif

// The depth of this pass's pixel p: from the positions with mips at geomLevel (useGeom), or
// from the top-left pixel of its block in the position buffer. useGeom is a constant in
// each copy of the filter (see main()), so there's no branch per tap.
float DenoiseDepth(int2 p, uint viewId, int scale, int geomLevel, bool useGeom)
{
	float z;
	if (useGeom)
	{
#ifdef INSTANCED_RENDERING
		z = HalfToFloat(texGeom.Load(int4(p, viewId, geomLevel)).z);
#else
		z = HalfToFloat(texGeom.Load(int3(p, geomLevel)).z);
#endif
	}
	else
	{
#ifdef INSTANCED_RENDERING
		z = texPos.Load(int4(p * scale, viewId, 0)).z;
#else
		z = texPos.Load(int3(p * scale, 0)).z;
#endif
	}
	return z;
}
#endif

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

// The filter. useGeom: the depths come from the positions with mips (the AO's denoise only;
// the indirect light's always reads them).
float4 Denoise(PixelShaderInput input, bool useGeom)
{
	const int scale = (int)gtao_scale;
	const int2 px = int2(input.pos.xy);
	// The part of the GTAO buffer that's used (rounded up at half resolution)
	const int2 maxPx = int2((uint2(gtao_buffer_size) + (uint)scale - 1) / (uint)scale) - 1;
#ifndef GTAO_GI_DENOISE
	// The depths from the positions with mips (gtao_geom_level >= 0) or the position buffer
	const int geomLevel = (int)gtao_geom_level;
#endif
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
	const float4 center = texGTAO.Load(int4(px, viewId, 0));
#else
	const float4 center = texGTAO.Load(int3(px, 0));
#endif
	const float z = DENOISE_DEPTH(px);
	// The background and the pixels that GTAO skipped are left as they are
	if (DENOISE_SKIPPED(center))
		return center;
#ifdef GTAO_GI_DENOISE
	// So are the ones with no light nearby (GI_EMPTY in GTAOCommon.h)
	if (center.a < 0.9)
		return center;
#endif

	const float tolerance = max(z * gtao_denoise_depth, 1e-3);
	float4 sum = 0;
	float weightSum = 0;
	[unroll]
	for (int y = -2; y < 2; y++)
	{
		[unroll]
		for (int x = -2; x < 2; x++)
		{
			const int2 p = clamp(px + int2(x, y), int2(0, 0), maxPx);
#ifdef INSTANCED_RENDERING
			const float4 value = texGTAO.Load(int4(p, viewId, 0));
#else
			const float4 value = texGTAO.Load(int3(p, 0));
#endif
			const float sz = DENOISE_DEPTH(p);
			// Skipped pixels (AO = 2) aren't blended in. The AO isn't clamped to 1 yet: the
			// values a bit above and below 1 on open surfaces average to 1 here.
			const float w = DENOISE_SKIPPED(value) ? 0.0 : saturate(1.0 - abs(sz - z) / tolerance);
			sum += value * w;
			weightSum += w;
		}
	}
	// The center pixel always has weight 1, so weightSum > 0
	float4 result = sum / weightSum;
#ifdef GTAO_GI_DENOISE
	result.a = 1.0;
#else
	result.xyz = length(result.xyz) > 1e-4 ? normalize(result.xyz) : center.xyz;
#endif
	return result;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
#ifdef GTAO_GI_DENOISE
	return Denoise(input, true);
#else
	// One copy of the filter for each source of the depths, so the choice is made once
	float4 result;
	[branch]
	if (gtao_geom_level >= 0.0)
		result = Denoise(input, true);
	else
		result = Denoise(input, false);
	return result;
#endif
}
