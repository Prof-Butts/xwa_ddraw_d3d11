/*
 * The positions and normals of the indirect light's passes, with mips (GIGeomMips.hlsl).
 * Licensed under the MIT license. See LICENSE.txt
 *
 * Level 0 is at 1 / gtao_scale of the resolution (the emitters' scale, or GI_SCALE without
 * them), and each level halves it. Each texel is the full-resolution pixel at its top-left
 * corner: texel h of level k is pixel h * gtao_scale * 2^k, the pixel that the passes at that
 * resolution already use for their depth. So:
 * - The GI denoise and blur filters read their depths and normals here, at the level of
 *   their resolution, with the same values as from the full-resolution buffers, but packed
 *   together and contiguous: the full-resolution buffers, read at a stride of 2 or 4 pixels,
 *   are mostly cache misses.
 * - The emitters' gather (pbr_gi_emitter_mips, GTAOCommon.h) reads its distant samples here,
 *   like XeGTAO reads its depth from mips: the samples spread across the screen stay in the
 *   texture cache.
 *
 * Each texel packs the position (view space, 16-bit floats in x, y, z: the position buffer's
 * own values) and the normal (the normal buffer's convention, octahedral, 8 bits per
 * component in w) into 16-bit uints (GIGeomPacking.h), so one read of 8 bytes gives both.
 *
 * Level 0 is built from the full-resolution buffers (gtao_scale > 1), the other levels from
 * the level above (gtao_scale = 1 marks them, t2). gtao_buffer_size is the size of the input.
 */
#include "GTAOCBuffer.h"
#include "GIGeomPacking.h"

#ifdef INSTANCED_RENDERING
Texture2DArray texPos          : register(t0);
Texture2DArray texNormal       : register(t1);
Texture2DArray<uint4> texAbove : register(t2);
#else
Texture2D texPos          : register(t0);
Texture2D texNormal       : register(t1);
Texture2D<uint4> texAbove : register(t2);
#endif

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

uint4 main(PixelShaderInput input) : SV_TARGET
{
#ifdef INSTANCED_RENDERING
	const uint viewId = input.viewId;
#else
	const uint viewId = 0;
#endif
	const bool fromFullRes = gtao_scale > 1.5;
	const int step = fromFullRes ? (int)gtao_scale : 2;
	const int2 p = min(int2(input.pos.xy) * step, int2(gtao_buffer_size) - 1);
	uint4 result;
	[branch]
	if (fromFullRes)
	{
#ifdef INSTANCED_RENDERING
		const float3 P = texPos.Load(int4(p, viewId, 0)).xyz;
		const float3 N = texNormal.Load(int4(p, viewId, 0)).xyz;
#else
		const float3 P = texPos.Load(int3(p, 0)).xyz;
		const float3 N = texNormal.Load(int3(p, 0)).xyz;
#endif
		result = PackGeom(P, length(N) > 1e-4 ? normalize(N) : float3(0, 0, 1));
	}
	else
	{
#ifdef INSTANCED_RENDERING
		result = texAbove.Load(int4(p, viewId, 0));
#else
		result = texAbove.Load(int3(p, 0));
#endif
	}
	return result;
}
