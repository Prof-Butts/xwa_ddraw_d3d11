// Constant buffer for the bloom shaders. Must match BloomPixelShaderCBuffer in
// ddraw/EffectsCommon.h. Other shaders share the first 48 bytes of this buffer.

// Must match MAX_BLOOM_LEVELS in ddraw/EffectsCommon.h
#define MAX_BLOOM_LEVELS 9

cbuffer ConstantBuffer : register(b2)
{
	float pixelSizeX, pixelSizeY, general_bloom_strength, amplifyFactor;
	// 16 bytes
	float bloomStrength, uvStepSize, saturationStrength, unused1;
	// 32 bytes
	float4 unused2; // Used by other shaders
	// 48 bytes
	// Blur passes: the input is sampled at clamp(input.uv * uvScale, uvMin, uvMax)
	float2 uvScale, uvMin;
	// 64 bytes
	float2 uvMax;
	int numLevels; // Combine pass: number of bloom levels
	float unused3;
	// 80 bytes
	// Combine pass, one entry per bloom level: xy = UV scale, zw = minimum UV
	float4 levelUV[MAX_BLOOM_LEVELS];
	// 224 bytes
	// Combine pass: strength of each bloom level, packed 4 per float4
	float4 levelStrength[3];
	// 272 bytes
};
