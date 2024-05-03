#include "material_defs.h"
#include "shader_common.h"

// PSShadingSystemCB
cbuffer ConstantBuffer : register(b4)
{
	float3 MainLight; // This is now the headlights' position in viewspace
	uint LightCount;
	// 16 bytes
	float4 MainColor; // The headlights color. The w component is the distance
	// 32 bytes
	float4 LightVector[MAX_XWA_LIGHTS];
	// 32+128 = 160 bytes
	float4 LightColor[MAX_XWA_LIGHTS];
	// 160+128 = 288 bytes
	float global_spec_intensity, global_glossiness, global_spec_bloom_intensity, global_bloom_glossiness_mult;
	// 304 bytes
	float saturation_boost, lightness_boost, ssdo_enabled;
	uint ss_debug;
	// 320 bytes
	float sso_disable, sqr_attenuation, laser_light_intensity;
	uint num_lasers;
	// 336 bytes
	float4 LightPoint[MAX_CB_POINT_LIGHTS];
	// 16 * 16 = 256
	// 592 bytes
	float4 LightPointColor[MAX_CB_POINT_LIGHTS];
	// 16 * 16 = 256
	// 848 bytes
	float4 LightPointDirection[MAX_CB_POINT_LIGHTS];
	// 16 * 16 = 256
	// 1104 bytes
	float ambient, headlights_angle_cos, HDR_white_point;
	uint HDREnabled;
	// 1120 bytes
};
