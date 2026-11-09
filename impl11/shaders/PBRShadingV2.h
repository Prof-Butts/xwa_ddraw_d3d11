#ifndef PBR_SHADING_V2_H
#define PBR_SHADING_V2_H

/*
 * BRDF and color functions for the PBR shading mode (ssao_type = PBR).
 *
 * Light convention: a light's color is the radiance that a white, perfectly diffuse
 * surface facing the light reflects. In other words, the irradiance is PI * color.
 * This keeps the meaning of the light colors used by the legacy shading, while the
 * BRDF itself conserves energy.
 */

#ifndef PI
#define PI 3.1415926535
#endif

// Exact sRGB transfer functions: https://en.wikipedia.org/wiki/SRGB
float3 SRGBToLinear(float3 srgb)
{
	const float3 lo = srgb / 12.92;
	const float3 hi = pow(abs((srgb + 0.055) / 1.055), 2.4);
	return float3(
		srgb.r > 0.04045 ? hi.r : lo.r,
		srgb.g > 0.04045 ? hi.g : lo.g,
		srgb.b > 0.04045 ? hi.b : lo.b);
}

float3 LinearToSRGB(float3 lin)
{
	const float3 lo = 12.92 * lin;
	const float3 hi = 1.055 * pow(abs(lin), 1.0 / 2.4) - 0.055;
	return float3(
		lin.r > 0.0031308 ? hi.r : lo.r,
		lin.g > 0.0031308 ? hi.g : lo.g,
		lin.b > 0.0031308 ? hi.b : lo.b);
}

// ACES filmic tonemapper, fitted by Stephen Hill (@self_shadow):
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
// Input: linear sRGB HDR color. Output: linear sRGB color in the range 0..1.
static const float3x3 ACESInputMat =
{
	{ 0.59719, 0.35458, 0.04823 },
	{ 0.07600, 0.90834, 0.01566 },
	{ 0.02840, 0.13383, 0.83777 }
};

static const float3x3 ACESOutputMat =
{
	{  1.60475, -0.53108, -0.07367 },
	{ -0.10208,  1.10813, -0.00605 },
	{ -0.00327, -0.07276,  1.07602 }
};

float3 RRTAndODTFit(float3 v)
{
	const float3 a = v * (v + 0.0245786) - 0.000090537;
	const float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return a / b;
}

float3 ACESFitted(float3 color)
{
	color = mul(ACESInputMat, color);
	color = RRTAndODTFit(color);
	color = mul(ACESOutputMat, color);
	return saturate(color);
}

// Smith-Schlick visibility term for one direction
float G1V(float dotNV, float k)
{
	return 1.0 / (dotNV * (1.0 - k) + k);
}

/*
 * Evaluates the Cook-Torrance BRDF for one light. All vectors must be normalized and
 * point away from the surface (in the same space).
 * roughness is the perceptual roughness: the GGX alpha is roughness^2.
 * diffuse and specular are BRDF values: multiply them by the irradiance to get the
 * reflected radiance.
 */
void EvaluateDirectBRDF(in float3 N, in float3 V, in float3 L, in float3 albedo,
	in float metallic, in float3 F0, in float roughness,
	out float3 diffuse, out float3 specular)
{
	const float3 H = normalize(V + L);
	const float dotNL = saturate(dot(N, L));
	const float dotNV = saturate(dot(N, V));
	const float dotNH = saturate(dot(N, H));
	const float dotLH = saturate(dot(L, H));

	// NDF: GGX
	const float alpha  = roughness * roughness;
	const float alpha2 = alpha * alpha;
	const float denom  = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	const float D = alpha2 / (PI * denom * denom);

	// Fresnel: Schlick
	const float3 F = F0 + (1.0 - F0) * pow(1.0 - dotLH, 5.0);

	// Visibility: Smith with Schlick's approximation. vis = G / (dotNL * dotNV)
	const float k = alpha / 2.0;
	const float vis = G1V(dotNL, k) * G1V(dotNV, k);

	// Cook-Torrance: D * F * G / (4 * dotNL * dotNV)
	specular = D * F * vis / 4.0;
	// Metals have no diffuse, and the light reflected by the specular lobe isn't
	// available for the diffuse lobe
	diffuse = (1.0 - F) * (1.0 - metallic) * albedo / PI;
}

/*
 * Specular reflectance of a surface lit by a uniform environment, integrated over the
 * hemisphere. Analytic approximation by Brian Karis:
 * https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
 */
float3 EnvBRDFApprox(float3 F0, float roughness, float dotNV)
{
	const float4 c0 = float4(-1.0, -0.0275, -0.572,  0.022);
	const float4 c1 = float4( 1.0,  0.0425,  1.04,  -0.04);
	const float4 r = roughness * c0 + c1;
	const float a004 = min(r.x * r.x, exp2(-9.28 * dotNV)) * r.x + r.y;
	const float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
	return F0 * AB.x + AB.y;
}

/*
 * Color grading, applied to the final (tonemapped, sRGB) colors. With contrast = 1,
 * saturation = 1 and white tints, it doesn't change anything.
 * - contrast: pushes the colors away from (> 1) or toward (< 1) mid-gray.
 * - saturation: 0 makes the image gray, 1 leaves it unchanged, > 1 makes it more colorful.
 * - shadowTint, highlightTint: the dark tones are multiplied by shadowTint and the bright
 *   ones by highlightTint, blended in between ("split toning"). The tints are scaled so
 *   they don't change the brightness, only the color.
 */
float3 ApplyColorGrade(float3 color, float contrast, float saturation, float3 shadowTint, float3 highlightTint)
{
	const float3 lumaWeights = float3(0.2126, 0.7152, 0.0722);
	color = (color - 0.5) * contrast + 0.5;
	float luma = dot(saturate(color), lumaWeights);
	color = lerp(luma, color, saturation);
	// Below ~0.05 it's all shadow tint, above ~0.6 it's all highlight tint
	float3 tint = lerp(shadowTint, highlightTint, smoothstep(0.05, 0.6, luma));
	tint /= max(dot(tint, lumaWeights), 1e-4);
	return saturate(color * tint);
}

#endif
