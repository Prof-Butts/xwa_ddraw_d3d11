#pragma once

#include <stdio.h>
#include <vector>
#include "Vectors.h"
#include "Matrices.h"
#include "config.h"
#include "EffectsCommon.h"
#include <openvr.h>

extern float g_fSteamVROverlayWidthInMeters;
extern float g_fSteamVROverlayDepthInMeters;

extern vr::IVRSystem* g_pHMD;
extern vr::IVRChaperone* g_pChaperone;
extern vr::IVRCompositor* g_pVRCompositor;
extern vr::IVROverlay* g_pVROverlay;
extern vr::VROverlayHandle_t g_VR2Doverlay; // Handle to the specific overlay displaying the 2D rendered scenes.
extern vr::IVRScreenshots* g_pVRScreenshots;
extern vr::TrackedDevicePose_t g_rTrackedDevicePose;
extern uint32_t g_steamVRWidth, g_steamVRHeight; // The resolution recommended by SteamVR is stored here
extern bool g_bSteamVREnabled; // The user sets this flag to true to request support for SteamVR.
extern bool g_bSteamVRInitialized; // The system will set this flag after SteamVR has been initialized
extern bool g_bUseSteamVR; // The system will set this flag if the user requested SteamVR and SteamVR was initialized properly
extern bool g_bUseTextureCube;
extern bool g_bRenderDefaultStarfield;
extern bool g_bDebugDefaultStarfield;
extern bool g_bEnableCubeMaps;
extern bool g_bReplaceBackdrops;
extern bool g_bDefaultStarfieldRendered;
extern bool g_bUseExternalCameraState;
extern bool g_bSteamVRMirrorWindowLeftEye;
extern const bool DEFAULT_INTERLEAVED_REPROJECTION;
extern const bool DEFAULT_STEAMVR_POS_FROM_FREEPIE;
extern bool g_bInterleavedReprojection;
extern bool g_bSteamVRDistortionEnabled;
extern vr::HmdMatrix34_t g_EyeMatrixLeft, g_EyeMatrixRight;
//float g_fMetricMult = DEFAULT_METRIC_MULT, 
extern float g_fFrameTimeRemaining;
extern int g_iSteamVR_Remaining_ms, g_iSteamVR_VSync_ms;
extern bool g_bSteamVRPosFromFreePIE;
extern float g_fOBJ_Z_MetricMult, g_fOBJGlobalMetricMult, g_fOBJCurMetricScale;
extern void* g_pSurface;
extern bool g_bTogglePostPresentHandoff;
extern bool g_bSteamVRMirrorWindowLeftEye;

namespace VRButtons
{
	enum VRButtons
	{
		TRIGGER = 0,
		GRIP,
		BUTTON_1,
		BUTTON_2,

		PAD_LEFT,
		PAD_RIGHT,
		PAD_UP,
		PAD_DOWN,
		PAD_CLICK,

		MAX // Sentinel, do not remove
	};
};

struct ControllerState
{
	Matrix4  pose;
	bool     bIsValid;
	bool     buttons[VRButtons::MAX];
	float    trackPadX, trackPadY;
	Vector4  centerFwd, centerUp;
	uint32_t packetNum; // Internal, do not modify
	bool     displayGlove;

	ControllerState()
	{
		pose.identity();
		for (int i = 0; i < VRButtons::MAX; i++)
			buttons[i] = false;
		bIsValid  = false;
		trackPadX = 0;
		trackPadY = 0;
		packetNum = 0xFFFFFFFF;
		displayGlove = true;
	}
};
extern ControllerState g_prevContStates[2];
extern ControllerState g_contStates[2];

enum class KBState
{
	OFF,
	HOVER,
	STATIC,
	CLOSING,
	MAX,
};

struct VRKeybState
{
	KBState state;
	KBState prevState;
	bool    bRendered;
	float   fMetersWidth;
	float   fPixelWidth;
	float   fPixelHeight;
	Matrix4 Transform; // Used to place the keyboard mesh inside a cockpit
	Matrix4 OPTTransform; // Used to test intersections in the Gunner Turret
	Matrix4 InitialTransform; // Captured when the keyb is initially turned on
	int     iHoverContIdx;
	char    sImageName[128];
	int     iGroupId;
	int     iImageId;

	// Region highlighting, sticky keys
	int      iNumStickyRegions;
	uvfloat4 stickyRegions[MAX_VRKEYB_REGIONS];

	// Region highlighting, regular clicks
	uvfloat4 clickRegions[2];

	VRKeybState()
	{
		state        = KBState::OFF;
		prevState    = KBState::OFF;
		bRendered    = false;
		fMetersWidth = 0.30f;
		fPixelWidth  = 1024;
		fPixelHeight = 480;
		Transform.identity();
		OPTTransform.identity();
		InitialTransform.identity();

		iHoverContIdx = 0;
		ClearRegions();

		sprintf_s(sImageName, 128, "%s", ".\\Effects\\ActiveCockpit.dat");
		iGroupId = 0;
		iImageId = 0;
	}

	void ToggleState()
	{
		state = (KBState)(((int)state + 1) % (int)KBState::MAX);
	}

	void AddLitRegion(const uvfloat4& coords)
	{
		if (iNumStickyRegions < MAX_VRKEYB_REGIONS)
		{
			stickyRegions[iNumStickyRegions++] = coords;
		}
	}

	inline void ClearRegions()
	{
		iNumStickyRegions = 0;
	}
};

extern VRKeybState g_vrKeybState;

/*
 *	SteamVR specific functions declarations
 */

bool InitSteamVR();
void ShutDownSteamVR();

void projectSteamVR(float X, float Y, float Z, vr::EVREye eye, float& x, float& y, float& z);
void ProcessSteamVREyeMatrices(vr::EVREye eye);
char* GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL);
void GetSteamVRPositionalData(float* yaw, float* pitch, float* roll, float* x, float* y, float* z, Matrix4* m4_hmdPose);
// WaitGetPoses is now called from the CockpitLook hook.
//bool WaitGetPoses();

void quatToEuler(vr::HmdQuaternionf_t q, float* yaw, float* roll, float* pitch);
vr::HmdQuaternionf_t eulerToQuat(float yaw, float pitch, float roll);
vr::HmdMatrix33_t quatToMatrix(vr::HmdQuaternionf_t q);
vr::HmdQuaternionf_t rotationToQuaternion(vr::HmdMatrix34_t m);
Matrix3 HmdMatrix34toMatrix3(const vr::HmdMatrix34_t& mat);
Matrix3 HmdMatrix33toMatrix3(const vr::HmdMatrix33_t& mat);

void DumpMatrix34(FILE* file, const vr::HmdMatrix34_t& m);
void DumpMatrix44(FILE* file, const vr::HmdMatrix44_t& m);
void DumpMatrix4(FILE* file, const Matrix4& mat);
void ShowMatrix4(const Matrix4& mat, char* name);
void ShowHmdMatrix34(const vr::HmdMatrix34_t& mat, char* name);
void ShowHmdMatrix44(const vr::HmdMatrix44_t& mat, char* name);
void Matrix4toHmdMatrix44(const Matrix4 & m4, vr::HmdMatrix44_t & mat);
void Matrix4toHmdMatrix34(const Matrix4& m4, vr::HmdMatrix34_t& mat);
Matrix4 HmdMatrix44toMatrix4(const vr::HmdMatrix44_t& mat);
Matrix4 HmdMatrix34toMatrix4(const vr::HmdMatrix34_t& mat);
void ShowVector4(const Vector4& X, char* name);