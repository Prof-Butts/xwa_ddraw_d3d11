/*
 * Ground Truth Ambient Occlusion (GTAO) with bent normals, used by ssao_type = PBR.
 * Licensed under the MIT license. See LICENSE.txt
 *
 * Based on "Practical Real-Time Strategies for Accurate Indirect Occlusion" (Jimenez et al.,
 * 2016) and Intel's XeGTAO (MIT license). For each pixel, a few directions ("slices") are
 * searched in screen space for the highest horizon on both sides; the visible arc between
 * the two horizons gives the cosine-weighted visibility of that slice. The bent normal (the
 * least occluded direction) is the normal turned towards the middle of the visible arcs.
 *
 * The horizons are measured as the elevation of the samples above the tangent plane (their
 * height along the normal), not as their angle from the view vector. The sample offsets are
 * rounded to whole pixels, which can move a sample off its slice; measured from the view
 * vector, a sample that's on the surface could then look like an occluder. Its elevation is
 * still 0, wherever it lands.
 *
 * The positions are stored as 16-bit floats, so they're rounded to steps of about 1/1024 of
 * their size: about 2 mm at 2 m, which is as big as a pixel at 4K. A flat surface is stored
 * as a staircase, and to the closest samples each step looks like a steep occluder (contour
 * bands that follow the depth). gtao_height_bias lowers every sample's height above the
 * tangent plane by that many steps, so the staircase stops counting and real occluders, which
 * are much taller, still count. It's the same as moving the origin of the samples along the
 * normal, but the distances (and the falloff) don't change.
 *
 * Output: rgb = bent normal (view space, same convention as the position buffer), a = AO
 * (1 = fully visible). The AO isn't clamped to 1 here: with few slices, open surfaces get
 * values a bit above and below 1 that the denoise filter averages to 1, and clamping first
 * would leave them slightly occluded. Pixels without AO (the background, transparent
 * surfaces) get GTAO_SKIPPED. The noise of the slice and step offsets repeats every 4x4
 * pixels, so the 4x4 filter in GTAODenoise.hlsl averages it out.
 *
 * At half resolution (gtao_scale = 2), this pass renders to the top-left quarter of the
 * GTAO buffer, and each of its pixels uses the top-left pixel of a 2x2 block of the full
 * resolution buffers. The radius and the sample offsets are still in full-resolution
 * pixels, so the result looks the same, only with less detail.
 *
 * The position buffer has the view-space position of each pixel (X+ right, Y+ up, Z+ away
 * from the camera), in meters. The normal buffer has Z+ pointing to the camera, so its Z is
 * flipped here. The projection isn't needed: the view ray of a pixel is P.xy / P.z, so the
 * size of a pixel at any depth is measured from the rays of nearby pixels. That works in
 * VR too, where each eye has its own projection.
 *
 * GTAO_GI (GTAOGI.hlsl, pbr_gi_enabled) adds the indirect light: one bounce of the light that
 * the surfaces send back out (the bounce light, GIBounceLight.hlsl), gathered with the same
 * slices and samples. It's the visibility-bitmask method ("Screen Space Indirect Lighting
 * with Visibility Bitmask", Therrien et al. 2023): each slice's half circle above the surface
 * is split into 32 sectors. A sample covers the sectors between its front and its back (the
 * back is gtao_gi_thickness further away from the camera, so thin objects don't hide
 * everything behind them), and the sectors it covers first, not yet covered by a closer
 * sample, add its bounce light. Each sector is weighted by its cosine to the normal, so a
 * slice fully covered by a uniform light L gathers L: the result is the average incoming
 * light weighted by the cosine (irradiance / PI), the same units as the cubemap's ambient.
 * Neither gather uses the AO, so the GTAO_GI variant only outputs the indirect light (rgb,
 * a = 1; a = 0 for the pixels GTAO skips, GI_EMPTY for the ones with no light nearby), and
 * the compiler drops the AO's math.
 *
 * The indirect light needs a much larger radius than the AO (light bounces off surfaces
 * tens of centimeters away, while the AO is about contact), so it runs in its own pass, at a
 * quarter of the resolution, with its own radius (pbr_gi_radius, pbr_gi_max_radius): see
 * GTAOPass() in ddraw. Its AO output isn't used. The bounce light is at 1 / gtao_bounce_scale
 * of the resolution (also a quarter).
 *
 * With gtao_mips (pbr_gtao_mips for the AO, pbr_gi_emitter_mips for the emitters' gather),
 * the distant samples come from mips of the positions and normals (GIGeomMipsCommon.h), like
 * XeGTAO reads its depth: the level grows with the distance (log2 of the distance in
 * half-resolution pixels, minus gtao_mip_offset), so the samples spread across the screen
 * stay in the texture cache. The closest samples still read the full-resolution buffers. The
 * emitters' gather also reads their light from its mips (gtao_light_mips), so the distant
 * light is averaged instead of point-sampled; the bounced light (pbr_gi_mips) has no mips, so
 * its gather reads the positions and normals from the mips, and the light at full detail.
 * The mips only exist when the indirect light is on.
 *
 * GTAO_BITMASK_AO (GTAOBitmask.hlsl, pbr_gtao_bitmask_ao) computes the AO and the bent normal
 * with the same kind of visibility bitmask, instead of the two horizons: each sample hides
 * the sectors between its front and its back (gtao_thickness further away from the camera),
 * so a thin object (a strut, an antenna) only hides what's right behind it, while the
 * horizons treat it as an infinitely deep wall. Each visible sector counts as much as it does
 * in the regular GTAO (the integral of its arc), so with deep occluders the result is the
 * same; the bent normal points to the weighted middle of the visible sectors. The biases and
 * the falloff lower the samples like they lower the horizons.
 */
#include "GTAOCBuffer.h"

#ifdef INSTANCED_RENDERING
Texture2DArray texPos    : register(t0);
Texture2DArray texNormal : register(t1);
#else
Texture2D texPos    : register(t0);
Texture2D texNormal : register(t1);
#endif

// The positions and normals with mips (gtao_mips, GIGeomMipsCommon.h)
#ifndef GTAO_GI
#define GEOM_PACKING_SM4 1
#endif
#include "GIGeomPacking.h"
#ifdef INSTANCED_RENDERING
Texture2DArray<uint4> texGeomMips : register(t3);
#else
Texture2D<uint4> texGeomMips : register(t3);
#endif

#ifdef GTAO_GI
// The bounce light (GIBounceLight.hlsl), at this pass's resolution
#ifdef INSTANCED_RENDERING
Texture2DArray texBounce : register(t2);
#else
Texture2D texBounce : register(t2);
#endif

// The pixels skipped because there's no light nearby (gtao_gi_presence): no light, but valid,
// so the filters blend them as 0. They return early from the filters.
static const float GI_EMPTY = 0.75;
#define GTAO_RETURN(gtaoValue, giValue) return (giValue);
#else
#define GTAO_RETURN(gtaoValue, giValue) return (gtaoValue);
#endif

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

static const float GTAO_PI      = 3.14159265;
static const float GTAO_PI_HALF = 1.57079633;
// Positions this far away are the background (the position buffer is cleared to Z = 32000)
static const float GTAO_FAR_Z   = 30000.0;
// The AO of the pixels that GTAO skips. Above any real AO, so the denoise filter can tell
// them apart; the lighting clamps it to 1.
static const float GTAO_SKIPPED = 2.0;

float3 LoadPos(int2 px, uint viewId)
{
	px = clamp(px, int2(0, 0), int2(gtao_buffer_size) - 1);
#ifdef INSTANCED_RENDERING
	return texPos.Load(int4(px, viewId, 0)).xyz;
#else
	return texPos.Load(int3(px, 0)).xyz;
#endif
}

float4 LoadNormal(int2 px, uint viewId)
{
#ifdef INSTANCED_RENDERING
	return texNormal.Load(int4(px, viewId, 0));
#else
	return texNormal.Load(int3(px, 0));
#endif
}

// Fast approximations for the indirect light's gather: its angles only pick one of 32
// sectors (PI/32 = 0.098 radians wide), so a few thousandths of a radian don't matter, and
// they made the gathers about 3% faster (measured). The AO keeps the exact functions.
// acos: XeGTAO's (max error about 0.0068 radians).
float FastAcos(float x)
{
	float res = -0.156583 * abs(x) + GTAO_PI_HALF;
	res *= sqrt(1.0 - abs(x));
	return x >= 0.0 ? res : GTAO_PI - res;
}

// atan2: a polynomial for atan on 0..1 (max error about 1e-5 radians), unfolded to the
// four quadrants
float FastAtan2(float y, float x)
{
	const float ax = abs(x), ay = abs(y);
	const float a = min(ax, ay) / max(max(ax, ay), 1e-20);
	const float s = a * a;
	float r = ((-0.0464964749 * s + 0.15931422) * s - 0.327622764) * s * a + a;
	r = ay > ax ? GTAO_PI_HALF - r : r;
	r = x < 0.0 ? GTAO_PI - r : r;
	return y < 0.0 ? -r : r;
}

bool IsGeometry(float3 P)
{
	return P.z > 0.0 && P.z < GTAO_FAR_Z;
}

// Change of the view ray (P.xy / P.z) per pixel along one axis, measured against the pixel
// `dist` pixels away in the positive (side = 1) or negative (side = -1) direction. Returns
// 0 if there's no geometry there to compare with.
float RayChange(int2 px, float2 ray, int axis, int dist, int side, uint viewId)
{
	const int2 dir = axis == 0 ? int2(1, 0) : int2(0, 1);
	const float3 S = LoadPos(px + dir * (dist * side), viewId);
	if (!IsGeometry(S))
		return 0;
	return ((S.xy / S.z)[axis] - ray[axis]) / (dist * side);
}

// Change of the view ray (P.xy / P.z) per pixel, along x and along y. It's signed, so it
// also tells how the screen axes map to the view axes. The position buffer is half
// precision, so the rays are compared over several pixels to keep the error small. The
// other pixels are only read when the previous ones aren't geometry, so it's usually two
// reads. Returns 0 on an axis when there's no geometry to compare with.
float2 RayChangePerPixel(int2 px, float3 P, uint viewId)
{
	const float2 ray = P.xy / P.z;
	float2 result = 0;
	[unroll]
	for (int axis = 0; axis < 2; axis++)
	{
		result[axis] = RayChange(px, ray, axis, 16, 1, viewId);
		[branch]
		if (result[axis] == 0)
			result[axis] = RayChange(px, ray, axis, 16, -1, viewId);
		[branch]
		if (result[axis] == 0)
			result[axis] = RayChange(px, ray, axis, 4, 1, viewId);
		[branch]
		if (result[axis] == 0)
			result[axis] = RayChange(px, ray, axis, 4, -1, viewId);
	}
	return result;
}

// The texel of texGeomMips that covers full-resolution pixel samplePx, at a mip level
int2 GeomTexel(int2 samplePx, int level)
{
	const uint2 p = uint2(clamp(samplePx, int2(0, 0), int2(gtao_buffer_size) - 1));
	return int2((p / GI_GEOM_SCALE) >> (uint)level);
}

uint4 LoadGeomMip(int2 samplePx, int level, uint viewId)
{
#ifdef INSTANCED_RENDERING
	return texGeomMips.Load(int4(GeomTexel(samplePx, level), viewId, level));
#else
	return texGeomMips.Load(int3(GeomTexel(samplePx, level), level));
#endif
}

// The position of the sample at full-resolution pixel samplePx: from the full-resolution
// buffer when level < 0, otherwise from that mip level of texGeomMips
float3 LoadSamplePos(int2 samplePx, int level, uint viewId)
{
	float3 S;
	[branch]
	if (level >= 0)
		S = UnpackGeomPos(LoadGeomMip(samplePx, level, viewId));
	else
		S = LoadPos(samplePx, viewId);
	return S;
}

// The last mip level of texGeomMips that the distant samples can read; -1 without gtao_mips
int LastGeomLevel()
{
	int lastLevel = -1;
	[branch]
	if (gtao_mips > 0.5)
	{
		uint geomWidth, geomHeight, geomLevels;
#ifdef INSTANCED_RENDERING
		uint geomElements;
		texGeomMips.GetDimensions(0, geomWidth, geomHeight, geomElements, geomLevels);
#else
		texGeomMips.GetDimensions(0, geomWidth, geomHeight, geomLevels);
#endif
		lastLevel = (int)geomLevels - 1;
	}
	return lastLevel;
}

// The mip level of a sample offset pixels away (in full-resolution pixels): -1 (the full
// resolution) up to lastLevel
int SampleGeomLevel(float2 offset, int lastLevel)
{
	int level = -1;
	[branch]
	if (lastLevel >= 0)
	{
		const float offsetPx = length(offset) / GI_GEOM_SCALE;
		level = min((int)floor(log2(max(offsetPx, 1.0)) - gtao_mip_offset), lastLevel);
		level = max(level, -1);
	}
	return level;
}

#ifdef GTAO_GI

// The indirect light that sample S (at full-resolution pixel samplePx, read at mip level
// level, see LoadSamplePos()) adds to the pixel at P, in a slice: sliceDir is the slice's
// direction perpendicular to V, n the angle of the projected normal from V (positive towards
// sliceDir). sectors is the slice's visibility bitmask: the sample covers the sectors between
// its front and its back, and only the ones that weren't covered by a closer sample add its
// light. weight is GTAO's falloff (0: not geometry).
float3 GatherIndirect(float3 P, float3 S, int2 samplePx, int level, uint viewId, float weight,
	float3 V, float3 sliceDir, float n, inout uint sectors)
{
	if (weight <= 0.0)
		return 0;
	const float3 delta = S - P;
	// The back of the sample: gtao_gi_thickness further away from the camera
	const float3 deltaBack = delta + normalize(S) * gtao_gi_thickness;
	// Their angles in the slice plane, from the normal (-PI/2 and PI/2 are the tangent plane).
	// atan2 uses the projection on the slice plane, so samples that the rounding moved off the
	// slice still get the right angle.
	const float thetaFront = FastAtan2(dot(delta, sliceDir), dot(delta, V)) - n;
	const float thetaBack  = FastAtan2(dot(deltaBack, sliceDir), dot(deltaBack, V)) - n;
	// The sectors between them (0..1 across the half circle above the surface)
	const float2 range = saturate((float2(thetaFront, thetaBack) + GTAO_PI_HALF) / GTAO_PI);
	const float a = min(range.x, range.y);
	const float b = max(range.x, range.y);
	const uint start = min((uint)(a * 32.0), 31u);
	const uint count = min((uint)ceil((b - a) * 32.0), 32u - start);
	const uint bits = (count >= 32 ? 0xFFFFFFFFu : ((1u << count) - 1u)) << start;
	const uint newBits = bits & ~sectors;
	sectors |= bits;
	[branch]
	if (newBits == 0)
		return 0;

	// The sample's surface must face the pixel: otherwise the pixel would see its back
	float3 sampleNormal;
	[branch]
	if (level >= 0)
		sampleNormal = UnpackGeomNormal(LoadGeomMip(samplePx, level, viewId));
	else
		sampleNormal = LoadNormal(samplePx, viewId).xyz;
	const float3 sampleN = normalize(float3(sampleNormal.x, sampleNormal.y, -sampleNormal.z));
	const float facing = smoothstep(-0.1, 0.1, dot(sampleN, -normalize(delta)));

	// Its bounce light, from the bounce light's pixel that covers samplePx. When the light has
	// mips (gtao_light_mips: the emitters'), at the level with the same texel size as the
	// position's: the average light of the area the position stands for. The light's level 0
	// is at 1 / gtao_bounce_scale of the resolution, the positions' at 1 / GI_GEOM_SCALE.
	const int bounceShift = (int)round(log2(gtao_bounce_scale / GI_GEOM_SCALE));
	const int lightLevel = gtao_light_mips > 0.5 ? max(level - bounceShift, 0) : 0;
	const uint2 clampedPx = uint2(clamp(samplePx, int2(0, 0), int2(gtao_buffer_size) - 1));
	const int2 bouncePx = int2((clampedPx / (uint)gtao_bounce_scale) >> (uint)lightLevel);
#ifdef INSTANCED_RENDERING
	const float3 bounce = texBounce.Load(int4(bouncePx, viewId, lightLevel)).rgb;
#else
	const float3 bounce = texBounce.Load(int3(bouncePx, lightLevel)).rgb;
#endif
	// Each sector is PI/32 wide; with the cosine at the middle of the sample, a full half
	// circle adds up to 2, so the weight is PI/64 per sector
	const float thetaMid = 0.5 * (a + b) * GTAO_PI - GTAO_PI_HALF;
	return bounce * (countbits(newBits) * (GTAO_PI / 64.0) * cos(thetaMid) * facing * weight);
}
#endif

#ifdef GTAO_BITMASK_AO
// The sectors (32 across the slice's half circle above the surface) that sample S hides from
// the pixel at P: between its front and its back, gtao_thickness further away from the
// camera. The angles are measured from the normal in the slice plane, like
// GatherIndirect(); sliceDir is the slice's direction perpendicular to V, and n the angle of
// the projected normal from V. Like the horizons of the regular GTAO, the sample is lowered
// by heightBias along the normal and by angleBias towards the tangent plane, and the falloff
// weight brings it down to the tangent plane.
uint OccludedSectors(float3 P, float3 S, float weight, float3 N, float3 V, float3 sliceDir,
	float n, float heightBias, float angleBias)
{
	if (weight <= 0.0)
		return 0;
	const float3 front = S - P - N * heightBias;
	const float3 back = front + normalize(S) * gtao_thickness;
	float2 theta = float2(atan2(dot(front, sliceDir), dot(front, V)), atan2(dot(back, sliceDir), dot(back, V))) - n;
	theta = clamp(theta, -GTAO_PI_HALF, GTAO_PI_HALF);
	// The tangent plane on the sample's side
	const float2 tangent = float2(theta.x >= 0.0 ? GTAO_PI_HALF : -GTAO_PI_HALF,
	                              theta.y >= 0.0 ? GTAO_PI_HALF : -GTAO_PI_HALF);
	theta = clamp(theta + sign(tangent) * angleBias, -GTAO_PI_HALF, GTAO_PI_HALF);
	theta = lerp(tangent, theta, weight);
	const float2 range = (theta + GTAO_PI_HALF) / GTAO_PI;
	const float a = min(range.x, range.y);
	const float b = max(range.x, range.y);
	const uint start = min((uint)(a * 32.0), 31u);
	const uint count = min((uint)ceil((b - a) * 32.0), 32u - start);
	return (count >= 32 ? 0xFFFFFFFFu : ((1u << count) - 1u)) << start;
}

// The antiderivative of cos(t - n) * sin(t), for the sector weights: the regular GTAO's
// visible arcs are the integral of cos(t - n) * |sin(t)| (t: the angle from V)
float SectorArc(float t, float n, float sinN)
{
	return (2.0 * t * sinN - cos(2.0 * t - n)) / 4.0;
}
#endif

// 4x4 ordered (Bayer) pattern: each value 0..15 appears once in every 4x4 block
float Bayer4x4(int2 px)
{
	const uint2 p = uint2(px) & 3;
	const uint bayer[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
	return (float)bayer[p.y * 4 + p.x];
}

float4 main(PixelShaderInput input) : SV_TARGET
{
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
#else
	const uint viewId = 0;
#endif
	// This pass's pixel, and the full-resolution pixel it uses
	const int2 outPx = int2(input.pos.xy);
	const int2 px = outPx * (int)gtao_scale;
	const float3 P = LoadPos(px, viewId);
	const float4 normalSample = LoadNormal(px, viewId);
	// The normal in the same space as the positions
	const float3 N = normalize(float3(normalSample.x, normalSample.y, -normalSample.z));

	// Background, and surfaces that aren't fully opaque (their normal.w is low): no occlusion
	if (!IsGeometry(P) || normalSample.w < 0.5)
		GTAO_RETURN(float4(N, GTAO_SKIPPED), 0)

	// Size of a pixel at this depth (both axes have the same size), and the signs that map
	// screen directions to view directions
	const float2 rayChange = RayChangePerPixel(px, P, viewId);
	const float2 absChange = abs(rayChange);
	const float pixelAngle = (absChange.x > 0 && absChange.y > 0) ? 0.5 * (absChange.x + absChange.y) :
		max(absChange.x, absChange.y);
	if (pixelAngle <= 0)
		GTAO_RETURN(float4(N, GTAO_SKIPPED), 0)
	// Screen y grows downwards and view y grows upwards, so that's the default
	const float2 axisSign = float2(rayChange.x != 0 ? sign(rayChange.x) : 1.0,
	                               rayChange.y != 0 ? sign(rayChange.y) : -1.0);
	const float pixelSize = pixelAngle * P.z; // Meters per pixel at this depth

	// The radius of the effect, in pixels and in meters. It's clamped in pixels: close
	// surfaces don't sample half of the screen, and far ones still get some occlusion.
	const float screenRadius = clamp(gtao_radius / pixelSize, gtao_min_radius_px, gtao_max_radius_px);
#ifdef GTAO_GI
	// The emitters' gather (gtao_gi_presence) skips the pixels with no emitter within the
	// radius: emitters cover little of the screen. A mip level of their light averages the
	// area each texel covers, and the light is never negative, so 0 means no emitter there.
	// The level whose texels are at least as wide as the radius, read in the 3x3 texels
	// around the pixel, covers the whole radius.
	[branch]
	if (gtao_gi_presence > 0.5)
	{
		uint sourceWidth, sourceHeight, sourceLevels;
#ifdef INSTANCED_RENDERING
		uint sourceElements;
		texBounce.GetDimensions(0, sourceWidth, sourceHeight, sourceElements, sourceLevels);
#else
		texBounce.GetDimensions(0, sourceWidth, sourceHeight, sourceLevels);
#endif
		const uint level = (uint)min(ceil(log2(max(screenRadius / gtao_bounce_scale, 1.0))), (float)sourceLevels - 1.0);
		const int2 levelSize = max(int2(uint2(sourceWidth, sourceHeight) >> level), int2(1, 1));
		const int2 center = int2((uint2(px) / (uint)gtao_bounce_scale) >> level);
		float3 nearby = 0;
		[unroll]
		for (int y = -1; y <= 1; y++)
		{
			[unroll]
			for (int x = -1; x <= 1; x++)
			{
				const int2 t = clamp(center + int2(x, y), int2(0, 0), levelSize - 1);
#ifdef INSTANCED_RENDERING
				nearby += texBounce.Load(int4(t, viewId, level)).rgb;
#else
				nearby += texBounce.Load(int3(t, level)).rgb;
#endif
			}
		}
		if (max(nearby.r, max(nearby.g, nearby.b)) <= 0.0)
			return float4(0, 0, 0, GI_EMPTY);
	}
#endif
	const float radius = screenRadius * pixelSize;
	// Samples fade out between falloffFrom and the radius, so occluders at the edge of the
	// radius don't pop in and out
	const float falloffRange = gtao_falloff * radius;
	const float falloffFrom  = radius * (1.0 - gtao_falloff);
	const float falloffMul   = -1.0 / falloffRange;
	const float falloffAdd   = falloffFrom / falloffRange + 1.0;

	const float3 V = normalize(-P);
	// Angle bias: the elevation of the horizons is lowered by this angle, so the small errors
	// of the G-buffer (half-precision positions, smoothed normals) don't count as occluders.
	// Real occluders rise well above the tangent plane, so they still count.
	float sinBias, cosBias;
	sincos(gtao_angle_bias, sinBias, cosBias);
	// Height bias: gtao_height_bias steps of the 16-bit rounding of this position. A 16-bit
	// float has 10 mantissa bits, so its step is at most 1/1024 of its magnitude.
	const float3 absP = abs(P);
	const float heightBias = gtao_height_bias * max(absP.z, max(absP.x, absP.y)) / 1024.0;
	// The pattern is in this pass's pixels, so the 4x4 denoise filter averages it out. With the
	// GI temporal filter, gtao_noise_offset changes it every frame (each 4x4 block still has all
	// 16 values), so the history accumulates other directions.
	const float noise = fmod(Bayer4x4(outPx) + gtao_noise_offset, 16.0);
	const float noiseSlice = (noise + 0.5) / 16.0;
	// A different order of the same 16 values, so the slice and step offsets don't line up
	const float noiseStep = frac((noise * 5.0 + 0.5) / 16.0);

	const int sliceCount = (int)gtao_slices;
	const int stepCount  = (int)gtao_steps;
	float visibility = 0.0;
	float3 bentNormal = 0.0;
#ifdef GTAO_GI
	float3 indirect = 0.0;
#endif
	const int lastLevel = LastGeomLevel();

	[loop]
	for (int slice = 0; slice < sliceCount; slice++)
	{
		const float phi = (slice + noiseSlice) * GTAO_PI / sliceCount;
		const float2 omega = float2(cos(phi), sin(phi)); // Screen direction (pixels)
		const float3 dirVec = normalize(float3(omega * axisSign, 0.0)); // The same, in view space
		const float3 orthoDirVec = dirVec - dot(dirVec, V) * V;
		const float3 axisVec = normalize(cross(orthoDirVec, V));
		// The normal projected on the slice plane, and its angle from V (positive towards dirVec)
		const float3 projNormal = N - axisVec * dot(N, axisVec);
		const float projNormalLength = length(projNormal);
		const float signNorm = sign(dot(orthoDirVec, projNormal));
		const float cosNorm = saturate(dot(projNormal, V) / max(projNormalLength, 1e-4));
#ifdef GTAO_GI
		const float n = signNorm * FastAcos(cosNorm);
#else
		const float n = signNorm * acos(cosNorm);
#endif

		// The highest elevation above the tangent plane on each side, as a sine, measured in
		// the slice plane: dot(delta, N) / (dist * projNormalLength). The horizon starts at
		// the tangent plane (0).
		const float invProjNormalLength = 1.0 / max(projNormalLength, 1e-4);
		float elevation0 = 0.0; // Along +omega
		float elevation1 = 0.0; // Along -omega
#ifdef GTAO_GI
		// The indirect light of this slice, and its visibility bitmask
		const float3 sliceDir = normalize(orthoDirVec);
		uint sectors = 0;
		float3 sliceIndirect = 0.0;
#endif
#ifdef GTAO_BITMASK_AO
		// The sectors of this slice that the samples hide
		const float3 aoSliceDir = normalize(orthoDirVec);
		uint aoSectors = 0;
#endif

		[loop]
		for (int step = 0; step < stepCount; step++)
		{
			// More samples close to the pixel, where the occluders matter most
			float s = (step + noiseStep) / stepCount;
			s = s * s;
			const float2 offset = round(omega * max(s * screenRadius, 1.0));

			// The mip level of both samples (see the top of this file): -1 for the full resolution
			const int level = SampleGeomLevel(offset, lastLevel);
			const float3 S0 = LoadSamplePos(px + int2(offset), level, viewId);
			const float3 S1 = LoadSamplePos(px - int2(offset), level, viewId);
			const float3 delta0 = S0 - P;
			const float3 delta1 = S1 - P;
			const float dist0 = length(delta0);
			const float dist1 = length(delta1);
			const float weight0 = IsGeometry(S0) ? saturate(dist0 * falloffMul + falloffAdd) : 0.0;
			const float weight1 = IsGeometry(S1) ? saturate(dist1 * falloffMul + falloffAdd) : 0.0;
			const float sinElev0 = saturate((dot(delta0, N) - heightBias) / max(dist0, 1e-4) * invProjNormalLength);
			const float sinElev1 = saturate((dot(delta1, N) - heightBias) / max(dist1, 1e-4) * invProjNormalLength);
			elevation0 = max(elevation0, weight0 * sinElev0);
			elevation1 = max(elevation1, weight1 * sinElev1);
#ifdef GTAO_GI
			sliceIndirect += GatherIndirect(P, S0, px + int2(offset), level, viewId, weight0, V, sliceDir, n, sectors);
			sliceIndirect += GatherIndirect(P, S1, px - int2(offset), level, viewId, weight1, V, sliceDir, n, sectors);
#endif
#ifdef GTAO_BITMASK_AO
			aoSectors |= OccludedSectors(P, S0, weight0, N, V, aoSliceDir, n, heightBias, gtao_angle_bias);
			aoSectors |= OccludedSectors(P, S1, weight1, N, V, aoSliceDir, n, heightBias, gtao_angle_bias);
#endif
		}
#ifdef GTAO_GI
		// Like the visibility, the slice counts as much as the normal lies in its plane
		indirect += sliceIndirect * projNormalLength;
#endif

#ifdef GTAO_BITMASK_AO
		// The visible sectors, each weighted like the regular GTAO's arcs: the integral of
		// cos(t - n) * |sin(t)| over the sector (t: the angle from V, the sector spans
		// n - PI/2 .. n + PI/2). The parts on each side of V (t = 0) are integrated separately.
		const float sinN = sin(n);
		const float arcAtV = SectorArc(0.0, n, sinN);
		float sliceVisible = 0.0;
		float visibleAngleSum = 0.0;
		float ta = n - GTAO_PI_HALF;
		float arcA = SectorArc(ta, n, sinN);
		[loop]
		for (uint k = 0; k < 32; k++)
		{
			const float tb = ta + GTAO_PI / 32.0;
			const float arcB = SectorArc(tb, n, sinN);
			if ((aoSectors & (1u << k)) == 0)
			{
				const float w = tb <= 0.0 ? arcA - arcB : (ta >= 0.0 ? arcB - arcA : (arcA - arcAtV) + (arcB - arcAtV));
				sliceVisible += w;
				visibleAngleSum += w * 0.5 * (ta + tb);
			}
			ta = tb;
			arcA = arcB;
		}
		const float localVisibility = projNormalLength * sliceVisible;
		visibility += localVisibility;
		// Bent normal: the normal, rotated towards the weighted middle of the visible sectors
		// (see below)
		const float delta = sliceVisible > 1e-6 ? -(visibleAngleSum / sliceVisible - n) : 0.0;
#else
		// Angle bias: sin(elevation - bias) = sin * cosBias - cos * sinBias, not below 0
		elevation0 = max(elevation0 * cosBias - sqrt(saturate(1.0 - elevation0 * elevation0)) * sinBias, 0.0);
		elevation1 = max(elevation1 * cosBias - sqrt(saturate(1.0 - elevation1 * elevation1)) * sinBias, 0.0);
		// The horizon angles (from V): the tangent plane on each side (n +- PI/2), raised by
		// the elevation towards the normal. They stay within the hemisphere around the normal.
		const float h1 = n + GTAO_PI_HALF - asin(elevation0);
		const float h0 = n - GTAO_PI_HALF + asin(elevation1);

		// Cosine-weighted visible arc between the horizons (GTAO paper, equation 10)
		const float sinN = sin(n);
		const float arc0 = (cosNorm + 2.0 * h0 * sinN - cos(2.0 * h0 - n)) / 4.0;
		const float arc1 = (cosNorm + 2.0 * h1 * sinN - cos(2.0 * h1 - n)) / 4.0;
		const float localVisibility = projNormalLength * (arc0 + arc1);
		visibility += localVisibility;

		// Bent normal: the normal, rotated in the slice plane by how far the middle of the
		// visible arc is from it, weighted by how much of the slice is visible. Without
		// occluders the arc is centered on the normal, so the bent normal is the normal.
		// Rotating by a positive angle around axisVec moves towards -orthoDirVec, so the
		// angle is negated (Rodrigues' rotation formula).
		const float delta = -(0.5 * (h0 + h1) - n);
#endif
		const float cosDelta = cos(delta), sinDelta = sin(delta);
		const float3 rotatedNormal = N * cosDelta + cross(axisVec, N) * sinDelta +
			axisVec * dot(axisVec, N) * (1.0 - cosDelta);
		bentNormal += rotatedNormal * localVisibility;
	}

	// Not clamped to 1, see the top of this file
	visibility = max(visibility / sliceCount, 0.0);
	visibility = pow(visibility, gtao_intensity);
	// A pixel that's visible can't be fully occluded
	visibility = max(visibility, 0.03);
	bentNormal = length(bentNormal) > 1e-4 ? normalize(bentNormal) : N;
#ifdef GTAO_GI
	GTAO_RETURN(float4(bentNormal, visibility), float4(indirect / sliceCount, 1.0))
#else
	return float4(bentNormal, visibility);
#endif
}
