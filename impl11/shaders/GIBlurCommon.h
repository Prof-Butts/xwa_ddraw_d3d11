/*
 * The wider filter of the indirect light (pbr_gi_blur_radius): a separable Gaussian blur,
 * run twice (gtao_blur_dir = (1, 0), then (0, 1)) on the indirect light after its 4x4
 * denoise filter (GIDenoise.hlsl), at a quarter of the resolution. The 4x4 filter removes
 * the noise of the gather, but it's a box over 4x4 blocks, so it leaves blocks where the
 * light changes quickly; the indirect light is smooth, so it can be blurred much more.
 * Neighbors at a different depth (another object) or facing another way (across a crease)
 * get less weight, so the light doesn't spread to them. Licensed under the MIT license.
 *
 * Input: t0 = the indirect light (rgb, a = 1; a = 0: skipped), t1 = the positions and
 * normals with mips (GIGeomMipsCommon.h), read at the level of this filter's resolution
 * (gtao_geom_level): the full-resolution buffers' top-left pixels, packed and contiguous.
 * Output: the same as t0. Shader model 5.0 (f16tof32).
 */
#include "GTAOCBuffer.h"
#include "GIGeomPacking.h"

#ifdef INSTANCED_RENDERING
Texture2DArray texGI          : register(t0);
Texture2DArray<uint4> texGeom : register(t1);
#define GI_LOAD(p) texGI.Load(int4(p, viewId, 0))
#define GI_GEOM(p) texGeom.Load(int4(p, viewId, (int)gtao_geom_level))
#else
Texture2D texGI          : register(t0);
Texture2D<uint4> texGeom : register(t1);
#define GI_LOAD(p) texGI.Load(int3(p, 0))
#define GI_GEOM(p) texGeom.Load(int3(p, (int)gtao_geom_level))
#endif

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

float4 main(PixelShaderInput input) : SV_TARGET
{
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
#else
	const uint viewId = 0;
#endif
	const int scale = (int)gtao_scale;
	const int2 px = int2(input.pos.xy);
	const int2 maxPx = int2((uint2(gtao_buffer_size) + (uint)scale - 1) / (uint)scale) - 1;
	const float4 center = GI_LOAD(px);
	// The pixels the gather skipped (a = 0) or that have no light nearby (a = GI_EMPTY, see
	// GTAOCommon.h) are left as they are
	if (center.a < 0.9)
		return center;

	const uint4 g = GI_GEOM(px);
	const float z = f16tof32(g.z);
	const float3 n = UnpackGeomNormal(g);
	const float tolerance = max(z * gtao_denoise_depth, 1e-3);
	const int radius = (int)gtao_blur_radius;
	const float invTwoSigma2 = 1.0 / (2.0 * max(0.25 * radius * radius, 0.25));
	const int2 dir = int2(gtao_blur_dir);

	float3 sum = 0;
	float weightSum = 0;
	[loop]
	for (int t = -radius; t <= radius; t++)
	{
		const int2 p = clamp(px + dir * t, int2(0, 0), maxPx);
		const float4 value = GI_LOAD(p);
		if (value.a < 0.5)
			continue;
		const uint4 sg = GI_GEOM(p);
		const float sz = f16tof32(sg.z);
		const float3 sn = UnpackGeomNormal(sg);
		float w = exp(-t * t * invTwoSigma2);
		w *= saturate(1.0 - abs(sz - z) / tolerance);
		const float facing = saturate(dot(n, sn));
		w *= facing * facing * facing * facing;
		sum += w * value.rgb;
		weightSum += w;
	}
	// The center always counts (weight 1), so weightSum > 0
	return float4(sum / weightSum, 1.0);
}
