#include "common.h"
#include "DynamicCockpit.h"
#include "globals.h"
#include "VRConfig.h"

/*********************************************************/
// DYNAMIC COCKPIT
char g_sCurrentCockpit[128] = { 0 };
DCHUDRegions g_DCHUDRegions;
DCElemSrcBoxes g_DCElemSrcBoxes;
dc_element g_DCElements[MAX_DC_SRC_ELEMENTS] = { 0 };
SubDCSrcBox g_DCSubRegions[MAX_DC_SUB_ELEMENTS];
Box g_speedBox;
Box g_chaffBox;
Box g_nameBox;
Box g_timeBox;
Box g_mslsBox[2], g_mslsBoxBoth, g_mslsBoxMis;
Box g_tgtNameBox, g_tgtShdBox, g_tgtHullBox, g_tgtSysBox;
Box g_tgtDistBox, g_tgtSubCmpBox, g_tgtCargoBox;

// DC Debug
Box  g_DCDebugBox       = { 0 };
bool g_bEnableDCDebug   = false;
int  g_iDCDebugSrcIndex = -1;
bool g_bDCDebugDisplayLabels = false;
std::string g_DCDebugLabel = "";

void DCResetSubRegions()
{
	log_debug("[DBG] [DC] Resetting DC Sub-regions");
	g_bRecomputeFontHeights = true;
	g_speedBox.Invalidate();
	g_chaffBox.Invalidate();
	g_nameBox.Invalidate();
	g_timeBox.Invalidate();
	g_mslsBox[0].Invalidate();
	g_mslsBox[1].Invalidate();
	g_mslsBoxBoth.Invalidate();
	g_mslsBoxMis.Invalidate();

	g_tgtShdBox.Invalidate();
	g_tgtHullBox.Invalidate();
	g_tgtSysBox.Invalidate();
	g_tgtDistBox.Invalidate();

	// Do not invalidate these fields. It will cause them to shink and text will look
	// quite stretched:
	//g_tgtNameBox.Invalidate();
	//g_tgtSubCmpBox.Invalidate();
	//g_tgtCargoBox.Invalidate();
	g_tgtNameBox   = g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_NAME_SRC_IDX].coords;
	g_tgtSubCmpBox = g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_SUBCMP_SRC_IDX].coords;
	g_tgtCargoBox  = g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_CARGO_SRC_IDX].coords;
}

float g_fCoverTextureBrightness = 1.0f;
float g_fDCBrightness = 1.0f;
int g_iNumDCElements = 0;
move_region_coords g_DCMoveRegions = { 0 };
bool g_bDCApplyEraseRegionCommands = false, g_bReRenderMissilesNCounterMeasures = false;
bool g_bDCEnabled = true, g_bDCEnableLaserBloom = true;
bool g_bGlobalDebugFlag = false, g_bInhibitCMDBracket = false;
bool g_bHUDVisibleOnStartup = false;
bool g_bCompensateFOVfor1920x1080 = true;
bool g_bDCWasClearedOnThisFrame = false;
int g_iHUDOffscreenCommandsRendered = 0;
bool g_bEdgeEffectApplied = false;
extern int g_WindowWidth, g_WindowHeight;
float4 g_DCTargetingColor, g_DCWireframeLuminance;
float4 g_DCTargetingIFFColors[6];
float4 g_DCTargetingFriend;
float4 g_DCTargetingFoe;
bool g_bGreenAndRedForIFFColorsOnly = false;
float g_DCWireframeContrast = 3.0f;
float g_fReticleScale = DEFAULT_RETICLE_SCALE;
bool g_bRenderLaserIonEnergyLevels = false, g_bRenderThrottle = false;
D2D1::ColorF g_DCLaserColor(0xFF0000, 0.35f), g_DCIonColor(0x0000FF, 0.35f), g_DCThrottleColor(0x0000FF, 1.0f);
extern Vector2 g_SubCMDBracket; // Populated in XwaDrawBracketHook for the sub-CMD bracket when the enhanced 2D renderer is on
// HOLOGRAMS
float g_fDCHologramFadeIn = 0.0f, g_fDCHologramFadeInIncr = 0.04f, g_fDCHologramTime = 0.0f;
bool g_bDCHologramsVisible = true, g_bDCHologramsVisiblePrev = true;
// DIEGETIC JOYSTICK
bool g_bDiegeticCockpitEnabled = true;

Vector2 g_TriangleCentroid;

const char* DC_TARGET_COMP_SRC_RESNAME = "dat,12000,1100,";
const char* DC_LEFT_SENSOR_SRC_RESNAME = "dat,12000,4500,";
const char* DC_LEFT_SENSOR_2_SRC_RESNAME = "dat,12000,300,";
const char* DC_RIGHT_SENSOR_SRC_RESNAME = "dat,12000,4600,";
const char* DC_RIGHT_SENSOR_2_SRC_RESNAME = "dat,12000,400,";
const char* DC_SHIELDS_SRC_RESNAME = "dat,12000,4300,";
const char* DC_SOLID_MSG_SRC_RESNAME = "dat,12000,100,";
const char* DC_BORDER_MSG_SRC_RESNAME = "dat,12000,200,";
const char* DC_LASER_BOX_SRC_RESNAME = "dat,12000,2300,";
const char* DC_ION_BOX_SRC_RESNAME = "dat,12000,2500,";
const char* DC_BEAM_BOX_SRC_RESNAME = "dat,12000,4400,";
const char* DC_TOP_LEFT_SRC_RESNAME = "dat,12000,2700,";
const char* DC_TOP_RIGHT_SRC_RESNAME = "dat,12000,2800,";

std::vector<const char*> g_HUDRegionNames = {
	"LEFT_SENSOR_REGION",		// 0
	"RIGHT_SENSOR_REGION",		// 1
	"SHIELDS_REGION",			// 2
	"BEAM_REGION",				// 3
	"TARGET_AND_LASERS_REGION",	// 4
	"LEFT_TEXT_BOX_REGION",		// 5
	"RIGHT_TEXT_BOX_REGION",	// 6
	"TOP_LEFT_REGION",			// 7
	"TOP_RIGHT_REGION",			// 8
	"TEXT_RADIOSYS_REGION",		// 9
	"TEXT_CMD_REGION",			// 10
};

std::vector<const char*> g_DCElemSrcNames = {
	"LEFT_SENSOR_SRC",			// 0
	"RIGHT_SENSOR_SRC",			// 1
	"LASER_RECHARGE_SRC",		// 2
	"SHIELD_RECHARGE_SRC",		// 3
	"ENGINE_POWER_SRC",			// 4
	"BEAM_RECHARGE_SRC",		// 5
	"SHIELDS_SRC",				// 6
	"BEAM_LEVEL_SRC"	,		// 7
	"TARGETING_COMPUTER_SRC",	// 8
	"QUAD_LASERS_LEFT_SRC",		// 9
	"QUAD_LASERS_RIGHT_SRC",	// 10
	"LEFT_TEXT_BOX_SRC",		// 11
	"RIGHT_TEXT_BOX_SRC",		// 12
	"SPEED_THROTTLE_SRC",		// 13
	"MISSILES_SRC",				// 14
	"NAME_TIME_SRC",			// 15
	"NUM_SHIPS_SRC",			// 16
	"QUAD_LASERS_BOTH_SRC",		// 17
	"DUAL_LASERS_L_SRC",		// 18
	"DUAL_LASERS_R_SRC",		// 19
	"DUAL_LASERS_BOTH_SRC",		// 20
	"B_WING_LASERS_SRC",		// 21
	"SIX_LASERS_BOTH_SRC",		// 22
	"SIX_LASERS_L_SRC",			// 23
	"SIX_LASERS_R_SRC",			// 24
	"SHIELDS_FRONT_SRC",		// 25
	"SHIELDS_BACK_SRC",			// 26
	"KW_TEXT_CMD_SRC",			// 27
	"KW_TEXT_TOP_SRC",			// 28
	"KW_TEXT_RADIOSYS_SRC",		// 29
	"TEXT_RADIO_SRC",			// 30
	"TEXT_SYSTEM_SRC",			// 31
	"TEXT_CMD_SRC",				// 32
	"TARGETED_OBJ_NAME_SRC",	// 33
	"TARGETED_OBJ_SHD_SRC",		// 34
	"TARGETED_OBJ_HULL_SRC",	// 35
	"TARGETED_OBJ_CARGO_SRC",	// 36
	"TARGETED_OBJ_SYS_SRC",		// 37
	"TARGETED_OBJ_DIST_SRC",	// 38
	"TARGETED_OBJ_SUBCMP_SRC",	// 39
	"EIGHT_LASERS_BOTH_SRC",	// 40
	"THROTTLE_BAR_SRC",			// 41
	"AUTOSIZE_MISSILES_L_SRC",  // 42
	"AUTOSIZE_MISSILES_R_SRC",  // 43
	"AUTOSIZE_SPEED_SRC",       // 44
	"AUTOSIZE_CHAFF_SRC",       // 45
	"AUTOSIZE_NAME_SRC",        // 46
	"AUTOSIZE_TIME_SRC",        // 47
	"AUTOSIZE_MISSILES_B_SRC",  // 48
	"AUTOSIZE_MISSILES_MIS_SRC", // 49
	"DUAL_LASER_ENERGY_BAR_L_SRC",  // 50
	"DUAL_LASER_ENERGY_BAR_R_SRC",  // 51
	"QUAD_LASER_ENERGY_BAR_LU_SRC", // 52
	"QUAD_LASER_ENERGY_BAR_LD_SRC", // 53
	"QUAD_LASER_ENERGY_BAR_RU_SRC", // 54
	"QUAD_LASER_ENERGY_BAR_RD_SRC", // 55
	"ALL_LASER_ENERGY_BAR_SRC",     // 56
};

bool DcIsLaserEnergySlot(int slot)
{
	return
		slot == DUAL_LASERS_L_DC_ELEM_SRC_IDX || slot == DUAL_LASERS_R_DC_ELEM_SRC_IDX || slot == DUAL_LASERS_BOTH_DC_ELEM_SRC_IDX ||

		slot == DUAL_LASER_ENERGY_BAR_L_SRC_IDX || slot == DUAL_LASER_ENERGY_BAR_R_SRC_IDX ||

		slot == QUAD_LASERS_L_DC_ELEM_SRC_IDX || slot == QUAD_LASERS_R_DC_ELEM_SRC_IDX || slot == QUAD_LASERS_BOTH_DC_ELEM_SRC_IDX ||

		slot == QUAD_LASER_ENERGY_BAR_LU_SRC_IDX || slot == QUAD_LASER_ENERGY_BAR_RU_SRC_IDX ||
		slot == QUAD_LASER_ENERGY_BAR_LD_SRC_IDX || slot == QUAD_LASER_ENERGY_BAR_RD_SRC_IDX ||

		slot == B_WING_LASERS_DC_ELEM_SRC_IDX || slot == EIGHT_LASERS_BOTH_SRC_IDX ||
		slot == SIX_LASERS_BOTH_DC_ELEM_SRC_IDX || slot == SIX_LASERS_L_DC_ELEM_SRC_IDX || slot == SIX_LASERS_R_DC_ELEM_SRC_IDX ||

		slot == ALL_LASER_ENERGY_BAR_SRC_IDX;
}

DcEnergyBarData g_DcEnergyBarData;

Box DcEnergyBarData::ReduceAndLabelBoxes()
{
	Box fullBox = { FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };
	const float midScreenX  = g_fCurInGameWidth  * 0.5f;
	const float midScreenY  = g_fCurInGameHeight * 0.5f;
	const uint32_t numBoxes = (uint32_t )src_boxes.size();

	float minBoxesY =  FLT_MAX;
	float maxBoxesY = -FLT_MAX;
	for (const Box& box : src_boxes)
	{
		minBoxesY = min(minBoxesY, box.y0);
		maxBoxesY = max(maxBoxesY, box.y1);
	}
	const float midBoxesY = (minBoxesY + maxBoxesY) / 2.0f;

	int dc_idx = -1;
	for (Box& box : src_boxes)
	{
		const float W = box.x1 - box.x0;
		const float H = box.y1 - box.y0;

		const float x = (box.x0 + box.x1) / 2.0f;
		const float y = (box.y0 + box.y1) / 2.0f;

		// Extend to the sides, but avoid the box containing the energy bar:
		box.x0 = x - (28.5f / 64.0f) * W;
		box.x1 = x + (28.5f / 64.0f) * W;

		// Make a small margin around the middle:
		box.y0 = y - (0.5f / 64.0f) * H;
		box.y1 = y + (0.5f / 64.0f) * H;

		fullBox.x0 = min(fullBox.x0, box.x0);
		fullBox.y0 = min(fullBox.y0, box.y0);

		fullBox.x1 = max(fullBox.x1, box.x1);
		fullBox.y1 = max(fullBox.y1, box.y1);

		switch (numBoxes)
		{
		case 2:
			if (x < midScreenX)
				dc_idx = DUAL_LASER_ENERGY_BAR_L_SRC_IDX;
			else
				dc_idx = DUAL_LASER_ENERGY_BAR_R_SRC_IDX;
			break;
		case 4:
			if (y < midBoxesY)
			{
				if (x < midScreenX)
					dc_idx = QUAD_LASER_ENERGY_BAR_LU_SRC_IDX;
				else
					dc_idx = QUAD_LASER_ENERGY_BAR_RU_SRC_IDX;
			}
			else
			{
				if (x < midScreenX)
					dc_idx = QUAD_LASER_ENERGY_BAR_LD_SRC_IDX;
				else
					dc_idx = QUAD_LASER_ENERGY_BAR_RD_SRC_IDX;
			}
			break;
		}

		if (dc_idx < 0)
		{
			log_debug("[DBG] [DC] ERROR: Could not compute box!");
			continue;
		}

		DCElemSrcBox* dc_box = &(g_DCElemSrcBoxes.src_boxes[dc_idx]);
		InGameToScreenCoords(box.x0, box.y0, &(dc_box->coords.x0), &(dc_box->coords.y0));
		InGameToScreenCoords(box.x1, box.y1, &(dc_box->coords.x1), &(dc_box->coords.y1));

		// Normalize to uv coords:
		dc_box->coords.x0 *= g_fCurScreenWidthRcp;
		dc_box->coords.y0 *= g_fCurScreenHeightRcp;
		dc_box->coords.x1 *= g_fCurScreenWidthRcp;
		dc_box->coords.y1 *= g_fCurScreenHeightRcp;
		dc_box->bComputed = true;
		log_debug("[DBG] [DC] %s computed (%0.1f, %0.1f)",
			g_DCElemSrcNames[dc_idx], x, y);
	}

	return fullBox;
}

void DcEnergyBarData::ComputeDCEnergyBoxes()
{
	Box fullBox = ReduceAndLabelBoxes();

	DCElemSrcBox* box = &(g_DCElemSrcBoxes.src_boxes[ALL_LASER_ENERGY_BAR_SRC_IDX]);
	InGameToScreenCoords(fullBox.x0, fullBox.y0, &(box->coords.x0), &(box->coords.y0));
	InGameToScreenCoords(fullBox.x1, fullBox.y1, &(box->coords.x1), &(box->coords.y1));

	// Normalize to uv coords:
	box->coords.x0 *= g_fCurScreenWidthRcp;
	box->coords.y0 *= g_fCurScreenHeightRcp;
	box->coords.x1 *= g_fCurScreenWidthRcp;
	box->coords.y1 *= g_fCurScreenHeightRcp;
	box->bComputed = true;
	log_debug("[DBG] [DC] LASER_ENERGY_BAR_SRC_IDX computed, num boxes: %d",
		(int)g_DcEnergyBarData.src_boxes.size());
	this->bComputed = true;
}

int HUDRegionNameToIndex(char* name) {
	if (name == NULL || name[0] == '\0')
		return -1;
	for (int i = 0; i < (int)g_HUDRegionNames.size(); i++)
		if (_stricmp(name, g_HUDRegionNames[i]) == 0)
			return i;
	return -1;
}

int DCSrcElemNameToIndex(char* name) {
	if (name == NULL || name[0] == '\0')
		return -1;
	for (int i = 0; i < (int)g_DCElemSrcNames.size(); i++)
		if (_stricmp(name, g_DCElemSrcNames[i]) == 0)
			return i;
	return -1;
}

/*
 * Loads a cover_texture_size row
 */
bool LoadDCCoverTextureSize(char* buf, float* width, float* height)
{
	int res = 0;
	char* c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f", width, height);
			if (res < 2) {
				log_debug("[DBG] [DC] Error (skipping), expected at least 2 elements in '%s'", c);
			}
		}
		catch (...) {
			log_debug("[DBG] [DC] Could not read 'width, height' from: %s", buf);
			return false;
		}
	}
	return true;
}

/*
 * Clears all the move_region commands
 */
//static inline void ClearDCMoveRegions() {
void ClearDCMoveRegions() {
	g_DCMoveRegions.numCoords = 0;
}

/*
 * Loads a move_region row into g_DCMoveRegions:
 * move_region = region, x0,y0,x1,y1
 */
bool LoadDCMoveRegion(char* buf)
{
	float x0, y0, x1, y1;
	int region_slot;
	int res = 0, idx = g_DCMoveRegions.numCoords;
	char* substr = NULL;
	char region_name[50];

	if (idx >= MAX_HUD_BOXES) {
		log_debug("[DBG] [DC] Too many move_region commands");
		return false;
	}

	substr = strchr(buf, '=');
	if (substr == NULL) {
		log_debug("[DBG] [DC] Missing '=' in '%s', skipping", buf);
		return false;
	}
	// Skip the equal sign:
	substr++;
	// Skip white space chars:
	while (*substr == ' ' || *substr == '\t')
		substr++;

	try {
		int len;

		region_slot = -1;
		region_name[0] = 0;
		len = ReadNameFromLine(substr, region_name);
		if (len == 0)
			return false;

		if (strstr(region_name, "REGION") == NULL) {
			log_debug("[DBG] [DC] ERROR: Invalid region name: [%s]", region_name);
			return false;
		}

		region_slot = HUDRegionNameToIndex(region_name);
		if (region_slot < 0) {
			log_debug("[DBG] [DC] ERROR: Could not find region named: [%s]", region_name);
			return false;
		}

		// Parse the rest of the parameters
		substr += len + 1;
		res = sscanf_s(substr, "%f, %f, %f, %f", &x0, &y0, &x1, &y1);
		if (res < 4) {
			log_debug("[DBG] [DC] ERROR (skipping), expected at least 5 elements in '%s'", substr);
		}
		else {
			g_DCMoveRegions.region_slot[idx] = region_slot;
			g_DCMoveRegions.dst[idx].x0 = x0;
			g_DCMoveRegions.dst[idx].y0 = y0;
			g_DCMoveRegions.dst[idx].x1 = x1;
			g_DCMoveRegions.dst[idx].y1 = y1;
			g_DCMoveRegions.numCoords++;

			log_debug("[DBG] [DC] move_region [%s=%d], (%0.3f, %0.3f)-(%0.3f, %0.3f)",
				region_name, region_slot, x0, y0, x1, y1);
		}
	}
	catch (...) {
		log_debug("[DBG] [DC] Could not read coords from: %s", buf);
		return false;
	}
	return true;
}

/*
 * Convert a cockpit name into a DC params file of the form:
 * DynamicCockpit\<CockpitName>.dc
 */
void CockpitNameToDCParamsFile(char* CockpitName, char* sFileName, int iFileNameSize) {
	snprintf(sFileName, iFileNameSize, "DynamicCockpit\\%s.dc", CockpitName);
}

/*
 * Convert a cockpit name into an AC params file of the form:
 * DynamicCockpit\<CockpitName>.ac
 */
void CockpitNameToACParamsFile(char* CockpitName, char* sFileName, int iFileNameSize) {
	snprintf(sFileName, iFileNameSize, "DynamicCockpit\\%s.ac", CockpitName);
}

/*
 * Load the DC params for an individual cockpit.
 * Resets g_DCElements (if we're not rendering in 3D), and the move regions.
 */
bool LoadIndividualDCParams(char* sFileName) {
	log_debug("[DBG] Loading Dynamic Cockpit params for [%s]...", sFileName);
	FILE* file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;
	float cover_tex_width = 1, cover_tex_height = 1;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading [%s]", error, sFileName);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Reset the dynamic cockpit vector if we're not rendering in 3D
	//if (!g_bRendering3D && g_DCElements.size() > 0) {
	//	log_debug("[DBG] [DC] Clearing g_DCElements");
	//	ClearDynCockpitVector(g_DCElements);
	//}
	//ClearDCMoveRegions();
	g_DCTargetingColor.w = 0.0f; // Reset the targeting mesh color
	// Do not re-render Missiles/Counters from first principles by default:
	g_bReRenderMissilesNCounterMeasures = false;
	// Do not render laser/ion energy levels by default:
	g_bRenderLaserIonEnergyLevels = false;
	// Do not render the throttle bar by default:
	g_bRenderThrottle = false;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (buf[0] == '[') {
				// This is a new DC element.
				dc_element dc_elem;
				strcpy_s(dc_elem.name, MAX_TEXTURE_NAME, buf + 1);
				// Get rid of the trailing ']'
				char* end = strstr(dc_elem.name, "]");
				if (end != NULL)
					*end = 0;
				// See if we have this DC element already
				lastDCElemSelected = isInVector(dc_elem.name, g_DCElements, g_iNumDCElements);
				//log_debug("[DBG] [DC] New dc_elem.name: [%s], idx: %d",
				//	dc_elem.name, lastDCElemSelected);
				if (lastDCElemSelected > -1) {
					g_DCElements[lastDCElemSelected].coords.numCoords = 0;
					g_DCElements[lastDCElemSelected].num_erase_slots = 0;
					log_debug("[DBG] [DC] Resetting coords of existing DC elem @ idx: %d", lastDCElemSelected);
				}
				else if (g_iNumDCElements < MAX_DC_SRC_ELEMENTS) {
					// Initialize this dc_elem:
					dc_elem.coverTextureName[0] = 0;
					// TODO: Replace the line below with the proper command now that the
					// coverTextures live inside DeviceResources
					//dc_elem.coverTexture = nullptr;
					dc_elem.coords = { 0 };
					dc_elem.num_erase_slots = 0;
					dc_elem.bActive = false;
					dc_elem.bNameHasBeenTested = false;
					dc_elem.bHologram = false;
					dc_elem.bNoisyHolo = false;
					dc_elem.bTransparent = false;
					dc_elem.bAlwaysVisible = false;
					//g_DCElements.push_back(dc_elem);
					g_DCElements[g_iNumDCElements] = dc_elem;
					//lastDCElemSelected = (int)g_DCElements.size() - 1;
					lastDCElemSelected = g_iNumDCElements;
					g_iNumDCElements++;
					//log_debug("[DBG] [DC] Added new dc_elem, count: %d", g_iNumDCElements);
				}
				else {
					if (g_iNumDCElements >= MAX_DC_SRC_ELEMENTS)
						log_debug("[DBG] [DC] ERROR: Max g_iNumDCElements: %d reached", g_iNumDCElements);
				}
			}
			else if (_stricmp(param, UV_COORDS_DCPARAM) == 0) {
				//if (g_DCElements.size() == 0) {
				if (g_iNumDCElements == 0) {
					log_debug("[DBG] [DC] ERROR. Line %d, g_DCElements is empty, cannot add %s", line, param, UV_COORDS_DCPARAM);
					continue;
				}
				if (lastDCElemSelected == -1) {
					log_debug("[DBG] [DC] ERROR. Line %d, %s without a corresponding texture section.", line, UV_COORDS_DCPARAM);
					continue;
				}
				LoadDCUVCoords(buf, cover_tex_width, cover_tex_height, &(g_DCElements[lastDCElemSelected].coords));
			}
			/*
			else if (_stricmp(param, MOVE_REGION_DCPARAM) == 0) {
				LoadDCMoveRegion(buf);
			}
			*/
			else if (_stricmp(param, ERASE_REGION_DCPARAM) == 0) {
				//if (g_DCElements.size() == 0) {
				if (g_iNumDCElements == 0) {
					log_debug("[DBG] [DC] ERROR. Line %d, g_DCElements is empty, cannot add %s", line, param, ERASE_REGION_DCPARAM);
					continue;
				}
				if (lastDCElemSelected == -1) {
					log_debug("[DBG] [DC] ERROR. Line %d, %s without a corresponding texture section.", line, ERASE_REGION_DCPARAM);
					continue;
				}
				// The name of the region must contain the word "REGION":
				if (strstr(svalue, "REGION") == NULL) {
					log_debug("[DBG] [DC] ERROR: Invalid region name: [%s]", svalue);
					continue;
				}

				int slot = HUDRegionNameToIndex(svalue);
				if (slot < 0) {
					log_debug("[DBG] [DC] ERROR: Unknown region: [%s]", svalue);
					continue;
				}

				if (slot < (int)g_DCHUDRegions.boxes.size()) {
					int next_idx = g_DCElements[lastDCElemSelected].num_erase_slots;
					g_DCElements[lastDCElemSelected].erase_slots[next_idx] = slot;
					g_DCElements[lastDCElemSelected].num_erase_slots++;
					//log_debug("[DBG] [DC] Added erase slot [%s] to DCElem %d", svalue,
					//	lastDCElemSelected);
				}
				else
					log_debug("[DBG] [DC] WARNING: erase_region = %d IGNORED: Not enough g_DCElemSrcBoxes", slot);
			}
			else if (_stricmp(param, COVER_TEX_NAME_DCPARAM) == 0) {
				if (lastDCElemSelected == -1) {
					log_debug("[DBG] [DC] ERROR. Line %d, %s without a corresponding texture section.", line, COVER_TEX_NAME_DCPARAM);
					continue;
				}
				strcpy_s(g_DCElements[lastDCElemSelected].coverTextureName, MAX_TEXTURE_NAME, svalue);
			}
			else if (_stricmp(param, COVER_TEX_SIZE_DCPARAM) == 0) {
				LoadDCCoverTextureSize(buf, &cover_tex_width, &cover_tex_height);
			}
			else if (_stricmp(param, "xwahacker_fov") == 0) {
				log_debug("[DBG] [FOV] [DC] XWA HACKER FOV: %0.3f", fValue);
				g_fCurrentShipFocalLength = SetCurrentShipFOV(fValue, false, g_bCompensateFOVfor1920x1080);
				log_debug("[DBG] [FOV] [DC] XWA HACKER FOCAL LENGTH: %0.3f", g_fCurrentShipFocalLength);
				g_CurrentFOVType = g_bEnableVR ? GLOBAL_FOV : XWAHACKER_FOV;
				// Force the new FOV to be applied
				g_bCustomFOVApplied = false;
			}
			else if (_stricmp(param, "xwahacker_large_fov") == 0) {
				log_debug("[DBG] [FOV] [DC] XWA HACKER LARGE FOV: %0.3f", fValue);
				// SetCurrentShipFOV can overwrite g_fCurrentShipFocalLength and g_fCurrentShipLargeFocalLength because
				// we need that when increasing the FOV using hotkeys. However, we must not overwrite those values here
				// because the current FOV at this point is XWAHACKER, so we end up writing the same value to both FOVs
				g_fCurrentShipLargeFocalLength = SetCurrentShipFOV(fValue, false, g_bCompensateFOVfor1920x1080);
				log_debug("[DBG] [FOV] [DC] XWA HACKER LARGE FOCAL LENGTH: %0.3f", g_fCurrentShipLargeFocalLength);
				g_CurrentFOVType = g_bEnableVR ? GLOBAL_FOV : XWAHACKER_FOV; // This is *NOT* an error, I want the default to be XWAHACKER_FOV
				// Force the new FOV to be applied
				g_bCustomFOVApplied = false;
			}
			else if (_stricmp(param, "wireframe_mesh_color") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingColor.x = x;
					g_DCTargetingColor.y = y;
					g_DCTargetingColor.z = z;
					g_DCTargetingColor.w = 1.0f;
				}
			}
			else if (_stricmp(param, "hologram") == 0) {
				g_DCElements[lastDCElemSelected].bHologram = (bool)fValue;
			}
			else if (_stricmp(param, "noisy_hologram") == 0) {
				g_DCElements[lastDCElemSelected].bNoisyHolo = (bool)fValue;
				log_debug("[DBG] noisy hologram");
			}
			else if (_stricmp(param, "fix_missles_countermeasures_text") == 0 ||
				     _stricmp(param, "fix_missiles_countermeasures_text") == 0) {
				g_bReRenderMissilesNCounterMeasures = (bool)fValue;
			}
			else if (_stricmp(param, "transparent") == 0) {
				g_DCElements[lastDCElemSelected].bTransparent = (bool)fValue;
			}
			else if (_stricmp(param, "always_visible") == 0) {
				g_DCElements[lastDCElemSelected].bAlwaysVisible = (bool)fValue;
			}
			else if (_stricmp(param, "render_laser_ions") == 0) {
				g_bRenderLaserIonEnergyLevels = (bool)fValue;
			}
			else if (_stricmp(param, "laser_energy_color") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCLaserColor.r = x;
					g_DCLaserColor.g = y;
					g_DCLaserColor.b = z;
					g_DCLaserColor.a = 0.35f;
				}
			}
			else if (_stricmp(param, "ion_energy_color") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCIonColor.r = x;
					g_DCIonColor.g = y;
					g_DCIonColor.b = z;
					g_DCIonColor.a = 0.35f;
				}
			}
			else if (_stricmp(param, "render_throttle") == 0) {
				g_bRenderThrottle = (bool)fValue;
			}
			else if (_stricmp(param, "throttle_color") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCThrottleColor.r = x;
					g_DCThrottleColor.g = y;
					g_DCThrottleColor.b = z;
					g_DCThrottleColor.a = 1.0f;
				}
			}
		}
	}
	fclose(file);
	return true;
}

/*
 * Saves the current FOV to the current dc file -- if it exists.
 * Depending on the current g_CurrentFOVType, it will write "xwahacker_fov" or
 * "xwahacker_large_fov".
 */
bool UpdateXWAHackerFOV()
{
	char sFileName[256], * sTempFileName = "./TempDCFile.dc";
	FILE* in_file, * out_file;
	int error = 0, line = 0;
	char buf[256];
	char* FOVname = NULL;

	switch (g_CurrentFOVType) {
	case GLOBAL_FOV:
		return false;
	case XWAHACKER_FOV:
		FOVname = "xwahacker_fov";
		break;
	case XWAHACKER_LARGE_FOV:
		FOVname = "xwahacker_large_fov";
		break;
	}
	if (FOVname == NULL) {
		log_debug("[DBG] [FOV] FOVname is NULL! Aborting UpdateXWAHackerFOV");
		return false;
	}

	if (strlen(g_sCurrentCockpit) <= 0) {
		log_debug("[DBG] [DC] No DC-enabled cockpit has been loaded, will not write current FOV");
		return false;
	}

	snprintf(sFileName, 256, ".\\DynamicCockpit\\%s.dc", g_sCurrentCockpit);
	log_debug("[DBG] Saving current FOV to Dynamic Cockpit params file [%s]...", sFileName);
	// Open sFileName for reading
	try {
		error = fopen_s(&in_file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not read [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when reading [%s]", error, sFileName);
		return false;
	}

	// Create sTempFileName
	try {
		error = fopen_s(&out_file, sTempFileName, "wt");
	}
	catch (...) {
		log_debug("[DBG] Could not create temporary file: [%s]", sTempFileName);
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when creating [%s]", error, sTempFileName);
		return false;
	}

	bool bFOVWritten = false;
	float focal_length = *g_fRawFOVDist;
	float FOV = atan2(g_fCurInGameHeight, focal_length) * 2.0f;
	// Convert radians to degrees:
	FOV *= 180.0f / 3.141592f;
	log_debug("[DBG] [FOV] FOVType: %d, FOV that will be saved: %s = %0.3f", g_CurrentFOVType, FOVname, FOV);
	while (fgets(buf, 256, in_file) != NULL) {
		line++;
		// Commented lines are automatically pass-through
		if (buf[0] == ';') {
			fprintf(out_file, buf);
			continue;
		}

		if (strstr(buf, FOVname) != NULL) {
			fprintf(out_file, "%s = %0.3f\n", FOVname, FOV);
			bFOVWritten = true;
		}
		else
			fprintf(out_file, buf);
	}

	// This DC file may not have the "xwahacker_fov" line, so let's add it:
	if (!bFOVWritten)
		fprintf(out_file, "\n%s = %0.3f\n", FOVname, FOV);

	fclose(out_file);
	fclose(in_file);

	// Swap the files
	remove(sFileName);
	rename(sTempFileName, sFileName);
	return true;
}

/*
 * bOverwriteCurrentShipFOV should be disabled when reading the DC file, so that
 * we don't overwrite both xwahacker FOVs with the same value.
 * bCompensateFOVfor1920x1080 should be true only when reading the DC file: compensating
 * every time the FOV is adjusted has some adverse effects when the in-game resolution is
 * lower than 1920x1080
 */
float SetCurrentShipFOV(float FOV, bool bOverwriteCurrentShipFOV, bool bCompensateFOVfor1920x1080)
{
	float FocalLength;
	// Prevent nonsensical values:
	if (FOV < 15.0f) FOV = 15.0f;
	if (FOV > 170.0f) FOV = 170.0f;
	//DisplayTimedMessageV(3, 1, "FOV: %0.1f", FOV);
	// Convert to radians
	FOV = FOV * 3.141592f / 180.0f;
	// This formula matches what Jeremy posted:
	FocalLength = g_fCurInGameHeight / tan(FOV / 2.0f);
	if (bCompensateFOVfor1920x1080) {
		// Compute the focal length that would be applied in 1920x1080
		float DFocalLength = 1080.0f / tan(FOV / 2.0f);
		// Compute the real HFOV and desired HFOV:
		float HFOV = 2.0f * atan2(0.5f * g_fCurInGameWidth, FocalLength);
		float DHFOV = 2.0f * atan2(0.5f * 1920.0f, DFocalLength);
		log_debug("[DBG] [FOV] [DC] HFOV: %0.3f, DHFOV: %0.3f", HFOV / DEG2RAD, DHFOV / DEG2RAD);
		// If the actual HFOV is lower than the desired HFOV, then we need to adjust:
		if (HFOV < DHFOV) {
			log_debug("[DBG] [FOV] [DC] ADJUSTING FOV. Original regular focal length: %0.3f", FocalLength);
			FocalLength = 0.5f * g_fCurInGameWidth / tan(0.5f * DHFOV);
			log_debug("[DBG] [FOV] [DC] ADJUSTING FOV. ADJUSTED regular focal length: %0.3f", FocalLength);
		}
	}

	if (bOverwriteCurrentShipFOV) {
		switch (g_CurrentFOVType) {
		case XWAHACKER_FOV:
			g_fCurrentShipFocalLength = FocalLength;
			break;
		case XWAHACKER_LARGE_FOV:
			g_fCurrentShipLargeFocalLength = FocalLength;
			break;
		}
	}
	return FocalLength;
}

/*
 * Loads a uv_coords row:
 * uv_coords = src-slot, x0,y0,x1,y1, [Hex-Color]
 */
bool LoadDCUVCoords(char* buf, float width, float height, uv_src_dst_coords* coords)
{
	float x0, y0, x1, y1, intensity;
	int src_slot;
	uint32_t uColor, hColor, wColor, uBitField, text_layer, obj_layer, bloomOn;
	int res = 0, idx = coords->numCoords;
	char* substr = NULL;
	char slot_name[50];

	if (idx >= MAX_DC_COORDS_PER_TEXTURE) {
		log_debug("[DBG] [DC] Too many coords already loaded");
		return false;
	}

	substr = strchr(buf, '=');
	if (substr == NULL) {
		log_debug("[DBG] [DC] Missing '=' in '%s', skipping", buf);
		return false;
	}
	// Skip the equal sign:
	substr++;
	// Skip white space chars:
	while (*substr == ' ' || *substr == '\t')
		substr++;

	try {
		int len;
		uColor = 0x121233;
		hColor = 0x0;
		wColor = 0x0;
		obj_layer = 1;
		text_layer = 1;
		intensity = 1.0f;
		bloomOn = 0;
		uBitField = (0x00) /* intensity: 1 */ | (0x04) /* Bit 2 enables text */ | (0x08) /* Bit 3 enables objects */ |
					(0x00); /* Bit 4 enables Bloom */

		src_slot = -1;
		slot_name[0] = 0;
		len = ReadNameFromLine(substr, slot_name);
		if (len == 0)
			return false;

		if (strstr(slot_name, "SRC") == NULL) {
			log_debug("[DBG] [DC] ERROR: Invalid source slot name: [%s]", slot_name);
			return false;
		}

		src_slot = DCSrcElemNameToIndex(slot_name);
		if (src_slot < 0) {
			log_debug("[DBG] [DC] ERROR: Could not find slot named: [%s]", slot_name);
			return false;
		}

		// Parse the rest of the parameters
		substr += len + 1;
		// uv_coords = SRC, x0, y0, x1, y1; bgColor; Intensity; TextEnable; ObjEnable; hgColor; whColor; bloomOn
		// TextEnable -- Display the text layer -- you'll need to enable this for the synthetic DC elems.
		// ObjEnable -- Display the object layer
		// hgColor -- Highlight Color: this is the color used when the aiming reticle is highlighted
		// whColor -- Warhead Lock Color: this is the color used warhead lock is solid
		// bloomOn -- Enable bloom in the DC element. Activated by brightness
		res = sscanf_s(substr, "%f, %f, %f, %f; 0x%x; %f; %d; %d; 0x%x; 0x%x; %d",
						&x0, &y0, &x1, &y1, &uColor, // 5 elements
						&intensity, &text_layer, &obj_layer, // 8 elements
						&hColor, &wColor, &bloomOn); // 11 elements
		//log_debug("[DBG] [DC] res: %d, slot_name: %s", res, slot_name);
		if (res < 4) {
			log_debug("[DBG] [DC] ERROR (skipping), expected at least 4 elements in '%s'", substr);
		}
		else {
			coords->src_slot[idx] = src_slot;
			coords->dst[idx].x0 = x0 / width;
			coords->dst[idx].y0 = y0 / height;
			coords->dst[idx].x1 = x1 / width;
			coords->dst[idx].y1 = y1 / height;
			// Process the custom intensity
			if (res >= 6)
			{
				uint32_t temp;
				// bits 0-1: An integer in the range 0..3 that specifies the intensity.
				if (intensity < 0.0f) intensity = 0.0f;
				temp = (uint32_t)(intensity)-1;
				temp = (temp > 3) ? 3 : temp;
				uBitField |= temp;
			}
			// Process the text layer enable/disable
			if (res >= 7)
			{
				// bit 2 enables the text layer
				if (text_layer)
					uBitField |= 0x04;
				else
					uBitField &= 0xFB;
			}
			// Process the object layer enable/disable
			if (res >= 8)
			{
				// bit 3 enables the object layer
				if (obj_layer)
					uBitField |= 0x08;
				else
					uBitField &= 0xF7; // Turn off bit 3
			}
			// Process the highlight color
			if (res < 9)
			{
				hColor = uColor;
			}
			// Process the highlight color
			if (res < 10)
			{
				wColor = uColor;
			}
			// Process the bloom enable/disable
			if (res >= 11) {
				// bit 4 enables bloom
				if (bloomOn)
					uBitField |= 0x10;
				else
					uBitField &= 0xE7; // Turn off bit 4
			}
			// Store the regular and highlight colors
			coords->uBGColor[idx] = uColor | (uBitField << 24);
			coords->uHGColor[idx] = hColor | (uBitField << 24); // The alpha channel in hColor is not used
			coords->uWHColor[idx] = wColor | (uBitField << 24); // The alpha channel in wColor is not used
			coords->numCoords++;
			//log_debug("[DBG] uColor: 0x%x, hColor: 0x%x, [%s]",
			//	coords->uBGColor[idx], coords->uHGColor[idx], buf);
			//log_debug("[DBG] [DC] src_slot: %d, (%0.3f, %0.3f)-(%0.3f, %0.3f)",
			//	src_slot, x0 / width, y0 / height, x1 / width, y1 / height);
		}
	}
	catch (...) {
		log_debug("[DBG] [DC] Could not read uv coords from: %s", buf);
		return false;
	}
	return true;
}

/*
 * Loads a "source_def" or "erase_def" line from the global coordinates file
 */
bool LoadDCGlobalUVCoords(char* buf, Box* coords)
{
	float width, height;
	float x0, y0, x1, y1;
	int res = 0;
	char* c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f, %f, %f, %f",
				&width, &height, &x0, &y0, &x1, &y1);
			if (res < 6) {
				log_debug("[DBG] [DC] ERROR (skipping), expected at least 6 elements in '%s'", c);
				return false;
			}
			else {
				coords->x0 = x0 / width;
				coords->y0 = y0 / height;
				coords->x1 = x1 / width;
				coords->y1 = y1 / height;
			}
		}
		catch (...) {
			log_debug("[DBG] [DC] Could not read uv coords from: %s", buf);
			return false;
		}
	}
	return true;
}

/*
 * Loads a "screen_def" or "erase_screen_def" line from the global coordinates file. These
 * lines contain direct uv coords relative to the in-game screen. These coords are converted
 * to screen uv coords and returned.
 */
bool LoadDCScreenUVCoords(char* buf, Box* coords)
{
	float x0, y0, x1, y1;
	int res = 0;
	char* c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f, %f",
				&x0, &y0, &x1, &y1);
			if (res < 4) {
				log_debug("[DBG] [DC] ERROR (skipping), expected at least 4 elements in '%s'", c);
				return false;
			}
			else {
				coords->x0 = x0;
				coords->y0 = y0;
				coords->x1 = x1;
				coords->y1 = y1;
			}
		}
		catch (...) {
			log_debug("[DBG] [DC] Could not read uv coords from: %s", buf);
			return false;
		}
	}
	return true;
}

DCHUDRegions::DCHUDRegions() {
	Clear();
	//log_debug("[DBG] [DC] Adding g_HUDRegionNames.size(): %d", g_HUDRegionNames.size());
	for (int i = 0; i < MAX_HUD_BOXES; i++) {
		DCHUDRegion box = { 0 };
		box.bLimitsComputed = false;
		boxes.push_back(box);
	}
}

DCElemSrcBoxes::DCElemSrcBoxes() {
	Clear();
	//log_debug("[DBG] [DC] Adding g_DCElemSrcNames.size(): %d", g_DCElemSrcNames.size());
	for (int i = 0; i < MAX_DC_SRC_ELEMENTS; i++) {
		DCElemSrcBox src_box;
		src_box.bComputed = false;
		src_boxes.push_back(src_box);
	}
}

/*
 * Load dynamic_cockpit_internal_areas.cfg
 * This function will not grow g_DCElemSrcBoxes and it expects it to be populated
 * already.
 */
bool LoadDCInternalCoordinates() {

	log_debug("[DBG] [DC] Loading Internal Dynamic Cockpit coordinates...");
	FILE* file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;

	try {
		error = fopen_s(&file, "./DynamicCockpit/dynamic_cockpit_internal_areas.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] [DC] Could not load DynamicCockpit/dynamic_cockpit_internal_areas.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] [DC] ERROR %d when loading DynamicCockpit/dynamic_cockpit_internal_areas.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	unsigned int erase_slot = 0, source_slot = 0;
	float value = 0.0f;

	// TODO: Reset the global source and erase coordinates
	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			value = (float)atof(svalue);

			if (_stricmp(param, "source_def") == 0) {
				if (source_slot >= g_DCElemSrcBoxes.src_boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", source_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCGlobalUVCoords(buf, &box))
				{
					// HACK ALERT: The coords for the following areas don't actually cover the strings
					//             very well. I could either distribute a new .cfg file to fix this, but
					//             honestly, who is even using these? (I would've received complaints by now).
					//             So, I'm just going to override the areas with coords that actually work.
					if (source_slot == TARGETED_OBJ_NAME_SRC_IDX)
					{
						box.x0 =  38.0f / 256.0f; box.y0 = 15.0f / 128.0f;
						box.x1 = 218.0f / 256.0f; box.y1 = 28.0f / 128.0f;
					}
					else if (source_slot == TARGETED_OBJ_CARGO_SRC_IDX)
					{
						box.x0 =  14.0f / 256.0f; box.y0 = 110.0f / 128.0f;
						box.x1 = 100.0f / 256.0f; box.y1 = 128.0f / 128.0f;
					}
					else if (source_slot == SHIELDS_FRONT_DC_ELEM_SRC_IDX)
					{
						box.x0 = 34.0f / 128.0f; box.y0 = 13.0f / 128.0f;
						box.x1 = 63.0f / 128.0f; box.y1 = 25.0f / 128.0f;
					}
					else if (source_slot == MISSILES_DC_ELEM_SRC_IDX)
					{
						box.x0 = 200.0f / 256.0f; box.y0 =  0.0f / 32.0f;
						box.x1 = 256.0f / 256.0f; box.y1 = 25.0f / 32.0f;
					}
					g_DCElemSrcBoxes.src_boxes[source_slot].uv_coords = box;
				}
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				source_slot++;
			}
			else if (_stricmp(param, "screen_def") == 0) {
				if (source_slot >= g_DCElemSrcBoxes.src_boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", source_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCScreenUVCoords(buf, &box))
					g_DCElemSrcBoxes.src_boxes[source_slot].uv_coords = box;
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				source_slot++;
			}
			else if (_stricmp(param, "erase_def") == 0) {
				if (erase_slot >= g_DCHUDRegions.boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", erase_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCGlobalUVCoords(buf, &box)) {
					g_DCHUDRegions.boxes[erase_slot].uv_erase_coords = box;
					g_DCHUDRegions.boxes[erase_slot].bLimitsComputed = false; // Force a recompute of the limits
				}
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				//log_debug("[DBG] [DC] Region %d = [%s] loaded", erase_slot, g_HUDRegionNames[erase_slot]);
				erase_slot++;
			}
			else if (_stricmp(param, "erase_screen_def") == 0) {
				if (erase_slot >= g_DCHUDRegions.boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", erase_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCScreenUVCoords(buf, &box)) {
					g_DCHUDRegions.boxes[erase_slot].uv_erase_coords = box;
					g_DCHUDRegions.boxes[erase_slot].bLimitsComputed = false; // Force a recompute of the limits
				}
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				//log_debug("[DBG] [DC] Region %d = [%s] loaded", erase_slot, g_HUDRegionNames[erase_slot]);
				erase_slot++;
			}
		}
	}
	fclose(file);
	return true;
}

/*
 * Reads the name-slot from a string of the form:
 * name, x0, y0, x1, y1
 * returns the number of chars read
 */
int ReadNameFromLine(char* buf, char* name)
{
	int res, len;

	res = sscanf_s(buf, "%s", name, 50);
	if (res < 1) {
		log_debug("[DBG] [DC] ERROR: Expected a slot name in '%s'", buf);
		return 0;
	}

	len = strlen(name);
	if (len <= 0) {
		log_debug("[DBG] [DC] ERROR: Empty slot name in '%s'", buf);
		return 0;
	}

	// Remove the trailing semi-colon from the name:
	if (name[len - 1] == ';' || name[len - 1] == ',')
		name[len - 1] = '\0';
	return len;
}

int isInVector(const char* name, dc_element* dc_elements, int num_elems) {
	for (int i = 0; i < num_elems; i++) {
		if (stristr(name, dc_elements[i].name) != NULL)
			return i;
	}
	return -1;
}