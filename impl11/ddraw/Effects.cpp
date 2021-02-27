#include "Effects.h"
#include "globals.h"
#include "common.h"
#include "XWAFramework.h"
#include "VRConfig.h"

// Main Pixel Shader constant buffer
MainShadersCBuffer			g_MSCBuffer;
// Constant Buffers
BloomPixelShaderCBuffer		g_BloomPSCBuffer;
PSShadingSystemCB			g_ShadingSys_PSBuffer;
SSAOPixelShaderCBuffer		g_SSAO_PSCBuffer;

std::vector<ColorLightPair> g_TextureVector;
/*
 * Used to store a list of textures for fast lookup. For instance, all suns must
 * have their associated lights reset after jumping through hyperspace; and all
 * textures with materials can be placed here so that material properties can be
 * applied while flying.
 */
std::vector<Direct3DTexture*> g_AuxTextureVector;

std::vector<char*> Text_ResNames = {
	"dat,16000,"
};

std::vector<char*> LaserIonEnergy_ResNames = {
	"dat,12000,2400,", // 0xd08b4437, (16x16) Laser charge. (master branch)
	"dat,12000,2300,", // 0xd0168df9, (64x64) Laser charge boxes.
	"dat,12000,2500,", // 0xe321d785, (64x64) Laser and ion charge boxes on B - Wing. (master branch)
	"dat,12000,2600,", // 0xca2a5c48, (8x8) Laser and ion charge on B - Wing. (master branch)
};

std::vector<char*> Floating_GUI_ResNames = {
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
std::vector<char*> Smoke_ResNames = {
	"dat,3003,", // Sparks.dat <-- Smoke when hitting a target
	"dat,3004,", // Sparks.dat <-- Smoke when hitting a target
	// The following used to be tagged as explosions, but they look like smoke
	// Animations.dat
	"dat,3006,", // Single-frame smoke?
};

std::vector<char*> Sparks_ResNames = {
	"dat,3000,",
	"dat,3001,",
	"dat,3002,",
	
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

float g_fCurInGameWidth = 1, g_fCurInGameHeight = 1, g_fCurInGameAspectRatio = 1, g_fCurScreenWidth = 1, g_fCurScreenHeight = 1, g_fCurScreenWidthRcp = 1, g_fCurScreenHeightRcp = 1;
FOVtype g_CurrentFOVType = GLOBAL_FOV;
float g_fRealHorzFOV = 0.0f; // The real Horizontal FOV, in radians
float g_fRealVertFOV = 0.0f; // The real Vertical FOV, in radians
bool g_bMetricParamsNeedReapply = false;
Matrix4 g_ReflRotX;

Vector3 g_LaserPointDebug(0.0f, 0.0f, 0.0f);
Vector3 g_HeadLightsPosition(0.0f, 0.0f, 20.0f), g_HeadLightsColor(0.85f, 0.85f, 0.90f);
float g_fHeadLightsAmbient = 0.05f, g_fHeadLightsDistance = 5000.0f, g_fHeadLightsAngleCos = 0.25f; // Approx cos(75)
bool g_bHeadLightsAutoTurnOn = true;

bool g_bKeybExitHyperspace = true;
int g_iDraw2DCounter = 0;
bool g_bRendering3D = false; // Set to true when the system is about to render in 3D
bool g_bPrevPlayerInHangar = false;
bool g_bInTechRoom = false; // Set to true in PrimarySurface Present 2D (Flip)

D3DTLVERTEX g_SpeedParticles2D[MAX_SPEED_PARTICLES * 12];

//#ifdef DBR_VR
bool g_bCapture2DOffscreenBuffer = false;
//#endif

void SmallestK::insert(Vector3 P, Vector3 col) {
	int i = _size - 1;
	while (i >= 0 && P.z < _elems[i].P.z) {
		// Copy the i-th element to the (i+1)-th index to make space at i
		if (i + 1 < MAX_CB_POINT_LIGHTS)
			_elems[i + 1] = _elems[i];
		i--;
	}

	// Insert at i + 1 (if possible) and we're done
	if (i + 1 < MAX_CB_POINT_LIGHTS) {
		_elems[i + 1].P = P;
		_elems[i + 1].col = col;
		if (_size < MAX_CB_POINT_LIGHTS)
			_size++;
	}
}

bool isInVector(uint32_t crc, std::vector<uint32_t>& vector) {
	for (uint32_t x : vector)
		if (x == crc)
			return true;
	return false;
}

bool isInVector(char* name, std::vector<char*>& vector) {
	for (char* x : vector)
		if (stristr(name, x) != NULL)
			return true;
	return false;
}

bool isInVector(char* OPTname, std::vector<OPTNameType>& vector) {
	for (OPTNameType x : vector)
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

void ScreenCoordsToInGame(float left, float top, float width, float height, float x, float y, float* x_out, float* y_out)
{
	*x_out = g_fCurInGameWidth * (x - left) / width;
	*y_out = g_fCurInGameHeight * (y - top) / height;
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