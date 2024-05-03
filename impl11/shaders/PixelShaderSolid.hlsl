// Copyright (c) 2014 J�r�my Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes, 2019
#include "shader_common.h"
#include "PixelShaderTextureCommon.h"

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

struct PixelShaderInput
{
	float4 pos   : SV_POSITION;
	float4 color : COLOR0;
	float2 tex   : TEXCOORD0;
	float3 pos3D : TEXCOORD1;
};

struct PixelShaderOutput
{
	float4 color		: SV_TARGET0;
	float4 bloom		: SV_TARGET1;
	float4 pos3D		: SV_TARGET2;
	float4 normal	: SV_TARGET3;
	float4 ssaoMask : SV_TARGET4;
	float4 ssMask   : SV_TARGET5;
};

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	bool IsShadow   = (special_control & SPECIAL_CONTROL_EXCLUSIVE_MASK) == SPECIAL_CONTROL_XWA_SHADOW;

	// Using an alpha of 0.25 for the shadow causes overlapping shadows to show, which isn't great
	// since shadows don't behave that way.
	//output.color	= IsShadow ? float4(0,0,0, 0.25) : input.color;
	output.color    = IsShadow ? float4(0, 0, 0, input.color.a) : input.color;
	output.bloom		= IsShadow ? 0.0 : float4(fBloomStrength * input.color.rgb, fBloomStrength);
	output.pos3D		= 0;
	output.normal	= 0;
	output.ssaoMask = IsShadow ? 0.0 : float4(1.0, 0.0, 0.0, 1.0);
	output.ssMask   = 0;
	return output;
}
