/*
 * Original Pixel Shader: Jeremy Ansel, 2022.
 * Updated for XWA Effects by Leo Reyes 2022.
 * PBR shaders based on https://www.shadertoy.com/view/4l3SWf
 */
#include "XwaD3dCommon.hlsl"
#include "HSV.h"
#include "shader_common.h"
#include "shading_system.h"
#include "PixelShaderTextureCommon.h"
#include "PBRShading.h"
#include "RT/RTCommon.h"

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
	//float4 color  : COLOR0;
	float4 tangent : TANGENT;
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
	// This is the per-vertex Gouraud-shaded color coming from the VR:
	//float4 color			= float4(input.color.xyz, 1.0f);
	float4 texelColor		= texture0.Sample(sampler0, input.tex);
	float3 normalMapColor	= bDoNormalMapping ? normalMap.Sample(sampler0, input.tex).rgb : float3(0, 0, 1);
	//uint bIsBlastMark		= special_control & SPECIAL_CONTROL_BLAST_MARK;
	uint ExclusiveMask		= special_control & SPECIAL_CONTROL_EXCLUSIVE_MASK;
	// TODO: Do we need blast marks in the Tech Room?
	//if (bIsBlastMark) texelColor = texture0.Sample(sampler0, (input.tex * 0.35) + 0.3);

	float alpha = texelColor.w;
	const float3 P = input.pos3D.xyz;
	float SSAOAlpha = saturate(min(alpha - fSSAOAlphaOfs, fPosNormalAlpha));

	// Zero-out the bloom mask and provide default output values
	output.bloom = 0;
	output.color = float4(brightness * texelColor.xyz, texelColor.w);
	output.pos3D = float4(P, SSAOAlpha);
	output.ssMask = 0;

	float3 N = normalize(input.normal.xyz);
	N.yz = -N.yz; // Invert the Y axis, originally Y+ is down
	output.normal = float4(N, SSAOAlpha);

	if (bDoNormalMapping) {
		float3 T = normalize(input.tangent.xyz);
		T.y = -T.y; // Invert the Y axis, originally Y+ is down
		T.z = -T.z;
		float3 B = cross(T, N);
		float3x3 TBN = float3x3(T, B, N);
		const float3 NM = normalize(mul((normalMapColor * 2.0) - 1.0, TBN));
		N = lerp(N, NM, fNMIntensity);
	}

	if (ExclusiveMask == SPECIAL_CONTROL_GRAYSCALE)
		texelColor = float4(0.7, 0.7, 0.7, 1.0);

	// ssaoMask: Material, Glossiness, Specular Intensity
	output.ssaoMask = float4(fSSAOMaskVal, fGlossiness, fSpecInt, alpha);
	// SS Mask: unused-formerly-NMIntensity, Specular Value, Shadeless
	output.ssMask = float4(0, fSpecVal, fAmbient, alpha);

	// DEBUG: Display the normal map
	/*
	if (bDoNormalMapping)
	{
		output.color = float4(normalMapColor, 1);
		return output;
	}
	*/
	// DEBUG: Display the tangent map
	/*
	if (ExclusiveMask == SPECIAL_CONTROL_GRAYSCALE) {
		output.color = float4(T, 1.0);
		//output.color = float4(input.tex.xy, 0, 1);
		return output;
	}
	*/
	// DEBUG

	const float exposure = 1.0;
	const float blackish = smoothstep(0.3, 0.0, dot(0.333, texelColor.rgb));
	//float glossiness = output.ssaoMask.g + 0.5;
	const float metallicity = 0.25;
	float glossiness = lerp(0.7, 0.6, blackish);
	//float reflectance = 0.6;
	float reflectance = lerp(0.6, 0.5, blackish);
	const float ambient = 0.1;
	float final_alpha = alpha;
	// Make glass more glossy
	if (alpha < 0.98)
	{
		glossiness = 0.92;
		reflectance = 1.0;
	}
	float3 eye_vec = normalize(-output.pos3D.xyz); // normalize(eye - pos3D); // Vector from pos3D to the eye (0,0,0)
	float3 col = addPBR_RT_TechRoom(P, N, output.normal.xyz, -eye_vec,
		srgb_to_linear(texelColor.rgb),
		globalLights[0].direction.xyz,
		globalLights[0].color,
		metallicity,
		glossiness, // Glossiness: 0 matte, 1 glossy/glass
		reflectance,
		ambient
	);
	output.color.rgb = linear_to_srgb(ToneMapFilmic_Hejl2015(col * exposure, 1.0));
	// Make glass reflections more visible (non-transparent)
	if (alpha < 0.95)
		final_alpha = max(alpha, dot(output.color.rgb, 0.333));
	output.color.a = final_alpha;

	if (rand1 > 0.5)
	{
		const float  luma = dot(output.color.rgb, float3(0.299, 0.587, 0.114));
		const float3 holo_col = 3.0 * float3(0.325, 0.541, 0.643);
		const float  scans = 0.7 + 0.3 * sin(rand0 + 0.45 * input.pos.y);
		const float3 mixed_color = lerp(output.color.rgb, luma * holo_col, 0.5f);
		output.color.rgb = mixed_color * scans;
	}

	// In the D3dRendererHook, lightmaps and regular textures are rendered on the same draw call.
	// Here's the case where a lightmap has been provided:
	if (renderTypeIllum == 1)
	{
		// We have a lightmap texture
		float4 texelColorIllum = texture1.Sample(sampler0, input.tex);
		float3 color = texelColorIllum.rgb;
		float alpha = texelColorIllum.a;
		// This is a light texture, process the bloom mask accordingly
		float3 HSV = RGBtoHSV(color);
		float val = HSV.z;
		// Make the light textures brighter in 32-bit mode
		HSV.z *= 1.25;
		// The alpha for light textures is either 0 or >0.1, so we multiply by 10 to make it [0, 1]
		alpha *= 10.0;
		color = HSVtoRGB(HSV);

		const float bloom_alpha = smoothstep(0.75, 0.85, val) * smoothstep(0.45, 0.55, alpha);
		output.bloom = float4(bloom_alpha * val * color, bloom_alpha);

		// Bloom should disable shadows. Here we use the bloom factor
		// to blend back to the original unshaded albedo.
		//const float V = saturate(bloom_alpha * val);
		const float V = saturate(2.0f * bloom_alpha * val);
		output.color.rgb = lerp(output.color.rgb, texelColor.rgb, V);

		// Write an emissive material where there's bloom:
		output.ssaoMask.r = lerp(output.ssaoMask.r, bloom_alpha, bloom_alpha);
		// Set fNMIntensity to 0 where we have bloom:
		output.ssMask.r = lerp(output.ssMask.r, 0, bloom_alpha);
		// Replace the current color with the lightmap color, where appropriate:
		output.color.rgb = lerp(output.color.rgb, color, bloom_alpha);
		// Apply the bloom strength to this lightmap
		output.bloom.rgb *= fBloomStrength;
		//if (bInHyperspace && output.bloom.a < 0.5)
		//	discard;
		return output;
	}

	// The HUD is shadeless and has transparency. Some planets in the background are also 
	// transparent (CHECK IF Jeremy's latest hooks fixed this) 
	// So glass is a non-shadeless surface with transparency:
#ifdef DISABLED
	if (fSSAOMaskVal < SHADELESS_LO /* This texture is *not* shadeless */
		&& !bIsShadeless /* Another way of saying "this texture isn't shadeless" */
		&& alpha < 0.95) /* This texture has transparency */
		//&& !bIsBlastMark) /* Blast marks have alpha but aren't glass. See Direct3DDevice.cpp, search for SPECIAL_CONTROL_BLAST_MARK */
	{
		// Change the material and do max glossiness and spec_intensity
		output.ssaoMask.r = GLASS_MAT;
		output.ssaoMask.gba = 1.0;
		// Also write the normals of this surface over the current background
		output.normal.a = 1.0;
		output.ssMask.r = 0.0; // No normal mapping
		output.ssMask.g = 1.0; // White specular value
		output.ssMask.a = 1.0; // Make glass "solid" in the mask texture
	}
#endif

	return output;
}
