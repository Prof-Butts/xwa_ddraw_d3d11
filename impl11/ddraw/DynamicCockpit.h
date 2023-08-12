#pragma once
#include <vector>
#include "D2D1.h"
#include "..\shaders\shader_common.h"
#include "EffectsCommon.h"

/*

HOW TO ADD NEW DC SOURCE ELEMENTS:

LoadDCInternalCoordinates loads the source areas into g_DCElemSrcBoxes.
g_DCElemSrcBoxes must be pre-populated.

1. Add new constant indices in DynamicCockpit.h.
   Look for MAX_DC_SRC_ELEMENTS and increase it. Then add the new *_DC_ELEM_SRC_IDX
   constants above it.

2. Add the new slots in Dynamic_Cockpit_Internal_Areas.cfg.
   Select a suitable *_HUD_BOX_IDX entry, find the corresponding image in HUD.dat and use
   a program like Photoshop to write down coordinates in this image. Then add an entry
   in Dynamic_Cockpit_Internal_Areas.cfg with the coordinates you wish to capture.
   Each entry looks like this:

	source_def = width,height, x0,y0, x1,y1

   Remember that you can use negative numbers for (x0,y0) and (x1,y1)

3. Add the code that captures the new slots in Direct3DDevice.cpp.
   Find the HUD_BOX_IDX entry in the Execute() method. The code should look like
   this:

   if (!g_DCHUDRegions.boxes[SHIELDS_HUD_BOX_IDX].bLimitsComputed)
   ...

   Add the code to capture the new slot(s) in the case where it belongs. There
   are plenty of examples in this area, just take a look at other cases.

4. Go to g_DCElemSrcNames in DynamicCockpit.cpp and add the labels that will
   be used for the new slots in the DC files. This is the text that cockpit
   authors will use to load the element.

   MAKE SURE YOU PLACE THE NEW LABELS ON THE SAME INDEX USED FOR THE NEW *_DC_ELEM_SRC_IDX


HOW TO ADD NEW ERASE REGION COMMANDS:

1. Go to DeviceResources.h and add the new region indices.
   Add new *_HUD_BOX_IDX indices and make sure MAX_HUD_BOXES has the total
   number of boxes.

2. Add the regions in Dynamic_Cockpit_areas.cfg. Make sure the indices match with the
   constants added in step 1.

3. Add the new region names in g_HUDRegionNames, in DeviceResources.cpp.
   Make sure the indices match. These names will be used in DC files to erase the
   new regions.

4. Add the code that computes the erase_region in Direct3DDevice.cpp.
   Look for the calls to ComputeCoordsFromUV()

*/

// DYNAMIC COCKPIT
typedef struct Box_struct {
	float x0, y0, x1, y1;
} Box;

// Also found in the Floating_GUI_RESNAME list:
extern const char* DC_TARGET_COMP_SRC_RESNAME;
extern const char* DC_LEFT_SENSOR_SRC_RESNAME;
extern const char* DC_LEFT_SENSOR_2_SRC_RESNAME;
extern const char* DC_RIGHT_SENSOR_SRC_RESNAME;
extern const char* DC_RIGHT_SENSOR_2_SRC_RESNAME;
extern const char* DC_SHIELDS_SRC_RESNAME;
extern const char* DC_SOLID_MSG_SRC_RESNAME;
extern const char* DC_BORDER_MSG_SRC_RESNAME;
extern const char* DC_LASER_BOX_SRC_RESNAME;
extern const char* DC_ION_BOX_SRC_RESNAME;
extern const char* DC_BEAM_BOX_SRC_RESNAME;
extern const char* DC_TOP_LEFT_SRC_RESNAME;
extern const char* DC_TOP_RIGHT_SRC_RESNAME;

const int SKIRMISH_MISSION_INDEX = 0;
const int PILOT_PROVING_GROUNDS_MISSION_INDEX = 6;
// Use the following with `const auto missionIndexLoaded = (int *)0x9F5E74;` to detect the DSII tunnel run mission.
const int DEATH_STAR_MISSION_INDEX = 52;

// Region names. Used in the erase_region and move_region commands
const int LEFT_RADAR_HUD_BOX_IDX = 0;
const int RIGHT_RADAR_HUD_BOX_IDX = 1;
const int SHIELDS_HUD_BOX_IDX = 2;
const int BEAM_HUD_BOX_IDX = 3;
const int TARGET_HUD_BOX_IDX = 4;
const int LEFT_MSG_HUD_BOX_IDX = 5;
const int RIGHT_MSG_HUD_BOX_IDX = 6;
const int TOP_LEFT_HUD_BOX_IDX = 7;
const int TOP_RIGHT_HUD_BOX_IDX = 8;
const int TEXT_RADIOSYS_HUD_BOX_IDX = 9;
const int TEXT_CMD_HUD_BOX_IDX = 10;
const int MAX_HUD_BOXES = 11;
extern std::vector<const char*>g_HUDRegionNames;
// Convert a string into a *_HUD_BOX_IDX constant
int HUDRegionNameToIndex(char* name);

// Also found in the Floating_GUI_RESNAME list:
extern const char* DC_TARGET_COMP_SRC_RESNAME;
extern const char* DC_LEFT_SENSOR_SRC_RESNAME;
extern const char* DC_LEFT_SENSOR_2_SRC_RESNAME;
extern const char* DC_RIGHT_SENSOR_SRC_RESNAME;
extern const char* DC_RIGHT_SENSOR_2_SRC_RESNAME;
extern const char* DC_SHIELDS_SRC_RESNAME;
extern const char* DC_SOLID_MSG_SRC_RESNAME;
extern const char* DC_BORDER_MSG_SRC_RESNAME;
extern const char* DC_LASER_BOX_SRC_RESNAME;
extern const char* DC_ION_BOX_SRC_RESNAME;
extern const char* DC_BEAM_BOX_SRC_RESNAME;
extern const char* DC_TOP_LEFT_SRC_RESNAME;
extern const char* DC_TOP_RIGHT_SRC_RESNAME;

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

typedef struct dc_element_struct {
	uv_src_dst_coords coords;
	int erase_slots[MAX_DC_COORDS_PER_TEXTURE];
	int num_erase_slots;
	char name[MAX_TEXTURE_NAME];
	char coverTextureName[MAX_TEXTURE_NAME];
	bool bActive, bNameHasBeenTested, bHologram, bNoisyHolo;
	// If set, the surface where the DC element is displayed will be removed, making it transparent.
	// Use this for floating text elements, for instance
	bool bTransparent;
	// bTransparent layers are completely removed when D is pressed to show the regular HUD. Set the
	// following flag to keep transparent layers visible even when D is pressed.
	bool bAlwaysVisible;
} dc_element;

typedef struct move_region_coords_struct {
	int region_slot[MAX_HUD_BOXES];
	uvfloat4 dst[MAX_HUD_BOXES];
	int numCoords;
} move_region_coords;

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
const int EIGHT_LASERS_BOTH_SRC_IDX = 40;
const int THROTTLE_BAR_DC_SRC_IDX = 41;
const int MAX_DC_SRC_ELEMENTS = 42;
extern std::vector<const char*>g_DCElemSrcNames;
// Convert a string into a *_DC_ELEM_SRC_IDX constant
int DCSrcElemNameToIndex(char* name);

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

extern bool g_bRenderLaserIonEnergyLevels; // If set, the Laser/Ion energy levels will be rendered from XWA's heap data
extern bool g_bRenderThrottle; // If set, render the throttle as a vertical bar next to the shields
extern D2D1::ColorF g_DCLaserColor, g_DCIonColor, g_DCThrottleColor;
//float g_fReticleOfsX = 0.0f;
//float g_fReticleOfsY = 0.0f;
//extern bool g_bInhibitCMDBracket; // Used in XwaDrawBracketHook
//extern float g_fXWAScale;

extern DCPixelShaderCBuffer g_DCPSCBuffer;
// g_DCElements is used when loading textures to load the cover texture.
extern dc_element g_DCElements[MAX_DC_SRC_ELEMENTS];
extern int g_iNumDCElements;
extern bool g_bDynCockpitEnabled, g_bReshadeEnabled;
extern DCHUDRegions g_DCHUDRegions;
extern move_region_coords g_DCMoveRegions;
extern DCElemSrcBoxes g_DCElemSrcBoxes;
extern float g_fCoverTextureBrightness;
extern float g_fDCBrightness;
extern move_region_coords g_DCMoveRegions;
extern char g_sCurrentCockpit[128];
extern bool g_bDCManualActivate, g_bDCApplyEraseRegionCommands, g_bReRenderMissilesNCounterMeasures;
extern bool g_bGlobalDebugFlag, g_bInhibitCMDBracket;
extern bool g_bHUDVisibleOnStartup;
extern bool g_bCompensateFOVfor1920x1080;
extern bool g_bDCWasClearedOnThisFrame;
extern int g_iHUDOffscreenCommandsRendered;
extern bool g_bEdgeEffectApplied;
extern int g_WindowWidth, g_WindowHeight;
extern float4 g_DCTargetingColor, g_DCWireframeLuminance;
extern float4 g_DCTargetingIFFColors[6];
extern float4 g_DCTargetingFriend;
extern float4 g_DCTargetingFoe;
extern bool g_bGreenAndRedForIFFColorsOnly;
extern float g_DCWireframeContrast;
extern float g_fReticleScale;
extern Vector2 g_SubCMDBracket; // Populated in XwaDrawBracketHook for the sub-CMD bracket when the enhanced 2D renderer is on
// HOLOGRAMS
extern float g_fDCHologramFadeIn, g_fDCHologramFadeInIncr, g_fDCHologramTime;
extern bool g_bDCHologramsVisible, g_bDCHologramsVisiblePrev, g_bDCTechHolosVisible;
// DIEGETIC JOYSTICK
extern bool g_bDiegeticCockpitEnabled;



bool LoadIndividualDCParams(char* sFileName);
bool LoadDCCoverTextureSize(char* buf, float* width, float* height);
void ClearDCMoveRegions();
bool LoadDCMoveRegion(char* buf);
void CockpitNameToDCParamsFile(char* CockpitName, char* sFileName, int iFileNameSize);
bool LoadDCInternalCoordinates();
float SetCurrentShipFOV(float FOV, bool OverwriteCurrentShipFOV, bool bCompensateFOVfor1920x1080);
bool LoadDCUVCoords(char* buf, float width, float height, uv_src_dst_coords* coords);
int ReadNameFromLine(char* buf, char* name);
int isInVector(char* name, dc_element* dc_elements, int num_elems);