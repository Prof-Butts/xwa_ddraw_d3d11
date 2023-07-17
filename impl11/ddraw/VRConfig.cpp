#include "common.h"
#include "VRConfig.h"
#include "config.h"
#include "utils.h"
#include "globals.h"
#include "effects.h"
#include "commonVR.h"
#include "SteamVR.h"
#include "DirectSBS.h"
#include "materials.h"
#include "Direct3DTexture.h"
#include "TextureSurface.h"
#include "SharedMem.h"

#include <map>

// This value (2.0f) was determined experimentally. It provides an almost 1:1 metric reconstruction when compared with the original models
//const float DEFAULT_FOCAL_DIST = 2.0f; 
const float DEFAULT_METRIC_MULT = 1.0f;
const float DEFAULT_HUD_PARALLAX = 1.7f;
const float DEFAULT_TEXT_PARALLAX = 0.45f;
const float DEFAULT_FLOATING_GUI_PARALLAX = 0.450f;
const float DEFAULT_FLOATING_OBJ_PARALLAX = -0.025f;

const float DEFAULT_TECH_LIB_PARALLAX = -2.0f;
const float DEFAULT_TRIANGLE_POINTER_DIST = 0.120f;
const float DEFAULT_GUI_ELEM_PZ_THRESHOLD = 0.0008f;
const float DEFAULT_ZOOM_OUT_SCALE = 0.5f;
const bool DEFAULT_ZOOM_OUT_INITIAL_STATE = false;
//const float DEFAULT_ASPECT_RATIO = 1.33f;
const float DEFAULT_ASPECT_RATIO = 1.25f;
//const float DEFAULT_CONCOURSE_SCALE = 0.4f;
const float DEFAULT_CONCOURSE_SCALE = 12.0f;
//const float DEFAULT_CONCOURSE_ASPECT_RATIO = 2.0f; // Default for non-SteamVR
const float DEFAULT_CONCOURSE_ASPECT_RATIO = 1.33f; // Default for non-SteamVR
const float DEFAULT_GLOBAL_SCALE = 1.0f;
const float DEFAULT_GUI_SCALE = 0.7f;

const float GAME_SCALE_FACTOR = 60.0f; // Estimated empirically
const float GAME_SCALE_FACTOR_Z = 60.0f; // Estimated empirically

/*
const float DEFAULT_LENS_K1 = 2.0f;
const float DEFAULT_LENS_K2 = 0.22f;
const float DEFAULT_LENS_K3 = 0.0f;
*/
const float DEFAULT_LENS_K1 = 3.80f;
const float DEFAULT_LENS_K2 = -0.28f;
const float DEFAULT_LENS_K3 = 100.0f;

//const float DEFAULT_COCKPIT_PZ_THRESHOLD = 0.166f; // I used 0.13f for a long time until I jumped on a TIE-Interceptor
const float DEFAULT_COCKPIT_PZ_THRESHOLD = 10.0f; // De-activated
const int DEFAULT_SKYBOX_INDEX = 2;
const bool DEFAULT_BARREL_EFFECT_STATE = true;
const bool DEFAULT_BARREL_EFFECT_STATE_STEAMVR = false; // SteamVR provides its own lens correction, only enable it if the user really wants it
const float DEFAULT_BRIGHTNESS = 0.95f;
const bool DEFAULT_FLOATING_AIMING_HUD = true;
const int DEFAULT_NATURAL_CONCOURSE_ANIM = 1;
const bool DEFAULT_DYNAMIC_COCKPIT_ENABLED = false;
const bool DEFAULT_FIXED_GUI_STATE = true;
// 6dof
const int DEFAULT_FREEPIE_SLOT = 0;
const float DEFAULT_ROLL_MULTIPLIER = -1.0f;
const float DEFAULT_POS_X_MULTIPLIER = 1.0f;
const float DEFAULT_POS_Y_MULTIPLIER = 1.0f;
const float DEFAULT_POS_Z_MULTIPLIER = -1.0f;
const float DEFAULT_MIN_POS_X = -1.5f;
const float DEFAULT_MAX_POS_X = 1.5f;
const float DEFAULT_MIN_POS_Y = -1.5f;
const float DEFAULT_MAX_POS_Y = 1.5f;
const float DEFAULT_MIN_POS_Z = -0.1f;
const float DEFAULT_MAX_POS_Z = 2.5f;
const bool DEFAULT_RESHADE_ENABLED_STATE = false;
const bool DEFAULT_BLOOM_ENABLED_STATE = false;
//const int MAX_BLOOM_PASSES = 9;
const int DEFAULT_BLOOM_PASSES = 5;
// TODO: Make this toggleable later
const bool DEFAULT_AO_ENABLED_STATE = false;
// cockpit look constants
const float DEFAULT_YAW_MULTIPLIER = 1.0f;
const float DEFAULT_PITCH_MULTIPLIER = 1.0f;
const float DEFAULT_YAW_OFFSET = 0.0f;
const float DEFAULT_PITCH_OFFSET = 0.0f;
const float DEFAULT_RETICLE_SCALE = 0.8f;

const char* FOCAL_DIST_VRPARAM = "focal_dist";
const char* IPD_VRPARAM = "IPD";
//const char *METRIC_MULT_VRPARAM = "stereoscopy_multiplier";
//const char *SIZE_3D_WINDOW_VRPARAM = "3d_window_size";
const char* SIZE_3D_WINDOW_ZOOM_OUT_VRPARAM = "3d_window_zoom_out_size";
const char* WINDOW_ZOOM_OUT_INITIAL_STATE_VRPARAM = "zoomed_out_on_startup";
const char* CONCOURSE_WINDOW_SCALE_VRPARAM = "concourse_window_scale";
const char* COCKPIT_Z_THRESHOLD_VRPARAM = "cockpit_z_threshold";
//const char *ASPECT_RATIO_VRPARAM = "3d_aspect_ratio";
const char* CONCOURSE_ASPECT_RATIO_VRPARAM = "concourse_aspect_ratio";
const char* K1_VRPARAM = "k1";
const char* K2_VRPARAM = "k2";
const char* K3_VRPARAM = "k3";
const char* HUD_PARALLAX_VRPARAM = "HUD_depth";
const char* GUI_PARALLAX_VRPARAM = "GUI_depth";
const char* GUI_OBJ_PARALLAX_VRPARAM = "GUI_target_relative_depth";
const char* TEXT_PARALLAX_VRPARAM = "Text_depth";
const char* TECH_LIB_PARALLAX_VRPARAM = "Tech_Library_relative_depth";
const char* BRIGHTNESS_VRPARAM = "brightness";
const char* VR_MODE_VRPARAM = "VR_Mode"; // Select "None", "DirectSBS" or "SteamVR"
const char* VR_MODE_NONE_SVAL = "None";
const char* VR_MODE_DIRECT_SBS_SVAL = "DirectSBS";
const char* VR_MODE_STEAMVR_SVAL = "SteamVR";
const char* INTERLEAVED_REPROJ_VRPARAM = "SteamVR_Interleaved_Reprojection";
const char* STEAMVR_DISTORTION_ENABLED_VRPARAM = "steamvr_distortion_enabled";
const char* BARREL_EFFECT_STATE_VRPARAM = "apply_lens_correction";
const char* INVERSE_TRANSPOSE_VRPARAM = "alternate_steamvr_eye_inverse";
const char* FLOATING_AIMING_HUD_VRPARAM = "floating_aiming_HUD";
const char* NATURAL_CONCOURSE_ANIM_VRPARAM = "concourse_animations_at_25fps";
const char* DYNAMIC_COCKPIT_ENABLED_VRPARAM = "dynamic_cockpit_enabled";
const char* FIXED_GUI_VRPARAM = "fixed_GUI";
const char* STICKY_ARROW_KEYS_VRPARAM = "sticky_arrow_keys";
const char* RETICLE_SCALE_VRPARAM = "reticle_scale";
const char* TRIANGLE_POINTER_DIST_VRPARAM = "triangle_pointer_distance";
// 6dof vrparams
const char* ROLL_MULTIPLIER_VRPARAM = "roll_multiplier";
const char* FREEPIE_SLOT_VRPARAM = "freepie_slot";
const char* STEAMVR_POS_FROM_FREEPIE_VRPARAM = "steamvr_pos_from_freepie";
// Cockpitlook params
const char* YAW_MULTIPLIER_CLPARAM = "yaw_multiplier";
const char* PITCH_MULTIPLIER_CLPARAM = "pitch_multiplier";
const char* YAW_OFFSET_CLPARAM = "yaw_offset";
const char* PITCH_OFFSET_CLPARAM = "pitch_offset";

// Dynamic Cockpit vrparams
const char* UV_COORDS_DCPARAM = "uv_coords";
const char* COVER_TEX_NAME_DCPARAM = "cover_texture";
const char* COVER_TEX_SIZE_DCPARAM = "cover_texture_size";
const char* ERASE_REGION_DCPARAM = "erase_region";
const char* MOVE_REGION_DCPARAM = "move_region";
const char* CT_BRIGHTNESS_DCPARAM = "cover_texture_brightness";
const char* DC_TARGET_COMP_UV_COORDS_VRPARAM = "dc_target_comp_uv_coords";
const char* DC_LEFT_RADAR_UV_COORDS_VRPARAM = "dc_left_radar_uv_coords";
const char* DC_RIGHT_RADAR_UV_COORDS_VRPARAM = "dc_right_radar_uv_coords";
const char* DC_SHIELDS_PANEL_UV_COORDS_VRPARAM = "dc_shields_panel_uv_coords";
const char* DC_LASERS_PANEL_UV_COORDS_VRPARAM = "dc_lasers_panel_uv_coords";
const char* DC_FRONT_PANEL_UV_COORDS_VRPARAM = "dc_front_panel_uv_coords";

//***************************
// Configuration globals
//***************************
bool g_bCockpitPZHackEnabled = true;
bool g_bOverrideAspectRatio = false;
/*
true if either DirectSBS or SteamVR are enabled. false for original display mode
*/
bool g_bEnableVR = false;
bool g_b3DVisionEnabled = false, g_b3DVisionForceFullScreen = true;
TrackerType g_TrackerType = TRACKER_NONE;

float g_fDebugFOVscale = 1.0f;
float g_fDebugYCenter = 0.0f;

float g_fCockpitPZThreshold = DEFAULT_COCKPIT_PZ_THRESHOLD; // The TIE-Interceptor needs this thresold!
float g_fBackupCockpitPZThreshold = g_fCockpitPZThreshold; // Backup of the cockpit threshold, used when toggling this effect on or off.

const float POVOffsetIncr = 0.025f;

// METRIC 3D RECONSTRUCTION
// The following values were determined by comparing the back-projected 3D reconstructed
// with ddraw against the OBJ exported from the OPT. The values were tweaked until a
// proper match was found.
float g_fOBJ_Z_MetricMult = 44.72f, g_fOBJGlobalMetricMult = 1.432f, g_fOBJCurMetricScale;

// Bloom
bool g_bReshadeEnabled = DEFAULT_RESHADE_ENABLED_STATE;
bool g_bBloomEnabled = DEFAULT_BLOOM_ENABLED_STATE;
BloomConfig g_BloomConfig = { 1 };

float g_fBloomLayerMult[MAX_BLOOM_PASSES + 1] = {
	1.000f, // 0
	1.025f, // 1
	1.030f, // 2
	1.035f, // 3
	1.045f, // 4
	1.055f, // 5
	1.070f, // 6
	1.100f, // 7
};
float g_fBloomSpread[MAX_BLOOM_PASSES + 1] = {
	2.0f, // 0
	3.0f, // 1
	4.0f, // 2
	4.0f, // 3
	4.0f, // 4
	4.0f, // 5
	4.0f, // 6
	4.0f, // 7
};
int g_iBloomPasses[MAX_BLOOM_PASSES + 1] = {
	1, 1, 1, 1, 1, 1, 1, 1
};

//extern FILE *colorFile, *lightFile;

// Debugging (personal): I'm tired of the mouse moving outside the window!
bool g_bKeepMouseInsideWindow = false;

// SSAO
float g_fMoireOffsetDir = 0.02f, g_fMoireOffsetInd = 0.1f;
SSAOTypeEnum g_SSAO_Type = SSO_AMBIENT;
float g_fHangarAmbient = 0.05f, g_fGlobalAmbient = 0.005f;

extern float g_fMoireOffsetDir, g_fMoireOffsetInd;
bool g_bAOEnabled = DEFAULT_AO_ENABLED_STATE;
int g_iSSDODebug = 0, g_iSSAOBlurPasses = 1;
float g_fSSAOZoomFactor = 2.0f, g_fSSAOZoomFactor2 = 4.0f, g_fSSAOWhitePoint = 0.7f, g_fNormWeight = 1.0f, g_fNormalBlurRadius = 0.01f;
float g_fSSAOAlphaOfs = 0.5f;
//float g_fViewYawSign = 1.0f, g_fViewPitchSign = -1.0f; // Old values for SSAO.cfg-based lightsf
float g_fViewYawSign = -1.0f, g_fViewPitchSign = 1.0f; // New values for XwaLights
float g_fSpecIntensity = 1.0f, g_fSpecBloomIntensity = 1.25f, g_fXWALightsSaturation = 0.8f, g_fXWALightsIntensity = 1.0f;
float g_fMinLightIntensity = 0.1f; // When lights are faded, this is the minimum intensity they will get
bool g_bApplyXWALightsIntensity = true, g_bProceduralSuns = true, g_bEnableHeadLights = false, g_bProceduralLava = true;
bool g_bNormalizeLights = false;
bool g_bBlurSSAO = true, g_bDepthBufferResolved = false; // g_bDepthBufferResolved gets reset to false at the end of each frame
bool g_bShowSSAODebug = false, g_bDumpSSAOBuffers = false, g_bEnableIndirectSSDO = false, g_bFNEnable = true, g_bFadeLights = false, g_bDisplayGlowMarks = true;
bool g_bDisableDualSSAO = false, g_bEnableSSAOInShader = true, g_bEnableBentNormalsInShader = true;
bool g_bOverrideLightPos = false, g_bEnableSpeedShader = true, g_bEnableAdditionalGeometry = false;
float g_fSpeedShaderScaleFactor = 35.0f, g_fSpeedShaderParticleSize = 0.0075f, g_fSpeedShaderMaxIntensity = 0.6f, g_fSpeedShaderTrailSize = 0.1f;
float g_fSpeedShaderParticleRange = 50.0f; // This used to be 10.0
float g_fCockpitTranslationScale = 0.0025f; // 1.0f / 400.0f;
float g_fGlowMarkZOfs = -1.0f; // Small offset to make the hit effects more visible (avoids Z-Fighting a little bit)
int g_iSpeedShaderMaxParticles = MAX_SPEED_PARTICLES;
Vector4 g_LightVector[2], g_TempLightVector[2];
Vector4 g_LightColor[2], g_TempLightColor[2];
int g_iHyperStyle = 1; // 0 = Regular, 1 = Disney style, 2 = Interdiction
bool g_bInterdictionActive; // If true, then the current hyperjump must render an interdiction
// The global interdiction bitfield. Each time a mission changes, this bitfield
// is populated from the mission's ini file and bits are turned on for regions
// that contain interdictors. For instance a value of 0x5 means regions 0 and 2
// contain interdictors. This information is used to render the Interdiction Effect.
uint8_t g_iInterdictionBitfield = 0;
// Modulates the magnitude of the cockpit shake when an interdiction is happening.
// Set to 0 to remove the shake effect
float g_fInterdictionShake = 0.5f;
// Modulates the cockpit shake when an interdiction is happening in VR
float g_fInterdictionShakeInVR = 0.0f;
// Controls the angle of the sin/cos used to shake the cockpit during an interdiction
float g_fInterdictionAngleScale = 0.25f;

int g_iDelayedDumpDebugBuffers = 0;
//float g_fFlareAspectMult = 1.0f; // DEBUG: Fudge factor to place the flares on the right spot...

// white_point = 1 --> OK
// white_point = 0.5 --> Makes everything bright
// white_point = 4.0 --> Makes everything dark
// So, a bright scene should cause the white point to go up, and a dark scence should cause
// the white point to go down... but not by much in either direction.
float g_fHDRLightsMultiplier = 2.0f, g_fHDRWhitePoint = 1.0f;
bool g_bHDREnabled = false;
bool g_bGlobalSpecToggle = true;

bool g_bDumpSpecificTex = false;
int g_iDumpSpecificTexIdx = 0;
bool g_bDisplayWidth = false;
extern bool g_bDumpDebug;
//bool g_bDumpBloomBuffers = false;

// This is the current resolution of the screen:
float g_fLensK1 = DEFAULT_LENS_K1;
float g_fLensK2 = DEFAULT_LENS_K2;
float g_fLensK3 = DEFAULT_LENS_K3;

// GUI elements seem to be in the range 0..0.0005, so 0.0008 sounds like a good threshold:
float g_fGUIElemPZThreshold = DEFAULT_GUI_ELEM_PZ_THRESHOLD;
float g_fTrianglePointerDist = DEFAULT_TRIANGLE_POINTER_DIST;
float g_fGlobalScale = DEFAULT_GLOBAL_SCALE;
//float g_fPostProjScale = 1.0f;
float g_fGlobalScaleZoomOut = DEFAULT_ZOOM_OUT_SCALE;
float g_fConcourseScale = DEFAULT_CONCOURSE_SCALE;
float g_fConcourseAspectRatio = DEFAULT_CONCOURSE_ASPECT_RATIO;
float g_fHUDDepth = DEFAULT_HUD_PARALLAX; // The aiming HUD is rendered at this depth
bool g_bFloatingAimingHUD = DEFAULT_FLOATING_AIMING_HUD; // The aiming HUD can be fixed to the cockpit glass or floating
float g_fTextDepth = DEFAULT_TEXT_PARALLAX; // All text gets rendered at this parallax
float g_fFloatingGUIDepth = DEFAULT_FLOATING_GUI_PARALLAX; // Floating GUI elements are rendered at this depth
float g_fFloatingGUIObjDepth = DEFAULT_FLOATING_OBJ_PARALLAX; // The targeted object must be rendered above the Floating GUI
float g_fTechLibraryParallax = DEFAULT_TECH_LIB_PARALLAX;
float g_fAspectRatio = DEFAULT_ASPECT_RATIO;
bool g_bZoomOut = DEFAULT_ZOOM_OUT_INITIAL_STATE;
bool g_bZoomOutInitialState = DEFAULT_ZOOM_OUT_INITIAL_STATE;
float g_fBrightness = DEFAULT_BRIGHTNESS;
float g_fGUIElemsScale = DEFAULT_GUI_SCALE; // Used to reduce the size of all the GUI elements
int g_iFreePIESlot = DEFAULT_FREEPIE_SLOT;
int g_iFreePIEControllerSlot = -1;
bool g_bFixedGUI = DEFAULT_FIXED_GUI_STATE;
//float g_fXWAScale = 1.0f; // This is the scale value as computed during Execute()

bool g_bExternalHUDEnabled = false, g_bEdgeDetectorEnabled = true, g_bStarDebugEnabled = false;
int g_iNaturalConcourseAnimations = DEFAULT_NATURAL_CONCOURSE_ANIM;
bool g_bDynCockpitEnabled = DEFAULT_DYNAMIC_COCKPIT_ENABLED;
float g_fYawMultiplier = DEFAULT_YAW_MULTIPLIER;
float g_fPitchMultiplier = DEFAULT_PITCH_MULTIPLIER;
float g_fRollMultiplier = DEFAULT_ROLL_MULTIPLIER;
float g_fYawOffset = DEFAULT_YAW_OFFSET;
float g_fPitchOffset = DEFAULT_PITCH_OFFSET;
float g_fPosXMultiplier = DEFAULT_POS_X_MULTIPLIER;
float g_fPosYMultiplier = DEFAULT_POS_Y_MULTIPLIER;
float g_fPosZMultiplier = DEFAULT_POS_Z_MULTIPLIER;
float g_fMinPositionX = DEFAULT_MIN_POS_X, g_fMaxPositionX = DEFAULT_MAX_POS_X;
float g_fMinPositionY = DEFAULT_MIN_POS_Y, g_fMaxPositionY = DEFAULT_MAX_POS_Y;
float g_fMinPositionZ = DEFAULT_MIN_POS_Z, g_fMaxPositionZ = DEFAULT_MAX_POS_Z;
bool g_bStickyArrowKeys = false, g_bYawPitchFromMouseOverride = false;

float g_f2DYawMul = 1.0f, g_f2DPitchMul = 1.0f, g_f2DRollMul = 1.0f;

bool g_bEnableLevelsShader = false;
float g_fLevelsWhitePoint = 235.0f;
float g_fLevelsBlackPoint = 16.0f;

#include "D3dRenderer.h"

inline float lerp(float x, float y, float s)
{
	return x + s * (y - x);
}

inline float clamp(float val, float min, float max)
{
	if (val < min) val = min;
	if (val > max) val = max;
	return val;
}

void SetHDRState(bool state)
{
	g_bHDREnabled = state;
	g_ShadingSys_PSBuffer.HDREnabled = g_bHDREnabled;
}

/* Loads the VR parameters from vrparams.cfg */
void LoadVRParams() {
	log_debug("[DBG] Loading view params...");
	FILE* file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;
	// Initialize the current renderer to use Effects by default.
	g_D3dRendererType = D3dRendererType::EFFECTS;

	try {
		error = fopen_s(&file, "./vrparams.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load vrparams.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading vrparams.cfg", error);
		goto next;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Reset the dynamic cockpit vector if we're not rendering in 3D
	//if (!g_bRendering3D && g_DCElements.size() > 0) {
	//	log_debug("[DBG] [DC] Clearing g_DCElements");
	//	ClearDynCockpitVector(g_DCElements);
	//}
	g_iSteamVR_VSync_ms = 11;
	g_iSteamVR_Remaining_ms = 3;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);
			if (_stricmp(param, FOCAL_DIST_VRPARAM) == 0) {
				g_fFocalDist = fValue;
				log_debug("[DBG] Focal Distance: %0.3f", g_fFocalDist);
			}
			else if (_stricmp(param, IPD_VRPARAM) == 0) {
				EvaluateIPD(fValue);
			}
			/*else if (_stricmp(param, METRIC_MULT_VRPARAM) == 0) {
				g_fMetricMult = fValue;
				log_debug("[DBG] [FOV] g_fMetricMult: %0.6f", g_fMetricMult);
			}
			else if (_stricmp(param, SIZE_3D_WINDOW_VRPARAM) == 0) {
				// Size of the window while playing the game
				g_fGlobalScale = fValue;
			}*/
			else if (_stricmp(param, SIZE_3D_WINDOW_ZOOM_OUT_VRPARAM) == 0) {
				// Size of the window while playing the game; but zoomed out to see all the GUI
				g_fGlobalScaleZoomOut = fValue;
			}
			else if (_stricmp(param, WINDOW_ZOOM_OUT_INITIAL_STATE_VRPARAM) == 0) {
				g_bZoomOutInitialState = (bool)fValue;
				g_bZoomOut = (bool)fValue;
			}
			else if (_stricmp(param, CONCOURSE_WINDOW_SCALE_VRPARAM) == 0) {
				// Concourse and 2D menus scale
				g_fConcourseScale = fValue;
			}
			/*
			else if (_stricmp(param, COCKPIT_Z_THRESHOLD_VRPARAM) == 0) {
				g_fCockpitPZThreshold = fValue;
			}
			*/
			/* else if (_stricmp(param, ASPECT_RATIO_VRPARAM) == 0) {
				g_fAspectRatio = fValue;
				g_bOverrideAspectRatio = true;
			} */
			else if (_stricmp(param, CONCOURSE_ASPECT_RATIO_VRPARAM) == 0) {
				g_fConcourseAspectRatio = fValue;
				g_bOverrideAspectRatio = true;
			}
			else if (_stricmp(param, K1_VRPARAM) == 0) {
				g_fLensK1 = fValue;
			}
			else if (_stricmp(param, K2_VRPARAM) == 0) {
				g_fLensK2 = fValue;
			}
			else if (_stricmp(param, K3_VRPARAM) == 0) {
				g_fLensK3 = fValue;
			}
			else if (_stricmp(param, BARREL_EFFECT_STATE_VRPARAM) == 0) {
				g_bDisableBarrelEffect = !((bool)fValue);
			}
			else if (_stricmp(param, HUD_PARALLAX_VRPARAM) == 0) {
				g_fHUDDepth = fValue;
			}
			else if (_stricmp(param, FLOATING_AIMING_HUD_VRPARAM) == 0) {
				g_bFloatingAimingHUD = (bool)fValue;
			}
			else if (_stricmp(param, GUI_PARALLAX_VRPARAM) == 0) {
				// "Floating" GUI elements: targetting computer and the like
				g_fFloatingGUIDepth = fValue;
			}
			else if (_stricmp(param, GUI_OBJ_PARALLAX_VRPARAM) == 0) {
				// "Floating" GUI targeted elements
				g_fFloatingGUIObjDepth = fValue;
			}
			else if (_stricmp(param, TEXT_PARALLAX_VRPARAM) == 0) {
				g_fTextDepth = fValue;
			}
			else if (_stricmp(param, TECH_LIB_PARALLAX_VRPARAM) == 0) {
				g_fTechLibraryParallax = fValue;
			}
			else if (_stricmp(param, BRIGHTNESS_VRPARAM) == 0) {
				g_fBrightness = fValue;
			}
			else if (_stricmp(param, STICKY_ARROW_KEYS_VRPARAM) == 0) {
				g_bStickyArrowKeys = (bool)fValue;
			}
			else if (_stricmp(param, "yaw_pitch_from_mouse_override") == 0) {
				g_bYawPitchFromMouseOverride = (bool)fValue;
			}
			else if (_stricmp(param, VR_MODE_VRPARAM) == 0) {
				if (_stricmp(svalue, VR_MODE_NONE_SVAL) == 0) {
					//g_VRMode = VR_MODE_NONE;
					g_bEnableVR = false;
					g_bSteamVREnabled = false;
					log_debug("[DBG] Disabling VR");
				}
				else if (_stricmp(svalue, VR_MODE_DIRECT_SBS_SVAL) == 0) {
					//g_VRMode = VR_MODE_DIRECT_SBS;
					g_bSteamVREnabled = false;
					g_bEnableVR = true;
					g_D3dRendererType = D3dRendererType::DIRECTSBS;
					// Let's force AspectRatioPreserved in VR mode. The aspect ratio is easier to compute that way
					g_config.AspectRatioPreserved = true;
					log_debug("[DBG] Using Direct SBS mode");
				}
				else if (_stricmp(svalue, VR_MODE_STEAMVR_SVAL) == 0) {
					//g_VRMode = VR_MODE_STEAMVR;
					g_bSteamVREnabled = true;
					g_bEnableVR = true;
					g_D3dRendererType = D3dRendererType::STEAMVR;
					// Let's force AspectRatioPreserved in VR mode. The aspect ratio is easier to compute that way
					g_config.AspectRatioPreserved = true;
					log_debug("[DBG] Using SteamVR");
				}
			}
			else if (_stricmp(param, INTERLEAVED_REPROJ_VRPARAM) == 0) {
				g_bInterleavedReprojection = (bool)fValue;
				if (g_bUseSteamVR) {
					log_debug("[DBG] Setting Interleaved Reprojection to: %d", g_bInterleavedReprojection);
					g_pVRCompositor->ForceInterleavedReprojectionOn(g_bInterleavedReprojection);
				}
			}
			else if (_stricmp(param, STEAMVR_DISTORTION_ENABLED_VRPARAM) == 0) {
				g_bSteamVRDistortionEnabled = (bool)fValue;
			}
			else if (_stricmp(param, NATURAL_CONCOURSE_ANIM_VRPARAM) == 0) {
				g_iNaturalConcourseAnimations = (int)fValue;
			}
			else if (_stricmp(param, FIXED_GUI_VRPARAM) == 0) {
				g_bFixedGUI = (bool)fValue;
			}

			else if (_stricmp(param, "manual_dc_activate") == 0) {
				g_bDCManualActivate = (bool)fValue;
			}

			else if (_stricmp(param, "frame_time_remaining") == 0) {
				g_fFrameTimeRemaining = fValue;
			}

			else if (_stricmp(param, "SteamVR_submit_frame_threshold_ms") == 0) {
				g_iSteamVR_Remaining_ms = (int)fValue;
			}
			else if (_stricmp(param, "SteamVR_VSync_ms") == 0) {
				g_iSteamVR_VSync_ms = (int)fValue;
			}
			else if (_stricmp(param, RETICLE_SCALE_VRPARAM) == 0) {
				g_fReticleScale = fValue;
			}
			else if (_stricmp(param, TRIANGLE_POINTER_DIST_VRPARAM) == 0) {
				g_fTrianglePointerDist = fValue;
			}

			else if (_stricmp(param, "2D_yaw_mul") == 0) {
				g_f2DYawMul = fValue;
			}
			else if (_stricmp(param, "2D_pitch_mul") == 0) {
				g_f2DPitchMul = fValue;
			}
			else if (_stricmp(param, "2D_roll_mul") == 0) {
				g_f2DRollMul = fValue;
			}

			else if (_stricmp(param, "steamvr_mirror_window_scale") == 0) {
				// This one is used to zoom in the view in the mirror window to avoid showing
				// wide-FOV-related artifacts
				g_fSteamVRMirrorWindow3DScale = fValue;
			}
			else if (_stricmp(param, "steamvr_mirror_aspect_ratio") == 0) {
				// A value greater than 0 overrides the automatic aspect ratio computed by ddraw in
				// resizeForSteamVR().
				g_fSteamVRMirrorWindowAspectRatio = fValue;
			}
			else if (_stricmp(param, "steamvr_yaw_pitch_roll_from_mouse_look") == 0) {
				g_bSteamVRYawPitchRollFromMouseLook = (bool)fValue;
			}


			param_read_count++;
		}
	} // while ... read file
	// Apply the initial Zoom Out state:
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;
	fclose(file);

next:
	/**g_fRawFOVDist = g_fDefaultFOVDist;
	*g_cachedFOVDist = g_fDefaultFOVDist / 512.0f;
	*g_rawFOVDist = (uint32_t)g_fDefaultFOVDist;*/

	// Load the 3D Vision params
	Load3DVisionParams();
	// Load cockpit look params
	LoadCockpitLookParams();
	// Load the global dynamic cockpit coordinates
	LoadDCInternalCoordinates();
	// Load Dynamic Cockpit params
	LoadDCParams();
	// Load Active Cockpit params
	LoadACParams();
	// Load the Bloom params
	LoadBloomParams();
	// Load the SSAO params
	LoadSSAOParams();
	// Load the Hyperspace params
	LoadHyperParams();
	// Load FOV params
	LoadFocalLength();
	// Load the default global material
	LoadDefaultGlobalMaterial();
	// Load hook_tourmultiplayer.cfg
	LoadMultiplayerConfig();
	// Load gimbal lock fix parameters
	LoadGimbaLockFixConfig();
	// Reload the materials
	ReloadMaterials();
}

/* Restores the various VR parameters to their default values. */
void ResetVRParams() {
	//g_fFocalDist = g_bSteamVREnabled ? DEFAULT_FOCAL_DIST_STEAMVR : DEFAULT_FOCAL_DIST;
	g_fFocalDist = DEFAULT_FOCAL_DIST;
	//g_fMetricMult = DEFAULT_METRIC_MULT;
	EvaluateIPD(DEFAULT_IPD);
	g_bCockpitPZHackEnabled = true;
	g_fGUIElemPZThreshold = DEFAULT_GUI_ELEM_PZ_THRESHOLD;
	g_fTrianglePointerDist = DEFAULT_TRIANGLE_POINTER_DIST;
	//g_fGlobalScale = g_bSteamVREnabled ? DEFAULT_GLOBAL_SCALE_STEAMVR : DEFAULT_GLOBAL_SCALE;
	g_fGlobalScale = DEFAULT_GLOBAL_SCALE;
	//g_fPostProjScale = 1.0f;
	g_fGlobalScaleZoomOut = DEFAULT_ZOOM_OUT_SCALE;
	g_bZoomOut = g_bZoomOutInitialState;
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;
	g_fConcourseScale = DEFAULT_CONCOURSE_SCALE;
	g_fCockpitPZThreshold = DEFAULT_COCKPIT_PZ_THRESHOLD;
	g_fBackupCockpitPZThreshold = g_fCockpitPZThreshold;
	g_fReticleScale = DEFAULT_RETICLE_SCALE;

	g_fAspectRatio = DEFAULT_ASPECT_RATIO;
	g_fConcourseAspectRatio = DEFAULT_CONCOURSE_ASPECT_RATIO;
	g_fLensK1 = DEFAULT_LENS_K1;
	g_fLensK2 = DEFAULT_LENS_K2;
	g_fLensK3 = DEFAULT_LENS_K3;

	g_bFixSkyBox = true;
	//g_iSkyBoxExecIndex = DEFAULT_SKYBOX_INDEX;
	g_bSkipText = false;
	g_bSkipGUI = false;
	g_bSkipSkyBox = false;

	g_iNoDrawBeforeIndex = 0; g_iNoDrawAfterIndex = -1;
	g_iNoExecBeforeIndex = 0; g_iNoExecAfterIndex = -1;
	g_fHUDDepth = DEFAULT_HUD_PARALLAX;
	g_bFloatingAimingHUD = DEFAULT_FLOATING_AIMING_HUD;
	g_fTextDepth = DEFAULT_TEXT_PARALLAX;
	g_fFloatingGUIDepth = DEFAULT_FLOATING_GUI_PARALLAX;
	g_fTechLibraryParallax = DEFAULT_TECH_LIB_PARALLAX;
	g_fFloatingGUIObjDepth = DEFAULT_FLOATING_OBJ_PARALLAX;
	g_iNaturalConcourseAnimations = DEFAULT_NATURAL_CONCOURSE_ANIM;

	g_fBrightness = DEFAULT_BRIGHTNESS;
	//g_bStickyArrowKeys = false;
	//g_bYawPitchFromMouseOverride = false;

	g_bInterleavedReprojection = DEFAULT_INTERLEAVED_REPROJECTION;
	if (g_bUseSteamVR) {
		g_pVRCompositor->ForceInterleavedReprojectionOn(g_bInterleavedReprojection);
		g_bSteamVRDistortionEnabled = true;
	}

	//g_bDisableBarrelEffect = g_bUseSteamVR ? !DEFAULT_BARREL_EFFECT_STATE_STEAMVR : !DEFAULT_BARREL_EFFECT_STATE;

	g_bFixedGUI = DEFAULT_FIXED_GUI_STATE;

	g_iFreePIESlot = DEFAULT_FREEPIE_SLOT;
	g_fYawMultiplier = DEFAULT_YAW_MULTIPLIER;
	g_fPitchMultiplier = DEFAULT_PITCH_MULTIPLIER;
	g_fRollMultiplier = DEFAULT_ROLL_MULTIPLIER;
	g_fYawOffset = DEFAULT_YAW_OFFSET;
	g_fPitchOffset = DEFAULT_PITCH_OFFSET;
	g_fPosXMultiplier = DEFAULT_POS_X_MULTIPLIER;
	g_fPosYMultiplier = DEFAULT_POS_Y_MULTIPLIER;
	g_fPosZMultiplier = DEFAULT_POS_Z_MULTIPLIER;
	g_fMinPositionX = DEFAULT_MIN_POS_X; g_fMaxPositionX = DEFAULT_MAX_POS_X;
	g_fMinPositionY = DEFAULT_MIN_POS_Y; g_fMaxPositionY = DEFAULT_MAX_POS_Y;
	g_fMinPositionZ = DEFAULT_MIN_POS_Z; g_fMaxPositionZ = DEFAULT_MAX_POS_Z;

	g_iSteamVR_Remaining_ms = 3; g_iSteamVR_VSync_ms = 11;

	// Recompute the eye and projection matrices
	if (!g_bUseSteamVR)
		InitDirectSBS();

	//g_bReshadeEnabled = DEFAULT_RESHADE_ENABLED_STATE;
	//g_bBloomEnabled = DEFAULT_BLOOM_ENABLED_STATE;
	//g_bDynCockpitEnabled = DEFAULT_DYNAMIC_COCKPIT_ENABLED;

	/**g_fRawFOVDist = g_fDefaultFOVDist;
	*g_cachedFOVDist = g_fDefaultFOVDist / 512.0f;
	*g_rawFOVDist = (uint32_t)g_fDefaultFOVDist;*/
	// Load CRCs
	LoadCockpitLookParams();
}

/* Saves the current view parameters to vrparams.cfg */
void SaveVRParams() {
	FILE* file;
	int error = 0;

	try {
		error = fopen_s(&file, "./VRParams.cfg", "wt");
	}
	catch (...) {
		log_debug("[DBG] Could not save VRParams.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when saving VRParams.cfg", error);
		return;
	}
	fprintf(file, "; VR parameters. Write one parameter per line.\n");
	fprintf(file, "; Always make a backup copy of this file before modifying it.\n");
	fprintf(file, "; If you want to restore it to its default settings, delete the\n");
	fprintf(file, "; file and restart the game. Then press Ctrl+Alt+S to save a\n");
	fprintf(file, "; new config file with the default parameters -- however the\n");
	fprintf(file, "; VR mode may need to be set manually.\n");
	fprintf(file, "; To reload this file during game (at any point) just press Ctrl+Alt+L.\n");
	fprintf(file, "; Most parameters can be re-applied when reloading.\n");
	//fprintf(file, "; You can also press Ctrl+Alt+R to reset the viewing params to default values.\n\n");

	fprintf(file, "; VR Mode. Select from None, DirectSBS and SteamVR.\n");
	if (!g_bEnableVR)
		fprintf(file, "%s = %s\n", VR_MODE_VRPARAM, VR_MODE_NONE_SVAL);
	else {
		if (!g_bSteamVREnabled)
			fprintf(file, "%s = %s\n", VR_MODE_VRPARAM, VR_MODE_DIRECT_SBS_SVAL);
		else
			fprintf(file, "%s = %s\n", VR_MODE_VRPARAM, VR_MODE_STEAMVR_SVAL);
	}
	fprintf(file, "\n");
	
	//fprintf(file, "focal_dist = %0.6f # Try not to modify this value, change IPD instead.\n", focal_dist);

	fprintf(file, "; %s is measured in cms. Set it to 0 to remove the stereoscopy effect.\n", IPD_VRPARAM);
	fprintf(file, "; This setting is ignored in SteamVR mode. Configure the IPD through SteamVR instead.\n");
	fprintf(file, "%s = %0.1f\n\n", IPD_VRPARAM, g_fIPD * IPD_SCALE_FACTOR);
	//fprintf(file, "; %s amplifies the stereoscopy of objects in the game. Never set it to 0\n", METRIC_MULT_VRPARAM);
	//fprintf(file, "%s = %0.3f\n", METRIC_MULT_VRPARAM, g_fMetricMult);
	//fprintf(file, "%s = %0.3f\n", SIZE_3D_WINDOW_VRPARAM, g_fGlobalScale);

	fprintf(file, "; Scale of the reticle in VR mode.\n");
	fprintf(file, "%s = %0.3f\n\n", RETICLE_SCALE_VRPARAM, g_fReticleScale);

	fprintf(file, "; The following setting will reduce the scale of the HUD in VR mode.\n");
	fprintf(file, "%s = %0.3f\n\n", SIZE_3D_WINDOW_ZOOM_OUT_VRPARAM, g_fGlobalScaleZoomOut);

	fprintf(file, "; Set the following to 1 to start the HUD in zoomed-out mode:\n");
	fprintf(file, "%s = %d\n\n", WINDOW_ZOOM_OUT_INITIAL_STATE_VRPARAM, g_bZoomOutInitialState);

	fprintf(file, "%s = %0.3f\n\n", CONCOURSE_WINDOW_SCALE_VRPARAM, g_fConcourseScale);

	fprintf(file, "; The concourse animations can be played as fast as possible, or at its original\n");
	fprintf(file, "; 25fps setting:\n");
	fprintf(file, "%s = %d\n\n", NATURAL_CONCOURSE_ANIM_VRPARAM, g_iNaturalConcourseAnimations);
	/*
	fprintf(file, "; The following is a hack to increase the stereoscopy on objects. Unfortunately it\n");
	fprintf(file, "; also causes some minor artifacts: this is basically the threshold between the\n");
	fprintf(file, "; cockpit and the 'outside' world in normalized coordinates (0 is ZNear 1 is ZFar).\n");
	fprintf(file, "; Set it to 2 to disable this hack (stereoscopy will be reduced).\n");
	fprintf(file, "%s = %0.3f\n\n", COCKPIT_Z_THRESHOLD_VRPARAM, g_fCockpitPZThreshold);
	*/

	//fprintf(file, "\n; Specify the aspect ratio here to override the aspect ratio computed by the library.\n");
	//fprintf(file, "; ALWAYS specify BOTH the Concourse and 3D window aspect ratio.\n");
	//fprintf(file, "; You can also edit ddraw.cfg and set 'PreserveAspectRatio = 1' to get the library to\n");
	//fprintf(file, "; estimate the aspect ratio for you (this is the preferred method).\n");
	//fprintf(file, "%s = %0.3f\n", ASPECT_RATIO_VRPARAM, g_fAspectRatio);
	fprintf(file, "%s = %0.3f\n\n", CONCOURSE_ASPECT_RATIO_VRPARAM, g_fConcourseAspectRatio);

	fprintf(file, "; DirectSBS Lens correction parameters -- ignored in SteamVR mode.\n");
	fprintf(file, "; k2 has the biggest effect and k1 fine-tunes the effect.\n");
	fprintf(file, "; Positive values = convex warping; negative = concave warping.\n");
	fprintf(file, "%s = %0.6f\n", K1_VRPARAM, g_fLensK1);
	fprintf(file, "%s = %0.6f\n", K2_VRPARAM, g_fLensK2);
	fprintf(file, "%s = %0.6f\n", K3_VRPARAM, g_fLensK3);
	fprintf(file, "%s = %d\n\n", BARREL_EFFECT_STATE_VRPARAM, !g_bDisableBarrelEffect);

	/*
	fprintf(file, "; The following parameter will enable/disable SteamVR's lens distortion correction\n");
	fprintf(file, "; The default is 1, only set it to 0 if you're seeing distortion in SteamVR.\n");
	fprintf(file, "; If you set it to 0, I suggest you enable %s above to use the internal lens\n", BARREL_EFFECT_STATE_VRPARAM);
	fprintf(file, "; distortion correction instead\n");
	fprintf(file, "%s = %d\n\n", STEAMVR_DISTORTION_ENABLED_VRPARAM, g_bSteamVRDistortionEnabled);
	*/

	fprintf(file, "; Depth for various GUI elements in meters from the head's origin.\n");
	fprintf(file, "; Positive depth is forwards, negative is backwards (towards you).\n");
	fprintf(file, "; As a reference, the background starfield is 65km meters away.\n");
	fprintf(file, "%s = %0.3f\n\n", HUD_PARALLAX_VRPARAM, g_fHUDDepth);

	fprintf(file, "; If 6dof is enabled, the aiming HUD can be fixed to the cockpit or it can \"float\"\n");
	fprintf(file, "; and follow the lasers. When it's fixed, it's probably more realistic; but it will\n");
	fprintf(file, "; be harder to aim when you lean.\n");
	fprintf(file, "; When the aiming HUD is floating, it will follow the lasers when you lean,\n");
	fprintf(file, "; making it easier to aim properly.\n");
	fprintf(file, "%s = %d\n\n", FLOATING_AIMING_HUD_VRPARAM, g_bFloatingAimingHUD);

	fprintf(file, "%s = %0.3f\n\n", GUI_PARALLAX_VRPARAM, g_fFloatingGUIDepth);

	fprintf(file, "%s = %0.3f\n\n", GUI_OBJ_PARALLAX_VRPARAM, g_fFloatingGUIObjDepth);

	fprintf(file, "; %s is relative and it's always added to %s\n", GUI_OBJ_PARALLAX_VRPARAM, GUI_PARALLAX_VRPARAM);
	fprintf(file, "; This has the effect of making the targeted object \"hover\" above the targeting computer\n");
	fprintf(file, "; As a rule of thumb always make %s <= %s so that\n", TEXT_PARALLAX_VRPARAM, GUI_PARALLAX_VRPARAM);
	fprintf(file, "; the text hovers above the targeting computer\n\n");
	fprintf(file, "%s = %0.3f\n\n", TEXT_PARALLAX_VRPARAM, g_fTextDepth);

	fprintf(file, "; This is the depth added to the controls in the tech library. Make it negative to bring the\n");
	fprintf(file, "; controls towards you. Objects in the tech library are obviously scaled by XWA, because there's\n");
	fprintf(file, "; otherwise no way to visualize both a Star Destroyer and an A-Wing in the same volume.\n");
	fprintf(file, "%s = %0.3f\n\n", TECH_LIB_PARALLAX_VRPARAM, g_fTechLibraryParallax);

	fprintf(file, "; The HUD/GUI can be fixed in space now. If this setting is enabled, you'll be\n");
	fprintf(file, "; able to see all the HUD simply by looking around. You may also lean forward to\n");
	fprintf(file, "; zoom-in on the text messages to make them more readable.\n");
	fprintf(file, "%s = %d\n\n", FIXED_GUI_VRPARAM, g_bFixedGUI);

	fprintf(file, "; Set the following parameter to lower the brightness of the text,\n");
	fprintf(file, "; Concourse and 2D menus (avoids unwanted bloom when using ReShade).\n");
	fprintf(file, "; A value of 1 is normal brightness, 0 will render everything black.\n");
	fprintf(file, "%s = %0.3f\n\n", BRIGHTNESS_VRPARAM, g_fBrightness);

	fprintf(file, "; Interleaved Reprojection is a SteamVR setting that locks the framerate at 45fps.\n");
	fprintf(file, "; In some cases, it may help provide a smoother experience. Try toggling it\n");
	fprintf(file, "; to see what works better for your specific case.\n");
	fprintf(file, "%s = %d\n\n", INTERLEAVED_REPROJ_VRPARAM, g_bInterleavedReprojection);

	//fprintf(file, "\n");
	//fprintf(file, "%s = %d\n", INVERSE_TRANSPOSE_VRPARAM, g_bInverseTranspose);
	fprintf(file, "; Cockpit roll multiplier. Set it to 0 to de-activate this axis.\n");
	fprintf(file, "; The settings for pitch, yaw and positional tracking are in CockpitLook.cfg\n");
	fprintf(file, "%s = %0.3f\n\n", ROLL_MULTIPLIER_VRPARAM, g_fRollMultiplier);

	// STEAMVR_POS_FROM_FREEPIE_VRPARAM is not saved because it's kind of a hack -- I'm only
	// using it because the PSMoveServiceSteamVRBridge is a bit tricky to setup and why would
	// I do that when my current FreePIEBridgeLite is working properly -- and faster.

	fprintf(file, "; Places the triangle pointer at the specified distance from the center of the\n");
	fprintf(file, "; screen. A value of 0 places it right at the center, a value of 0.5 puts it\n");
	fprintf(file, "; near the edge of the screen.\n");
	fprintf(file, "%s = %0.3f\n\n", TRIANGLE_POINTER_DIST_VRPARAM, g_fTrianglePointerDist);

	fclose(file);
	log_debug("[DBG] vrparams.cfg saved");
}

/* Loads cockpitlook params that are relevant to tracking */
void LoadCockpitLookParams() {
	log_debug("[DBG] Loading cockpit look params...");
	FILE* file;
	int error = 0;

	try {
		error = fopen_s(&file, "./cockpitlook.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load cockpitlook.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading cockpitlook.cfg", error);
		return;
	}

	char buf[160], param[80], svalue[80];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 160, file) != NULL) {
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 80, svalue, 80) > 0) {
			fValue = (float)atof(svalue);
			if (_stricmp(param, YAW_MULTIPLIER_CLPARAM) == 0) {
				g_fYawMultiplier = fValue;
			}
			else if (_stricmp(param, PITCH_MULTIPLIER_CLPARAM) == 0) {
				g_fPitchMultiplier = fValue;
			}
			else if (_stricmp(param, YAW_OFFSET_CLPARAM) == 0) {
				g_fYawOffset = fValue;
			}
			else if (_stricmp(param, PITCH_OFFSET_CLPARAM) == 0) {
				g_fPitchOffset = fValue;
			}
			else if (_stricmp(param, "tracker_type") == 0) {
				if (_stricmp(svalue, "FreePIE") == 0) {
					log_debug("Using FreePIE for tracking");
					g_TrackerType = TRACKER_FREEPIE;
				}
				else if (_stricmp(svalue, "SteamVR") == 0) {
					log_debug("Using SteamVR for tracking");
					g_TrackerType = TRACKER_STEAMVR;
				}
				else if (_stricmp(svalue, "TrackIR") == 0) {
					log_debug("Using TrackIR for tracking");
					g_TrackerType = TRACKER_TRACKIR;
				}
				else if (_stricmp(svalue, "None") == 0) {
					log_debug("Tracking disabled");
					g_TrackerType = TRACKER_NONE;
				}

			}
			/*else if (_stricmp(param, "cockpit_inertia_enabled") == 0) {
				g_bCockpitInertiaEnabled = (bool)fValue;
				log_debug("[DBG] Cockpit Inertia: %d", g_bCockpitInertiaEnabled);
			}*/
			// 6dof parameters
			else if (_stricmp(param, FREEPIE_SLOT_VRPARAM) == 0) {
				g_iFreePIESlot = (int)fValue;
			}
			param_read_count++;
		}
	} // while ... read file
	fclose(file);
	log_debug("[DBG] Loaded %d cockpitlook params", param_read_count);
}

/* Save the current FOV and metric multiplier to an external file */
void SaveFocalLength() {
	FILE* file;
	int error = 0;

	try {
		error = fopen_s(&file, "./FocalLength.cfg", "wt");
	}
	catch (...) {
		log_debug("[DBG] [FOV] Could not save FocalLength.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] [FOV] Error %d when saving FocalLength.cfg", error);
		return;
	}

	//fprintf(file, "; The focal length is measured in pixels. This parameter can be modified without\n");
	//fprintf(file, "; VR, so, technically, it's not a 'VRParam'\n");
	// Let's not write the focal length in pixels anymore. It doesn't make any sense to
	// anyone and it's only useful internally. We'll continue to read it, but let's start
	// using something sensible
	//fprintf(file, "focal_length = %0.6f\n", *g_fRawFOVDist);
	// Save the *real* vert FOV
	fprintf(file, "; The FOV is measured in degrees. This is the actual vertical FOV.\n");
	if (!g_bEnableVR) {
		fprintf(file, "; This FOV is used when the game is in non-VR mode.\n");
		fprintf(file, "real_FOV = %0.3f\n", ComputeRealVertFOV());
	}
	else {
		fprintf(file, "; This FOV is used only when the game is in VR mode.\n");
		fprintf(file, "VR_FOV = %0.3f\n", ComputeRealVertFOV());
	}
	fclose(file);
}

bool LoadFocalLength() {
	log_debug("[DBG] [FOV] Loading FocalLength...");
	FILE* file;
	int error = 0;
	bool bApplied = false;

	try {
		error = fopen_s(&file, "./FocalLength.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] [FOV] Could not load FocalLength.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] [FOV] Error %d when loading FocalLength.cfg", error);
		return bApplied;
	}

	char buf[160], param[80], svalue[80];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 160, file) != NULL) {
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 80, svalue, 80) > 0) {
			fValue = (float)atof(svalue);
			if (_stricmp(param, "focal_length") == 0) {
				ApplyFocalLength(fValue);
				log_debug("[DBG] [FOV] Applied FOV: %0.3f", fValue);
				bApplied = true;
			}
			else if (_stricmp(param, "real_FOV") == 0 && !g_bEnableVR) {
				float RawFocalLength = RealVertFOVToRawFocalLength(fValue);
				ApplyFocalLength(RawFocalLength);
				log_debug("[DBG] [FOV] Applied Real FOV: %0.3f", RawFocalLength);
				bApplied = true;
			}
			else if (_stricmp(param, "VR_FOV") == 0 && g_bEnableVR) {
				float RawFocalLength = RealVertFOVToRawFocalLength(fValue);
				ApplyFocalLength(RawFocalLength);
				log_debug("[DBG] [FOV] Applied VR FOV: %0.3f", RawFocalLength);
				bApplied = true;
			}
		}
	}
	fclose(file);
	return bApplied;
}

float ComputeRealVertFOV() {
	return 2.0f * atan2(0.5f * g_fCurInGameHeight, *g_fRawFOVDist) / DEG2RAD;
}

float ComputeRealHorzFOV() {
	return 2.0f * atan2(0.5f * g_fCurInGameWidth, *g_fRawFOVDist) / DEG2RAD;
}

float RealVertFOVToRawFocalLength(float real_FOV_deg) {
	return 0.5f * g_fCurInGameHeight / tan(0.5f * real_FOV_deg * DEG2RAD);
}

/**
 * Compute FOVscale and y_center for the hyperspace effect (and others that may need the FOVscale)
 * This function is no longer used, but since it took *a lot* of effort to get it right, I'm keeping it
 * just in case we need to review how it worked later.
 * This function needs to capture the reticle's position on the screen, so it's a bit complicated.
 */
void ComputeHyperFOVParamsOld() {
	// m0rgg found this variable. We can use it to calibrate the camera without having to capture the reticle.
	DWORD *ReticleYCenter = (DWORD *)0x68C55C;
	float y_center_raw, FOVscale_raw;
	// Find the current center of the screen after it has been displaced by the cockpit camera.
	// We only care about the y-coordinate, as we're going to use it along with the reticle to
	// compute y_center.

	// To find the y-center of the screen, we're going to use math. The tangent of the current
	// pitch can give us the information right away. This is closely related to the FOV and
	// is easy to see if you draw the focal length and the in-game height to form a right triangle.
	float pitch = (float)PlayerDataTable[*g_playerIndex].MousePositionY / 65536.0f * 2.0f * PI;
	float H = *g_fRawFOVDist * tan(pitch); // This will give us the height, in pixels, measured from the center of the screen
	H += g_fCurInGameHeight / 2.0f;
	log_debug("[DBG] [FOV] ReticleYCenter: %u, Screen Y-Center: %0.3f, ReticleCentroid: %0.3f, pitch: %0.3f",
		*ReticleYCenter, H, g_ReticleCentroid.y, pitch / DEG2RAD);
	if (g_ReticleCentroid.y > -1.0f) {
		// *sigh* for whatever stupid reason, sometimes we can have ReticleCentroid.y == 0.0 while looking straight ahead (pitch == 0)
		// This situation completely destroys the calculation below, so we need to make sure that the camera pitch and the centroid
		// are consistent.
		// Another way to solve this problem is to prevent this calculation until the reticle centroid is close-ish to the center
		// of the screen
		float y_dist_from_screen_center = (float)fabs(g_ReticleCentroid.y - g_fCurInGameHeight / 2.0f);
		if (y_dist_from_screen_center < g_fCurInGameHeight / 2.0f) {
			// The reticle center visible this frame and it's not close to the edge of the screen.
			// We can use it to compute y_center...

			// The formula to compute y_center seems to be:
			// (in-game-center - HUD_center) / in-game-height * 2.0f * comp_factor.
			// The in-game-center has to be computer properly if the cockpit isn't facing forward
			y_center_raw = 2.0f * (H - g_ReticleCentroid.y) / g_fCurInGameHeight;
			log_debug("[DBG] [FOV] HUD_center to y_center: %0.3f", y_center_raw);
			// We can stop looking for the reticle center now:
			g_bYCenterHasBeenFixed = true;
		}
		else
			log_debug("[DBG] [FOV] RETICLE COULD NOT BE USED COMPUTE Y_CENTER. WILL RETRY. Frame: %d", g_iPresentCounter);
		// If the reticle center can't be used to compute y_center, then g_bYCenterHasBeenFixed will stay false, and we'll
		// come back to this path on the next frame where a reticle is visible.
	}

	if (!g_bYCenterHasBeenFixed) {
		// Provide a default value if we couldn't compute y_center:
		y_center_raw = 153.0f / g_fCurInGameHeight;
		//y_center_raw = 0.0f; // I can't do this because the cockpits look wrong in the hangar. They look skewed
	}
	FOVscale_raw = 2.0f * *g_fRawFOVDist / g_fCurInGameHeight;

	// Compute the aspect-ratio fix factors
	float g_fWindowAspectRatio = max(1.0f, (float)g_WindowWidth / (float)g_WindowHeight);
	if (g_bUseSteamVR)
		g_fWindowAspectRatio = max(1.0f, (float)g_steamVRWidth / (float)g_steamVRHeight);
	// The point where fixed and non-fixed params are about the same is given by the window aspect ratio
	bool bFixFactors = g_fCurInGameAspectRatio > g_fWindowAspectRatio;
	// The compensation factor is given by the ratio between the window aspect ratio and the in-game's 
	// aspect ratio. In other words, the size of the display window will stretch the view, so we
	// have to compensate for that.
	float comp_factor = bFixFactors ? g_fWindowAspectRatio / g_fCurInGameAspectRatio : 1.0f;

	log_debug("[DBG] [FOV] y_center raw: %0.3f, FOVscale raw: %0.3f, W/H: %0.0f, %0.0f, a/r: %0.3f, FIX: %d, comp_factor: %0.3f",
		y_center_raw, FOVscale_raw, g_fCurInGameWidth, g_fCurInGameHeight, g_fCurInGameAspectRatio, bFixFactors, comp_factor);

	// Compute the compensated FOV and y_center:
	g_fFOVscale = comp_factor * FOVscale_raw;
	g_fYCenter = comp_factor * y_center_raw;

	// Store the global FOVscale, y_center in the CB:
	g_ShadertoyBuffer.FOVscale = g_fFOVscale;
	g_ShadertoyBuffer.y_center = g_fYCenter;

	// If PreserveAspectRatio is 0, then we need to apply further compensation because the image
	// will further stretch in either the X or Y axis.
	g_ShadertoyBuffer.preserveAspectRatioComp[0] = 1.0f;
	g_ShadertoyBuffer.preserveAspectRatioComp[1] = 1.0f;
	// The VR modes behave just like PreserveAspectRatio = 0. The funny thing is that
	// it will stretch the image in the same amount as when running in non-VR mode.
	// This means that the aspect ratio of the in-game resolution should be an integer
	// multiple of the Display/SteamVR window to avoid distortion.
	if (!g_config.AspectRatioPreserved || g_bEnableVR)
	{
		float RealWindowAspectRatio = (float)g_WindowWidth / (float)g_WindowHeight;
		// In SteamVR mode, the must compensate against the SteamVR window size
		if (g_bUseSteamVR)
			RealWindowAspectRatio = (float)g_steamVRWidth / (float)g_steamVRHeight;

		if (RealWindowAspectRatio > g_fCurInGameAspectRatio) {
			// The display window is going to stretch the image horizontally, so we need
			// to shrink the x axis:
			if (RealWindowAspectRatio > 1.0f) // Make sure we shrink. If we divide by a value lower than 1, we'll stretch!
				g_ShadertoyBuffer.preserveAspectRatioComp[0] = g_fCurInGameAspectRatio / RealWindowAspectRatio;
			else
				g_ShadertoyBuffer.preserveAspectRatioComp[0] = RealWindowAspectRatio / g_fCurInGameAspectRatio;
		}
		else {
			// The display window is going to stretch the image vertically, so we need
			// to shrink the y axis:
			if (g_fCurInGameAspectRatio > 1.0f)
				g_ShadertoyBuffer.preserveAspectRatioComp[1] = RealWindowAspectRatio / g_fCurInGameAspectRatio;
			else
				g_ShadertoyBuffer.preserveAspectRatioComp[1] = g_fCurInGameAspectRatio / RealWindowAspectRatio;
		}

		log_debug("[DBG] [FOV] Real Window a/r: %0.3f, preserveA/R-Comp: %0.3f, %0.3f",
			RealWindowAspectRatio, g_ShadertoyBuffer.preserveAspectRatioComp[0], g_ShadertoyBuffer.preserveAspectRatioComp[1]);
	}

	// Compute the *real* vertical and horizontal FOVs:
	g_fRealVertFOV = ComputeRealVertFOV();
	g_fRealHorzFOV = ComputeRealHorzFOV();
	// Compute the metric scale factor conversion
	g_fOBJCurMetricScale = g_fCurInGameHeight * g_fOBJGlobalMetricMult / (SHADOW_OBJ_SCALE * 3200.0f);

	// Populate the metric reconstruction CB. Noticeably, the Metric Rec values need the raw
	// FOVscale and y_center, not the fixed ones. Making a 3D OBJ crosshairs as additional geometry
	// also places the crosshairs at the right spot (the crosshairs need to be at (0,1,100))
	g_MetricRecCBuffer.mr_FOVscale = FOVscale_raw;
	g_MetricRecCBuffer.mr_y_center = y_center_raw;
	g_MetricRecCBuffer.mr_cur_metric_scale = g_fOBJCurMetricScale;
	g_MetricRecCBuffer.mr_aspect_ratio = g_fCurInGameAspectRatio;
	g_MetricRecCBuffer.mr_z_metric_mult = g_fOBJ_Z_MetricMult;
	g_MetricRecCBuffer.mr_shadow_OBJ_scale = SHADOW_OBJ_SCALE;
	g_MetricRecCBuffer.mr_screen_aspect_ratio = g_fCurScreenWidth / g_fCurScreenHeight;
	//g_MetricRecCBuffer.mr_vr_aspect_ratio_comp[0] = 1.0f;
	//g_MetricRecCBuffer.mr_vr_aspect_ratio_comp[1] = 1.0f;
	g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[0] = 1.0f;
	g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[1] = 1.0f;
	g_MetricRecCBuffer.mr_vr_aspect_ratio = 1.0f;
	if (g_bEnableVR) {
		if (g_bUseSteamVR) {
			g_MetricRecCBuffer.mr_vr_aspect_ratio = (float)g_steamVRHeight / (float)g_steamVRWidth;
		}
		else {
			// This is the DirectSBS mode. I don't have a reliable way to get the resolution of the 
			// HMD device (which could be any cell phone + Google Cardboard for all we know). Instead,
			// we need to trust that the user will set the current desktop resolution to the HMD's 
			// resolution. So let's use the desktop's resolution to compensate for aspect ratio in
			// SBS mode:
			g_MetricRecCBuffer.mr_vr_aspect_ratio = (float)g_WindowHeight / (float)g_WindowWidth;
		}
	}

	if (g_bEnableVR) {
		if (g_bUseSteamVR) {
			if (g_config.AspectRatioPreserved) {
				g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[0] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[0];
				g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[1] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[1];
			}
		}
		else {
			// DirectSBS path
			g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[0] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[0];
			g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[1] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[1];
		}
	}

	// We just modified the Metric Reconstruction parameters, let's reapply them
	g_bMetricParamsNeedReapply = true;

	log_debug("[DBG] [FOV] Final y_center: %0.3f, FOV_Scale: %0.6f, RealVFOV: %0.2f, RealHFOV: %0.2f, mr_aspect_ratio: %0.3f, Frame: %d",
		g_ShadertoyBuffer.y_center, g_ShadertoyBuffer.FOVscale, g_fRealVertFOV, g_fRealHorzFOV,
		g_MetricRecCBuffer.mr_aspect_ratio, g_iPresentCounter);

	// DEBUG
	//static bool bFirstTime = true;
	//if (bFirstTime) {
	//	g_fDebugFOVscale = g_ShadertoyBuffer.FOVscale;
	//	//g_fDebugYCenter = g_ShadertoyBuffer.y_center;
	//	bFirstTime = false;
	//}
	//g_fFOVscale = g_fDebugFOVscale;
	//g_ShadertoyBuffer.FOVscale = g_fDebugFOVscale;
	//
	//g_fYCenter = g_fDebugYCenter;
	//g_ShadertoyBuffer.y_center = g_fDebugYCenter;
	////g_MetricRecCBuffer.mr_y_center = g_fDebugYCenter;
	////log_debug("[DBG] [FOV] g_fDebugYCenter: %0.3f", g_fDebugYCenter);
	//log_debug("[DBG] [FOV] g_fDebugYCenter: %0.3f, g_fDebugFOVscale: %0.6f", g_fDebugYCenter, g_fDebugFOVscale);
	// DEBUG
}

/**
 * Compute FOVscale and y_center for the hyperspace effect (and others that may need the FOVscale)
 * This function superseeds ComputeHyperFOVParamsOld. It's using a variable that m0rgg found that gives us
 * the vertical offset for the reticle. We need to test this more thoroughly before we remove the old version.
 * The BIG advantage of this method is that we no longer need to capture the reticle to calibrate the camera
 * since we can read the vertical offset directly from XWA's heap.
 */
void ComputeHyperFOVParams() {
	// m0rgg found this variable. We can use it to calibrate the camera without having to capture the reticle.
	DWORD *ReticleYCenter = (DWORD *)0x68C55C;
	float ReticleCentroidY = (float)*ReticleYCenter;
	float y_center_raw, FOVscale_raw;

	// H is the y-center of the screen, in in-game coordinates.
	float H = g_fCurInGameHeight / 2.0f;
	log_debug("[DBG] [FOV] ReticleYCenter: %u, Screen Y-Center: %0.3f", *ReticleYCenter, H);
	if (ReticleCentroidY > -1.0f) {
		// *sigh* for whatever stupid reason, sometimes we can have ReticleCentroid.y == 0.0 while looking straight ahead (pitch == 0)
		// This situation completely destroys the calculation below, so we need to make sure that the camera pitch and the centroid
		// are consistent.
		// Another way to solve this problem is to prevent this calculation until the reticle centroid is close-ish to the center
		// of the screen
		float y_dist_from_screen_center = (float)fabs(ReticleCentroidY - g_fCurInGameHeight / 2.0f);
		if (y_dist_from_screen_center < g_fCurInGameHeight / 2.0f) {
			// The reticle center visible this frame and it's not close to the edge of the screen.
			// We can use it to compute y_center...

			// The formula to compute y_center seems to be:
			// (in-game-center - HUD_center) / in-game-height * 2.0f * comp_factor.
			// The in-game-center has to be computer properly if the cockpit isn't facing forward
			y_center_raw = 2.0f * (H - ReticleCentroidY) / g_fCurInGameHeight;
			log_debug("[DBG] [FOV] HUD_center to y_center: %0.3f", y_center_raw);
			// We can stop looking for the reticle center now:
			g_bYCenterHasBeenFixed = true;
		}
		else
			log_debug("[DBG] [FOV] RETICLE COULD NOT BE USED COMPUTE Y_CENTER. WILL RETRY. Frame: %d", g_iPresentCounter);
		// If the reticle center can't be used to compute y_center, then g_bYCenterHasBeenFixed will stay false, and we'll
		// come back to this path on the next frame where a reticle is visible.
	}

	if (!g_bYCenterHasBeenFixed) {
		// Provide a default value if we couldn't compute y_center:
		y_center_raw = 153.0f / g_fCurInGameHeight;
		//y_center_raw = 0.0f; // I can't do this because the cockpits look wrong in the hangar. They look skewed
	}
	FOVscale_raw = 2.0f * *g_fRawFOVDist / g_fCurInGameHeight;

	// Compute the aspect-ratio fix factors
	float g_fWindowAspectRatio = max(1.0f, (float)g_WindowWidth / (float)g_WindowHeight);
	if (g_bUseSteamVR)
		g_fWindowAspectRatio = max(1.0f, (float)g_steamVRWidth / (float)g_steamVRHeight);
	// The point where fixed and non-fixed params are about the same is given by the window aspect ratio
	bool bFixFactors = g_fCurInGameAspectRatio > g_fWindowAspectRatio;
	// The compensation factor is given by the ratio between the window aspect ratio and the in-game's 
	// aspect ratio. In other words, the size of the display window will stretch the view, so we
	// have to compensate for that.
	float comp_factor = bFixFactors ? g_fWindowAspectRatio / g_fCurInGameAspectRatio : 1.0f;

	log_debug("[DBG] [FOV] y_center raw: %0.3f, FOVscale raw: %0.3f, W/H: %0.0f, %0.0f, a/r: %0.3f, FIX: %d, comp_factor: %0.3f",
		y_center_raw, FOVscale_raw, g_fCurInGameWidth, g_fCurInGameHeight, g_fCurInGameAspectRatio, bFixFactors, comp_factor);

	// Compute the compensated FOV and y_center:
	g_fFOVscale = comp_factor * FOVscale_raw;
	g_fYCenter = comp_factor * y_center_raw;

	// Store the global FOVscale, y_center in the CB:
	g_ShadertoyBuffer.FOVscale = g_fFOVscale;
	g_ShadertoyBuffer.y_center = g_fYCenter;

	// If PreserveAspectRatio is 0, then we need to apply further compensation because the image
	// will further stretch in either the X or Y axis.
	g_ShadertoyBuffer.preserveAspectRatioComp[0] = 1.0f;
	g_ShadertoyBuffer.preserveAspectRatioComp[1] = 1.0f;
	// The VR modes behave just like PreserveAspectRatio = 0. The funny thing is that
	// it will stretch the image in the same amount as when running in non-VR mode.
	// This means that the aspect ratio of the in-game resolution should be an integer
	// multiple of the Display/SteamVR window to avoid distortion.
	if (!g_config.AspectRatioPreserved || g_bEnableVR)
	{
		float RealWindowAspectRatio = (float)g_WindowWidth / (float)g_WindowHeight;
		// In SteamVR mode, the must compensate against the SteamVR window size
		if (g_bUseSteamVR)
			RealWindowAspectRatio = (float)g_steamVRWidth / (float)g_steamVRHeight;

		if (RealWindowAspectRatio > g_fCurInGameAspectRatio) {
			// The display window is going to stretch the image horizontally, so we need
			// to shrink the x axis:
			if (RealWindowAspectRatio > 1.0f) // Make sure we shrink. If we divide by a value lower than 1, we'll stretch!
				g_ShadertoyBuffer.preserveAspectRatioComp[0] = g_fCurInGameAspectRatio / RealWindowAspectRatio;
			else
				g_ShadertoyBuffer.preserveAspectRatioComp[0] = RealWindowAspectRatio / g_fCurInGameAspectRatio;
		}
		else {
			// The display window is going to stretch the image vertically, so we need
			// to shrink the y axis:
			if (g_fCurInGameAspectRatio > 1.0f)
				g_ShadertoyBuffer.preserveAspectRatioComp[1] = RealWindowAspectRatio / g_fCurInGameAspectRatio;
			else
				g_ShadertoyBuffer.preserveAspectRatioComp[1] = g_fCurInGameAspectRatio / RealWindowAspectRatio;
		}

		log_debug("[DBG] [FOV] Real Window a/r: %0.3f, preserveA/R-Comp: %0.3f, %0.3f",
			RealWindowAspectRatio, g_ShadertoyBuffer.preserveAspectRatioComp[0], g_ShadertoyBuffer.preserveAspectRatioComp[1]);
	}

	// Compute the *real* vertical and horizontal FOVs:
	g_fRealVertFOV = ComputeRealVertFOV();
	g_fRealHorzFOV = ComputeRealHorzFOV();
	// Compute the metric scale factor conversion
	g_fOBJCurMetricScale = g_fCurInGameHeight * g_fOBJGlobalMetricMult / (SHADOW_OBJ_SCALE * 3200.0f);

	// Populate the metric reconstruction CB. Noticeably, the Metric Rec values need the raw
	// FOVscale and y_center, not the fixed ones. Making a 3D OBJ crosshairs as additional geometry
	// also places the crosshairs at the right spot (the crosshairs need to be at (0,1,100))
	g_MetricRecCBuffer.mr_FOVscale = FOVscale_raw;
	g_MetricRecCBuffer.mr_y_center = y_center_raw;
	g_MetricRecCBuffer.mr_cur_metric_scale = g_fOBJCurMetricScale;
	g_MetricRecCBuffer.mr_aspect_ratio = g_fCurInGameAspectRatio;
	g_MetricRecCBuffer.mr_z_metric_mult = g_fOBJ_Z_MetricMult;
	g_MetricRecCBuffer.mr_shadow_OBJ_scale = SHADOW_OBJ_SCALE;
	g_MetricRecCBuffer.mr_screen_aspect_ratio = g_fCurScreenWidth / g_fCurScreenHeight;
	//g_MetricRecCBuffer.mr_vr_aspect_ratio_comp[0] = 1.0f;
	//g_MetricRecCBuffer.mr_vr_aspect_ratio_comp[1] = 1.0f;
	g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[0] = 1.0f;
	g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[1] = 1.0f;
	g_MetricRecCBuffer.mr_vr_aspect_ratio = 1.0f;
	if (g_bEnableVR) {
		if (g_bUseSteamVR) {
			g_MetricRecCBuffer.mr_vr_aspect_ratio = (float)g_steamVRHeight / (float)g_steamVRWidth;
		}
		else {
			// This is the DirectSBS mode. I don't have a reliable way to get the resolution of the 
			// HMD device (which could be any cell phone + Google Cardboard for all we know). Instead,
			// we need to trust that the user will set the current desktop resolution to the HMD's 
			// resolution. So let's use the desktop's resolution to compensate for aspect ratio in
			// SBS mode:
			g_MetricRecCBuffer.mr_vr_aspect_ratio = (float)g_WindowHeight / (float)g_WindowWidth;
		}
	}

	if (g_bEnableVR) {
		if (g_bUseSteamVR) {
			if (g_config.AspectRatioPreserved) {
				g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[0] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[0];
				g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[1] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[1];
			}
		}
		else {
			// DirectSBS path
			g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[0] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[0];
			g_MetricRecCBuffer.mv_vr_vertexbuf_aspect_ratio_comp[1] = 1.0f / g_ShadertoyBuffer.preserveAspectRatioComp[1];
		}
	}

	// We just modified the Metric Reconstruction parameters, let's reapply them
	g_bMetricParamsNeedReapply = true;

	log_debug("[DBG] [FOV] Final y_center: %0.3f, FOV_Scale: %0.6f, RealVFOV: %0.2f, RealHFOV: %0.2f, mr_aspect_ratio: %0.3f, Frame: %d",
		g_ShadertoyBuffer.y_center, g_ShadertoyBuffer.FOVscale, g_fRealVertFOV, g_fRealHorzFOV,
		g_MetricRecCBuffer.mr_aspect_ratio, g_iPresentCounter);

	// DEBUG
	//static bool bFirstTime = true;
	//if (bFirstTime) {
	//	g_fDebugFOVscale = g_ShadertoyBuffer.FOVscale;
	//	//g_fDebugYCenter = g_ShadertoyBuffer.y_center;
	//	bFirstTime = false;
	//}
	//g_fFOVscale = g_fDebugFOVscale;
	//g_ShadertoyBuffer.FOVscale = g_fDebugFOVscale;
	//
	//g_fYCenter = g_fDebugYCenter;
	//g_ShadertoyBuffer.y_center = g_fDebugYCenter;
	////g_MetricRecCBuffer.mr_y_center = g_fDebugYCenter;
	////log_debug("[DBG] [FOV] g_fDebugYCenter: %0.3f", g_fDebugYCenter);
	//log_debug("[DBG] [FOV] g_fDebugYCenter: %0.3f, g_fDebugFOVscale: %0.6f", g_fDebugYCenter, g_fDebugFOVscale);
	// DEBUG
}

/* Loads the dynamic_cockpit.cfg file */
bool LoadDCParams() {
	log_debug("[DBG] Loading Dynamic Cockpit params...");
	FILE* file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;
	float cover_tex_width = 1, cover_tex_height = 1;

	try {
		error = fopen_s(&file, "./dynamic_cockpit.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load dynamic_cockpit.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading dynamic_cockpit.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Initialize the IFF colors
	// Rebel
	g_DCTargetingIFFColors[0].x = 0.1f;
	g_DCTargetingIFFColors[0].y = 0.7f;
	g_DCTargetingIFFColors[0].z = 0.1f;
	// Imperial
	g_DCTargetingIFFColors[1].x = 0.5f;
	g_DCTargetingIFFColors[1].y = 0.1f;
	g_DCTargetingIFFColors[1].z = 0.1f;
	// Neutral
	g_DCTargetingIFFColors[2].x = 0.1f;
	g_DCTargetingIFFColors[2].y = 0.1f;
	g_DCTargetingIFFColors[2].z = 0.5f;
	// Viraxo
	g_DCTargetingIFFColors[3].x = 0.3f;
	g_DCTargetingIFFColors[3].y = 0.2f;
	g_DCTargetingIFFColors[3].z = 0.1f;
	// Backdrop (?)
	g_DCTargetingIFFColors[4].x = 0.1f;
	g_DCTargetingIFFColors[4].y = 0.1f;
	g_DCTargetingIFFColors[4].z = 0.1f;
	// Azzameen
	g_DCTargetingIFFColors[5].x = 0.5f;
	g_DCTargetingIFFColors[5].y = 0.1f;
	g_DCTargetingIFFColors[5].z = 0.5f;
	// Other wireframe initialization
	g_DCWireframeLuminance.x = 0.33f;
	g_DCWireframeLuminance.y = 0.50f;
	g_DCWireframeLuminance.z = 0.16f;
	g_DCWireframeLuminance.w = 0.05f;

	g_DCTargetingFriend.x = 0.1f;
	g_DCTargetingFriend.y = 0.6f;
	g_DCTargetingFriend.z = 0.1f;
	g_DCTargetingFriend.w = 1.0f;

	g_DCTargetingFoe.x = 0.6f;
	g_DCTargetingFoe.y = 0.1f;
	g_DCTargetingFoe.z = 0.1f;
	g_DCTargetingFoe.w = 1.0f;

	g_DCWireframeContrast = 3.0f;

	// Reset the dynamic cockpit vector if we're not rendering in 3D
	//if (!g_bRendering3D && g_DCElements.size() > 0) {
	//	log_debug("[DBG] [DC] Clearing g_DCElements");
	//	ClearDynCockpitVector(g_DCElements);
	//}
	ClearDCMoveRegions();

	/* Reload individual cockit parameters if the current cockpit is set */
	bool bCockpitParamsLoaded = false;
	if (g_sCurrentCockpit[0] != 0) {
		char sFileName[80];
		CockpitNameToDCParamsFile(g_sCurrentCockpit, sFileName, 80);
		bCockpitParamsLoaded = LoadIndividualDCParams(sFileName);
	}

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, DYNAMIC_COCKPIT_ENABLED_VRPARAM) == 0) {
				g_bDynCockpitEnabled = (bool)fValue;
				log_debug("[DBG] [DC] g_bDynCockpitEnabled: %d", g_bDynCockpitEnabled);
				if (!g_bDynCockpitEnabled) {
					// Early abort: stop reading coordinates if the dynamic cockpit is disabled
					fclose(file);
					return false;
				}
			}
			else if (_stricmp(param, MOVE_REGION_DCPARAM) == 0) {
				// Individual cockpit move_region commands override the global move_region commands:
				// EDIT: I don't remember very well why I did this; but maybe it was when I thought the move_region
				// commands would go into each cockpit. So the per-cockpit move_region commands would override
				// any global move_regions. However, at this point, the move_region commands seem to be global
				// and apply to all cockpits, so let's reload these commands to make it easier to edit the HUD
				// with Ctrl+Alt+L
				//if (!bCockpitParamsLoaded)
				LoadDCMoveRegion(buf);
			}
			else if (_stricmp(param, CT_BRIGHTNESS_DCPARAM) == 0) {
				g_fCoverTextureBrightness = fValue;
			}
			/*else if (_stricmp(param, "ignore_erase_commands") == 0) {
				//g_bDCApplyEraseRegionCommands = (bool)fValue;
			}*/
			else if (_stricmp(param, "HUD_visible_on_startup") == 0) {
				g_bHUDVisibleOnStartup = (bool)fValue;
				//g_bDCApplyEraseRegionCommands = (bool)fValue;
			}
			else if (_stricmp(param, "compensate_FOV_for_1920x1080") == 0) {
				g_bCompensateFOVfor1920x1080 = (bool)fValue;
			}

			else if (_stricmp(param, "dc_brightness") == 0) {
				g_fDCBrightness = fValue;
			}

			else if (_stricmp(param, "enable_wireframe_CMD") == 0) {
				g_bEdgeDetectorEnabled = (bool)fValue;
			}
			else if (_stricmp(param, "wireframe_IFF_color_0") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[0].x = x;
					g_DCTargetingIFFColors[0].y = y;
					g_DCTargetingIFFColors[0].z = z;
					g_DCTargetingIFFColors[0].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_1") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[1].x = x;
					g_DCTargetingIFFColors[1].y = y;
					g_DCTargetingIFFColors[1].z = z;
					g_DCTargetingIFFColors[1].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_2") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[2].x = x;
					g_DCTargetingIFFColors[2].y = y;
					g_DCTargetingIFFColors[2].z = z;
					g_DCTargetingIFFColors[2].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_3") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[3].x = x;
					g_DCTargetingIFFColors[3].y = y;
					g_DCTargetingIFFColors[3].z = z;
					g_DCTargetingIFFColors[3].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_4") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[4].x = x;
					g_DCTargetingIFFColors[4].y = y;
					g_DCTargetingIFFColors[4].z = z;
					g_DCTargetingIFFColors[4].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_5") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[5].x = x;
					g_DCTargetingIFFColors[5].y = y;
					g_DCTargetingIFFColors[5].z = z;
					g_DCTargetingIFFColors[5].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_color_friend") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingFriend.x = x;
					g_DCTargetingFriend.y = y;
					g_DCTargetingFriend.z = z;
					g_DCTargetingFriend.w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_color_foe") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingFoe.x = x;
					g_DCTargetingFoe.y = y;
					g_DCTargetingFoe.z = z;
					g_DCTargetingFoe.w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_luminance_vector") == 0) {
				float x, y, z;
				log_debug("[DBG] [DC] Loading wireframe luminance vector...");
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCWireframeLuminance.x = x;
					g_DCWireframeLuminance.y = y;
					g_DCWireframeLuminance.z = z;
					g_DCWireframeLuminance.w = 0.0f;
					log_debug("[DBG] [DC] WireframeLuminance: %0.3f, %0.3f, %0.3f, %0.3f",
						g_DCWireframeLuminance.x, g_DCWireframeLuminance.y, g_DCWireframeLuminance.z, g_DCWireframeLuminance.w);
				}
			}
			else if (_stricmp(param, "wireframe_contrast") == 0) {
				g_DCWireframeContrast = fValue;
				log_debug("[DBG] [DC] Wireframe contrast: %0.3f", g_DCWireframeContrast);
			}
			else if (_stricmp(param, "diegetic_cockpit_enabled") == 0) {
				g_bDiegeticCockpitEnabled = (bool)fValue;
			}
		}
	}
	fclose(file);
	return true;
}

/* Loads the dynamic_cockpit.cfg file */
bool LoadACParams() {
	log_debug("[DBG] [AC] Loading Active Cockpit params...");
	FILE* file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./active_cockpit.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] [AC] Could not load active_cockpit.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] [AC] Error %d when loading active_cockpit.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	/* Reload individual cockpit parameters if the current cockpit is set */
	// TODO: Reset AC params!
	bool bActiveCockpitParamsLoaded = false;
	if (g_sCurrentCockpit[0] != 0) {
		char sFileName[80];
		CockpitNameToACParamsFile(g_sCurrentCockpit, sFileName, 80);
		bActiveCockpitParamsLoaded = LoadIndividualACParams(sFileName);
	}

	g_LaserPointerBuffer.bDebugMode = 0;
	g_LaserPointerBuffer.cursor_radius = 0.01f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "active_cockpit_enabled") == 0) {
				g_bActiveCockpitEnabled = (bool)fValue;
				log_debug("[DBG] [AC] g_bActiveCockpitEnabled: %d", g_bActiveCockpitEnabled);
				if (!g_bActiveCockpitEnabled) {
					// Early abort: stop reading coordinates if the active cockpit is disabled
					fclose(file);
					return false;
				}
			}
			else if (_stricmp(param, "freepie_controller_slot") == 0) {
				g_iFreePIEControllerSlot = (int)fValue;
				InitFreePIE();
			}
			else if (_stricmp(param, "button_data_available") == 0) {
				g_bFreePIEControllerButtonDataAvailable = (bool)fValue;
			}
			else if (_stricmp(param, "controller_multiplier_x") == 0) {
				g_fContMultiplierX = fValue;
			}
			else if (_stricmp(param, "controller_multiplier_y") == 0) {
				g_fContMultiplierY = fValue;
			}
			else if (_stricmp(param, "controller_multiplier_z") == 0) {
				g_fContMultiplierZ = fValue;
			}
			else if (_stricmp(param, "origin_from_HMD_position") == 0) {
				g_bOriginFromHMD = (bool)fValue;
			}
			else if (_stricmp(param, "cursor_origin_init_x") == 0) {
				g_contOriginWorldSpace.x = fValue;
			}
			else if (_stricmp(param, "cursor_origin_init_y") == 0) {
				g_contOriginWorldSpace.y = fValue;
			}
			else if (_stricmp(param, "cursor_origin_init_z") == 0) {
				g_contOriginWorldSpace.z = fValue;
			}
			else if (_stricmp(param, "compensate_HMD_rotation") == 0) {
				g_bCompensateHMDRotation = (bool)fValue;
			}
			else if (_stricmp(param, "compensate_HMD_position") == 0) {
				g_bCompensateHMDPosition = (bool)fValue;
			}
			else if (_stricmp(param, "compensate_HMD_motion") == 0) {
				g_bCompensateHMDRotation = (bool)fValue;
				g_bCompensateHMDPosition = (bool)fValue;
			}
			else if (_stricmp(param, "full_cockpit_test") == 0) {
				g_bFullCockpitTest = (bool)fValue;
			}
			else if (_stricmp(param, "laser_pointer_length") == 0) {
				g_fLaserPointerLength = fValue;
			}
			else if (_stricmp(param, "debug_laser_dir") == 0) {
				g_iLaserDirSelector = (int)fValue;
			}
			else if (_stricmp(param, "debug") == 0) {
				g_LaserPointerBuffer.bDebugMode = (bool)fValue;
			}
			else if (_stricmp(param, "cursor_radius") == 0) {
				g_LaserPointerBuffer.cursor_radius = fValue;
			}
		}
	}
	fclose(file);
	return true;
}

/* Loads the Bloom parameters from bloom.cfg */
bool LoadBloomParams() {
	log_debug("[DBG] Loading Bloom params...");
	FILE* file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./bloom.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load bloom.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading bloom.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;
	// Set some default values
	g_BloomConfig.uvStepSize1 = 3.0f;
	g_BloomConfig.uvStepSize2 = 2.0f;
	g_BloomConfig.fLasersStrength = 4.0f;
	g_BloomConfig.fEngineGlowStrength = 0.5f;
	g_BloomConfig.fSparksStrength = 0.5f;
	g_BloomConfig.fSkydomeLightStrength = 0.1f;
	g_BloomConfig.fBracketStrength = 0.0f;
	g_BloomPSCBuffer.general_bloom_strength = 1.0f;
	// TODO: Complete the list of default values...
	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			// ReShade state
			if (_stricmp(param, "bloom_enabled") == 0) {
				bool state = (bool)fValue;
				g_bReshadeEnabled |= state;
				g_bBloomEnabled = state;
			}

			// Bloom
			/*
			else if (_stricmp(param, "general_bloom_strength") == 0) {
				g_BloomPSCBuffer.general_bloom_strength = fValue;
				log_debug("[DBG] [Bloom] general bloom strength: %0.3f",
					g_BloomPSCBuffer.general_bloom_strength);
			}
			*/
			else if (_stricmp(param, "saturation_strength") == 0) {
				g_BloomConfig.fSaturationStrength = fValue;
			}
			else if (_stricmp(param, "bloom_levels") == 0) {
				g_BloomConfig.iNumPasses = (int)fValue;
				if (g_BloomConfig.iNumPasses > MAX_BLOOM_PASSES)
					g_BloomConfig.iNumPasses = MAX_BLOOM_PASSES;
				log_debug("[DBG] [Bloom] iNumPasses: %d", g_BloomConfig.iNumPasses);
			}
			else if (_stricmp(param, "uv_step_size_1") == 0) {
				g_BloomConfig.uvStepSize1 = fValue;
			}
			else if (_stricmp(param, "uv_step_size_2") == 0) {
				g_BloomConfig.uvStepSize2 = fValue;
			}
			else if (_stricmp(param, "background_suns_strength") == 0) {
				g_BloomConfig.fSunsStrength = fValue;
			}
			else if (_stricmp(param, "lens_flare_strength") == 0) {
				g_BloomConfig.fLensFlareStrength = fValue;
			}
			else if (_stricmp(param, "cockpit_lights_strength") == 0) {
				g_BloomConfig.fCockpitStrength = fValue;
			}
			else if (_stricmp(param, "light_map_strength") == 0) {
				g_BloomConfig.fLightMapsStrength = fValue;
			}
			else if (_stricmp(param, "lasers_strength") == 0) {
				g_BloomConfig.fLasersStrength = fValue;
			}
			else if (_stricmp(param, "turbolasers_strength") == 0) {
				g_BloomConfig.fTurboLasersStrength = fValue;
			}
			else if (_stricmp(param, "engine_glow_strength") == 0) {
				g_BloomConfig.fEngineGlowStrength = fValue;
			}
			else if (_stricmp(param, "explosions_strength") == 0) {
				g_BloomConfig.fExplosionsStrength = fValue;
			}
			else if (_stricmp(param, "sparks_strength") == 0) {
				g_BloomConfig.fSparksStrength = fValue;
			}
			else if (_stricmp(param, "cockpit_sparks_strength") == 0) {
				g_BloomConfig.fCockpitSparksStrength = fValue;
			}
			else if (_stricmp(param, "missile_strength") == 0) {
				g_BloomConfig.fMissileStrength = fValue;
			}
			else if (_stricmp(param, "hyper_streak_strength") == 0) {
				g_BloomConfig.fHyperStreakStrength = fValue;
			}
			else if (_stricmp(param, "hyper_tunnel_strength") == 0) {
				g_BloomConfig.fHyperTunnelStrength = fValue;
			}
			else if (_stricmp(param, "skydome_light_strength") == 0) {
				g_BloomConfig.fSkydomeLightStrength = fValue;
			}
			else if (_stricmp(param, "bracket_strength") == 0) {
				g_BloomConfig.fBracketStrength = fValue;
			}

			// Bloom strength per pyramid level
			else if (_stricmp(param, "bloom_layer_mult_0") == 0) {
				g_fBloomLayerMult[0] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_1") == 0) {
				g_fBloomLayerMult[1] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_2") == 0) {
				g_fBloomLayerMult[2] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_3") == 0) {
				g_fBloomLayerMult[3] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_4") == 0) {
				g_fBloomLayerMult[4] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_5") == 0) {
				g_fBloomLayerMult[5] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_6") == 0) {
				g_fBloomLayerMult[6] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_7") == 0) {
				g_fBloomLayerMult[7] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_8") == 0) {
				g_fBloomLayerMult[8] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_9") == 0) {
				g_fBloomLayerMult[9] = fValue;
			}

			// Bloom Spread
			else if (_stricmp(param, "bloom_spread_1") == 0) {
				g_fBloomSpread[1] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_2") == 0) {
				g_fBloomSpread[2] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_3") == 0) {
				g_fBloomSpread[3] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_4") == 0) {
				g_fBloomSpread[4] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_5") == 0) {
				g_fBloomSpread[5] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_6") == 0) {
				g_fBloomSpread[6] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_7") == 0) {
				g_fBloomSpread[7] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_8") == 0) {
				g_fBloomSpread[8] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_9") == 0) {
				g_fBloomSpread[9] = fValue;
			}

			// Bloom Passes
			else if (_stricmp(param, "bloom_passes_1") == 0) {
				g_iBloomPasses[1] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_2") == 0) {
				g_iBloomPasses[2] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_3") == 0) {
				g_iBloomPasses[3] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_4") == 0) {
				g_iBloomPasses[4] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_5") == 0) {
				g_iBloomPasses[5] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_6") == 0) {
				g_iBloomPasses[6] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_7") == 0) {
				g_iBloomPasses[7] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_8") == 0) {
				g_iBloomPasses[8] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_9") == 0) {
				g_iBloomPasses[9] = (int)fValue;
			}
		}
	}
	fclose(file);

	log_debug("[DBG] Reshade Enabled: %d", g_bReshadeEnabled);
	log_debug("[DBG] Bloom Enabled: %d", g_bBloomEnabled);
	return true;
}

bool LoadSSAOParams() {
	log_debug("[DBG] Loading SSAO params...");
	FILE* file;
	int error = 0, line = 0;

	// Provide some default values in case they are missing in the config file
	g_SSAO_PSCBuffer.bias = 0.05f;
	g_SSAO_PSCBuffer.intensity = 4.0f;
	g_SSAO_PSCBuffer.indirect_intensity = 1.5f;
	g_SSAO_PSCBuffer.power = 1.0f;
	g_SSAO_PSCBuffer.black_level = 0.2f;
	g_SSAO_PSCBuffer.bentNormalInit = 0.1f; // 0.2f // TODO: Check if we need to update this now that the reconstruction uses DEFAULT_FOCAL_DIST
	g_SSAO_PSCBuffer.near_sample_radius = 0.005f;
	g_SSAO_PSCBuffer.far_sample_radius = 0.005f;
	g_SSAO_PSCBuffer.z_division = 0;
	g_SSAO_PSCBuffer.samples = 8;
	g_fMoireOffsetDir = 0.02f;
	g_fMoireOffsetInd = 0.1f;
	g_SSAO_PSCBuffer.moire_offset = g_fMoireOffsetDir;
	g_SSAO_PSCBuffer.fn_sharpness = 1.0f;
	g_SSAO_PSCBuffer.fn_max_xymult = 0.4f;
	g_SSAO_PSCBuffer.shadow_epsilon = 0.0f;
	g_SSAO_PSCBuffer.Bz_mult = 0.05f;
	g_SSAO_PSCBuffer.debug = 0;
	g_SSAO_PSCBuffer.moire_scale = 0.5f; // Previous: 0.1f
	g_fSSAOAlphaOfs = 0.5;
	g_SSAO_Type = SSO_PBR;
	// Default position of the global light (the sun)
	g_LightVector[0].x = 0;
	g_LightVector[0].y = 1;
	g_LightVector[0].z = 0;
	g_LightVector[0].w = 0;
	g_LightVector[0].normalize();

	g_LightVector[1].x = 1;
	g_LightVector[1].y = 1;
	g_LightVector[1].z = 0;
	g_LightVector[1].w = 0;
	g_LightVector[0].normalize();

	// Default view-yaw/pitch signs for SSAO.cfg-based lights
	//g_fViewYawSign = 1.0f;
	//g_fViewPitchSign = -1.0f;

	// Default values for the shading system CB
	g_ShadingSys_PSBuffer.spec_intensity = 1.0f;
	g_ShadingSys_PSBuffer.spec_bloom_intensity = 0.9f;
	g_ShadingSys_PSBuffer.glossiness = 128.0f;
	g_ShadingSys_PSBuffer.bloom_glossiness_mult = 3.0f;
	g_ShadingSys_PSBuffer.saturation_boost = 0.75f;
	g_ShadingSys_PSBuffer.lightness_boost = 2.0f;
	g_ShadingSys_PSBuffer.sqr_attenuation = 0.001f; // Smaller numbers fade less
	g_ShadingSys_PSBuffer.laser_light_intensity = 6.0f;
	g_bHDREnabled = false;
	g_fHDRWhitePoint = 1.0f;
	g_fHDRLightsMultiplier = 2.8f;
	g_ShadingSys_PSBuffer.HDREnabled = g_bHDREnabled;
	g_ShadingSys_PSBuffer.HDR_white_point = g_fHDRWhitePoint;
	g_fGlobalAmbient = 0.005f;
	g_fHangarAmbient = 0.05f;

	g_ShadertoyBuffer.flare_intensity = 2.0f;

	g_ShadowMapping.bEnabled = false;
	g_ShadowMapping.bCSMEnabled = false;
	g_bShadowMapDebug = false;
	g_ShadowMapVSCBuffer.sm_bias = 0.01f;
	g_ShadowMapVSCBuffer.sm_debug = g_bShadowMapDebug;
	g_ShadowMapVSCBuffer.sm_pcss_radius = 1.0f / SHADOW_MAP_SIZE;
	g_ShadowMapVSCBuffer.sm_light_size = 0.1f;

	g_fRTGaussFactor = 0.1f;
	g_bRTEnableSoftShadows = true;
	g_RTConstantsBuffer.RTEnableSoftShadows = true;
	g_RTConstantsBuffer.RTShadowMaskSizeFactor = 4;

	for (int i = 0; i < MAX_XWA_LIGHTS; i++)
		if (!g_XWALightInfo[i].bTagged)
			g_ShadowMapVSCBuffer.sm_black_levels[i] = 0.2f;

	try {
		error = fopen_s(&file, "./ssao.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load ssao.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading ssao.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "ssao_enabled") == 0) {
				bool state = (bool)fValue;
				g_bReshadeEnabled |= state;
				g_bAOEnabled = state;
			}

			else if (_stricmp(param, "ssao_type") == 0) {
				if (_stricmp(svalue, "ambient") == 0)
					g_SSAO_Type = SSO_AMBIENT;
				else if (_stricmp(svalue, "directional") == 0)
					g_SSAO_Type = SSO_DIRECTIONAL;
				else if (_stricmp(svalue, "bent_normals") == 0)
					g_SSAO_Type = SSO_BENT_NORMALS;
				else if (_stricmp(svalue, "deferred") == 0)
				{
					if (!g_bRTEnabled)
					{
						g_SSAO_Type = SSO_DEFERRED;
					}
					else
					{
						// Force the PBR Rendering mode and enable HDR if RT is enabled.
						if (g_bRTEnabled) g_SSAO_Type = SSO_PBR;
						SetHDRState(g_bRTEnabled);
					}
				}
				else if (_stricmp(svalue, "PBR") == 0)
				{
					g_SSAO_Type = SSO_PBR;
					SetHDRState(true);
				}
			}

			if (_stricmp(param, "raytracing_enabled_in_tech_room") == 0) {
				g_bRTEnabledInTechRoom = (bool)fValue;
			}
			if (_stricmp(param, "raytracing_enabled") == 0) {
				g_bRTEnabled = (bool)fValue;
				// Force the PBR Rendering mode and enable HDR if RT is enabled.
				if (g_bRTEnabled) g_SSAO_Type = SSO_PBR;
				SetHDRState(g_bRTEnabled);
			}
			if (_stricmp(param, "raytracing_enabled_in_cockpit") == 0) {
				g_bRTEnabledInCockpit = (bool)fValue;
			}
			if (_stricmp(param, "raytracing_enable_soft_shadows") == 0) {
				g_bRTEnableSoftShadows = (bool)fValue;
				g_RTConstantsBuffer.RTEnableSoftShadows = g_bRTEnableSoftShadows;
			}
			if (_stricmp(param, "raytracing_shadow_mask_size_factor") == 0) {
				g_RTConstantsBuffer.RTShadowMaskSizeFactor = (int)fValue;
			}
			if (_stricmp(param, "raytracing_soft_shadow_sharpness") == 0) {
				// 0.01 --> Soft
				// 1.5 --> Sharp
				g_fRTGaussFactor = lerp(0.01f, 1.5f, clamp(fValue, 0.0f, 1.0f));
			}
			if (_stricmp(param, "raytracing_enable_embree") == 0) {
				//g_bRTEnableEmbree = (bool)fValue;
				g_bRTEnableEmbree = false;
				//g_BVHBuilderType = g_bRTEnableEmbree ? BVHBuilderType_Embree : DEFAULT_BVH_BUILDER;
				g_BLASBuilderType = DEFAULT_BLAS_BUILDER;
			}

			if (_stricmp(param, "keep_mouse_inside_window") == 0) {
				g_bKeepMouseInsideWindow = (bool)fValue;
			}

			if (_stricmp(param, "bias") == 0) {
				g_SSAO_PSCBuffer.bias = fValue;
			}
			else if (_stricmp(param, "intensity") == 0) {
				g_SSAO_PSCBuffer.intensity = fValue;
			}
			else if (_stricmp(param, "near_sample_radius") == 0) { // "sample_radius" was previously read here too
				g_SSAO_PSCBuffer.near_sample_radius = fValue;
			}
			else if (_stricmp(param, "far_sample_radius") == 0) {
				g_SSAO_PSCBuffer.far_sample_radius = fValue;
			}
			else if (_stricmp(param, "samples") == 0) {
				g_SSAO_PSCBuffer.samples = (int)fValue;
			}
			else if (_stricmp(param, "ssao_buffer_scale_divisor") == 0) {
				g_fSSAOZoomFactor = (float)fValue;
			}
			/*else if (_stricmp(param, "ssao2_buffer_scale_divisor") == 0) {
				g_fSSAOZoomFactor2 = (float)fValue;
			}*/
			else if (_stricmp(param, "enable_blur") == 0) {
				g_bBlurSSAO = (bool)fValue;
			}
			else if (_stricmp(param, "black_level") == 0) {
				g_SSAO_PSCBuffer.black_level = fValue;
			}
			else if (_stricmp(param, "perspective_correct") == 0) {
				g_SSAO_PSCBuffer.z_division = (bool)fValue;
			}
			else if (_stricmp(param, "bent_normal_init") == 0) {
				g_SSAO_PSCBuffer.bentNormalInit = fValue; // Default: 0.2f
			}
			else if (_stricmp(param, "max_dist") == 0) {
				g_SSAO_PSCBuffer.max_dist = fValue;
			}
			else if (_stricmp(param, "power") == 0) {
				g_SSAO_PSCBuffer.power = fValue;
			}
			/*else if (_stricmp(param, "enable_dual_ssao") == 0) {
				g_bDisableDualSSAO = !(bool)fValue;
			}
			else if (_stricmp(param, "enable_ssao_in_shader") == 0) {
				g_bEnableSSAOInShader = (bool)fValue;
			}
			else if (_stricmp(param, "enable_bent_normals_in_shader") == 0) {
				g_bEnableBentNormalsInShader = (bool)fValue;
			}*/
			else if (_stricmp(param, "debug") == 0) {
				g_SSAO_PSCBuffer.debug = (int)fValue;
				g_BloomPSCBuffer.debug = (int)fValue;
			}
			else if (_stricmp(param, "indirect_intensity") == 0) {
				g_SSAO_PSCBuffer.indirect_intensity = fValue;
			}
			/*else if (_stricmp(param, "normal_blur_radius") == 0) {
				g_fNormalBlurRadius = fValue;
			}*/
			else if (_stricmp(param, "debug_ssao") == 0) {
				g_iSSDODebug = (int)fValue;
			}
			else if (_stricmp(param, "blur_passes") == 0) {
				g_iSSAOBlurPasses = (int)fValue;
			}
			else if (_stricmp(param, "enable_indirect_ssdo") == 0) {
				g_bEnableIndirectSSDO = (bool)fValue;
			}
			else if (_stricmp(param, "moire_offset") == 0) {
				g_fMoireOffsetDir = fValue;
			}
			else if (_stricmp(param, "moire_offset_ind") == 0) {
				g_fMoireOffsetInd = fValue;
			}
			else if (_stricmp(param, "moire_scale") == 0) {
				g_SSAO_PSCBuffer.moire_scale = fValue;
			}
			/* else if (_stricmp(param, "add_ssdo_to_indirect_pass") == 0) {
				g_SSAO_PSCBuffer.addSSDO = (int)fValue;
			} */
			else if (_stricmp(param, "normal_mapping_enable") == 0) {
				g_SSAO_PSCBuffer.fn_enable = (int)fValue;
				g_bFNEnable = g_SSAO_PSCBuffer.fn_enable;
			}
			else if (_stricmp(param, "nm_max_xymult") == 0) {
				g_SSAO_PSCBuffer.fn_max_xymult = fValue;
			}
			else if (_stricmp(param, "nm_sharpness") == 0) {
				g_SSAO_PSCBuffer.fn_sharpness = fValue;
			}
			else if (_stricmp(param, "override_game_light_pos") == 0) {
				g_bOverrideLightPos = (bool)fValue;
			}
			else if (_stricmp(param, "alpha_to_solid_offset") == 0) {
				g_fSSAOAlphaOfs = fValue;
			}
			else if (_stricmp(param, "ssdo_ambient") == 0 ||
				_stricmp(param, "ambient") == 0) {
				g_ShadingSys_PSBuffer.ambient = fValue;
				g_fGlobalAmbient = fValue;
			}
			else if (_stricmp(param, "hangar_ambient") == 0) {
				g_fHangarAmbient = fValue;
			}
			else if (_stricmp(param, "xwa_lights_saturation") == 0) {
				g_fXWALightsSaturation = fValue;
			}
			else if (_stricmp(param, "xwa_lights_apply_original_intensity") == 0) {
				g_bApplyXWALightsIntensity = (bool)fValue;
			}
			else if (_stricmp(param, "xwa_lights_global_intensity") == 0) {
				g_fXWALightsIntensity = fValue;
			}
			else if (_stricmp(param, "xwa_normalize_lights") == 0) {
				g_bNormalizeLights = (bool)fValue;
			}
			else if (_stricmp(param, "procedural_suns") == 0) {
				g_bProceduralSuns = (bool)fValue;
			}
			else if (_stricmp(param, "flare_intensity") == 0) {
				g_ShadertoyBuffer.flare_intensity = fValue;
			}
			else if (_stricmp(param, "enable_speed_shader") == 0) {
				g_bEnableSpeedShader = (bool)fValue;
			}
			else if (_stricmp(param, "speed_shader_scale_factor") == 0) {
				g_fSpeedShaderScaleFactor = fValue;
			}
			else if (_stricmp(param, "speed_shader_particle_size") == 0) {
				g_fSpeedShaderParticleSize = fValue;
			}
			else if (_stricmp(param, "speed_shader_max_intensity") == 0) {
				g_fSpeedShaderMaxIntensity = fValue;
			}
			else if (_stricmp(param, "speed_shader_trail_length") == 0) {
				g_fSpeedShaderTrailSize = fValue;
			}
			else if (_stricmp(param, "speed_shader_max_particles") == 0) {
				g_iSpeedShaderMaxParticles = (int)fValue;
				if (g_iSpeedShaderMaxParticles < 0) g_iSpeedShaderMaxParticles = 0;
				g_iSpeedShaderMaxParticles = min(g_iSpeedShaderMaxParticles, MAX_SPEED_PARTICLES);
			}
			else if (_stricmp(param, "speed_shader_particle_range") == 0) {
				g_fSpeedShaderParticleRange = fValue;
			}
			// Additional Geometry Shader
			else if (_stricmp(param, "enable_additional_geometry") == 0) {
				g_bEnableAdditionalGeometry = (bool)fValue;
			}
			else if (_stricmp(param, "add_geom_trans_scale") == 0) {
				g_fCockpitTranslationScale = fValue;
			}
			// Shadow Mapping
			else if (_stricmp(param, "shadow_mapping_enable") == 0) {
				g_bShadowMapEnable = (bool)fValue;
				g_ShadowMapping.bEnabled = g_bShadowMapEnable;
				g_ShadowMapVSCBuffer.sm_enabled = g_ShadowMapping.bEnabled;
				log_debug("[DBG] [SHW] g_ShadowMapping.Enabled: %d", g_ShadowMapping.bEnabled);
			}
			else if (_stricmp(param, "cascaded_shadow_mapping_enable") == 0) {
				g_ShadowMapping.bCSMEnabled = (bool)fValue;
				g_ShadowMapVSCBuffer.sm_csm_enabled = g_ShadowMapping.bEnabled;
				log_debug("[DBG] [SHW] g_ShadowMapping.Enabled: %d", g_ShadowMapping.bEnabled);
			}
			else if (_stricmp(param, "shadow_mapping_anisotropic_scale") == 0) {
				g_ShadowMapping.bAnisotropicMapScale = (bool)fValue;
			}
			else if (_stricmp(param, "shadow_mapping_angle_x") == 0) {
				g_fShadowMapAngleX = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapAngleX: %0.3f", g_fShadowMapAngleX);
			}
			else if (_stricmp(param, "shadow_mapping_angle_y") == 0) {
				g_fShadowMapAngleY = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapAngleY: %0.3f", g_fShadowMapAngleY);
			}
			else if (_stricmp(param, "shadow_mapping_scale") == 0) {
				g_fShadowMapScale = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapScale: %0.3f", g_fShadowMapScale);
			}

			else if (_stricmp(param, "shadow_mapping_OBJ_scale") == 0) {
				SHADOW_OBJ_SCALE = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapScale: %0.3f", SHADOW_OBJ_SCALE);
			}

			else if (_stricmp(param, "shadow_mapping_z_factor") == 0) {
				g_ShadowMapVSCBuffer.sm_z_factor = fValue;
				log_debug("[DBG] [SHW] sm_z_factor: %0.3f", g_ShadowMapVSCBuffer.sm_z_factor);
			}

			else if (_stricmp(param, "shadow_mapping_pcss_samples") == 0) {
				g_ShadowMapVSCBuffer.sm_pcss_samples = (int)fValue;
			}

			else if (_stricmp(param, "shadow_mapping_black_level") == 0) {
				g_ShadowMapping.black_level = fValue;
				for (int i = 0; i < MAX_XWA_LIGHTS; i++)
					if (!g_XWALightInfo[i].bTagged)
						g_ShadowMapVSCBuffer.sm_black_levels[i] = fValue;
			}

			else if (_stricmp(param, "shadow_mapping_black_level_0") == 0) {
				if (!g_XWALightInfo[0].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[0] = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_black_level_1") == 0) {
				if (!g_XWALightInfo[1].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[1] = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_black_level_2") == 0) {
				if (!g_XWALightInfo[2].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[2] = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_black_level_3") == 0) {
				if (!g_XWALightInfo[3].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[3] = fValue;
			}

			else if (_stricmp(param, "shadow_mapping_depth_bias") == 0) {
				g_ShadowMapping.DepthBias = (int)fValue;
			}
			else if (_stricmp(param, "shadow_mapping_depth_bias_clamp") == 0) {
				g_ShadowMapping.DepthBiasClamp = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_slope_scale_depth_bias") == 0) {
				g_ShadowMapping.SlopeScaledDepthBias = fValue;
			}

			else if (_stricmp(param, "shadow_mapping_POV_XY_FACTOR") == 0) {
				g_ShadowMapping.POV_XY_FACTOR = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_POV_Z_FACTOR") == 0) {
				g_ShadowMapping.POV_Z_FACTOR = fValue;
			}


			else if (_stricmp(param, "shadow_mapping_depth_trans") == 0) {
				g_fShadowMapDepthTrans = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_invert_camera_matrix") == 0) {
				g_bShadowMappingInvertCameraMatrix = (bool)fValue;
			}

			else if (_stricmp(param, "shadow_mapping_bias_sw") == 0) {
				//g_ShadowMapVSCBuffer.sm_bias = fValue;
				g_ShadowMapping.sw_pcf_bias = fValue;
				log_debug("[DBG] [SHW] sw_pcf_bias: %0.3f", g_ShadowMapping.sw_pcf_bias);
			}
			else if (_stricmp(param, "shadow_mapping_bias_hw") == 0) {
				//g_ShadowMapVSCBuffer.sm_bias = fValue;
				g_ShadowMapping.hw_pcf_bias = fValue;
				log_debug("[DBG] [SHW] hw_pcf_bias: %0.3f", g_ShadowMapping.hw_pcf_bias);
			}

			else if (_stricmp(param, "shadow_mapping_debug") == 0) {
				g_bShadowMapDebug = (bool)fValue;
				g_ShadowMapVSCBuffer.sm_debug = g_bShadowMapDebug;
			}
			else if (_stricmp(param, "shadow_mapping_pcss_radius") == 0) {
				g_ShadowMapVSCBuffer.sm_pcss_radius = fValue;
			}
			//else if (_stricmp(param, "shadow_mapping_light_size") == 0) {
			//	g_ShadowMapVSCBuffer.sm_light_size = fValue;
			//}
			//else if (_stricmp(param, "shadow_mapping_blocker_radius") == 0) {
			//	g_ShadowMapVSCBuffer.sm_blocker_radius = fValue;
			//}
			else if (_stricmp(param, "shadow_mapping_POV_X") == 0) {
				g_ShadowMapVSCBuffer.POV.x = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_POV_Y") == 0) {
				g_ShadowMapVSCBuffer.POV.y = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_POV_Z") == 0) {
				g_ShadowMapVSCBuffer.POV.z = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_OBJrange_override") == 0) {
				g_ShadowMapping.bOBJrange_override = (bool)fValue;
			}
			else if (_stricmp(param, "shadow_mapping_OBJrange_value") == 0) {
				g_ShadowMapping.fOBJrange_override_value = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_fovdist_scale") == 0) {
				g_ShadowMapping.FOVDistScale = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_enable_multiple_suns") == 0) {
				g_ShadowMapping.bMultipleSuns = (bool)fValue;
			}

			/*
			else if (_stricmp(param, "shadow_mapping_XWA_LIGHT_Y_CONV_SCALE") == 0) {
				g_ShadowMapping.XWA_LIGHT_Y_CONV_SCALE = fValue;
			}
			*/

			else if (_stricmp(param, "dump_OBJ_enabled") == 0) {
				g_bDumpOBJEnabled = (bool)fValue;
			}
			else if (_stricmp(param, "HDR_enabled") == 0) {
				SetHDRState((bool)fValue);
			}
			else if (_stricmp(param, "HDR_lights_intensity") == 0) {
				g_fHDRLightsMultiplier = fValue;
			}

			/*
			else if (_stricmp(param, "flare_aspect_mult") == 0) {
				g_fFlareAspectMult = fValue;
				log_debug("[DBG] g_fFlareAspectMult: %0.3f", g_fFlareAspectMult);
			}
			*/

			/*else if (_stricmp(param, "viewYawSign") == 0) {
				g_fViewYawSign = fValue;
			}
			else if (_stricmp(param, "viewPitchSign") == 0) {
				g_fViewPitchSign = fValue;
			}*/

			/*
			else if (_stricmp(param, "gamma") == 0) {
				g_SSAO_PSCBuffer.gamma = fValue;
			}
			*/
			/*else if (_stricmp(param, "shadow_step_size") == 0) {
				g_SSAO_PSCBuffer.shadow_step_size = fValue;
			}
			else if (_stricmp(param, "shadow_steps") == 0) {
				g_SSAO_PSCBuffer.shadow_steps = fValue;
			}
			else if (_stricmp(param, "shadow_k") == 0) {
				g_SSAO_PSCBuffer.shadow_k = fValue;
			}
			else if (_stricmp(param, "shadow_epsilon") == 0) {
				g_SSAO_PSCBuffer.shadow_epsilon = fValue;
			}
			*/
			/*
			else if (_stricmp(param, "white_point") == 0) {
				g_SSAO_PSCBuffer.white_point = fValue;
			}
			*/
			else if (_stricmp(param, "Bz_mult") == 0) {
				g_SSAO_PSCBuffer.Bz_mult = fValue;
			}
			/*else if (_stricmp(param, "light_vector") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightVector[0].x = x;
				g_LightVector[0].y = y;
				g_LightVector[0].z = z;
				g_LightVector[0].w = 0.0f;
				g_LightVector[0].normalize();
				log_debug("[DBG] [AO] Light vec: [%0.3f, %0.3f, %0.3f]",
					g_LightVector[0].x, g_LightVector[0].y, g_LightVector[0].z);
			}
			else if (_stricmp(param, "light_vector2") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightVector[1].x = x;
				g_LightVector[1].y = y;
				g_LightVector[1].z = z;
				g_LightVector[1].w = 0.0f;
				g_LightVector[1].normalize();
				log_debug("[DBG] [AO] Light vec2: [%0.3f, %0.3f, %0.3f]",
					g_LightVector[1].x, g_LightVector[1].y, g_LightVector[1].z);
			}
			else if (_stricmp(param, "light_color") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightColor[0].x = x;
				g_LightColor[0].y = y;
				g_LightColor[0].z = z;
				g_LightColor[0].w = 0.0f;
				log_debug("[DBG] [AO] Light Color: [%0.3f, %0.3f, %0.3f]",
					g_LightColor[0].x, g_LightColor[0].y, g_LightColor[0].z);
			}
			else if (_stricmp(param, "light_color2") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightColor[1].x = x;
				g_LightColor[1].y = y;
				g_LightColor[1].z = z;
				g_LightColor[1].w = 0.0f;
				log_debug("[DBG] [AO] Light Color2: [%0.3f, %0.3f, %0.3f]",
					g_LightColor[1].x, g_LightColor[1].y, g_LightColor[1].z);
			}*/
			/*
			else if (_stricmp(param, "shadow_color") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_SSAO_PSCBuffer.invLightR = x;
				g_SSAO_PSCBuffer.invLightG = y;
				g_SSAO_PSCBuffer.invLightB = z;
			}
			*/

			/* Shading System Settings */
			else if (_stricmp(param, "specular_intensity") == 0) {
				g_ShadingSys_PSBuffer.spec_intensity = fValue;
				g_fSpecIntensity = fValue;
			}
			else if (_stricmp(param, "specular_bloom_intensity") == 0) {
				g_ShadingSys_PSBuffer.spec_bloom_intensity = fValue;
				g_fSpecBloomIntensity = fValue;
			}
			else if (_stricmp(param, "lightness_boost") == 0) {
				g_ShadingSys_PSBuffer.lightness_boost = fValue;
			}
			else if (_stricmp(param, "min_light_intensity_after_fade") == 0) {
				g_fMinLightIntensity = fValue;
			}
			else if (_stricmp(param, "fade_lights") == 0) {
				g_bFadeLights = (bool)fValue;
			}
			else if (_stricmp(param, "saturation_boost") == 0) {
				g_ShadingSys_PSBuffer.saturation_boost = fValue;
			}
			else if (_stricmp(param, "glossiness") == 0) {
				g_ShadingSys_PSBuffer.glossiness = fValue;
			}
			else if (_stricmp(param, "bloom_glossiness_multiplier") == 0) {
				g_ShadingSys_PSBuffer.bloom_glossiness_mult = fValue;
			}
			else if (_stricmp(param, "ss_debug") == 0) {
				g_ShadingSys_PSBuffer.ss_debug = (int)fValue;
			}
			else if (_stricmp(param, "key_set") == 0) {
				g_KeySet = (int)fValue;
				log_debug("[DBG] key_set: %d", g_KeySet);
			}
			else if (_stricmp(param, "enable_laser_lights") == 0) {
				g_bEnableLaserLights = (bool)fValue;
			}
			else if (_stricmp(param, "enable_explosion_lights") == 0) {
				g_bEnableExplosionLights = (bool)fValue;
			}
			else if (_stricmp(param, "sqr_attenuation") == 0) {
				g_ShadingSys_PSBuffer.sqr_attenuation = fValue;
			}
			else if (_stricmp(param, "laser_light_intensity") == 0) {
				// The way the laser light is attached to the laser texture has changed. I'm now using
				// barycentric coords. I believe the old method may have attached two lights per laser
				// so the new method now looks too dim. To compensate, we probably need to double the
				// laser light intensity here.
				g_ShadingSys_PSBuffer.laser_light_intensity = 2.0f * fValue;
			}
			else if (_stricmp(param, "headlights_pos") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_HeadLightsPosition.x = x;
				g_HeadLightsPosition.y = y;
				g_HeadLightsPosition.z = z;
			}
			else if (_stricmp(param, "headlights_col") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_HeadLightsColor.x = x;
				g_HeadLightsColor.y = y;
				g_HeadLightsColor.z = z;
			}
			else if (_stricmp(param, "headlights_ambient") == 0) {
				g_fHeadLightsAmbient = fValue;
			}
			else if (_stricmp(param, "headlights_distance") == 0) {
				// Let's make the headlights a bit more powerful -- this helps in the core room in the DS2
				// By multiplying by 3.33, we make the headlights good enough for the Death Star
				g_fHeadLightsDistance = 3.33f * fValue;
			}
			else if (_stricmp(param, "headlights_angle") == 0) {
				g_fHeadLightsAngleCos = cos(0.01745f * fValue);
			}
			else if (_stricmp(param, "headlights_auto_turn_on") == 0) {
				g_bHeadLightsAutoTurnOn = (bool)fValue;
			}
			else if (_stricmp(param, "reload_materials_enabled") == 0) {
				g_bReloadMaterialsEnabled = (bool)fValue;
				log_debug("[DBG] [MAT] Material Reloading Enabled? %d", g_bReloadMaterialsEnabled);
			}

			/*else if (_stricmp(param, "g_fOBJZMetricMult") == 0) {
				g_fOBJ_Z_MetricMult = fValue;
				log_debug("[DBG] [SHOW] g_fOBJZMetricMult: %0.3f", g_fOBJ_Z_MetricMult);
			}
			else if (_stricmp(param, "g_fOBJGlobalMetricMult") == 0) {
				g_fOBJGlobalMetricMult = fValue;
				log_debug("[DBG] [SHOW] g_fOBJGlobalMetricMult: %0.3f", g_fOBJGlobalMetricMult);
			}*/

			/*else if (_stricmp(param, "emission_intensity") == 0) {
				g_ShadingSys_PSBuffer.emission_intensity = fValue;
			}*/

			else if (_stricmp(param, "external_hud_enabled") == 0) {
				g_bExternalHUDEnabled = (bool)fValue;
			}
			else if (_stricmp(param, "ext_hud_ar0") == 0) {
				g_ShadertoyBuffer.preserveAspectRatioComp[0] = fValue;
			}
			else if (_stricmp(param, "ext_hud_ar1") == 0) {
				g_ShadertoyBuffer.preserveAspectRatioComp[1] = fValue;
			}

			if (_stricmp(param, "g_fBlastMarkOfsX") == 0) {
				g_fBlastMarkOfsX = fValue;
				log_debug("[DBG] g_fBlastMarkOfsX: %0.6f", g_fBlastMarkOfsX);
			}
			if (_stricmp(param, "g_fBlastMarkOfsY") == 0) {
				g_fBlastMarkOfsY = fValue;
				log_debug("[DBG] g_fBlastMarkOfsY: %0.6f", g_fBlastMarkOfsY);
			}

			if (_stricmp(param, "star_debug_enabled") == 0) {
				g_bStarDebugEnabled = (bool)fValue;
			}

			if (_stricmp(param, "DS2_force_procedural_explosions") == 0) {
				g_bDS2ForceProceduralExplosions = (bool)fValue;
			}
			if (_stricmp(param, "force_alt_explosion") == 0) {
				g_iForceAltExplosion = (int)fValue;
			}
			// Set this to false to prevent the cleanup of temporary ZIP files.
			// This makes loading missions a little bit faster after the first
			// time, since files will remain uncompressed on disk.
			if (_stricmp(param, "cleanup_temp_zip_files") == 0) {
				g_bCleanupZIPDirs = (bool)fValue;
			}

			if (_stricmp(param, "enable_levels_shader") == 0) {
				g_bEnableLevelsShader = (bool)fValue;
			}
			if (_stricmp(param, "levels_white_point") == 0) {
				g_fLevelsWhitePoint = fValue;
			}
			if (_stricmp(param, "levels_black_point") == 0) {
				g_fLevelsBlackPoint = fValue;
			}

			if (_stricmp(param, "raytracing_exclude_opt") == 0) {
				toupper(svalue);
				std::string OPTName(svalue);
				log_debug("[DBG] [BVH] Excluding OPT: %s", OPTName.c_str());
				g_RTExcludeOPTNames[OPTName] = true;
			}

			if (_stricmp(param, "raytracing_exclude_class") == 0)
			{
				if (_stricmp(svalue, "Starships") == 0)
				{
					g_RTExcludeShipCategories[Genus_Starship] = true;
					log_debug("[DBG] [BVH] Excluding Starships");
				}
				else if (_stricmp(svalue, "Stations") == 0)
				{
					g_RTExcludeShipCategories[Genus_Platform] = true;
					log_debug("[DBG] [BVH] Excluding Stations");
				}
				else if (_stricmp(svalue, "Starfighters") == 0)
				{
					g_RTExcludeShipCategories[Genus_Starfighter] = true;
					log_debug("[DBG] [BVH] Excluding Starfighters");
				}
				else if (_stricmp(svalue, "Freighters") == 0)
				{
					g_RTExcludeShipCategories[Genus_Freighter] = true;
					log_debug("[DBG] [BVH] Excluding Freighters");
				}
				else if (_stricmp(svalue, "Transports") == 0)
				{
					g_RTExcludeShipCategories[Genus_Transport] = true;
					log_debug("[DBG] [BVH] Excluding Transports");
				}
				else if (_stricmp(svalue, "Mines") == 0)
				{
					g_RTExcludeShipCategories[Genus_Mine] = true;
					log_debug("[DBG] [BVH] Excluding Mines");
				}
				else if (_stricmp(svalue, "Satellites") == 0)
				{
					g_RTExcludeShipCategories[Genus_Satellite] = true;
					log_debug("[DBG] [BVH] Excluding Satellites");
				}
			}
		}
	}
	fclose(file);

	log_debug("[DBG] [AO] SSAO Enabled: %d", g_bAOEnabled);
	log_debug("[DBG] [AO] SSAO bias: %0.3f", g_SSAO_PSCBuffer.bias);
	log_debug("[DBG] [AO] SSAO intensity: %0.3f", g_SSAO_PSCBuffer.intensity);
	log_debug("[DBG] [AO] SSAO near_sample_radius: %0.6f", g_SSAO_PSCBuffer.near_sample_radius);
	log_debug("[DBG] [AO] SSAO far_sample_radius: %0.6f", g_SSAO_PSCBuffer.far_sample_radius);
	log_debug("[DBG] [AO] SSAO samples: %d", g_SSAO_PSCBuffer.samples);
	log_debug("[DBG] [AO] SSAO black_level: %f", g_SSAO_PSCBuffer.black_level);
	if (g_SSAO_Type == SSO_AMBIENT)
		log_debug("[DBG] [AO] SSO: AMBIENT OCCLUSION");
	else if (g_SSAO_Type == SSO_DIRECTIONAL)
		log_debug("[DBG] [AO] SSO: DIRECTIONAL OCCLUSION");
	return true;
}

bool LoadHyperParams() {
	log_debug("[DBG] Loading Hyperspace params...");
	FILE* file;
	int error = 0, line = 0;

	// Provide some default values in case they are missing in the config file
	g_ShadertoyBuffer.y_center = 0.15f;
	g_ShadertoyBuffer.FOVscale = 1.0f;
	g_ShadertoyBuffer.viewMat.identity();
	g_ShadertoyBuffer.Style = g_iHyperStyle;
	g_ShadertoyBuffer.tunnel_speed = 5.5f;
	g_ShadertoyBuffer.twirl = 1.0f;
	g_fHyperLightRotationSpeed = 50.0f;
	g_fHyperShakeRotationSpeed = 50.0f;
	g_bHyperDebugMode = false;

	try {
		error = fopen_s(&file, "./hyperspace.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load hyperspace.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading hyperspace.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "disney_style") == 0) {
				g_iHyperStyle = (int)fValue;
				g_ShadertoyBuffer.Style = g_iHyperStyle;
			}
			else if (_stricmp(param, "tunnel_speed") == 0) {
				g_fHyperspaceTunnelSpeed = fValue;
			}
			else if (_stricmp(param, "twirl") == 0) {
				g_ShadertoyBuffer.twirl = fValue;
			}
			else if (_stricmp(param, "debug_mode") == 0) {
				g_bHyperDebugMode = (bool)fValue;
			}
			else if (_stricmp(param, "debug_fsm") == 0) {
				g_iHyperStateOverride = (int)fValue;
			}
			else if (_stricmp(param, "light_rotation_speed") == 0) {
				g_fHyperLightRotationSpeed = fValue;
			}
			else if (_stricmp(param, "shake_speed") == 0) {
				g_fHyperShakeRotationSpeed = fValue;
			}
			else if (_stricmp(param, "interdiction_shake_scale") == 0) {
				g_fInterdictionShake = fValue;
			}
			else if (_stricmp(param, "interdiction_shake_scale_in_VR") == 0) {
				g_fInterdictionShakeInVR = fValue;
			}
			else if (_stricmp(param, "interdiction_angle_scale") == 0) {
				g_fInterdictionAngleScale = fValue;
			}
		}
	}
	fclose(file);

	return true;
}

bool Load3DVisionParams() {
	log_debug("[DBG] Loading 3DVision params...");
	FILE* file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./3DVision.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load 3DVision.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading 3DVision.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;
	// Set some default values
	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "enable_3D_vision") == 0) {
				g_b3DVisionEnabled = (bool)fValue;
				if (g_b3DVisionEnabled) {
					log_debug("[DBG] [3DV] 3D Vision enabled");
					// Force DirectSBS mode
					g_bSteamVREnabled = false;
					g_bEnableVR = true;
					g_config.AspectRatioPreserved = true;
				}
			}
			else if (_stricmp(param, "force_fullscreen") == 0) {
				g_b3DVisionForceFullScreen = (bool)fValue;
			}
		}
	}
	fclose(file);

	return true;
}

bool LoadDefaultGlobalMaterial() {
	FILE* file;
	int error = 0, line = 0;
	char buf[256];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Failsafe: populate with constant defaults:
	g_DefaultGlobalMaterial.Metallic = DEFAULT_METALLIC;
	g_DefaultGlobalMaterial.Intensity = DEFAULT_SPEC_INT;
	g_DefaultGlobalMaterial.Glossiness = DEFAULT_GLOSSINESS;
	g_DefaultGlobalMaterial.NMIntensity = DEFAULT_NM_INT;
	g_DefaultGlobalMaterial.SpecValue = DEFAULT_SPEC_VALUE;
	g_DefaultGlobalMaterial.IsShadeless = false;
	g_DefaultGlobalMaterial.Light.set(0.0f, 0.0f, 0.0f);

	try {
		error = fopen_s(&file, "./Materials/DefaultGlobalMaterial.mat", "rt");
	}
	catch (...) {
		//log_debug("[DBG] [MAT] Could not load [%s]", sFileName);
	}

	if (error != 0) {
		//log_debug("[DBG] [MAT] Error %d when loading [%s]", error, sFileName);
		return false;
	}
	log_debug("[DBG] [MAT] Loading DefaultGlobalMaterial.mat");

	// Now, try to load the global materials file
	while (fgets(buf, 256, file) != NULL) {
		line++;
		ReadMaterialLine(buf, &g_DefaultGlobalMaterial, "GlobalMaterial");
	}
	fclose(file);
	return true;
}

bool LoadMultiplayerConfigFromHooks_ini() {
	/*
	 In the LoadMultiplayerConfig function in VRConfig.cpp you read the content of the
	 hook_tourmultiplayer.cfg config file. In the future these settings will eventualy
	 be moved to Hooks.ini in a [hook_tourmultiplayer] section. Can you update the code
	 to also read the ini file?
	 */
	log_debug("[DBG] Loading hooks.ini...");
	FILE* file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./hooks.ini", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load hooks.ini");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading hooks.ini", error);
		return false;
	}

	bool mpSection = false;
	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (buf[0] == '[') {
			bool prevMPSection = mpSection;
			mpSection = (stristr(buf, "[hook_tourmultiplayer]") != NULL);
			if (!mpSection && prevMPSection)
			{
				// We've seen the MP section before and this is a new section:
				// we're done here
				break;
			}
		}
		else if (mpSection)
		{
			if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
				fValue = (float)atof(svalue);

				if (_stricmp(param, "GreenAndRedForIFFColorsOnly") == 0) {
					g_bGreenAndRedForIFFColorsOnly = (bool)fValue;
					break;
				}
			}
		}
	}
	fclose(file);

	return true;
}

bool LoadMultiplayerConfig() {
	// Try loading the MP configuration from hooks.ini first
	if (LoadMultiplayerConfigFromHooks_ini())
	{
		return true;
	}

	log_debug("[DBG] Loading hook_tourmultiplayer.cfg...");
	FILE* file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./hook_tourmultiplayer.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load hook_tourmultiplayer.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading hook_tourmultiplayer.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "GreenAndRedForIFFColorsOnly") == 0) {
				g_bGreenAndRedForIFFColorsOnly = (bool)fValue;
				break;
			}
		}
	}
	fclose(file);

	return true;
}

bool LoadGimbaLockFixConfig() {
	log_debug("[DBG] Loading GimbalLockFix.cfg...");
	FILE* file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./GimbalLockFix.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load GimbalLockFix.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading GimbalLockFix.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "enable_gimbal_lock_fix") == 0) {
				g_bEnableGimbalLockFix = (bool)fValue;
			}
			else if (_stricmp(param, "enable_rudder") == 0) {
				g_bEnableRudder = (bool)fValue;
			}
			/*
			else if (_stricmp(param, "debug_mode") == 0) {
				g_bGimbalLockDebugMode = (bool)fValue;
			}
			*/
			else if (_stricmp(param, "yaw_accel_rate_s") == 0) {
				g_fYawAccelRate_s = fValue;
			}
			else if (_stricmp(param, "pitch_accel_rate_s") == 0) {
				g_fPitchAccelRate_s = fValue;
			}
			else if (_stricmp(param, "roll_accel_rate_s") == 0) {
				g_fRollAccelRate_s = fValue;
			}

			else if (_stricmp(param, "max_yaw_rate_s") == 0) {
				g_fMaxYawRate_s = fValue;
			}
			else if (_stricmp(param, "max_pitch_rate_s") == 0) {
				g_fMaxYawRate_s = fValue;
			}
			else if (_stricmp(param, "max_roll_rate_s") == 0) {
				g_fMaxRollRate_s = fValue;
			}

			else if (_stricmp(param, "turn_rate_scale_throttle_0") == 0) {
				g_fTurnRateScaleThr_0 = fValue;
			}
			else if (_stricmp(param, "turn_rate_scale_throttle_100") == 0) {
				g_fTurnRateScaleThr_100 = fValue;
			}
			else if (_stricmp(param, "max_turn_accel_rate_s") == 0) {
				g_fMaxTurnAccelRate_s = fValue;
			}

			else if (_stricmp(param, "mouse_range_x") == 0) {
				g_fMouseRangeX = fValue;
			}
			else if (_stricmp(param, "mouse_range_y") == 0) {
				g_fMouseRangeY = fValue;
			}
			else if (_stricmp(param, "mouse_decel_rate_s") == 0) {
				g_fMouseDecelRate_s = fValue;
			}

			else if (_stricmp(param, "modulate_accel_rate_with_throttle") == 0) {
				g_bThrottleModulationEnabled = (bool)fValue;
			}
		}
	}
	fclose(file);

	return true;
}

void ReloadMaterials()
{
	char* surface_name;
	char texname[MAX_TEXNAME];
	bool bIsDat;
	OPTNameType OPTname;

	if (!g_bReloadMaterialsEnabled) {
		log_debug("[DBG] [MAT] Material Reloading is not enabled");
		return;
	}

	log_debug("[DBG] [MAT] Reloading materials.");
	ClearAnimatedMaterials();
	ClearCraftMaterials();
	ClearOPTnames();

	for (Direct3DTexture* texture : g_AuxTextureVector) {
		OPTname.name[0] = 0;
		surface_name = texture->_surface->_cname;
		bIsDat = false;
		//bIsDat = strstr(surface_name, "dat,") != NULL;

		// Capture the OPT/DAT name and load the material file
		char* start = strstr(surface_name, "\\");
		char* end = strstr(surface_name, ".opt");
		char sFileName[180], sFileNameShort[180];
		if (start != NULL && end != NULL) {
			start += 1; // Skip the backslash
			int size = end - start;
			strncpy_s(OPTname.name, MAX_OPT_NAME, start, size);
			if (!isInVector(OPTname.name, g_OPTnames)) {
				//log_debug("[DBG] [MAT] OPT Name Captured: '%s'", OPTname.name);
				// Add the name to the list of OPTnames so that we don't try to process it again
				g_OPTnames.push_back(OPTname);
				OPTNameToMATParamsFile(OPTname.name, sFileName, 180);
				log_debug("[DBG] [MAT] [OPT] Reloading file %s...", sFileName);
				LoadIndividualMATParams(OPTname.name, sFileName); // OPT material
			}
		}
		else if (strstr(surface_name, "dat,") != NULL) {
			bIsDat = true;
			// For DAT images, OPTname.name is the full DAT name:
			strncpy_s(OPTname.name, MAX_OPT_NAME, surface_name, strlen(surface_name));
			DATNameToMATParamsFile(OPTname.name, sFileName, sFileNameShort, 180);
			if (sFileName[0] != 0) {
				log_debug("[DBG] [MAT] [DAT] Reloading file %s...", sFileName);
				// Reload a regular DAT material file:
				if (!LoadIndividualMATParams(OPTname.name, sFileName)) {
					// If the above failed, try reloading the animated mat file:
					if (sFileNameShort[0] != 0)
						LoadIndividualMATParams(OPTname.name, sFileNameShort, false);
				}
			}
		}

		int craftIdx = FindCraftMaterial(OPTname.name);
		//log_debug("[DBG] [MAT] craftIdx: %d", craftIdx);
		if (bIsDat)
			texname[0] = 0; // Retrieve the default material
		else
		{
			//log_debug("[DBG] [MAT] Craft Material %s found", OPTname.name);
			char* start = strstr(surface_name, ".opt");
			// Skip the ".opt," part
			start += 5;
			// Find the next comma
			char* end = strstr(start, ",");
			int size = end - start;
			strncpy_s(texname, MAX_TEXNAME, start, size);
		}
		//log_debug("[DBG] [MAT] Looking for material for %s", texname);
		//bool Debug = strstr(surface_name, "CalamariLulsa") != NULL || bIsDat;
		texture->material = FindMaterial(craftIdx, texname, false);
		//if (Debug)
		//	log_debug("[DBG] Re-Applied Mat: %0.3f, %0.3f, %0.3f", texture->material.Metallic, texture->material.Intensity, texture->material.Glossiness);
		texture->bHasMaterial = true;
	}
}

std::map<int, std::string> LoadMissionList() {
	FILE* file;
	int error = 0, line = 0;
	std::map<int, std::string> missionMap;
	log_debug("[DBG] [INT] Loading Mission List...");

	// Read Missions\Mission.lst and build a map of mission index -> filename.
	try {
		error = fopen_s(&file, ".\\Missions\\Mission.lst", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load Mission.lst");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading Mission.lst", error);
		return missionMap;
	}

	char buf[256], param[256], *aux;
	int curIdx = -1;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		int len = strlen(buf);
		if (strlen(buf) == 0)
			continue;
		if (buf[0] == '/' || buf[0] == '!')
			continue;

		if (stristr(buf, ".tie") != NULL) {
			aux = &(buf[0]);
			// Sometimes, missions begin with "* "
			if (*aux == '*') {
				aux += 2;
				len -= 2;
			}
			// Remove the ".tie" ending
			aux[len - 5] = 0;
			sprintf_s(param, 256, ".\\Missions\\%s.ini", aux);
			missionMap[curIdx] = std::string(param);
			//log_debug("[DBG] [INT] %d --> %s", curIdx, missionMap[curIdx].c_str());
		} else if (isdigit(buf[0])) {
			curIdx = atoi(buf);
		}
	}
	fclose(file);
	return missionMap;
}

// Reads a list of integers of the form:
// X, Y, Z, ...
// and returns the equivalent vector
std::vector<int> ReadIntegerList(char *svalue) {
	int i = 0;
	int len = strlen(svalue);
	std::vector<int> result;

	while (i < len) {
		int j = i;
		// Advance to the next comma
		while (j < len && svalue[j] != ',') j++;
		// Terminate the string at the comma
		if (j < len) svalue[j] = 0;
		// Parse the integer
		int elem = atoi(svalue + i);
		result.push_back(elem);
		// Repeat, starting at the first character after the comma
		i = j + 1;
	}
	return result;
}

uint8_t LoadInterdictionMap(const char *fileName)
{
	int error = 0;
	FILE *file = NULL;

	try {
		error = fopen_s(&file, fileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [INT] Could not load %s", fileName);
	}

	if (error != 0) {
		log_debug("[DBG] [INT] Error %d when loading %s", error, fileName);
		return 0;
	}

	// Read the mission's .ini file. Find the "[Interdiction]" section and
	// populate the bitfield
	bool interdictionSection = false;
	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	int iValue = -1, line = 0;
	uint8_t bitfield = 0x0;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		if (strlen(buf) == 0)
			continue;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;

		if (buf[0] == '[') {
			bool prevInterdictionSection = interdictionSection;
			interdictionSection = (stristr(buf, "[Interdiction]") != NULL);
			if (!interdictionSection && prevInterdictionSection)
				break;
		}
		else if (interdictionSection) {
			if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
				iValue = atoi(svalue);

				if (_stricmp(param, "Region") == 0) {
					// scanf stops reading strings on commas, so we need the full right
					// substring:
					char *right_side = stristr(buf, "=");
					// Skip the "=" sign
					right_side++;
					std::vector regions = ReadIntegerList(right_side);
					for (int region : regions)
						bitfield |= (0x1 << region);
					//log_debug("[DBG] [INT] Interdiction set on current mission, bitfield: 0x%x", bitfield);
				}
			}
		}

	}
	fclose(file);
	return bitfield;
}

void ReloadInterdictionMap() {
	static char prevMission[128] = "";
	char aux[128];
	int len;
	
	if (_stricmp(xwaMissionFileName, prevMission) != 0)
	{
		// A new mission has been loaded since the last time this function
		// was called. Reset the bitfield and reload the .ini file
		g_iInterdictionBitfield = 0;
		// Update the name of the previous mission
		len = strlen(xwaMissionFileName);
		strncpy_s(prevMission, 128, xwaMissionFileName, len);
		strncpy_s(aux, 128, xwaMissionFileName, len);
		
		// Replace the ".tie" ending with an ".ini" ending.
		aux[len - 3] = 'i';
		aux[len - 2] = 'n';
		aux[len - 1] = 'i';
		g_iInterdictionBitfield = LoadInterdictionMap(aux);
		log_debug("[DBG] [INT] Interdictions in mission: [%s] = 0x%x",
			aux, g_iInterdictionBitfield);
	}
}

void ToggleCockpitPZHack() {
	g_bCockpitPZHackEnabled = !g_bCockpitPZHackEnabled;
	//log_debug("[DBG] CockpitHackEnabled: %d", g_bCockpitPZHackEnabled);
	if (!g_bCockpitPZHackEnabled)
		g_fCockpitPZThreshold = 2.0f;
	else
		g_fCockpitPZThreshold = g_fBackupCockpitPZThreshold;
}

void ToggleZoomOutMode() {
	g_bZoomOut = !g_bZoomOut;
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;
}

void IncreaseZOverride(float Delta) {
	g_fZBracketOverride += Delta;
	//log_debug("[DBG] g_fZOverride: %f", g_fZOverride);
}

void IncreaseZoomOutScale(float Delta) {
	g_fGlobalScaleZoomOut += Delta;
	if (g_fGlobalScaleZoomOut < 0.2f)
		g_fGlobalScaleZoomOut = 0.2f;

	// Apply this change by modifying the global scale:
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;

	g_fConcourseScale += Delta;
	if (g_fConcourseScale < 0.2f)
		g_fConcourseScale = 0.2f;
}

void IncreaseHUDParallax(float Delta) {
	g_fHUDDepth += Delta;
	log_debug("[DBG] HUD parallax: %f", g_fHUDDepth);
}

void IncreaseFloatingGUIParallax(float Delta) {
	g_fFloatingGUIDepth += Delta;
	log_debug("[DBG] GUI parallax: %f", g_fFloatingGUIDepth);
}

void IncreaseTextParallax(float Delta) {
	g_fTextDepth += Delta;
	log_debug("[DBG] Text parallax: %f", g_fTextDepth);
}

void IncreaseCockpitThreshold(float Delta) {
	g_fCockpitPZThreshold += Delta;
	g_fBackupCockpitPZThreshold = g_fCockpitPZThreshold;
	log_debug("[DBG] New cockpit threshold: %f", g_fCockpitPZThreshold);
}

void IncreaseGUIThreshold(float Delta) {
	g_fGUIElemPZThreshold += Delta;
	//log_debug("[DBG] New GUI threshold: %f", GUI_elem_pz_threshold);
}

void IncreaseScreenScale(float Delta) {
	g_fGlobalScale += Delta;
	if (g_fGlobalScale < 0.2f)
		g_fGlobalScale = 0.2f;
	log_debug("[DBG] New g_fGlobalScale: %0.3f", g_fGlobalScale);
}

/*
void IncreasePostProjScale(float Delta) {
	g_fPostProjScale += Delta;
	if (g_fPostProjScale < 0.2f)
		g_fPostProjScale = 0.2f;
	log_debug("[DBG] New g_fPostProjScale: %0.3ff", g_fPostProjScale);
}
*/

/*
void IncreaseFocalDist(float Delta) {
	g_fFocalDist += Delta;
	if (g_fFocalDist < 0.01f)
		g_fFocalDist = 0.01f;
	log_debug("[DBG] g_fFocalDist: %f", g_fFocalDist);
}
*/

void IncreaseLensK1(float Delta) {
	g_fLensK1 += Delta;
	log_debug("[DBG] New k1: %f", g_fLensK1);
}

void IncreaseLensK2(float Delta) {
	g_fLensK2 += Delta;
	log_debug("[DBG] New k2: %f", g_fLensK2);
}

#ifdef DBG_VR

void IncreaseNoDrawBeforeIndex(int Delta) {
	g_iNoDrawBeforeIndex += Delta;
	log_debug("[DBG] NoDraw BeforeIdx, AfterIdx: %d, %d", g_iNoDrawBeforeIndex, g_iNoDrawAfterIndex);
}

void IncreaseNoDrawAfterIndex(int Delta) {
	g_iNoDrawAfterIndex += Delta;
	log_debug("[DBG] NoDraw BeforeIdx, AfterIdx: %d, %d", g_iNoDrawBeforeIndex, g_iNoDrawAfterIndex);
}

void IncreaseNoExecIndices(int DeltaBefore, int DeltaAfter) {
	g_iNoExecBeforeIndex += DeltaBefore;
	g_iNoExecAfterIndex += DeltaAfter;
	if (g_iNoExecBeforeIndex < -1)
		g_iNoExecBeforeIndex = -1;

	log_debug("[DBG] NoExec BeforeIdx, AfterIdx: %d, %d", g_iNoExecBeforeIndex, g_iNoExecAfterIndex);
}

void IncreaseSkipNonZBufferDrawIdx(int Delta) {
	g_iSkipNonZBufferDrawIdx += Delta;
	log_debug("[DBG] New g_iSkipNonZBufferDrawIdx: %d", g_iSkipNonZBufferDrawIdx);
}

void IncreaseNoDrawAfterHUD(int Delta) {
	g_iNoDrawAfterHUD += Delta;
	log_debug("[DBG] NoDrawAfterHUD: %d", g_iNoDrawAfterHUD);
}

#endif


//void IncreaseSkyBoxIndex(int Delta) {
//	g_iSkyBoxExecIndex += Delta;
//	log_debug("[DBG] New g_iSkyBoxExecIndex: %d", g_iSkyBoxExecIndex);
//}