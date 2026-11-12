// ShadowMapVertexShaderMatrixCB (the same struct is used for both the vertex and pixel shader)
cbuffer ConstantBuffer : register(b5)
{
	matrix Camera;
	matrix lightWorldMatrix[MAX_XWA_LIGHTS];
	
	uint sm_enabled, sm_debug;
	float sm_light_size, sm_blocker_radius;

	float sm_aspect_ratio, sm_bias, sm_pcss_radius;
	bool sm_csm_enabled;

	float3 POV;
	float sm_resolution;

	int light_index;
	float sm_FOVscale, sm_y_center, sm_z_factor;

	uint sm_PCSS_enabled, sm_pcss_samples, sm_hardware_pcf, sm_VR_mode;

	float4 sm_black_levels[MAX_XWA_LIGHTS / 4];
	float4 OBJrange[MAX_XWA_LIGHTS / 4];
	float4 OBJminZ[MAX_XWA_LIGHTS / 4];
	float4 sm_minZ[MAX_XWA_LIGHTS / 4];
	float4 sm_maxZ[MAX_XWA_LIGHTS / 4];
};

inline float get_OBJrange(uint idx) {
	return OBJrange[idx >> 2][idx & 0x03];
}

inline float get_OBJminZ(uint idx) {
	return OBJminZ[idx >> 2][idx & 0x03];
}

inline float MetricZToDepth(uint idx, float Z) {
	const float range = get_OBJrange(idx);
	const float minZ = get_OBJminZ(idx);
	// Objects that are too far away correspond to a depth of 1.0
	if (Z > range + minZ) return 1.0;
	return 0.98 * (Z - minZ) / range;
}

inline float DepthToMetricZ(uint idx, float z) {
	const float range = get_OBJrange(idx);
	const float minZ = get_OBJminZ(idx);
	// This is the inverse of the above: map 0..0.98 to the range OBJminZ to OBJminZ + OBJrange
	// Z values above 0.98 are "unpopulated" during the creation of the shadow map, so they are "at infinity"
	if (z > 0.98) return INFINITY_Z0;
	return z / 0.98 * range + minZ;
}

inline float get_black_level(uint idx)
{
	return sm_black_levels[idx >> 2][idx & 0x03];
}

inline void get_black_level_and_minmaxZ(uint idx, out float black_level, out float minZ, out float maxZ)
{
	const uint i = idx >> 2;
	const uint j = idx & 0x03;
	black_level = sm_black_levels[i][j];
	minZ = sm_minZ[i][j];
	maxZ = sm_maxZ[i][j];
}

// The lowest black level of the lights that cast shadows (black level 0.95 or less), or 1
// when none of them does
inline float get_shadow_caster_black_level(uint lightCount)
{
	float result = 1.0;
	for (uint i = 0; i < lightCount; i++)
	{
		const float black_level = get_black_level(i);
		if (black_level <= 0.95)
			result = min(result, black_level);
	}
	return result;
}

// The shadow factor for the specular reflections of the lights. The ray-traced and the
// shadow-mapped shadows never go below the black level (shadow_mapping_black_level), so
// the shadows don't turn completely black: that's a stand-in for the light that fills them.
// But it's still the sun's light: in a full shadow, the sun's highlights would keep that
// fraction of their strength, and glossy highlights are many times brighter than 1, so
// they'd still show inside the shadow. This removes the black level: 0 in a full shadow,
// 1 in the light, and the penumbras in between.
inline float specular_shadow_factor(float shadow_factor, float caster_black_level)
{
	if (caster_black_level > 0.95)
		return shadow_factor;
	return saturate((shadow_factor - caster_black_level) / (1.0 - caster_black_level));
}
