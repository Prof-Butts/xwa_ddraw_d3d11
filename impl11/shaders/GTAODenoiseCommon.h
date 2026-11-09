/*
 * Denoise filter for GTAO (GTAOCommon.h). Licensed under the MIT license. See LICENSE.txt
 *
 * The GTAO pass offsets its samples with a pattern that repeats every 4x4 pixels, so the
 * average of any 4x4 block uses all 16 offsets. This pass averages the 4x4 block around
 * each pixel, skipping the pixels at a different depth, so the occlusion doesn't bleed
 * across the edges of objects.
 *
 * Input: t0 = GTAO buffer (bent normal, AO), t1 = position buffer. Output: the same as the
 * GTAO buffer.
 */
#include "GTAOCBuffer.h"

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
	const int2 px = int2(input.pos.xy);
	const int2 maxPx = int2(gtao_buffer_size) - 1;
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
	const float4 center = texGTAO.Load(int4(px, viewId, 0));
	const float z = texPos.Load(int4(px, viewId, 0)).z;
#else
	const float4 center = texGTAO.Load(int3(px, 0));
	const float z = texPos.Load(int3(px, 0)).z;
#endif
	// The background and the pixels that GTAO skipped are left as they are
	if (center.a >= 1.0)
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
			const float sz = texPos.Load(int4(p, viewId, 0)).z;
#else
			const float4 value = texGTAO.Load(int3(p, 0));
			const float sz = texPos.Load(int3(p, 0)).z;
#endif
			const float w = saturate(1.0 - abs(sz - z) / tolerance);
			sum += value * w;
			weightSum += w;
		}
	}
	// The center pixel always has weight 1, so weightSum > 0
	float4 result = sum / weightSum;
	result.xyz = length(result.xyz) > 1e-4 ? normalize(result.xyz) : center.xyz;
	return result;
}
