#include "BloomCommon.fxh"
#include "BloomCBuffer.h"
// https://github.com/martymcmodding/qUINT/blob/master/Shaders/qUINT_bloom.fx

// Based on the implementation from:
// https://learnopengl.com/Advanced-Lighting/Bloom

#ifdef INSTANCED_RENDERING
// This texture slot should be the original backbuffer SRV
Texture2DArray texture0 : register(t0);
SamplerState sampler0 : register(s0);

// These texture slots should be the bloom levels, starting at t1
Texture2DArray bloomLevels[MAX_BLOOM_LEVELS] : register(t1);
SamplerState bloomSampler : register(s1);
#else
// This texture slot should be the original backbuffer SRV
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

// These texture slots should be the bloom levels, starting at t1
Texture2D bloomLevels[MAX_BLOOM_LEVELS] : register(t1);
SamplerState bloomSampler : register(s1);
#endif

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint viewId : SV_RenderTargetArrayIndex;
#endif
};

// From http://www.chilliant.com/rgb2hsv.html
static float Epsilon = 1e-10;

float3 RGBtoHCV(in float3 RGB)
{
	// Based on work by Sam Hocevar and Emil Persson
	float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0 / 3.0) : float4(RGB.gb, 0.0, -1.0 / 3.0);
	float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
	float C = Q.x - min(Q.w, Q.y);
	float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
	return float3(H, C, Q.x);
}

float3 HUEtoRGB(in float H)
{
	float R = abs(H * 6 - 3) - 1;
	float G = 2 - abs(H * 6 - 2);
	float B = 2 - abs(H * 6 - 4);
	return saturate(float3(R, G, B));
}

float3 RGBtoHSV(in float3 RGB)
{
	float3 HCV = RGBtoHCV(RGB);
	float S = HCV.y / (HCV.z + Epsilon);
	return float3(HCV.x, S, HCV.z);
}

float3 HSVtoRGB(in float3 HSV)
{
	float3 RGB = HUEtoRGB(HSV.x);
	return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

/*
float4 main_32_bit(PixelShaderInput input) : SV_TARGET
{
	float2 input_uv_sub = input.uv * amplifyFactor;
	float4 color = texture0.Sample(sampler0, input.uv);
	float4 bloom = float4(bloomTex.Sample(bloomSampler, input_uv_sub).xyz, 1);
	color.w = 1.0f;

	//return color + bloomStrength * bloom;
	// Screen blending mode, see http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
	// 1 - (1 - Target) * (1 - Blend)

	float3 HSV = RGBtoHSV(bloom.xyz);
	float V = bloomStrength * HSV.z;
	V = V / (V + 1);
	HSV.z = V;
	// Increase the saturation only in bright areas (otherwise halos are created):
	HSV.y = lerp(HSV.y, HSV.y * saturationStrength, HSV.z);
	bloom.rgb = HSVtoRGB(HSV);

	color.rgb = 1 - (1 - color.rgb) * (1 - bloom.rgb);
	return color;
}
*/

/*
float4 main_float_1(PixelShaderInput input) : SV_TARGET
{
	float2 input_uv_sub = input.uv * amplifyFactor;
	float4 color = texture0.Sample(sampler0, input.uv);
	//float4 bloom = float4(bloomTex.Sample(bloomSampler, input_uv_sub).xyz, 1);
	float4 bloom = bloomTex.Sample(bloomSampler, input_uv_sub);
	color.w = 1.0f;

	float3 HSV = RGBtoHSV(bloom.xyz);
	float V = bloomStrength * HSV.z;
	V = V / (V + 1);
	HSV.z = V;
	// Increase the saturation only in bright areas (otherwise halos are created):
	HSV.y = lerp(HSV.y, HSV.y * saturationStrength, HSV.z);
	bloom.rgb = HSVtoRGB(HSV);
	bloom = saturate(bloom);

	// Screen blending mode, see http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
	// 1 - (1 - Target) * (1 - Blend)

	color.rgb = 1 - (1 - color.rgb) * (1 - bloom.rgb);
	return color;
}
*/

/*
struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
	float4 bloom : SV_TARGET1;
};
*/

float4 main(PixelShaderInput input) : SV_TARGET
{
#ifdef INSTANCED_RENDERING
	float4 color = texture0.Sample(sampler0, float3(input.uv, input.viewId));
#else
	float4 color = texture0.Sample(sampler0, input.uv);
#endif
	color.w = 1.0f;

	// Add up all the bloom levels. Each level covers the screen, but its texture is
	// rounded up in size, so levelUV maps the screen UVs to the area it covers.
	float3 bloom = 0;
	[unroll]
	for (int i = 0; i < MAX_BLOOM_LEVELS; i++) {
		if (i < numLevels) {
			// Clamping to half a texel replicates the edge on the left and top sides. The
			// bloom sampler returns zero past the right and bottom sides.
			float2 uv = max(input.uv * levelUV[i].xy, levelUV[i].zw);
#ifdef INSTANCED_RENDERING
			float3 level = bloomLevels[i].SampleLevel(bloomSampler, float3(uv, input.viewId), 0).rgb;
#else
			float3 level = bloomLevels[i].SampleLevel(bloomSampler, uv, 0).rgb;
#endif
			// Truncate negative values coming from the bloom levels
			bloom += max(0, levelStrength[i / 4][i % 4] * level);
		}
	}

	//float3 HSV = RGBtoHSV(output.bloom.xyz);
	//float V = HSV.z;
	//V = V / (V + 1);
	//HSV.z = V;
	//// Increase the saturation only in bright areas (otherwise halos are created):
	//HSV.y = lerp(HSV.y, HSV.y * saturationStrength, HSV.z);
	//output.bloom.rgb = HSVtoRGB(HSV);

	bloom = bloom / (bloom + 1);
	float3 HSV = RGBtoHSV(bloom);
	HSV.y = saturate(lerp(HSV.y, HSV.y * saturationStrength, HSV.z));
	//bloom = general_bloom_strength * saturate(HSVtoRGB(HSV));
	bloom = saturate(HSVtoRGB(HSV));

	/*color.xyz = 1 - (1 - color.xyz) * (1 - bloomStrength * bloom.xyz);
	color.rgb = pow(max(0, color.rgb), BLOOM_TONEMAP_COMPRESSION);
	color.rgb = color.rgb / (1.0 + color.rgb);
	color.rgb = pow(color.rgb, 1.0 / BLOOM_TONEMAP_COMPRESSION);*/

	// Screen blending mode, see http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
	// 1 - (1 - Target) * (1 - Blend)
	color.rgb = 1 - (1 - color.rgb) * (1 - bloom);
	return color;
}