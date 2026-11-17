/*
 * The soft-shadow filter of the RT shadow mask (raytracing_enable_soft_shadows), at the
 * mask's resolution (1 / RTShadowMaskSizeFactor), for the new PBR shading. Licensed under the
 * MIT license. See LICENSE.txt
 *
 * The mask (RTShadowMaskPSCommon.h) has the shadow in x and the depth of the pixel it was
 * traced from in y. This is the same filter the lighting pass used to run at every pixel
 * (until commit 4462c1a4): a 5x5 Gaussian of the mask, 2 dilation steps
 * apart, where the taps on another surface (a depth more than RTSoftShadowThresholdMult * z
 * away) use the darkest value of the 3x3 texels around instead, so the shadows don't get a
 * lit halo at the edges of objects. At every screen pixel it took about 60 texture reads,
 * and the screen pixels of one mask texel all ran it on almost the same data; here it runs
 * once per texel, and the lighting pass upsamples the result (SoftShadowUpsample()).
 *
 * Output: x = the filtered shadow, y = the texel's depth (copied from the mask), for the
 * upsample's depth test.
 */
#include "RTCommon.h"

#ifdef INSTANCED_RENDERING
Texture2DArray rtShadowMask : register(t0);
#define MASK_LOAD(p) rtShadowMask.Load(int4(p, viewId, 0)).xy
#else
Texture2D rtShadowMask : register(t0);
#define MASK_LOAD(p) rtShadowMask.Load(int3(p, 0)).xy
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
	uint maskW, maskH;
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
	uint maskElems;
	rtShadowMask.GetDimensions(maskW, maskH, maskElems);
#else
	rtShadowMask.GetDimensions(maskW, maskH);
#endif
	const int2 maxPx = int2(maskW, maskH) - 1;
	const int2 px = int2(input.pos.xy);
	const float2 center = MASK_LOAD(px);
	const float z = center.y;

	// The spacing of the dilation's taps, in texels, and twice that for the filter's: the
	// same as the per-pixel filter (max(2, RTShadowMaskSizeFactor) screen pixels)
	const float sizeFactor = (float)RTShadowMaskSizeFactor;
	const int d = max(1, (int)round(max(2.0, sizeFactor) / sizeFactor));

	// The darkest value around, for the taps on other surfaces
	float rtMin = center.x;
	[unroll]
	for (int dy = -1; dy <= 1; dy++)
	{
		[unroll]
		for (int dx = -1; dx <= 1; dx++)
			rtMin = min(rtMin, MASK_LOAD(clamp(px + d * int2(dx, dy), int2(0, 0), maxPx)).x);
	}

	// The Gaussian weights, exp(-RTGaussFactor * (i^2 + j^2)), are powers of one value
	const float g1 = exp(-RTGaussFactor);
	const float g2 = g1 * g1;
	const float g4 = g2 * g2;
	// Objects far away should have bigger thresholds too
	const float threshold = RTSoftShadowThresholdMult * z;
	float rtVal = 0;
	float Gsum = 0;
	[unroll]
	for (int i = -2; i <= 2; i++)
	{
		[unroll]
		for (int j = -2; j <= 2; j++)
		{
			const float2 tap = MASK_LOAD(clamp(px + 2 * d * int2(j, i), int2(0, 0), maxPx));
			// i and j are constants here, so this picks one of the powers
			const int k = i * i + j * j;
			const float G = k == 0 ? 1.0 : (k == 1 ? g1 : (k == 2 ? g2 : (k == 4 ? g4 : (k == 5 ? g4 * g1 : g4 * g4))));
			rtVal += G * (abs(z - tap.y) < threshold ? tap.x : rtMin);
			Gsum += G;
		}
	}
	return float4(rtVal / Gsum, z, 0, 1);
}
