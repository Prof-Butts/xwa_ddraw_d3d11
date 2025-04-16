#include "common.h"
#include "Effects.h"
#include "globals.h"
#include "XWAFramework.h"
#include "VRConfig.h"

// Main Pixel Shader constant buffer
MainShadersCBuffer      g_MSCBuffer;
// Constant Buffers
BloomPixelShaderCBuffer g_BloomPSCBuffer;
PSShadingSystemCB       g_ShadingSys_PSBuffer;
SSAOPixelShaderCBuffer  g_SSAO_PSCBuffer;
OPTMeshTransformCBuffer g_OPTMeshTransformCB;
RTConstantsBuffer       g_RTConstantsBuffer;
VRGeometryCBuffer       g_VRGeometryCBuffer;

ExternalCameraState     g_externalCameraState;

/*
 * Used to store a list of textures for fast lookup. For instance, all suns must
 * have their associated lights reset after jumping through hyperspace; and all
 * textures with materials can be placed here so that material properties can be
 * applied while flying.
 */
class XwaTextureData;
std::vector<XwaTextureData*> g_AuxTextureVector;

std::vector<char*> Text_ResNames = {
	"dat,16000,"
};

std::vector<char*> LaserIonEnergy_ResNames = {
	"dat,12000,2400,", // 0xd08b4437, (16x16) Laser charge.
	"dat,12000,2300,", // 0xd0168df9, (64x64) Laser charge boxes.
	"dat,12000,2500,", // 0xe321d785, (64x64) Laser and ion charge boxes on B-Wing.
	"dat,12000,2600,", // 0xca2a5c48, (8x8) Laser and ion charge on B-Wing.
};

std::vector<char*> Floating_GUI_ResNames = {
	"dat,12000,2400,", // 0xd08b4437, (16x16) Laser charge.
	"dat,12000,2300,", // 0xd0168df9, (64x64) Laser charge boxes.
	"dat,12000,2500,", // 0xe321d785, (64x64) Laser and ion charge boxes on B-Wing.
	"dat,12000,2600,", // 0xca2a5c48, (8x8) Laser and ion charge on B-Wing.
	"dat,12000,1100,", // 0x3b9a3741, (256x128) Full targetting computer, solid.
	"dat,12000,100,",  // 0x7e1b021d, (128x128) Left targetting computer, solid.
	"dat,12000,200,",  // 0x771a714,  (256x256) Left targetting computer, frame only.
};

// List of regular GUI elements (this is not an exhaustive list). It's mostly used to detect when
// the game has started rendering the GUI
std::vector<char*> GUI_ResNames = {
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
std::vector<char*> Electricity_ResNames = {
	// Animations (electricity)
	"dat,2007,",
	"dat,2008,", // <-- I think this is the hit effect of ions on disabled targets
	"dat,3005,", // <-- I think this is displayed when a target has been disabled
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

	"dat,3055,", // Electricity -- Animations.dat
};

// List of common roots for the Explosion names
std::vector<char*> Explosion_ResNames = {
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
std::vector<char*> Fire_ResNames = {
	"dat,3003,", // Sparks.dat <-- Smoke when hitting a target
	"dat,3004,", // Sparks.dat <-- Smoke when hitting a target
};

std::vector<char*> Sparks_ResNames = {
	"dat,3000,",
	"dat,3001,",
	"dat,3002,",
};

// These groups live in ANIMATIONS.dat
std::vector<char*> HitEffects_ResNames = {
	"dat,3100,", // <-- This is the hit effect for ions
	"dat,3200,", // <-- This is the hit effect for red lasers
	"dat,3300,", // <-- This is the hit effect for green lasers
	"dat,3400,",
	"dat,3500,",
};

// List of Lens Flare effects
std::vector<char*> LensFlare_ResNames = {
	"dat,1000,3,",
	"dat,1000,4,",
	"dat,1000,5,",
	"dat,1000,6,",
	"dat,1000,7,",
	"dat,1000,8,",
};

// List of Suns in the Backdrop.dat file
std::vector<char*> Sun_ResNames = {
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

std::vector<char*> SpaceDebris_ResNames = {
	"dat,4000,",
	"dat,4001,",
	"dat,4002,",
	"dat,4003,"
};

std::vector<char*> Trails_ResNames = {
	"dat,21000,",
	"dat,21005,",
	"dat,21010,",
	"dat,21015,",
	"dat,21020,",
	"dat,21025,",
};


// Global y_center and FOVscale parameters. These are updated only in ComputeHyperFOVParams.
float g_fYCenter = 0.0f, g_fFOVscale = 0.75f;
Vector2 g_ReticleCentroid(-1.0f, -1.0f);
bool g_bTriggerReticleCapture = false, g_bYCenterHasBeenFixed = false;

float g_fCurInGameWidth = 1, g_fCurInGameHeight = 1, g_fCurInGameAspectRatio = 1;
float g_fCurScreenWidth = 1, g_fCurScreenHeight = 1, g_fCurScreenWidthRcp = 1, g_fCurScreenHeightRcp = 1;
float g_fCurInGame2DWidth = -1, g_fCurInGame2DHeight = -1;
FOVtype g_CurrentFOVType = GLOBAL_FOV;
float g_fRealHorzFOV = 0.0f; // The real Horizontal FOV, in radians
float g_fRealVertFOV = 0.0f; // The real Vertical FOV, in radians
bool g_bMetricParamsNeedReapply = false;
Matrix4 g_ReflRotX;

// LASER LIGHTS AND DYNAMIC LIGHTS
Vector3 g_LaserPointDebug(0.0f, 0.0f, 0.0f);
Vector3 g_HeadLightsPosition(0.0f, 0.0f, 20.0f), g_HeadLightsColor(0.85f, 0.85f, 0.90f);
float g_fHeadLightsAmbient = 0.05f, g_fHeadLightsDistance = 5000.0f, g_fHeadLightsAngleCos = 0.25f; // Approx cos(75)
bool g_bHeadLightsAutoTurnOn = true;
bool g_bCurrentMissionUsesHeadLights = false;
const float DEFAULT_DYNAMIC_LIGHT_FALLOFF = 4.0f;
const Vector3 DEFAULT_EXPLOSION_COLOR = Vector3(1.0f, 0.75f, 0.375f);

bool g_bKeybExitHyperspace = true;
int g_iDraw2DCounter = 0;
bool g_bRendering3D = false; // Set to true when the system is about to render in 3D
bool g_bPrevPlayerInHangar = false;
bool g_bInTechRoom = false; // Set to true in PrimarySurface Present 2D (Flip)
bool g_bInBriefingRoom = false;

D3DTLVERTEX g_SpeedParticles2D[MAX_SPEED_PARTICLES * 12];

Vector3 g_CockpitPOVOffset = { 0, 0, 0 };
Vector3 g_GunnerTurretPOVOffset = { 0, 0, 0 };
Vector3 g_HologramDisp = { 0, 0, 0 };

// **************************
// DATReader global vars and function pointers
HMODULE g_hDATReader = nullptr;

uint32_t g_DATReaderVersion = DAT_READER_VERSION_101;

GetDATReaderVersionFun     GetDATReaderVersion     = nullptr;
LoadDATFileFun             LoadDATFile             = nullptr;
GetDATImageMetadataFun     GetDATImageMetadata     = nullptr;
ReadDATImageDataFun        ReadDATImageData        = nullptr;
ReadFlippedDATImageDataFun ReadFlippedDATImageData = nullptr;
SetDATVerbosityFun         SetDATVerbosity         = nullptr;
GetDATGroupImageCountFun   GetDATGroupImageCount   = nullptr;
GetDATGroupImageListFun    GetDATGroupImageList    = nullptr;
GetDATGroupCountFun        GetDATGroupCount        = nullptr;
GetDATGroupListFun         GetDATGroupList         = nullptr;
// **************************

// **************************
// ZIPReader global vars and function pointers
HMODULE g_hZIPReader = nullptr;
bool g_bCleanupZIPDirs = true;

SetZIPVerbosityFun             SetZIPVerbosity = nullptr;
LoadZIPFileFun                 LoadZIPFile = nullptr;
GetZIPGroupImageCountFun       GetZIPGroupImageCount = nullptr;
GetZIPGroupImageListFun        GetZIPGroupImageList = nullptr;
DeleteAllTempZIPDirectoriesFun DeleteAllTempZIPDirectories = nullptr;
GetZIPImageMetadataFun         GetZIPImageMetadata = nullptr;
// **************************

void SmallestK::insert(Vector3 P, Vector3 col, Vector3 dir, float falloff, float angle) {
	int i = _size - 1;
	while (i >= 0 && P.z < _elems[i].P.z) {
		float dx = fabs(_elems[i].P.x - P.x);
		float dy = fabs(_elems[i].P.y - P.y);
		float dz = fabs(_elems[i].P.z - P.z);
		// Avoid inserting duplicate elements in the list.
		if (dx < 0.0001f && dy < 0.0001f && dz < 0.0001f)
			return;
		// Copy the i-th element to the (i+1)-th index to make space at i
		if (i + 1 < MAX_CB_POINT_LIGHTS)
			_elems[i + 1] = _elems[i];
		i--;
	}

	// Insert at i + 1 (if possible) and we're done
	if (i + 1 < MAX_CB_POINT_LIGHTS) {
		_elems[i + 1].P = P;
		_elems[i + 1].col = col;
		_elems[i + 1].falloff = falloff;
		_elems[i + 1].angle = angle;
		_elems[i + 1].dir = dir;
		if (_size < MAX_CB_POINT_LIGHTS)
			_size++;
	}
}

void SmallestK::remove_duplicates() {
	bool Active[MAX_CB_POINT_LIGHTS] = { true };
	VectorColor tmp[MAX_CB_POINT_LIGHTS];
	int j = 0;

	for (int i = 0; i < _size - 1; i++) {
		float dx = fabs(_elems[i].P.x - _elems[i + 1].P.x);
		float dy = fabs(_elems[i].P.y - _elems[i + 1].P.y);
		float dz = fabs(_elems[i].P.z - _elems[i + 1].P.z);
		if (dx < 0.0001f && dy < 0.0001f && dz < 0.0001f) {
			if (g_bDumpSSAOBuffers)
				log_debug("[DBG] Laser light %d disabled: it's duplicated", i);
			Active[i] = false;
		}
		else if (fabs(_elems[i].P.x < 2.0f) &&
			fabs(_elems[i].P.y < 2.0f) &&
			fabs(_elems[i].P.z < 2.0f))
		{
			if (g_bDumpSSAOBuffers)
				log_debug("[DBG] Laser light: %d disabled: too close to the camera", i);
			Active[i] = false;
		}
		else {
			Active[i] = true;
			tmp[i] = _elems[i];
		}
	}

	for (int i = 0; i < _size; i++) {
		if (Active[i])
		{
			_elems[j++] = tmp[i];
		}
	}
	_size = j;
}

bool isInVector(uint32_t crc, std::vector<uint32_t>& vector) {
	for (uint32_t x : vector)
		if (x == crc)
			return true;
	return false;
}

bool isInVector(const char* name, std::vector<char*>& vector) {
	for (char* x : vector)
		if (stristr(name, x) != NULL)
			return true;
	return false;
}

bool isInVector(const char* OPTname, std::vector<OPTNameType>& vector) {
	for (const OPTNameType& x : vector)
		if (_stricmp(OPTname, x.name) == 0) // We need to avoid substrings because OPTs can be "Awing", "AwingExterior", "AwingCockpit"
			return true;
	return false;

}

// ***************************************************************
// HiResTimer
// ***************************************************************
void HiResTimer::ResetGlobalTime() {
	QueryPerformanceCounter(&(g_HiResTimer.start_time));
	g_HiResTimer.global_time_s = 0.0f;
	g_HiResTimer.last_time_s = 0.0f;
}

/*
// Returns the seconds since the last time this function was called
float HiResTimer::GetElapsedTimeSinceLastCall() {
	// Query the performance counters. This will let us render animations at a consistent speed.
	// The way this works is by computing the current time (curT) and then substracting the previous
	// time (lastT) from it. Then lastT gets curT. The elapsed time, in seconds is placed in
	// g_HiResTimer.elapsed_s.
	// lastT is not properly initialized on the very first frame; but nothing much seems
	// to happen. So: ignoring for now.
	QueryPerformanceCounter(&(g_HiResTimer.curT));
	g_HiResTimer.elapsed_us.QuadPart = g_HiResTimer.curT.QuadPart - g_HiResTimer.lastT.QuadPart;
	g_HiResTimer.elapsed_us.QuadPart *= 1000000;
	g_HiResTimer.elapsed_us.QuadPart /= g_HiResTimer.PC_Frequency.QuadPart;
	g_HiResTimer.elapsed_s = ((float)g_HiResTimer.elapsed_us.QuadPart / 1000000.0f);
	g_HiResTimer.lastT = g_HiResTimer.curT;
	//log_debug("[DBG] elapsed_us.Q: %llu, elapsed_s: %0.6f", g_HiResTimer.elapsed_us.QuadPart, g_HiResTimer.elapsed_s);
	//float FPS = 1.0f / g_HiResTimer.elapsed_s;
	//char buf[40];
	//sprintf_s(buf, 40, "%0.1f", FPS);
	//DisplayTimedMessage(1, 0, buf);
	return g_HiResTimer.elapsed_s;
}
*/

// Get the time since the last time ResetGlobalTime was called, also computes the time elapsed since
// this function was called.
float HiResTimer::GetElapsedTime() {
	// Query the performance counters. This will let us render animations at a consistent speed.
	// The way this works is by computing the current time (curT) and then substracting the start
	// time from it.
	QueryPerformanceCounter(&(g_HiResTimer.curT));
	g_HiResTimer.elapsed_us.QuadPart = g_HiResTimer.curT.QuadPart - g_HiResTimer.start_time.QuadPart;
	g_HiResTimer.elapsed_us.QuadPart *= 1000000;
	g_HiResTimer.elapsed_us.QuadPart /= g_HiResTimer.PC_Frequency.QuadPart;
	g_HiResTimer.global_time_s = ((float)g_HiResTimer.elapsed_us.QuadPart / 1000000.0f);
	g_HiResTimer.elapsed_s = g_HiResTimer.global_time_s - g_HiResTimer.last_time_s;
	g_HiResTimer.last_time_s = g_HiResTimer.global_time_s;
	return g_HiResTimer.global_time_s;
}

void DisplayCoords(LPD3DINSTRUCTION instruction, UINT curIndex) {
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	D3DTLVERTEX vert;
	uint32_t index;
	UINT idx = curIndex;

	log_debug("[DBG] START Geom");
	for (WORD i = 0; i < instruction->wCount; i++)
	{
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		vert = g_OrigVerts[index];
		log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f", vert.sx, vert.sy, vert.sz, vert.rhw);
		// , tu: %0.3f, tv: %0.3f, vert.tu, vert.tv

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f", vert.sx, vert.sy, vert.sz, vert.rhw);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f", vert.sx, vert.sy, vert.sz, vert.rhw);
		triangle++;
	}
	log_debug("[DBG] END Geom");
}

void InGameToScreenCoords(UINT left, UINT top, UINT width, UINT height, float x, float y, float* x_out, float* y_out)
{
	*x_out = left + x / g_fCurInGameWidth * width;
	*y_out = top + y / g_fCurInGameHeight * height;
}

void InGameToScreenCoords(float x, float y, float* x_out, float* y_out)
{
	InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
		(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x, y, x_out, y_out);
}

void ScreenCoordsToInGame(float left, float top, float width, float height, float x, float y, float* x_out, float* y_out)
{
	*x_out = g_fCurInGameWidth * (x - left) / width;
	*y_out = g_fCurInGameHeight * (y - top) / height;
}

void ScreenCoordsToInGame(float x, float y, float* x_out, float* y_out)
{
	ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
		g_nonVRViewport.Width, g_nonVRViewport.Height, x, y, x_out, y_out);
}

void CycleFOVSetting()
{
	// Don't change the FOV in VR mode: use your head to look around!
	if (g_bEnableVR) {
		g_CurrentFOVType = GLOBAL_FOV;
	}
	else {
		switch (g_CurrentFOVType) {
		case GLOBAL_FOV:
			g_CurrentFOVType = XWAHACKER_FOV;
			log_debug("[DBG] [FOV] Current FOV: xwahacker_fov");
			DisplayTimedMessage(4, 0, "Current Craft FOV");
			break;
		case XWAHACKER_FOV:
			g_CurrentFOVType = XWAHACKER_LARGE_FOV;
			log_debug("[DBG] [FOV] Current FOV: xwahacker_large_fov");
			DisplayTimedMessage(4, 0, "Current Craft Large FOV");
			break;
		case XWAHACKER_LARGE_FOV:
			g_CurrentFOVType = GLOBAL_FOV;
			log_debug("[DBG] [FOV] Current FOV: GLOBAL");
			DisplayTimedMessage(4, 0, "Global FOV");
			break;
		}

		// Apply the current FOV and recompute FOV-related parameters
		g_bYCenterHasBeenFixed = false;
		g_bCustomFOVApplied = false;
	}
}

//******************************************************************************************
// DAT Reader code
//******************************************************************************************
bool InitDATReader() {
	if (g_hDATReader != nullptr)
		return true;

	g_hDATReader = LoadLibrary("DATReader.dll");
	if (g_hDATReader == nullptr) {
		LoadDATFile = nullptr;
		GetDATImageMetadata = nullptr;
		ReadDATImageData = nullptr;
		return false;
	}

	GetDATReaderVersion     = (GetDATReaderVersionFun)GetProcAddress(g_hDATReader, "GetDATReaderVersion");
	LoadDATFile             = (LoadDATFileFun)GetProcAddress(g_hDATReader, "LoadDATFile");
	GetDATImageMetadata     = (GetDATImageMetadataFun)GetProcAddress(g_hDATReader, "GetDATImageMetadata");
	ReadDATImageData        = (ReadDATImageDataFun)GetProcAddress(g_hDATReader, "ReadDATImageData");
	ReadFlippedDATImageData = (ReadFlippedDATImageDataFun)GetProcAddress(g_hDATReader, "ReadFlippedDATImageData");
	SetDATVerbosity         = (SetDATVerbosityFun)GetProcAddress(g_hDATReader, "SetDATVerbosity");
	GetDATGroupImageCount   = (GetDATGroupImageCountFun)GetProcAddress(g_hDATReader, "GetDATGroupImageCount");
	GetDATGroupImageList    = (GetDATGroupImageListFun)GetProcAddress(g_hDATReader, "GetDATGroupImageList");
	GetDATGroupCount        = (GetDATGroupCountFun)GetProcAddress(g_hDATReader, "GetDATGroupCount");
	GetDATGroupList         = (GetDATGroupListFun)GetProcAddress(g_hDATReader, "GetDATGroupList");
	if (LoadDATFile == nullptr || GetDATImageMetadata == nullptr || ReadDATImageData == nullptr ||
		SetDATVerbosity == nullptr || GetDATGroupImageCount == nullptr || GetDATGroupImageList == nullptr ||
		GetDATGroupCount == nullptr || GetDATGroupList == nullptr)
	{
		log_debug("[DBG] Error in InitDATReader: Some functions could not be loaded from DATReader.dll");
		FreeLibrary(g_hDATReader);
		g_hDATReader = nullptr;
		return false;
	}

	if (GetDATReaderVersion != nullptr)
	{
		g_DATReaderVersion = GetDATReaderVersion();
	}
	log_debug("[DBG] DAT Reader Version: 0x%u (%d.%d.%d)",
		g_DATReaderVersion,
		g_DATReaderVersion >> 16,
		(g_DATReaderVersion >> 8) & 0xFF,
		g_DATReaderVersion & 0xFF);

	return true;
}

void CloseDATReader() {
	if (g_hDATReader == nullptr)
		return;

	FreeLibrary(g_hDATReader);
	g_hDATReader = nullptr;
}

std::vector<short> ReadDATImageListFromGroup(const char *sDATFileName, int GroupId) {
	std::vector<short> result;
	if (!InitDATReader()) { // Idempotent call, does nothing if DATReader is already loaded
		result.clear();
		return result;
	}

	if (!LoadDATFile(sDATFileName)) {
		log_debug("[DBG] Could not load DAT file: %s", sDATFileName);
		return result;
	}

	int NumImages = GetDATGroupImageCount(GroupId);
	if (NumImages > 0) {
		short *ImageIds = new short[NumImages];
		if (GetDATGroupImageList(GroupId, ImageIds, NumImages)) {
			for (int i = 0; i < NumImages; i++) {
				result.push_back(ImageIds[i]);
				//log_debug("[DBG] DAT ImageId: %d added", ImageIds[i]);
			}
		}
		delete[] ImageIds;
	}
	return result;
}
//******************************************************************************************

//******************************************************************************************
// ZIP Reader code
//******************************************************************************************
bool IsZIPReaderLoaded() {
	if (g_hZIPReader != nullptr)
		return true;
	return false;
}

bool InitZIPReader() {
	if (IsZIPReaderLoaded())
		return true;

	g_hZIPReader = LoadLibrary("ZIPReader.dll");
	if (g_hZIPReader == nullptr) {
		LoadZIPFile = nullptr;
		return false;
	}

	LoadZIPFile = (LoadZIPFileFun)GetProcAddress(g_hZIPReader, "LoadZIPFile");
	SetZIPVerbosity = (SetZIPVerbosityFun)GetProcAddress(g_hZIPReader, "SetZIPVerbosity");
	GetZIPGroupImageCount = (GetZIPGroupImageCountFun)GetProcAddress(g_hZIPReader, "GetZIPGroupImageCount");
	GetZIPGroupImageList = (GetZIPGroupImageListFun)GetProcAddress(g_hZIPReader, "GetZIPGroupImageList");
	DeleteAllTempZIPDirectories = (DeleteAllTempZIPDirectoriesFun)GetProcAddress(g_hZIPReader, "DeleteAllTempZIPDirectories");
	GetZIPImageMetadata = (GetZIPImageMetadataFun)GetProcAddress(g_hZIPReader, "GetZIPImageMetadata");
	if (LoadZIPFile == nullptr || SetZIPVerbosity == nullptr || GetZIPGroupImageCount == nullptr ||
		GetZIPGroupImageList == nullptr || DeleteAllTempZIPDirectories == nullptr ||
		GetZIPImageMetadata == nullptr)
	{
		log_debug("[DBG] Error in InitZIPReader: Some functions could not be loaded from ZIPReader.dll");
		FreeLibrary(g_hZIPReader);
		g_hZIPReader = nullptr;
		return false;
	}
	return true;
}

void CloseZIPReader() {
	if (g_hZIPReader == nullptr)
		return;

	FreeLibrary(g_hZIPReader);
	g_hZIPReader = nullptr;
}

std::vector<short> ReadZIPImageListFromGroup(const char *sZIPFileName, int GroupId) {
	std::vector<short> result;
	if (!InitZIPReader()) { // Idempotent call, does nothing if ZIPReader is already loaded
		result.clear();
		return result;
	}

	// Unzip the file
	if (!LoadZIPFile(sZIPFileName)) {
		log_debug("[DBG] Could not load ZIP file: %s", sZIPFileName);
		return result;
	}

	int NumImages = GetZIPGroupImageCount(GroupId);
	if (NumImages > 0) {
		short *ImageIds = new short[NumImages];
		if (GetZIPGroupImageList(GroupId, ImageIds, NumImages)) {
			for (int i = 0; i < NumImages; i++) {
				result.push_back(ImageIds[i]);
				//log_debug("[DBG] DAT ImageId: %d added", ImageIds[i]);
			}
		}
		delete[] ImageIds;
	}
	return result;
}
//******************************************************************************************

/*
 * Saves the current POV Offset to the current .ini file.
 * Only call this function if the shared memory pointer proxy (g_pSharedData)
 * has been initialized.
 */
bool SavePOVOffsetToIniFile()
{
	char sFileName[256], *sTempFileName = "./TempIniFile.txt";
	char sCraftName[128];
	FILE* in_file, *out_file;
	int error = 0, line = 0, len = 0;
	char buf[256];
	// In order to parse the .ini file, we need a finite state machine so that we can
	// tell when we see the [Section] we're interested in, and when we exit that same
	// [Section]
	enum FSM {
		INIT_ST,
		IN_TAG_ST,
	} fsm = INIT_ST;

	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const char* sectionName = bGunnerTurret ? "GunnerTurretPOVOffset" : "CockpitPOVOffset";
	Vector3 POVOffset = bGunnerTurret ? g_GunnerTurretPOVOffset : g_CockpitPOVOffset;

	if (g_pSharedDataCockpitLook == NULL || !g_SharedMemCockpitLook.IsDataReady()) {
		log_debug("[DBG] [POV] Shared Memory has not been initialized, cannot write POV Offset");
		return false;
	}

	if (strlen(g_sCurrentCockpit) <= 0) {
		log_debug("[DBG] [POV] Cockpit name hasn't been captured, will not write current POV Offset");
		return false;
	}

	// We need to remove the word "Cockpit" from g_sCurrentCockpit:
	strncpy_s(sCraftName, 128, g_sCurrentCockpit, 128);
	len = strlen(sCraftName);
	sCraftName[len - 7] = 0;

	snprintf(sFileName, 256, ".\\FlightModels\\%s.ini", sCraftName);
	log_debug("[DBG] [POV] Saving current POV Offset [%s] to INI file [%s]...", sectionName, sFileName);
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

	bool bPOVWritten = false;
	int prevLineLen = 0;
	bool bPrevLineBlank = false;
	while (fgets(buf, 256, in_file) != NULL) {
		line++;
		// Commented lines are automatically pass-through
		if (buf[0] == ';') {
			fprintf(out_file, buf);
			continue;
		}

		// Catch section names
		if (buf[0] == '[') {
			if (strstr(buf, sectionName) != NULL) {
				fsm = IN_TAG_ST;
			}
			else {
				if (fsm == IN_TAG_ST) {
					// A new [section] is beginning and we just skipped over the [sectionName] region.
					// Let's write the new POV offset here, at the same location it was previously within
					// the .ini file.
					fsm = INIT_ST;
					if (!bPrevLineBlank)
						fprintf(out_file, "\n");
					fprintf(out_file, "[%s]\n", sectionName);
					fprintf(out_file, "OffsetX = %0.3f\n", POVOffset.x);
					fprintf(out_file, "OffsetY = %0.3f\n", POVOffset.y);
					fprintf(out_file, "OffsetZ = %0.3f\n", POVOffset.z);
					fprintf(out_file, "\n");
					bPOVWritten = true;
				}
			}
		}

		// If we're not in-tag, then just pass-through:
		if (fsm != IN_TAG_ST) {
			prevLineLen = strlen(buf);
			bPrevLineBlank = (prevLineLen == 1) && buf[prevLineLen - 1] == '\n';
			fprintf(out_file, buf);
		}
	}

	// This DC file may not have the POV section, so let's add it:
	if (!bPOVWritten) {
		if (!bPrevLineBlank)
			fprintf(out_file, "\n");
		fprintf(out_file, "[%s]\n", sectionName);
		fprintf(out_file, "OffsetX = %0.3f\n", POVOffset.x);
		fprintf(out_file, "OffsetY = %0.3f\n", POVOffset.y);
		fprintf(out_file, "OffsetZ = %0.3f\n", POVOffset.z);
		bPOVWritten = true;
	}

	fclose(out_file);
	fclose(in_file);

	// Swap the files
	remove(sFileName);
	rename(sTempFileName, sFileName);
	return true;
}

/*
 * Loads the current POV Offset from the current .ini file.
 * Only call this function if the shared memory pointer (g_pSharedDataCockpitLook)
 * has been initialized.
 */
bool LoadPOVOffsetFromIniFile()
{
	char sFileName[256], sCraftName[128];
	char buf[256], param[128], svalue[128];
	FILE* in_file;
	int error = 0, line = 0, len = 0;
	float fValue;
	// In order to parse the .ini file, we need a finite state machine so that we can
	// tell when we see the [Section] we're interested in, and when we exit that same
	// [Section]
	enum FSM {
		OUT_OF_TAG_ST,
		IN_CP_TAG_ST,
		IN_GT_TAG_ST,
	} fsm = OUT_OF_TAG_ST;
	g_CockpitPOVOffset = { 0, 0, 0 };
	g_GunnerTurretPOVOffset = { 0, 0, 0 };

	log_debug("[DBG] [POV] LoadPOVOffset");
	if (g_pSharedDataCockpitLook == NULL || !g_SharedMemCockpitLook.IsDataReady()) {
		log_debug("[DBG] [POV] Shared Memory has not been initialized. Cannot read current POV Offset");
		return false;
	}

	if (strlen(g_sCurrentCockpit) <= 0) {
		log_debug("[DBG] [POV] Cockpit name hasn't been captured, cannot read current POV Offset");
		return false;
	}

	// We need to remove the word "Cockpit" from g_sCurrentCockpit:
	strncpy_s(sCraftName, 128, g_sCurrentCockpit, 128);
	len = strlen(sCraftName);
	sCraftName[len - 7] = 0;

	snprintf(sFileName, 256, ".\\FlightModels\\%s.ini", sCraftName);
	log_debug("[DBG] [POV] Loading current POV Offset from INI file [%s]...", sFileName);
	// Open sFileName for reading
	try {
		error = fopen_s(&in_file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [POV] Could not read [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] [POV] Error %d when reading [%s]", error, sFileName);
		return false;
	}

	while (fgets(buf, 256, in_file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		// Catch section names
		if (buf[0] == '[') {
			if (strstr(buf, "CockpitPOVOffset") != NULL)
				fsm = IN_CP_TAG_ST;
			else if (strstr(buf, "GunnerTurretPOVOffset") != NULL)
				fsm = IN_GT_TAG_ST;
			else
				fsm = OUT_OF_TAG_ST;
		}

		if (fsm == IN_CP_TAG_ST || fsm == IN_GT_TAG_ST) {
			Vector3& POVOffset = fsm == IN_CP_TAG_ST ? g_CockpitPOVOffset : g_GunnerTurretPOVOffset;
			if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
				fValue = (float)atof(svalue);
				// Read the relevant parameters
				if (_stricmp(param, "OffsetX") == 0) {
					log_debug("[DBG] [POV] Read [%d] OffsetX: %0.3f", fsm, fValue);
					POVOffset.x = fValue;
				}
				if (_stricmp(param, "OffsetY") == 0) {
					log_debug("[DBG] [POV] Read [%d] OffsetY: %0.3f", fsm, fValue);
					POVOffset.y = fValue;
				}
				if (_stricmp(param, "OffsetZ") == 0) {
					log_debug("[DBG] [POV] Read [%d] OffsetZ: %0.3f", fsm, fValue);
					POVOffset.z = fValue;
				}
			}
		}
	}

	log_debug("[DBG] [POV] numlines read: %d", line);
	fclose(in_file);
	return true;
}

/*
 * Saves the current Hologram offset to the current .ini file.
 */
bool SaveHoloOffsetToIniFile()
{
	char sFileName[256], *sTempFileName = "./TempIniFile.txt";
	char sCraftName[128];
	FILE* in_file, *out_file;
	int error = 0, line = 0, len = 0;
	char buf[256];
	// In order to parse the .ini file, we need a finite state machine so that we can
	// tell when we see the [Section] we're interested in, and when we exit that same
	// [Section]
	enum FSM {
		INIT_ST,
		IN_TAG_ST,
	} fsm = INIT_ST;

	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const char* sectionName = bGunnerTurret ? "GunnerTurretHoloOffsets" : "CockpitHoloOffsets";

	if (strlen(g_sCurrentCockpit) <= 0) {
		log_debug("[DBG] [HOLO] Cockpit name hasn't been captured, will not write current Holo Offset");
		return false;
	}

	// We need to remove the word "Cockpit" from g_sCurrentCockpit:
	strncpy_s(sCraftName, 128, g_sCurrentCockpit, 128);
	len = strlen(sCraftName);
	sCraftName[len - 7] = 0;

	snprintf(sFileName, 256, ".\\FlightModels\\%s.ini", sCraftName);
	log_debug("[DBG] [HOLO] Saving current Holo Offset [%s] to INI file [%s]...", sectionName, sFileName);
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

	bool bHoloWritten = false;
	int prevLineLen = 0;
	bool bPrevLineBlank = false;
	while (fgets(buf, 256, in_file) != NULL) {
		line++;
		// Commented lines are automatically pass-through
		if (buf[0] == ';') {
			fprintf(out_file, buf);
			continue;
		}

		// Catch section names
		if (buf[0] == '[') {
			if (strstr(buf, sectionName) != NULL) {
				fsm = IN_TAG_ST;
			}
			else {
				if (fsm == IN_TAG_ST) {
					// A new [section] is beginning and we just skipped over the [sectionName] region.
					// Let's write the new POV offset here, at the same location it was previously within
					// the .ini file.
					fsm = INIT_ST;
					if (!bPrevLineBlank)
						fprintf(out_file, "\n");
					fprintf(out_file, "[%s]\n", sectionName);
					fprintf(out_file, "OffsetX = %0.3f\n", g_HologramDisp.x);
					fprintf(out_file, "OffsetY = %0.3f\n", g_HologramDisp.y);
					fprintf(out_file, "OffsetZ = %0.3f\n", g_HologramDisp.z);
					fprintf(out_file, "\n");
					bHoloWritten = true;
				}
			}
		}

		// If we're not in-tag, then just pass-through:
		if (fsm != IN_TAG_ST) {
			prevLineLen = strlen(buf);
			bPrevLineBlank = (prevLineLen == 1) && buf[prevLineLen - 1] == '\n';
			fprintf(out_file, buf);
		}
	}

	// This DC file may not have the POV section, so let's add it:
	if (!bHoloWritten) {
		if (!bPrevLineBlank)
			fprintf(out_file, "\n");
		fprintf(out_file, "[%s]\n", sectionName);
		fprintf(out_file, "OffsetX = %0.3f\n", g_HologramDisp.x);
		fprintf(out_file, "OffsetY = %0.3f\n", g_HologramDisp.y);
		fprintf(out_file, "OffsetZ = %0.3f\n", g_HologramDisp.z);
		bHoloWritten = true;
	}

	fclose(out_file);
	fclose(in_file);

	// Swap the files
	remove(sFileName);
	rename(sTempFileName, sFileName);
	return true;
}

/*
 * Loads the current Hologram Offset from the current .ini file.
 */
bool LoadHoloOffsetFromIniFile()
{
	char sFileName[256], sCraftName[128];
	char buf[256], param[128], svalue[128];
	FILE* in_file;
	int error = 0, line = 0, len = 0;
	float fValue;
	// In order to parse the .ini file, we need a finite state machine so that we can
	// tell when we see the [Section] we're interested in, and when we exit that same
	// [Section]
	enum FSM {
		OUT_OF_TAG_ST,
		IN_CP_TAG_ST,
		IN_GT_TAG_ST,
	} fsm = OUT_OF_TAG_ST;

	log_debug("[DBG] [HOLO] LoadHoloOffset");
	g_HologramDisp = { 0, 0, 0 };

	if (strlen(g_sCurrentCockpit) <= 0) {
		log_debug("[DBG] [HOLO] Cockpit name hasn't been captured, cannot read current Holo Offset");
		return false;
	}

	// We need to remove the word "Cockpit" from g_sCurrentCockpit:
	strncpy_s(sCraftName, 128, g_sCurrentCockpit, 128);
	len = strlen(sCraftName);
	sCraftName[len - 7] = 0;

	snprintf(sFileName, 256, ".\\FlightModels\\%s.ini", sCraftName);
	log_debug("[DBG] [HOLO] Loading current Holo Offset from INI file [%s]...", sFileName);
	// Open sFileName for reading
	try {
		error = fopen_s(&in_file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [HOLO] Could not read [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] [HOLO] Error %d when reading [%s]", error, sFileName);
		return false;
	}

	while (fgets(buf, 256, in_file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		// Catch section names
		if (buf[0] == '[') {
			if (strstr(buf, "CockpitHoloOffsets") != NULL)
				fsm = IN_CP_TAG_ST;
			else if (strstr(buf, "GunnerTurretHoloOffsets") != NULL)
				fsm = IN_GT_TAG_ST;
			else
				fsm = OUT_OF_TAG_ST;
		}

		if (fsm == IN_CP_TAG_ST || fsm == IN_GT_TAG_ST) {
			Vector3& POVOffset = fsm == IN_CP_TAG_ST ? g_CockpitPOVOffset : g_GunnerTurretPOVOffset;
			if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
				fValue = (float)atof(svalue);
				// Read the relevant parameters
				if (_stricmp(param, "OffsetX") == 0) {
					log_debug("[DBG] [HOLO] Read [%d] OffsetX: %0.3f", fsm, fValue);
					g_HologramDisp.x = fValue;
				}
				if (_stricmp(param, "OffsetY") == 0) {
					log_debug("[DBG] [HOLO] Read [%d] OffsetY: %0.3f", fsm, fValue);
					g_HologramDisp.y = fValue;
				}
				if (_stricmp(param, "OffsetZ") == 0) {
					log_debug("[DBG] [HOLO] Read [%d] OffsetZ: %0.3f", fsm, fValue);
					g_HologramDisp.z = fValue;
				}
			}
		}
	}

	log_debug("[DBG] [HOLO] numlines read: %d", line);
	fclose(in_file);
	return true;
}

void ApplyCustomHUDColor()
{
	if (g_iHUDInnerColor != 0)
		*g_XwaFlightHudColor = g_iHUDInnerColor;
	if (g_iHUDBorderColor != 0)
		*g_XwaFlightHudBorderColor = g_iHUDBorderColor;
}

/*
 * Loads the HUD color from the current INI file. This function will only work if
 * g_sCurrentCockpit has been filled with the current cockpit name.
 */
bool LoadHUDColorFromIniFile()
{
	char sFileName[256], sCraftName[128];
	char buf[256], param[128], svalue[128];
	FILE* in_file;
	int error = 0, line = 0, len = 0;
	float fValue, x, y, z;
	uint32_t color = 0x0;
	g_iHUDInnerColor = 0;
	g_iHUDBorderColor = 0;
	//g_iHUDInnerColor = g_iOriginalHUDInnerColor;
	//g_iHUDBorderColor = g_iOriginaHUDBorderColor;

	// In order to parse the .ini file, we need a finite state machine so that we can
	// tell when we see the [Section] we're interested in, and when we exit that same
	// [Section]
	enum FSM {
		OUT_OF_TAG_ST,
		IN_TAG_ST,
	} fsm = OUT_OF_TAG_ST;

	if (strlen(g_sCurrentCockpit) <= 0) {
		log_debug("[DBG] [HUD] Cockpit name hasn't been captured, cannot read current HUD color");
		return false;
	}

	// We need to remove the word "Cockpit" from g_sCurrentCockpit:
	strncpy_s(sCraftName, 128, g_sCurrentCockpit, 128);
	len = strlen(sCraftName);
	sCraftName[len - 7] = 0;

	snprintf(sFileName, 256, ".\\FlightModels\\%s.ini", sCraftName);
	log_debug("[DBG] [HUD] Loading current HUD color from INI file [%s]...", sFileName);
	// Open sFileName for reading
	try {
		error = fopen_s(&in_file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [HUD] Could not read [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] [HUD] Error %d when reading [%s]", error, sFileName);
		return false;
	}

	while (fgets(buf, 256, in_file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		// Catch section names
		if (buf[0] == '[') {
			fsm = (strstr(buf, "[HUDColor]") != NULL) ? IN_TAG_ST : OUT_OF_TAG_ST;
		}

		// If we're not in-tag, then just pass-through:
		if (fsm == IN_TAG_ST) {
			if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
				fValue = (float)atof(svalue);
				// Read the relevant parameters
				if (_stricmp(param, "HUDBorderColor") == 0) {
					if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
						color = 0xff << 24 | uint32_t(x * 255) << 16 | uint32_t(y * 255) << 8 | uint32_t(z * 255);
						g_iHUDBorderColor = color;
						log_debug("[DBG] [HUD] HUD border color: 0x%x", color);
					}
				} else if (_stricmp(param, "HUDInnerColor") == 0) {
					if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
						color = 0xff << 24 | uint32_t(x * 255) << 16 | uint32_t(y * 255) << 8 | uint32_t(z * 255);
						g_iHUDInnerColor = color;
						log_debug("[DBG] [HUD] HUD inner color: 0x%x", color);
					}
				}
			}
		}
	}
	fclose(in_file);
	ApplyCustomHUDColor();
	return true;
}

CraftInstance *GetPlayerCraftInstanceSafe()
{
	// I've seen the game crash when trying to access the CraftInstance table in the
	// first few frames of a new mission. Let's add this test to prevent this crash.
	if (g_iPresentCounter <= PLAYERDATATABLE_MIN_SAFE_FRAME) return NULL;

	// Fetch the pointer to the current CraftInstance
	int16_t objectIndex = (int16_t)PlayerDataTable[*g_playerIndex].objectIndex;
	if (objectIndex < 0) return NULL;
	if (*objects == NULL) return NULL;
	ObjectEntry *object = &((*objects)[objectIndex]);
	if (object == NULL) return NULL;
	MobileObjectEntry *mobileObject = object->MobileObjectPtr;
	if (mobileObject == NULL) return NULL;
	CraftInstance *craftInstance = mobileObject->craftInstancePtr;
	if (craftInstance == NULL) return NULL;
	return craftInstance;
}

CraftInstance* GetPlayerCraftInstanceSafe(ObjectEntry **object)
{
	// I've seen the game crash when trying to access the CraftInstance table in the
	// first few frames of a new mission. Let's add this test to prevent this crash.
	if (g_iPresentCounter <= PLAYERDATATABLE_MIN_SAFE_FRAME) return NULL;

	// Fetch the pointer to the current CraftInstance
	int16_t objectIndex = (int16_t)PlayerDataTable[*g_playerIndex].objectIndex;
	if (objectIndex < 0) return NULL;
	if (*objects == NULL) return NULL;
	*object = &((*objects)[objectIndex]);
	if (object == NULL) return NULL;
	MobileObjectEntry* mobileObject = (*object)->MobileObjectPtr;
	if (mobileObject == NULL) return NULL;
	CraftInstance* craftInstance = mobileObject->craftInstancePtr;
	if (craftInstance == NULL) return NULL;
	return craftInstance;
}

CraftInstance* GetPlayerCraftInstanceSafe(ObjectEntry** object, MobileObjectEntry **mobileObject)
{
	// I've seen the game crash when trying to access the CraftInstance table in the
	// first few frames of a new mission. Let's add this test to prevent this crash.
	if (g_iPresentCounter <= PLAYERDATATABLE_MIN_SAFE_FRAME) return NULL;

	// Fetch the pointer to the current CraftInstance
	int16_t objectIndex = (int16_t)PlayerDataTable[*g_playerIndex].objectIndex;
	if (objectIndex < 0) return NULL;
	if (*objects == NULL) return NULL;
	*object = &((*objects)[objectIndex]);
	if (object == NULL) return NULL;
	*mobileObject = (*object)->MobileObjectPtr;
	if (*mobileObject == NULL) return NULL;
	CraftInstance* craftInstance = (*mobileObject)->craftInstancePtr;
	if (craftInstance == NULL) return NULL;
	return craftInstance;
}

CraftInstance* GetCraftInstanceSafe(int16_t objectIndex, ObjectEntry** object, MobileObjectEntry **mobileObject)
{
	// I've seen the game crash when trying to access the CraftInstance table in the
	// first few frames of a new mission. Let's add this test to prevent this crash.
	if (g_iPresentCounter <= PLAYERDATATABLE_MIN_SAFE_FRAME || objectIndex < 0) return NULL;

	// Fetch the pointer to the current CraftInstance
	if (objectIndex < 0) return NULL;
	if (*objects == NULL) return NULL;
	*object = &((*objects)[objectIndex]);
	if (object == NULL) return NULL;
	*mobileObject = (*object)->MobileObjectPtr;
	if (*mobileObject == NULL) return NULL;
	CraftInstance* craftInstance = (*mobileObject)->craftInstancePtr;
	if (craftInstance == NULL) return NULL;
	return craftInstance;
}

CraftInstance* GetCraftInstanceForCurrentTargetSafe()
{
	if (g_iPresentCounter <= PLAYERDATATABLE_MIN_SAFE_FRAME) return nullptr;

	short currentTargetIndex = *g_playerInHangar ?
		PlayerDataTable[*g_playerIndex].objectIndex : PlayerDataTable[*g_playerIndex].currentTargetIndex;
	if (currentTargetIndex < 0)
		return nullptr;

	ObjectEntry *object = &((*objects)[currentTargetIndex]);
	if (object == NULL) return nullptr;
	MobileObjectEntry *mobileObject = object->MobileObjectPtr;
	if (mobileObject == NULL) return nullptr;
	CraftInstance *craftInstance = mobileObject->craftInstancePtr;
	return craftInstance;
}

// This code is courtesy of Jeremy.
bool InTechGlobe()
{
	const int currentGameState = *(int*)(0x09F60E0 + 0x25FA9);
	const int updateCallback = *(int*)(0x09F60E0 + 0x25FB1 + currentGameState * 0x850 + 0x0844);
	const int XwaTechLibraryGameStateUpdate = 0x00574D70;
	const int XwaMissionBriefingGameStateUpdate = 0x00564E90;
	//g_bInTechRoom = (updateCallback == XwaTechLibraryGameStateUpdate);
	// For functional purposes, "Tech Room" has been used everywhere where 2D rendering is done,
	// and that includes the briefing room too.
	g_bInTechRoom = (updateCallback == XwaTechLibraryGameStateUpdate) ||
		(updateCallback == XwaMissionBriefingGameStateUpdate);
	return g_bInTechRoom;
}

// Also courtesy of Jeremy -- of course.
bool InBriefingRoom()
{
	const int currentGameState = *(int*)(0x009F60E0 + 0x25FA9);
	const int updateCallback = *(int*)(0x009F60E0 + 0x25FB1 + currentGameState * 0x850 + 0x0844);
	const int XwaMissionBriefingGameStateUpdate = 0x00564E90;
	g_bInBriefingRoom = (updateCallback == XwaMissionBriefingGameStateUpdate);
	return g_bInBriefingRoom;
}
