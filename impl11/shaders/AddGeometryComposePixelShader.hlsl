// Copyright Leo Reyes, 2019.
// Licensed under the MIT license. See LICENSE.txt
// This shader draws the cockpit on top of the blurred background during
// post-hyper-exit
#include "HSV.h"
#include "ShaderToyDefs.h"
#include "ShadertoyCBuffer.h"

// The offscreenBuffer up to this point
Texture2DArray colorTex     : register(t0);
SamplerState   colorSampler : register(s0);

// The effects (trails, additional geometry, etc) buffer up to this point
Texture2DArray effectTex     : register(t1);
SamplerState   effectSampler : register(s1);

struct PixelShaderInput
{
	float4 pos  : SV_POSITION;
	float2 uv   : TEXCOORD;
	uint viewId : SV_RenderTargetArrayIndex;
};

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;

	float4 texColor    = colorTex.Sample(colorSampler, float3(input.uv,input.viewId));
	float4 effectColor = effectTex.Sample(effectSampler, float3(input.uv, input.viewId));

	// Initialize the result:
	output.color = texColor;

	output.color.rgb = lerp(texColor.rgb, effectColor.rgb, effectColor.a);
	output.color.a = 1.0;
	return output;
}
