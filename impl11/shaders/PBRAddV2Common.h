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
 *   tinting the lights. pbr_ambient_color tints the base ambient and the cubemap's.
 * - An optional color grading step (pbr_grade_*) on the final colors of the lit surfaces.
 * - GTAO (pbr_gtao_*): ambient occlusion and bent normals for the ambient light. It replaces
 *   the SSDO input of the legacy shading, which this pass doesn't use. An optional coarse
 *   pass (pbr_gtao_coarse*) adds the broad occlusion of large structures. Multi-bounce AO
 *   (pbr_gtao_multibounce, Ctrl+Alt+U) lightens it on bright surfaces, see GTAOMultiBounce().
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
// across the edges of objects. maxPx is the last GTAO pixel that was rendered.
float4 LoadGTAO(GTAO_TEXTURE tex, int scale, int2 maxPx, int2 px, uint viewId, float z)
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
		result = weightSum > 1e-4 ? sum / weightSum : GTAO_LOAD(min(base, maxPx));
		result.xyz = length(result.xyz) > 1e-4 ? normalize(result.xyz) : result.xyz;
	}
	// The GTAO buffers keep values above 1 (and 2 for the skipped pixels), see GTAOCommon.h
	result.a = saturate(result.a);
	return result;
#undef GTAO_LOAD
#undef GTAO_DEPTH
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

// The AO of the diffuse ambient light: GTAO with multi-bounce (gtao_multibounce = 0 is plain
// GTAO). It's applied to the combined AO of both passes: it's a function of all the
// occlusion around a point, and applying it to each pass separately would lighten crevices
// too much.
float3 GTAODiffuseAO(float ao, float3 albedo)
{
	return lerp(ao, GTAOMultiBounce(ao, albedo), gtao_multibounce);
}

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
		gtao = LoadGTAO(texGTAO, fineScale, fineMaxPx, px, viewId, pos3D.z);
		fineAO = gtao.a;
		// Two-scale AO: the coarse pass adds the broad occlusion of large structures. The
		// visibilities multiply, and the bent normal turns by the sum of both deviations
		// from the normal. Where one of them sees no occluders, it changes nothing.
		[branch]
		if (gtao_coarse_enabled > 0.5)
		{
			const float4 coarse = LoadGTAO(texGTAOCoarse, 4, GTAOBufferSize(texGTAOCoarse) - 1, px, viewId, pos3D.z);
			const float3 n = normalize(float3(Normal.x, Normal.y, -Normal.z));
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

	// We need to invert the Z-axis for illumination because the normals are Z+ when pointing to the camera
	// so that implies that Z increases towards the viewer and decreases away from the camera.
	const float3 P = pos3D.xyz;
	pos3D.z = -pos3D.z;

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

	// ************************************************************************************************
	// Material properties. These heuristics are the same as in PBRAddCommon.h.
	// Approximate luma:
	const float Value = max(dot(float3(0.299f, 0.587f, 0.114f), color.xyz), spec_val_mask);
	const float blackish = smoothstep(0.1, 0.0, Value);
	const float metallicity = (mask < METAL_HI) ? metallic : 0.25;
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

	const float roughness = 1.0 - glossiness * glossiness;
	const float3 F0 = 0.16 * reflectance * reflectance * (1.0 - metallicity) + albedo * metallicity;

	// The lighting below is done in the same space as PBRAddCommon.h: N, V and L are all
	// negated with respect to view space, so the dot products are the same.
	float3 N_PBR = N;
	N_PBR.xy = -N_PBR.xy;
	const float3 V_PBR = normalize(P);
	const float dotNV = saturate(dot(N_PBR, V_PBR));

	// Sample the cubemaps. They tint the light colors and provide the ambient light.
	// pos3D.z is inverted because it was just inverted above:
	const float3 viewDir   = normalize(float3(pos3D.x, pos3D.y, -pos3D.z));
	// Normal.z is inverted because Z+ should point away from the camera:
	float3 skyBoxReflected = reflect(viewDir, normalize(float3(Normal.x, Normal.y, -Normal.z)));
	// The bent normal is in the same space (Z+ away from the camera)
	float3 skyBoxNormal    = bGTAO ? normalize(gtao.xyz) : normalize(float3(Normal.x, Normal.y, -Normal.z));
	skyBoxReflected        = mul(viewMat, float4(skyBoxReflected, 0)).xyz;
	skyBoxNormal           = mul(viewMat, float4(skyBoxNormal, 0)).xyz;
	float3 skyBoxReflCol   = 1;
	float3 skyBoxNormalCol = 1;
	// Sample the hi-res cubemap:
	if (cubeMappingEnabled == 1)
	{
		skyBoxReflCol   = skybox.SampleLevel(sampColor, skyBoxReflected.xyz, cubeMapMipLevel).rgb;
		skyBoxNormalCol = skybox.SampleLevel(sampColor, skyBoxNormal.xyz, cubeMapMipLevel).rgb;
	}
	// Sample the lo-res cubemap:
	else if (cubeMappingEnabled == 2)
	{
		skyBoxReflCol   = illumSkybox.SampleLevel(sampColor, skyBoxReflected.xyz, cubeMapMipLevel).rgb;
		skyBoxNormalCol = illumSkybox.SampleLevel(sampColor, skyBoxNormal.xyz, cubeMapMipLevel).rgb;
	}
	// The ambient light is pbr_ambient everywhere, tinted by ambient_tint (pbr_ambient_color),
	// for instance to make shadows bluish. When there's a cubemap, its light is added on top:
	// it lights the surfaces from all directions (diffuse) and it's reflected by them (specular).
	const float3 baseAmbient = ambient_tint * ambient;
	// ambient_tint (pbr_ambient_color) tints the cubemap's ambient light too. Its reflections
	// aren't tinted: they're the image of the surroundings.
	const float3 envDiffuse  = cubeMappingEnabled ? ambient_tint * cubeMapAmbientInt * skyBoxNormalCol : 0.0;
	const float3 envSpecular = cubeMappingEnabled ? cubeMapSpecInt * skyBoxReflCol : 0.0;

	// Direct light from the XWA lights
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
		const float3 irradiance = PI * lightColor * saturate(dot(N_PBR, L)) * shadow_factor;
		const float3 diffuse  = diffuseBRDF  * irradiance;
		const float3 specular = specularBRDF * irradiance;

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
		tmp_bloom += total_shadow_factor * spec_bloom;
	}
	output.bloom = tmp_bloom;

	// Ambient light, added once. It's treated as uniform light coming from all directions,
	// so it has a diffuse and a specular component.
	// The ambient occlusion darkens the diffuse ambient light. For the reflections, it's
	// turned into specular occlusion, which also depends on the roughness and the view angle
	// (Lagarde & de Rousiers, "Moving Frostbite to PBR", 2014).
	const float specOcclusion = saturate(pow(abs(dotNV + ao), exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
	// Multi-bounce only applies to the diffuse light: the reflections keep the plain AO
	const float3 ambientDiffuse  = GTAODiffuseAO(ao, albedo) * (baseAmbient + envDiffuse) * albedo * (1.0 - metallicity);
	const float3 ambientSpecular = specOcclusion * (baseAmbient + envSpecular) * EnvBRDFApprox(F0, roughness, dotNV);
	float3 tmp_color = ambientDiffuse + ambientSpecular + direct;

	// Apply the shadeless mask: shadeless surfaces show their own color
	tmp_color = lerp(tmp_color, color, shadeless);

	// Add the laser/dynamic lights. This is the same as in PBRAddCommon.h
#define L_FADEOUT_A_0 30.0
#define L_FADEOUT_A_1 50.0
#define L_FADEOUT_B_0 50.0
#define L_FADEOUT_B_1 1000.0
	float3 laser_light_sum = 0.0;
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
		laser_light_sum += depth_attenuation * attenuation * angle_attenuation * diff_val * LightPointColor[i].rgb;
	}
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
