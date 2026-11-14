/*
 * Deferred PBR shading pass, used when ssao_type = PBR.
 * Copyright 2019, Leo Reyes.
 * Licensed under the MIT license. See LICENSE.txt
 *
 * Based on PBRAddCommon.h (the legacy PBR shading). The differences are:
 * - The BRDF conserves energy: metals have no diffuse, the diffuse is reduced by the
 *   light reflected by the specular, the Cook-Torrance specular includes the 1/4 term,
 *   and the light intensity and the shadows are applied once.
 * - The light colors are used as they are, instead of being multiplied by their own
 *   brightness.
 * - The ambient light is added once instead of once per light, and it also has a
 *   specular component.
 * - Colors are converted with the exact sRGB curves, and the result is tonemapped with
 *   a filmic (ACES) curve. pbr_exposure controls the brightness.
 * - pbr_min_albedo lifts the dark colors of the textures, and pbr_ambient_color tints
 *   the ambient light.
 * - pbr_ambient is always the base ambient light. A cubemap adds its own light on top
 *   (AmbientInt) and its reflections (Specular), instead of replacing the ambient and
 *   tinting the lights. pbr_ambient_color tints the base ambient and the cubemap's. The
 *   cubemap is converted to linear like the textures, its reflections get sharper on
 *   glossy surfaces (the mip comes from the roughness, from a GGX-prefiltered copy with
 *   pbr_cubemap_prefilter), and pbr_cubemap_ambient and pbr_cubemap_specular scale both.
 *   See "Sample the cubemaps" below.
 * - An optional color grading step (pbr_grade_*) on the final colors of the lit surfaces.
 * - GTAO (pbr_gtao_*): ambient occlusion and bent normals for the ambient light. It replaces
 *   the SSDO input of the legacy shading, which this pass doesn't use. An optional coarse
 *   pass (pbr_gtao_coarse*) adds the broad occlusion of large structures. Multi-bounce AO
 *   (pbr_gtao_multibounce) lightens it on bright surfaces, see GTAOMultiBounce().
 *   The laser and explosion lights, which have no shadows, are faded by the directions GTAO
 *   found blocked (pbr_gtao_light_occlusion), see the visibility cone below. The
 *   reflections are occluded with the same cone (pbr_gtao_specular_occlusion), see
 *   ConeSpecularOcclusion().
 * - The indirect light (pbr_gi_*, Ctrl+Alt+U): one bounce of the light of the surfaces
 *   around, gathered by GTAOGI.hlsl from the bounce light (the GI_BOUNCE_LIGHT_PASS entry
 *   point) and added to the diffuse light. See c:\temp\ssilvb-plan.md.
 * The shadows, the material heuristics and the composition with the background and the
 * transparent layers are the same as in PBRAddCommon.h.
 */
#include "shader_common.h"
#include "HSV.h"
#include "ShadertoyCBuffer.h"
#include "shading_system.h"
#include "SSAOPSConstantBuffer.h"
#include "shadow_mapping_common.h"
#include "PixelShaderTextureCommon.h"
#include "RT\RTCommon.h"
#include "PBRShadingV2.h"

#ifdef INSTANCED_RENDERING
// The color buffer
Texture2DArray texColor : register(t0);
SamplerState sampColor : register(s0);

// GTAO: bent normal (rgb) and ambient occlusion (a), when gtao_enabled. See GTAOCommon.h.
Texture2DArray texGTAO : register(t1);
// Coarse GTAO (quarter resolution, larger radius), when gtao_coarse_enabled
Texture2DArray texGTAOCoarse : register(t8);
// The bounce light (GIBounceLight.hlsl), at a quarter of the resolution, when gi_enabled
Texture2DArray texBounce : register(t9);
// The indirect light gathered by GTAOGI.hlsl, at a quarter of the resolution, when gi_enabled
Texture2DArray texGI : register(t10);
// The same after the denoise filter (GIDenoise.hlsl)
Texture2DArray texGIDenoised : register(t11);

// The Background buffer
Texture2DArray texBackground : register(t2);

// The SSAO mask
Texture2DArray texSSAOMask : register(t3);
SamplerState samplerSSAOMask : register(s3);

// The position/depth buffer
Texture2DArray texPos : register(t4);
SamplerState sampPos : register(s4);

// The (Smooth) Normals buffer
Texture2DArray texNormal : register(t5);
SamplerState samplerNormal : register(s5);

// The Shading System Mask buffer
Texture2DArray texSSMask : register(t6);
SamplerState samplerSSMask : register(s6);

// The RT Shadow Mask
Texture2DArray rtShadowMask : register(t17);

// Transparent layer 1
Texture2DArray transp1 : register(t18);
// Transparent layer 2
Texture2DArray transp2 : register(t19);

#else
// The color buffer
Texture2D texColor : register(t0);
SamplerState sampColor : register(s0);

// GTAO: bent normal (rgb) and ambient occlusion (a), when gtao_enabled. See GTAOCommon.h.
Texture2D texGTAO : register(t1);
// Coarse GTAO (quarter resolution, larger radius), when gtao_coarse_enabled
Texture2D texGTAOCoarse : register(t8);
// The bounce light (GIBounceLight.hlsl), at a quarter of the resolution, when gi_enabled
Texture2D texBounce : register(t9);
// The indirect light gathered by GTAOGI.hlsl, at a quarter of the resolution, when gi_enabled
Texture2D texGI : register(t10);
// The same after the denoise filter (GIDenoise.hlsl)
Texture2D texGIDenoised : register(t11);

// The Background buffer
Texture2D texBackground : register(t2);

// The SSAO mask
Texture2D texSSAOMask : register(t3);
SamplerState samplerSSAOMask : register(s3);

// The position/depth buffer
Texture2D texPos : register(t4);
SamplerState sampPos : register(s4);

// The (Smooth) Normals buffer
Texture2D texNormal : register(t5);
SamplerState samplerNormal : register(s5);

// The Shading System Mask buffer
Texture2D texSSMask : register(t6);
SamplerState samplerSSMask : register(s6);

// The RT Shadow Mask
Texture2D rtShadowMask : register(t17);

// Transparent layer 1
Texture2D transp1 : register(t18);
// Transparent layer 2
Texture2D transp2 : register(t19);

// Reticle (only available in non VR mode)
Texture2D reticleTex : register(t20);
#endif

// The Shadow Map buffer
Texture2DArray<float> texShadowMap : register(t7);
SamplerComparisonState cmpSampler : register(s7);

TextureCube skybox : register(t21);
// The GGX-prefiltered version of skybox (linear), when env_prefilter is set
TextureCube prefSkybox : register(t22);
TextureCube illumSkybox : register(t23);

// We're reusing the same constant buffer used to blur bloom; but here
// we really only use the amplifyFactor to upscale the SSAO buffer (if
// it was rendered at half the resolution, for instance)
// Bloom Constant Buffer
cbuffer ConstantBuffer : register(b2)
{
	float pixelSizeX, pixelSizeY, unused0, amplifyFactor;
	// 16 bytes
	float bloomStrength, uvStepSize, saturationStrength;
	uint unused1;
	// 32 bytes
	uint unused2;
	float unused3, depth_weight;
	uint debug;
};

// The indirect light's buffers (the bounce light and the gathered light) are at 1 / GI_SCALE
// of the resolution. Keep in sync with GI_SCALE in PrimarySurface.cpp.
#define GI_SCALE 4

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
	float4 bloom : SV_TARGET1;
};

#ifdef INSTANCED_RENDERING
#define GTAO_TEXTURE Texture2DArray
#else
#define GTAO_TEXTURE Texture2D
#endif

// Size of a GTAO buffer, in pixels
int2 GTAOBufferSize(GTAO_TEXTURE tex)
{
	uint width, height;
#ifdef INSTANCED_RENDERING
	uint elements;
	tex.GetDimensions(width, height, elements);
#else
	tex.GetDimensions(width, height);
#endif
	return int2(width, height);
}

// The GTAO result (bent normal, AO) for a full-resolution pixel, from a GTAO buffer computed
// at 1 / scale of the full resolution. Each GTAO pixel h was computed at the full-resolution
// pixel h * scale. Below full resolution, the four nearest GTAO pixels are blended with
// bilinear weights, skipping the ones at a different depth so the occlusion doesn't bleed
// across the edges of objects. maxPx is the last GTAO pixel that was rendered. n is the
// pixel's normal (Z+ away from the camera), for the pixels without any GTAO.
//
// Along the silhouettes of objects, all four can be on the background or at another depth:
// a GTAO pixel only sees the depth of the first full-resolution pixel of its block. Those
// pixels take the GTAO pixel at the closest depth in the surrounding 4x4 instead. Without
// that, a band of up to `scale` pixels along every silhouette had no occlusion at all, a
// bright, stair-stepped outline.
float4 LoadGTAO(GTAO_TEXTURE tex, int scale, int2 maxPx, int2 px, uint viewId, float z, float3 n)
{
#ifdef INSTANCED_RENDERING
#define GTAO_LOAD(p) tex.Load(int4(p, viewId, 0))
#define GTAO_DEPTH(p) texPos.Load(int4(p, viewId, 0)).z
#else
#define GTAO_LOAD(p) tex.Load(int3(p, 0))
#define GTAO_DEPTH(p) texPos.Load(int3(p, 0)).z
#endif
	float4 result;
	[branch]
	if (scale <= 1)
	{
		result = GTAO_LOAD(px);
	}
	else
	{
		const float2 pos = px / (float)scale;
		const int2 base = int2(floor(pos));
		const float2 f = pos - base;
		const float tolerance = max(z * 0.05, 1e-3);
		float4 sum = 0;
		float weightSum = 0;
		[unroll]
		for (int y = 0; y < 2; y++)
		{
			[unroll]
			for (int x = 0; x < 2; x++)
			{
				const int2 h = min(base + int2(x, y), maxPx);
				float w = (x ? f.x : 1.0 - f.x) * (y ? f.y : 1.0 - f.y);
				w *= saturate(1.0 - abs(GTAO_DEPTH(h * scale) - z) / tolerance);
				// Pixels that GTAO skipped (AO = 2) aren't blended in
				const float4 value = GTAO_LOAD(h);
				w *= value.a > 1.5 ? 0.0 : 1.0;
				sum += w * value;
				weightSum += w;
			}
		}
		if (weightSum > 1e-4)
		{
			result = sum / weightSum;
		}
		else
		{
			// No usable neighbor: the GTAO pixel at the closest depth around it, if it's on the
			// same surface (within a few times the tolerance), or no occlusion
			result = float4(n, 1.0);
			float bestDiff = 4.0 * tolerance;
			[loop]
			for (int sy = -1; sy <= 2; sy++)
			{
				[loop]
				for (int sx = -1; sx <= 2; sx++)
				{
					const int2 h = clamp(base + int2(sx, sy), 0, maxPx);
					const float4 value = GTAO_LOAD(h);
					const float diff = abs(GTAO_DEPTH(h * scale) - z);
					if (value.a < 1.5 && diff < bestDiff)
					{
						bestDiff = diff;
						result = value;
					}
				}
			}
		}
		result.xyz = length(result.xyz) > 1e-4 ? normalize(result.xyz) : n;
	}
	// The GTAO buffers keep values above 1 (and 2 for the skipped pixels), see GTAOCommon.h
	result.a = saturate(result.a);
	return result;
#undef GTAO_LOAD
#undef GTAO_DEPTH
}

// The indirect light (after the denoise filter) for a full-resolution pixel px at depth z,
// upsampled from a quarter of the resolution like LoadGTAO(): the four nearest pixels are
// blended with bilinear weights, skipping the ones at a different depth and the ones the
// gather skipped (a = 0), so the light doesn't bleed across the edges of objects. When none
// of them is usable (along silhouettes), the one at the closest depth in the surrounding
// 4x4, if it's on the same surface; otherwise no indirect light.
float3 LoadIndirectLight(int2 px, uint viewId, float z)
{
#ifdef INSTANCED_RENDERING
#define GI_LOAD(p) texGIDenoised.Load(int4(p, viewId, 0))
#define GI_DEPTH(p) texPos.Load(int4(p, viewId, 0)).z
#else
#define GI_LOAD(p) texGIDenoised.Load(int3(p, 0))
#define GI_DEPTH(p) texPos.Load(int3(p, 0)).z
#endif
	const int scale = GI_SCALE;
	const int2 maxPx = int2((uint2(GTAOBufferSize(texGIDenoised)) + (uint)scale - 1) / (uint)scale) - 1;
	const float2 pos = px / (float)scale;
	const int2 base = int2(floor(pos));
	const float2 f = pos - base;
	const float tolerance = max(z * 0.05, 1e-3);
	float3 sum = 0;
	float weightSum = 0;
	[unroll]
	for (int y = 0; y < 2; y++)
	{
		[unroll]
		for (int x = 0; x < 2; x++)
		{
			const int2 h = min(base + int2(x, y), maxPx);
			const float4 value = GI_LOAD(h);
			float w = (x ? f.x : 1.0 - f.x) * (y ? f.y : 1.0 - f.y);
			w *= saturate(1.0 - abs(GI_DEPTH(h * scale) - z) / tolerance);
			w *= value.a > 0.5 ? 1.0 : 0.0;
			sum += w * value.rgb;
			weightSum += w;
		}
	}
	float3 result = 0;
	[branch]
	if (weightSum > 1e-4)
	{
		result = sum / weightSum;
	}
	else
	{
		float bestDiff = 4.0 * tolerance;
		[loop]
		for (int sy = -1; sy <= 2; sy++)
		{
			[loop]
			for (int sx = -1; sx <= 2; sx++)
			{
				const int2 h = clamp(base + int2(sx, sy), 0, maxPx);
				const float4 value = GI_LOAD(h);
				const float diff = abs(GI_DEPTH(h * scale) - z);
				if (value.a > 0.5 && diff < bestDiff)
				{
					bestDiff = diff;
					result = value.rgb;
				}
			}
		}
	}
	return result;
#undef GI_LOAD
#undef GI_DEPTH
}

// From https://www.gamedev.net/tutorials/programming/graphics/effect-area-light-shadows-part-1-pcss-r4971/
/*
 * We expect Q.xy to be 2D coords; but Q.z must be a depth stencil value in the range 0...1
 */
inline float ShadowMapPCF(float idx, float3 Q, float resolution, int filterSize, float radius)
{
	float shadow = 0.0f;
	float2 sm_pos = Q.xy;
	// Convert to texture coords: this maps -1..1 to 0..1:
	sm_pos = lerp(0, 1, sm_pos * float2(0.5, -0.5) + 0.5);
	float2 grad = frac(sm_pos * resolution + 0.5);
	// We assume that Q.z has already been bias-compensated
	float samples = 0.0;
	float2 ofs = 0.0;

	ofs.y = -radius;
	for (int i = -filterSize; i <= filterSize; i++)
	{
		ofs.x = -radius;
		for (int j = -filterSize; j <= filterSize; j++)
		{
			float4 tmp = texShadowMap.Gather(sampPos, //samplerShadowMap,
				float3(sm_pos + ofs, idx));
			// We're comparing depth values here. For OBJ-based shadow maps, 1 is infinity 0 is ZNear
			// so if a dpeth value is *lower* than Q.z it's an occluder
			tmp.x = tmp.x < Q.z ? 0.0f : 1.0f;
			tmp.y = tmp.y < Q.z ? 0.0f : 1.0f;
			tmp.z = tmp.z < Q.z ? 0.0f : 1.0f;
			tmp.w = tmp.w < Q.z ? 0.0f : 1.0f;
			shadow += lerp(lerp(tmp.w, tmp.z, grad.x), lerp(tmp.x, tmp.y, grad.x), grad.y);
			samples++;
			ofs.x += radius;
		}
		ofs.y += radius;
	}

	return shadow / samples;
}

// See: https://wickedengine.net/2017/10/22/which-blend-state-for-me/
inline float4 PreMulBlend(in float4 src, in float4 dst)
{
	// Equivalent DX11 state:
	// desc.SrcBlend       = D3D11_BLEND_ONE;
	// desc.DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
	// desc.BlendOp        = D3D11_BLEND_OP_ADD;
	// Final alpha value settings:
	// desc.SrcBlendAlpha  = D3D11_BLEND_ONE;
	// desc.DestBlendAlpha = D3D11_BLEND_ONE;
	// desc.BlendOpAlpha   = D3D11_BLEND_OP_ADD;
	dst.rgb = src.rgb + (1 - src.a) * dst.rgb;
	return dst;
}

float4 BlendTransparentLayers(
	in float4 color,
	in float4 transpColor1,
	in float4 transpColor2)
{
	color = PreMulBlend(transpColor1, color);
	color = PreMulBlend(transpColor2, color);
	return color;
}

float4 BlendTransparentLayers(
	in float4 color,
	in float4 transpColor1,
	in float4 transpColor2,
	in float4 transpColor3)
{
	color = PreMulBlend(transpColor1, color);
	color = PreMulBlend(transpColor2, color);
	color = PreMulBlend(transpColor3, color);
	return color;
}

// Multi-bounce AO (Jimenez et al. 2016, "Practical Real-Time Strategies for Accurate
// Indirect Occlusion"). AO alone treats the occluders as black: the light that hits them is
// lost. Real occluders reflect part of it, and the light bounces around a crevice before it
// leaves, so crevices are lighter than AO says, more so on bright materials. This is the
// paper's fit of path-traced results: the visibility with all the bounces, from the AO and
// the albedo (linear), per color channel. It assumes the occluders around a point have its
// albedo, which also tints crevices with the surface's color. Black surfaces keep the AO.
float3 GTAOMultiBounce(float ao, float3 albedo)
{
	const float3 a =  2.0404 * albedo - 0.3324;
	const float3 b = -4.7951 * albedo + 0.6417;
	const float3 c =  2.7552 * albedo + 0.6903;
	return max(ao, ((ao * a + b) * ao + c) * ao);
}

// The cubemap's irradiance in direction d (unit, in the cubemap's space): its 9
// spherical-harmonic coefficients (env_sh) evaluated in that direction. It's the average
// light a surface facing d receives from the whole sky, already weighted by the cosine,
// in the same units as the sky's colors. Skies with very bright spots can make it ring
// slightly below 0 on the opposite side, so it's clamped.
float3 EvalEnvSH(float3 d)
{
	float3 result = env_sh[0].rgb * 0.282095;
	result += env_sh[1].rgb * (0.488603 * d.y);
	result += env_sh[2].rgb * (0.488603 * d.z);
	result += env_sh[3].rgb * (0.488603 * d.x);
	result += env_sh[4].rgb * (1.092548 * d.x * d.y);
	result += env_sh[5].rgb * (1.092548 * d.y * d.z);
	result += env_sh[6].rgb * (0.315392 * (3.0 * d.z * d.z - 1.0));
	result += env_sh[7].rgb * (1.092548 * d.x * d.z);
	result += env_sh[8].rgb * (0.546274 * (d.x * d.x - d.y * d.y));
	return max(result, 0.0);
}

// The half angle of the cone that holds most of the energy of a GGX reflection, from the
// perceptual roughness: the roughness is turned into the equivalent Phong exponent, and the
// cone is the fit of Uludag ("Hi-Z Screen-Space Cone-Traced Reflections", GPU Pro 5). About
// 3 degrees at roughness 0.2, 17 at 0.5 and 76 at 1.
float SpecularConeHalfAngle(float roughness)
{
	const float alpha = max(roughness * roughness, 1e-3);
	const float power = 2.0 / (alpha * alpha) - 2.0;
	return acos(pow(0.244, 1.0 / (power + 1.0)));
}

// The fraction of a reflection cone (axis reflDir, half angle reflHalfAngle) that's inside
// a visibility cone (axis visAxis, half angle visHalfAngle), from the area of the overlap
// of their spherical caps. The overlap is approximated with a smoothstep between "one cap
// inside the other" and "no overlap" (Oat & Sander, "Ambient Aperture Lighting", 2007; the
// same approximation as UE4's distance field AO).
float ReflectionConeVisibility(float visHalfAngle, float3 visAxis, float reflHalfAngle, float3 reflDir)
{
	const float d = acos(clamp(dot(visAxis, reflDir), -1.0, 1.0));
	const float capVis  = 1.0 - cos(visHalfAngle);
	const float capRefl = 1.0 - cos(reflHalfAngle);
	const float diff = abs(visHalfAngle - reflHalfAngle);
	const float t = 1.0 - saturate((d - diff) / max(visHalfAngle + reflHalfAngle - diff, 1e-4));
	return smoothstep(0.0, 1.0, t) * min(capVis, capRefl) / max(capRefl, 1e-6);
}

// Specular occlusion from GTAO's visibility cone (Jimenez et al. 2016): how much of the
// reflection's cone falls in the directions where the point sees open space. A glossy
// surface in a crevice keeps reflecting the sky when its reflection points out of the
// crevice, and loses it when it points into a wall; the AO alone only knows how much is
// occluded, not where. The visibility cone's axis is the bent normal (bentN), and its half
// angle comes from the AO as a fraction of the hemisphere's solid angle: cos = 1 - AO (the
// whole hemisphere around n without occlusion). The cone assumes the occluders are spread
// evenly around it, which isn't true on a hull where they're on one side; the stricter
// cosine-weighted cone, cos = sqrt(1 - AO), removed the grazing reflections of whole open
// areas as soon as a large GTAO radius found a few occluders. The result is relative to an
// unoccluded hemisphere around the geometric normal n, so the part of a grazing reflection
// that's below the horizon (which the Fresnel term already accounts for) doesn't count as
// occlusion: without occluders, it's 1. All the directions are unit vectors in the same
// space.
//
// The reflection's cone is at least 15 degrees wide for this test, even for a mirror: the
// visibility cone is only an approximation, from GTAO buffers at half and a quarter of the
// resolution, and a narrower cone makes the test all-or-nothing, so the small variations of
// the bent normal showed as steps and noise.
float ConeSpecularOcclusion(float ao, float3 bentN, float3 n, float3 reflDir, float roughness)
{
	const float reflHalfAngle = max(SpecularConeHalfAngle(roughness), 0.26);
	const float visHalfAngle = acos(saturate(1.0 - ao));
	const float occluded = ReflectionConeVisibility(visHalfAngle, bentN, reflHalfAngle, reflDir);
	const float open = ReflectionConeVisibility(0.5 * 3.14159265, n, reflHalfAngle, reflDir);
	return saturate(occluded / max(open, 1e-4));
}

// The AO of the diffuse ambient light: GTAO with multi-bounce (gtao_multibounce = 0 is plain
// GTAO). It's applied to the combined AO of both passes: it's a function of all the
// occlusion around a point, and applying it to each pass separately would lighten crevices
// too much.
float3 GTAODiffuseAO(float ao, float3 albedo)
{
	return lerp(ao, GTAOMultiBounce(ao, albedo), gtao_multibounce);
}

// The shadows of the lights that cast them, for a pixel: the ray-traced shadows (the
// filtered RT shadow mask, or rays traced here) and the shadow maps, multiplied. They never
// go below the black level. Shared by the lighting pass and the bounce light pass.
float ComputeSunShadowFactor(PixelShaderInput input, float3 P, float3 N)
{
	// Raytraced shadows. The output of this section is written to rt_shadow_factor
	float rt_shadow_factor = 1.0f;
	if (bRTEnabled)
	{
		if (RTEnableSoftShadows)
		{
			int i, j;
			float2 uv;
			float rtVal = 0;
			const int range = 2;
#ifdef INSTANCED_RENDERING
			const float rtCenter = rtShadowMask.Sample(sampColor, float3(input.uv, input.viewId)).x;
#else
			const float rtCenter = rtShadowMask.Sample(sampColor, input.uv).x;
#endif
#ifdef GI_BOUNCE_LIGHT_PASS
			// The bounce light only needs the shadow at the pixel: the gather samples it sparsely
			// and the indirect light is blurred anyway. The soft-shadow filter below reads the mask
			// and the positions about 60 times per pixel, which made the bounce pass expensive.
			rt_shadow_factor = rtCenter;
#else
			// When RTShadowMaskSizeFactor is 1, we won't get soft shadows because the uv_delta
			// is too small. So we need to set a lower bound to have a good delta.
			const float rtSizeFactor = max(2.0, RTShadowMaskSizeFactor);
			const float2 uv_delta = float2(RTShadowMaskPixelSizeX * rtSizeFactor, RTShadowMaskPixelSizeY * rtSizeFactor);
			const float2 uv_delta_d = uv_delta; // Dilation filter delta
			const float2 uv_delta_r = range * uv_delta; // Soft shadow delta

			// Apply a quick-and-dirty dilation filter to prevent haloes
			const float2 uv0_d = input.uv - uv_delta_d;
			float rtMin = rtCenter;
			uv = uv0_d;
			[unroll]
			for (i = -1; i <= 1; i++)
			{
				uv.x = uv0_d.x;
				[unroll]
				for (j = -1; j <= 1; j++)
				{
#ifdef INSTANCED_RENDERING
					rtMin = min(rtMin, rtShadowMask.Sample(sampColor, float3(uv, input.viewId)).x);
#else
					rtMin = min(rtMin, rtShadowMask.Sample(sampColor, uv).x);
#endif
					uv.x += uv_delta_d.x;
				}
				uv.y += uv_delta_d.y;
			}

			float Gsum = 0;
			const float2 uv0_r = input.uv - range * uv_delta_r;
			uv = uv0_r;
			[unroll]
			for (i = -range; i <= range; i++)
			{
				uv.x = uv0_r.x;
				[unroll]
				for (j = -range; j <= range; j++)
				{
#ifdef INSTANCED_RENDERING
					const float z = texPos.Sample(sampPos, float3(uv, input.viewId)).z;
#else
					const float z = texPos.Sample(sampPos, uv).z;
#endif
					const float delta_z = abs(P.z - z);
					const float delta_ij = -RTGaussFactor * (i * i + j * j);
					const float G = exp(delta_ij);
					// Objects far away should have bigger thresholds too
					if (delta_z < RTSoftShadowThresholdMult * P.z)
#ifdef INSTANCED_RENDERING
						rtVal += G * rtShadowMask.Sample(sampColor, float3(uv, input.viewId)).x;
#else
						rtVal += G * rtShadowMask.Sample(sampColor, uv).x;
#endif
					else
						rtVal += G * rtMin;
					Gsum += G;
					uv.x += uv_delta_r.x;
				}
				uv.y += uv_delta_r.y;
			}
			rt_shadow_factor = rtVal / Gsum;
#endif
		}
		else
		{
			// Do ray-tracing right here, no RTShadowMask
			for (uint i = 0; i < LightCount; i++)
			{
				float black_level, dummyMinZ, dummyMaxZ;
				get_black_level_and_minmaxZ(i, black_level, dummyMinZ, dummyMaxZ);
				// Skip lights that won't project black-enough shadows
				if (black_level > 0.95)
					continue;

				const float3 L = LightVector[i].xyz;
				const float dotLFlatN = dot(L, N); // The "flat" normal is needed here (without Normal Mapping)
				// "hover" prevents noise by displacing the origin of the ray away from the surface
				// The displacement is directly proportional to the depth of the surface
				// The position buffer's Z increases with depth
				// The normal buffer's Z+ points towards the camera
				// We have to invert N.z:
				const float3 hover = 0.01 * P.z * float3(N.x, N.y, -N.z);
				// Don't do raytracing on surfaces that face away from the light source
				if (dotLFlatN > 0.01)
				{
					Ray ray;
					ray.origin = P + hover; // Metric, Y+ is up, Z+ is forward.
					ray.dir = float3(L.x, -L.y, -L.z);
					ray.max_dist = RT_MAX_DIST;

					Intersection inters = TLASTraceRaySimpleHit(ray);
					if (inters.TriID > -1)
						rt_shadow_factor *= black_level;
				}
				else
				{
					rt_shadow_factor *= black_level;
				}
			}
		}
	}

	// Shadow Mapping.
	// The shadow casting contribution for both RT and SM ends up in total_shadow_factor.
	float total_shadow_factor = rt_shadow_factor;
	// Don't compute shadow mapping if Raytraced shadows are enabled in the cockpit... it's redundant.
	if (sm_enabled && bRTAllowShadowMapping)
	{
		[loop]
		for (uint i = 0; i < LightCount; i++)
		{
			float shadow_factor = 1.0;
			float black_level, minZ, maxZ;
			get_black_level_and_minmaxZ(i, black_level, minZ, maxZ);
			// Skip lights that won't project black-enough shadows or that are
			// out-of-range for ShadowMapping
			if (black_level > 0.95 || P.z < minZ || P.z > maxZ)
				continue;
			// Apply the same transform we applied to P in ShadowMapVS.hlsl
			float3 Q = mul(lightWorldMatrix[i], float4(P, 1.0)).xyz;
			// Distant objects require more bias, here we're using maxZ as a proxy to tell us
			// how distant is the shadow map and we use that to compensate the bias
			float bias = sm_bias - 1.0 * step(50.0f, maxZ);
			// shadow_factor: 1 -- No shadow
			// shadow_factor: 0 -- Full shadow
			shadow_factor = ShadowMapPCF(i, float3(Q.xy, MetricZToDepth(i, Q.z + bias)), sm_resolution, sm_pcss_samples, sm_pcss_radius);
			// Limit how black the shadows can be to a minimum of black_level
			shadow_factor = max(shadow_factor, black_level);
			// Accumulate this light's shadow factor with the total shadow factor
			total_shadow_factor *= shadow_factor;
		}
	}
	return total_shadow_factor;
}

// The material of a pixel, from the G-buffer masks. These heuristics are the same as in
// PBRAddCommon.h. color is the linear color, albedo the one used for lighting. It also
// reduces total_shadow_factor (the shadows, see ComputeSunShadowFactor()) on black areas.
// Shared by the lighting pass and the bounce light pass.
void ComputeMaterial(float3 color, float3 albedo, float mask, float gloss_mask, float spec_int_mask,
	float spec_val_mask, float glass, float metallic, inout float total_shadow_factor,
	out float blackish, out float metallicity, out float roughness, out float3 F0)
{
	// Approximate luma:
	const float Value = max(dot(float3(0.299f, 0.587f, 0.114f), color.xyz), spec_val_mask);
	blackish = smoothstep(0.1, 0.0, Value);
	metallicity = (mask < METAL_HI) ? metallic : 0.25;
	float glossiness = min(0.93, gloss_mask);
	float reflectance = lerp(spec_int_mask, 0.0, blackish);

	// The following will reduce the shininess of black areas. This behavior is similar
	// to the current materials model.
	total_shadow_factor = lerp(total_shadow_factor, total_shadow_factor * 0.5, blackish);

	// Make glass more glossy
	if (glass > 0.01)
	{
		glossiness = 0.93;
		reflectance = 1.0;
	}

	roughness = 1.0 - glossiness * glossiness;
	F0 = 0.16 * reflectance * reflectance * (1.0 - metallicity) + albedo * metallicity;
}

// The laser and explosion lights (the dynamic lights) on a pixel: their diffuse light, added
// to the final color. pos3D has Z inverted, like the light positions, and N is Z+ towards the
// camera. lightOcclusion, lightConeAxis and lightConeCos: GTAO's occlusion of these lights,
// see the lighting pass (0 turns it off). Shared by the lighting pass and the bounce light
// pass.
#define L_FADEOUT_A_0 30.0
#define L_FADEOUT_A_1 50.0
#define L_FADEOUT_B_0 50.0
#define L_FADEOUT_B_1 1000.0
#define LIGHT_CONE_SOFTNESS 0.3 // Width of the fade past the cone's edge, in cosine
float3 ComputeDynamicLights(float3 pos3D, float3 N, float lightOcclusion, float3 lightConeAxis,
	float lightConeCos)
{
	float3 laser_light_sum = 0.0;
	uint i;
	[loop]
	for (i = 0; i < num_lasers; i++)
	{
		// P is the original point, pos3D has z inverted:
		// pos3D = (P.xy, -P.z)
		// LightPoint already comes with z inverted (-z) from ddraw, so we can subtract LightPoint - pos3D
		// because both points have -z:
		float3 L = LightPoint[i].xyz - pos3D;
		const float Z = -LightPoint[i].z; // Z is positive depth
		const float falloff = LightPoint[i].w;
		const float3 LDir = LightPointDirection[i].xyz;
		const float angle_falloff_cos = LightPointDirection[i].w;

		float attenuation, depth_attenuation, angle_attenuation = 1.0f;
		const float distance_sqr = dot(L, L);
		L *= rsqrt(distance_sqr); // Normalize L
		// calculate the attenuation for laser lights
		if (falloff == 0.0f) {
			const float depth_attenuation_A = 0.15 * smoothstep(L_FADEOUT_A_1, L_FADEOUT_A_0, Z); // Fade the cockpit flash quickly
			const float depth_attenuation_B = 0.1 * smoothstep(L_FADEOUT_B_1, L_FADEOUT_B_0, Z); // Fade the distant flash slowly
			depth_attenuation = max(depth_attenuation_A, depth_attenuation_B);
			attenuation = 1.0 / (1.0 + sqr_attenuation * distance_sqr);
		}
		// calculate the attenuation for other lights (explosions, etc)
		else {
			depth_attenuation = 1.0;
			attenuation = falloff / distance_sqr;
		}
		// calculate the attenuation for directional lights
		if (angle_falloff_cos != 0.0f) {
			// compute the angle between the light's direction and the
			// vector from the current point to the light's center
			const float angle = max(dot(L, LDir), 0.0);
			angle_attenuation = smoothstep(angle_falloff_cos, 1.0, angle);
		}
		// compute the diffuse contribution
		const float diff_val = max(dot(N, L), 0.0);
		// GTAO occlusion: 1 inside the visibility cone, fading to 0 past its edge
		const float coneVisibility = smoothstep(lightConeCos - LIGHT_CONE_SOFTNESS, lightConeCos, dot(lightConeAxis, L));
		const float occlusion = lerp(1.0, coneVisibility, lightOcclusion);
		laser_light_sum += occlusion * depth_attenuation * attenuation * angle_attenuation * diff_val * LightPointColor[i].rgb;
	}
	return laser_light_sum;
}

#ifdef GI_BOUNCE_LIGHT_PASS
/*
 * The bounce light pass (GIBounceLight.hlsl, pbr_gi_enabled): the diffuse light that each
 * surface sends back out, which the indirect light gathers (see c:\temp\ssilvb-plan.md).
 * It runs before GTAO, at a quarter of the resolution (GI_SCALE), like the gather, which
 * only reads it at scattered sample points. Its pixel h uses the full-resolution pixel
 * h * GI_SCALE. It uses the same functions as the lighting pass below, so the light matches
 * it:
 * - The lights (the sun and the others), diffuse only. The shadows of the lights that cast
 *   them don't get their black level: it's a stand-in for the light that fills the shadows,
 *   not sunlight, so it doesn't bounce.
 * - The laser and explosion lights, as the lighting pass adds them (without GTAO's
 *   occlusion, which isn't computed yet).
 * - Shadeless surfaces (engine glows, lights, displays): their own color, as emitters.
 * No ambient light: the sky's bounce is approximated by the multi-bounce AO, and it would
 * feed back into itself. The output is linear, before the exposure, like the lighting
 * pass. Alpha is the coverage: 0 on the background.
 */
float4 main(PixelShaderInput input) : SV_TARGET0
{
	// Pixel h uses the full-resolution pixel h * GI_SCALE, like the gather pass
	uint fullWidth, fullHeight;
#ifdef INSTANCED_RENDERING
	uint elements;
	texPos.GetDimensions(fullWidth, fullHeight, elements);
	input.uv = (floor(input.pos.xy) * GI_SCALE + 0.5) / float2(fullWidth, fullHeight);
	const float4 texelColor = texColor.Sample(sampColor, float3(input.uv, input.viewId));
	const float4 Normal     = texNormal.Sample(samplerNormal, float3(input.uv, input.viewId));
	float3 pos3D            = texPos.Sample(sampPos, float3(input.uv, input.viewId)).xyz;
	const float3 ssaoMask   = texSSAOMask.Sample(samplerSSAOMask, float3(input.uv, input.viewId)).xyz;
	const float3 ssMask     = texSSMask.Sample(samplerSSMask, float3(input.uv, input.viewId)).xyz;
#else
	texPos.GetDimensions(fullWidth, fullHeight);
	input.uv = (floor(input.pos.xy) * GI_SCALE + 0.5) / float2(fullWidth, fullHeight);
	const float4 texelColor = texColor.Sample(sampColor, input.uv);
	const float4 Normal     = texNormal.Sample(samplerNormal, input.uv);
	float3 pos3D            = texPos.Sample(sampPos, input.uv).xyz;
	const float3 ssaoMask   = texSSAOMask.Sample(samplerSSAOMask, input.uv).xyz;
	const float3 ssMask     = texSSMask.Sample(samplerSSMask, input.uv).xyz;
#endif
	const float mask          = ssaoMask.x;
	const float gloss_mask    = ssaoMask.y;
	const float spec_int_mask = ssaoMask.z;
	const float metallic      = mask / METAL_MAT;
	const float glass         = ssMask.x;
	const float spec_val_mask = ssMask.y;
	const float shadeless     = ssMask.z;

	// The same coverage as the lighting pass, where it blends with the background
	const float blendAlphaSat = saturate(2.0 * Normal.w);
	const float blendAlpha = blendAlphaSat * blendAlphaSat;
	if (blendAlpha <= 0.0)
		return 0;

	const float3 P = pos3D.xyz;
	pos3D.z = -pos3D.z;
	const float3 color = SRGBToLinear(texelColor.rgb);
	if (shadeless >= 1.0)
		return float4(color * blendAlpha, blendAlpha);

	const float3 albedo = albedo_floor + (1.0 - albedo_floor) * color;
	const float3 N = normalize(Normal.xyz);
	float total_shadow_factor = ComputeSunShadowFactor(input, P, N);
	float blackish, metallicity, roughness;
	float3 F0;
	ComputeMaterial(color, albedo, mask, gloss_mask, spec_int_mask, spec_val_mask, glass, metallic,
		total_shadow_factor, blackish, metallicity, roughness, F0);
	// The shadows without their black level
	const float visibility = specular_shadow_factor(total_shadow_factor, get_shadow_caster_black_level(LightCount));

	// The lights, as in the lighting pass: the ones that don't cast shadows are dimmed
	float3 N_PBR = N;
	N_PBR.xy = -N_PBR.xy;
	const float3 V_PBR = normalize(P);
	float3 direct = 0.0;
	[loop]
	for (uint i = 0; i < LightCount; i++)
	{
		const bool isShadowCaster = get_black_level(i) <= 0.95;
		float3 L = LightVector[i].xyz;
		L.xy = -L.xy;
		L = normalize(L);
		const float3 lightColor = (isShadowCaster ? 1.0 : 0.25) * LightColor[i].rgb;
		float3 diffuseBRDF, specularBRDF;
		EvaluateDirectBRDF(N_PBR, V_PBR, L, albedo, metallicity, F0, roughness, diffuseBRDF, specularBRDF);
		direct += diffuseBRDF * PI * lightColor * saturate(dot(N_PBR, L)) * (isShadowCaster ? visibility : 1.0);
	}

	float3 radiance = lerp(direct, color, shadeless);
	radiance += laser_light_intensity * ComputeDynamicLights(pos3D, N, 0.0, float3(0, 0, 1), 0.0);
	return float4(radiance * blendAlpha, blendAlpha);
}
#else
PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	output.color = 0;
	output.bloom = 0;

#ifdef INSTANCED_RENDERING
	float4 texelColor     = texColor.Sample(sampColor, float3(input.uv, input.viewId));
	float4 Normal         = texNormal.Sample(samplerNormal, float3(input.uv, input.viewId));
	float3 pos3D          = texPos.Sample(sampPos, float3(input.uv, input.viewId)).xyz;
	float3 background     = texBackground.Sample(sampColor, float3(input.uv, input.viewId)).rgb;
	float3 ssaoMask       = texSSAOMask.Sample(samplerSSAOMask, float3(input.uv, input.viewId)).xyz;
	float3 ssMask         = texSSMask.Sample(samplerSSMask, float3(input.uv, input.viewId)).xyz;
	float4 transpColor1   = transp1.Sample(sampColor, float3(input.uv, input.viewId));
	float4 transpColor2   = transp2.Sample(sampColor, float3(input.uv, input.viewId));
#else
	float4 texelColor     = texColor.Sample(sampColor, input.uv);
	float4 Normal         = texNormal.Sample(samplerNormal, input.uv);
	float3 pos3D          = texPos.Sample(sampPos, input.uv).xyz;
	float3 background     = texBackground.Sample(sampColor, input.uv).rgb;
	float3 ssaoMask       = texSSAOMask.Sample(samplerSSAOMask, input.uv).xyz;
	float3 ssMask         = texSSMask.Sample(samplerSSMask, input.uv).xyz;
	float4 transpColor1   = transp1.Sample(sampColor, input.uv);
	float4 transpColor2   = transp2.Sample(sampColor, input.uv);
	float4 reticleColor   = reticleTex.Sample(sampColor, input.uv);
#endif
	float3 color          = texelColor.rgb;
	float  mask           = ssaoMask.x;
	float  gloss_mask     = ssaoMask.y;
	float  spec_int_mask  = ssaoMask.z;
	float  metallic       = mask / METAL_MAT;
	float  glass          = ssMask.x;
	float  spec_val_mask  = ssMask.y;
	float  shadeless      = ssMask.z;

	// The pow() term makes the alpha drop faster from 1 to 0, this reduces
	// the halos around objects
	const float blendAlphaSat = saturate(2.0 * Normal.w);
	const float blendAlpha = blendAlphaSat * blendAlphaSat;

	// GTAO (Ctrl+Alt+J toggles it): it occludes the ambient light, and the bent normal (the
	// least occluded direction) is used to look up the cubemap's ambient light. The sun has
	// real shadows, so it isn't occluded.
	const bool bGTAO = gtao_enabled > 0.5;
	float4 gtao = 1.0;
	float fineAO = 1.0, coarseAO = 1.0; // For pbr_gtao_debug
	[branch]
	if (bGTAO)
	{
#ifdef INSTANCED_RENDERING
		const uint viewId = input.viewId;
#else
		const uint viewId = 0;
#endif
		const int2 px = int2(input.pos.xy);
		const int fineScale = (int)gtao_scale;
		const int2 fineMaxPx = int2((uint2(GTAOBufferSize(texGTAO)) + (uint)fineScale - 1) / (uint)fineScale) - 1;
		const float3 n = normalize(float3(Normal.x, Normal.y, -Normal.z));
		gtao = LoadGTAO(texGTAO, fineScale, fineMaxPx, px, viewId, pos3D.z, n);
		fineAO = gtao.a;
		// Two-scale AO: the coarse pass adds the broad occlusion of large structures. The
		// visibilities multiply, and the bent normal turns by the sum of both deviations
		// from the normal. Where one of them sees no occluders, it changes nothing.
		[branch]
		if (gtao_coarse_enabled > 0.5)
		{
			const float4 coarse = LoadGTAO(texGTAOCoarse, 4, GTAOBufferSize(texGTAOCoarse) - 1, px, viewId, pos3D.z, n);
			gtao.xyz = gtao.xyz + coarse.xyz - n;
			gtao.a *= coarse.a;
			coarseAO = coarse.a;
		}
	}
	const float ao = gtao.a;

	// Debug view (pbr_gtao_debug): the AO of each pass, or the bent normal, on the surfaces.
	// Both passes combined (3) includes the multi-bounce, so it shows its colors.
	[branch]
	if (bGTAO && gtao_debug > 0.5)
	{
		const float3 debugAlbedo = albedo_floor + (1.0 - albedo_floor) * SRGBToLinear(texelColor.rgb);
		float3 debugColor = GTAODiffuseAO(ao, debugAlbedo);
		if (gtao_debug < 1.5)      debugColor = fineAO;
		else if (gtao_debug < 2.5) debugColor = coarseAO;
		else if (gtao_debug > 3.5) debugColor = normalize(gtao.xyz) * 0.5 + 0.5;
		output.color = float4(lerp(background, debugColor, blendAlpha), 1);
		return output;
	}

	// Debug views (pbr_gi_debug), exposed and tonemapped like the final colors:
	// 1: the bounce light, the light that the surfaces send back out (GIBounceLight.hlsl, at
	//    a quarter of the resolution);
	// 2: the indirect light gathered by GTAOGI.hlsl, before the denoise: the light that each
	//    surface receives from the others (at a quarter of the resolution);
	// 3: the indirect light after the denoise filter, upsampled to full resolution, as the
	//    shading uses it.
	[branch]
	if (gi_enabled > 0.5 && gi_debug > 0.5 && gi_debug < 3.5)
	{
		const int2 giPx = int2(uint2(input.pos.xy) / (uint)GI_SCALE);
#ifdef INSTANCED_RENDERING
		const uint giViewId = input.viewId;
		float3 debugLight = gi_debug < 1.5 ? texBounce.Load(int4(giPx, giViewId, 0)).rgb :
			texGI.Load(int4(giPx, giViewId, 0)).rgb;
#else
		const uint giViewId = 0;
		float3 debugLight = gi_debug < 1.5 ? texBounce.Load(int3(giPx, 0)).rgb :
			texGI.Load(int3(giPx, 0)).rgb;
#endif
		if (gi_debug > 2.5)
			debugLight = LoadIndirectLight(int2(input.pos.xy), giViewId, pos3D.z);
		output.color = float4(lerp(background, LinearToSRGB(ACESFitted(exposure_mult * debugLight)), blendAlpha), 1);
		return output;
	}

	// We need to invert the Z-axis for illumination because the normals are Z+ when pointing to the camera
	// so that implies that Z increases towards the viewer and decreases away from the camera.
	const float3 P = pos3D.xyz;
	pos3D.z = -pos3D.z;

	// The direction reflected towards the cubemap (Z+ away from the camera, then in the
	// cubemap's space), and the mip level that the screen can resolve for it: on curved
	// surfaces it changes fast from pixel to pixel, and the sharp mips would shimmer. It's
	// computed here, before any pixel can return early, because it uses screen derivatives.
	const float3 reflDirView = reflect(normalize(P), normalize(float3(Normal.x, Normal.y, -Normal.z)));
	const float3 skyBoxReflected = mul(viewMat, float4(reflDirView, 0)).xyz;
	const float reflScreenMip = skybox.CalculateLevelOfDetail(sampColor, skyBoxReflected);

	if (shadeless >= 1.0)
	{
		output.color = float4(lerp(background, color, blendAlpha), 1);
#ifdef INSTANCED_RENDERING
		output.color = BlendTransparentLayers(output.color, transpColor1, transpColor2);
#else
		output.color = BlendTransparentLayers(output.color, transpColor1, transpColor2, reticleColor);
#endif
		return output;
	}

	// The textures are in sRGB, lighting is done in linear space
	color = SRGBToLinear(color);
	// The albedo used for lighting. Real materials are rarely darker than ~0.03, so the dark
	// colors are lifted to albedo_floor (pbr_min_albedo) and the rest are scaled to keep white at 1. This
	// keeps dark details instead of flattening them. The shadeless mask still uses color.
	const float3 albedo = albedo_floor + (1.0 - albedo_floor) * color;

	float3 N = normalize(Normal.xyz);

	// The sun's shadows (ray-traced and shadow-mapped), see ComputeSunShadowFactor()
	float total_shadow_factor = ComputeSunShadowFactor(input, P, N);

	// ************************************************************************************************
	// Material properties, see ComputeMaterial()
	float blackish, metallicity, roughness;
	float3 F0;
	ComputeMaterial(color, albedo, mask, gloss_mask, spec_int_mask, spec_val_mask, glass, metallic,
		total_shadow_factor, blackish, metallicity, roughness, F0);

	// The lighting below is done in the same space as PBRAddCommon.h: N, V and L are all
	// negated with respect to view space, so the dot products are the same.
	float3 N_PBR = N;
	N_PBR.xy = -N_PBR.xy;
	const float3 V_PBR = normalize(P);
	const float dotNV = saturate(dot(N_PBR, V_PBR));

	// Sample the cubemaps: their ambient light and their reflections. The cubemaps are in
	// sRGB (their files have no color profile, so they're loaded as UNORM), and lighting is
	// done in linear space, so they're converted like the textures.
	// - Ambient: the cubemap's irradiance in the direction of the bent normal (Z+ away from
	//   the camera, like skyBoxReflected): the light from the whole sky, weighted by the
	//   cosine, from its spherical harmonics (EvalEnvSH). When there's an _illum cubemap (a
	//   low-res version made for lighting), its irradiance is used. If the harmonics aren't
	//   available, the cubemap is sampled at the mission's diffuse mip level (MipRes).
	// - Reflections: always the main cubemap, at the mip level that matches the roughness, so
	//   glossy surfaces reflect a sharp image and rough ones a blurry one. The mips are a box
	//   filter of the full image, so the mip whose texels are as wide as the GGX lobe is
	//   used. The lobe is about PI/4 * alpha radians wide (alpha = roughness^2, the whole
	//   hemisphere at roughness 1), and a face is PI/2 wide, so the face size that matches it
	//   is 2 / alpha pixels: mip = log2(faceSize * alpha / 2). It's never sharper than the
	//   screen can resolve (reflScreenMip), but the screen's limit is capped, so the edges of
	//   objects (where the derivatives jump to the background) don't get a blurry outline.
	float3 skyBoxNormal    = bGTAO ? normalize(gtao.xyz) : normalize(float3(Normal.x, Normal.y, -Normal.z));
	skyBoxNormal           = mul(viewMat, float4(skyBoxNormal, 0)).xyz;
	float3 skyBoxReflCol   = 1;
	float3 skyBoxNormalCol = 1;
	if (cubeMappingEnabled)
	{
		uint faceSize, faceSize2, mipCount;
		skybox.GetDimensions(0, faceSize, faceSize2, mipCount);
		[branch]
		if (env_prefilter > 0.5)
		{
			// The prefiltered cubemap (pbr_cubemap_prefilter, CubeMapPrefilterCS.hlsl): each mip
			// is the sky convolved with the GGX lobe of a roughness, from 0 at mip 0 to 1 at the
			// last one, already linear (and boosted). The screen's limit is in the main
			// cubemap's mips, and this one is smaller.
			uint prefSize, prefSize2, prefMips;
			prefSkybox.GetDimensions(0, prefSize, prefSize2, prefMips);
			const float prefMaxMip = (float)prefMips - 1.0;
			const float roughnessMip = roughness * prefMaxMip;
			const float screenMip = min(reflScreenMip - log2((float)faceSize / (float)prefSize),
				log2((float)prefSize / 32.0)); // At most 32x32 per face
			const float reflMip = clamp(max(roughnessMip, screenMip), 0.0, prefMaxMip);
			skyBoxReflCol = prefSkybox.SampleLevel(sampColor, skyBoxReflected, reflMip).rgb;
		}
		else
		{
			const float maxMip = (float)mipCount - 1.0;
			const float roughnessMip = log2(max(faceSize * roughness * roughness * 0.5, 1.0));
			const float screenMip = min(reflScreenMip, maxMip - 5.0); // At most 32x32 per face
			const float reflMip = clamp(max(roughnessMip, screenMip), 0.0, maxMip);
			skyBoxReflCol = SRGBToLinear(skybox.SampleLevel(sampColor, skyBoxReflected, reflMip).rgb);
		}
		if (env_sh_valid > 0.5)
			skyBoxNormalCol = EvalEnvSH(normalize(skyBoxNormal));
		else if (cubeMappingEnabled == 2)
			skyBoxNormalCol = SRGBToLinear(illumSkybox.SampleLevel(sampColor, skyBoxNormal, cubeMapMipLevel).rgb);
		else
			skyBoxNormalCol = SRGBToLinear(skybox.SampleLevel(sampColor, skyBoxNormal, cubeMapMipLevel).rgb);
	}
	// The ambient light is pbr_ambient everywhere, tinted by ambient_tint (pbr_ambient_color),
	// for instance to make shadows bluish. When there's a cubemap, its light is added on top:
	// it lights the surfaces from all directions (diffuse) and it's reflected by them (specular).
	const float3 baseAmbient = ambient_tint * ambient;
	// ambient_tint (pbr_ambient_color) tints the cubemap's ambient light too. Its reflections
	// aren't tinted: they're the image of the surroundings.
	const float3 envDiffuse  = cubeMappingEnabled ? env_ambient_mult * ambient_tint * cubeMapAmbientInt * skyBoxNormalCol : 0.0;
	const float3 envSpecular = cubeMappingEnabled ? env_specular_mult * cubeMapSpecInt * skyBoxReflCol : 0.0;

	// Direct light from the XWA lights. Their highlights don't get the black level of the
	// shadows (see specular_shadow_factor()), so there are none in a full shadow.
	const float spec_shadow_factor = specular_shadow_factor(total_shadow_factor, get_shadow_caster_black_level(LightCount));
	float3 direct = 0.0;
	float4 tmp_bloom = 0.0;
	uint i;
	[loop]
	for (i = 0; i < LightCount; i++)
	{
		float black_level, dummyMinZ, dummyMaxZ;
		float shadow_factor = 1.0;
		float light_modulation = 1.0;
		float is_shadow_caster = 1.0;
		get_black_level_and_minmaxZ(i, black_level, dummyMinZ, dummyMaxZ);
		// Skip lights that won't project black-enough shadows, we'll take care of them later
		if (black_level > 0.95)
		{
			// This is not a shadow caster, ignore RT and SM shadows:
			shadow_factor = 1.0;
			// Dim the light a bit
			light_modulation = 0.25;
			is_shadow_caster = 0.0;
		}
		else
		{
			// This is a shadow caster, apply RT and SM shadows
			shadow_factor = total_shadow_factor;
			light_modulation = 1.0;
			is_shadow_caster = 1.0;
		}

		// Lights come with Z inverted from ddraw, so they expect negative Z values in front of the camera
		float3 L = LightVector[i].xyz;
		L.xy = -L.xy;
		L = normalize(L);

		// The cubemap doesn't tint the lights: its reflections are added with the ambient light
		const float3 lightColor = light_modulation * LightColor[i].rgb;

		float3 diffuseBRDF, specularBRDF;
		EvaluateDirectBRDF(N_PBR, V_PBR, L, albedo, metallicity, F0, roughness, diffuseBRDF, specularBRDF);
		// See the light convention in PBRShadingV2.h: the irradiance is PI * lightColor * dotNL
		const float3 irradiance = PI * lightColor * saturate(dot(N_PBR, L));
		const float3 diffuse  = diffuseBRDF  * irradiance * shadow_factor;
		const float3 specular = specularBRDF * irradiance * (is_shadow_caster > 0.5 ? spec_shadow_factor : 1.0);

		// When there's a glass material, we want to lerp between the current color and
		// the specular reflection, so that we get a nice white spot when glass reflects
		// light.
		const float spec_val = dot(0.333, specular);
		const float glass_interpolator = lerp(0, saturate(spec_val), glass);
		direct += lerp(diffuse + specular, specular, glass_interpolator);

		// Add some bloom where appropriate:
		// only-shadow-casters-emit-bloom * Glass-blooms-more-than-other-surfaces * excess-specular-energy
		const float excess_energy = smoothstep(0.95, 4.0, spec_val);
		const float spec_bloom = is_shadow_caster * lerp(0.4, 1.25, glass) * excess_energy;
		tmp_bloom += spec_shadow_factor * spec_bloom;
	}
	output.bloom = tmp_bloom;

	// Ambient light, added once. It's treated as uniform light coming from all directions,
	// so it has a diffuse and a specular component.
	// The ambient occlusion darkens the diffuse ambient light. For the reflections, it's
	// turned into specular occlusion: with GTAO, the part of the reflection's cone that's
	// outside the visibility cone (ConeSpecularOcclusion, pbr_gtao_specular_occlusion = 1);
	// otherwise, an estimate from the AO, the roughness and the view angle (Lagarde & de
	// Rousiers, "Moving Frostbite to PBR", 2014).
	float specOcclusion = saturate(pow(abs(dotNV + ao), exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
	[branch]
	if (bGTAO && gtao_cone_spec_occlusion > 0.5)
	{
		// The directions are in the GTAO space: Z+ away from the camera
		specOcclusion = ConeSpecularOcclusion(ao, normalize(gtao.xyz),
			normalize(float3(Normal.x, Normal.y, -Normal.z)), reflDirView, roughness);
	}
	// Multi-bounce only applies to the diffuse light: the reflections keep the plain AO
	const float3 ambientDiffuse  = GTAODiffuseAO(ao, albedo) * (baseAmbient + envDiffuse) * albedo * (1.0 - metallicity);
	const float3 ambientSpecular = specOcclusion * (baseAmbient + envSpecular) * EnvBRDFApprox(F0, roughness, dotNV);
	// The indirect light (pbr_gi_*): the light the surface receives from the surfaces around
	// it, in the same units as the cubemap's ambient light, so it's added the same way. The
	// gather's visibility bitmask already accounts for what the surface sees, so it isn't
	// multiplied by the AO.
	float3 indirectDiffuse = 0.0;
	[branch]
	if (gi_enabled > 0.5)
	{
#ifdef INSTANCED_RENDERING
		const uint giViewId = input.viewId;
#else
		const uint giViewId = 0;
#endif
		indirectDiffuse = gi_intensity * LoadIndirectLight(int2(input.pos.xy), giViewId, P.z) *
			albedo * (1.0 - metallicity);
	}
	float3 tmp_color = ambientDiffuse + ambientSpecular + direct + indirectDiffuse;

	// Debug view (pbr_gi_debug = 4): only the indirect light's contribution to the surface
	// (pbr_gi_intensity, the albedo), exposed and tonemapped like the final colors
	[branch]
	if (gi_enabled > 0.5 && gi_debug > 3.5)
	{
		output.color = float4(lerp(background, LinearToSRGB(ACESFitted(exposure_mult * indirectDiffuse)), blendAlpha), 1);
		output.bloom = 0;
		return output;
	}

	// Debug view (pbr_cubemap_debug): only the cubemap's light, on the surfaces.
	// 1: the sky the surface reflects, as a perfect mirror with the blur of its roughness
	//    (no strength or Fresnel): to check how sharp the reflections are.
	// 2: the cubemap's reflections as they're added to the lighting (Specular,
	//    pbr_cubemap_specular, Fresnel and specular occlusion).
	// 3: the cubemap's ambient light on the surface (AmbientInt, pbr_cubemap_ambient, AO).
	// 2 and 3 are exposed and tonemapped like the final colors.
	[branch]
	if (env_debug > 0.5)
	{
		float3 debugColor = skyBoxReflCol;
		if (env_debug > 2.5)
			debugColor = ACESFitted(exposure_mult * GTAODiffuseAO(ao, albedo) * envDiffuse * albedo * (1.0 - metallicity));
		else if (env_debug > 1.5)
			debugColor = ACESFitted(exposure_mult * specOcclusion * envSpecular * EnvBRDFApprox(F0, roughness, dotNV));
		output.color = float4(lerp(background, LinearToSRGB(debugColor), blendAlpha), 1);
		output.bloom = 0;
		return output;
	}

	// Apply the shadeless mask: shadeless surfaces show their own color
	tmp_color = lerp(tmp_color, color, shadeless);

	// Add the laser/dynamic lights. This is the same as in PBRAddCommon.h, plus the GTAO
	// occlusion below.
	// These lights have no shadows, so GTAO occludes them (pbr_gtao_light_occlusion): a
	// laser going by doesn't light the inside of a trench or the far side of a greeble.
	// GTAO gives each pixel a visibility cone, the directions it sees open space through
	// (Jimenez et al. 2016): the bent normal is its axis, and its half angle comes from the
	// AO, cos = sqrt(1 - AO) (no occlusion: the whole hemisphere). A light inside the cone
	// isn't occluded, and it fades out past the cone's edge. GTAO doesn't know how far the
	// occluders are, so a light closer than them is occluded too, but it only finds
	// occluders within its radius.
	const float lightOcclusion = bGTAO ? gtao_light_occlusion : 0.0;
	// The bent normal is Z+ away from the camera; N and L below are Z+ towards it
	const float3 lightConeAxis = normalize(float3(gtao.x, gtao.y, -gtao.z));
	const float lightConeCos = sqrt(saturate(1.0 - ao));
	const float3 laser_light_sum = ComputeDynamicLights(pos3D, N, lightOcclusion, lightConeAxis, lightConeCos);
	tmp_color += laser_light_intensity * laser_light_sum;

	// Tonemap and convert back to sRGB
	tmp_color = ACESFitted(exposure_mult * tmp_color);
	output.color.rgb = LinearToSRGB(tmp_color);
	// Color grading (pbr_grade_*). Only the lit surfaces get it: the background and the
	// shadeless surfaces (displays, engine glows) keep their colors.
	output.color.rgb = ApplyColorGrade(output.color.rgb, grade_contrast, grade_saturation,
		grade_shadows, grade_highlights);
	output.color = float4(lerp(background, output.color.rgb, blendAlpha), 1);

#ifdef INSTANCED_RENDERING
	output.color = BlendTransparentLayers(output.color, transpColor1, transpColor2);
#else
	output.color = BlendTransparentLayers(output.color, transpColor1, transpColor2, reticleColor);
#endif
	return output;
}
#endif // GI_BOUNCE_LIGHT_PASS
