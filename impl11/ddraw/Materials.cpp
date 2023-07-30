#include "common.h"
#include "globals.h"
#include "Materials.h"
#include "utils.h"
#include <string.h>
#include <string>
#include <filesystem>
#include "Vectors.h"
#include "ShadowMapping.h"
#include "DeviceResources.h"

namespace fs = std::filesystem;

bool g_bReloadMaterialsEnabled = false;
Material g_DefaultGlobalMaterial;
GlobalGameEvent g_GameEvent, g_PrevGameEvent;
// Maps an ObjectId-MaterialId to its InstanceEvent
extern std::map<uint64_t, InstanceEvent> g_objectIdToInstanceEvent;
// Maps an ObjectId-MaterialId to its FixedInstanceData
extern std::map<uint64_t, FixedInstanceData> g_fixedInstanceDataMap;
// Set of flags that tell us when events fired on the current frame
bool bEventsFired[MAX_GAME_EVT] = { 0 };
// Rule 1: The ordering in this array MUST match the ordering in GameEventEnum
// Rule 2: Always begin an event with the token "EVT_" (it's used to parse it)
char *g_sGameEventNames[MAX_GAME_EVT] = {
	"EVT_NONE",
	// Target Events
	"EVT_TARGET_SEL",
	"EVT_LASER_LOCKED",
	"EVT_WARHEAD_LOCKING",
	"EVT_WARHEAD_LOCKED",
	// Cockpit Damage Events
	"EVT_BROKEN_CMD",
	"EVT_BROKEN_LASER_ION",
	"EVT_BROKEN_BEAM_WEAPON",
	"EVT_BROKEN_SHIELDS",
	"EVT_BROKEN_THROTTLE",
	"EVT_BROKEN_SENSORS",
	"EVT_BROKEN_LASER_RECHARGE",
	"EVT_BROKEN_ENGINE_POWER",
	"EVT_BROKEN_SHIELD_RECHARGE",
	"EVT_BROKEN_BEAM_RECHARGE",
	// Hull Damage Events
	"EVT_DAMAGE_75",
	"EVT_DAMAGE_50",
	"EVT_DAMAGE_25",
	// Warning Lights Events
	"EVT_WLIGHT_LEFT",
	"EVT_WLIGHT_MID_LEFT",
	"EVT_WLIGHT_MID_RIGHT",
	"EVT_WLIGHT_RIGHT_YELLOW",
	"EVT_WLIGHT_RIGHT_RED",
	// Laser/Ion cannon ready to fire events
	"EVT_CANNON_1_READY",
	"EVT_CANNON_2_READY",
	"EVT_CANNON_3_READY",
	"EVT_CANNON_4_READY",
	"EVT_CANNON_5_READY",
	"EVT_CANNON_6_READY",
	"EVT_CANNON_7_READY",
	"EVT_CANNON_8_READY",
};

// Rule 1: The ordering in this array MUST match the ordering in InstanceEventEnum
// Rule 2: Always begin an event with the token "IEVT_" (it's used to parse it)
char *g_sInstEventNames[MAX_INST_EVT] = {
	"IEVT_NONE",
	// Hull Damage Events
	"IEVT_DAMAGE_75",
	"IEVT_DAMAGE_50",
	"IEVT_DAMAGE_25",

	"IEVT_SHIELDS_DOWN",

	"IEVT_TRACTOR_BEAM",
	"IEVT_JAMMING_BEAM",
};

/*
 Contains all the materials for all the OPTs currently loaded
*/
std::vector<CraftMaterials> g_Materials;
// List of all the global animated materials used in the current mission.
// Instance events still have an entry in this array, but they are "templates" used
// to add ATCs to g_AnimatedInstMaterials below
// This is used to update the timers on these materials.
std::vector<AnimatedTexControl> g_AnimatedMaterials;
// List of all the instance animated materials.
// TODO: Add support for this map so that each instance material has its own timer
std::vector<AnimatedTexControl> g_AnimatedInstMaterials;

// List of all the OPTs seen so far
std::vector<OPTNameType> g_OPTnames;

char *g_sGreebleBlendModes[GBM_MAX_MODES - 1] = {
	"MULTIPLY",					// Mode 1, the ordering must match GreebleBlendMode
	"OVERLAY",					// 2
	"SCREEN",					// 3
	"REPLACE",					// 4
	"NORMAL_MAP",				// 5
	"UV_DISP",					// 6
	"UV_DISP_AND_NORMAL_MAP",	// 7
};
// Global Greeble Data (mask, textures, blending modes)
std::vector<GreebleData> g_GreebleData;

// Looks for sBlendMode in g_sGreebleBlendModes. Returns the corresponding
// GreebleBlendMode if found, or 0 if not.
int GreebleBlendModeToEnum(char *sBlendMode) {
	for (int i = 0; i < GBM_MAX_MODES - 1; i++)
		if (_stricmp(sBlendMode, g_sGreebleBlendModes[i]) == 0)
			return i + 1; // Greeble blend modes are 1-based.
	return 0;
}

// Alternate Explosion Selector. -1 means: use the default version.
int g_AltExplosionSelector[MAX_XWA_EXPLOSIONS] = { -1, -1, -1,  -1, -1, -1,  -1 };
bool g_bExplosionsDisplayedOnCurrentFrame = false;
// Force a specific alternate explosion set. -1 doesn't force anything.
int g_iForceAltExplosion = -1;

/*
 * Convert an OPT name into a MAT params file of the form:
 * Materials\<OPTName>.mat
 */
void OPTNameToMATParamsFile(char* OPTName, char* sFileName, int iFileNameSize) {
	snprintf(sFileName, iFileNameSize, "Materials\\%s.mat", OPTName);
}

/*
 * Loads an UV coord row
 */
bool LoadUVCoord(char* buf, Vector2* UVCoord)
{
	int res = 0;
	char* c = NULL;
	float x, y;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f", &x, &y);
			if (res < 2) {
				log_debug("[DBG] [MAT] Error (skipping), expected at least 2 elements in '%s'", c);
			}
			UVCoord->x = x;
			UVCoord->y = y;
		}
		catch (...) {
			log_debug("[DBG] [MAT] Could not read 'x, y' from: %s", buf);
			return false;
		}
	}
	return true;
}

/*
 * Loads a Light color row
 */
bool LoadLightColor(char* buf, Vector3* Light)
{
	int res = 0;
	char* c = NULL;
	float r, g, b;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f", &r, &g, &b);
			if (res < 3) {
				log_debug("[DBG] [MAT] Error (skipping), expected at least 3 elements in '%s'", c);
			}
			Light->x = r;
			Light->y = g;
			Light->z = b;
		}
		catch (...) {
			log_debug("[DBG] [MAT] Could not read 'r, g, b' from: %s", buf);
			return false;
		}
	}
	return true;
}

#define SKIP_WHITESPACES(s) {\
	while (*s != 0 && *s == ' ') s++;\
	if (*s == 0) return false;\
}

bool ParseEvent(char *s, GameEvent *eventType, InstEventType *instEventType, bool *isInstanceEvt) {
	char s_event[80];
	int res;
	// Default is EVT_NONE: if no event can be parsed, that's the event we'll assign
	*eventType = EVT_NONE;
	*instEventType = IEVT_NONE;
	*isInstanceEvt = false;

	// Parse the event
	try {
		res = sscanf_s(s, "%s", s_event, 80);
		if (res < 1) {
			log_debug("[DBG] [MAT] Error reading event in '%s'", s);
		}
		else {
			// Check the game event list
			for (int i = 0; i < MAX_GAME_EVT; i++)
				if (_stricmp(s_event, g_sGameEventNames[i]) == 0) {
					*eventType = (GameEvent)i;
					log_debug("[DBG] [MAT] (GameEvent) EventType: %s", s_event);
					break;
				}
			// Check the instance event list
			for (int i = 0; i < MAX_INST_EVT; i++)
				if (_stricmp(s_event, g_sInstEventNames[i]) == 0) {
					*instEventType = (InstEventType)i;
					*isInstanceEvt = true;
					log_debug("[DBG] [INST] (InstEvent) EventType: %s", s_event);
					break;
				}
		}
	}
	catch (...) {
		log_debug("[DBG] [MAT} Could not read event in '%s'", s);
		return false;
	}
	return true;
}

void UpgradeOverayCtrlToLightmap(uint32_t* overlayCtrl, bool IsLightMap) {
	if (overlayCtrl == nullptr)
		return;
	if (*overlayCtrl == 0 || !IsLightMap)
		return;

	if ((*overlayCtrl & OVERLAY_CTRL_MULT) != 0x0) {
		*overlayCtrl |= OVERLAY_ILLUM_CTRL_MULT;
		*overlayCtrl &= ~OVERLAY_CTRL_MULT;
		return;
	}

	if ((*overlayCtrl & OVERLAY_CTRL_SCREEN) != 0x0) {
		*overlayCtrl |= OVERLAY_ILLUM_CTRL_SCREEN;
		*overlayCtrl &= ~OVERLAY_CTRL_SCREEN;
		return;
	}
}

bool ParseOverlayControl(char* buf, bool IsLightingLayer, uint32_t* OverlayCtrl) {
	if (_stricmp(buf, "LYR_MULT") == 0) {
		if (IsLightingLayer)
			*OverlayCtrl |= OVERLAY_ILLUM_CTRL_MULT;
		else
			*OverlayCtrl |= OVERLAY_CTRL_MULT;
		return true;
	}
	if (_stricmp(buf, "LYR_SCREEN") == 0) {
		if (IsLightingLayer)
			*OverlayCtrl |= OVERLAY_ILLUM_CTRL_SCREEN;
		else
			*OverlayCtrl |= OVERLAY_CTRL_SCREEN;
		return true;
	}
	return false;
}

bool ParseDatFileNameAndGroup(char *buf, char *sDATFileNameOut, int sDATFileNameSize, short *GroupId) {
	std::string s_buf(buf);
	*GroupId = -1;
	sDATFileNameOut[0] = 0;

	int split_idx = s_buf.find_last_of('-');
	if (split_idx > 0) {
		std::string sDATFileName = s_buf.substr(0, split_idx);
		std::string sGroup = s_buf.substr(split_idx + 1);
		try {
			*GroupId = (short)std::stoi(sGroup);
		}
		catch (...) {
			sDATFileNameOut[0] = 0;
			*GroupId = -1;
			return false;
		}
		strcpy_s(sDATFileNameOut, sDATFileNameSize, sDATFileName.c_str());
		return true;
	}
	
	return false;
}

// Parse the DAT/ZIP file name, GroupId and ImageId from a string of the form:
// <Path>\<DATorZIPFileName>-<GroupId>-<ImageId>
// Returns false if the string could not be parsed
bool ParseDatZipFileNameGroupIdImageId(char *buf, char *sDATZIPFileNameOut, int sDATZIPFileNameSize, short *GroupId, short *ImageId)
{
	std::string s_buf(buf);
	*GroupId = -1; *ImageId = -1;
	sDATZIPFileNameOut[0] = 0;
	int split_idx = -1;

	// Find the last hyphen and get the ImageId
	split_idx = s_buf.find_last_of('-');
	if (split_idx < 0)
		return false;
	std::string sImageId = s_buf.substr(split_idx + 1);

	// Terminate the string here and find the next hyphen
	s_buf[split_idx] = 0;
	split_idx = s_buf.find_last_of('-');
	if (split_idx < 0)
		return false;

	std::string sDATZIPFileName = s_buf.substr(0, split_idx);
	std::string sGroupId = s_buf.substr(split_idx + 1);
	try {
		*ImageId = (short)std::stoi(sImageId);
		*GroupId = (short)std::stoi(sGroupId);
	}
	catch (...) {
		sDATZIPFileNameOut[0] = 0;
		*GroupId = -1;
		*ImageId = -1;
		return false;
	}
	strcpy_s(sDATZIPFileNameOut, sDATZIPFileNameSize, sDATZIPFileName.c_str());
	return true;
}

// Interpolates the ImageId between two TexSeqElem's.
// Only call this if the tex sequence is for a DAT file, so that texnames are in the format
// GroupId-ImageId.
void InterpolateTexSequence(std::vector<TexSeqElem> &tex_sequence, TexSeqElem tex_seq_elem0, TexSeqElem tex_seq_elem1)
{
	TexSeqElem cur_tex_seq_elem;
	int ImageId0, ImageId1, ImageIdStep, CurImageId, num_steps;
	float size;
	int res = 0;
	log_debug("[DBG] [MAT] Interpolating tex sequence elements");
	//log_debug("[DBG] [MAT] tex_seq_elem0: [%s], sec: %0.3f, int: %0.3f",
	//	tex_seq_elem0.texname, tex_seq_elem0.seconds, tex_seq_elem0.intensity);

	ImageId0 = tex_seq_elem0.ImageId;
	ImageId1 = tex_seq_elem1.ImageId;
	if (ImageId0 == ImageId1) return; // Nothing to do, both ImageIds are equal and the previous one has already been added.
	ImageIdStep = (ImageId1 > ImageId0) ? 1 : -1;
	num_steps = abs(ImageId1 - ImageId0);
	size = (float )(ImageId1 - ImageId0);
	// The cycle starts at ImageId0 + ImageIdStep because ImageId0 is already part of the sequence:
	// it was read and added in the previous loop in LoadTextureSequence().
	CurImageId = ImageId0 + ImageIdStep;
	for (int i = 0; i < num_steps; i++, CurImageId += ImageIdStep) {
		// Copy the DAT file name:
		strcpy_s(cur_tex_seq_elem.texname, MAX_TEX_SEQ_NAME, tex_seq_elem0.texname);
		// Interpolate the seconds and intensity attributes:
		cur_tex_seq_elem.seconds = tex_seq_elem1.seconds * (CurImageId - ImageId0) / size + tex_seq_elem0.seconds * (ImageId1 - CurImageId) / size;
		cur_tex_seq_elem.intensity = tex_seq_elem1.intensity * (CurImageId - ImageId0) / size + tex_seq_elem0.intensity * (ImageId1 - CurImageId) / size;
		// Fill the rest of the fields:
		cur_tex_seq_elem.IsDATImage = true;
		cur_tex_seq_elem.GroupId = tex_seq_elem0.GroupId;
		cur_tex_seq_elem.ImageId = CurImageId;
		cur_tex_seq_elem.ExtraTextureIndex = -1;
		//log_debug("[DBG] [MAT] Adding interpolated elem: %d-%d, sec: %0.3f, int: %0.3f",
		//	cur_tex_seq_elem.GroupId, cur_tex_seq_elem.ImageId, cur_tex_seq_elem.seconds, cur_tex_seq_elem.intensity);
		tex_sequence.push_back(cur_tex_seq_elem);
	}
}

int AlphaModeToSpecialControl(int alpha_mode) {
	switch (alpha_mode) {
	case 1:
		return SPECIAL_CONTROL_BLACK_TO_ALPHA;
		break;
	case 2:
		return SPECIAL_CONTROL_ALPHA_IS_BLOOM_MASK;
		break;
	default:
		return 0;
	}
}

bool ParseOptionalTexSeqArgs(char *buf, int *alpha_mode, float4 *AuxColor)
{
	int res = 0, raw_alpha_mode;
	float r, g, b;

	*alpha_mode = 0;
	AuxColor->x = 1.0f;
	AuxColor->y = 1.0f;
	AuxColor->z = 1.0f;
	AuxColor->w = 1.0f;
	res = sscanf_s(buf, "%d, [%f, %f, %f]", &raw_alpha_mode, &r, &g, &b);
	if (res < 4)
	{
		log_debug("[DBG] [MAT] Error, expected at least 4 elements reading OptTexSeqArgs in %s", buf);
		return false;
	}
	*alpha_mode = AlphaModeToSpecialControl(raw_alpha_mode);
	AuxColor->x = r;
	AuxColor->y = g;
	AuxColor->z = b;
	AuxColor->w = 1.0f;
	return true;
}

void ParseUVArea(char* s, float2 *uvSrc0, float2 *uvSrc1)
{
	float x0, y0, x1, y1;
	sscanf_s(s, "%f, %f, %f, %f,", &x0, &y0, &x1, &y1);
	uvSrc0->x = x0;
	uvSrc0->y = y0;
	uvSrc1->x = x1;
	uvSrc1->y = y1;
}

char *ParseOptionalModifiers(char* s, AnimatedTexControl *atc)
{
	char* t;
	bool DoLoop = true;

	while (DoLoop)
	{
		DoLoop = false;

		// Parse the UV modifier
		if (stristr(s, "UV_AREA") != NULL) {
			DoLoop = true;

			t = s;
			// Skip to the next '=' char
			while (*t != 0 && *t != '=') t++;
			// If we reached the end of the string, that's an error
			if (*t == 0) return nullptr;
			// Skip the '='
			t++;

			// Start a new string here
			s = t;
			// Skip 4 commas
			for (int i = 0; i < 4; i++)
			{
				// Skip to the next ',' char
				while (*t != 0 && *t != ',') t++;
				// If we reached the end of the string, that's an error
				if (*t == 0) return nullptr;
				// Skip the comma
				t++;
			}

			// End the string on this comma to make a new substring
			*t = 0;
			ParseUVArea(s, &(atc->uvSrc0), &(atc->uvSrc1));
			// Skip the previous comma to continue parsing
			t++;

			// Re-start the string here and continue parsing
			s = t;
			SKIP_WHITESPACES(s);
		}
		// Parse the FIXED_LOC modifier
		else if (stristr(s, "FIXED_LOC") != NULL)
		{
			DoLoop = true;

			atc->uvRandomLoc = 0; // Disable RAND_LOC

			t = s;
			// Skip to the next comma
			while (*t != 0 && *t != ',') t++;
			// If we reached the end of the string, that's an error
			if (*t == 0) return nullptr;
			// Store the separator char where we stopped
			char sep = *t;
			// Skip the comma
			t++;

			// Re-start the string here and continue parsing
			s = t;
			SKIP_WHITESPACES(s);
		}
		// Parse the RANDOM_LOC modifier
		else if (stristr(s, "RAND_LOC") != NULL)
		{
			DoLoop = true;

			atc->uvRandomLoc = 1; // RAND_LOC, no params

			t = s;
			// Skip to the next comma or '=' sign
			while (*t != 0 && (*t != ',' && *t != '=')) t++;
			// If we reached the end of the string, that's an error
			if (*t == 0) return nullptr;
			// Store the separator char where we stopped
			char sep = *t;
			// Skip the comma or equals sign
			t++;

			// Parse the next values if we found an equals sign
			if (sep == '=')
			{
				// Start a new string here and continue parsing
				s = t;
				// Skip 4 commas
				for (int i = 0; i < 4; i++)
				{
					// Skip to the next ',' char
					while (*t != 0 && *t != ',') t++;
					// If we reached the end of the string, that's an error
					if (*t == 0) return nullptr;
					// Skip the comma
					t++;
				}

				// End the string on this comma to make a new substring
				*t = 0;
				float x0, y0, x1, y1;
				sscanf_s(s, "%f, %f, %f, %f", &x0, &y0, &x1, &y1);
				atc->uvOffsetMin.x = x0;
				atc->uvOffsetMin.y = y0;
				atc->uvOffsetMax.x = x1;
				atc->uvOffsetMax.y = y1;
				atc->uvRandomLoc = 2; // RAND_LOC with 4 params
				// Skip the previous comma to continue parsing
				t++;
			}

			// Re-start the string here and continue parsing
			s = t;
			SKIP_WHITESPACES(s);
			/*if (atc->uvRandomLoc == 2)
			{
				log_debug("[DBG] [UV] RAND_LOC: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
					atc->uvOffsetMin.x, atc->uvOffsetMin.y,
					atc->uvOffsetMax.x, atc->uvOffsetMax.y);
			}*/
		}
		// Parse the SCALE modifier
		else if (stristr(s, "SCALE") != NULL && stristr(s, "RAND_SCALE") == NULL)
		{
			DoLoop = true;

			t = s;
			// Skip to the next '=' char
			while (*t != 0 && *t != '=') t++;
			// If we reached the end of the string, that's an error
			if (*t == 0) return nullptr;
			// Skip the '='
			t++;

			// Start a new string here
			s = t;
			// Skip 1 comma
			for (int i = 0; i < 1; i++)
			{
				// Skip to the next ',' char
				while (*t != 0 && *t != ',') t++;
				// If we reached the end of the string, that's an error
				if (*t == 0) return nullptr;
				// Skip the comma
				t++;
			}

			// End the string on this comma to make a new substring
			*t = 0;
			float scale;
			sscanf_s(s, "%f", &scale);
			atc->uvScaleMin.x = atc->uvScaleMin.y = scale;
			atc->uvScaleMax.x = atc->uvScaleMax.y = scale;
			atc->uvRandomScale = false;
			// Skip the previous comma to continue parsing
			t++;

			// Re-start the string here and continue parsing
			s = t;
			SKIP_WHITESPACES(s);
			/*log_debug("[DBG] [UV] Scale: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
				atc->uvScaleMin.x, atc->uvScaleMin.y,
				atc->uvScaleMax.x, atc->uvScaleMax.y);*/
		}
		// Parse the RANDOM_SCALE modifier
		else if (stristr(s, "RAND_SCALE") != NULL)
		{
			DoLoop = true;

			t = s;
			// Skip to the next '=' char
			while (*t != 0 && *t != '=') t++;
			// If we reached the end of the string, that's an error
			if (*t == 0) return nullptr;
			// Skip the '='
			t++;

			// Start a new string here
			s = t;
			// Skip 2 commas
			for (int i = 0; i < 2; i++)
			{
				// Skip to the next ',' char
				while (*t != 0 && *t != ',') t++;
				// If we reached the end of the string, that's an error
				if (*t == 0) return nullptr;
				// Skip the comma
				t++;
			}

			// End the string on this comma to make a new substring
			*t = 0;
			float minScale, maxScale;
			sscanf_s(s, "%f, %f", &minScale, &maxScale);
			atc->uvScaleMin.x = atc->uvScaleMin.y = minScale;
			atc->uvScaleMax.x = atc->uvScaleMax.y = maxScale;
			atc->uvRandomScale = true;
			// Skip the previous comma to continue parsing
			t++;

			// Re-start the string here and continue parsing
			s = t;
			SKIP_WHITESPACES(s);
			/*log_debug("[DBG] [UV] Rand scale: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
				atc->uvScaleMin.x, atc->uvScaleMin.y,
				atc->uvScaleMax.x, atc->uvScaleMax.y);*/
		}
	}
	return s;
}

/*
 Load a texture sequence line of the form:

 lightmap_seq|rand|anim_seq|rand = [Event], [DATFileName], <TexName1>,<seconds1>,<intensity1>, <TexName2>,<seconds2>,<intensity2>, ... 
*/
bool LoadTextureSequence(char* buf, bool IsLightMap, std::vector<TexSeqElem>& tex_sequence,
	int* alpha_mode, AnimatedTexControl *atc)
{
	TexSeqElem tex_seq_elem, prev_tex_seq_elem;
	int res = 0;
	constexpr int MAX_TEMP_LEN = 2048;
	char temp[MAX_TEMP_LEN];
	char *s = NULL, *t = NULL, texname[80], sDATZIPFileName[128], sOptionalArgs[128];
	float seconds, intensity = 1.0f;
	bool IsDATFile = false, IsZIPFile = false, bEllipsis = false;
	atc->Event = EVT_NONE;
	atc->InstEvent = IEVT_NONE;
	atc->isInstEvent = false;
	std::string s_temp;
	*alpha_mode = 0;
	atc->Tint.x = 1.0f;
	atc->Tint.y = 1.0f;
	atc->Tint.z = 1.0f;
	atc->Tint.w = 1.0f;
	
	// Remove any parentheses from the line
	int i = 0, j = 0;
	while (buf[i] && j < MAX_TEMP_LEN - 1) {
		if (buf[i] != '(' && buf[i] != ')')
			temp[j++] = buf[i];
		i++;
	}
	temp[j] = 0;
	
	//log_debug("[DBG] [MAT] Reading texture sequence");
	s = strchr(temp, '=');
	if (s == NULL) return false;

	// Skip the equals sign:
	s += 1;

	// Parse the event, if it exists
	if (stristr(s, "EVT_") != NULL || stristr(s, "IEVT_") != NULL) {
		//if (stristr(s, "IEVT_") != NULL)
		//	log_debug("[DBG] [INST] Parsing INSTANCE EVENT from: [%s]", s);
		// Skip to the next comma
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the comma so that we can parse a string
		*t = 0;
		ParseEvent(s, &(atc->Event), &(atc->InstEvent), &(atc->isInstEvent));
		//if (*isInstEvent)
		//	log_debug("[DBG] [INST] INSTANCE EVENT parsed, %d", *instEventType);
		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);
	}

	// Parse the overlay modifier
	if (stristr(s, "LYR_") != NULL) {
		// Skip to the next comma
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the comma so that we can parse a string
		*t = 0;
		ParseOverlayControl(s, IsLightMap, &(atc->OverlayCtrl));
		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);
	}

	// Parse the optional modifiers
	s = ParseOptionalModifiers(s, atc);

	// Parse the DAT file name, if specified
	if (stristr(s, ".dat") != NULL) {
		IsDATFile = true;
		IsZIPFile = false;
		SKIP_WHITESPACES(s);
		// Skip to the next comma
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the comma so that we can parse a string
		*t = 0;
		// Copy the DAT's filename
		strcpy_s(sDATZIPFileName, 128, s);
		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);
	}
	// Parse the ZIP file name, if specified
	else if (stristr(s, ".zip") != NULL) {
		IsZIPFile = true;
		IsDATFile = false;
		SKIP_WHITESPACES(s);
		// Skip to the next comma
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the comma so that we can parse a string
		*t = 0;
		// Copy the DAT's filename
		strcpy_s(sDATZIPFileName, 128, s);
		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);
	}

	// Parse optional args, if specified
	if (s[0] == '{') {
		// Skip the opening braces
		s++;
		// Skip to the closing braces
		t = s;
		while (*t != 0 && *t != '}') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the closing braces so that we can parse a string
		*t = 0;
		// Copy the optional args
		strcpy_s(sOptionalArgs, 128, s);
		// log_debug("[DBG] [MAT] sOptionalArgs: [%s]", sOptionalArgs);
		// Parse the optional args...
		ParseOptionalTexSeqArgs(sOptionalArgs, alpha_mode, &(atc->Tint));
		// Skip the closing braces
		s = t; s += 1;
		// Skip to the next comma
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// Skip the comma and remaining white spaces
		s = t; s += 1;
		SKIP_WHITESPACES(s);
	}

	while (*s != 0)
	{
		// Skip to the next comma, we want to read the texname
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the comma so that we can parse a string
		*t = 0;
		// Parse the texture name/GroupId-ImageId
		try {
			res = sscanf_s(s, "%s", texname, 80);
			if (res < 1) log_debug("[DBG] [MAT] Error reading texname in '%s'", s);
		}
		catch (...) {
			log_debug("[DBG] [MAT] Could not read texname in '%s'", s);
			return false;
		}

		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);

		// if texname happens to be an ellipsis, then we didn't read the texname, so
		// we need to do it again
		if (strstr(texname, "..") != NULL) {
			texname[0] = 0;
			bEllipsis = true;
			// Skip to the next comma, we want to read the texname
			t = s;
			while (*t != 0 && *t != ',') t++;
			// If we reached the end of the string, that's an error
			if (*t == 0) return false;
			// End this string on the comma so that we can parse a string
			*t = 0;
			// Parse the texture name/GroupId-ImageId
			try {
				res = sscanf_s(s, "%s", texname, 80);
				if (res < 1) log_debug("[DBG] [MAT] Error reading texname in '%s'", s);
			}
			catch (...) {
				log_debug("[DBG] [MAT] Could not read texname in '%s'", s);
				return false;
			}

			// Skip the comma
			s = t; s += 1;
			SKIP_WHITESPACES(s);
		}
		else {
			bEllipsis = false;
		}

		// Parse the seconds
		t = s;
		while (*t != 0 && *t != ',')
			t++;
		// If we reached the end of the string, that's an error
		if (*t == 0)
			return false;
		try {
			res = sscanf_s(s, "%f", &seconds);
			if (res < 1) log_debug("[DBG] [MAT] Error reading seconds in '%s'", s);
		}
		catch (...) {
			log_debug("[DBG] [MAT] Could not read seconds in '%s'", s);
			return false;
		}

		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);

		// Parse the intensity
		t = s;
		while (*t != 0 && *t != ',')
			t++;
		try {
			res = sscanf_s(s, "%f", &intensity);
			if (res < 1) log_debug("[DBG] [MAT] Error reading intensity in '%s'", s);
		}
		catch (...) {
			log_debug("[DBG] [MAT] Could not read intensity in '%s'", s);
			return false;
		}

		// Save the last tex_seq_elem, we might need it later, to interpolate a range of elems.
		prev_tex_seq_elem = tex_seq_elem;
		// Populate the tex_seq_elem with the data we just read
		if (IsDATFile || IsZIPFile) {
			// This is a DAT/ZIP file sequence, texname should be parsed as GroupId-ImageId
			try {
				int GroupId = -1, ImageId = -1;
				int res = sscanf_s(texname, "%d-%d", &GroupId, &ImageId);
				if (res < 2) {
					log_debug("[DBG] [MAT] Could not parse GroupId-ImageId from [%s]", texname);
					return false;
				}
				strcpy_s(tex_seq_elem.texname, MAX_TEX_SEQ_NAME, sDATZIPFileName);
				tex_seq_elem.seconds = seconds;
				tex_seq_elem.intensity = intensity;
				tex_seq_elem.IsDATImage = IsDATFile;
				tex_seq_elem.IsZIPImage = IsZIPFile;
				tex_seq_elem.GroupId = GroupId;
				tex_seq_elem.ImageId = ImageId;
				// The texture itself will be loaded later. So the reference index is initialized to -1 here:
				tex_seq_elem.ExtraTextureIndex = -1;
				if (!bEllipsis) {
					tex_sequence.push_back(tex_seq_elem);
					log_debug("[DBG] %s tex_seq_elem added: [%s], Group: %d, ImageId: %d",
						IsDATFile ? "DAT" : "ZIP", tex_seq_elem.texname, GroupId, ImageId);
				}
				else {
					// Interpolate between prev_tex_seq_elem and tex_seq_elem
					InterpolateTexSequence(tex_sequence, prev_tex_seq_elem, tex_seq_elem);
				}
			}
			catch (...) {
				log_debug("[DBG] ERROR: Could not parse [%s] as an ImageId", texname);
				return false;
			}
		}
		else {
			strcpy_s(tex_seq_elem.texname, MAX_TEX_SEQ_NAME, texname);
			tex_seq_elem.seconds = seconds;
			tex_seq_elem.intensity = intensity;
			tex_seq_elem.IsDATImage = false;
			tex_seq_elem.IsZIPImage = false;
			// The texture itself will be loaded later. So the reference index is initialized to -1 here:
			tex_seq_elem.ExtraTextureIndex = -1;
			tex_sequence.push_back(tex_seq_elem);
		}

		// If we reached the end of the string, we're done
		if (*t == 0) break;
		// Else, we need to parse the next texture...
		// Skip the current char (which should be a comma) and repeat
		s = t; s += 1;
	}
	//log_debug("[DBG] [MAT] Texture sequence done");
	return true;
}

bool LoadFrameSequence(char* buf, std::vector<TexSeqElem>& tex_sequence,
	int* is_lightmap, int* alpha_mode, AnimatedTexControl *atc)
{
	constexpr int MAX_TEMP_LEN = 2048;
	char temp[MAX_TEMP_LEN];
	int res = 0, clamp, raw_alpha_mode;
	char* s = NULL, * t = NULL, path[256];
	float fps = 30.0f, intensity = 0.0f, r, g, b, OfsX, OfsY, ar;
	*is_lightmap = 0; *alpha_mode = 0;
	atc->Event = EVT_NONE; atc->InstEvent = IEVT_NONE; atc->isInstEvent = false;
	atc->Tint.x = 1.0f;
	atc->Tint.y = 1.0f;
	atc->Tint.z = 1.0f;
	atc->Tint.w = 1.0f;
	atc->Offset.x = 0.0f; atc->Offset.y = 0.0f;
	atc->AspectRatio = 1.0f; atc->Clamp = 0;

	// Remove any parentheses from the line
	int i = 0, j = 0;
	while (buf[i] && j < MAX_TEMP_LEN - 1) {
		if (buf[i] != '(' && buf[i] != ')')
			temp[j++] = buf[i];
		i++;
	}
	temp[j] = 0;

	//log_debug("[DBG] [MAT] Reading texture sequence");
	s = strchr(temp, '=');
	if (s == NULL) return false;

	// Skip the equals sign:
	s += 1;

	// Parse the event, if it exists
	{
		// Skip to the next comma
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the comma so that we can parse a string
		*t = 0;
		ParseEvent(s, &(atc->Event), &(atc->InstEvent), &(atc->isInstEvent));

		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);
	}

	// Parse the overlay modifier
	if (stristr(s, "LYR_") != NULL) {
		// Skip to the next comma
		t = s;
		while (*t != 0 && *t != ',') t++;
		// If we reached the end of the string, that's an error
		if (*t == 0) return false;
		// End this string on the comma so that we can parse a string
		*t = 0;
		ParseOverlayControl(s, false, &(atc->OverlayCtrl));
		// Skip the comma
		s = t; s += 1;
		SKIP_WHITESPACES(s);
	}

	// Parse the optional modifiers
	s = ParseOptionalModifiers(s, atc);

	// Skip to the next comma
	t = s;
	while (*t != 0 && *t != ',') t++;
	// If we reached the end of the string, that's an error
	if (*t == 0) return false;
	// End this string on the comma so that we can parse a string
	*t = 0;
	// Parse the texture name
	try {
		res = sscanf_s(s, "%s", path, 256);
		if (res < 1) log_debug("[DBG] [MAT] Error reading path in '%s'", s);
	}
	catch (...) {
		log_debug("[DBG] [MAT] Could not read path in '%s'", s);
		return false;
	}

	// Skip the comma
	s = t; s += 1;
	SKIP_WHITESPACES(s);

	// Parse the remaining fields: fps, lightmap, intensity, black-to-alpha
	try {
		res = sscanf_s(s, "%f, %d, %f, %d; [%f, %f, %f], %f, %f, %f, %d",
			&fps, is_lightmap, &intensity, &raw_alpha_mode,
			&r, &g, &b, &ar, &OfsX, &OfsY, &clamp);
		if (res < 4) {
			log_debug("[DBG] [MAT] Error (using defaults), expected at least 4 elements in %s", s);
		}

		*alpha_mode = AlphaModeToSpecialControl(raw_alpha_mode);
		if (res >= 7) {
			atc->Tint.x = r;
			atc->Tint.y = g;
			atc->Tint.z = b;
			atc->Tint.w = 1.0f;
		}
		if (res >= 8)
			atc->AspectRatio = ar; // Aspect Ratio
		if (res >= 10) {
			atc->Offset.x = OfsX;
			atc->Offset.y = OfsY;
		}
		if (res >= 11)
			atc->Clamp = clamp; // clamp uv
		
	}
	catch (...) {
		log_debug("[DBG] [MAT] Could not read (fps, lightmap, intensity, black-to-alpha) from %s", s);
		return false;
	}

	// We only know if this is a lightmap animation here, so we need to
	// upgrade the previous OverlayCtrl now.
	if (atc->OverlayCtrl != 0) {
		UpgradeOverayCtrlToLightmap(&(atc->OverlayCtrl), *is_lightmap);
	}

	TexSeqElem tex_seq_elem;
	// The path is either an actual path that contains the frame sequence, or it's
	// a <Path>\<DATFile|ZIPFile>-<GroupId> or <Path>\<DATFile|ZIPFile>-<GroupId>-<ImageId>.
	// Let's check if path contains the ".DAT-" token first:
	char *splitDAT = stristr(path, ".dat-");
	char *splitZIP = stristr(path, ".zip-");
	char *split = splitDAT != NULL ? splitDAT : splitZIP;
	bool IsDATFile = splitDAT != NULL;
	bool IsZIPFile = splitZIP != NULL;
	if (split != NULL) {
		// Split the string at the first dash and move the cursor after it:
		split[4] = 0;
		split += 5;
		// sDATZIPFileName contains the path and DAT|ZIP file name:
		std::string sDATZIPFileName(path);
		std::string sGroupImage(split);
		std::string sGroup, sImage;
		int split_idx = sGroupImage.find_last_of('-');
		if (split_idx > 0) {
			sGroup = sGroupImage.substr(0, split_idx);
			sImage = sGroupImage.substr(split_idx + 1);
		}
		else {
			sGroup = sGroupImage;
			sImage = "";
		}
		log_debug("[DBG] sDATZIPFileName: [%s], Group: [%s], Image: [%s]",
			sDATZIPFileName.c_str(), sGroup.c_str(), sImage.c_str());

		short GroupId = -1, ImageId = -1;
		try {
			GroupId = (short)std::stoi(sGroup);
		}
		catch (...) {
			log_debug("[DBG] Could not parse: %s into an integer", sGroup.c_str());
			return false;
		}
		if (sImage.size() > 0) {
			try {
				ImageId = (short)std::stoi(sImage);
			}
			catch (...) {
				log_debug("[DBG] Could not parse: %s into an integer", sImage.c_str());
				return false;
			}
		}

		std::vector<short> ImageList;
		if (ImageId > -1) {
			ImageList.push_back(ImageId);
			log_debug("[DBG] Using only %d-%d", GroupId, ImageId);
		}
		else {
			if (IsDATFile)
				ImageList = ReadDATImageListFromGroup(sDATZIPFileName.c_str(), GroupId);
			else if (IsZIPFile)
				ImageList = ReadZIPImageListFromGroup(sDATZIPFileName.c_str(), GroupId);
			log_debug("[DBG] Group %d has %d images", GroupId, ImageList.size());
		}
		// Iterate over the list of Images and add one TexSeqElem for each one of them
		for each (short ImageId in ImageList)
		{
			// Store the DAT filename in texname and set the appropriate flag. texname contains
			// the path and filename.
			strcpy_s(tex_seq_elem.texname, MAX_TEX_SEQ_NAME, sDATZIPFileName.c_str());
			// Prevent division by 0:
			fps = max(0.0001f, fps);
			tex_seq_elem.seconds = 1.0f / fps;
			tex_seq_elem.intensity = intensity;
			// Save the DAT image data:
			tex_seq_elem.IsDATImage = IsDATFile;
			tex_seq_elem.IsZIPImage = IsZIPFile;
			tex_seq_elem.GroupId = GroupId;
			tex_seq_elem.ImageId = ImageId;
			// The texture itself will be loaded later. So the reference index is initialized to -1 here:
			tex_seq_elem.ExtraTextureIndex = -1;
			tex_sequence.push_back(tex_seq_elem);
		}
		ImageList.clear();
	}
	else {
		std::string s_path = std::string("Effects\\Animations\\") + std::string(path);
		if (fs::is_directory(s_path)) {
			// We just finished reading the path where a frame sequence is stored
			// Now we need to load all the frames in that path into a tex_seq_elem

			//log_debug("[DBG] Listing files under path: %s", s_path.c_str());
			for (const auto & entry : fs::directory_iterator(s_path)) {
				// Surely there's a better way to get the filename...
				std::string filename = std::string(path) + "\\" + entry.path().filename().string();
				//log_debug("[DBG] file: %s", filename.c_str());

				strcpy_s(tex_seq_elem.texname, MAX_TEX_SEQ_NAME, filename.c_str());
				tex_seq_elem.IsDATImage = false;
				tex_seq_elem.seconds = 1.0f / fps;
				tex_seq_elem.intensity = intensity;
				// The texture itself will be loaded later. So the reference index is initialized to -1 here:
				tex_seq_elem.ExtraTextureIndex = -1;
				tex_sequence.push_back(tex_seq_elem);
			}
		}
	}
	
	return true;
}

/*
 * Loads all the frames from GroupId in a DAT|ZIP file into tex_sequence. This function is currently
 * only used to load alternate explosion animations. See "alt_frames_1" (etc) below.
 */
bool LoadSimpleFrames(char *buf, std::vector<TexSeqElem> &tex_sequence)
{
	int res = 0;
	char *s = NULL, *t = NULL, path[256];
	//float fps = 30.0f, intensity = 0.0f, r, g, b, OfsX, OfsY, ar;

	s = strchr(buf, '=');
	if (s == NULL) return false;

	// Skip the equals sign:
	s += 1;

	// Skip to the next comma/end of the string
	t = s;
	while (*t != 0 && *t != ',') t++;
	// If we reached the end of the string, that's an error
	//if (*t == 0) return false;
	// End this string on the comma so that we can parse a string
	*t = 0;
	// Parse the texture name
	try {
		res = sscanf_s(s, "%s", path, 256);
		if (res < 1) log_debug("[DBG] [MAT] Error reading path in '%s'", s);
	}
	catch (...) {
		log_debug("[DBG] [MAT] Could not read path in '%s'", s);
		return false;
	}

	TexSeqElem tex_seq_elem;
	// The path is either an actual path that contains the frame sequence, or it's
	// a <Path>\<DATZIPFile>-<GroupId>. Let's check if path contains the ".DAT-" or
	// ".ZIP-" token first:
	char *split_dat = stristr(path, ".dat-");
	char *split_zip = stristr(path, ".zip-");
	bool bIsDATFile = split_dat != NULL;
	bool bIsZIPFile = split_zip != NULL;
	char *split = bIsDATFile ? split_dat : split_zip;
	if (split != NULL) {
		// Split the string at the first dash and move the cursor after it:
		split[4] = 0;
		split += 5;
		// sDATZIPFileName contains the path and DAT|ZIP file name:
		std::string sDATZIPFileName(path);
		std::string sGroup(split);
		log_debug("[DBG] [MAT] sDATZIPFileName: [%s], Group: [%s]", sDATZIPFileName.c_str(), sGroup.c_str());

		short GroupId = -1;
		try {
			GroupId = (short)std::stoi(sGroup);
		}
		catch (...) {
			log_debug("[DBG] [MAT] Could not parse: %s into an integer", sGroup.c_str());
			return false;
		}
		
		std::vector<short> ImageList;
		if (bIsDATFile)
			ImageList = ReadDATImageListFromGroup(sDATZIPFileName.c_str(), GroupId);
		else
			ImageList = ReadZIPImageListFromGroup(sDATZIPFileName.c_str(), GroupId);
		log_debug("[DBG] [MAT] Frame-by-Frame data. Group %d has %d images", GroupId, ImageList.size());
		// Iterate over the list of Images and add one TexSeqElem for each one of them
		for each (short ImageId in ImageList)
		{
			// Store the DAT filename in texname and set the appropriate flag. texname contains
			// the path and filename.
			strcpy_s(tex_seq_elem.texname, MAX_TEX_SEQ_NAME, sDATZIPFileName.c_str());
			// Prevent division by 0:
			// Save the DAT image data:
			tex_seq_elem.IsDATImage = bIsDATFile;
			tex_seq_elem.IsZIPImage = bIsZIPFile;
			tex_seq_elem.GroupId = GroupId;
			tex_seq_elem.ImageId = -1;
			// The texture itself will be loaded later. So the reference index is initialized to -1 here:
			tex_seq_elem.ExtraTextureIndex = -1;
			tex_sequence.push_back(tex_seq_elem);
		}
		ImageList.clear();
	}
	else {
		std::string s_path = std::string("Effects\\Animations\\") + std::string(path);
		if (fs::is_directory(s_path)) {
			// We just finished reading the path where a frame sequence is stored
			// Now we need to load all the frames in that path into a tex_seq_elem

			//log_debug("[DBG] Listing files under path: %s", s_path.c_str());
			for (const auto & entry : fs::directory_iterator(s_path)) {
				// Surely there's a better way to get the filename...
				std::string filename = std::string(path) + "\\" + entry.path().filename().string();
				//log_debug("[DBG] file: %s", filename.c_str());

				strcpy_s(tex_seq_elem.texname, MAX_TEX_SEQ_NAME, filename.c_str());
				tex_seq_elem.IsDATImage = false;
				// The texture itself will be loaded later. So the reference index is initialized to -1 here:
				tex_seq_elem.ExtraTextureIndex = -1;
				tex_sequence.push_back(tex_seq_elem);
			}
		}
	}

	return true;
}

uint32_t GenerateUniqueMaterialId()
{
	static uint32_t Id = 1;
	Id++;
	return Id - 1;
}

uint64_t InstEventIdFromObjectMaterialId(int objectId, uint32_t materialId) {
	uint64_t Id = (uint64_t)objectId;
	Id = (Id << 32) | materialId;
	return Id;
}

void InstEventIdToObjectMatId(uint64_t instId, int *objectId, uint32_t *materialId)
{
	*materialId = 0xFFFFFFFF & instId;
	*objectId = instId >> 32;
}

inline void AssignTextureEvent(GameEvent eventType, InstEventType instEventType, bool isInstEvent,
	Material* curMaterial, int ATCType)
{
	// Assign a unique Id to this material
	curMaterial->Id = GenerateUniqueMaterialId();
	if (!isInstEvent)
		curMaterial->TextureATCIndices[ATCType][eventType] = g_AnimatedMaterials.size() - 1;
	else {
		curMaterial->InstTextureATCIndices[ATCType][instEventType].push_back(g_AnimatedMaterials.size() - 1);
		//log_debug("[DBG] [INST] Added sub-template to event %s --> g_AnimatedMaterials[%d]",
		//	g_sInstEventNames[instEventType], g_AnimatedMaterials.size() - 1);
	}
}

GreebleData *GetOrAddGreebleData(Material *curMaterial) {
	if (curMaterial->GreebleDataIdx == -1) {
		// If this material doesn't have a GreebleData entry, add one and link it
		GreebleData greeble_data;
		g_GreebleData.push_back(greeble_data);
		curMaterial->GreebleDataIdx = g_GreebleData.size() - 1;
	}
	// else // This material has a GreebleData entry, fetch it
	return &(g_GreebleData[curMaterial->GreebleDataIdx]);
}

void PrintGreebleData(GreebleData *greeble_data) {
	log_debug("[DBG] [GRB] Greeble 1: %s (%d), Greeble 2: %s (%d)",
		greeble_data->GreebleTexName[0], greeble_data->GreebleTexIndex[0],
		greeble_data->GreebleTexName[1], greeble_data->GreebleTexIndex[1]);
	log_debug("[DBG] [GRB] Greeble Dist 1: %0.0f, Dist 2: %0.0f, Blend Mode 1: %d, Blend Mode 2: %d",
		greeble_data->GreebleDist[0], greeble_data->GreebleDist[1],
		greeble_data->greebleBlendMode[0], greeble_data->greebleBlendMode[1]);
}

void ReadMaterialLine(char* buf, Material* curMaterial, char *OPTname) {
	constexpr int MAX_SVALUE_LEN = 2048;
	char param[256], svalue[MAX_SVALUE_LEN];
	float fValue = 0.0f;
	bool bIsExteriorOPT(stristr(OPTname, "Exterior") != NULL);
	bool bIsCockpitOPT(stristr(OPTname, "Cockpit") != NULL);
	// OPTname doesn't have the extension of the file, but materials for DAT
	// files begin with dat-GroupId
	bool bIsDATFile(stristr(OPTname, "dat") != NULL);
	bool bIsGlobalOPT = bIsExteriorOPT || bIsCockpitOPT || bIsDATFile;
	
	// Skip comments and blank lines
	if (buf[0] == ';' || buf[0] == '#')
		return;
	if (strlen(buf) == 0)
		return;

	// Read the parameter
	if (sscanf_s(buf, "%s = %s", param, 256, svalue, MAX_SVALUE_LEN) > 0) {
		fValue = (float)atof(svalue);
	}

	if (_stricmp(param, "Metallic") == 0) {
		//log_debug("[DBG] [MAT] Metallic: %0.3f", fValue);
		curMaterial->Metallic = fValue;
	}
	else if (_stricmp(param, "Intensity") == 0) {
		//log_debug("[DBG] [MAT] Intensity: %0.3f", fValue);
		curMaterial->Intensity = fValue;
	}
	else if (_stricmp(param, "Glossiness") == 0) {
		//log_debug("[DBG] [MAT] Glossiness: %0.3f", fValue);
		curMaterial->Glossiness = fValue;
	}
	else if (_stricmp(param, "NMIntensity") == 0) {
		curMaterial->NMIntensity = fValue;
	}
	else if (_stricmp(param, "SpecularVal") == 0) {
		curMaterial->SpecValue = fValue;
	}
	else if (_stricmp(param, "Shadeless") == 0) {
		curMaterial->IsShadeless = (bool)fValue;
		//log_debug("[DBG] Shadeless texture loaded");
	}
	else if (_stricmp(param, "Light") == 0) {
		LoadLightColor(buf, &(curMaterial->Light));
		curMaterial->IsLightEmitter = true;
		//log_debug("[DBG] [MAT] Light: %0.3f, %0.3f, %0.3f",
		//	curMaterialTexDef.material.Light.x, curMaterialTexDef.material.Light.y, curMaterialTexDef.material.Light.z);
	}
	else if (_stricmp(param, "light_uv_coord_pos") == 0) {
		LoadUVCoord(buf, &(curMaterial->LightUVCoordPos));
		//log_debug("[DBG] [MAT] LightUVCoordPos: %0.3f, %0.3f",
		//	curMaterial->LightUVCoordPos.x, curMaterial->LightUVCoordPos.y);
	}
	else if (_stricmp(param, "light_falloff") == 0) {
		curMaterial->LightFalloff = fValue;
	}
	else if (_stricmp(param, "light_angle") == 0) {
		curMaterial->LightAngle = cos(fValue * DEG_TO_RAD);
	}
	else if (_stricmp(param, "NoBloom") == 0) {
		curMaterial->NoBloom = (bool)fValue;
		log_debug("[DBG] NoBloom: %d", curMaterial->NoBloom);
	}
	// Shadow Mapping settings
	else if (_stricmp(param, "shadow_map_mult_x") == 0) {
		g_ShadowMapping.shadow_map_mult_x = fValue;
		log_debug("[DBG] [SHW] shadow_map_mult_x: %0.3f", fValue);
	}
	else if (_stricmp(param, "shadow_map_mult_y") == 0) {
		g_ShadowMapping.shadow_map_mult_y = fValue;
		log_debug("[DBG] [SHW] shadow_map_mult_y: %0.3f", fValue);
	}
	else if (_stricmp(param, "shadow_map_mult_z") == 0) {
		g_ShadowMapping.shadow_map_mult_z = fValue;
		log_debug("[DBG] [SHW] shadow_map_mult_z: %0.3f", fValue);
	}
	// Lava Settings
	else if (_stricmp(param, "Lava") == 0) {
		curMaterial->IsLava = (bool)fValue;
	}
	else if (_stricmp(param, "LavaSpeed") == 0) {
		curMaterial->LavaSpeed = fValue;
	}
	else if (_stricmp(param, "LavaSize") == 0) {
		curMaterial->LavaSize = fValue;
	}
	else if (_stricmp(param, "EffectBloom") == 0) {
		// Used for specific effects where specifying bloom is difficult, like the Lava
		// and the AlphaToBloom shaders.
		curMaterial->EffectBloom = fValue;
	}
	else if (_stricmp(param, "LavaColor") == 0) {
		LoadLightColor(buf, &(curMaterial->LavaColor));
	}
	else if (_stricmp(param, "LavaTiling") == 0) {
		curMaterial->LavaTiling = (bool)fValue;
	}
	else if (_stricmp(param, "AlphaToBloom") == 0) {
		// Uses the color alpha to apply bloom. Can be used to force surfaces to glow
		// when they don't have an illumination texture.
		curMaterial->AlphaToBloom = (bool)fValue;
	}
	else if (_stricmp(param, "NoColorAlpha") == 0) {
		// Forces color alpha to 0. Only takes effect when AlphaToBloom is set.
		curMaterial->NoColorAlpha = (bool)fValue;
	}
	else if (_stricmp(param, "AlphaIsntGlass") == 0) {
		// When set, semi-transparent areas aren't converted to glass
		curMaterial->AlphaIsntGlass = (bool)fValue;
	}
	else if (_stricmp(param, "Ambient") == 0) {
		// Additional ambient component. Only used in PixelShaderNoGlass
		curMaterial->Ambient = fValue;
	}
	else if (_stricmp(param, "TotalFrames") == 0) {
		curMaterial->TotalFrames = (int)fValue;
	}
	else if (_stricmp(param, "ExplosionScale") == 0) {
		// The user-facing explosion scale must be translated into the scale that the
		// ExplosionShader users. For some reason (the code isn't mine) the scale in
		// the shader works like this:
		// ExplosionScale: 4.0 == small, 2.0 == normal, 1.0 == big.
		// So the formula is:
		// ExplosionScale = 4.0 / UserExplosionScale
		// Because:
		// 4.0 / UserExplosionScale yields:
		// 4.0 / 1.0 -> 4 = small
		// 4.0 / 2.0 -> 2 = medium
		// 4.0 / 4.0 -> 1 = big
		float UserExplosionScale = max(0.5f, fValue); // Disallow values smaller than 0.5
		curMaterial->ExplosionScale = 4.0f / UserExplosionScale;
	}
	else if (_stricmp(param, "ExplosionSpeed") == 0) {
		curMaterial->ExplosionSpeed = fValue;
	}
	else if (_stricmp(param, "ExplosionBlendMode") == 0) {
		// 0: No Blending, use the original texture
		// 1: Blend with the original texture
		// 2: Replace original texture with procedural explosion
		curMaterial->ExplosionBlendMode = (int)fValue;
	} 
	else if (_stricmp(param, "lightmap_seq") == 0 ||
			 _stricmp(param, "lightmap_rand") == 0 ||
			 _stricmp(param, "lightmap_once") == 0 ||
			 _stricmp(param, "anim_seq") == 0 ||
			 _stricmp(param, "anim_rand") == 0 ||
			 _stricmp(param, "anim_once") == 0) {
		// TOOD: Add support for multiline sequences
		AnimatedTexControl atc;
		int alpha_mode = 0;
		atc.IsRandom = (_stricmp(param, "lightmap_rand") == 0) || (_stricmp(param, "anim_rand") == 0);
		bool IsLightMap = (_stricmp(param, "lightmap_rand") == 0) || (_stricmp(param, "lightmap_seq") == 0);
		atc.NoLoop = _stricmp(param, "lightmap_once") == 0 || _stricmp(param, "anim_once") == 0;
		// Clear the current Sequence and release the associated textures in the DeviceResources...
		// TODO: Either release the ExtraTextures pointed at by LightMapSequence, or garbage-collect them
		//       later...
		atc.Sequence.clear();
		log_debug("[DBG] [MAT] Loading Animated LightMap/Texture data for [%s]", buf);
		if (!LoadTextureSequence(buf, IsLightMap, atc.Sequence, &alpha_mode, &atc))
			log_debug("[DBG] [MAT] Error loading animated LightMap/Texture data for [%s], syntax error?", buf);
		if (atc.Sequence.size() > 0) {
			log_debug("[DBG] [MAT] Sequence.size() = %d for Texture [%s]", atc.Sequence.size(), buf);
			// Initialize the timer for this animation
			atc.ResetAnimation();
			atc.BlackToAlpha = false;
			atc.AlphaIsBloomMask = false;
			//if (atc.OverlayCtrl) {
			//	log_debug("[DBG] [INST] OverlayCtrl: 0x%x", atc.OverlayCtrl);
			//}
			
			switch (alpha_mode) {
			case SPECIAL_CONTROL_BLACK_TO_ALPHA:
				atc.BlackToAlpha = true;
				break;
			case SPECIAL_CONTROL_ALPHA_IS_BLOOM_MASK:
				atc.AlphaIsBloomMask = true;
				break;
			}
			// Add a reference to this material on the list of animated materials
			g_AnimatedMaterials.push_back(atc);
			if (atc.isInstEvent) {
				curMaterial->bInstanceMaterial = true;
				//log_debug("[DBG] [INST] Found instance material in [%s]", OPTname);
			}
			AssignTextureEvent(atc.Event, atc.InstEvent, atc.isInstEvent, curMaterial,
				IsLightMap ? LIGHTMAP_ATC_IDX : TEXTURE_ATC_IDX);

			/*
			log_debug("[DBG] [MAT] >>>>>> Animation Sequence Start");
			for each (TexSeqElem t in atc.LightMapSequence) {
				log_debug("[DBG] [MAT] <%s>, %0.3f, %0.3f", t.texname, t.seconds, t.intensity);
			}
			log_debug("[DBG] [MAT] <<<<<< Animation Sequence End");
			*/
		}
		else {
			log_debug("[DBG] [MAT] ERROR: No Animation Data Loaded for [%s]", buf);
		}
	}
	else if (_stricmp(param, "frame_seq") == 0 ||
			 _stricmp(param, "frame_rand") == 0 ||
			 _stricmp(param, "frame_once") == 0) {
		AnimatedTexControl atc;
		int is_lightmap, alpha_mode;
		atc.IsRandom = _stricmp(param, "frame_rand") == 0;
		atc.NoLoop = _stricmp(param, "frame_once") == 0;
		// Clear the current Sequence and release the associated textures in the DeviceResources...
		// TODO: Either release the ExtraTextures pointed at by LightMapSequence, or garbage-collect them
		//       later...
		atc.Sequence.clear();
		log_debug("[DBG] [MAT] Loading Frame Sequence data for [%s]", buf);
		if (!LoadFrameSequence(buf, atc.Sequence, &is_lightmap, &alpha_mode, &atc))
			log_debug("[DBG] [MAT] Error loading animated LightMap/Texture data for [%s], syntax error?", buf);
		if (atc.Sequence.size() > 0) {
			log_debug("[DBG] [MAT] Sequence.size() = %d for Texture [%s]", atc.Sequence.size(), buf);
			// Initialize the timer for this animation
			atc.ResetAnimation();
			atc.BlackToAlpha = false;
			atc.AlphaIsBloomMask = false;
			//if (atc.OverlayCtrl) {
			//	log_debug("[DBG] [INST] OverlayCtrl: 0x%x", atc.OverlayCtrl);
			//}
			switch (alpha_mode) {
			case SPECIAL_CONTROL_BLACK_TO_ALPHA:
				atc.BlackToAlpha = true;
				break;
			case SPECIAL_CONTROL_ALPHA_IS_BLOOM_MASK:
				atc.AlphaIsBloomMask = true;
				break;
			}
			// Add a reference to this material on the list of animated materials
			g_AnimatedMaterials.push_back(atc);
			if (atc.isInstEvent) {
				curMaterial->bInstanceMaterial = true;
				//log_debug("[DBG] [INST] Found instance material in [%s]", OPTname);
			}
			AssignTextureEvent(atc.Event, atc.InstEvent, atc.isInstEvent, curMaterial,
				is_lightmap ? LIGHTMAP_ATC_IDX : TEXTURE_ATC_IDX);
		}
		else {
			log_debug("[DBG] [MAT] ERROR: No Animation Data Loaded for [%s]", buf);
		}
	}
	else if (_stricmp(param, "alt_frames_1") == 0 ||
			 _stricmp(param, "alt_frames_2") == 0 ||
			 _stricmp(param, "alt_frames_3") == 0 ||
			 _stricmp(param, "alt_frames_4") == 0 ||
			 _stricmp(param, "ds2_frames") == 0)
	{
		AnimatedTexControl atc;
		int AltIdx = (_stricmp(param, "ds2_frames") == 0) ? DS2_ALT_EXPLOSION_IDX : param[11] - '1'; // Get the alt idx (1-4)
		// Avoid loading this animation if we have already loaded it before
		if (curMaterial->AltExplosionIdx[AltIdx] == -1) {
			// Alternate frames are currently only used in explosions. They replace the original explosion
			// animations on a frame-by-frame basis. So a number of fields in the ATC struct are not needed.
			// TODO: Either release the ExtraTextures pointed at by LightMapSequence, or garbage-collect them
			//       later...
			atc.Sequence.clear();
			log_debug("[DBG] [MAT] [ALT] Loading Frame-by-Frame alternate data for [%s] AltIdx: %d", OPTname, AltIdx);
			if (!LoadSimpleFrames(buf, atc.Sequence))
				log_debug("[DBG] [MAT] [ALT] Error loading animated Frame-by-Frame data for [%s], syntax error?", buf);
			if (atc.Sequence.size() > 0) {
				log_debug("[DBG] [MAT] [ALT] Frame-by-Frame Sequence.size() = %d for Texture [%s], AltIdx: %d", atc.Sequence.size(), buf, AltIdx);
				// Add a reference to this material on the list of animated materials
				g_AnimatedMaterials.push_back(atc);
				curMaterial->AltExplosionIdx[AltIdx] = g_AnimatedMaterials.size() - 1;
				log_debug("[DBG] [MAT] [ALT] curMaterial->AltExplosionIdx[%d] = %d", AltIdx, curMaterial->AltExplosionIdx[AltIdx]);
			}
			else {
				log_debug("[DBG] [MAT] [ALT] ERROR: No Animation Data Loaded for [%s], [%s]", OPTname, buf);
			}
		}
		//else
		//	log_debug("[DBG] [MAT] [ALT] Frame-by-Frame data already loaded for [%s], AltIdx: %d", OPTname, AltIdx);
	}
	else if (_stricmp(param, "GreebleTex1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		log_debug("[DBG] [GRB] Loading Greeble 1 Information from %s", svalue);
		strcpy_s(greeble_data->GreebleTexName[0], MAX_GREEBLE_NAME, svalue);
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleTex2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		log_debug("[DBG] [GRB] Loading Greeble 2 Information from %s", svalue);
		strcpy_s(greeble_data->GreebleTexName[1], MAX_GREEBLE_NAME, svalue);
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMap1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		log_debug("[DBG] [GRB] Loading Lightmap Greeble 1 Information from %s", svalue);
		strcpy_s(greeble_data->GreebleLightMapName[0], MAX_GREEBLE_NAME, svalue);
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMap2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		log_debug("[DBG] [GRB] Loading Lightmap Greeble 2 Information from %s", svalue);
		strcpy_s(greeble_data->GreebleLightMapName[1], MAX_GREEBLE_NAME, svalue);
		PrintGreebleData(greeble_data);
	}

	else if (_stricmp(param, "GreebleBlendMode1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		if (isdigit(svalue[0]))
			greeble_data->greebleBlendMode[0] = (GreebleBlendMode)((int)fValue);
		else
			greeble_data->greebleBlendMode[0] = (GreebleBlendMode)GreebleBlendModeToEnum(svalue);
	}
	else if (_stricmp(param, "GreebleBlendMode2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		if (isdigit(svalue[0]))
			greeble_data->greebleBlendMode[1] = (GreebleBlendMode)((int)fValue);
		else
			greeble_data->greebleBlendMode[1] = (GreebleBlendMode)GreebleBlendModeToEnum(svalue);
	}
	else if (_stricmp(param, "GreebleLightMapBlendMode1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		if (isdigit(svalue[0]))
			greeble_data->greebleLightMapBlendMode[0] = (GreebleBlendMode)((int)fValue);
		else
			greeble_data->greebleLightMapBlendMode[0] = (GreebleBlendMode)GreebleBlendModeToEnum(svalue);
	}
	else if (_stricmp(param, "GreebleLightMapBlendMode2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		if (isdigit(svalue[1]))
			greeble_data->greebleLightMapBlendMode[1] = (GreebleBlendMode)((int)fValue);
		else
			greeble_data->greebleLightMapBlendMode[1] = (GreebleBlendMode)GreebleBlendModeToEnum(svalue);
	}

	else if (_stricmp(param, "GreebleDistance1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleDist[0] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleDistance2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleDist[1] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMapDistance1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleLightMapDist[0] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMapDistance2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleLightMapDist[1] = fValue;
		PrintGreebleData(greeble_data);
	}

	else if (_stricmp(param, "GreebleMix1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleMix[0] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleMix2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleMix[1] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMapMix1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleLightMapMix[0] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMapMix2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleLightMapMix[1] = fValue;
		PrintGreebleData(greeble_data);
	}

	else if (_stricmp(param, "GreebleScale1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleScale[0] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleScale2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleScale[1] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMapScale1") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleLightMapScale[0] = fValue;
		PrintGreebleData(greeble_data);
	}
	else if (_stricmp(param, "GreebleLightMapScale2") == 0) {
		GreebleData *greeble_data = GetOrAddGreebleData(curMaterial);
		greeble_data->GreebleLightMapScale[1] = fValue;
		PrintGreebleData(greeble_data);
	}

	else if (_stricmp(param, "NormalMap") == 0) {
		strcpy_s(curMaterial->NormalMapName, MAX_NORMALMAP_NAME, svalue);
		curMaterial->NormalMapLoaded = false;
		//log_debug("[DBG] [MAT] NormalMap set to %s", svalue);
	}
	
	if (_stricmp(param, "JoystickRoot") == 0) {
		curMaterial->DiegeticMesh = DM_JOYSTICK;
		LoadLightColor(buf, &(curMaterial->JoystickRoot));
		// This point must be scaled to match the OPT coord sys
		curMaterial->JoystickRoot *= METERS_TO_OPT;
		//log_debug("[DBG] [MAT] JoystickRoot: %0.3f, %0.3f, %0.3f",
		//	curMaterial->JoystickRoot.x, curMaterial->JoystickRoot.y, curMaterial->JoystickRoot.z);
	}
	else if (_stricmp(param, "JoystickMaxYaw") == 0) {
		curMaterial->JoystickMaxYaw = fValue;
	}
	else if (_stricmp(param, "JoystickMaxPitch") == 0) {
		curMaterial->JoystickMaxPitch = fValue;
	}

	if (_stricmp(param, "ThrottleRotXAxis") == 0) {
		curMaterial->DiegeticMesh = DM_THR_ROT_X;
		LoadLightColor(buf, &(curMaterial->ThrottleRoot));
		curMaterial->ThrottleRoot *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "ThrottleRotYAxis") == 0) {
		curMaterial->DiegeticMesh = DM_THR_ROT_Y;
		LoadLightColor(buf, &(curMaterial->ThrottleRoot));
		curMaterial->ThrottleRoot *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "ThrottleRotZAxis") == 0) {
		curMaterial->DiegeticMesh = DM_THR_ROT_Z;
		LoadLightColor(buf, &(curMaterial->ThrottleRoot));
		curMaterial->ThrottleRoot *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "ThrottleMaxAngle") == 0) {
		curMaterial->ThrottleMaxAngle = fValue;
	}
	else if (_stricmp(param, "`ThrottleStart") == 0) {
		curMaterial->DiegeticMesh = DM_THR_TRANS;
		LoadLightColor(buf, &(curMaterial->ThrottleStart));
		curMaterial->ThrottleStart *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "ThrottleEnd") == 0) {
		curMaterial->DiegeticMesh = DM_THR_TRANS;
		LoadLightColor(buf, &(curMaterial->ThrottleEnd));
		curMaterial->ThrottleEnd *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "ThrottleAxis0") == 0) {
		curMaterial->DiegeticMesh = DM_THR_ROT_ANY;
		LoadLightColor(buf, &(curMaterial->ThrottleStart));
		curMaterial->ThrottleStart *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "ThrottleAxis1") == 0) {
		curMaterial->DiegeticMesh = DM_THR_ROT_ANY;
		LoadLightColor(buf, &(curMaterial->ThrottleEnd));
		curMaterial->ThrottleEnd *= METERS_TO_OPT;
	}
	

	if (_stricmp(param, "HyperRotXAxis") == 0) {
		curMaterial->DiegeticMesh = DM_HYPER_ROT_X;
		LoadLightColor(buf, &(curMaterial->ThrottleRoot));
		curMaterial->ThrottleRoot *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "HyperRotYAxis") == 0) {
		curMaterial->DiegeticMesh = DM_HYPER_ROT_Y;
		LoadLightColor(buf, &(curMaterial->ThrottleRoot));
		curMaterial->ThrottleRoot *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "HyperRotZAxis") == 0) {
		curMaterial->DiegeticMesh = DM_HYPER_ROT_Z;
		LoadLightColor(buf, &(curMaterial->ThrottleRoot));
		curMaterial->ThrottleRoot *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "HyperMaxAngle") == 0) {
		curMaterial->ThrottleMaxAngle = fValue;
	}
	else if (_stricmp(param, "HyperStart") == 0) {
		curMaterial->DiegeticMesh = DM_HYPER_TRANS;
		LoadLightColor(buf, &(curMaterial->ThrottleStart));
		curMaterial->ThrottleStart *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "HyperEnd") == 0) {
		curMaterial->DiegeticMesh = DM_HYPER_TRANS;
		LoadLightColor(buf, &(curMaterial->ThrottleEnd));
		curMaterial->ThrottleEnd *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "HyperAxis0") == 0) {
		curMaterial->DiegeticMesh = DM_HYPER_ROT_ANY;
		LoadLightColor(buf, &(curMaterial->ThrottleStart));
		curMaterial->ThrottleStart *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "HyperAxis1") == 0) {
		curMaterial->DiegeticMesh = DM_HYPER_ROT_ANY;
		LoadLightColor(buf, &(curMaterial->ThrottleEnd));
		curMaterial->ThrottleEnd *= METERS_TO_OPT;
	}

	if (_stricmp(param, "TransformCenter") == 0) {
		curMaterial->meshTransform.bDoTransform = true;
		LoadLightColor(buf, &(curMaterial->meshTransform.Center));
		curMaterial->meshTransform.Center *= METERS_TO_OPT;
	}
	else if (_stricmp(param, "TransformRotX") == 0) {
		curMaterial->meshTransform.bDoTransform = true;
		curMaterial->meshTransform.RotXDelta = fValue;
	}
	else if (_stricmp(param, "TransformRotY") == 0) {
		curMaterial->meshTransform.bDoTransform = true;
		curMaterial->meshTransform.RotYDelta = fValue;
	}
	else if (_stricmp(param, "TransformRotZ") == 0) {
		curMaterial->meshTransform.bDoTransform = true;
		curMaterial->meshTransform.RotZDelta = fValue;
	}

	else if (_stricmp(param, "SkipWhenDisabled") == 0) {
		curMaterial->SkipWhenDisabled = (bool)fValue;
		// This property makes this a per-instance material
		curMaterial->bInstanceMaterial = true;
	}

	else if (_stricmp(param, "DisplayIfSpeedGE") == 0) {
		curMaterial->DisplayIfSpeedGE = (int)fValue;
		// This property makes this a per-instance material
		curMaterial->bInstanceMaterial = true;
	}

	else if (_stricmp(param, "DisplayIfThrottleGE") == 0) {
		curMaterial->DisplayIfThrottleGE = fValue / 100.0f;
		// This property makes this a per-instance material
		curMaterial->bInstanceMaterial = true;
	}

	else if (_stricmp(param, "DisplayIfMissionSetSpeedGE") == 0) {
		curMaterial->DisplayIfMissionSetSpeedGE = (int)fValue;
		// This property makes this a per-instance material
		curMaterial->bInstanceMaterial = true;
	}

	else if (_stricmp(param, "DisplayIfMissionSetSpeedGE") == 0) {
		curMaterial->DisplayIfMissionSetSpeedGE = (int)fValue;
		// This property makes this a per-instance material
		curMaterial->bInstanceMaterial = true;
	}

	else if (_stricmp(param, "IncreaseBrightnessWithMissionSetSpeed") == 0) {
		curMaterial->IncreaseBrightnessWithMissionSetSpeed = (int)fValue;
		// This property makes this a per-instance material
		curMaterial->bInstanceMaterial = true;
	}

	/*
	else if (_stricmp(param, "Raytrace") == 0) {
		curMaterial->Raytrace = (int)fValue;
	}
	*/

	/*
	else if (_stricmp(param, "LavaNormalMult") == 0) {
		LoadLightColor(buf, &(curMaterial->LavaNormalMult));
		log_debug("[DBG] [MAT] LavaNormalMult: %0.3f, %0.3f, %0.3f",
			curMaterial->LavaNormalMult.x, curMaterial->LavaNormalMult.y, curMaterial->LavaNormalMult.z);
	}
	else if (_stricmp(param, "LavaPosMult") == 0) {
		LoadLightColor(buf, &(curMaterial->LavaPosMult));
		log_debug("[DBG] [MAT] LavaPosMult: %0.3f, %0.3f, %0.3f",
			curMaterial->LavaPosMult.x, curMaterial->LavaPosMult.y, curMaterial->LavaPosMult.z);
	}
	else if (_stricmp(param, "LavaTranspose") == 0) {
		curMaterial->LavaTranspose = (bool)fValue;
	}
	*/
}

/// <summary>
/// Load the craft material parameters for an individual OPT or DAT.
/// If successful, it will add a new entry into g_Materials.
/// </summary>
/// <param name="OPTname"></param>
/// <param name="sFileName"></param>
/// <param name="verbose"></param>
/// <returns>Returns true if the file could be loaded successfully</returns>
bool LoadIndividualMATParams(char *OPTname, char *sFileName, bool verbose) {
	// I may have to use std::array<char, DIM> and std::vector<std::array<char, Dim>> instead
	// of TexnameType
	// https://stackoverflow.com/questions/21829451/push-back-on-a-vector-of-array-of-char
	FILE* file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		//log_debug("[DBG] [MAT] Could not load [%s]", sFileName);
	}

	if (error != 0) {
		//log_debug("[DBG] [MAT] Error %d when loading [%s]", error, sFileName);
		return false;
	}

	if (verbose) log_debug("[DBG] [MAT] Loading Craft Material params for [%s]...", sFileName);
	constexpr int MAX_BUF_LEN = 3000;
	char buf[MAX_BUF_LEN], param[256], svalue[MAX_BUF_LEN]; // texname[MAX_TEXNAME];
	std::vector<TexnameType> texnameList;
	int param_read_count = 0;
	float fValue = 0.0f;
	bool bIsExplosion = false;
	MaterialTexDef curMaterialTexDef;

	// Explosions need a different material mechanism. Each frame in the explosion is loaded separately,
	// and on top of that, each frame seems to be loaded multiple times. Explosion materials are defined
	// for the whole animation, so we need to coalesce all frames into a single material slot. The code
	// below has several "unhappy" paths for explosions so that each explosion animation has a single
	// material and the [Default] material gets overwritten every time.
	if (isInVector(OPTname, Explosion_ResNames)) {
		int GroupId, ImageId;
		bIsExplosion = true;
		GetGroupIdImageIdFromDATName(OPTname, &GroupId, &ImageId);
		sprintf_s(OPTname, MAX_OPT_NAME, "dat,%d,0,0,0", GroupId);
	}
	// Find this OPT in the global materials and clear it if necessary...
	int craftIdx = FindCraftMaterial(OPTname);
	if (craftIdx < 0) {
		// New Craft Material
		if (verbose) log_debug("[DBG] [MAT] New Craft Material (%s)", OPTname);
	}
	else {
		// Existing Craft Material, clear it
		if (verbose) log_debug("[DBG] [MAT] Existing Craft Material, %s clearing %s", bIsExplosion ? "NOT" : "", OPTname);
		// We must not clear explosion materials because each frame is loaded multiple times
		if (!bIsExplosion) g_Materials[craftIdx].MaterialList.clear();
	}
	CraftMaterials craftMat;
	if (bIsExplosion) {
		if (craftIdx == -1) {
			craftMat.MaterialList.clear();
			strncpy_s(craftMat.OPTname, OPTname, MAX_OPT_NAME);

			curMaterialTexDef.material = g_DefaultGlobalMaterial;
			strncpy_s(curMaterialTexDef.texname, "Default", MAX_TEXNAME);
			// The default material will always be in slot 0:
			craftMat.MaterialList.push_back(curMaterialTexDef);
		}
		else {
			craftMat = g_Materials[craftIdx];
			curMaterialTexDef = craftMat.MaterialList[0];
		}
	}
	else {
		// Clear the materials for this craft and add a default material
		craftMat.MaterialList.clear();
		strncpy_s(craftMat.OPTname, OPTname, MAX_OPT_NAME);

		curMaterialTexDef.material = g_DefaultGlobalMaterial;
		strncpy_s(curMaterialTexDef.texname, "Default", MAX_TEXNAME);
		// The default material will always be in slot 0:
		craftMat.MaterialList.push_back(curMaterialTexDef);
		//craftMat.MaterialList.insert(craftMat.MaterialList.begin(), materialTexDef);
	}

	// We always start the craft material with one material: the default material in slot 0
	bool MaterialSaved = true;
	texnameList.clear();
	while (fgets(buf, MAX_BUF_LEN, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;
		
		if (sscanf_s(buf, "%s = %s", param, 256, svalue, MAX_BUF_LEN) > 0) {
			fValue = (float)atof(svalue);

			if (buf[0] == '[') {

				//strcpy_s(texname, MAX_TEXNAME, buf + 1);
				// Get rid of the trailing ']'
				//char *end = strstr(texname, "]");
				//if (end != NULL)
				//	*end = 0;
				//log_debug("[DBG] [MAT] texname: %s", texname);

				if (!MaterialSaved) {
					// A new material section begins in this line, but there's a previous
					// material that needs to be saved before proceeding
					for (const TexnameType &texname : texnameList) {
						// Copy the texture name from the list to the current material
						strncpy_s(curMaterialTexDef.texname, texname.name, MAX_TEXNAME);
						// Special case: overwrite the default material
						if (_stricmp("default", curMaterialTexDef.texname) == 0) {
							//log_debug("[DBG] [MAT] Overwriting the default material for this craft");
							craftMat.MaterialList[0] = curMaterialTexDef;
						}
						else {
							//log_debug("[DBG] [MAT] Adding new material: %s", curMaterialTexDef.texname);
							craftMat.MaterialList.push_back(curMaterialTexDef);
						}
					}
				}
				texnameList.clear();

				// Parse the "[TEXNNNNN,TEXMMMMM,...]" line and extract the name(s) of the texture(s)
				// texnameList is populated here
				{
					char* start = buf + 1; // Skip the '[' bracket
					char* end;
					while (*start && *start != ']') {
						end = start;
						// Skip chars until we find ',' ']' or EOS
						while (*end && *end != ',' && *end != ']')
							end++;
						// If we didn't hit EOS, then add the token to the list
						if (*end)
						{
							TexnameType texname;
							strncpy_s(texname.name, start, end - start);
							//log_debug("[DBG] [MAT] Adding texname: [%s]", texname.name);
							texnameList.push_back(texname);
							start = end + 1;
						}
						else
							start = end;
					}

					// DEBUG
					/*log_debug("[DBG] [MAT] ---------------");
					for (uint32_t i = 0; i < texnameList.size(); i++)
						log_debug("[DBG] [MAT] %s, ", texnameList[i].name);
					log_debug("[DBG] [MAT] ===============");*/
					// DEBUG
				}

				// Start a new material -- but not for Explosions. Explosions are loaded multiple times and they all
				// share the Default material
				if (!bIsExplosion) {
					//strncpy_s(curMaterialTexDef.texname, texname, MAX_TEXNAME);
					curMaterialTexDef.texname[0] = 0;
					// For new materials, use the Default material defined for this ship
					curMaterialTexDef.material = craftMat.MaterialList[0].material;
					// If the default material has instance event data, clear it:
					curMaterialTexDef.material.bInstanceMaterial = false;
					curMaterialTexDef.material.bIsDefaultMaterial = false;
					curMaterialTexDef.material.ClearATCIndices();
					// For new materials, use the default global material;
					//curMaterialTexDef.material = g_DefaultGlobalMaterial;
				}
				MaterialSaved = false;
			}
			else {
				ReadMaterialLine(buf, &(curMaterialTexDef.material), OPTname);
			}
		}
	}
	fclose(file);

	// Save the last material if necessary...
	if (bIsExplosion) {
		craftMat.MaterialList[0] = curMaterialTexDef;
	}
	else if (!MaterialSaved) {
		// There's an existing material that needs to be saved before proceeding
		for (const TexnameType &texname : texnameList) {
			// Copy the texture name from the list to the current material
			strncpy_s(curMaterialTexDef.texname, texname.name, MAX_TEXNAME);
			// Special case: overwrite the default material
			if (_stricmp("default", curMaterialTexDef.texname) == 0) {
				//log_debug("[DBG] [MAT] (last) Overwriting the default material for this craft");
				craftMat.MaterialList[0] = curMaterialTexDef;
			}
			else {
				//log_debug("[DBG] [MAT] (last) Adding new material: %s", curMaterialTexDef.texname);
				craftMat.MaterialList.push_back(curMaterialTexDef);
			}
		}
	}
	texnameList.clear();

	// Mark the first material as the default material
	craftMat.MaterialList[0].material.bIsDefaultMaterial = true;

	// DEBUG
	/*
	if (craftMat.MaterialList[0].material.bInstanceMaterial) {
		log_debug("[DBG] [INST] DEFAULT MATERIAL FOR [%s] HAS AN INSTANCE EVENT",
			OPTname);
		for (int i = 0; i < MAX_ATC_TYPES; i++)
			for (int j = 0; j < MAX_INST_EVT; j++) {
				int size = craftMat.MaterialList[0].material.InstTextureATCIndices[i][j].size();
				if (size > 0)
					log_debug("[DBG] [INST] DEFAULT MATERIAL FOR [%s] HAS INSTANCE EVENT [%s]",
						OPTname, g_sInstEventNames[j]);
			}
	}
	*/
	// DEBUG

	// Replace the craft material in g_Materials
	if (craftIdx < 0) {
		//log_debug("[DBG] [MAT] Adding new craft material %s", OPTname);
		g_Materials.push_back(craftMat);
		craftIdx = g_Materials.size() - 1;
	}
	else {
		//log_debug("[DBG] [MAT] Replacing existing craft material %s", OPTname);
		g_Materials[craftIdx] = craftMat;
	}

	// Have each material point back to its parent CraftMaterials entry in g_Materials
	for (auto& mat : g_Materials[craftIdx].MaterialList) {
		mat.material.craftIdx = craftIdx;
	}

	// DEBUG
	// Print out the materials for this craft
	/*log_debug("[DBG] [MAT] *********************");
	log_debug("[DBG] [MAT] Craft Materials for OPT: %s", g_Materials[craftIdx].OPTname);
	for (uint32_t i = 0; i < g_Materials[craftIdx].MaterialList.size(); i++) {
		Material defMat = g_Materials[craftIdx].MaterialList[i].material;
		log_debug("[DBG] [MAT] %s, M:%0.3f, I:%0.3f, G:%0.3f",
			g_Materials[craftIdx].MaterialList[i].texname,
			defMat.Metallic, defMat.Intensity, defMat.Glossiness);
	}
	log_debug("[DBG] [MAT] *********************");*/
	// DEBUG
	return true;
}

bool GetGroupIdImageIdFromDATName(char* DATName, int* GroupId, int* ImageId) {
	// Extract the Group Id and Image Id from the dat name:
	// Sample dat name:
	// [dat,9002,1200,0,0]
	char* idx = strstr(DATName, "dat,");
	if (idx == NULL) {
		log_debug("[DBG] [MAT] Could not find 'dat,' substring in [%s]", DATName);
		return false;
	}
	idx += 4; // Skip the "dat," token, we're now pointing to the first char of the Group Id
	*GroupId = atoi(idx); // Convert the current string to the Group Id

	// Advance to the next comma
	idx = strstr(idx, ",");
	if (idx == NULL) {
		log_debug("[DBG] [MAT] Error while parsing [%s], could not find ImageId", DATName);
		return false;
	}
	idx += 1; // Skip the comma
	*ImageId = atoi(idx); // Convert the current string to the Image Id
	return true;
}

/*
 * Convert a DAT name into two MAT params files of the form:
 *
 * sFileName:		Materials\dat-<GroupId>-<ImageId>.mat
 * sFileNameShort:	Materials\dat-<GroupId>.mat
 *
 * Both sFileName and sFileNameShort must have iFileNameSize bytes to store the string.
 * sFileNameShort can be used to load mat files for animations, like explosions.
 */
void DATNameToMATParamsFile(char *DATName, char *sFileName, char *sFileNameShort, int iFileNameSize) {
	int GroupId, ImageId;
	// Get the GroupId and ImageId from the DATName, nullify sFileName if we can't extract
	// the data from the name
	if (!GetGroupIdImageIdFromDATName(DATName, &GroupId, &ImageId)) {
		sFileName[0] = 0;
		return;
	}
	// Build the material filename for the DAT texture:
	snprintf(sFileName, iFileNameSize, "Materials\\dat-%d-%d.mat", GroupId, ImageId);
	snprintf(sFileNameShort, iFileNameSize, "Materials\\dat-%d.mat", GroupId);
}


/*
void InitCachedMaterials() {
	for (int i = 0; i < MAX_CACHED_MATERIALS; i++)
		g_CachedMaterials[i].texname[0] = 0;
	g_iFirstCachedMaterial = g_iLastCachedMaterial = 0;
}
*/

void InitOPTnames() {
	ClearOPTnames();
}

void ClearOPTnames() {
	g_OPTnames.clear();
}

void InitCraftMaterials() {
	ClearCraftMaterials();
	//log_debug("[DBG] [MAT] g_Materials initialized (cleared)");
}

void ClearCraftMaterials() {
	for (uint32_t i = 0; i < g_Materials.size(); i++) {
		// Release the materials for each craft
		g_Materials[i].MaterialList.clear();
	}
	// Release the global materials
	g_Materials.clear();
	//log_debug("[DBG] [MAT] g_Materials cleared");
}

/*
Find the index where the materials for the specific OPT is loaded, or return -1 if the
OPT isn't loaded yet.
*/
int FindCraftMaterial(char* OPTname) {
	for (uint32_t i = 0; i < g_Materials.size(); i++) {
		if (_stricmp(OPTname, g_Materials[i].OPTname) == 0)
			return i;
	}
	return -1;
}

/// <summary>
/// Find the material in the specified CraftIndex of g_Materials that corresponds to
/// TexName. Returns the default material if it wasn't found or if TexName is null/empty
/// </summary>
/// <param name="CraftIndex"></param>
/// <param name="TexName"></param>
/// <param name="debug"></param>
/// <returns></returns>
Material FindMaterial(int CraftIndex, char* _TexName, bool debug) {
	CraftMaterials* craftMats = &(g_Materials[CraftIndex]);
	char TexName[MAX_TEXNAME + 1];

	// Check for "_fg_" in the texname: that means this texture has a skin
	char* end = stristr(_TexName, "_fg_");
	if (end != NULL)
	{
		// Remove the skin part
		char* start = _TexName;
		int size = end - start;
		strncpy_s(TexName, MAX_TEXNAME, start, size);
		if (debug)
			log_debug("[DBG] [MAT] Removing skin. Now using: %s", TexName);
	}
	else
	{
		strcpy_s(TexName, MAX_TEXNAME, _TexName);
	}

	// Slot 0 should always be present and it should be the default craft material
	Material defMat = craftMats->MaterialList[0].material;
	if (TexName == NULL || TexName[0] == 0)
		return defMat;
	for (uint32_t i = 1; i < craftMats->MaterialList.size(); i++) {
		if (_stricmp(TexName, craftMats->MaterialList[i].texname) == 0) {
			defMat = craftMats->MaterialList[i].material;
			if (debug)
				log_debug("[DBG] [MAT] Material %s found: M:%0.3f, I:%0.3f, G:%0.3f",
					TexName, defMat.Metallic, defMat.Intensity, defMat.Glossiness);
			return defMat;
		}
	}

	if (debug)
		log_debug("[DBG] [MAT] Material %s not found, returning default: M:%0.3f, I:%0.3f, G:%0.3f",
			TexName, defMat.Metallic, defMat.Intensity, defMat.Glossiness);
	return defMat;
}

void AnimatedTexControl::ResetAnimation() {
	this->AnimIdx = 0;
	if (this->Sequence.size() >= 1)
		this->TimeLeft = this->Sequence[0].seconds;
}

void AnimatedTexControl::Animate() {
	int num_frames = this->Sequence.size();
	if (num_frames == 0) return;

	int idx = this->AnimIdx;
	float time = this->TimeLeft - g_HiResTimer.elapsed_s;
	// If the 3D display is paused for an extended period of time (for instance, if
	// the user presses ESC to display the menu for several seconds), then the animation
	// will have to advance for several seconds as well, so we need the "while" below
	// to advance the animation accordingly.
	while (time < 0.0f) {
		if (this->IsRandom)
			idx = rand() % num_frames;
		else {
			if (NoLoop)
				idx = min(num_frames - 1, idx + 1);
			else
				idx = (idx + 1) % num_frames;
		}
		// Use the time specified in the sequence for the next index
		if (this->Sequence[idx].seconds < 0.00001f) { // prevent infinite loops
			time = 0.0f;
		} else
			time += this->Sequence[idx].seconds;
	}
	this->AnimIdx = idx;
	this->TimeLeft = time;
}

bool AnimatedTexControl::IsRandomizableOverlay() {
	if (this->InstEvent == IEVT_SHIELDS_DOWN ||
		this->InstEvent == IEVT_TRACTOR_BEAM ||
		this->InstEvent == IEVT_JAMMING_BEAM)
		return true;
	return false;
}

bool AnimatedTexControl::IsHullDamageEvent()
{
	if (this->InstEvent == IEVT_HULL_DAMAGE_25 ||
		this->InstEvent == IEVT_HULL_DAMAGE_50 ||
		this->InstEvent == IEVT_HULL_DAMAGE_75)
		return true;
	return false;
}

void AnimateMaterials() {
	// I can't use a shorthand loop like the following:
	// for (AnimatedTexControl atc : g_AnimatedMaterials) 
	// because that'll create local copies of each element in the std::vector in atc.
	// In other words, the iterator is a local copy of each element in g_AnimatedMaterials,
	// which doesn't work, because we need to modify the source element
	for (uint32_t i = 0; i < g_AnimatedMaterials.size(); i++) {
		AnimatedTexControl *atc = &(g_AnimatedMaterials[i]);
		// Do not animate instance materials here. Any instance materials in this array
		// are templates.
		if (atc->isInstEvent)
			continue;
		// Reset this ATC if its event fired during the current frame
		if (EventFired(atc->Event))
			atc->ResetAnimation();
		else
			atc->Animate();
	}

	// Animate instance events. This allows independent timers for each instance.
	for (uint32_t i = 0; i < g_AnimatedInstMaterials.size(); i++) {
		AnimatedTexControl *atc = &(g_AnimatedInstMaterials[i]);
		if (atc == nullptr || atc->objectId == -1) {
			//log_debug("[DBG] [INST] SKIPPING animation on instance materials, objectId == -1");
			continue;
		}
		uint64_t Id = InstEventIdFromObjectMaterialId(atc->objectId, atc->materialId);
		auto& it = g_objectIdToInstanceEvent.find(Id);
		if (it != g_objectIdToInstanceEvent.end()) {
			if (it->second.EventFired(atc->InstEvent))
				atc->ResetAnimation();
			else
				atc->Animate();
		}
	}
}

void ClearAnimatedMaterials() {
	for (AnimatedTexControl atc : g_AnimatedMaterials)
		atc.Sequence.clear();
	for (AnimatedTexControl atc : g_AnimatedInstMaterials)
		atc.Sequence.clear();
	g_AnimatedMaterials.clear();
	g_AnimatedInstMaterials.clear();
}

void CockpitInstrumentState::FromXWADamage(WORD XWADamage) {
	CockpitDamage x;
	x.Flags			= XWADamage;
	CMD				= (x.CMD != 0);
	LaserIon		= (x.LaserIon == 0x7);
	BeamWeapon		= (x.BeamWeapon != 0);
	Shields			= (x.Shields != 0);
	Throttle		= (x.Throttle != 0);
	Sensors			= (x.Sensors == 0x3);
	LaserRecharge	= (x.LaserRecharge != 0);
	EnginePower		= (x.EngineLevel != 0);
	ShieldRecharge	= (x.ShieldRecharge != 0);
	BeamRecharge	= (x.BeamRecharge != 0);
}

void ResetGameEvent() {
	//////////////////// GLOBAL EVENTS ////////////////////
	g_GameEvent.TargetEvent = EVT_NONE;
	memset(&(g_GameEvent.CockpitInstruments), 1, sizeof(CockpitInstrumentState));
	g_GameEvent.bCockpitInitialized = false;
	g_GameEvent.HullEvent = EVT_NONE;
	g_GameEvent.WLightLLEvent = false;
	g_GameEvent.WLightMLEvent = false;
	g_GameEvent.WLightMREvent = false;
	g_GameEvent.WLightRREvent = 0;

	for (int i = 0; i < MAX_CANNONS; i++)
		g_GameEvent.CannonReady[i] = false;

	// Add new global events here
	// ...

	// Don't modify the code below this line if you're just adding new events
	// Reset the Events Fired flags
	for (int i = 0; i < MAX_GAME_EVT; i++)
		bEventsFired[i] = false;

	// Always keep this line at the end of the global event section
	g_PrevGameEvent = g_GameEvent;

	//////////////////// INSTANCE EVENTS ////////////////////
	for (auto &it : g_objectIdToInstanceEvent) {
		InstanceEvent &instEvent = it.second;
		instEvent.HullEvent = IEVT_NONE;
		instEvent.ShieldBeamEvent = IEVT_NONE;

		// Add new instance events here
		// ...

		instEvent.ResetEventsFired();
		instEvent.CopyCurrentEventsToPrev();
	}
}

// Check which events were set on the current frame and set the corresponding flags
void UpdateEventsFired() {
	// Set all events to false
	for (int i = 0; i < MAX_GAME_EVT; i++)
		bEventsFired[i] = false;

	// Set the current target event to true if it changed:
	if (g_PrevGameEvent.TargetEvent != g_GameEvent.TargetEvent)
		bEventsFired[g_GameEvent.TargetEvent] = true;

	// Set the current hull damage event to true if it changed:
	if (g_PrevGameEvent.HullEvent != g_GameEvent.HullEvent)
		bEventsFired[g_GameEvent.HullEvent] = true;

	// Manually check and set each damage event
	bEventsFired[CPT_EVT_BROKEN_CMD] = (g_PrevGameEvent.CockpitInstruments.CMD && !g_GameEvent.CockpitInstruments.CMD);
	bEventsFired[CPT_EVT_BROKEN_LASER_ION] = (g_PrevGameEvent.CockpitInstruments.LaserIon && !g_GameEvent.CockpitInstruments.LaserIon);
	bEventsFired[CPT_EVT_BROKEN_BEAM_WEAPON] = (g_PrevGameEvent.CockpitInstruments.BeamWeapon && !g_GameEvent.CockpitInstruments.BeamWeapon);

	bEventsFired[CPT_EVT_BROKEN_SHIELDS] = (g_PrevGameEvent.CockpitInstruments.Shields && !g_GameEvent.CockpitInstruments.Shields);
	bEventsFired[CPT_EVT_BROKEN_THROTTLE] = (g_PrevGameEvent.CockpitInstruments.Throttle && !g_GameEvent.CockpitInstruments.Throttle);
	bEventsFired[CPT_EVT_BROKEN_SENSORS] = (g_PrevGameEvent.CockpitInstruments.Sensors && !g_GameEvent.CockpitInstruments.Sensors);

	bEventsFired[CPT_EVT_BROKEN_LASER_RECHARGE] = (g_PrevGameEvent.CockpitInstruments.LaserRecharge && !g_GameEvent.CockpitInstruments.LaserRecharge);
	bEventsFired[CPT_EVT_BROKEN_ENGINE_POWER] = (g_PrevGameEvent.CockpitInstruments.EnginePower && !g_GameEvent.CockpitInstruments.EnginePower);
	bEventsFired[CPT_EVT_BROKEN_SHIELD_RECHARGE] = (g_PrevGameEvent.CockpitInstruments.ShieldRecharge && !g_GameEvent.CockpitInstruments.ShieldRecharge);
	bEventsFired[CPT_EVT_BROKEN_BEAM_RECHARGE] = (g_PrevGameEvent.CockpitInstruments.BeamRecharge && !g_GameEvent.CockpitInstruments.BeamRecharge);

	// Set the current Warning Light events to true if they changed:
	if (!g_PrevGameEvent.WLightLLEvent && g_GameEvent.WLightLLEvent)
		bEventsFired[WLIGHT_EVT_LEFT] = true;
	if (!g_PrevGameEvent.WLightMLEvent && g_GameEvent.WLightMLEvent)
		bEventsFired[WLIGHT_EVT_MID_LEFT] = true;
	if (!g_PrevGameEvent.WLightMREvent && g_GameEvent.WLightMREvent)
		bEventsFired[WLIGHT_EVT_MID_RIGHT] = true;
	if (g_PrevGameEvent.WLightRREvent != 1 && g_GameEvent.WLightRREvent == 1)
		bEventsFired[WLIGHT_EVT_RIGHT_YELLOW] = true;
	if (g_PrevGameEvent.WLightRREvent != 2 && g_GameEvent.WLightRREvent == 2)
		bEventsFired[WLIGHT_EVT_RIGHT_RED] = true;

	// Trigger the cannon ready events if they changed
	for (int i = 0; i < MAX_CANNONS; i++)
		if (!g_PrevGameEvent.CannonReady[i] && g_GameEvent.CannonReady[i])
			bEventsFired[CANNON_EVT_1_READY + i] = true;

	// Alternate explosions cannot be triggered as events. Instead, we need to do a frame-by-frame replacement.

	// Don't modify this block if you're only adding new events
	{
#ifdef DEBUG_EVENTS
		for (int i = 0; i < MAX_GAME_EVT; i++)
			if (bEventsFired[i]) log_debug("[DBG] --> Event [%s] FIRED", g_sGameEventNames[i]);
#endif
		// Copy the events
		g_PrevGameEvent = g_GameEvent;
	}

	////////////////////// Update instance events //////////////////////
	for (auto &it : g_objectIdToInstanceEvent) {
		InstanceEvent &instEvent = it.second;
		// Set all instance events to false
		instEvent.ResetEventsFired();

		// Update instance events

		// Set the current shield damage/beam event to true if it changed:
		if (instEvent.PrevShieldBeamEvent != instEvent.ShieldBeamEvent) {
			instEvent.bEventsFired[instEvent.ShieldBeamEvent] = true;
			// This random value gets re-computed each time the event gets triggered.
			// Thus it can help us randomize the shields down effect.
			instEvent.rand0 = (float)rand() / RAND_MAX;
			instEvent.rand1 = (float)rand() / RAND_MAX;
			instEvent.rand2 = (float)rand() / RAND_MAX;
		}

		// Set the current hull damage event to true if it changed:
		if (instEvent.PrevHullEvent != instEvent.HullEvent)
			instEvent.bEventsFired[instEvent.HullEvent] = true;

		// Do not modify this block if you're only adding new instance events
		{
#ifdef DEBUG_EVENTS
			for (int i = 0; i < MAX_INST_EVT; i++)
				if (instEvent.bEventsFired[i]) {
					int objectId;
					uint32_t materialId;
					InstEventIdToObjectMatId(it.first, &objectId, &materialId);
					log_debug("[DBG] [INST] ===> InstEvent, object-matId: %d-%d, [%s] FIRED",
						objectId, materialId, g_sInstEventNames[i]);
				}
#endif
			// Copy the instance events
			instEvent.CopyCurrentEventsToPrev();
		}
	}
}

bool EventFired(GameEvent Event) {
	return bEventsFired[Event];
}

Matrix4 MeshTransform::ComputeTransform()
{
	Matrix4 T, R, Transform;

	// Translate the center to the origin
	T.identity(); T.translate(-Center);
	Transform = T;

	R.identity(); R.rotateX(CurRotX);
	Transform = R * Transform;

	R.identity(); R.rotateY(CurRotY);
	Transform = R * Transform;

	R.identity(); R.rotateZ(CurRotZ);
	Transform = R * Transform;

	// Return the system to its original position
	T.identity(); T.translate(Center);
	Transform = T * Transform;

	// We need to transpose the matrix because XwaD3dVertexShader is expecting the
	// matrix in this format
	Transform.transpose();
	return Transform;
}

void MeshTransform::UpdateTransform()
{
	CurRotX += RotXDelta;
	CurRotY += RotYDelta;
	CurRotZ += RotZDelta;

	while (CurRotX > 360.0f) CurRotX -= 360.0f;
	while (CurRotY > 360.0f) CurRotY -= 360.0f;
	while (CurRotZ > 360.0f) CurRotZ -= 360.0f;

	while (CurRotX < 0.0f) CurRotX += 360.0f;
	while (CurRotY < 0.0f) CurRotY += 360.0f;
	while (CurRotZ < 0.0f) CurRotZ += 360.0f;
}