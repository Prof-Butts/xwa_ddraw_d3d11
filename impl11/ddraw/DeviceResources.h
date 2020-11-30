// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

#pragma once
#include "Matrices.h"
#include "../shaders/material_defs.h"
#include "../shaders/shader_common.h"
#include <vector>

// Also found in the Floating_GUI_RESNAME list:
extern const char *DC_TARGET_COMP_SRC_RESNAME;
extern const char *DC_LEFT_SENSOR_SRC_RESNAME;
extern const char *DC_LEFT_SENSOR_2_SRC_RESNAME;
extern const char *DC_RIGHT_SENSOR_SRC_RESNAME;
extern const char *DC_RIGHT_SENSOR_2_SRC_RESNAME;
extern const char *DC_SHIELDS_SRC_RESNAME;
extern const char *DC_SOLID_MSG_SRC_RESNAME;
extern const char *DC_BORDER_MSG_SRC_RESNAME;
extern const char *DC_LASER_BOX_SRC_RESNAME;
extern const char *DC_ION_BOX_SRC_RESNAME;
extern const char *DC_BEAM_BOX_SRC_RESNAME;
extern const char *DC_TOP_LEFT_SRC_RESNAME;
extern const char *DC_TOP_RIGHT_SRC_RESNAME;

// Use the following with `const auto missionIndexLoaded = (int *)0x9F5E74;` to detect the DSII tunnel run mission.
const int DEATH_STAR_MISSION_INDEX = 52;

typedef struct Box_struct {
	float x0, y0, x1, y1;
} Box;

typedef struct uvfloat4_struct {
	float x0, y0, x1, y1;
} uvfloat4;

typedef struct float3_struct {
	float x, y, z;
} float3;

typedef struct float4_struct {
	float x, y, z, w;
} float4;

// Region names. Used in the erase_region and move_region commands
const int LEFT_RADAR_HUD_BOX_IDX	= 0;
const int RIGHT_RADAR_HUD_BOX_IDX	= 1;
const int SHIELDS_HUD_BOX_IDX		= 2;
const int BEAM_HUD_BOX_IDX			= 3;
const int TARGET_HUD_BOX_IDX		= 4;
const int LEFT_MSG_HUD_BOX_IDX		= 5;
const int RIGHT_MSG_HUD_BOX_IDX		= 6;
const int TOP_LEFT_HUD_BOX_IDX		= 7;
const int TOP_RIGHT_HUD_BOX_IDX		= 8;
const int TEXT_RADIOSYS_HUD_BOX_IDX	= 9;
const int TEXT_CMD_HUD_BOX_IDX		= 10;
const int MAX_HUD_BOXES				= 11;
extern std::vector<const char *>g_HUDRegionNames;
// Convert a string into a *_HUD_BOX_IDX constant
int HUDRegionNameToIndex(char *name);

class DCHUDRegion {
public:
	Box coords;
	Box uv_erase_coords;
	uvfloat4 erase_coords;
	bool bLimitsComputed;
};

/*
 * This class stores the coordinates for each HUD Region : left radar, right radar, text
 * boxes, etc. It does not store the individual HUD elements within each HUD texture. For
 * that, look at DCElementSourceBox
 */
class DCHUDRegions {
public:
	std::vector<DCHUDRegion> boxes;

	DCHUDRegions();

	void Clear() {
		boxes.clear();
	}

	void ResetLimits() {
		for (unsigned int i = 0; i < boxes.size(); i++) 
			boxes[i].bLimitsComputed = false;
	}
};
const int MAX_DC_REGIONS = 9;

const int LEFT_RADAR_DC_ELEM_SRC_IDX = 0;
const int RIGHT_RADAR_DC_ELEM_SRC_IDX = 1;
const int LASER_RECHARGE_DC_ELEM_SRC_IDX = 2;
const int SHIELD_RECHARGE_DC_ELEM_SRC_IDX = 3;
const int ENGINE_RECHARGE_DC_ELEM_SRC_IDX = 4;
const int BEAM_RECHARGE_DC_ELEM_SRC_IDX = 5;
const int SHIELDS_DC_ELEM_SRC_IDX = 6;
const int BEAM_DC_ELEM_SRC_IDX = 7;
const int TARGET_COMP_DC_ELEM_SRC_IDX = 8;
const int QUAD_LASERS_L_DC_ELEM_SRC_IDX = 9;
const int QUAD_LASERS_R_DC_ELEM_SRC_IDX = 10;
const int LEFT_MSG_DC_ELEM_SRC_IDX = 11;
const int RIGHT_MSG_DC_ELEM_SRC_IDX = 12;
const int SPEED_N_THROTTLE_DC_ELEM_SRC_IDX = 13;
const int MISSILES_DC_ELEM_SRC_IDX = 14;
const int NAME_TIME_DC_ELEM_SRC_IDX = 15;
const int NUM_CRAFTS_DC_ELEM_SRC_IDX = 16;
const int QUAD_LASERS_BOTH_DC_ELEM_SRC_IDX = 17;
const int DUAL_LASERS_L_DC_ELEM_SRC_IDX = 18;
const int DUAL_LASERS_R_DC_ELEM_SRC_IDX = 19;
const int DUAL_LASERS_BOTH_DC_ELEM_SRC_IDX = 20;
const int B_WING_LASERS_DC_ELEM_SRC_IDX = 21;
const int SIX_LASERS_BOTH_DC_ELEM_SRC_IDX = 22;
const int SIX_LASERS_L_DC_ELEM_SRC_IDX = 23;
const int SIX_LASERS_R_DC_ELEM_SRC_IDX = 24;
const int SHIELDS_FRONT_DC_ELEM_SRC_IDX = 25;
const int SHIELDS_BACK_DC_ELEM_SRC_IDX = 26;
const int KW_TEXT_CMD_DC_ELEM_SRC_IDX = 27;
const int KW_TEXT_TOP_DC_ELEM_SRC_IDX = 28;
const int KW_TEXT_RADIOSYS_DC_ELEM_SRC_IDX = 29;
const int TEXT_RADIO_DC_ELEM_SRC_IDX = 30;
const int TEXT_SYSTEM_DC_ELEM_SRC_IDX = 31;
const int TEXT_CMD_DC_ELEM_SRC_IDX = 32;
const int TARGETED_OBJ_NAME_SRC_IDX = 33;
const int TARGETED_OBJ_SHD_SRC_IDX = 34;
const int TARGETED_OBJ_HULL_SRC_IDX = 35;
const int TARGETED_OBJ_CARGO_SRC_IDX = 36;
const int TARGETED_OBJ_SYS_SRC_IDX = 37;
const int TARGETED_OBJ_DIST_SRC_IDX = 38;
const int TARGETED_OBJ_SUBCMP_SRC_IDX = 39;
const int MAX_DC_SRC_ELEMENTS = 40;
extern std::vector<const char *>g_DCElemSrcNames;
// Convert a string into a *_DC_ELEM_SRC_IDX constant
int DCSrcElemNameToIndex(char *name);

class DCElemSrcBox {
public:
	Box uv_coords;
	Box coords;
	bool bComputed;

	DCElemSrcBox() {
		bComputed = false;
	}
};

/*
 * Stores the uv_coords and pixel coords for each individual HUD element. Examples of HUD elems
 * are:
 * Laser recharge rate, Shield recharage rate, Radars, etc.
 */
class DCElemSrcBoxes {
public:
	std::vector<DCElemSrcBox> src_boxes;

	void Clear() {
		src_boxes.clear();
	}

	DCElemSrcBoxes();
	void Reset() {
		for (unsigned int i = 0; i < src_boxes.size(); i++)
			src_boxes[i].bComputed = false;
	}
};

enum RenderMainColorKeyType
{
	RENDERMAIN_NO_COLORKEY,
	RENDERMAIN_COLORKEY_20,
	RENDERMAIN_COLORKEY_00,
};

class PrimarySurface;
class DepthSurface;
class BackbufferSurface;
class FrontbufferSurface;
class OffscreenSurface;

/*****************/
// This struct is in the process of moving to the cockpit look hook. It can be removed
// later
typedef struct HeadPosStruct {
	float x, y, z;
} HeadPos;
/*****************/

/* 2D Constant Buffers */
typedef struct MainShadersCBStruct {
	float scale, aspectRatio, parallax, brightness;
	float use_3D, inv_scale, unused1, unused2;
} MainShadersCBuffer;

typedef struct BarrelPixelShaderCBStruct {
	float k1, k2, k3;
	int unused;
} BarrelPixelShaderCBuffer;

#define BACKBUFFER_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM
//#define BLOOM_BUFFER_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM
#define BLOOM_BUFFER_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
#define AO_DEPTH_BUFFER_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
//#define AO_DEPTH_BUFFER_FORMAT DXGI_FORMAT_R32G32B32A32_FLOAT
//#define AO_MASK_FORMAT DXGI_FORMAT_R8_UINT
#define AO_MASK_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM
#define HDR_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT

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
	float fn_max_xymult, fn_scale, fn_sharpness, nm_intensity_near;
	// 80 bytes
	float far_sample_radius, nm_intensity_far, ssao_unused0, amplifyFactor2;
	// 96 bytes
	float x0, y0, x1, y1; // Viewport limits in uv space
	// 112 bytes
	float enable_dist_fade, ssaops_unused1, ssaops_unused2, shadow_epsilon;
	// 128 bytes
	float ssaops_unused3, shadow_step_size, shadow_steps, aspect_ratio;
	// 144 bytes
	float vpScale[4];
	// 160 bytes
	int shadow_enable;
	float shadow_k, Bz_mult, moire_scale;
	// 176 bytes
} SSAOPixelShaderCBuffer;

typedef struct ShadertoyCBStruct {
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
	int bDisneyStyle; // Enables the flare when jumping into hyperspace and other details
	int hyperspace_phase; // 1 = HYPER_ENTRY, 2 = HYPER_TUNNEL, 3 = HYPER_EXIT, 4 = POST_HYPER_EXIT (same as HypespacePhaseEnum)
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

// Let's make this Constant Buffer the same size as the ShadertoyCBuffer
// so that we can reuse the same CB slot -- after all, we can't manipulate
// anything while travelling through hyperspace anyway...
typedef struct LaserPointerCBStruct {
	int TriggerState; // 0 = Not pressed, 1 = Pressed
	float FOVscale, iResolution[2];
	// 16 bytes
	float x0, y0, x1, y1; // Limits in uv-coords of the viewport
	// 32 bytes
	float contOrigin[2], intersection[2];
	// 48 bytes
	int bContOrigin, bIntersection, bHoveringOnActiveElem;
	int DirectSBSEye;
	// 64 bytes
	float v0[2], v1[2]; // DEBUG
	// 80 bytes
	float v2[2], uv[2]; // DEBUG
	// 96
	int bDebugMode;
	float cursor_radius, lp_aspect_ratio[2];
	// 112 bytes
} LaserPointerCBuffer;

/* 3D Constant Buffers */
typedef struct VertexShaderCBStruct {
	float viewportScale[4];
	// 16 bytes
	float aspect_ratio;
	uint32_t apply_uv_comp;
	float z_override, sz_override;
	// 32 bytes
	float mult_z_override, bPreventTransform, bFullTransform, scale_override;
	// 48 bytes
	//float vsunused0, vsunused1, vsunused2, vsunused3;
	// 64 bytes
} VertexShaderCBuffer;

typedef struct VertexShaderMatrixCBStruct {
	Matrix4 projEye;
	Matrix4 viewMat;
	Matrix4 fullViewMat;
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
	// 8 * 16 = 128
	// 464 bytes
	float4 LightPointColor[MAX_CB_POINT_LIGHTS];
	// 8 * 16 = 128
	// 592 bytes
	float ambient, headlights_angle_cos, HDR_white_point;
	uint32_t HDREnabled;
	// 608 bytes
} PSShadingSystemCB;

// See PixelShaderTextureCommon.h for an explanation of these settings
typedef struct PixelShaderCBStruct {
	float brightness;			// Used to control the brightness of some elements -- mostly for ReShade compatibility
	uint32_t DynCockpitSlots;
	uint32_t bUseCoverTexture;
	uint32_t bInHyperspace;
	// 16 bytes
	
	uint32_t bIsLaser;
	uint32_t bIsLightTexture;
	uint32_t bIsEngineGlow;
	uint32_t ps_unused1;
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
	uint32_t special_control;
	float fAmbient;
	// 80 bytes
} PixelShaderCBuffer;

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
	float transparent; // If set to 1, the background will be transparent
	// 448 bytes
} DCPixelShaderCBuffer;

// Vertex Shader constant buffer used in ShadowMapVS.hlsl, register b5
typedef struct ShadowMapVertexShaderMatrixCBStruct {
	Matrix4 Camera;
	Matrix4 lightWorldMatrix[MAX_XWA_LIGHTS];
	// 128 bytes

	uint32_t sm_enabled, sm_debug;
	float sm_light_size, sm_blocker_radius;

	float sm_aspect_ratio, sm_bias, sm_unused, sm_pcss_radius;

	Vector3 POV;
	float sm_resolution;

	int light_index;
	float sm_FOVscale, sm_y_center, sm_z_factor;

	uint32_t sm_PCSS_enabled, sm_pcss_samples, sm_hardware_pcf, sm_VR_mode;

	float sm_black_levels[MAX_XWA_LIGHTS]; // 8 levels: 2 16-byte rows
	float OBJrange[MAX_XWA_LIGHTS]; // 8 ranges: 2 16-byte rows
	float OBJminZ[MAX_XWA_LIGHTS]; // 8 values: 2 16-byte rows
} ShadowMapVertexShaderMatrixCB;

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

typedef struct uv_coords_src_dst_struct {
	int src_slot[MAX_DC_COORDS_PER_TEXTURE]; // This src slot references one of the pre-defined DC internal areas
	uvfloat4 dst[MAX_DC_COORDS_PER_TEXTURE];
	uint32_t uBGColor[MAX_DC_COORDS_PER_TEXTURE];
	uint32_t uHGColor[MAX_DC_COORDS_PER_TEXTURE];
	uint32_t uWHColor[MAX_DC_COORDS_PER_TEXTURE];
	int numCoords;
} uv_src_dst_coords;

typedef struct uv_coords_struct {
	uvfloat4 src[MAX_DC_COORDS_PER_TEXTURE];
	int numCoords;
} uv_coords;

const int MAX_TEXTURE_NAME = 128;
typedef struct dc_element_struct {
	uv_src_dst_coords coords;
	int erase_slots[MAX_DC_COORDS_PER_TEXTURE];
	int num_erase_slots;
	char name[MAX_TEXTURE_NAME];
	char coverTextureName[MAX_TEXTURE_NAME];
	//ComPtr<ID3D11ShaderResourceView> coverTexture = nullptr;
	//ID3D11ShaderResourceView *coverTexture = NULL;
	bool bActive, bNameHasBeenTested, bHologram, bNoisyHolo, bTransparent;
} dc_element;

typedef struct move_region_coords_struct {
	int region_slot[MAX_HUD_BOXES];
	uvfloat4 dst[MAX_HUD_BOXES];
	int numCoords;
} move_region_coords;

// ACTIVE COCKPIT
#define MAX_AC_COORDS_PER_TEXTURE 64
#define MAX_AC_TEXTURES_PER_COCKPIT 16
#define MAX_AC_ACTION_LEN 8 // WORDs (scan codes) used to specify an action
#define AC_HOLOGRAM_FAKE_VK_CODE 0x01 // Internal AC code to toggle the holograms
typedef struct ac_uv_coords_struct {
	uvfloat4 area[MAX_AC_COORDS_PER_TEXTURE];
	WORD action[MAX_AC_COORDS_PER_TEXTURE][MAX_AC_ACTION_LEN]; // List of scan codes
	char action_name[MAX_AC_COORDS_PER_TEXTURE][16]; // For debug purposes only, remove later
	int numCoords;
} ac_uv_coords;

typedef struct ac_element_struct {
	ac_uv_coords coords;
	//int idx; // "Back pointer" into the g_ACElements array
	char name[MAX_TEXTURE_NAME];
	bool bActive, bNameHasBeenTested;
	short width, height; // DEBUG, remove later
} ac_element;

// SSAO Type
typedef enum {
	SSO_AMBIENT,
	SSO_DIRECTIONAL,
	SSO_BENT_NORMALS,
	SSO_DEFERRED, // New Shading Model
} SSAOTypeEnum;

// In order to blend the background with the hyperspace effect when exiting, we need to extend the
// effect for a few more frames. To do that, we need to track the current state of the effect and
// that's why we need a small state machine:
enum HyperspacePhaseEnum {
	HS_INIT_ST = 0,				// Initial state, we're not even in Hyperspace
	HS_HYPER_ENTER_ST = 1,		// We're entering hyperspace
	HS_HYPER_TUNNEL_ST = 2,		// Traveling through the blue Hyperspace tunnel
	HS_HYPER_EXIT_ST = 3,		// HyperExit streaks are being rendered
	HS_POST_HYPER_EXIT_ST = 4   // HyperExit streaks have finished rendering; but now we're blending with the backround
};
const int MAX_POST_HYPER_EXIT_FRAMES = 10; // I had 20 here up to version 1.1.1. Making this smaller makes the zoom faster

// General types and globals
typedef enum {
	TRACKER_NONE,
	TRACKER_FREEPIE,
	TRACKER_STEAMVR,
	TRACKER_TRACKIR
} TrackerType;

struct MaterialStruct;
extern struct MaterialStruct g_DefaultGlobalMaterial;

// Materials
typedef struct MaterialStruct {
	float Metallic;
	float Intensity;
	float Glossiness;
	float NMIntensity;
	float SpecValue;
	bool  IsShadeless;
	bool  NoBloom;
	Vector3 Light;
	Vector2 LightUVCoordPos;
	bool  IsLava;
	float LavaSpeed;
	float LavaSize;
	float EffectBloom;
	Vector3 LavaColor;
	bool LavaTiling;
	bool AlphaToBloom;
	bool NoColorAlpha; // When set, forces the alpha of the color output to 0
	bool AlphaIsntGlass; // When set, semi-transparent areas aren't translated to a Glass material
	float Ambient;
	int TotalFrames; // Used for animated DAT files, like the explosions
	// DEBUG properties, remove later
	//Vector3 LavaNormalMult;
	//Vector3 LavaPosMult;
	//bool LavaTranspose;

	MaterialStruct() {
		Metallic		= g_DefaultGlobalMaterial.Metallic;
		Intensity		= g_DefaultGlobalMaterial.Intensity;
		Glossiness		= g_DefaultGlobalMaterial.Glossiness;
		NMIntensity		= g_DefaultGlobalMaterial.NMIntensity;
		SpecValue		= g_DefaultGlobalMaterial.SpecValue;
		IsShadeless		= g_DefaultGlobalMaterial.IsShadeless;
		Light			= g_DefaultGlobalMaterial.Light;
		LightUVCoordPos = Vector2(0.1f, 0.9f);
		NoBloom			= false;
		IsLava			= false;
		LavaSpeed		= 1.0f;
		LavaSize		= 1.0f;
		EffectBloom		= 1.0f;
		LavaTiling		= true;

		LavaColor.x		= 1.00f;
		LavaColor.y		= 0.35f;
		LavaColor.z		= 0.05f;

		AlphaToBloom	= false;
		NoColorAlpha	= false;
		AlphaIsntGlass	= false;
		Ambient			= 0.0f;

		TotalFrames			= 0;

		/*
		// DEBUG properties, remove later
		LavaNormalMult.x = 1.0f;
		LavaNormalMult.y = 1.0f;
		LavaNormalMult.z = 1.0f;

		LavaPosMult.x = -1.0f;
		LavaPosMult.y = -1.0f;
		LavaPosMult.z = -1.0f;
		LavaTranspose = true;
		*/
	}
} Material;

// Color-Light links
class Direct3DTexture;
typedef struct ColorLightPairStruct {
	Direct3DTexture *color, *light;

	ColorLightPairStruct(Direct3DTexture *color) {
		this->color = color;
		this->light = NULL;
	}
} ColorLightPair;

typedef enum {
	GLOBAL_FOV,
	XWAHACKER_FOV,
	XWAHACKER_LARGE_FOV
} FOVtype;
extern FOVtype g_CurrentFOVType;

/*
 * Used to store a list of textures for fast lookup. For instance, all suns must
 * have their associated lights reset after jumping through hyperspace; and all
 * textures with materials can be placed here so that material properties can be
 * re-applied while flying.
 */
/*
typedef struct AuxTextureDataStruct {
	Direct3DTexture *texture;
} AuxTextureData;
*/

// For shadow mapping, maybe we'd like to distinguish between sun-lights and
// planet/background-lights. We'll use this struct to tag lights and fade
// those lights which aren't suns
class XWALightInfo {
public:
	bool bTagged, bIsSun;

public:
	XWALightInfo() {
		this->Reset();
	}

	void Reset() {
		this->bTagged = false;
		this->bIsSun = true;
	}
};

// Text Rendering
// Font indices that can be used with the PrimarySurface::AddText() methods (and others) below
#define FONT_MEDIUM_IDX 0
#define FONT_LARGE_IDX 1
#define FONT_SMALL_IDX 2
#define FONT_BLUE_COLOR 0x5555FF

class TimedMessage {
public:
	time_t t_exp;
	char msg[128];
	short y;
	short font_size_idx;
	uint32_t color;

	TimedMessage() {
		this->msg[0] = 0;
		this->y = 200;
		this->color = FONT_BLUE_COLOR;
		this->font_size_idx = FONT_LARGE_IDX;
	}

	inline bool IsExpired() {
		return this->msg[0] == 0;
	}

	inline void SetMsg(char *msg, time_t seconds, short y, short font_size_idx, uint32_t color) {
		strcpy_s(this->msg, 128, msg);
		this->t_exp = time(NULL) + seconds;
		this->y = y;
		this->font_size_idx = font_size_idx;
		this->color = color;
	}

	inline void Tick() {
		time_t t = time(NULL);
		if (t > this->t_exp)
			this->msg[0] = 0;
	}
};
const int MAX_TIMED_MESSAGES = 3;

/*
  Only rows 0..2 are available
 */
void DisplayTimedMessage(uint32_t seconds, int row, char *msg);

// S0x07D4FA0
struct XwaGlobalLight
{
	/* 0x0000 */ int PositionX;
	/* 0x0004 */ int PositionY;
	/* 0x0008 */ int PositionZ;
	/* 0x000C */ float DirectionX;
	/* 0x0010 */ float DirectionY;
	/* 0x0014 */ float DirectionZ;
	/* 0x0018 */ float Intensity;
	/* 0x001C */ float XwaGlobalLight_m1C;
	/* 0x0020 */ float ColorR;
	/* 0x0024 */ float ColorB;
	/* 0x0028 */ float ColorG;
	/* 0x002C */ float BlendStartIntensity;
	/* 0x0030 */ float BlendStartColor1C;
	/* 0x0034 */ float BlendStartColorR;
	/* 0x0038 */ float BlendStartColorB;
	/* 0x003C */ float BlendStartColorG;
	/* 0x0040 */ float BlendEndIntensity;
	/* 0x0044 */ float BlendEndColor1C;
	/* 0x0048 */ float BlendEndColorR;
	/* 0x004C */ float BlendEndColorB;
	/* 0x0050 */ float BlendEndColorG;
};

#define MAX_HEAP_ELEMS 32
class VectorColor {
public:
	Vector3 P;
	Vector3 col;
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

	void insert(Vector3 P, Vector3 col);
};

#define MAX_SPEED_PARTICLES 256
extern Vector4 g_SpeedParticles[MAX_SPEED_PARTICLES];

#define SHADOW_MAP_SIZE 1024

class ShadowMappingData {
public:
	bool bEnabled;
	bool bAnisotropicMapScale;
	bool bAllLightsTagged;
	bool bMultipleSuns;
	bool bUseShadowOBJ; // This should be set to true when the Shadow OBJ is loaded
	bool bOBJrange_override;
	float fOBJrange_override_value;
	int ShadowMapSize;
	D3D11_VIEWPORT ViewPort;
	int NumVertices, NumIndices; // This should be set when the Shadow OBJ is loaded
	float black_level;
	float POV_XY_FACTOR;
	float POV_Z_FACTOR;
	float FOVDistScale;
	float sw_pcf_bias;
	float hw_pcf_bias;
	//float XWA_LIGHT_Y_CONV_SCALE;
	float shadow_map_mult_x;
	float shadow_map_mult_y;
	float shadow_map_mult_z;

	int DepthBias;
	float DepthBiasClamp;
	float SlopeScaledDepthBias;

	ShadowMappingData() {
		this->bEnabled = false;
		this->bAnisotropicMapScale = true;
		this->bAllLightsTagged = false;
		this->bMultipleSuns = false;
		this->bUseShadowOBJ = false;
		this->NumVertices = 0;
		this->NumIndices = 0;
		this->ShadowMapSize   = SHADOW_MAP_SIZE;
		// Initialize the Viewport
		this->ViewPort.TopLeftX = 0.0f;
		this->ViewPort.TopLeftY = 0.0f;
		this->ViewPort.Height   = (float)this->ShadowMapSize;
		this->ViewPort.Width    = (float)this->ShadowMapSize;
		this->ViewPort.MinDepth = D3D11_MIN_DEPTH;
		this->ViewPort.MaxDepth = D3D11_MAX_DEPTH;
		this->black_level = 0.2f;
		this->POV_XY_FACTOR = 24.974f;
		this->POV_Z_FACTOR = 25.0f;
		this->bAnisotropicMapScale = true;
		this->bOBJrange_override = false;
		this->fOBJrange_override_value = 5.0f;
		//this->FOVDistScale = 624.525f;
		this->FOVDistScale = 620.0f; // This one seems a bit better
		this->sw_pcf_bias = -0.03f;
		this->hw_pcf_bias = -0.03f;
		// The following scale factor is used when tagging lights (associating XWA lights
		// with sun textures). I don't have a good explanation for this value; but
		// it's used to compensate the Y coordinate so that the light and the centroid of
		// the sun texture line up better. I'll investigate this in detail later.
		//this->XWA_LIGHT_Y_CONV_SCALE = -62.5f;

		this->shadow_map_mult_x =  1.0f;
		this->shadow_map_mult_y =  1.0f;
		this->shadow_map_mult_z = -1.0f;

		this->DepthBias = 0;
		this->DepthBiasClamp = 0.0f;
		this->SlopeScaledDepthBias = 0.0f;
	}

	void SetSize(int Width, int Height) {
		this->ShadowMapSize = Width;
		this->ViewPort.Width = (float)ShadowMapSize;
		this->ViewPort.Height = (float)ShadowMapSize;
	}
};

extern ShadowMappingData g_ShadowMapping;

class DeviceResources
{
public:
	DeviceResources();

	HRESULT Initialize();

	HRESULT OnSizeChanged(HWND hWnd, DWORD dwWidth, DWORD dwHeight);

	HRESULT LoadMainResources();

	HRESULT LoadResources();

	void InitInputLayout(ID3D11InputLayout* inputLayout);
	void InitVertexShader(ID3D11VertexShader* vertexShader);
	void InitPixelShader(ID3D11PixelShader* pixelShader);
	void InitTopology(D3D_PRIMITIVE_TOPOLOGY topology);
	void InitRasterizerState(ID3D11RasterizerState* state);
	void InitPSShaderResourceView(ID3D11ShaderResourceView* texView);
	HRESULT InitSamplerState(ID3D11SamplerState** sampler, D3D11_SAMPLER_DESC* desc);
	HRESULT InitBlendState(ID3D11BlendState* blend, D3D11_BLEND_DESC* desc);
	HRESULT InitDepthStencilState(ID3D11DepthStencilState* depthState, D3D11_DEPTH_STENCIL_DESC* desc);
	void InitVertexBuffer(ID3D11Buffer** buffer, UINT* stride, UINT* offset);
	void InitIndexBuffer(ID3D11Buffer* buffer, bool isFormat32);
	void InitViewport(D3D11_VIEWPORT* viewport);
	void InitVSConstantBuffer3D(ID3D11Buffer** buffer, const VertexShaderCBuffer* vsCBuffer);
	void InitVSConstantBufferMatrix(ID3D11Buffer** buffer, const VertexShaderMatrixCB* vsCBuffer);
	void InitPSConstantShadingSystem(ID3D11Buffer** buffer, const PSShadingSystemCB* psCBuffer);
	void InitVSConstantBuffer2D(ID3D11Buffer** buffer, const float parallax, const float aspectRatio, const float scale, const float brightness, const float use_3D);
	void InitVSConstantBufferHyperspace(ID3D11Buffer ** buffer, const ShadertoyCBuffer * psConstants);
	void InitPSConstantBuffer2D(ID3D11Buffer** buffer, const float parallax, const float aspectRatio, const float scale, const float brightness, float inv_scale = 1.0f);
	void InitPSConstantBufferBarrel(ID3D11Buffer** buffer, const float k1, const float k2, const float k3);
	void InitPSConstantBufferBloom(ID3D11Buffer ** buffer, const BloomPixelShaderCBuffer * psConstants);
	void InitPSConstantBufferSSAO(ID3D11Buffer ** buffer, const SSAOPixelShaderCBuffer * psConstants);
	void InitPSConstantBuffer3D(ID3D11Buffer** buffer, const PixelShaderCBuffer *psConstants);
	void InitPSConstantBufferDC(ID3D11Buffer** buffer, const DCPixelShaderCBuffer * psConstants);
	void InitPSConstantBufferHyperspace(ID3D11Buffer ** buffer, const ShadertoyCBuffer * psConstants);
	void InitPSConstantBufferLaserPointer(ID3D11Buffer ** buffer, const LaserPointerCBuffer * psConstants);
	// Shadow Mapping CBs
	void InitVSConstantBufferShadowMap(ID3D11Buffer **buffer, const ShadowMapVertexShaderMatrixCB *vsCBuffer);
	void InitPSConstantBufferShadowMap(ID3D11Buffer **buffer, const ShadowMapVertexShaderMatrixCB *psCBuffer);
	// Metric Reconstruction CBs
	void InitVSConstantBufferMetricRec(ID3D11Buffer **buffer, const MetricReconstructionCB *vsCBuffer);
	void InitPSConstantBufferMetricRec(ID3D11Buffer **buffer, const MetricReconstructionCB *psCBuffer);

	void BuildHUDVertexBuffer(float width, float height);
	void BuildHyperspaceVertexBuffer(float width, float height);
	void BuildPostProcVertexBuffer();
	void InitSpeedParticlesVB();
	void BuildSpeedVertexBuffer();
	void CreateShadowVertexIndexBuffers(D3DTLVERTEX *vertices, WORD *indices, UINT numVertices, UINT numIndices);
	//void FillReticleVertexBuffer(float width, float height /*float sz, float rhw*/);
	//void CreateReticleVertexBuffer();
	void CreateRandomVectorTexture();
	void DeleteRandomVectorTexture();
	void CreateGrayNoiseTexture();
	void DeleteGrayNoiseTexture();
	void ClearDynCockpitVector(dc_element DCElements[], int size);
	void ClearActiveCockpitVector(ac_element ACElements[], int size);

	void ResetDynamicCockpit();

	void ResetActiveCockpit();

	HRESULT RenderMain(char* buffer, DWORD width, DWORD height, DWORD bpp, RenderMainColorKeyType useColorKey = RENDERMAIN_COLORKEY_20);

	HRESULT RetrieveBackBuffer(char* buffer, DWORD width, DWORD height, DWORD bpp);

	UINT GetMaxAnisotropy();

	void CheckMultisamplingSupport();

	bool IsTextureFormatSupported(DXGI_FORMAT format);

	DWORD _displayWidth;
	DWORD _displayHeight;
	DWORD _displayBpp;
	DWORD _displayTempWidth;
	DWORD _displayTempHeight;
	DWORD _displayTempBpp;

	D3D_DRIVER_TYPE _d3dDriverType;
	D3D_FEATURE_LEVEL _d3dFeatureLevel;
	ComPtr<ID3D11Device> _d3dDevice;
	ComPtr<ID3D11DeviceContext> _d3dDeviceContext;
	ComPtr<IDXGISwapChain> _swapChain;
	// Buffers/Textures
	ComPtr<ID3D11Texture2D> _backBuffer;
	ComPtr<ID3D11Texture2D> _offscreenBuffer;
	ComPtr<ID3D11Texture2D> _offscreenBufferR; // When SteamVR is used, _offscreenBuffer becomes the left eye and this one becomes the right eye
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInput;
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInputR; // When SteamVR is used, this is the right eye as input buffer
	// Dynamic Cockpit
	ComPtr<ID3D11Texture2D> _offscreenBufferDynCockpit;    // Used to render the targeting computer dynamically <-- Need to re-check this claim
	ComPtr<ID3D11Texture2D> _offscreenBufferDynCockpitBG;  // Used to render the targeting computer dynamically <-- Need to re-check this claim
	ComPtr<ID3D11Texture2D> _offscreenAsInputDynCockpit;   // HUD elements buffer
	ComPtr<ID3D11Texture2D> _offscreenAsInputDynCockpitBG; // HUD element backgrounds buffer
	ComPtr<ID3D11Texture2D> _DCTextMSAA;				   // "RTV" to render text
	ComPtr<ID3D11Texture2D> _DCTextAsInput;				   // Resolved from DCTextMSAA for use in shaders
	ComPtr<ID3D11Texture2D> _ReticleBufMSAA;			   // "RTV" to render the HUD in VR mode
	ComPtr<ID3D11Texture2D> _ReticleBufAsInput;			   // Resolved from DCTextMSAA for use in shaders
	// Barrel effect
	ComPtr<ID3D11Texture2D> _offscreenBufferPost;  // This is the output of the barrel effect
	ComPtr<ID3D11Texture2D> _offscreenBufferPostR; // This is the output of the barrel effect for the right image when using SteamVR
	ComPtr<ID3D11Texture2D> _steamVRPresentBuffer; // This is the buffer that will be presented for SteamVR
	// ShaderToy effects
	ComPtr<ID3D11Texture2D> _shadertoyBufMSAA;
	ComPtr<ID3D11Texture2D> _shadertoyBufMSAA_R;
	ComPtr<ID3D11Texture2D> _shadertoyBuf;      // No MSAA
	ComPtr<ID3D11Texture2D> _shadertoyBufR;     // No MSAA
	ComPtr<ID3D11Texture2D> _shadertoyAuxBuf;   // No MSAA
	ComPtr<ID3D11Texture2D> _shadertoyAuxBufR;  // No MSAA
	// Bloom
	ComPtr<ID3D11Texture2D> _offscreenBufferBloomMask;  // Used to render the bloom mask
	ComPtr<ID3D11Texture2D> _offscreenBufferBloomMaskR; // Used to render the bloom mask to the right image (SteamVR)
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInputBloomMask;  // Used to resolve offscreenBufferBloomMask
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInputBloomMaskR; // Used to resolve offscreenBufferBloomMaskR
	ComPtr<ID3D11Texture2D> _bloomOutput1; // Output from bloom pass 1
	ComPtr<ID3D11Texture2D> _bloomOutput2; // Output from bloom pass 2
	ComPtr<ID3D11Texture2D> _bloomOutputSum; // Bloom accummulator
	ComPtr<ID3D11Texture2D> _bloomOutput1R; // Output from bloom pass 1, right image (SteamVR)
	ComPtr<ID3D11Texture2D> _bloomOutput2R; // Output from bloom pass 2, right image (SteamVR)
	ComPtr<ID3D11Texture2D> _bloomOutputSumR; // Bloom accummulator (SteamVR)
	// Ambient Occlusion
	ComPtr<ID3D11Texture2D> _depthBuf;
	ComPtr<ID3D11Texture2D> _depthBufR;
	ComPtr<ID3D11Texture2D> _depthBufAsInput;
	ComPtr<ID3D11Texture2D> _depthBufAsInputR; // Used in SteamVR mode
	//ComPtr<ID3D11Texture2D> _depthBuf2;
	//ComPtr<ID3D11Texture2D> _depthBuf2R;
	//ComPtr<ID3D11Texture2D> _depthBuf2AsInput;
	//ComPtr<ID3D11Texture2D> _depthBuf2AsInputR; // Used in SteamVR mode
	ComPtr<ID3D11Texture2D> _bentBuf;		// No MSAA
	ComPtr<ID3D11Texture2D> _bentBufR;		// No MSAA
	ComPtr<ID3D11Texture2D> _ssaoBuf;		// No MSAA
	ComPtr<ID3D11Texture2D> _ssaoBufR;		// No MSAA
	// Shading System
	ComPtr<ID3D11Texture2D> _normBufMSAA;
	ComPtr<ID3D11Texture2D> _normBufMSAA_R;
	ComPtr<ID3D11Texture2D> _normBuf;		 // No MSAA so that it can be both bound to RTV and SRV
	ComPtr<ID3D11Texture2D> _normBufR;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssaoMaskMSAA;
	ComPtr<ID3D11Texture2D> _ssaoMaskMSAA_R;
	ComPtr<ID3D11Texture2D> _ssaoMask;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssaoMaskR;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssMaskMSAA;	
	ComPtr<ID3D11Texture2D> _ssMaskMSAA_R;
	ComPtr<ID3D11Texture2D> _ssMask;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssMaskR;		 // No MSAA
	// Shadow Mapping
	ComPtr<ID3D11Texture2D> _shadowMap;
	ComPtr<ID3D11Texture2D> _shadowMapArray;
	ComPtr<ID3D11Texture2D> _shadowMapDebug; // TODO: Disable this before release
	// Generated/Procedural Textures
	ComPtr<ID3D11Texture2D> _grayNoiseTex;

	// RTVs
	ComPtr<ID3D11RenderTargetView> _renderTargetView;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewR; // When SteamVR is used, _renderTargetView is the left eye, and this one is the right eye
	// Dynamic Cockpit
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpit; // Used to render the HUD to an offscreen buffer
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpitBG; // Used to render the HUD to an offscreen buffer
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpitAsInput; // RTV that writes to _offscreenBufferAsInputDynCockpit directly
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpitAsInputBG; // RTV that writes to _offscreenBufferAsInputDynCockpitBG directly
	ComPtr<ID3D11RenderTargetView> _DCTextRTV;
	ComPtr<ID3D11RenderTargetView> _DCTextAsInputRTV;
	ComPtr<ID3D11RenderTargetView> _ReticleRTV;
	// Barrel Effect
	ComPtr<ID3D11RenderTargetView> _renderTargetViewPost;  // Used for the barrel effect
	ComPtr<ID3D11RenderTargetView> _renderTargetViewPostR; // Used for the barrel effect (right image) when SteamVR is used.
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSteamVRResize; // Used for the barrel effect
	// ShaderToy
	ComPtr<ID3D11RenderTargetView> _shadertoyRTV;
	ComPtr<ID3D11RenderTargetView> _shadertoyRTV_R;
	// Bloom
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomMask  = NULL; // Renders to _offscreenBufferBloomMask
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomMaskR = NULL; // Renders to _offscreenBufferBloomMaskR
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom1;    // Renders to bloomOutput1
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom2;    // Renders to bloomOutput2
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomSum;  // Renders to bloomOutputSum
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom1R;   // Renders to bloomOutput1R
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom2R;   // Renders to bloomOutput2R
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomSumR; // Renders to bloomOutputSumR
	// Ambient Occlusion
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBuf;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBufR;
	//ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBuf2;
	//ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBuf2R;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBentBuf;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBentBufR;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAO;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAO_R;
	// Shading System
	ComPtr<ID3D11RenderTargetView> _renderTargetViewNormBuf;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewNormBufR;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAOMask;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAOMaskR;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSMask;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSMaskR;

	// SRVs
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputShaderResourceView;
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputShaderResourceViewR; // When SteamVR is enabled, this is the SRV for the right eye
	// Dynamic Cockpit
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputDynCockpitSRV;    // SRV for HUD elements without background
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputDynCockpitBG_SRV; // SRV for HUD element backgrounds
	ComPtr<ID3D11ShaderResourceView> _DCTextSRV;						// SRV for the HUD text
	ComPtr<ID3D11ShaderResourceView> _ReticleSRV;						// SRV for the HUD text
	// Shadertoy
	ComPtr<ID3D11ShaderResourceView> _shadertoySRV;
	ComPtr<ID3D11ShaderResourceView> _shadertoySRV_R;
	ComPtr<ID3D11ShaderResourceView> _shadertoyAuxSRV;
	ComPtr<ID3D11ShaderResourceView> _shadertoyAuxSRV_R;
	// Bloom
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputBloomMaskSRV;
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputBloomMaskSRV_R;
	ComPtr<ID3D11ShaderResourceView> _bloomOutput1SRV;     // SRV for bloomOutput1
	ComPtr<ID3D11ShaderResourceView> _bloomOutput2SRV;     // SRV for bloomOutput2
	ComPtr<ID3D11ShaderResourceView> _bloomOutputSumSRV;   // SRV for bloomOutputSum
	ComPtr<ID3D11ShaderResourceView> _bloomOutput1SRV_R;   // SRV for bloomOutput1R
	ComPtr<ID3D11ShaderResourceView> _bloomOutput2SRV_R;   // SRV for bloomOutput2R
	ComPtr<ID3D11ShaderResourceView> _bloomOutputSumSRV_R; // SRV for bloomOutputSumR
	// Ambient Occlusion
	ComPtr<ID3D11ShaderResourceView> _depthBufSRV;    // SRV for depthBufAsInput
	ComPtr<ID3D11ShaderResourceView> _depthBufSRV_R;  // SRV for depthBufAsInputR
	//ComPtr<ID3D11ShaderResourceView> _depthBuf2SRV;   // SRV for depthBuf2AsInput
	//ComPtr<ID3D11ShaderResourceView> _depthBuf2SRV_R; // SRV for depthBuf2AsInputR
	ComPtr<ID3D11ShaderResourceView> _bentBufSRV;     // SRV for bentBuf
	ComPtr<ID3D11ShaderResourceView> _bentBufSRV_R;   // SRV for bentBufR
	ComPtr<ID3D11ShaderResourceView> _ssaoBufSRV;     // SRV for ssaoBuf
	ComPtr<ID3D11ShaderResourceView> _ssaoBufSRV_R;   // SRV for ssaoBuf
	// Shading System
	ComPtr<ID3D11ShaderResourceView> _normBufSRV;     // SRV for normBuf
	ComPtr<ID3D11ShaderResourceView> _normBufSRV_R;   // SRV for normBufR
	ComPtr<ID3D11ShaderResourceView> _ssaoMaskSRV;    // SRV for ssaoMask
	ComPtr<ID3D11ShaderResourceView> _ssaoMaskSRV_R;  // SRV for ssaoMaskR
	ComPtr<ID3D11ShaderResourceView> _ssMaskSRV;      // SRV for ssMask
	ComPtr<ID3D11ShaderResourceView> _ssMaskSRV_R;    // SRV for ssMaskR
	// Shadow Mapping
	ComPtr<ID3D11ShaderResourceView> _shadowMapArraySRV; // This is an array SRV
	//ComPtr<ID3D11ShaderResourceView> _shadowMapSingleSRV;
	//ComPtr<ID3D11ShaderResourceView> _shadowMapSRV_R;
	// Generated/Procedural Textures SRVs
	ComPtr<ID3D11ShaderResourceView> _grayNoiseSRV; // SRV for _grayNoise

	ComPtr<ID3D11Texture2D> _depthStencilL;
	ComPtr<ID3D11Texture2D> _depthStencilR;
	ComPtr<ID3D11DepthStencilView> _depthStencilViewL;
	ComPtr<ID3D11DepthStencilView> _depthStencilViewR;
	ComPtr<ID3D11DepthStencilView> _shadowMapDSV;
	//ComPtr<ID3D11DepthStencilView> _shadowMapDSV_R; // Do I really need a shadow map for the right eye? I don't think so...

	ComPtr<ID2D1Factory> _d2d1Factory;
	ComPtr<IDWriteFactory> _dwriteFactory;
	ComPtr<ID2D1RenderTarget> _d2d1RenderTarget;
	ComPtr<ID2D1RenderTarget> _d2d1OffscreenRenderTarget;
	ComPtr<ID2D1RenderTarget> _d2d1DCRenderTarget;
	ComPtr<ID2D1DrawingStateBlock> _d2d1DrawingStateBlock;

	ComPtr<ID3D11VertexShader> _mainVertexShader;
	ComPtr<ID3D11InputLayout> _mainInputLayout;
	ComPtr<ID3D11PixelShader> _mainPixelShader;
	ComPtr<ID3D11PixelShader> _mainPixelShaderBpp2ColorKey20;
	ComPtr<ID3D11PixelShader> _mainPixelShaderBpp2ColorKey00;
	ComPtr<ID3D11PixelShader> _mainPixelShaderBpp4ColorKey20;
	ComPtr<ID3D11PixelShader> _steamVRMirrorPixelShader;
	ComPtr<ID3D11PixelShader> _barrelPixelShader;
	ComPtr<ID3D11PixelShader> _bloomHGaussPS;
	ComPtr<ID3D11PixelShader> _bloomVGaussPS;
	ComPtr<ID3D11PixelShader> _bloomCombinePS;
	ComPtr<ID3D11PixelShader> _bloomBufferAddPS;
	ComPtr<ID3D11PixelShader> _ssaoPS;
	ComPtr<ID3D11PixelShader> _ssaoBlurPS;
	ComPtr<ID3D11PixelShader> _ssaoAddPS;
	ComPtr<ID3D11PixelShader> _ssdoDirectPS;
	ComPtr<ID3D11PixelShader> _ssdoIndirectPS;
	ComPtr<ID3D11PixelShader> _ssdoAddPS;
	ComPtr<ID3D11PixelShader> _headLightsPS;
	ComPtr<ID3D11PixelShader> _headLightsSSAOPS;
	ComPtr<ID3D11PixelShader> _ssdoBlurPS;
	ComPtr<ID3D11PixelShader> _deathStarPS;
	ComPtr<ID3D11PixelShader> _hyperEntryPS;
	ComPtr<ID3D11PixelShader> _hyperExitPS;
	ComPtr<ID3D11PixelShader> _hyperTunnelPS;
	ComPtr<ID3D11PixelShader> _hyperZoomPS;
	ComPtr<ID3D11PixelShader> _hyperComposePS;
	ComPtr<ID3D11PixelShader> _laserPointerPS;
	ComPtr<ID3D11PixelShader> _fxaaPS;
	ComPtr<ID3D11PixelShader> _externalHUDPS;
	ComPtr<ID3D11PixelShader> _sunShaderPS;
	ComPtr<ID3D11PixelShader> _sunFlareShaderPS;
	ComPtr<ID3D11PixelShader> _sunFlareComposeShaderPS;
	ComPtr<ID3D11PixelShader> _edgeDetectorPS;
	ComPtr<ID3D11PixelShader> _starDebugPS;
	ComPtr<ID3D11PixelShader> _lavaPS;
	ComPtr<ID3D11PixelShader> _explosionPS;
	ComPtr<ID3D11PixelShader> _alphaToBloomPS;
	ComPtr<ID3D11PixelShader> _noGlassPS;
	ComPtr<ID3D11SamplerState> _repeatSamplerState;
	
	ComPtr<ID3D11PixelShader> _speedEffectPS;
	ComPtr<ID3D11PixelShader> _speedEffectComposePS;
	ComPtr<ID3D11PixelShader> _addGeomPS;
	ComPtr<ID3D11PixelShader> _addGeomComposePS;

	ComPtr<ID3D11PixelShader> _shadowMapPS;
	ComPtr<ID3D11SamplerState> _shadowPCFSamplerState;

	ComPtr<ID3D11PixelShader> _singleBarrelPixelShader;
	ComPtr<ID3D11RasterizerState> _mainRasterizerState;
	ComPtr<ID3D11SamplerState> _mainSamplerState;
	ComPtr<ID3D11BlendState> _mainBlendState;
	ComPtr<ID3D11DepthStencilState> _mainDepthState;
	ComPtr<ID3D11Buffer> _mainVertexBuffer;
	ComPtr<ID3D11Buffer> _steamVRPresentVertexBuffer; // Used in SteamVR mode to correct the image presented on the monitor
	ComPtr<ID3D11Buffer> _mainIndexBuffer;
	ComPtr<ID3D11Texture2D> _mainDisplayTexture;
	ComPtr<ID3D11ShaderResourceView> _mainDisplayTextureView;
	ComPtr<ID3D11Texture2D> _mainDisplayTextureTemp;
	ComPtr<ID3D11ShaderResourceView> _mainDisplayTextureViewTemp;

	ComPtr<ID3D11VertexShader> _vertexShader;
	ComPtr<ID3D11VertexShader> _sbsVertexShader;
	ComPtr<ID3D11VertexShader> _passthroughVertexShader;
	ComPtr<ID3D11VertexShader> _speedEffectVS;
	ComPtr<ID3D11VertexShader> _addGeomVS;
	ComPtr<ID3D11VertexShader> _shadowMapVS;
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11PixelShader> _pixelShaderTexture;
	ComPtr<ID3D11PixelShader> _pixelShaderDC;
	ComPtr<ID3D11PixelShader> _pixelShaderDCHolo;
	ComPtr<ID3D11PixelShader> _pixelShaderEmptyDC;
	ComPtr<ID3D11PixelShader> _pixelShaderHUD;
	ComPtr<ID3D11PixelShader> _pixelShaderSolid;
	ComPtr<ID3D11PixelShader> _pixelShaderClearBox;
	ComPtr<ID3D11RasterizerState> _rasterizerState;
	//ComPtr<ID3D11RasterizerState> _sm_rasterizerState; // TODO: Remove this if proven useless
	ComPtr<ID3D11Buffer> _VSConstantBuffer;
	ComPtr<ID3D11Buffer> _VSMatrixBuffer;
	ComPtr<ID3D11Buffer> _shadingSysBuffer;
	ComPtr<ID3D11Buffer> _PSConstantBuffer;
	ComPtr<ID3D11Buffer> _PSConstantBufferDC;
	ComPtr<ID3D11Buffer> _barrelConstantBuffer;
	ComPtr<ID3D11Buffer> _bloomConstantBuffer;
	ComPtr<ID3D11Buffer> _ssaoConstantBuffer;
	ComPtr<ID3D11Buffer> _hyperspaceConstantBuffer;
	ComPtr<ID3D11Buffer> _laserPointerConstantBuffer;
	ComPtr<ID3D11Buffer> _mainShadersConstantBuffer;
	ComPtr<ID3D11Buffer> _shadowMappingVSConstantBuffer;
	ComPtr<ID3D11Buffer> _shadowMappingPSConstantBuffer;
	ComPtr<ID3D11Buffer> _metricRecVSConstantBuffer;
	ComPtr<ID3D11Buffer> _metricRecPSConstantBuffer;

	ComPtr<ID3D11Buffer> _postProcessVertBuffer;
	ComPtr<ID3D11Buffer> _HUDVertexBuffer;
	ComPtr<ID3D11Buffer> _clearHUDVertexBuffer;
	ComPtr<ID3D11Buffer> _hyperspaceVertexBuffer;
	ComPtr<ID3D11Buffer> _speedParticlesVertexBuffer;
	ComPtr<ID3D11Buffer> _shadowVertexBuffer;
	ComPtr<ID3D11Buffer> _shadowIndexBuffer;
	//ComPtr<ID3D11Buffer> _reticleVertexBuffer;
	bool _bHUDVerticesReady;

	// Dynamic Cockpit coverTextures:
	ComPtr<ID3D11ShaderResourceView> dc_coverTexture[MAX_DC_SRC_ELEMENTS];

	BOOL _useAnisotropy;
	BOOL _useMultisampling;
	DXGI_SAMPLE_DESC _sampleDesc;
	UINT _backbufferWidth;
	UINT _backbufferHeight;
	DXGI_RATIONAL _refreshRate;
	bool _are16BppTexturesSupported;
	bool _use16BppMainDisplayTexture;
	DWORD _mainDisplayTextureBpp;

	float clearColor[4];
	float clearColorRGBA[4];
	float clearDepth;
	bool sceneRendered;
	bool sceneRenderedEmpty;
	bool inScene;
	bool inSceneBackbufferLocked;

	PrimarySurface* _primarySurface;
	DepthSurface* _depthSurface;
	BackbufferSurface* _backbufferSurface;
	FrontbufferSurface* _frontbufferSurface;
	OffscreenSurface* _offscreenSurface;
};
