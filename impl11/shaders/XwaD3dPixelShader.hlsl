#include "XwaD3dCommon.hlsl"
#include "HSV.h"
#include "shader_common.h"
#include "shading_system.h"
#include "PixelShaderTextureCommon.h"
#include "NormalMapping.h"

Texture2D texture0 : register(t0); // This is the regular color texture
Texture2D texture1 : register(t1); // If present, this is the light texture
SamplerState sampler0 : register(s0);
// Normal Map, slot 13
Texture2D normalMap : register(t13);

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float4 pos3D  : COLOR1;
	float4 normal : NORMAL;
	float2 tex	  : TEXCOORD;
};

struct PixelShaderOutput
{
	float4 color      : SV_TARGET0;
	float4 bloom      : SV_TARGET1;
	float4 pos3D      : SV_TARGET2;
	float4 normal     : SV_TARGET3;
	float4 ssaoMask   : SV_TARGET4;
	float4 ssMask     : SV_TARGET5;
	float4 transp1Lyr : SV_TARGET6;
};

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	// This is the per-vertex Gouraud-shaded color coming from the VS:
	//float4 color = float4(input.color.xyz, 1.0f);
	float4 texelColor = texture0.Sample(sampler0, input.tex);

	const uint bIsBlastMark = special_control & SPECIAL_CONTROL_BLAST_MARK;
	const uint ExclusiveMask = special_control & SPECIAL_CONTROL_EXCLUSIVE_MASK;
	if (bIsBlastMark)
		texelColor = texture0.Sample(sampler0, (input.tex * 0.35) + 0.3);

	const float alpha = texelColor.w;
	float3 P = input.pos3D.xyz;
	float SSAOAlpha = saturate(min(alpha - fSSAOAlphaOfs, fPosNormalAlpha));
	
	// Zero-out the bloom mask and provide default output values
	output.bloom = 0;
	output.color = float4(brightness * texelColor.xyz, texelColor.w);
	output.pos3D = float4(P, SSAOAlpha);
	output.ssMask = 0;
	output.transp1Lyr = 0;

	if (ExclusiveMask == SPECIAL_CONTROL_GRAYSCALE && alpha >= 0.95)
		output.color.rgb = float3(0.7, 0.7, 0.7);

	if (bIsShadeless && bIsTransparent &&
		renderType != 2) // Do not process lasers (they are also transparent and shadeless!)
	{
		// Shadeless and transparent texture, this is cockpit glass or similar, we don't need to
		// worry about setting fAmbient because this draw call will be rendered on transpBuffer1
		// ssaoMask: Material, Glossiness, Specular Intensity
		output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, alpha);
		//output.color.a = sqrt(output.color.a); // Gamma correction (approx)
		output.normal = 0;
		// Cockpit glass gets rendered to transp layer 2, so we don't need to write
		// anything to transp1Lyr.
		return output;
	}

	float3 N = normalize(input.normal.xyz);
	N.yz = -N.yz; // Invert the Y axis, originally Y+ is down

	if (bDoNormalMapping)
	{
		float3 normalMapColor = normalMap.Sample(sampler0, input.tex).rgb;

		// Deprecated: this path used the tangent buffers to compute the TBN matrix.
		/*
		T = normalize(input.tangent.xyz);
		T.yz = -T.yz; // Replicate the same transforms we're applying to the normal N
		B = cross(T, N);
		const float3x3 TBN = float3x3(T, B, N);
		*/

		const float3x3 TBN = cotangent_frame(N, P, input.tex);
		const float3 NM = normalize(mul((normalMapColor * 2.0) - 1.0, TBN));
		N = lerp(N, NM, fNMIntensity);
	}
	output.normal = float4(N, SSAOAlpha);

	// ssaoMask: Material, Glossiness, Specular Intensity
	output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, alpha);
	// SS Mask: unused (Normal Mapping Intensity), Specular Value, Shadeless
	output.ssMask = float4(0, fSpecVal, max(fAmbient, bIsShadeless), alpha);

	// renderType == 2 is for lasers and missiles (and probably other ordinance as well)
	// Here we make an exception for missiles as they are handled separately
	if (renderType == 2 && ExclusiveMask != SPECIAL_CONTROL_MISSILE)
	{
		// Do not write the 3D position
		output.pos3D.a = 0;
		// Do not write the normal
		output.normal.a = 0;
		// Lasers are now rendered to a separate shadeless layer, so we don't need to
		// write any material properties:
		output.ssaoMask = 0;
		output.ssMask = 0;

		// This is a laser texture, process the bloom mask accordingly
		float3 HSV = RGBtoHSV(texelColor.xyz);
		HSV.y *= 1.5;
		// Enhance the lasers in 32-bit mode
		// Increase the saturation and lightness
		HSV.z *= 2.0;
		float3 color = HSVtoRGB(HSV);
		output.color = float4(color, alpha);
		output.bloom = float4(fBloomStrength * color, alpha);
		return output;
	}

	// DEBUG: Remove textures to display shading only
	//output.color.rgb = 0.75;
	//return output;

	if (ExclusiveMask == SPECIAL_CONTROL_MISSILE)
	{
		output.color.a = alpha;
		output.normal.a = 0.5 * alpha;
		output.ssMask.ba = alpha; // Make it all shadeless
		// Missiles may have illumination maps, so the next block sometimes is executed too and
		// that adds bloom to the OPT part of the missile
	}

	/*
	if (rand2 > 50.0)
	{
		output.color = float4(1, 0, 0, 1);
	}
	*/

	// In the D3dRendererHook, lightmaps and regular textures are rendered on the same draw call.
	// Here's the case where a lightmap has been provided:
	if (renderTypeIllum == 1)
	{
		// We have a lightmap texture
		const float4 texelColorIllum = texture1.Sample(sampler0, input.tex);
		// The alpha for light textures is either 0 or >0.1, so we multiply by 10 to make it [0, 1]
		const float alpha = texelColorIllum.a * 5.0;
		float3 illumColor = texelColorIllum.rgb;
		// This is a light texture, process the bloom mask accordingly
		float3 HSV = RGBtoHSV(illumColor);
		float val = HSV.z;
		// Make the light textures brighter in 32-bit mode
		HSV.z *= 1.25;

		illumColor = HSVtoRGB(HSV);

		const float valFactor = bIsTransparent ? 1.0 : smoothstep(0.75, 0.85, val);
		// The first term below uses "val" (lightness) to apply bloom on light areas. That's how we avoid
		// applying bloom on dark areas.
		//const float bloom_alpha = valFactor * smoothstep(0.45, 0.55, alpha);
		const float bloom_alpha = valFactor * smoothstep(0.2, 0.90, alpha);
		// Apply the bloom strength to this lightmap
		output.bloom = float4(fBloomStrength * bloom_alpha * val * illumColor, bloom_alpha);
		// Write an emissive material where there's bloom:
		output.ssaoMask.r = lerp(output.ssaoMask.r, 0, bloom_alpha);
		//output.ssMask.ba  = bloom_alpha; // Shadeless area
		output.ssMask.b = lerp(output.ssMask.b, 1, bloom_alpha); // Shadeless area
		// Replace the current color with the lightmap color, where appropriate:
		output.color.rgb = lerp(output.color.rgb, illumColor, bloom_alpha);

		if (bInHyperspace && output.bloom.a < 0.5)
			discard;
		return output;
	}

	// The HUD is shadeless and has transparency. Some planets in the background are also 
	// transparent (CHECK IF Jeremy's latest hooks fixed this) 
	// So glass is a non-shadeless surface with transparency:
	if (!bIsShadeless  &&
	    bIsTransparent &&
	    !bIsBlastMark) // Blast marks have alpha but aren't glass. See Direct3DDevice.cpp, search for "special_control.bBlastMark = 1;"
	{
		// Change the material and do max glossiness and spec_intensity
		//output.ssaoMask = float4(0, 1, 1, 1);
		// If alpha < 0.95, then we have a glassy surface, so let's do a smooth threshold for
		// the transition:
		// glassThreshold: 0 --> glass
		// glassThreshold: 1 --> opaque
		const float glassThreshold = smoothstep(0.95, 1.0, alpha);
		// We can make glass a bit more transparent. This might help external glass to be less
		// dark:
		const float adjustedGlassAlpha = alpha * alpha;
		output.color.a = lerp(adjustedGlassAlpha, alpha, glassThreshold);
		output.ssaoMask = float4(lerp(float3(0, 1, 1), output.ssaoMask.rgb, glassThreshold), 1);

		// Also write the normals of this surface over the current background
		// 0.5 is what we get from hook_normals for a solid normal, but here
		// we need to use 1.0, or the normal will be blended with the current
		// background... go figure!
		output.normal.a = 1.0;

		float3 glass = float3(1.0,  // Glass material
		                      1.0,  // White specular value
		                      0.8); // Almost shadeless (we're doing this to make the inside of cockpits easier to see
		                      //output.ssMask.b); // Keep the current ambient setting
		output.ssMask = float4(lerp(glass, output.ssMask.rgb, glassThreshold), 1);
		// The bloom should only be pass-through on transparent areas:
		output.bloom = lerp(output.bloom, float4(0, 0, 0, alpha), glassThreshold);
		// Render transparent areas to transp1Lyr, solid areas still go to _offscreenBuffer
		output.transp1Lyr = lerp(output.color, 0, glassThreshold);
	}

	return output;
}
