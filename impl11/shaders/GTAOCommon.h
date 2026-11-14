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
 * The AO and the bent normal don't change. The indirect light goes to a second render target
 * (rgb, a = 1, or 0 for the pixels GTAO skips).
 *
 * The indirect light needs a much larger radius than the AO (light bounces off surfaces
 * tens of centimeters away, while the AO is about contact), so it runs in its own pass, at a
 * quarter of the resolution, with its own radius (pbr_gi_radius, pbr_gi_max_radius): see
 * GTAOPass() in ddraw. Its AO output isn't used. The bounce light is at 1 / gtao_bounce_scale
 * of the resolution (also a quarter).
 */
#include "GTAOCBuffer.h"

#ifdef INSTANCED_RENDERING
Texture2DArray texPos    : register(t0);
Texture2DArray texNormal : register(t1);
#else
Texture2D texPos    : register(t0);
Texture2D texNormal : register(t1);
#endif

#ifdef GTAO_GI
// The bounce light (GIBounceLight.hlsl), at this pass's resolution
#ifdef INSTANCED_RENDERING
Texture2DArray texBounce : register(t2);
#else
Texture2D texBounce : register(t2);
#endif

struct GTAOOutput
{
	float4 gtao : SV_TARGET0; // Bent normal, AO
	float4 gi   : SV_TARGET1; // Indirect light (rgb), a = 1 (0: skipped)
};
#define GTAO_RETURN(gtaoValue, giValue) { GTAOOutput o; o.gtao = (gtaoValue); o.gi = (giValue); return o; }
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

#ifdef GTAO_GI
// The indirect light that sample S (at full-resolution pixel samplePx) adds to the pixel at
// P, in a slice: sliceDir is the slice's direction perpendicular to V, n the angle of the
// projected normal from V (positive towards sliceDir). sectors is the slice's visibility
// bitmask: the sample covers the sectors between its front and its back, and only the ones
// that weren't covered by a closer sample add its light. weight is GTAO's falloff (0: not
// geometry).
float3 GatherIndirect(float3 P, float3 S, int2 samplePx, uint viewId, float weight,
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
	const float thetaFront = atan2(dot(delta, sliceDir), dot(delta, V)) - n;
	const float thetaBack  = atan2(dot(deltaBack, sliceDir), dot(deltaBack, V)) - n;
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
	const float4 sampleNormal = LoadNormal(samplePx, viewId);
	const float3 sampleN = normalize(float3(sampleNormal.x, sampleNormal.y, -sampleNormal.z));
	const float facing = smoothstep(-0.1, 0.1, dot(sampleN, -normalize(delta)));

	// Its bounce light, from the bounce light's pixel that covers samplePx
	const int2 bouncePx = int2(uint2(clamp(samplePx, int2(0, 0), int2(gtao_buffer_size) - 1)) / (uint)gtao_bounce_scale);
#ifdef INSTANCED_RENDERING
	const float3 bounce = texBounce.Load(int4(bouncePx, viewId, 0)).rgb;
#else
	const float3 bounce = texBounce.Load(int3(bouncePx, 0)).rgb;
#endif
	// Each sector is PI/32 wide; with the cosine at the middle of the sample, a full half
	// circle adds up to 2, so the weight is PI/64 per sector
	const float thetaMid = 0.5 * (a + b) * GTAO_PI - GTAO_PI_HALF;
	return bounce * (countbits(newBits) * (GTAO_PI / 64.0) * cos(thetaMid) * facing * weight);
}
#endif

// 4x4 ordered (Bayer) pattern: each value 0..15 appears once in every 4x4 block
float Bayer4x4(int2 px)
{
	const uint2 p = uint2(px) & 3;
	const uint bayer[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
	return (float)bayer[p.y * 4 + p.x];
}

#ifdef GTAO_GI
GTAOOutput main(PixelShaderInput input)
#else
float4 main(PixelShaderInput input) : SV_TARGET
#endif
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
	// The pattern is in this pass's pixels, so the 4x4 denoise filter averages it out
	const float noise = Bayer4x4(outPx);
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
		const float n = signNorm * acos(cosNorm);

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

		[loop]
		for (int step = 0; step < stepCount; step++)
		{
			// More samples close to the pixel, where the occluders matter most
			float s = (step + noiseStep) / stepCount;
			s = s * s;
			const float2 offset = round(omega * max(s * screenRadius, 1.0));

			const float3 S0 = LoadPos(px + int2(offset), viewId);
			const float3 S1 = LoadPos(px - int2(offset), viewId);
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
			sliceIndirect += GatherIndirect(P, S0, px + int2(offset), viewId, weight0, V, sliceDir, n, sectors);
			sliceIndirect += GatherIndirect(P, S1, px - int2(offset), viewId, weight1, V, sliceDir, n, sectors);
#endif
		}
#ifdef GTAO_GI
		// Like the visibility, the slice counts as much as the normal lies in its plane
		indirect += sliceIndirect * projNormalLength;
#endif

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
