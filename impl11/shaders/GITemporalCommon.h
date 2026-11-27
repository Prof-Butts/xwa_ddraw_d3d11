/*
 * The indirect light's temporal filter (c:\temp\gi-temporal-plan.md). Licensed under the MIT
 * license. See LICENSE.txt
 *
 * Each GI pixel takes its position (the screen pixel at its top-left corner, like the gather),
 * moves it to where it was last frame with each candidate motion (GIMotion.h in ddraw),
 * projects it with last frame's camera, and checks last frame's depth there. Where it matches,
 * this frame's gathered light is blended with last frame's result. The output is this frame's
 * history, which the GI filters read instead of the gathered light:
 *   rgb: the light (or the debug colors of pbr_gi_debug 7 and 8),
 *   a:   1 + its depth where it has light. Elsewhere, the gather's own alpha: 0 for the pixels
 *        it skipped, GI_EMPTY (0.75) for the ones with no light nearby. The filters skip
 *        a < 0.9, so they still see the gather's meanings, and next frame's pass only uses the
 *        texels with a >= 1 as history.
 */

#ifdef INSTANCED_RENDERING
Texture2DArray texPos      : register(t0); // The positions (full resolution)
Texture2DArray texHistPrev : register(t1); // Last frame's history (rgb: light, a: 1 + its depth, < 1: none)
Texture2DArray texCurrent  : register(t2); // This frame's gathered light
#define POS_LOAD(p)  texPos.Load(int4(p, viewId, 0))
#define HIST_LOAD(p) texHistPrev.Load(int4(p, viewId, 0))
#define CUR_LOAD(p)  texCurrent.Load(int4(p, viewId, 0))
#else
Texture2D texPos      : register(t0);
Texture2D texHistPrev : register(t1);
Texture2D texCurrent  : register(t2);
#define POS_LOAD(p)  texPos.Load(int3(p, 0))
#define HIST_LOAD(p) texHistPrev.Load(int3(p, 0))
#define CUR_LOAD(p)  texCurrent.Load(int3(p, 0))
#endif

// GITemporalCBuffer in EffectsCommon.h
cbuffer ConstantBuffer : register(b13)
{
	// The candidate motions, last frame's metric camera space from this frame's:
	// P_prev = (dot(row[0], P1), dot(row[1], P1), dot(row[2], P1)), P1 = float4(P, 1)
	float4 attachedRow[3];
	float4 worldRow[3];
	// 96 bytes
	float4 attachedSphere;    // xyz: the attached object's center (this frame's metric camera space), w: radius (m); w <= 0: no attached candidate
	// 112 bytes
	float  worldValid;        // 1: the world candidate is valid
	float  historyValid;      // 0: no usable history (first frame, a pause, a reset)
	float  giScale;           // A GI pixel stands for giScale x giScale screen pixels
	float  debugMode;         // pbr_gi_debug: 7 the candidates' matches, 8 the projection self-check
	// 128 bytes
	float2 fullSize;          // The screen buffers' size, in pixels
	float2 histSize;          // The part of the history buffers in use, in GI pixels
	// 144 bytes
	float  depthTolerance;    // A history texel matches if its depth is within this fraction
	float  vrMode;            // 1: SteamVR (projEyeMatrix * view matrix); 0: flat (TransformProjection())
	float  temporalWeight;    // How much of the history is kept (pbr_gi_temporal_weight)
	float  accumulate;        // 1: blend with the history (pbr_gi_temporal); 0: pass the light through
	// 160 bytes
	// Flat mode: XWA's projection (TransformProjection() in XwaD3dCommon.hlsl), last frame's and this frame's
	float4 prevProj;          // projectionValue1, projectionDeltaX, projectionDeltaY, unused
	float4 prevViewportScale;
	float4 curProj;
	float4 curViewportScale;
	// 224 bytes
	// The scene's viewport, in pixels (TopLeftX, TopLeftY, Width, Height): the projection's
	// normalized coordinates map to it, not to the whole buffer
	float4 prevViewport;
	float4 curViewport;
	// 256 bytes
	// VR: projEyeMatrix[eye] * (fullViewMatrix or viewMatrix), last frame's and this frame's
	matrix prevProjView[2];
	matrix curProjView[2];
	// 512 bytes
};

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

// The normalized device coordinates of a point of the G-buffer's metric camera space (Y+ up),
// as XWA's vertex shader projects it in flat mode: in OPT scale, Y down
float2 ProjectFlat(float3 P, float4 proj, float4 vpScale)
{
	const float3 Popt = float3(P.x, -P.y, P.z) * 40.96;
	const float st0 = proj.x / Popt.z;
	const float2 xy = Popt.xy * st0 + proj.yz;
	return (xy * vpScale.xy + float2(-1.0, 1.0)) * vpScale.z;
}

// The same in VR: the vertex shader flips Z, then applies the view and the eye's projection
float2 ProjectVR(float3 P, matrix projView)
{
	const float4 clip = mul(projView, float4(P.x, P.y, -P.z, 1.0));
	return clip.xy / clip.w;
}

// Normalized device coordinates -> the buffer's 0..1 coordinates, through the scene's viewport
float2 NDCToUV(float2 ndc, float4 viewport)
{
	const float2 px = viewport.xy + float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5) * viewport.zw;
	return px / fullSize;
}

// In VR, the eye's projection maps to the whole eye buffer, not to the scene viewport (which
// D3dRenderer::GetViewport() letterboxes to XWA's 4:3 in VR too): verified with the self-check
// (pbr_gi_debug = 8), the viewport only matched along the middle row.
float4 SceneViewport(float4 viewport)
{
	return vrMode > 0.5 ? float4(0.0, 0.0, fullSize) : viewport;
}

float2 ProjectPrev(float3 P, uint viewId)
{
	const float2 ndc = vrMode > 0.5 ? ProjectVR(P, prevProjView[viewId]) : ProjectFlat(P, prevProj, prevViewportScale);
	return NDCToUV(ndc, SceneViewport(prevViewport));
}

float2 ProjectCur(float3 P, uint viewId)
{
	const float2 ndc = vrMode > 0.5 ? ProjectVR(P, curProjView[viewId]) : ProjectFlat(P, curProj, curViewportScale);
	return NDCToUV(ndc, SceneViewport(curViewport));
}

// How much of last frame's history at the previous position Pprev matches its depth: the
// weight of the matching texels in its 2x2 bilinear footprint (0..1), or -1 when it was off
// the screen. history: the matching texels' light, averaged with their weights.
float MatchHistory(float3 Pprev, uint viewId, out float3 history)
{
	history = 0.0;
	if (Pprev.z <= 0.0)
		return -1.0;
	const float2 uv = ProjectPrev(Pprev, viewId);
	if (any(uv < 0.0) || any(uv > 1.0))
		return -1.0;
	// GI pixel h stands for the screen pixel h * giScale (its top-left corner)
	const float2 hp = (uv * fullSize - 0.5) / giScale;
	const int2 base = int2(floor(hp));
	const float2 f = hp - base;
	float weight = 0.0;
	[unroll]
	for (int k = 0; k < 4; k++)
	{
		const int2 t = base + int2(k & 1, k >> 1);
		if (any(t < 0) || any(t >= int2(histSize)))
			continue;
		const float4 hist = HIST_LOAD(t);
		const float w = ((k & 1) ? f.x : 1.0 - f.x) * ((k >> 1) ? f.y : 1.0 - f.y);
		if (hist.a >= 1.0 && abs((hist.a - 1.0) - Pprev.z) <= depthTolerance * Pprev.z)
		{
			weight += w;
			history += w * hist.rgb;
		}
	}
	if (weight > 0.0)
		history /= weight;
	return weight;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
#else
	const uint viewId = 0;
#endif
	const int2 h = int2(input.pos.xy);
	const int2 fp = min(h * (int)giScale, int2(fullSize) - 1);
	const float4 current = CUR_LOAD(h);
	// The pixels the gather skipped (the background) or that have no light nearby (GI_EMPTY)
	// stay as they are, and aren't history next frame (their light is 0). They're most of the
	// emitters' pixels, so they return before anything else is read.
	if (debugMode < 6.5 && current.a < 0.9)
		return current;
	const float4 Pw = POS_LOAD(fp);
	const float3 P = Pw.xyz;
	// Without accumulation, the light passes through (with its depth, for the next frame)
	if (debugMode < 6.5 && accumulate < 0.5)
		return float4(current.rgb, 1.0 + P.z);
	// The background: the positions are cleared to (0, 0, 32000, 0) (Direct3DDevice.cpp), and
	// surfaces write their coverage in w. No history there.
	if (Pw.w <= 0.0 || !(P.z > 0.0 && P.z < 31999.0))
		return current;

	float3 color;
	if (debugMode > 7.5)
	{
		// The projection self-check: this frame's projection of the pixel's own position should
		// be the pixel's own center. Green: within half a pixel, yellow: within 2, red: more.
		const float2 uv = ProjectCur(P, viewId);
		const float errPx = length(uv * fullSize - (fp + 0.5));
		color = errPx < 0.5 ? float3(0, 1, 0) : (errPx < 2.0 ? float3(1, 1, 0) : float3(1, 0, 0));
		return float4(color, 1.0 + P.z);
	}

	// The candidates, the attached one first (inside its bounding sphere): the first whose
	// reprojected history matches gives the history
	int match = 0; // 0: none, 1: attached, 2: world
	bool offscreen = false;
	float matchWeight = 0.0;
	float3 history = 0.0;
	if (historyValid > 0.5)
	{
		const float4 P1 = float4(P, 1.0);
		float3 hist;
		if (attachedSphere.w > 0.0 && distance(P, attachedSphere.xyz) <= attachedSphere.w)
		{
			const float3 Pp = float3(dot(attachedRow[0], P1), dot(attachedRow[1], P1), dot(attachedRow[2], P1));
			const float m = MatchHistory(Pp, viewId, hist);
			if (m >= 0.5)
			{
				match = 1;
				matchWeight = m;
				history = hist;
			}
			else if (m < 0.0)
				offscreen = true;
		}
		if (match == 0 && worldValid > 0.5)
		{
			const float3 Pp = float3(dot(worldRow[0], P1), dot(worldRow[1], P1), dot(worldRow[2], P1));
			const float m = MatchHistory(Pp, viewId, hist);
			if (m >= 0.5)
			{
				match = 2;
				matchWeight = m;
				history = hist;
			}
			else if (m < 0.0)
				offscreen = true;
		}
	}

	if (debugMode > 6.5)
	{
		// The candidates' matches. Green: the attached motion, cyan: the world motion, red: none,
		// magenta: none, and a candidate put it off the screen last frame, gray: no history.
		if (historyValid < 0.5)
			color = float3(0.3, 0.3, 0.3);
		else if (match == 1)
			color = float3(0, 1, 0);
		else if (match == 2)
			color = float3(0, 1, 1);
		else
			color = offscreen ? float3(1, 0, 1) : float3(1, 0, 0);
		return float4(color, 1.0 + P.z);
	}

	// The accumulation: this frame's light, blended with the history of the same surface. Where
	// only part of the history's footprint matched, less of it is kept.
	if (current.a < 0.9)
		return current;
	if (accumulate < 0.5 || match == 0 || any(isnan(history)))
		return float4(current.rgb, 1.0 + P.z);
	return float4(lerp(current.rgb, history, temporalWeight * saturate(matchWeight)), 1.0 + P.z);
}
