#include "RTCommon.h"
#include "../shading_system.h"
#include "../shadow_mapping_common.h"

#ifdef INSTANCED_RENDERING
// The (Smooth) Normals buffer
Texture2DArray texNormal : register(t0);
SamplerState samplerNormal : register(s0);

// The position/depth buffer
Texture2DArray texPos : register(t1);
SamplerState sampPos : register(s1);
#else
// The (Smooth) Normals buffer
Texture2D texNormal : register(t0);
SamplerState samplerNormal : register(s0);

// The position/depth buffer
Texture2D texPos : register(t1);
SamplerState sampPos : register(s1);
#endif

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
};

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;

	// DEBUG
	//output.color = 1.0;
	//return output;
	// DEBUG

#ifdef INSTANCED_RENDERING
    const float3 P      = texPos.Sample(sampPos, float3(input.uv, input.viewId)).xyz;
    const float4 Normal = texNormal.Sample(samplerNormal, float3(input.uv, input.viewId));
#else
	const float3 P      = texPos.Sample(sampPos, input.uv).xyz;
    const float4 Normal = texNormal.Sample(samplerNormal, input.uv);
#endif
	const float3 N      = Normal.xyz;

	float rt_shadow_factor = 1.0f;
	float occ_dist = RT_MAX_DIST;

	// If I remove the following, then the bloom mask is messed up!
	// The skybox gets alpha = 0, but when MSAA is on, alpha goes from 0 to 0.5
	if (Normal.w < 0.475) {
		output.color = float4(rt_shadow_factor, occ_dist, 0, 1);
		return output;
	}

	// Raytraced shadows
	for (uint i = 0; i < LightCount; i++)
	{
		float shadow_factor = 1.0;
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
			{
				rt_shadow_factor *= black_level;
				occ_dist = inters.T * 0.024414; // Convert to meters (1/40.96)
			}
		}
		else
		{
			rt_shadow_factor *= black_level;
			occ_dist = 0; // This surface faces away from the light. Distance to occluder: 0
		}
	}

	output.color = float4(rt_shadow_factor, occ_dist, 0, 1);
	return output;
}
