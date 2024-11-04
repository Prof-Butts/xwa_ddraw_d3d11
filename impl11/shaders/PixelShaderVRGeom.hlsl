// Copyright (c) 2024 Leo Reyes
// Licensed under the MIT license. See LICENSE.txt
// This shader is currently used to render the VR keyboard and gloves.
#include "shader_common.h"
#include "shading_system.h"
#include "PixelShaderTextureCommon.h"

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

Texture2D texture1 : register(t1);

TextureCube skybox : register(t21);

// VRGeometryCBuffer
cbuffer ConstantBuffer : register(b11)
{
	uint   numStickyRegions;
	uint2  clicked;
	uint   bRenderBracket;
	// 16 bytes
	float4 stickyRegions[4];
	// 80 bytes
	float4 clickRegions[2];
	// 112 bytes
	float  strokeWidth;
	float3 bracketColor;
	// 128 bytes
	//float4 U;
	uint   renderText;
	uint   isSubComponent;
	uint2  vrg_unused0;
	// 144 bytes
	matrix viewMat;
	// 208 bytes
	float4 F;
	// 224 bytes
};

// New PixelShaderInput needed for the D3DRendererHook
struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float4 pos3D  : COLOR1;
	float4 normal : NORMAL;
	float2 tex    : TEXCOORD;
};

struct PixelShaderOutput
{
	float4 color    : SV_TARGET0;
	float4 bloom    : SV_TARGET1;
	float4 pos3D    : SV_TARGET2;
	float4 normal   : SV_TARGET3;
	float4 ssaoMask : SV_TARGET4;
	float4 ssMask   : SV_TARGET5;
};

// Noise from https://www.shadertoy.com/view/4sfGzS
float hash(float3 p)  // replace this by something better
{
	p = frac(p * 0.3183099 + .1);
	p *= 17.0;
	return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(in float3 x)
{
	float3 i = floor(x);
	float3 f = frac(x);
	f = f * f * (3.0 - 2.0 * f);

	return lerp(lerp(lerp(hash(i + float3(0, 0, 0)),
		hash(i + float3(1, 0, 0)), f.x),
		lerp(hash(i + float3(0, 1, 0)),
			hash(i + float3(1, 1, 0)), f.x), f.y),
		lerp(lerp(hash(i + float3(0, 0, 1)),
			hash(i + float3(1, 0, 1)), f.x),
			lerp(hash(i + float3(0, 1, 1)),
				hash(i + float3(1, 1, 1)), f.x), f.y), f.z);
}

PixelShaderOutput RenderBracket(PixelShaderInput input)
{
	PixelShaderOutput output;

	float alpha =
		(input.tex.x <= strokeWidth) || (input.tex.x >= 1.0 - strokeWidth) || // Left and right bars
		(input.tex.y <= strokeWidth) || (input.tex.y >= 1.0 - strokeWidth) ?  // Top and bottom bars
		1 : 0;

	if (renderText)
	{
		//float2 uv = 0.5 * (float2(1, -1) * (input.tex - 0.5)) + 0.5;
		float2 uv = 1.0 * (float2(1, -1) * (input.tex - 0.5)) + 0.5;

		float4 textCol  = float4(texture1.Sample(sampler0, uv));
		textCol.a      += 0.5;
		output.color    = textCol;
		output.bloom    = float4(0.5 * output.color.rgb, textCol.a);
		output.pos3D    = 0;
		output.normal   = 0;
		output.ssMask   = 0;
		output.ssaoMask = float4(fSSAOMaskVal, 0.1, 0, textCol.a);
		return output;
	}

	if (alpha < 0.7)
		discard;

	// gapSize prevents the gap from being greater than the size of the stroke. In other words
	// if the bracket is so small that the stroke will create a closed bracket, then let's leave
	// a closed bracket.
	// Create the gaps in the bracket:
	const float gapSize = max(strokeWidth, 0.2);
	if ((input.tex.x >= gapSize && input.tex.x <= 1.0 - gapSize) ||
	    (input.tex.y >= gapSize && input.tex.y <= 1.0 - gapSize))
		discard;

	output.color    = float4(bracketColor, alpha);
	output.bloom    = float4(0.5 * output.color.rgb, alpha);
	output.pos3D    = 0;
	output.normal   = 0;
	output.ssMask   = 0;
	output.ssaoMask = float4(fSSAOMaskVal, 0.1, 0, alpha);
	return output;
}

PixelShaderOutput RenderSkyBox(PixelShaderInput input)
{
	PixelShaderOutput output;

	output.color    = 0;
	output.bloom    = 0;
	output.ssaoMask = 0;
	output.ssMask   = 0;
	output.pos3D    = 0;
	output.normal   = 0;

	const float3 P = input.pos3D.xyz;
#ifndef DISABLED
	const float4 V = mul(viewMat, float4(normalize(P), 0));
	output.color = float4(skybox.Sample(sampler0, V.xyz).rgb, 1);
#else
	float4 V = float4(normalize(P), 0);
	const float Vdist = max(0, dot(V.xyz, U.xyz));
	const float Fdist = max(0, dot(V.xyz, F.xyz));
	output.color = float4(0.2, 0.2, 0.4, 0.3);

	output.color.r += 0.5 * Vdist;
	output.color.a += pow(Vdist, 4.0);

	output.color.g += 0.5 * Fdist;
	output.color.a += pow(Fdist, 4.0);
#endif

	return output;
}

PixelShaderOutput RenderSkyCylinder(PixelShaderInput input)
{
	PixelShaderOutput output;

	const float4 texelColor = texture0.Sample(sampler0, input.tex);

	output.color    = texelColor;
	output.bloom    = 0;
	output.ssaoMask = 0;
	output.ssMask   = 0;
	output.pos3D    = 0;
	output.normal   = 0;

	//output.color = float4(input.tex, 1, 1);
	output.color.b += 0.1;
	return output;
}

PixelShaderOutput main(PixelShaderInput input)
{
	if (bIsShadeless == 2)
	{
		return RenderSkyBox(input);
	}

	if (bIsShadeless == 3)
	{
		return RenderSkyCylinder(input);
	}

	if (bRenderBracket)
		return RenderBracket(input);

	PixelShaderOutput output;
	const float4 texelColor = texture0.Sample(sampler0, input.tex);
	const float  alpha      = texelColor.w;
	const float2 uv         = input.tex;

	if (alpha < 0.75)
		discard;

	output.color = texelColor;
	// Zero-out the bloom mask.
	output.bloom = float4(0, 0, 0, 0);

	uint i;
	for (i = 0; i < numStickyRegions; i++)
	{
		const float4 region = stickyRegions[i];
		if (uv.x >= region.x && uv.x <= region.z &&
		    uv.y >= region.y && uv.y <= region.w)
		{
			output.color.rgb = 1 - output.color.rgb;
			output.bloom.rgb = output.color.rgb;
			output.bloom.a   = 0.75;
		}
	}

	for (i = 0; i < 2; i++)
	{
		if (clicked[i])
		{
			const float4 region = clickRegions[i];
			if (uv.x >= region.x && uv.x <= region.z &&
			    uv.y >= region.y && uv.y <= region.w)
			{
				output.color.rgb = 1 - output.color.rgb;
				output.bloom.rgb = output.color.rgb;
				output.bloom.a   = 0.75;
			}
		}
	}

	//output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, alpha);
	output.ssaoMask = float4(fSSAOMaskVal, 0.1, 0, alpha);

	// SS Mask: Normal Mapping Intensity (overriden), Specular Value, Shadeless
	output.ssMask = float4(0, 0, bIsShadeless, alpha);

	// Black-noise fade in/out -- this part will be played when turning the VR keyboard
	// on and off.
	const float fadeIn = rand0 - 1.0; // fadeIn is in the range 1..2
	if (rand0 > 0) // The effect is only applied if rand0 is not zero
	{
		const float size = 10.0 + 30.0 * fadeIn;
		float speckle_noise = 3.0 * (noise(size * float3(input.tex, 20.0 * fadeIn)) - 0.5);
		float4 nvideo = output.color * lerp(speckle_noise, 1.0, fadeIn);
		output.color = lerp(0.0, nvideo, clamp(fadeIn * 3.0, 0.0, 1.0));
		output.bloom = lerp(0.0, 2.0 * nvideo, clamp(fadeIn, 0.0, 1.0));
		output.ssMask.a = min(output.ssMask.a, output.color.a);
	}

	if (bIsShadeless)
	{
		// If we set "normal" to 0, then it will disappear when blending the
		// offscreen with the background (the background has normal = 0). So
		// we need to set a solid (i.e. non-transparent) normal, but still
		// keep it as a null normal.
		output.normal = float4(0, 0, 0, 0.5);
		output.pos3D  = 0;
		return output;
	}

	float3 P = input.pos3D.xyz;
	output.pos3D = float4(P, 1);

	float3 N = normalize(input.normal.xyz);
	N.yz = -N.yz; // Invert the Y axis, originally Y+ is down
	// The hook_normals hook sets alpha to 0.5, so we must follow suit here.
	// If I set the alpha to 1, I'll get a white halo around the VR gloves!
	output.normal = float4(N, 0.5);

	return output;
}
