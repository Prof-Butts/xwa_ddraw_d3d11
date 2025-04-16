#pragma once

#include "EffectsCommon.h"
#include "DynamicCockpit.h"
#include "SteamVR.h"
#include <wincodec.h>


// ACTIVE COCKPIT
#define MAX_AC_COORDS_PER_TEXTURE 80
#define MAX_AC_TEXTURES_PER_COCKPIT 32
#define MAX_AC_ACTION_LEN 16 // WORDs (scan codes) used to specify an action
#define AC_HOLOGRAM_FAKE_VK_CODE 0x01 // Internal AC code to toggle the holograms

#define AC_JOYBUTTON1_FAKE_VK_CODE 0x02 // Internal AC code to emulate joystick button clicks
#define AC_JOYBUTTON2_FAKE_VK_CODE 0x03
#define AC_JOYBUTTON3_FAKE_VK_CODE 0x04
#define AC_JOYBUTTON4_FAKE_VK_CODE 0x05
#define AC_JOYBUTTON5_FAKE_VK_CODE 0x06

#define AC_VRKEYB_TOGGLE_FAKE_VK_CODE 0x07
#define AC_VRKEYB_HOVER_FAKE_VK_CODE  0x08
#define AC_VRKEYB_PLACE_FAKE_VK_CODE  0x09
#define AC_VRKEYB_OFF_FAKE_VK_CODE    0x0A

#define AC_HOLD_CTRL_FAKE_VK_CODE  0x10
#define AC_HOLD_ALT_FAKE_VK_CODE   0x11
#define AC_HOLD_SHIFT_FAKE_VK_CODE 0x12

#define AC_POV_UP_FAKE_VK_CODE 0x13
#define AC_POV_DN_FAKE_VK_CODE 0x14
#define AC_POV_FD_FAKE_VK_CODE 0x15
#define AC_POV_BK_FAKE_VK_CODE 0x16
#define AC_POV_RESET_FAKE_VK_CODE 0x17

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

// ACTIVE COCKPIT
extern bool g_bActiveCockpitEnabled;
extern bool g_bEnableVRPointerInConcourse;
extern Vector4 g_contOriginWorldSpace[2]; // This is the origin of the controller in 3D, in world-space coords
extern Vector4 g_controllerForwardVector; // Forward direction in the controller's frame of reference
extern Vector4 g_controllerUpVector;
extern Vector4 g_contDirWorldSpace[2]; // This is the direction in which the controller is pointing in world-space coords
extern Vector3 g_LaserPointer3DIntersection[2];
extern Vector3 g_LaserPointerIntersSteamVR[2];
extern float g_fBestIntersectionDistance[2], g_fLaserIntersectionDistance[2];
extern float g_fPushButtonThreshold, g_fReleaseButtonThreshold;
extern int g_iBestIntersTexIdx[2]; // The index into g_ACElements where the intersection occurred
extern bool g_bPrevHoveringOnActiveElem[2];
extern bool g_bFreePIEControllerButtonDataAvailable;
extern ac_element g_ACElements[MAX_AC_TEXTURES_PER_COCKPIT];
extern int g_iNumACElements, g_iVRKeyboardSlot;
extern int g_iVRGloveSlot[2];
extern bool g_bACActionTriggered[2], g_bACLastTriggerState[2], g_bACTriggerState[2];
extern bool g_bFreePIEInitialized, g_bFreePIEControllerButtonDataAvailable;
extern int g_iFreePIESlot, g_iFreePIEControllerSlot;

struct ACJoyEmulSettings
{
	bool  throttleEnabled;
	bool  yprEnabled;
	// Internally, the left-hand index is always 0 and the right hand is at index 1
	// We use these indices to enable handedness:
	int   joyHandIdx;
	int   thrHandIdx;
	float joyHalfRangeX;
	float joyHalfRangeZ;
	float deadZonePerc;
	float rollDeadZonePerc;
	float thrHalfRange;

	float yawHalfRange;
	float pitchHalfRange;
	float rollHalfRange;

	ACJoyEmulSettings()
	{
		throttleEnabled = false;
		yprEnabled      = true;

		joyHandIdx = 1;
		thrHandIdx = 0;

		joyHalfRangeX = 0.075f;
		joyHalfRangeZ = 0.075f;
		thrHalfRange  = 0.07f;

		deadZonePerc     = 0.15f;
		rollDeadZonePerc = 0.17f;

		yawHalfRange   = 25.0f;
		pitchHalfRange = 25.0f;
		rollHalfRange  = 15.0f;
	}
};
extern ACJoyEmulSettings g_ACJoyEmul;

struct ACJoyMapping
{
	WORD action[VRButtons::MAX][MAX_AC_ACTION_LEN];
};
extern ACJoyMapping g_ACJoyMappings[2];

// Used to tell which controller and button are used in VR for activating AC controls
struct ACPointerData
{
	float mouseSpeedX;
	float mouseSpeedY;

	ACPointerData()
	{
		mouseSpeedX = 3.0f;
		mouseSpeedY = 3.0f;
	}
};
extern ACPointerData g_ACPointerData;

// DEBUG vars
extern Vector3 g_debug_v0, g_debug_v1, g_debug_v2;
extern bool g_bDumpLaserPointerDebugInfo;
extern Vector3 g_LPdebugPoint;
extern float g_fLPdebugPointOffset;
// DEBUG vars

bool LoadIndividualACParams(char* sFileName);
bool LoadACAction(int lineNum, char* buf, float width, float height, ac_uv_coords* coords, bool flip=true);
void CockpitNameToACParamsFile(char* CockpitName, char* sFileName, int iFileNameSize);
bool TranslateACAction(int lineNum, WORD* scanCodes, char* action, bool* bIsVRKeybActivator);
void DisplayACAction(WORD* scanCodes);
int isInVector(const char* name, ac_element* ac_elements, int num_elems);
void ACRunAction(WORD* action, const uvfloat4& coords, int ACSlot, int contIdx, struct joyinfoex_tag* pji = nullptr);
bool IsContinousAction(WORD* action);
