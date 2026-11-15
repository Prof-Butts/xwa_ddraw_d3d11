#ifndef GI_GEOM_PACKING_H
#define GI_GEOM_PACKING_H

// The packing of the emitters' gather's positions and normals (GIGeomMipsCommon.h): xyz =
// the position as 16-bit floats, w = the normal, octahedral, 8 bits per component. Shader
// model 5.0 (f32tof16).

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
	return float3(f16tof32(g.x), f16tof32(g.y), f16tof32(g.z));
}

float3 UnpackGeomNormal(uint4 g)
{
	const float2 e = float2(g.w & 0xFF, (g.w >> 8) & 0xFF) / 255.0 * 2.0 - 1.0;
	return OctDecode(e);
}

#endif
