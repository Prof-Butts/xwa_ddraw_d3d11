// Copyright (c) 2014 J�r�my Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

Texture2DArray texture0 : register(t0);
SamplerState sampler0 : register(s0);

// MainShadersCBuffer
cbuffer ConstantBuffer : register(b0)
{
	float scale, aspect_ratio, parallax, brightness;
	float use_3D, inv_scale, unused1, unused2;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	// Resize the mirror window about the center of the image so that we can 
	// hide some of the ugly distortion we see near  the edges due to the wide
	// FOV used in VR:
	float3 uv;
	uv.xy = (input.tex - 0.5) * inv_scale + 0.5;
	// The third component is the array index. We choose the left eye.
	uv.z = 0;
	float4 texelColor = texture0.Sample(sampler0, uv, 0);
	//float2 D = abs(input.tex - 0.5);
	//if (any(D > crop_amount))
	//	texelColor.b += 0.7;

	return float4(brightness * texelColor.xyz, 1.0f);
}
