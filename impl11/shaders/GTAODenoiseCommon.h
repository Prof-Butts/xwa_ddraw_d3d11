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
 * for the pixels the gather skipped), so it isn't normalized like a bent normal.
 */
#include "GTAOCBuffer.h"

#ifdef GTAO_GI_DENOISE
#define DENOISE_SKIPPED(value) ((value).a < 0.5)
#else
// The pixels that GTAO skipped have AO = 2
#define DENOISE_SKIPPED(value) ((value).a > 1.5)
#endif

#ifdef INSTANCED_RENDERING
Texture2DArray texGTAO : register(t0);
Texture2DArray texPos  : register(t1);
#else
Texture2D texGTAO : register(t0);
Texture2D texPos  : register(t1);
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
	const int scale = (int)gtao_scale;
	const int2 px = int2(input.pos.xy);
	// The part of the GTAO buffer that's used (rounded up at half resolution)
	const int2 maxPx = int2((uint2(gtao_buffer_size) + (uint)scale - 1) / (uint)scale) - 1;
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
	const float4 center = texGTAO.Load(int4(px, viewId, 0));
	const float z = texPos.Load(int4(px * scale, viewId, 0)).z;
#else
	const float4 center = texGTAO.Load(int3(px, 0));
	const float z = texPos.Load(int3(px * scale, 0)).z;
#endif
	// The background and the pixels that GTAO skipped are left as they are
	if (DENOISE_SKIPPED(center))
		return center;

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
			const float sz = texPos.Load(int4(p * scale, viewId, 0)).z;
#else
			const float4 value = texGTAO.Load(int3(p, 0));
			const float sz = texPos.Load(int3(p * scale, 0)).z;
#endif
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
