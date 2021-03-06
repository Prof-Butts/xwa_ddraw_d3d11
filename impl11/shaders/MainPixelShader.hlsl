// Copyright (c) 2014 J�r�my Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer ConstantBuffer : register(b0)
{
	float scale;
	float aspect_ratio;
	float parallax;
	float brightness; // Only one used in this PixelShader
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 texelColor = texture0.Sample(sampler0, input.tex);

	// In Win7 we need to discard pixels or else we'll get black artifacts
	// in the Tech Library, the Pilot Proving Grounds and other places.
	// In Win10 we can remove the discard instruction and it will look fine;
	// but we need to keep the original semantics for backwards-compatibility.
	if (texelColor.a > 0.4f)
	{
		discard;
	}

	return float4(brightness * texelColor.xyz, 1.0f);
}
