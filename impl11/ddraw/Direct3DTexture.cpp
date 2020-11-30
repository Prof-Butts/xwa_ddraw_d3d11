// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes, 2019

#include "common.h"
#include "../shaders/material_defs.h"
#include "DeviceResources.h"
#include "Direct3DTexture.h"
#include "TextureSurface.h"
#include "MipmapSurface.h"
#include <comdef.h>
//#include <shlwapi.h>

#include <ScreenGrab.h>
#include <WICTextureLoader.h>
#include <wincodec.h>
#include <vector>

extern ShadowMapVertexShaderMatrixCB g_ShadowMapVSCBuffer;
extern float SHADOW_OBJ_SCALE;
extern std::vector<Vector4> g_OBJLimits;

const char *TRIANGLE_PTR_RESNAME = "dat,13000,100,";
const char *TARGETING_COMP_RESNAME = "dat,12000,1100,";

std::vector<ColorLightPair> g_TextureVector;
/*
 * Used to store a list of textures for fast lookup. For instance, all suns must
 * have their associated lights reset after jumping through hyperspace; and all
 * textures with materials can be placed here so that material properties can be
 * applied while flying.
 */
std::vector<Direct3DTexture *> g_AuxTextureVector;

/*
hook_reticle mapping:
Reticle_5 = 5
Reticle_6 = 6
Reticle_7 = 7
Reticle_8 = 8
Reticle_9 = 9
Reticle_10 = 10
Reticle_15 = 15
Reticle_16 = 16
Reticle_17 = 17
Reticle_18 = 18
Reticle_19 = 19
Reticle_20 = 20
Reticle_21 = 21
Reticle_22 = 22
*/
std::vector<char *> Reticle_ResNames = {
	"dat,12000,500,",  // 0xdcb8e4f4, // Main Laser reticle.
	"dat,12000,600,",  // 0x0793c7d6, // Semi circles that indicate target is ready to be fired upon.
	"dat,12000,700,",  // 0xa4870ab3, // Main Warhead reticle.
	"dat,12000,800,",  // 0x756c8f81, // Warhead semi-circles that indicate lock is being acquired.
	"dat,12000,900,",  // 0x6acc3e3a, // Green dot for next laser available.
	"dat,12000,1000,", // 0x19f6f5a2, // Next laser available to fire.
	"dat,12000,1500,", // 0x1c5e0b86, // Laser warning indicator, left.
	"dat,12000,1600,", // 0xc54d8171, // Laser warning indicator, mid-left.
	"dat,12000,1700,", // 0xf4388255, // Laser warning indicator, mid-right.
	"dat,12000,1800,", // 0xee802582, // Laser warning indicator, right.
	"dat,12000,1900,", // 0x671e8041, // Warhead top indicator, left.
	"dat,12000,2000,", // 0x6cd5d81f, // Warhead top indicator, mid-left,right
	"dat,12000,2100,", // 0x6cd5d81f, // Warhead top indicator, mid-left,right
	"dat,12000,2200,", // 0xc33a94b3, // Warhead top indicator, right.
};

std::vector<char *> ReticleCenter_ResNames = {
	"dat,12000,500,",  // 0xdcb8e4f4, // Main Laser reticle.
	"dat,12000,700,",  // 0xa4870ab3, // Main Warhead reticle.
};

std::vector<char *> CustomReticleCenter_ResNames;

std::vector<char *> Text_ResNames = {
	"dat,16000,"
};

std::vector<char *> Floating_GUI_ResNames = {
	"dat,12000,2400,", // 0xd08b4437, (16x16) Laser charge. (master branch)
	"dat,12000,2300,", // 0xd0168df9, (64x64) Laser charge boxes.
	"dat,12000,2500,", // 0xe321d785, (64x64) Laser and ion charge boxes on B - Wing. (master branch)
	"dat,12000,2600,", // 0xca2a5c48, (8x8) Laser and ion charge on B - Wing. (master branch)
	"dat,12000,1100,", // 0x3b9a3741, (256x128) Full targetting computer, solid. (master branch)
	"dat,12000,100,",  // 0x7e1b021d, (128x128) Left targetting computer, solid. (master branch)
	"dat,12000,200,",  // 0x771a714,  (256x256) Left targetting computer, frame only. (master branch)
};

// List of regular GUI elements (this is not an exhaustive list). It's mostly used to detect when
// the game has started rendering the GUI
std::vector<char *> GUI_ResNames = {
	"dat,12000,2700,", // 0xc2416bf9, // (256x32) Top-left bracket (master branch)
	"dat,12000,2800,", // 0x71ce88f1, // (256x32) Top-right bracket (master branch)
	"dat,12000,4500,", // 0x75b9e062, // (128x128) Left radar (master branch)
	"dat,12000,300,",  // (128x128) Left sensor when... no shields are present?
	"dat,12000,4600,", // 0x1ec963a9, // (128x128) Right radar (master branch)
	"dat,12000,400,",  // 0xbe6846fb, // Right radar when no tractor beam is present
	"dat,12000,4300,", // 0x3188119f, // (128x128) Left Shield Display (master branch)
	"dat,12000,4400,", // 0x75082e5e, // (128x128) Right Tractor Beam Display (master branch)
};

// List of common roots for the electricity names
std::vector<char *> Electricity_ResNames = {
	// Animations (electricity)
	"dat,2007,",
	"dat,2008,",
	"dat,3005,",
	//"dat,3051,", // Hyperspace!
	
	// Sparks
	"dat,3000,",
	"dat,3001,",
	"dat,3002,",

	// Backdrops
	/*"dat,9001,",
	"dat,9002,",
	"dat,9003,",
	"dat,9004,",
	"dat,9005,",
	"dat,9006,",
	"dat,9007,",
	"dat,9008,",
	"dat,9009,",
	"dat,9010,",
	"dat,9100,",*/
	// Particles
	"dat,22000,",
	"dat,22003,",
	"dat,22005,",
	//"dat,22007,", // Cockpit sparks
};

// List of common roots for the Explosion names
std::vector<char *> Explosion_ResNames = {
	// Explosions (these are all animated)
	"dat,2000,",
	"dat,2001,",
	"dat,2002,",
	"dat,2003,",
	"dat,2004,",
	"dat,2005,",
	"dat,2006,",
};

// Smoke from explosions:
std::vector<char *> Smoke_ResNames = {
	"dat,3003,",
	"dat,3004,",
	// The following used to be tagged as explosions, but they look like smoke
	"dat,3006,",
	"dat,3055,",
	"dat,3100,",
	"dat,3200,",
	"dat,3300,",
	"dat,3400,",
	"dat,3500,",
};

std::vector<char *> Sparks_ResNames = {
	"dat,3000,",
	"dat,3001,",
	"dat,3002,",
	"dat,3003,",
	"dat,3004,",
};

// List of Lens Flare effects
std::vector<char *> LensFlare_ResNames = {
	"dat,1000,3,",
	"dat,1000,4,",
	"dat,1000,5,",
	"dat,1000,6,",
	"dat,1000,7,",
	"dat,1000,8,",
};

// List of Suns in the Backdrop.dat file
std::vector<char *> Sun_ResNames = {
	"dat,9001,",
	"dat,9002,",
	"dat,9003,",
	"dat,9004,",
	"dat,9005,",
	"dat,9006,",
	"dat,9007,",
	"dat,9008,",
	"dat,9009,",
	"dat,9010,",
};

/*
// With a few exceptions, all planets are in the dat,6XXX, series. It's 
// probably more efficient to parse that algorithmically.
std::vector<char *> Planet_ResNames = {
	"dat,6010,",
	"dat,6010,",
	...
};
*/

std::vector<char *> SpaceDebris_ResNames = {
	"dat,4000,",
	"dat,4001,",
	"dat,4002,",
	"dat,4003,"
};

std::vector<char *> Trails_ResNames = {
	"dat,21000,",
	"dat,21005,",
	"dat,21010,",
	"dat,21015,",
	"dat,21020,",
	"dat,21025,",
};

bool GetGroupIdImageIdFromDATName(char *DATName, int *GroupId, int *ImageId);

// DYNAMIC COCKPIT
// g_DCElements is used when loading textures to load the cover texture.
extern dc_element g_DCElements[MAX_DC_SRC_ELEMENTS];
extern int g_iNumDCElements;
extern bool g_bDynCockpitEnabled, g_bReshadeEnabled;
extern char g_sCurrentCockpit[128];
extern DCHUDRegions g_DCHUDRegions;
bool LoadIndividualDCParams(char *sFileName);
void CockpitNameToDCParamsFile(char *CockpitName, char *sFileName, int iFileNameSize);

// ACTIVE COCKPIT
extern ac_element g_ACElements[MAX_AC_TEXTURES_PER_COCKPIT];
extern int g_iNumACElements;
extern bool g_bActiveCockpitEnabled;
bool LoadIndividualACParams(char *sFileName);
void CockpitNameToACParamsFile(char *CockpitName, char *sFileName, int iFileNameSize);

extern bool g_b3DSunPresent, g_b3DSkydomePresent;

// MATERIALS
// Contains all the materials for all the OPTs currently loaded
std::vector<CraftMaterials> g_Materials;
// List of all the OPTs seen so far
std::vector<OPTNameType> g_OPTnames;
void OPTNameToMATParamsFile(char *OPTName, char *sFileName, int iFileNameSize);
void DATNameToMATParamsFile(char *DATName, char *sFileName, char *sFileNameShort, int iFileNameSize);
bool LoadIndividualMATParams(char *OPTname, char *sFileName, bool verbose=true);

// SHADOW MAPPING;
extern ShadowMappingData g_ShadowMapping;

// METRIC RECONSTRUCTION:
extern bool g_bYCenterHasBeenFixed;

bool isInVector(uint32_t crc, std::vector<uint32_t> &vector) {
	for (uint32_t x : vector)
		if (x == crc)
			return true;
	return false;
}

bool isInVector(char *name, std::vector<char *> &vector) {
	for (char *x : vector)
		if (stristr(name, x) != NULL)
			return true;
	return false;
}

int isInVector(char *name, dc_element *dc_elements, int num_elems) {
	for (int i = 0; i < num_elems; i++) {
		if (stristr(name, dc_elements[i].name) != NULL)
			return i;
	}
	return -1;
}

int isInVector(char *name, ac_element *ac_elements, int num_elems) {
	for (int i = 0; i < num_elems; i++) {
		if (stristr(name, ac_elements[i].name) != NULL)
			return i;
	}
	return -1;
}

bool isInVector(char *OPTname, std::vector<OPTNameType> &vector) {
	for (OPTNameType x : vector)
		if (_stricmp(OPTname, x.name) == 0) // We need to avoid substrings because OPTs can be "Awing", "AwingExterior", "AwingCockpit"
			return true;
	return false;
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
int FindCraftMaterial(char *OPTname) {
	for (uint32_t i = 0; i < g_Materials.size(); i++) {
		if (_stricmp(OPTname, g_Materials[i].OPTname) == 0)
			return i;
	}
	return -1;
}

/*
Find the material in the specified CraftIndex of g_Materials that corresponds to
TexName. Returns the default material if it wasn't found or if TexName is null/empty
*/
Material FindMaterial(int CraftIndex, char *TexName, bool debug=false) {
	CraftMaterials *craftMats = &(g_Materials[CraftIndex]);
	// Slot should always be present and it should be the default craft material
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

#ifdef DBG_VR
/*
void DumpTexture(ID3D11DeviceContext *context, ID3D11Resource *texture, int index) {
	// DBG: Hack: Save the texture and its CRC
	wchar_t filename[80];
	swprintf_s(filename, 80, L"c:\\temp\\Load-img-%d.png", index);
	DirectX::SaveWICTextureToFile(context, texture, GUID_ContainerFormatPng, filename);
	log_debug("[DBG] Saved Texture: %d", index);
}
*/
#endif

/*
 Converts an index number as specified in the custom reticle files to a slot index
 as stored in HUD.dat.
*/
int ReticleIndexToHUDSlot(int ReticleIndex) {
	// I hate this stupid numbering system
	// The reticles start at 5151
	// Every 14 slots, the group number will increase
	// Every 100th slot, the group number will *also* increase, because, hey, why the hell not?
	// This leads to this very stupid and complex algorithm:
	int sub_slot = (ReticleIndex - 51) / 14;
	int super_slot = (ReticleIndex / 100);
	int slot_mod = (ReticleIndex % 100);
	int slot = 5100 + 100 * sub_slot + 100 * super_slot + slot_mod;
	log_debug("[DBG] [RET] index: %d, sub_slot: %d, super_slot: %d, slot_mod: %d slot: %d",
		ReticleIndex, sub_slot, super_slot, slot_mod, slot);
	return slot;
}

/*
 Converts an index number as specified in the custom reticle files to a HUD resname
 in the form "dat,12000,<slot>,"
*/
void ReticleIndexToHUDresname(int ReticleIndex, char *out_str, int out_str_len) {
	int slot = ReticleIndexToHUDSlot(ReticleIndex);
	sprintf_s(out_str, out_str_len, "dat,12000,%d,", slot);
}

void ClearCustomReticleCenterResNames() {
	for (char *res : CustomReticleCenter_ResNames) {
		if (res != NULL) {
			delete[] res;
			res = NULL;
		}
	}
	CustomReticleCenter_ResNames.clear();
}

bool LoadReticleTXTFile(char *sFileName) {
	log_debug("[DBG] [RET] Loading Reticle Text file...");
	FILE *file;
	int error = 0;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [RET] Could not load %s", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] [RET] Error %d when loading %s", error, sFileName);
		return false;
	}

	// The reticle file exists, let's parse it
	char buf[160], param[80], svalue[80], *ResName;
	int param_read_count = 0;
	int iValue = 0;

	ClearCustomReticleCenterResNames();
	while (fgets(buf, 160, file) != NULL) {
		// Skip comments and blank lines
		if (buf[0] == ';')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 80, svalue, 80) > 0) {
			iValue = atoi(svalue);
			if (_stricmp(param, "Reticle_5") == 0) {
				log_debug("[DBG] [RET] Reticle_5: %d", iValue);
				ResName = new char[80];
				ReticleIndexToHUDresname(iValue, ResName, 80);
				CustomReticleCenter_ResNames.push_back(ResName);
				log_debug("[DBG] [RET] Custom Reticle Center: %s Added", ResName);
			}
			else if (_stricmp(param, "Reticle_7") == 0) {
				log_debug("[DBG] [RET] Reticle_7: %d", iValue);
				ResName = new char[80];
				ReticleIndexToHUDresname(iValue, ResName, 80);
				CustomReticleCenter_ResNames.push_back(ResName);
				log_debug("[DBG] [RET] Custom Reticle Center: %s Added", ResName);
			}
		}
	}
	fclose(file);
	return true;
}

void LoadCustomReticle(char *sCurrentCockpit) {
	char sFileName[80], sCurrent[80];
	int len;
	ClearCustomReticleCenterResNames();

	strcpy_s(sCurrent, sCurrentCockpit);
	len = strlen(sCurrent);
	// Remove the "Cockpit" from the name:
	sCurrent[len - 7] = 0;
	// Look in the Reticle.txt file
	snprintf(sFileName, 80, "./FlightModels/%sReticle.txt", sCurrent);
	log_debug("[DBG] [RET] Loading file: %s", sFileName);
	if (LoadReticleTXTFile(sFileName))
		return;

	log_debug("[DBG] [RET] Could not load %s, searching the INI file for custom reticles", sFileName);

	// Look in the INI file
	snprintf(sFileName, 80, "./FlightModels/%s.ini", sCurrent);
	log_debug("[DBG] [RET] Loading file: %s", sFileName);
	if (!LoadReticleTXTFile(sFileName)) {
		log_debug("[DBG] [RET] Could not load %s. No custom reticles for this cockpit", sFileName);
	}
}

bool Direct3DTexture::LoadShadowOBJ(char *sFileName) {
	FILE *file;
	int error = 0;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [SHW] Could not load file %s", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] [SHW] Error %d when loading %s", error, sFileName);
		return false;
	}

	std::vector<D3DTLVERTEX> vertices;
	std::vector<WORD> indices;
	float minx = 100000.0f, miny = 100000.0f, minz = 100000.0f;
	float maxx = -100000.0f, maxy = -100000.0f, maxz = -100000.0f;
	float rangex, rangey, rangez;

	char line[256];
	while (!feof(file)) {
		fgets(line, 256, file);
		if (line[0] == 'v') {
			D3DTLVERTEX v;
			float x, y, z;
			sscanf_s(line, "v %f %f %f", &x, &y, &z);
			// g_ShadowMapping.shadow_map_mult_x/y/z are supposed to be either 1 or -1
			x *= g_ShadowMapping.shadow_map_mult_x * SHADOW_OBJ_SCALE;
			y *= g_ShadowMapping.shadow_map_mult_y * SHADOW_OBJ_SCALE;
			z *= g_ShadowMapping.shadow_map_mult_z * SHADOW_OBJ_SCALE;

			v.sx = x;
			v.sy = y;
			v.sz = z;

			vertices.push_back(v);
			if (x < minx) minx = x; 
			if (y < miny) miny = y; 
			if (z < minz) minz = z; 
			
			if (x > maxx) maxx = x;
			if (y > maxy) maxy = y;
			if (z > maxz) maxz = z;
		}
		else if (line[0] == 'f') {
			int i, j, k;
			sscanf_s(line, "f %d %d %d", &i, &j, &k);
			indices.push_back((WORD)(i - 1));
			indices.push_back((WORD)(j - 1));
			indices.push_back((WORD)(k - 1));
		}
	}
	fclose(file);
	
	rangex = maxx - minx;
	rangey = maxy - miny;
	rangez = maxz - minz;
	//g_ShadowMapVSCBuffer.OBJrange = max(max(rangex, rangey), rangez);

	log_debug("[DBG] [SHW] Loaded %d vertices, %d faces.",
		vertices.size(), indices.size() / 3);
	log_debug("[DBG] [SHW] min,max: [%0.3f, %0.3f], [%0.3f, %0.3f], [%0.3f, %0.3f]",
		minx, maxx, miny, maxy, minz, maxz);
	log_debug("[DBG] [SHW] range-x,y,z: %0.3f, %0.3f, %0.3f",
		rangex, rangey, rangez);

	g_ShadowMapping.bUseShadowOBJ = true;
	log_debug("[DBG] [SHW] Shadow Map OBJ loaded, enabling shadow mapping");
	this->_deviceResources->CreateShadowVertexIndexBuffers(vertices.data(), indices.data(), vertices.size(), indices.size());
	vertices.clear();
	indices.clear();

	g_OBJLimits.clear();
	// Build a box with the limits. This box can be transformed later to find the 2D limits
	// of the shadow map
	g_OBJLimits.push_back(Vector4(minx, miny, minz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, miny, minz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, maxy, minz, 1.0f));
	g_OBJLimits.push_back(Vector4(minx, maxy, minz, 1.0f));

	g_OBJLimits.push_back(Vector4(minx, miny, maxz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, miny, maxz, 1.0f));
	g_OBJLimits.push_back(Vector4(maxx, maxy, maxz, 1.0f));
	g_OBJLimits.push_back(Vector4(minx, maxy, maxz, 1.0f));
	return true;
}

char* convertFormat(char* src, DWORD width, DWORD height, DXGI_FORMAT format)
{
	int length = width * height;
	char* buffer = new char[length * 4];

	if (format == BACKBUFFER_FORMAT)
	{
		memcpy(buffer, src, length * 4);
	}
	else if (format == DXGI_FORMAT_B4G4R4A4_UNORM)
	{
		unsigned short* srcBuffer = (unsigned short*)src;
		unsigned int* destBuffer = (unsigned int*)buffer;

		for (int i = 0; i < length; ++i)
		{
			unsigned short color16 = srcBuffer[i];

			destBuffer[i] = convertColorB4G4R4A4toB8G8R8A8(color16);
		}
	}
	else if (format == DXGI_FORMAT_B5G5R5A1_UNORM)
	{
		unsigned short* srcBuffer = (unsigned short*)src;
		unsigned int* destBuffer = (unsigned int*)buffer;

		for (int i = 0; i < length; ++i)
		{
			unsigned short color16 = srcBuffer[i];

			destBuffer[i] = convertColorB5G5R5A1toB8G8R8A8(color16);
		}
	}
	else if (format == DXGI_FORMAT_B5G6R5_UNORM)
	{
		unsigned short* srcBuffer = (unsigned short*)src;
		unsigned int* destBuffer = (unsigned int*)buffer;

		for (int i = 0; i < length; ++i)
		{
			unsigned short color16 = srcBuffer[i];

			destBuffer[i] = convertColorB5G6R5toB8G8R8A8(color16);
		}
	}
	else
	{
		memset(buffer, 0, length * 4);
	}

	return buffer;
}

Direct3DTexture::Direct3DTexture(DeviceResources* deviceResources, TextureSurface* surface)
{
	this->_refCount = 1;
	this->_deviceResources = deviceResources;
	this->_surface = surface;
	this->is_Tagged = false;
	this->TagCount = 3;
	this->is_Reticle = false;
	this->is_ReticleCenter = false;
	this->is_HighlightedReticle = false;
	this->is_TrianglePointer = false;
	this->is_Text = false;
	this->is_Floating_GUI = false;
	this->is_GUI = false;
	this->is_TargetingComp = false;
	this->is_Laser = false;
	this->is_TurboLaser = false;
	this->is_LightTexture = false;
	this->is_EngineGlow = false;
	this->is_Electricity = false;
	this->is_Explosion = false;
	this->is_Smoke = false;
	this->is_CockpitTex = false;
	this->is_GunnerTex = false;
	this->is_Exterior = false;
	this->is_HyperspaceAnim = false;
	this->is_FlatLightEffect = false;
	this->is_LensFlare = false;
	this->is_Sun = false;
	this->is_3DSun = false;
	//this->AssociatedXWALight = -1;
	this->is_Debris = false;
	this->is_Trail = false;
	this->is_Spark = false;
	this->is_CockpitSpark = false;
	this->is_Chaff = false;
	this->is_Missile = false;
	this->is_GenericSSAOMasked = false;
	this->is_Skydome = false;
	this->is_SkydomeLight = false;
	this->is_BlastMark = false;
	this->is_DS2_Reactor_Explosion = false;
	//this->is_DS2_Energy_Field = false;
	this->ActiveCockpitIdx = -1;
	this->AuxVectorIndex = -1;
	// Dynamic cockpit data
	this->DCElementIndex = -1;
	this->is_DynCockpitDst = false;
	this->is_DynCockpitAlphaOverlay = false;
	this->is_DC_HUDRegionSrc = false;
	this->is_DC_TargetCompSrc = false;
	this->is_DC_LeftSensorSrc = false;
	this->is_DC_RightSensorSrc = false;
	this->is_DC_ShieldsSrc = false;
	this->is_DC_SolidMsgSrc = false;
	this->is_DC_BorderMsgSrc = false;
	this->is_DC_LaserBoxSrc = false;
	this->is_DC_IonBoxSrc = false;
	this->is_DC_BeamBoxSrc = false;
	this->is_DC_TopLeftSrc = false;
	this->is_DC_TopRightSrc = false;

	this->bHasMaterial = false;
	// Create the default material for this texture
	this->material.Glossiness = DEFAULT_GLOSSINESS;
	this->material.Intensity  = DEFAULT_SPEC_INT;
	this->material.Metallic   = DEFAULT_METALLIC;
}

int Direct3DTexture::GetWidth() {
	return this->_surface->_width;
}

int Direct3DTexture::GetHeight() {
	return this->_surface->_height;
}

Direct3DTexture::~Direct3DTexture()
{
	const auto &resources = this->_deviceResources;
	
	//if (this->is_CockpitTex) {
		// There's code in ResetDynamicCockpit that prevents resetting it multiple times.
		// In other words, ResetDynamicCockpit is idempotent.
		// ResetDynamicCockpit will also reset the Active Cockpit
		//resources->ResetDynamicCockpit();
	//}
	/* 
	We can't reliably reset DC here because the textures may not be reloaded and then we
	just disable DC. This happens if hook_60fps.dll is used. We need to be smarter and
	detect when the cockpit name has changed. If it has, then we reset DC and reload the
	elements.
	*/
	//log_debug("[DBG] [DC] Destroying texture %s", this->_surface->_name);
}

HRESULT Direct3DTexture::QueryInterface(
	REFIID riid,
	LPVOID* obp
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tE_NOINTERFACE");
	LogText(str.str());
#endif

	return E_NOINTERFACE;
}

ULONG Direct3DTexture::AddRef()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount++;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	return this->_refCount;
}

ULONG Direct3DTexture::Release()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount--;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	if (this->_refCount == 0)
	{
		delete this;
		return 0;
	}

	return this->_refCount;
}

HRESULT Direct3DTexture::Initialize(
	LPDIRECT3DDEVICE lpD3DDevice,
	LPDIRECTDRAWSURFACE lpDDSurface)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DTexture::GetHandle(
	LPDIRECT3DDEVICE lpDirect3DDevice,
	LPD3DTEXTUREHANDLE lpHandle)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpHandle == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	*lpHandle = (D3DTEXTUREHANDLE)this;

	return D3D_OK;
}

HRESULT Direct3DTexture::PaletteChanged(
	DWORD dwStart,
	DWORD dwCount)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

void Direct3DTexture::TagTexture() {
	TextureSurface *surface = this->_surface;
	auto &resources = this->_deviceResources;
	this->is_Tagged = true;
	
	// DEBUG: Remove later!
	//log_debug("[DBG] %s", surface->_name);
	{
		// Capture the textures
#ifdef DBG_VR

		{
			static int TexIndex = 0;
			wchar_t filename[300];
			swprintf_s(filename, 300, L"c:\\XWA-Tex-w-names-3\\img-%d.png", TexIndex);
			saveSurface(filename, (char *)textureData[0].pSysMem, surface->_width, surface->_height, bpp);

			char buf[300];
			sprintf_s(buf, 300, "c:\\XWA-Tex-w-names-3\\data-%d.txt", TexIndex);
			FILE *file;
			fopen_s(&file, buf, "wt");
			fprintf(file, "0x%x, size: %d, %d, name: '%s'\n", crc, surface->_width, surface->_height, surface->_name);
			fclose(file);

			TexIndex++;
		}
#endif

		if (strstr(surface->_name, TRIANGLE_PTR_RESNAME) != NULL)
			this->is_TrianglePointer = true;
		if (strstr(surface->_name, TARGETING_COMP_RESNAME) != NULL)
			this->is_TargetingComp = true;
		if (isInVector(surface->_name, Reticle_ResNames))
			this->is_Reticle = true; // Standard Reticle from Reticle_ResNames
		if (isInVector(surface->_name, Text_ResNames))
			this->is_Text = true;
		if (isInVector(surface->_name, ReticleCenter_ResNames)) {
			this->is_ReticleCenter = true; // Standard Reticle Center
			log_debug("[DBG] [RET] %s is a Regular Reticle Center", surface->_name);
		}
		if (isInVector(surface->_name, CustomReticleCenter_ResNames)) {
			this->is_ReticleCenter = true; // Custom Reticle Center
			log_debug("[DBG] [RET] %s is a Custom Reticle Center", surface->_name);
			// Stop tagging this texture
			this->is_Tagged = true;
			this->TagCount = 0;
		}

		/*
		 * TODO: For custom reticles, I would need to load the list of reticles from either the
		 * ship reticle cfg or ship ini file, look for index 6 and test against the name in there.
		 * Of course, this will only work if we already know the ship's name, or we won't be able
		 * to find the file we need to load... Maybe I need to ask Jeremy if this information is
		 * stored somewhere in-memory... For now, only the standard HUD is supported!
		 */
		if (strstr(surface->_name, "dat,12000,600,") != NULL) // TODO: how to do this for custom reticles?
			this->is_HighlightedReticle = true;

		if (isInVector(surface->_name, Floating_GUI_ResNames))
			this->is_Floating_GUI = true;
		if (isInVector(surface->_name, GUI_ResNames))
			this->is_GUI = true;

		// Catch the engine glow and mark it
		if (strstr(surface->_name, "dat,1000,1,") != NULL)
			this->is_EngineGlow = true;
		// Catch the flat light effect
		if (strstr(surface->_name, "dat,1000,2") != NULL)
			this->is_FlatLightEffect = true;
		// Catch the electricity textures and mark them
		if (isInVector(surface->_name, Electricity_ResNames))
			this->is_Electricity = true;
		// Catch the explosion textures and mark them
		if (isInVector(surface->_name, Explosion_ResNames))
			this->is_Explosion = true;
		if (isInVector(surface->_name, Smoke_ResNames))
			this->is_Smoke = true;
		// Catch the lens flare and mark it
		if (isInVector(surface->_name, LensFlare_ResNames))
			this->is_LensFlare = true;
		// Catch the hyperspace anim and mark it
		if (strstr(surface->_name, "dat,3051,") != NULL)
			this->is_HyperspaceAnim = true;
		// Catch the backdrup suns and mark them
		if (isInVector(surface->_name, Sun_ResNames)) {
			this->is_Sun = true;
			//g_AuxTextureVector.push_back(this); // We're no longer tracking which textures are Sun-textures
		}
		// Catch the space debris
		if (isInVector(surface->_name, SpaceDebris_ResNames))
			this->is_Debris = true;
		// Catch DAT files
		if (strstr(surface->_name, "dat,") != NULL) {
			int GroupId, ImageId;
			this->is_DAT = true;
			// Check if this DAT image is a custom reticle
			if (GetGroupIdImageIdFromDATName(surface->_name, &GroupId, &ImageId)) {
				if (GroupId == 12000 && ImageId > 5000) {
					this->is_Reticle = true; // Custom Reticle
					//log_debug("[DBG] CUSTOM RETICLE: %s", surface->_name);
					// This candidate may be a custom reticle center, let's tag it a few more times so that
					// we can see the current cockpit name and check this texture to see if it's a custom
					// reticle center
					if (this->TagCount > 0) {
						this->is_Tagged = false;
						this->TagCount--;
					}
					//else
					//	log_debug("[DBG] [RET] %s will not be tagged anymore", surface->_name);
				}
			}
		}
		// Catch blast marks
		if (strstr(surface->_name, "dat,3050,") != NULL)
			this->is_BlastMark = true;
		// Catch the trails
		if (isInVector(surface->_name, Trails_ResNames))
			this->is_Trail = true;
		// Catch the sparks
		if (isInVector(surface->_name, Sparks_ResNames))
			this->is_Spark = true;
		if (strstr(surface->_name, "dat,22007,") != NULL)
			this->is_CockpitSpark = true;
		if (strstr(surface->_name, "dat,5000,") != NULL)
			this->is_Chaff = true;
		if (strstr(surface->_name, "dat,17002,") != NULL) // DSII reactor core explosion animation textures
			this->is_DS2_Reactor_Explosion = true;
		//if (strstr(surface->_name, "dat,17001,") != NULL) // DSII reactor core explosion animation textures
		//	this->is_DS2_Energy_Field = true;
		
		/* Special handling for Dynamic Cockpit source HUD textures */
		if (g_bDynCockpitEnabled || g_bReshadeEnabled) {
			if (stristr(surface->_name, DC_TARGET_COMP_SRC_RESNAME) != NULL) {
				this->is_DC_TargetCompSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if ((stristr(surface->_name, DC_LEFT_SENSOR_SRC_RESNAME) != NULL) ||
				(stristr(surface->_name, DC_LEFT_SENSOR_2_SRC_RESNAME) != NULL)) {
				this->is_DC_LeftSensorSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if ((stristr(surface->_name, DC_RIGHT_SENSOR_SRC_RESNAME) != NULL) ||
				(stristr(surface->_name, DC_RIGHT_SENSOR_2_SRC_RESNAME) != NULL)) {
				this->is_DC_RightSensorSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_SHIELDS_SRC_RESNAME) != NULL) {
				this->is_DC_ShieldsSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_SOLID_MSG_SRC_RESNAME) != NULL) {
				this->is_DC_SolidMsgSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_BORDER_MSG_SRC_RESNAME) != NULL) {
				this->is_DC_BorderMsgSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_LASER_BOX_SRC_RESNAME) != NULL) {
				this->is_DC_LaserBoxSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_ION_BOX_SRC_RESNAME) != NULL) {
				this->is_DC_IonBoxSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_BEAM_BOX_SRC_RESNAME) != NULL) {
				this->is_DC_BeamBoxSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_TOP_LEFT_SRC_RESNAME) != NULL) {
				this->is_DC_TopLeftSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
			if (stristr(surface->_name, DC_TOP_RIGHT_SRC_RESNAME) != NULL) {
				this->is_DC_TopRightSrc = true;
				this->is_DC_HUDRegionSrc = true;
			}
		}
	}

	// Load the relevant MAT file for the current OPT if necessary
	OPTNameType OPTname;
	OPTname.name[0] = 0;
	{
		// Capture the OPT name
		char *start = strstr(surface->_name, "\\");
		char *end = strstr(surface->_name, ".opt");
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
				//log_debug("[DBG] [MAT] Loading file %s...", sFileName);
				LoadIndividualMATParams(OPTname.name, sFileName); // OPT material
			}
		}
		else if (strstr(surface->_name, "dat,") != NULL) {
			// For DAT images, OPTname.name is the full DAT name:
			strncpy_s(OPTname.name, MAX_OPT_NAME, surface->_name, strlen(surface->_name));
			DATNameToMATParamsFile(OPTname.name, sFileName, sFileNameShort, 180);
			if (sFileName[0] != 0) {
				// Load the regular DAT material:
				if (!LoadIndividualMATParams(OPTname.name, sFileName)) { 
					// If the regular DAT material failed, try loading the mat file for DAT animations:
					if (sFileNameShort[0] != 0)
						// For some reason, each frame of the explosion animations is loaded multiple times.
						// There doesn't seem to be any bad effects for this behavior, it's only a bit spammy.
						// So, here we're just muting the debug messages to prevent the spam:
						LoadIndividualMATParams(OPTname.name, sFileNameShort, false);
				}
			}
		}
	}

	// Tag Lasers, Missiles, Cockpit textures, Exterior textures, light textures
	{
		//log_debug("[DBG] [DC] name: [%s]", surface->_name);
		// Catch the laser-related textures and mark them
		if (strstr(surface->_name, "Laser") != NULL) {
			// Ignore "LaserBat.OPT"
			if (strstr(surface->_name, "LaserBat") == NULL) {
				this->is_Laser = true;
			}
		}

		// Tag all the missiles
		// Flare
		if (strstr(surface->_name, "Missile") != NULL || strstr(surface->_name, "Torpedo") != NULL ||
			strstr(surface->_name, "SpaceBomb") != NULL || strstr(surface->_name, "Pulse") != NULL ||
			strstr(surface->_name, "Rocket") != NULL || strstr(surface->_name, "Flare") != NULL) {
			if (strstr(surface->_name, "Boat") == NULL) {
				//log_debug("[DBG] Missile texture: %s", surface->_name);
				this->is_Missile = true;
			}
		}

		if (strstr(surface->_name, "Turbo") != NULL) {
			this->is_TurboLaser = true;
		}

		if (strstr(surface->_name, "Cockpit") != NULL) {
			this->is_CockpitTex = true;

			// DEBUG
			// Looks like we always tag the color texture before the light texture.
			// Then we Load() the color and light textures.
			//if (strstr(surface->_name, "TEX00061") != NULL)
			//log_debug("[DBG] [AC] Tagging: %s", surface->_name);
			// DEBUG

			/* 
			   Here's a funny story: you can change the craft when in the hangar. So we need to pay attention
			   to changes in the cockpit's name. One way to do this is by resetting DC when textures are freed;
			   but doing that causes problems. When hook_time.dll is used, the textures are reloaded after they
			   are freed; but with Hook_60FPS.dll, textures are not reloaded. So, in the latter case, DC is
			   disabled when exiting the hangar, because for some reason *that's* when textures are released.
			   So, the fix is to *always* pay attention to cockpit name changes and reset DC when we detect a
			   change.
			*/
			// Capture and store the name of the cockpit the very first time we see a cockpit texture
			if (this->is_CockpitTex) {
				char CockpitName[128];
				//strstr(surface->_name, "Gunner")  != NULL)  {
				// Extract the name of the cockpit and put it in CockpitName
				char *start = strstr(surface->_name, "\\");
				char *end   = strstr(surface->_name, ".opt");
				if (start != NULL && end != NULL) {
					start += 1; // Skip the backslash
					int size = end - start;
					strncpy_s(CockpitName, 128, start, size);
					//log_debug("[DBG] Cockpit Name Captured: '%s'", CockpitName);
				}

				bool bCockpitNameChanged = !(strcmp(g_sCurrentCockpit, CockpitName) == 0);
				if (bCockpitNameChanged) 
				{
					// Force the recomputation of y_center for the next cockpit
					g_bYCenterHasBeenFixed = false;
					// The cockpit name has changed, update it and reset DC
					resources->ResetDynamicCockpit(); // Resetting DC will erase the cockpit name
					strncpy_s(g_sCurrentCockpit, 128, CockpitName, 128);
					log_debug("[DBG] Cockpit Name Changed/Captured: '%s'", g_sCurrentCockpit);
					// Load the relevant DC file for the current cockpit if necessary
					if (g_bDynCockpitEnabled) {
						char sFileName[80];
						CockpitNameToDCParamsFile(g_sCurrentCockpit, sFileName, 80);
						if (!LoadIndividualDCParams(sFileName))
							log_debug("[DBG] [DC] WARNING: Could not load DC params");
					}
					// Load the relevant AC file for the current cockpit if necessary
					if (g_bActiveCockpitEnabled) {
						char sFileName[80];
						CockpitNameToACParamsFile(g_sCurrentCockpit, sFileName, 80);
						if (!LoadIndividualACParams(sFileName))
							log_debug("[DBG] [AC] WARNING: Could not load AC params");
					}
					// Load the cockpit Shadow geometry
					if (g_ShadowMapping.bEnabled) {
						char sFileName[80];
						snprintf(sFileName, 80, "./ShadowMapping/%s.obj", g_sCurrentCockpit);
						log_debug("[DBG] [SHW] Loading file: %s", sFileName);
						// Disable shadow mapping, if we can load a Shadow Map OBJ, then it will be re-enabled again.
						g_ShadowMapping.bUseShadowOBJ = false;
						LoadShadowOBJ(sFileName);
					}
					// Load the reticle definition file
					{
						LoadCustomReticle(g_sCurrentCockpit);
					}
				}
					
			}
			
		}

		if (strstr(surface->_name, "Gunner") != NULL) {
			this->is_GunnerTex = true;
		}

		if (strstr(surface->_name, "Exterior") != NULL) {
			this->is_Exterior = true;
		}

		// Catch light textures and mark them appropriately
		if (strstr(surface->_name, ",light,") != NULL)
			this->is_LightTexture = true;

		// Link light and color textures:
		/*
		{
			if (!this->is_LightTexture) {
				this->lightTexture = nullptr;
				g_TextureVector.push_back(ColorLightPair(this));
			}
			else {
				// Find the corresponding color texture and link it
				// TODO: Do I need to worry about .DAT textures? Maybe not because they don't have "light"
				//       textures? I didn't see a single DAT file using the following line:
				//log_debug("[DBG] LIGHT: %s", this->_surface->_name);
				for (int i = g_TextureVector.size() - 1; i >= 0; i--) {
					char *light_name = this->_surface->_name;
					char *color_name = g_TextureVector[i].color->_surface->_name;
					char *light_start, *light_end, *color_start, *color_end;
					int len = 0;

					// Find the "TEX#####" token:
					light_start = strstr(light_name, ".opt,");
					if (light_start == NULL) break;
					light_start += 5; // Skip the ".opt," part
					light_end = strstr(light_start, ",");
					if (light_end == NULL) break;
					len = light_end - light_start;

					color_start = color_name + (light_start - light_name);
					color_end = color_name + (light_end - light_name);
					if (_strnicmp(light_start, color_start, len) == 0)
					{
						//log_debug("[DBG] %d, %s maps to %s", i, light_start, color_start);
						g_TextureVector[i].light = this;
						g_TextureVector[i].color->lightTexture = this;
						// After the color and light textures have been linked, g_TextureVector can be cleared
						break;
					}
				}
			}
		}
		*/

		//if (strstr(surface->_name, "color-transparent") != NULL) {
			//this->is_ColorTransparent = true;
			//log_debug("[DBG] [DC] ColorTransp: [%s]", surface->_name);
		//}

		/*
			Naming conventions from DTM:

			Surface_XXX_xxKm.opt = Planetary Surface
			Building_XXX.opt = Planetary Building
			Skydome_XXX_xxKm.opt = Planetary Sky
			Planet3D_XXX_xxKm.opt = Planet OPT object
			Star3D_XXX_ccKm.opt = Sun OPT object
			DeathStar3D_XXX.opt = Part of the real size Death Star (in progress)
		*/

		// Disable SSAO/SSDO for all Skydomes (This fix is specific for DTM's maps)
		if (strstr(surface->_name, "Cielo") != NULL ||
			strstr(surface->_name, "Skydome") != NULL)
		{
			g_b3DSkydomePresent = true;
			this->is_Skydome = true;
			this->is_GenericSSAOMasked = true;
			if (this->is_LightTexture) {
				this->is_SkydomeLight = true;
				this->is_LightTexture = false; // The is_SkydomeLight attribute overrides this attribute
			}
		}

		// 3D Sun is present in this scene: [opt, FlightModels\Planet3D_Sole_26Km.opt, TEX00001, color, 0]
		if (strstr(surface->_name, "Sole") != NULL ||
			strstr(surface->_name, "Star3D") != NULL) {
			g_b3DSunPresent = true;
			this->is_3DSun = true;
			//log_debug("[DBG] 3D Sun is present in this scene: [%s]", surface->_name);
		}
		
		if (g_bDynCockpitEnabled) {
			/* Process Dynamic Cockpit destination textures: */
			int idx = isInVector(surface->_name, g_DCElements, g_iNumDCElements);
			if (idx > -1) {
				// "light" and "color" textures are processed differently
				if (strstr(surface->_name, ",color") != NULL) {
					// This texture is a Dynamic Cockpit destination texture
					this->is_DynCockpitDst = true;
					// Make this texture "point back" to the right dc_element
					this->DCElementIndex = idx;
					// Activate this dc_element
					g_DCElements[idx].bActive = true;
					// Load the cover texture if necessary
					if (resources->dc_coverTexture[idx] == nullptr &&
						g_DCElements[idx].coverTextureName[0] != 0) {
						//ID3D11ShaderResourceView *coverTexture;
						wchar_t wTexName[MAX_TEXTURE_NAME];
						size_t len = 0;
						mbstowcs_s(&len, wTexName, MAX_TEXTURE_NAME, g_DCElements[idx].coverTextureName, MAX_TEXTURE_NAME);
						HRESULT res = DirectX::CreateWICTextureFromFile(resources->_d3dDevice, wTexName, NULL,
							&(resources->dc_coverTexture[idx]));
						if (FAILED(res)) {
							//log_debug("[DBG] [DC] ***** Could not load cover texture [%s]: 0x%x",
							//	g_DCElements[idx].coverTextureName, res);
							resources->dc_coverTexture[idx] = nullptr;
						}
						//else {
							//resources->dc_coverTexture[idx] = coverTexture;
							//log_debug("[DBG] [DC] ***** Loaded cover texture [%d][%s]", idx, g_DCElements[idx].coverTextureName);
						//}
					}
				}
				else if (strstr(surface->_name, ",light") != NULL) {
					this->is_DynCockpitAlphaOverlay = true;
				}
			} // if (idx > -1)
		} // if (g_bDynCockpitEnabled)

		if (g_bActiveCockpitEnabled) {
			if (this->is_CockpitTex && !this->is_LightTexture)
			{
				/* Process Active Cockpit destination textures: */
				int idx = isInVector(surface->_name, g_ACElements, g_iNumACElements);
				if (idx > -1) {
					// "Point back" into the right ac_element index:
					this->ActiveCockpitIdx = idx;
					g_ACElements[idx].bActive = true;
					//log_debug("[DBG] [AC] %s is an AC Texture, ActiveCockpitIdx: %d", surface->_name, this->ActiveCockpitIdx);
				}
			}
		}

		// Materials
		//if (g_bReshadeEnabled) 
		if (OPTname.name[0] != 0)
		{
			int craftIdx = FindCraftMaterial(OPTname.name);
			if (craftIdx > -1) {
				char texname[MAX_TEXNAME];
				bool bIsDat = strstr(OPTname.name, "dat,") != NULL;
				// We need to check if this is a DAT or an OPT first
				if (bIsDat) 
				{
					texname[0] = 0; // Retrieve the default material
				}
				else 
				{
					//log_debug("[DBG] [MAT] Craft Material %s found", OPTname.name);
					char *start = strstr(surface->_name, ".opt");
					// Skip the ".opt," part
					start += 5;
					// Find the next comma
					char *end = strstr(start, ",");
					int size = end - start;
					strncpy_s(texname, MAX_TEXNAME, start, size);
				}
				//log_debug("[DBG] [MAT] Looking for material for %s", texname);
				this->material = FindMaterial(craftIdx, texname);
				this->bHasMaterial = true;
				// This texture has a material associated with it, let's save it in the aux list:
				g_AuxTextureVector.push_back(this);
				this->AuxVectorIndex = g_AuxTextureVector.size() - 1;
				// DEBUG
				/*if (bIsDat) {
					log_debug("[DBG] [MAT] [%s] --> Material: %0.3f, %0.3f, %0.3f",
						surface->_name, material.Light.x, material.Light.y, material.Light.z);
				}*/
				// DEBUG
			}
			//else {
				// Material not found, use the default material (already created in the constructor)...
				//log_debug("[DBG] [MAT] Material %s not found", OPTname.name);
			//}
		}
	}
}

HRESULT Direct3DTexture::Load(
	LPDIRECT3DTEXTURE lpD3DTexture)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	str << " " << lpD3DTexture;
	LogText(str.str());
#endif

	if (lpD3DTexture == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	Direct3DTexture* d3dTexture = (Direct3DTexture*)lpD3DTexture;
	TextureSurface* surface = d3dTexture->_surface;
	//log_debug("[DBG] Loading %s", surface->name);
	// The changes from Jeremy's commit fe50cc59e03225bb7e39ae2852e87d305e7c7891 to reduce
	// memory usage cause mipmapped textures to call Load() again. So we must copy all the
	// settings from the input texture to this level.
	this->is_Tagged = d3dTexture->is_Tagged;
	this->TagCount = d3dTexture->TagCount;
	this->is_Reticle = d3dTexture->is_Reticle;
	this->is_ReticleCenter = d3dTexture->is_ReticleCenter;
	this->is_HighlightedReticle = d3dTexture->is_HighlightedReticle;
	this->is_TrianglePointer = d3dTexture->is_TrianglePointer;
	this->is_Text = d3dTexture->is_Text;
	this->is_Floating_GUI = d3dTexture->is_Floating_GUI;
	this->is_GUI = d3dTexture->is_GUI;
	this->is_TargetingComp = d3dTexture->is_TargetingComp;
	this->is_Laser = d3dTexture->is_Laser;
	this->is_TurboLaser = d3dTexture->is_TurboLaser;
	this->is_LightTexture = d3dTexture->is_LightTexture;
	this->is_EngineGlow = d3dTexture->is_EngineGlow;
	this->is_Electricity = d3dTexture->is_Electricity;
	this->is_Explosion = d3dTexture->is_Explosion;
	this->is_Smoke = d3dTexture->is_Smoke;
	this->is_CockpitTex = d3dTexture->is_CockpitTex;
	this->is_GunnerTex = d3dTexture->is_GunnerTex;
	this->is_Exterior = d3dTexture->is_Exterior;
	this->is_HyperspaceAnim = d3dTexture->is_HyperspaceAnim;
	this->is_FlatLightEffect = d3dTexture->is_FlatLightEffect;
	this->is_LensFlare = d3dTexture->is_LensFlare;
	this->is_Sun = d3dTexture->is_Sun;
	this->is_3DSun = d3dTexture->is_3DSun;
	//this->AssociatedXWALight = d3dTexture->AssociatedXWALight;
	this->is_Debris = d3dTexture->is_Debris;
	this->is_Trail = d3dTexture->is_Trail;
	this->is_Spark = d3dTexture->is_Spark;
	this->is_CockpitSpark = d3dTexture->is_CockpitSpark;
	this->is_Chaff = d3dTexture->is_Chaff;
	this->is_Missile = d3dTexture->is_Missile;
	this->is_GenericSSAOMasked = d3dTexture->is_GenericSSAOMasked;
	this->is_Skydome = d3dTexture->is_Skydome;
	this->is_SkydomeLight = d3dTexture->is_SkydomeLight;
	this->is_DAT = d3dTexture->is_DAT;
	this->is_BlastMark = d3dTexture->is_BlastMark;
	this->is_DS2_Reactor_Explosion = d3dTexture->is_DS2_Reactor_Explosion;
	//this->is_DS2_Energy_Field = d3dTexture->is_DS2_Energy_Field;
	this->ActiveCockpitIdx = d3dTexture->ActiveCockpitIdx;
	this->AuxVectorIndex = d3dTexture->AuxVectorIndex;
	// g_AuxTextureVector will keep a list of references to textures that have associated materials.
	// We use this list to reload materials by pressing Ctrl+Alt+L.
	// If d3dTexture has already been inserted in g_AuxTextureVector, then we need to update the 
	// reference in g_AuxTextureVector so that it points to *this* object now:
	if (this->AuxVectorIndex != -1 && this->AuxVectorIndex < (int)g_AuxTextureVector.size())
		g_AuxTextureVector[this->AuxVectorIndex] = this;
	// TODO: Instead of copying textures, let's have a single pointer shared by all instances
	// Actually, it looks like we need to copy the texture names in order to have them available
	// during 3D rendering. This makes them available both in the hangar and after launching from
	// the hangar.
	//log_debug("[DBG] [DC] Load Copying name: [%s]", d3dTexture->_surface->_name);
	strncpy_s(this->_surface->_name, d3dTexture->_surface->_name, MAX_TEXTURE_NAME);
	// Dynamic Cockpit data
	this->is_DynCockpitDst = d3dTexture->is_DynCockpitDst;
	this->is_DynCockpitAlphaOverlay = d3dTexture->is_DynCockpitAlphaOverlay;
	this->DCElementIndex = d3dTexture->DCElementIndex;
	this->is_DC_HUDRegionSrc = d3dTexture->is_DC_HUDRegionSrc;
	this->is_DC_TargetCompSrc = d3dTexture->is_DC_TargetCompSrc;
	this->is_DC_LeftSensorSrc = d3dTexture->is_DC_LeftSensorSrc;
	this->is_DC_RightSensorSrc = d3dTexture->is_DC_RightSensorSrc;
	this->is_DC_ShieldsSrc = d3dTexture->is_DC_ShieldsSrc;
	this->is_DC_SolidMsgSrc = d3dTexture->is_DC_SolidMsgSrc;
	this->is_DC_BorderMsgSrc = d3dTexture->is_DC_BorderMsgSrc;
	this->is_DC_LaserBoxSrc = d3dTexture->is_DC_LaserBoxSrc;
	this->is_DC_IonBoxSrc = d3dTexture->is_DC_IonBoxSrc;
	this->is_DC_BeamBoxSrc = d3dTexture->is_DC_BeamBoxSrc;
	this->is_DC_TopLeftSrc = d3dTexture->is_DC_TopLeftSrc;
	this->is_DC_TopRightSrc = d3dTexture->is_DC_TopRightSrc;

	this->material = d3dTexture->material;
	this->bHasMaterial = d3dTexture->bHasMaterial;
	//this->lightTexture = d3dTexture->lightTexture;

	// DEBUG
	// Looks like we always tag the color texture before the light texture.
	// Then we Load() the color and light textures.
	//if (this->is_CockpitTex) {
		//	log_debug("[DBG] [AC] Loading: %s", surface->_name);
	//}

	// DEBUG
	if (d3dTexture->_textureView)
	{
#if LOGGER
		str.str("\tretrieve existing texture");
		LogText(str.str());
#endif

		d3dTexture->_textureView->AddRef();
		*&this->_textureView = d3dTexture->_textureView.Get();

		return D3D_OK;
	}

#if LOGGER
	str.str("\tcreate new texture");
	LogText(str.str());
#endif

#if LOGGER
	str.str("");
	str << "\t" << surface->_pixelFormat.dwRGBBitCount;
	str << " " << (void*)surface->_pixelFormat.dwRBitMask;
	str << " " << (void*)surface->_pixelFormat.dwGBitMask;
	str << " " << (void*)surface->_pixelFormat.dwBBitMask;
	str << " " << (void*)surface->_pixelFormat.dwRGBAlphaBitMask;
	LogText(str.str());
#endif

	DWORD bpp = surface->_pixelFormat.dwRGBBitCount == 32 ? 4 : 2;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	if (bpp == 4)
	{
		format = BACKBUFFER_FORMAT;
	}
	else
	{
		DWORD alpha = surface->_pixelFormat.dwRGBAlphaBitMask;

		if (alpha == 0x0000F000)
		{
			format = DXGI_FORMAT_B4G4R4A4_UNORM;
		}
		else if (alpha == 0x00008000)
		{
			format = DXGI_FORMAT_B5G5R5A1_UNORM;
		}
		else
		{
			format = DXGI_FORMAT_B5G6R5_UNORM;
		}
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = surface->_width;
	textureDesc.Height = surface->_height;
	textureDesc.Format = this->_deviceResources->_are16BppTexturesSupported || format == BACKBUFFER_FORMAT ? format : BACKBUFFER_FORMAT;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.MipLevels = surface->_mipmapCount;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA* textureData = new D3D11_SUBRESOURCE_DATA[textureDesc.MipLevels];

	bool useBuffers = !this->_deviceResources->_are16BppTexturesSupported && format != BACKBUFFER_FORMAT;
	char** buffers = nullptr;

	if (useBuffers)
	{
		buffers = new char*[textureDesc.MipLevels];
		buffers[0] = convertFormat(surface->_buffer, surface->_width, surface->_height, format);
	}

	textureData[0].pSysMem = useBuffers ? buffers[0] : surface->_buffer;
	textureData[0].SysMemPitch = surface->_width * (useBuffers ? 4 : bpp);
	textureData[0].SysMemSlicePitch = 0;

	MipmapSurface* mipmap = surface->_mipmap;
	for (DWORD i = 1; i < textureDesc.MipLevels; i++)
	{
		if (useBuffers)
		{
			buffers[i] = convertFormat(mipmap->_buffer, mipmap->_width, mipmap->_height, format);
		}

		textureData[i].pSysMem = useBuffers ? buffers[i] : mipmap->_buffer;
		textureData[i].SysMemPitch = mipmap->_width * (useBuffers ? 4 : bpp);
		textureData[i].SysMemSlicePitch = 0;

		mipmap = mipmap->_mipmap;
	}

	// This is where the various textures are created from data already loaded into RAM
	ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = this->_deviceResources->_d3dDevice->CreateTexture2D(&textureDesc, textureData, &texture);
	if (FAILED(hr)) {
		log_debug("[DBG] Failed when calling CreateTexture2D, reason: 0x%x",
			this->_deviceResources->_d3dDevice->GetDeviceRemovedReason());
		goto out;
	}

	TagTexture();

out:
	if (useBuffers)
	{
		for (DWORD i = 0; i < textureDesc.MipLevels; i++)
		{
			delete[] buffers[i];
		}

		delete[] buffers;
	}

	delete[] textureData;

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			MessageBox(nullptr, _com_error(hr).ErrorMessage(), __FUNCTION__, MB_ICONERROR);
		}

		messageShown = true;

#if LOGGER
		str.str("\tD3DERR_TEXTURE_LOAD_FAILED");
		LogText(str.str());
#endif

		return D3DERR_TEXTURE_LOAD_FAILED;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc{};
	textureViewDesc.Format = textureDesc.Format;
	textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	textureViewDesc.Texture2D.MostDetailedMip = 0;

	if (FAILED(this->_deviceResources->_d3dDevice->CreateShaderResourceView(texture, &textureViewDesc, &d3dTexture->_textureView)))
	{
#if LOGGER
		str.str("\tD3DERR_TEXTURE_LOAD_FAILED");
		LogText(str.str());
#endif

		return D3DERR_TEXTURE_LOAD_FAILED;
	}

	d3dTexture->_textureView->AddRef();
	*&this->_textureView = d3dTexture->_textureView.Get();

	return D3D_OK;
}

HRESULT Direct3DTexture::Unload()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}
