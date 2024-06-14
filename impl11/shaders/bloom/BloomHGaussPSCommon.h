#include "BloomCommon.fxh"

// Based on the implementation from:
// https://learnopengl.com/Advanced-Lighting/Bloom
// Also look here:
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// Kernel calculator:
// http://dev.theomader.com/gaussian-kernel-calculator/

#ifdef INSTANCED_RENDERING
Texture2DArray texture0 : register(t0);
#else
Texture2D      texture0 : register(t0);
#endif
SamplerState   sampler0 : register(s0);

cbuffer ConstantBuffer : register(b2)
{
	float pixelSizeX, pixelSizeY, unused1, amplifyFactor;
	// 16 bytes
	float bloomStrength, uvStepSize, saturationStrength, unused2;
	// 32 bytes
};

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
	float2 input_uv = input.uv * amplifyFactor;
#ifdef INSTANCED_RENDERING
	float3 color = texture0.SampleLevel(sampler0, float3(input_uv, input.viewId), 0).xyz * weight[0];
#else
	float3 color = texture0.SampleLevel(sampler0, input_uv, 0).xyz * weight[0];
#endif
	float3 s1, s2;
	float2 dx = uvStepSize * float2(pixelSizeX, 0);
	float2 uv1 = input_uv + dx;
	float2 uv2 = input_uv - dx;

	[unroll]
	for (int i = 1; i < 3; i++) {
#ifdef INSTANCED_RENDERING
		s1 = texture0.SampleLevel(sampler0, float3(uv1,input.viewId), 0).xyz * weight[i];
		s2 = texture0.SampleLevel(sampler0, float3(uv2,input.viewId), 0).xyz * weight[i];
#else
		s1 = texture0.SampleLevel(sampler0, uv1, 0).xyz * weight[i];
		s2 = texture0.SampleLevel(sampler0, uv2, 0).xyz * weight[i];
#endif
		color += s1 + s2;
		uv1 += dx;
		uv2 -= dx;
	}
	return float4(color, 1);
}