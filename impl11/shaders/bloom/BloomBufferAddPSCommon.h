//#include "BloomCommon.fxh"
// https://github.com/martymcmodding/qUINT/blob/master/Shaders/qUINT_bloom.fx

// Based on the implementation from:
// https://learnopengl.com/Advanced-Lighting/Bloom

#ifdef INSTANCED_RENDERING
// This texture slot should be the source bloom buffer
Texture2DArray bloomTex0 : register(t0);
SamplerState   sampler0 : register(s0);

// This texture slot should be a copy of the accummulator
Texture2DArray bloomSumTex : register(t1);
SamplerState   bloomSumSampler : register(s1);
#else
// This texture slot should be the source bloom buffer
Texture2D    bloomTex0 : register(t0);
SamplerState sampler0 : register(s0);

// This texture slot should be a copy of the accummulator
Texture2D    bloomSumTex : register(t1);
SamplerState bloomSumSampler : register(s1);
#endif

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
	float2 input_uv_sub = input.uv * amplifyFactor;
#ifdef INSTANCED_RENDERING
	float3 bloom = bloomTex0.SampleLevel(sampler0, float3(input_uv_sub, input.viewId), 0).rgb;
	float3 bloomSum = bloomSumTex.SampleLevel(bloomSumSampler, float3(input.uv, input.viewId), 0).rgb;
#else
	float3 bloom = bloomTex0.SampleLevel(sampler0, input_uv_sub, 0).rgb;
	float3 bloomSum = bloomSumTex.SampleLevel(bloomSumSampler, input.uv, 0).rgb;
#endif

	// Truncate negative values coming from the bloom texture:
	return float4(bloomSum.rgb + max(0, bloomStrength * bloom.rgb), 1);
}