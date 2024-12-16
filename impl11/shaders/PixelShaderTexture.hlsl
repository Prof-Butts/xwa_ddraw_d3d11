// Copyright (c) 2014 J�r�my Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019
#include "shader_common.h"
#include "HSV.h"
#include "shading_system.h"
#include "PixelShaderTextureCommon.h"

Texture2D    texture0 : register(t0);
SamplerState sampler0 : register(s0);

// pos3D/Depth buffer has the following coords:
// X+: Right
// Y+: Up
// Z+: Away from the camera
// (0,0,0) is the camera center, (0,0,Z) is the center of the screen

//#define diffuse_intensity 0.95

//static float3 light_dir = float3(0.9, 1.0, -0.8);
//#define ambient 0.03
//static float3 ambient_col = float3(0.025, 0.025, 0.03);
//static float3 ambient_col = float3(0.10, 0.10, 0.15);

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float4 color  : COLOR0;
	float2 tex    : TEXCOORD0;
	float4 pos3D  : COLOR1;
	float4 normal : NORMAL;
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

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	float4 texelColor = texture0.Sample(sampler0, input.tex);
	uint   bIsBlastMark = special_control & SPECIAL_CONTROL_BLAST_MARK;
	if (bIsBlastMark) {
		//texelColor = texture0.Sample(sampler0, (input.tex * 0.5) + 0.25);
		texelColor = texture0.Sample(sampler0, (input.tex * 0.35) + 0.3);
		//texelColor = texture0.Sample(sampler0, (input.tex * 0.1) + 0.5);
	}
	float  alpha	  = texelColor.w;
	float3 diffuse    = lerp(input.color.xyz, 1.0, fDisableDiffuse);
	float3 P		  = input.pos3D.xyz;
	float  SSAOAlpha  = saturate(min(alpha - fSSAOAlphaOfs, fPosNormalAlpha));
	uint   ExclusiveMask = special_control & SPECIAL_CONTROL_EXCLUSIVE_MASK;
	// Zero-out the bloom mask.
	output.bloom  = 0;
	output.color  = texelColor;
	output.pos3D  = float4(P, SSAOAlpha);
	output.ssMask = 0;

	// DEBUG
	//output.normal = 0;
	//output.ssaoMask = 0;
	//output.color = float4(brightness * diffuse * texelColor.xyz, texelColor.w);
	//return output;
	// DEBUG

	// Original code:
	//float3 N = normalize(cross(ddx(P), ddy(P)));
	// Since Z increases away from the camera, the normals end up being negative when facing the
	// viewer, so let's mirror the Z axis:
	// Test:
	//N.z = -N.z;

	// hook_normals code:
	float3 N = normalize(input.normal.xyz);
	N.yz = -N.yz; // Invert the Y axis, originally Y+ is down
	// N *= input.normal.w; // Zero-out normals when w == 0 ?
	
	//if (N.z < 0.0) N.z = 0.0; // Avoid vectors pointing away from the view
	// Flipping N.z seems to have a bad effect on SSAO: flat unoccluded surfaces become shaded
	output.normal = float4(N, SSAOAlpha);
	
	// SSAO Mask/Material, Glossiness, Spec_Intensity
	// Glossiness is multiplied by 128 to compute the exponent
	//output.ssaoMask = float4(fSSAOMaskVal, DEFAULT_GLOSSINESS, DEFAULT_SPEC_INT, alpha);
	// ssaoMask.r: Material
	// ssaoMask.g: Glossiness
	// ssaoMask.b: Specular Intensity
	output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, alpha);
	// SS Mask: Normal Mapping Intensity, Specular Value, Shadeless
	output.ssMask = float4(0, fSpecVal, fAmbient, alpha);

	// Make GUI elements (reticle, triangle pointer, etc), explosions, debris, etc shadeless:
	if (bIsShadeless)
	{
		output.ssMask.ba = alpha; // Shadeless material
		output.normal = 0;
	}

	if (ExclusiveMask == SPECIAL_CONTROL_FIRE)
	{
		const float a   = 0.2 * alpha;
		output.color    = float4(texelColor.rgb, a);
		output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, a);
		output.ssMask   = float4(0, fSpecVal, 0.0, a);
		output.normal   = 0;
		output.bloom    = float4(0.5 * texelColor.rgb, a);
		return output;
	}

	if (ExclusiveMask == SPECIAL_CONTROL_ENGINE_GLOW_SMOKE)
	{
		//output.color = float4(brightness * diffuse * texelColor.xyz, texelColor.w);
		//const float a   = 0.1 * alpha;
		const float a   = 0.3 * alpha;
		output.color    = float4(input.color.xyz * texelColor.rgb, a);
		output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, a);
		output.ssMask   = float4(0, fSpecVal, 0.0, a);
		output.normal   = 0;
		output.bloom    = 0;
		return output;
	}

	if (ExclusiveMask == SPECIAL_CONTROL_MISSILE)
	{
		// I'm not quite sure, but I _think_ missiles used to be rendered as "engine glow".
		// The engine glow path below increases the value by 1.25, so let's do that here
		// to recover the original look.
		output.bloom = float4(1.25 * fBloomStrength * output.color.rgb, alpha);
	}

	if (ExclusiveMask == SPECIAL_CONTROL_EXPLOSION)
	{
		alpha = sqrt(alpha); // Gamma correction (approx)
		// Explosions are now rendered to the first transparent layer. We don't need to
		// worry about materials or normals anymore -- they're shadeless and blended in post
		output.ssaoMask = 0;
		output.ssMask   = 0;
		output.normal   = 0;
		output.pos3D    = 0;
		output.bloom    = float4(fBloomStrength * output.color.rgb, output.color.a);
		return output;
	}

	// Enhance the engine glow. In this texture, the diffuse component also provides
	// the hue. The engine glow is also used to render smoke, so that's why the smoke
	// glows.
	if (bIsEngineGlow) {
		// Disable depth-buffer and materials write for engine glow textures: they are now
		// rendered to their own separate transparent layer.
		output.pos3D    = 0;
		output.normal   = 0;
		output.ssaoMask = 0;
		output.ssMask   = 0;

		float3 color = texelColor.rgb * input.color.xyz;
		// This is an engine glow, process the bloom mask accordingly
		if (bIsEngineGlow > 1) {
			// Enhance the glow in 32-bit mode
			float3 HSV = RGBtoHSV(color);
			HSV.y *= 1.25;
			HSV.z *= 1.25;
			color = HSVtoRGB(HSV);
		} 
		output.color = float4(color, alpha);
		output.bloom = float4(fBloomStrength * output.color.rgb, alpha);

		// This block applies the Tech Room hologram effect to the engine glows
		if (rand1 > 0.5)
		{
			const float  luma = dot(output.color.rgb, float3(0.299, 0.587, 0.114));
			const float4 holo_col = 3.0 * float4(0.325, 0.541, 0.643, alpha);
			const float  scans = 0.7 + 0.3 * sin(rand0 + 0.45 * input.pos.y);
			const float4 mixed_color = lerp(output.color, luma * holo_col, 0.5f);
			output.color = mixed_color * scans;
		}

		return output;
	}

	// Original code:
	output.color = float4(brightness * diffuse * texelColor.xyz, texelColor.w);

	//if (ExclusiveMask == SPECIAL_CONTROL_BACKGROUND)
	//	output.color.r += 0.7;
	return output;
}
