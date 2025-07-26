// SSAOPixelShaderCBuffer
cbuffer ConstantBuffer : register(b3)
{
	float screenSizeX, screenSizeY, indirect_intensity, bias;
	// 16 bytes
	float intensity, near_sample_radius, black_level;
	uint samples;
	// 32 bytes
	uint z_division;
	float bentNormalInit, max_dist, power;
	// 48 bytes
	uint ssao_debug;
	float moire_offset, ssao_amplifyFactor;
	uint fn_enable;
	// 64 bytes
	float fn_max_xymult, cubeMapSpecInt, fn_sharpness, cubeMapAmbientInt;
	// 80 bytes
	float far_sample_radius, cubeMapAmbientMin, ssao_unused3, ssao_amplifyFactor2;
	// 96 bytes
	float2 ssao_p0, ssao_p1; // Viewport limits in uv space
	// 112 bytes
	float enable_dist_fade, ssaops_unused1, ssaops_unused2, shadow_epsilon;
	// 128 bytes
	float ssaops_unused3, shadow_step_size, shadow_steps, aspect_ratio;
	// 144 bytes
	float4 vpScale;
	// 160 bytes
	uint cubeMappingEnabled;
	float shadow_k, Bz_mult, moire_scale;
	// 176 bytes
};

