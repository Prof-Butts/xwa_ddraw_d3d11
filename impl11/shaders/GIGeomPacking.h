#ifndef GI_GEOM_PACKING_H
#define GI_GEOM_PACKING_H

// The packing of the positions and normals with mips (GIGeomMipsCommon.h): xyz = the
// position as 16-bit floats, w = the normal, octahedral, 8 bits per component. Packing needs
// shader model 5.0 (f32tof16). Unpacking works with shader model 4 too: define
// GEOM_PACKING_SM4 first (the GTAO shaders, which also run with feature level 10).

// Level 0 of the mips is at 1 / GI_GEOM_SCALE of the resolution (GI_GEOM_SCALE in ddraw)
static const uint GI_GEOM_SCALE = 2;

// f16tof32 for shader model 4: exact for zero and the normal numbers; the denormals (below
// 6.1e-5) become 0, and the positions don't have infinities or NaNs
float HalfToFloat(uint h)
{
	const uint e = h & 0x7C00;
	const uint s = (h & 0x8000) << 16;
	return asfloat(e == 0 ? s : (s | ((e + 0x1C000) << 13) | ((h & 0x03FF) << 13)));
}

#ifdef GEOM_PACKING_SM4
#define GEOM_F16_TO_F32(h) HalfToFloat(h)
#else
#define GEOM_F16_TO_F32(h) f16tof32(h)
#endif

// The normal (unit length) to two values in -1..1
float2 OctEncode(float3 n)
{
	n /= abs(n.x) + abs(n.y) + abs(n.z);
	const float2 signs = float2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
	return n.z >= 0.0 ? n.xy : (1.0 - abs(n.yx)) * signs;
}

float3 OctDecode(float2 e)
{
	float3 n = float3(e, 1.0 - abs(e.x) - abs(e.y));
	const float t = saturate(-n.z);
	n.xy += float2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
	return normalize(n);
}

uint4 PackGeom(float3 P, float3 N)
{
	const uint2 e = (uint2)round(saturate(OctEncode(N) * 0.5 + 0.5) * 255.0);
	return uint4(f32tof16(P.x), f32tof16(P.y), f32tof16(P.z), e.x | (e.y << 8));
}

float3 UnpackGeomPos(uint4 g)
{
	return float3(GEOM_F16_TO_F32(g.x), GEOM_F16_TO_F32(g.y), GEOM_F16_TO_F32(g.z));
}

float3 UnpackGeomNormal(uint4 g)
{
	const float2 e = float2(g.w & 0xFF, (g.w >> 8) & 0xFF) / 255.0 * 2.0 - 1.0;
	return OctDecode(e);
}

#endif
