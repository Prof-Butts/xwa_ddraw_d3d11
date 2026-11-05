#include "BloomCommon.fxh"
#include "BloomCBuffer.h"

// Based on the implementation from:
// https://learnopengl.com/Advanced-Lighting/Bloom
// Also look here:
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// Kernel calculator:
// http://dev.theomader.com/gaussian-kernel-calculator/

#ifdef INSTANCED_RENDERING
Texture2DArray texture0 : register(t0);
#else
Texture2D    texture0 : register(t0);
#endif
SamplerState sampler0 : register(s0);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint viewId : SV_RenderTargetArrayIndex;
#endif
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 input_uv = input.uv * uvScale;
#ifdef INSTANCED_RENDERING
	float3 color = texture0.Sample(sampler0, float3(clamp(input_uv, uvMin, uvMax), input.viewId)).xyz * weight[0];
#else
	float3 color = texture0.Sample(sampler0, clamp(input_uv, uvMin, uvMax)).xyz * weight[0];
#endif
	float3 s1, s2;
	float2 dx = uvStepSize * float2(pixelSizeX, 0);
	float2 uv1 = input_uv + dx;
	float2 uv2 = input_uv - dx;

	[unroll]
	for (int i = 1; i < 3; i++) {
#ifdef INSTANCED_RENDERING
		s1 = texture0.Sample(sampler0, float3(clamp(uv1, uvMin, uvMax), input.viewId)).xyz * weight[i];
		s2 = texture0.Sample(sampler0, float3(clamp(uv2, uvMin, uvMax), input.viewId)).xyz * weight[i];
#else
		s1 = texture0.Sample(sampler0, clamp(uv1, uvMin, uvMax)).xyz * weight[i];
		s2 = texture0.Sample(sampler0, clamp(uv2, uvMin, uvMax)).xyz * weight[i];
#endif
		color += s1 + s2;
		uv1 += dx;
		uv2 -= dx;
	}
	return float4(color, 1);
}
