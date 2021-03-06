// Copyright (c) 2014 J�r�my Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR/Dynamic Cockpit by Leo Reyes, 2019.
// This shader is used to render the offscreen HUD FG and BG buffers

struct VertexShaderInput
{
	float4 pos : POSITION;
	float4 color : COLOR0;
	float4 specular : COLOR1;
	float2 tex : TEXCOORD;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD;
};

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;

	output.pos.xy = (input.pos.xy - 0.5) * float2(2.0, -2.0);
	output.pos.z = input.pos.z;
	output.pos.w = 1.0f;
	output.color = input.color.zyxw;
	output.tex   = input.tex;
	return output;
}
