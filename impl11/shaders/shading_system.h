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
	// Read by the Deferred and PBR shading. SetLights() fills them with the settings of the
	// active mode: exposure, ambient_color and min_albedo, or their pbr_* versions.
	float exposure_mult;      // Overall brightness
	float3 ambient_tint;      // Multiplies the ambient light
	// 1136 bytes
	float albedo_floor;       // The albedo of black areas (linear): dark colors are lifted to this
	// Color grading, only used when ssao_type = PBR (pbr_grade_*). Neutral: all 1.
	float grade_contrast;     // Contrast around mid-gray, on the final colors
	float grade_saturation;   // 0 = gray, 1 = unchanged
	float gtao_enabled;       // 1 when the GTAO buffer (bent normal + AO) is bound to t1
	// 1152 bytes
	float3 grade_shadows;     // Tint for the dark tones; it changes the color, not the brightness
	float gtao_scale;         // 1: GTAO at full resolution, 2: at half resolution
	// 1168 bytes
	float3 grade_highlights;  // Tint for the bright tones
	float gtao_coarse_enabled; // 1 when the coarse GTAO buffer is bound to t8
	// 1184 bytes
	float gtao_debug;         // pbr_gtao_debug: 0 off, 1 fine AO, 2 coarse AO, 3 both, 4 bent normal
	float gtao_multibounce;   // Multi-bounce AO strength (pbr_gtao_multibounce), 0 when it's off
	float gtao_light_occlusion; // GTAO occlusion of the laser/dynamic lights (pbr_gtao_light_occlusion)
	float unused_ss3;
	// 1200 bytes
	float env_ambient_mult;   // pbr_cubemap_ambient: multiplies the cubemap's ambient light (AmbientInt)
	float env_specular_mult;  // pbr_cubemap_specular: multiplies the cubemap's reflections (Specular)
	float env_debug;          // pbr_cubemap_debug: 0 off, 1 reflected sky, 2 reflections, 3 ambient
	float unused_ss4;
	// 1216 bytes
};
