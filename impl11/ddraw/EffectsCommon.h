#pragma once

#include "Matrices.h"
#include "../shaders/material_defs.h"
#include "../shaders/shader_common.h"
#include "common.h"


#ifndef EFFECTS_CONST
#define EFFECTS_CONST
const int MAX_TEXTURE_NAME = 128;
const float PI = 3.141593f;
const float DEG2RAD = PI / 180.0f;
const float RAD2DEG = 180.0f / PI;
#endif

// General types and globals

class Direct3DTexture;

struct uvfloat4 {
	float x0, y0, x1, y1;
};

struct float3 {
	float x, y, z;

	float3()
	{
		x = y = z = 0.0f;
	}

	float3(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	float3(const float data[4])
	{
		x = data[0];
		y = data[1];
		z = data[2];
	}

	float3(const Vector4 &P)
	{
		this->x = P[0];
		this->y = P[1];
		this->z = P[2];
	}

	float3(const Vector3& P)
	{
		this->x = P[0];
		this->y = P[1];
		this->z = P[2];
	}

	float3 xyz()
	{
		return float3(x, y, z);
	}

	inline float& operator[] (uint32_t index) { return (&x)[index]; }

	inline float3 operator+(const float3& rhs)
	{
		return float3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
	}

	inline float3 operator-(const float3& rhs)
	{
		return float3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
	}

	inline float3 operator*(const float3& rhs)
	{
		return float3(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
	}

	inline float3& operator*=(const float rhs)
	{
		this->x *= rhs;
		this->y *= rhs;
		this->z *= rhs;

		return *this;
	}
};

inline float3 operator*(const float s, const float3& rhs)
{
	return float3(s * rhs.x, s * rhs.y, s * rhs.z);
}

inline float3 operator/(const float s, const float3& rhs)
{
	return float3(s / rhs.x, s / rhs.y, s / rhs.z);
}

struct float4 {
	float x, y, z, w;

	inline float& operator[] (uint32_t index) { return (&x)[index]; }

	float4()
	{
		x = y = z = w = 0.0f;
	}

	float4(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = 1.0f;
	}

	float4(float x, float y, float z, float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	float4(float3 P, float w)
	{
		this->x = P.x;
		this->y = P.y;
		this->z = P.z;
		this->w = w;
	}

	float4(uvfloat4 P)
	{
		x = P.x0;
		y = P.y0;
		z = P.x1;
		w = P.y1;
	}
};

struct float2 {
	float x, y;

	float2()
	{
	}

	float2(float x, float y)
	{
		this->x = x;
		this->y = y;
	}

	float2(float* P)
	{
		this->x = P[0];
		this->y = P[1];
	}

	float2(Vector4 P)
	{
		this->x = P.x;
		this->y = P.y;
	}

	inline float& operator[] (uint32_t index) { return (&x)[index]; }
};

// Holds the current 3D reconstruction constants, register b6
typedef struct MetricReconstructionCBStruct {
	float mr_aspect_ratio;   // Same as sm_aspect_ratio (g_fCurInGameAspectRatio), remove sm_* later
	float mr_FOVscale;       // Same as sm_FOVscale NOT the same as g_ShadertoyBuffer.FOVscale
	float mr_y_center;       // Same as sm_y_center NOT the same as g_ShadertoyBuffer.y_center
	float mr_z_metric_mult;  // Probably NOT the same as sm_z_factor

	float mr_cur_metric_scale, mr_shadow_OBJ_scale, mr_screen_aspect_ratio, mr_debug_value;

	//float mr_vr_aspect_ratio_comp[2]; // This is used with shaders that work with the postproc vertexbuf, like the reticle shader
	float mr_vr_aspect_ratio, mr_unused0;
	float mv_vr_vertexbuf_aspect_ratio_comp[2]; // This is used to render the HUD
} MetricReconstructionCB;

typedef struct BloomConfigStruct {
	float fSaturationStrength, fCockpitStrength, fEngineGlowStrength, fSparksStrength;
	float fLightMapsStrength, fLasersStrength, fHyperStreakStrength, fHyperTunnelStrength;
	float fTurboLasersStrength, fLensFlareStrength, fExplosionsStrength, fSunsStrength;
	float fCockpitSparksStrength, fMissileStrength, fSkydomeLightStrength, fBracketStrength;
	float uvStepSize1, uvStepSize2;
	int iNumPasses;
} BloomConfig;

typedef struct BloomPixelShaderCBStruct {
	float pixelSizeX, pixelSizeY, general_bloom_strength, amplifyFactor;
	// 16 bytes
	float bloomStrength, uvStepSize, saturationStrength;
	int unused1;
	// 32 bytes
	int unused2;
	float unused3, depth_weight;
	int debug;
	// 48 bytes
} BloomPixelShaderCBuffer;

typedef struct SSAOPixelShaderCBStruct {
	float screenSizeX, screenSizeY, indirect_intensity, bias;
	// 16 bytes
	float intensity, near_sample_radius, black_level;
	int samples;
	// 32 bytes
	int z_division;
	float bentNormalInit, max_dist, power;
	// 48 bytes
	int debug;
	float moire_offset, amplifyFactor;
	int fn_enable;
	// 64 bytes
	float fn_max_xymult, ssao_unused0, fn_sharpness, ssao_unused1;
	// 80 bytes
	float far_sample_radius, ssao_unused2, ssao_unused3, amplifyFactor2;
	// 96 bytes
	float x0, y0, x1, y1; // Viewport limits in uv space
	// 112 bytes
	float enable_dist_fade, ssaops_unused1, ssaops_unused2, shadow_epsilon;
	// 128 bytes
	float ssaops_unused3, shadow_step_size, shadow_steps, aspect_ratio;
	// 144 bytes
	float vpScale[4];
	// 160 bytes
	int ssao_unused;
	float shadow_k, Bz_mult, moire_scale;
	// 176 bytes
} SSAOPixelShaderCBuffer;

typedef struct ShadertoyCBStruct {
	// twirl: renamed to ExplosionScale in ExplosionShader.hlsl
	float iTime, twirl, bloom_strength, srand;
	// 16 bytes
	float iResolution[2];
	// 0: Non-VR, 1: DirectSBS, 2: SteamVR. 
	// Set to 1 when the Hyperspace Effect is in the HS_POST_HYPER_EXIT_ST state
	// Used in the SunShader too.
	int VRmode;
	float y_center;
	// 32 bytes
	float x0, y0, x1, y1; // Limits in uv-coords of the viewport
	// 48 bytes
	Matrix4 viewMat; // The view rotation matrix
	// 4*4 = 16 elements, 16 * 4 = 64 bytes
	// 48 + 64 = 112 bytes
	// Style: Enables the flare when jumping into hyperspace and other details
	// 0: Standard (no flare)
	// 1: Flare (Disney style)
	// 2: Interdiction
	// Renamed to ExplosionBlendMode in ExplosionShader
	int Style;
	int hyperspace_phase; // 1 = HYPER_ENTRY, 2 = HYPER_TUNNEL, 3 = HYPER_EXIT, 4 = POST_HYPER_EXIT (same as HypespacePhaseEnum)
	// tunnel_speed: renamed to ExplosionTime in ExplosionShader.hlsl
	float tunnel_speed, FOVscale;
	// 128 bytes
	int SunFlareCount;
	float flare_intensity;
	float preserveAspectRatioComp[2]; // Used to compensate for the distortion introduced when PreserveAspectRatio = 0 in DDraw.cfg
	// 144 bytes
	//float SunX, SunY, SunZ, flare_intensity;
	float4 SunCoords[MAX_SUN_FLARES];
	// 208 bytes
	float4 SunColor[MAX_SUN_FLARES];
	// 272 bytes
} ShadertoyCBuffer;

typedef struct OPTMeshTransformCBufferStruct {
	Matrix4 MeshTransform;
	// 64 bytes
} OPTMeshTransformCBuffer;

// Let's make this Constant Buffer the same size as the ShadertoyCBuffer
// so that we can reuse the same CB slot -- after all, we can't manipulate
// anything while travelling through hyperspace anyway...
struct LaserPointerCBuffer {
	int TriggerState[2]; // 0 = Not pressed, 1 = Pressed
	float iResolution[2];
	// 16 bytes
	float x0, y0, x1, y1; // Limits in uv-coords of the viewport
	// 32 bytes
	int bContOrigin[2], bHoveringOnActiveElem[2];
	// 48 bytes
	float4 contOrigin[2][2];
	// 112 bytes
	float4 intersection[2][2];
	// 176 bytes
	float uv[2][2];
	// 192 bytes
	int bDebugMode, unused0;
	float cursor_radius[2];
	// 208 bytes
	float inters_radius[2];
	int bDisplayLine[2];
	// 224 bytes
	int bIntersection[2];
	float lp_aspect_ratio[2];
	// 240 bytes
	float2 v0L, v1L; // DEBUG
	// 256 bytes
	float2 v2L, v0R;
	// 272 bytes
	float2 v1R, v2R;
	// 288 bytes
};

/* 3D Constant Buffers */
typedef struct VertexShaderCBStruct {
	float viewportScale[4];
	// 16 bytes
	float s_V0x08B94CC;
	float s_V0x05B46B4;
	float s_V0x05B46B4_Offset;
	bool  bIsMapIcon;
	// 32 bytes
	float4 ProjectionParameters;
	// 48 bytes
	float aspect_ratio;
	uint32_t apply_uv_comp;
	float z_override, sz_override;
	// 64 bytes
	float mult_z_override, bPreventTransform, bFullTransform, scale_override;
	// 80 bytes
} VertexShaderCBuffer;

typedef struct VertexShaderMatrixCBStruct {
	Matrix4 projEye[2];
	Matrix4 viewMat;
	Matrix4 fullViewMat;
	// 256 bytes
	float Znear, Zfar, DeltaX, DeltaY;
	// 272 bytes
} VertexShaderMatrixCB;

typedef struct PSShadingSystemCBStruct {
	float3 MainLight;
	uint32_t LightCount;
	// 16 bytes
	float4 MainColor;
	// 32 bytes
	float4 LightVector[MAX_XWA_LIGHTS];
	// 32+128 = 160 bytes
	float4 LightColor[MAX_XWA_LIGHTS];
	// 160+128 = 288 bytes
	float spec_intensity, glossiness, spec_bloom_intensity, bloom_glossiness_mult;
	// 304 bytes
	float saturation_boost, lightness_boost, ssdo_enabled;
	uint32_t ss_debug;
	// 320 bytes
	float sso_disable, sqr_attenuation, laser_light_intensity;
	uint32_t num_lasers;
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
	uint32_t HDREnabled;
	// 1120 bytes
} PSShadingSystemCB;

typedef struct RTConstantsBufferStruct {
	uint32_t bRTEnable;
	uint32_t bRTAllowShadowMapping;
	uint32_t RTUnused0;
	uint32_t RTGetBestIntersection;
	// 16 bytes
	uint32_t RTEnableSoftShadows;
	uint32_t RTShadowMaskSizeFactor;
	float    RTShadowMaskPixelSizeX;
	float    RTShadowMaskPixelSizeY;
	// 32 bytes
	float    RTSoftShadowThreshold;
	float    RTGaussFactor;
	float    RTUnused1[2];
	// 48 bytes
} RTConstantsBuffer;

struct VRGeometryCBuffer {
	uint32_t numStickyRegions;
	uint32_t clicked[2];
	bool     bRenderBracket;
	// 16 bytes
	float4   stickyRegions[MAX_VRKEYB_REGIONS];
	// 80 bytes
	float4   clickRegions[2];
	// 112 bytes
	float    strokeWidth;
	float3   bracketColor;
	// 128 bytes
	float4   U;
	// 144 bytes
	Matrix4  viewMat;
	// 208 bytes
	float4   F;
	// 224 bytes
};

typedef struct {
	/* Exclusive Flags. Only one flag can be set at the same time */
	uint32_t ExclusiveMask : 8;
	/* Bitfields */
	uint32_t bBlastMark : 1;
	uint32_t UnusedBits : 23;
} special_control_bitfield;

// See PixelShaderTextureCommon.h for an explanation of these settings
struct PixelShaderCBuffer {
	float brightness;			// Used to control the brightness of some elements -- mostly for ReShade compatibility
	uint32_t DynCockpitSlots;
	uint32_t bUseCoverTexture;
	uint32_t bInHyperspace;
	// 16 bytes

	uint32_t bIsLaser;
	float fOverlayBloomPower;
	uint32_t bIsEngineGlow;
	uint32_t GreebleControl;
	// 32 bytes

	float fBloomStrength;
	float fPosNormalAlpha;
	float fSSAOMaskVal;
	float fSSAOAlphaMult;
	// 48 bytes

	uint32_t bIsShadeless;
	float fGlossiness, fSpecInt, fNMIntensity;
	// 64 bytes

	float fSpecVal, fDisableDiffuse;
	special_control_bitfield special_control;
	float fAmbient;
	// 80 bytes

	float4 AuxColor;
	// 96 bytes
	float2 Offset;
	float AspectRatio;
	uint32_t Clamp;
	// 112 bytes
	float GreebleDist1, GreebleDist2;
	float GreebleScale1, GreebleScale2;
	// 128 bytes

	float GreebleMix1, GreebleMix2;
	float2 UVDispMapResolution;
	// 144 bytes

	float4 AuxColorLight;
	// 160 bytes
	special_control_bitfield special_control_light;
	uint32_t bDoNormalMapping;
	uint32_t bDoRaytracing;
	uint32_t OverlayCtrl;
	// 176 bytes

	float rand0;
	float rand1;
	float rand2;
	bool  bIsTransparent;
	// 192 bytes

	float2 uvSrc0;
	float2 uvSrc1;
	// 208 bytes

	float2 uvOffset;
	float2 uvScale;
	// 224 bytes
};

// Pixel Shader constant buffer for the Dynamic Cockpit
typedef struct DCPixelShaderCBStruct {
	uvfloat4 src[MAX_DC_COORDS_PER_TEXTURE];
	// 4 * MAX_DC_COORDS * 4 = 192
	uvfloat4 dst[MAX_DC_COORDS_PER_TEXTURE];
	// 4 * MAX_DC_COORDS * 4 = 192
	// 384 bytes thus far
	uint32_t bgColor[MAX_DC_COORDS_PER_TEXTURE]; // 32-bit Background colors
	// 4 * MAX_DC_COORDS = 48
	// 432 bytes thus far

	float ct_brightness, dc_brightness;
	uint32_t noisy_holo; // If set to 1, the hologram shader will be noisy!
	uint32_t transparent; // If set to 1, the background will be transparent
	// 448 bytes
	uint32_t use_damage_texture; // If set to 1, force the use of the damage texture
	uint32_t dc_unused[3];
	// 464 bytes
} DCPixelShaderCBuffer;

/* 2D Constant Buffers */
typedef struct MainShadersCBStruct {
	float scale, aspect_ratio, parallax, brightness;
	float use_3D, inv_scale, unused1, unused2;
} MainShadersCBuffer;

typedef struct BarrelPixelShaderCBStruct {
	float k1, k2, k3;
	int unused;
} BarrelPixelShaderCBuffer;

#define MAX_HEAP_ELEMS 32
class VectorColor {
public:
	Vector3 P;
	Vector3 col;
	Vector3 dir;
	float falloff;
	float angle;
};

class SmallestK {
public:
	// Insert-sort-like algorithm to keep the k smallest elements from a constant flow
	VectorColor _elems[MAX_CB_POINT_LIGHTS];
	int _size;

public:
	SmallestK() {
		clear();
	}

	inline void clear() {
		_size = 0;
	}

	void insert(Vector3 P, Vector3 col, Vector3 dir={}, float falloff=0.0f, float angle=0.0f);
	void remove_duplicates();
};

typedef enum {
	GLOBAL_FOV,
	XWAHACKER_FOV,
	XWAHACKER_LARGE_FOV
} FOVtype;

extern FOVtype g_CurrentFOVType;
extern FOVtype g_CurrentFOV;
extern bool g_bCustomFOVApplied;

extern VertexShaderCBuffer g_VSCBuffer;
extern PixelShaderCBuffer g_PSCBuffer;
extern MetricReconstructionCB g_MetricRecCBuffer;
extern float g_fAspectRatio, g_fGlobalScale, g_fBrightness, g_fGUIElemsScale, g_fHUDDepth, g_fFloatingGUIDepth;
extern float g_fCurScreenWidth, g_fCurScreenHeight, g_fCurScreenWidthRcp, g_fCurScreenHeightRcp;
extern float g_fCurInGameWidth, g_fCurInGameHeight, g_fCurInGameAspectRatio;
extern float g_fMetricMult;
extern int g_WindowWidth, g_WindowHeight;
extern D3D11_VIEWPORT g_nonVRViewport;

void DisplayTimedMessage(uint32_t seconds, int row, char* msg);
void DisplayTimedMessageV(uint32_t seconds, int row, const char* format, ...);

bool SavePOVOffsetToIniFile();
bool LoadPOVOffsetFromIniFile();
bool LoadHUDColorFromIniFile();
void ApplyCustomHUDColor();