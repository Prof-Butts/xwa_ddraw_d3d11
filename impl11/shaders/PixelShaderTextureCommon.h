#ifndef PIXEL_SHADER_COMMON_H
#define PIXEL_SHADER_COMMON_H

// PixelShaderCBuffer, used to be register(b0), but the new D3dRendererHook
// is using that, so let's use another register: b9
cbuffer ConstantBuffer : register(b9)
{
	float brightness;			// Used to dim some elements to prevent the Bloom effect -- mostly for ReShade compatibility
	uint DynCockpitSlots;		// (DC) How many DC slots will be used.
	uint bUseCoverTexture;		// (DC) When set, use the first texture as cover texture for the dynamic cockpit
	uint bInHyperspace;			// 1 if we're rendering while in hyperspace
	// 16 bytes

	uint bIsLaser;				// 1 for Laser objects, setting this to 2 will make them brighter (intended for 32-bit mode)
	float fOverlayBloomPower;	// Controls the intensity of the bloom effect coming from SCREEN overlay textures
	uint bIsEngineGlow;			// 1 if this is an engine glow textures, 2 will make it brighter (intended for 32-bit mode)
	uint GreebleControl;		// Bitmask: 0x1 0000 -- Use Normal Map
								// 0x00F 1st Tex Blend Mode
								// 0x0F0 2nd Tex Blend Mode
								// 0xF00 3rd Tex Blend Mode
								// 0x1: Multiply, 0x2: Overlay, 0x3: Screen, 0x4: Replace, 0x5: Normal Map, 0x6: UVDisp, 0x7: UVDisp+NM
	// 32 bytes

	float fBloomStrength;		// General multiplier for the bloom effect
	float fPosNormalAlpha;		// Override for pos3D and normal output alpha
	float fSSAOMaskVal;			// SSAO mask value
	float fSSAOAlphaOfs;		// Additional offset substracted from alpha when rendering SSAO. Helps prevent halos around transparent objects.
	// 48 bytes

	uint bIsShadeless;
	float fGlossiness, fSpecInt, fNMIntensity;
	// 64 bytes

	float fSpecVal, fDisableDiffuse;
	uint special_control;
	float fAmbient;				// Only used in PixelShaderNoGlass and implemented as a "soft-shadeless" setting
	// 80 bytes

	float4 AuxColor;			// Used as tint for animated frames. Could also be used to apply tint to DC cover textures
	// 96 bytes
	float2 Offset;				// Used to offset the input UVs
	float AspectRatio;			// Used to change the aspect ratio of the input UVs
	uint Clamp;					// Used to clamp (saturate) the input UVs
	// 112 bytes

	float GreebleDist1, GreebleDist2;
	float GreebleScale1, GreebleScale2;
	// 128 bytes

	float GreebleMix1, GreebleMix2;
	float2 UVDispMapResolution;
	// 144 bytes
	float4 AuxColorLight;
	// 160 bytes
	uint special_control_light;
	uint bDoNormalMapping;
	uint bDoRaytracing;
	uint OverlayCtrl;			// Used to control overlay effects, like shields down or hull damage
	// 176 bytes

	float rand0;				// Used as a random values in PixelShaderAnim.hlsl, also used for Tech Room holos
	float rand1;				// Used for Tech Room holos
	float rand2;
	float PS_unused0;
	// 192 bytes

	float2 uvSrc0; // Used in animated textures to constrain the area where damage textures will be applied
	float2 uvSrc1;
	// 208 bytes

	float2 uvOffset; // Used in animated textures to offset the damage textures
	float2 uvScale;  // Used in animated textures to scale the damage textures
	// 224 bytes
};

#endif