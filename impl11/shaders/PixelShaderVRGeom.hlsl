// Copyright (c) 2019 Leo Reyes
// Licensed under the MIT license. See LICENSE.txt
// This shader is currently used to render the VR keyboard and gloves.
//#include "HSV.h"
#include "shader_common.h"
#include "shading_system.h"
#include "PixelShaderTextureCommon.h"

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

//Texture2D texture1 : register(t1);
//SamplerState sampler1 : register(s1);

// When the Dynamic Cockpit is active:
// texture0 == regular texture
// texture1 == ??? illumination texture?

// VRGeometryCBuffer
cbuffer ConstantBuffer : register(b11)
{
	uint  numStickyRegions;
	uint2 clicked;
	uint  vr_unused0;
	// 16 bytes
	float4 stickyRegions[4];
	// 80 bytes
	float4 clickRegions[2];
	// 112 bytes
	float4 U;
	// 128 bytes
	float4 R;
	// 144 bytes
	float4 F;
	// 160 bytes
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

float3 genericDistortion(float2 p, float2 LC, float k1, float k2, float k3)
{
	// Lens distortion value, positive values of k1 & k2 give barrel distortion, negative give pincushion.
	float2 q = p - LC;
	float r = length(q);
	float r2 = pow(r, 2.0f);

	float newRadius = (1.0f + r2 * k1) + (k2 * pow(r, 4.0f)) + (k3 * pow(r, 6.0f));
	return float3(newRadius * q + LC, r);
}

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;

	const float4 texelColor = texture0.Sample(sampler0, input.tex);
	const float  alpha      = texelColor.w;
	const float2 uv         = input.tex;
	//if (alpha < 0.75)
	//	discard;

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
			//output.color.r += 0.3;
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

	float3 P = input.pos3D.xyz;
	output.pos3D = float4(P, 1);

	float3 N = normalize(input.normal.xyz);
	N.y = -N.y; // Invert the Y axis, originally Y+ is down
	N.z = -N.z;
	output.normal = float4(N, 1);

	//output.ssaoMask = 0;
	//output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, alpha);
	output.ssaoMask = float4(fSSAOMaskVal, 0.1, 0, alpha);
	//output.diffuse = input.color;

	// SS Mask: Normal Mapping Intensity (overriden), Specular Value, Shadeless
	//output.ssMask = float4(fNMIntensity, fSpecVal, 0.0, 0.0);
	output.ssMask = float4(0, 0, 0, 0);

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

	//output.color.r  = 2.0 * input.tex.x;
	//output.color.b  = 2.0 * input.tex.y;
	//output.color.a += 0.25;

	output.color.a = 0;

	float3 V = normalize(input.pos3D.xyz);

	// Remove the pincushion effect
	float3 q = genericDistortion(V.xy, 0, 0.5, 0.5, 0);
	V.xy = q.xy;
	normalize(V);

	// Colorize the cap:
	//const float Vdist = dot(V, U.xyz);
	//output.color.rb += 0.5 * Vdist;
	//output.color.a = max(output.color.a, pow(Vdist, 4.0));

	float inc = 2.0 / 16;
	float p = -1.0;
	for (i = 0; i < 16; i++)
	{
		const float Hdist = abs(dot(V, U.xyz) - p) < 0.0025 ? 1 : 0;
		const float Rdist = abs(dot(V, R.xyz) - p) < 0.0025 ? 1 : 0;
		const float Fdist = abs(dot(V, F.xyz) - p) < 0.0025 ? 1 : 0;

		// Horizon lines appear as circles when ViewMatrix is set to identity
		//output.color.gb += 0.9 * Hdist;
		//output.color.a = max(output.color.a, pow(Hdist, 4.0));

		output.color.r += 0.9 * Rdist;
		output.color.a = max(output.color.a, pow(Rdist, 4.0));

		output.color.rg += 0.9 * Fdist;
		output.color.a = max(output.color.a, pow(Fdist, 4.0));

		p += inc;
	}

	if (output.color.a < 0.2)
		discard;
	return output;
}
