// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>
#include <objbase.h>
#include <hidusage.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#include <dxgi1_3.h>
#endif

#include <stdio.h>
#include <vector>

#include "Vectors.h"
#include "Matrices.h"
#include "config.h"
#include "XwaDrawTextHook.h"
#include "XwaDrawRadarHook.h"
#include "XwaDrawBracketHook.h"
#include "XwaD3dRendererHook.h"
#include "XwaConcourseHook.h"

#include "utils.h"
#include "effects.h"
#include "VRConfig.h"
#include "globals.h"
#include "commonVR.h"
#include "XWAFramework.h"
#include "SharedMem.h"
#include "LBVH.h"

extern HiResTimer g_HiResTimer;
extern PlayerDataEntry* PlayerDataTable;
extern uint32_t* g_playerIndex;
extern float *g_cachedFOVDist; // cached FOV dist / 512.0 (float), seems to be used for some sprite processing

// Current window width and height
int g_WindowWidth, g_WindowHeight;

extern int g_KeySet;
//extern float g_fMetricMult, 
extern float g_fAspectRatio, g_fCockpitTranslationScale;
extern bool g_bTriggerReticleCapture;
extern bool g_bEnableAnimations;
extern bool g_bFadeLights;
extern bool g_bEnableQBVHwSAH;
extern bool g_bUseCentroids;

void RenderEngineGlowHook(void* A4, int A8, void* textureSurface, uint32_t A10, uint32_t A14);
char RenderBackdropsHook();

void Normalize(float4 *Vector) {
	float x = Vector->x;
	float y = Vector->y;
	float z = Vector->z;
	float L = sqrt(x*x + y*y + z*z);
	if (L < 0.001) L = 1.0f;

	Vector->x = x / L;
	Vector->y = y / L;
	Vector->z = z / L;
}

void PrintVector(const Vector4 &Vector) {
	log_debug("[DBG] Vector: %0.3f, %0.3f, %0.3f",
		Vector.x, Vector.y, Vector.z);
}

inline bool InGunnerTurret()
{
	return (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
}

extern bool bFreePIEAlreadyInitialized, g_bDCApplyEraseRegionCommands, g_bEnableLaserLights, g_bDCHologramsVisible;
void ShutdownFreePIE();

// DEBUG
enum HyperspacePhaseEnum;
extern float g_fHyperTimeOverride;
extern int g_iHyperStateOverride;

// ACTIVE COCKPIT
extern Vector4 g_contOriginWorldSpace[2];
extern bool g_bActiveCockpitEnabled, g_bACActionTriggered[2], g_bACTriggerState[2];

extern Vector3 g_LaserPointDebug;

HWND g_ThisWindow = 0;
WNDPROC OldWindowProc = 0;

void IncreaseScreenScale(float Delta); // Changes overall zoom
//void IncreaseFocalDist(float Delta);   // Changes overall zoom after matrix projection
//void IncreasePostProjScale(float Delta);
void IncreaseSMZFactor(float Delta);
void ToggleZoomOutMode();
void IncreaseZoomOutScale(float Delta);
void IncreaseHUDParallax(float Delta);
void IncreaseTextParallax(float Delta);
void IncreaseFloatingGUIParallax(float Delta);
void ToggleCockpitPZHack();
void IncreaseSkipNonZBufferDrawIdx(int Delta);
void IncreaseD3DExecuteCounterSkipHi(int Delta);
void IncreaseD3DExecuteCounterSkipLo(int Delta);

// Lens distortion
void IncreaseLensK1(float Delta);
void IncreaseLensK2(float Delta);

// CSM
void ToggleCSM();
void SetHDRState(bool state);

void IncreaseReticleScale(float delta) {
	g_fReticleScale += delta;
	if (g_fReticleScale < 0.2f)
		g_fReticleScale = 0.2f;
	if (g_fReticleScale > 5.0f)
		g_fReticleScale = 5.0f;
	log_debug("[DBG] g_fReticleScale: %0.3f", g_fReticleScale);
}

void ApplyFocalLength(float focal_length)
{
	log_debug("[DBG] [FOV] Old FOV: %0.3f, Applying: %0.3f", *g_fRawFOVDist, focal_length);
	*g_fRawFOVDist = focal_length;
	*g_cachedFOVDist = *g_fRawFOVDist / 512.0f;
	*g_rawFOVDist = (uint32_t)*g_fRawFOVDist;
	// Force recomputation of the y center:
	g_bYCenterHasBeenFixed = false;
	ComputeHyperFOVParams();
}

void IncreaseFOV(float delta) 
{
	*g_fRawFOVDist += delta;
	*g_cachedFOVDist = *g_fRawFOVDist / 512.0f;
	*g_rawFOVDist = (uint32_t)*g_fRawFOVDist;
	log_debug("[DBG] [FOV] IncreaseFOV. fRawFOV: %0.6f", *g_fRawFOVDist);
	SetCurrentShipFOV(2.0f * atan2(g_fCurInGameHeight, *g_fRawFOVDist) / 0.01745f, true, false);
	// Force recomputation of the y center:
	g_bYCenterHasBeenFixed = false;
	ComputeHyperFOVParams();
}

/*
void IncreaseMetricMult(float delta) 
{
	g_fMetricMult += delta;
	if (g_fMetricMult < 0.1f)
		g_fMetricMult = 0.1f;
	log_debug("[DBG] [FOV] g_fMetricMult: %0.3f", g_fMetricMult);
}
*/

void IncreaseAspectRatio(float delta) {
	g_fAspectRatio += delta;
	g_fConcourseAspectRatio += delta;
	log_debug("[DBG] [FOV] Aspect Ratio: %0.6f, Concourse Aspect Ratio: %0.6f", g_fAspectRatio, g_fConcourseAspectRatio);
}

void POVBackwards()
{
	if (g_pSharedDataCockpitLook != NULL && g_SharedMemCockpitLook.IsDataReady()) {
		if (!InGunnerTurret())
			g_CockpitPOVOffset.z -= POVOffsetIncr;
		else
			g_GunnerTurretPOVOffset.y -= POVOffsetIncr;
		SavePOVOffsetToIniFile();
	}
}

void POVForwards()
{
	if (g_pSharedDataCockpitLook != NULL && g_SharedMemCockpitLook.IsDataReady()) {
		if (!InGunnerTurret())
			g_CockpitPOVOffset.z += POVOffsetIncr;
		else
			g_GunnerTurretPOVOffset.y += POVOffsetIncr;
		SavePOVOffsetToIniFile();
	}
}

void POVUp()
{
	if (g_pSharedDataCockpitLook != NULL && g_SharedMemCockpitLook.IsDataReady()) {
		if (!InGunnerTurret())
			g_CockpitPOVOffset.y += POVOffsetIncr;
		else
			g_GunnerTurretPOVOffset.x += POVOffsetIncr;
		SavePOVOffsetToIniFile();
	}
}

void POVDown()
{
	if (g_pSharedDataCockpitLook != NULL && g_SharedMemCockpitLook.IsDataReady()) {
		if (!InGunnerTurret())
			g_CockpitPOVOffset.y -= POVOffsetIncr;
		else
			g_GunnerTurretPOVOffset.x -= POVOffsetIncr;
		SavePOVOffsetToIniFile();
	}
}

void POVReset()
{
	if (g_bUseSteamVR)
		g_pChaperone->ResetZeroPose(vr::TrackingUniverseSeated);
	g_bResetHeadCenter = true;
	//g_contOriginWorldSpace.set(0.0f, 0.0f, 0.05f, 1);
	g_fFakeRoll = 0.0f;
	//g_contOriginViewSpace = g_contOriginWorldSpace;
	g_LaserPointDebug.x = 0.0f;
	g_LaserPointDebug.y = 0.0f;
	g_LaserPointDebug.z = 0.0f;
	g_fShadowMapAngleX = 0.0f;
	g_fShadowMapAngleY = 0.0f;
	g_fShadowMapScale = 1.0f;
	// Force recalculation of y-center:
	g_bYCenterHasBeenFixed = false;
}

void ResetCockpitShake()
{
	if (g_playerIndex == nullptr)
		return;

	PlayerDataTable[*g_playerIndex].Camera.ShakeX = 0;
	PlayerDataTable[*g_playerIndex].Camera.ShakeY = 0;
	PlayerDataTable[*g_playerIndex].Camera.ShakeZ = 0;
	//DisplayTimedMessage(3, 0, "Resetting Cockpit Shake");
}

#define MAX_ACTION_LEN 10
void RunAction(WORD *action) {
	// Scan codes from: http://www.philipstorr.id.au/pcbook/book3/scancode.htm
	// Scan codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
	// Based on code from: https://stackoverflow.com/questions/18647053/sendinput-not-equal-to-pressing-key-manually-on-keyboard-in-c
	// Virtual key codes: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	// How to send extended scan codes
	// https://stackoverflow.com/questions/36972524/winapi-extended-keyboard-scan-codes/36976260#36976260
	// https://stackoverflow.com/questions/26283738/how-to-use-extended-scancodes-in-sendinput
	INPUT input[MAX_ACTION_LEN];
	bool bEscapedAction = (action[0] == 0xe0);

	if (action[0] == 0) // void action, skip
		return;

	// Copy & initialize the scan codes
	int i = 0, j = bEscapedAction ? 1 : 0;
	while (action[j] && j < MAX_ACTION_LEN) {
		input[i].ki.wScan = action[j];
		input[i].type = INPUT_KEYBOARD;
		input[i].ki.time = 0;
		input[i].ki.wVk = 0;
		input[i].ki.dwExtraInfo = 0;
		input[i].ki.dwFlags = KEYEVENTF_SCANCODE;
		if (bEscapedAction)
			input[i].ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
		i++; j++;
	}

	j = bEscapedAction ? 1 : 0;
	while (action[j] && j < MAX_ACTION_LEN) {
		input[i].ki.wScan = action[j];
		input[i].type = INPUT_KEYBOARD;
		input[i].ki.time = 0;
		input[i].ki.wVk = 0;
		input[i].ki.dwExtraInfo = 0;
		input[i].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
		if (bEscapedAction)
			input[i].ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
		i++; j++;
	}

	// Send keydown/keyup events in one go: (this is the only way I found to enable the arrow/escaped keys)
	SendInput(i, input, sizeof(INPUT));
}

LRESULT CALLBACK MyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	bool AltKey   = (GetAsyncKeyState(VK_MENU)    & 0x8000) == 0x8000;
	bool CtrlKey  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000;
	bool ShiftKey = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) == 0x8000;
	bool UpKey    = (GetAsyncKeyState(VK_UP)      & 0x8000) == 0x8000;
	bool DownKey  = (GetAsyncKeyState(VK_DOWN)    & 0x8000) == 0x8000;
	bool LeftKey  = (GetAsyncKeyState(VK_LEFT)    & 0x8000) == 0x8000;
	bool RightKey = (GetAsyncKeyState(VK_RIGHT)   & 0x8000) == 0x8000;

	/*
	if (AltKey && !CtrlKey && !ShiftKey) {
		if (LeftKey) {
			g_fFakeRoll += 1.0f;
			return 0;
		}
		else if (RightKey) {
			g_fFakeRoll -= 1.0f;
			return 0;
		}
	}
	*/

	switch (uMsg)
	{
	case WM_KEYUP: {
		// Ctrl + Alt + Shift
		if (AltKey && CtrlKey && ShiftKey) {
			switch (wParam) {
			case 0xbb:
				if (g_bDCDebugDisplayLabels)
				{
					g_iDCDebugSrcIndex = (g_iDCDebugSrcIndex + 1) % MAX_DC_SRC_ELEMENTS;
					g_DCDebugLabel = g_DCElemSrcNames[g_iDCDebugSrcIndex];
				}
				else
					IncreaseZoomOutScale(0.1f);
				return 0;
			case 0xbd:
				if (g_bDCDebugDisplayLabels)
				{
					g_iDCDebugSrcIndex--;
					if (g_iDCDebugSrcIndex < 0)
						g_iDCDebugSrcIndex = MAX_DC_SRC_ELEMENTS - 1;
					g_DCDebugLabel = g_DCElemSrcNames[g_iDCDebugSrcIndex];
				}
				else
					IncreaseZoomOutScale(-0.1f);
				return 0;

			case VK_RIGHT:
				switch (g_KeySet) {
				case 1:
					g_LightVector[0].x += 0.1f;
					g_LightVector[0].normalize();
					PrintVector(g_LightVector[0]);
					break;
				case 2:
					IncreaseFOV(50.0f);
					// First try to update the DC file. If that fails, then save the regular focal length
					if (!UpdateXWAHackerFOV())
						SaveFocalLength();
					break;
				case 3:
					g_LaserPointDebug.x += 0.1f;
					log_debug("[DBG] g_LaserPointDebug: %0.3f, %0.3f, %0.3f",
						g_LaserPointDebug.x, g_LaserPointDebug.y, g_LaserPointDebug.z);
					break;
				case 4:
					g_fDebugFOVscale += 0.01f;
					log_debug("[DBG] g_fDebugFOVscale: %0.3f", g_fDebugFOVscale);
					//ComputeHyperFOVParams();
					//(*g_hudScale) += 0.1f;
					//log_debug("[DBG] g_hudScale: %0.3f", *g_hudScale);
					break;
				case 5:
					g_fCockpitTranslationScale += 0.0005f;
					log_debug("[DBG] g_fCockpitTranslationScale: %0.6f", g_fCockpitTranslationScale);
					break;
				case 6:
					g_fShadowMapAngleY += 7.5f;
					log_debug("[DBG] [SHW] g_fLightMapAngleY: %0.3f", g_fShadowMapAngleY);
					break;
				case 8:
					g_fOBJGlobalMetricMult += 0.05f;
					log_debug("[DBG] g_fOBJGlobalMetricMult: %0.3f", g_fOBJGlobalMetricMult);
					break;
				case 9:
					g_fReticleScale += 0.1f;
					log_debug("[DBG] g_fReticleScale: %0.3f", g_fReticleScale);
					break;
				/*case 9:
					g_fReticleOfsX += 0.01f;
					log_debug("[DBG] g_fReticleOfsX: %0.3f", g_fReticleOfsX);
					break;*/
				case 10:
					g_bSteamVRMirrorWindowLeftEye = !g_bSteamVRMirrorWindowLeftEye;
					break;
				case 11:
					g_contOriginWorldSpace[0].z += 0.02f;
					/*log_debug("[DBG] g_contOriginWorldSpace.xyz: %0.3f, %0.3f, %0.3f",
						g_contOriginWorldSpace.x, g_contOriginWorldSpace.y, g_contOriginWorldSpace.z);*/
					break;
				case 12:
					g_bShowBlastMarks = !g_bShowBlastMarks;
					log_debug("[DBG] g_bShowBlastMarks: %d", g_bShowBlastMarks);
					break;
				case 13:
					g_fBlastMarkOfsX += 0.01f;
					log_debug("[DBG] g_fBlastMarkOfsX: %0.6f", g_fBlastMarkOfsX);
					break;
				case 15:
					//g_fRTSoftShadowThresholdMult += 0.05f;
					//log_debug("[DBG] g_fRTSoftShadowThreshold: %0.6f", g_fRTSoftShadowThresholdMult);

					g_fRTGaussFactor += 0.1f;
					log_debug("[DBG] g_fRTGaussFactor: %0.6f", g_fRTGaussFactor);
					break;
				}

				/*
				g_fHyperTimeOverride += 0.1f;
				if (g_fHyperTimeOverride > 2.5f)
					g_fHyperTimeOverride = 2.5f;
				if (g_fHyperTimeOverride >= 1.5f)
					g_iHyperStateOverride = 3; // HYPER_EXIT
				else
					g_iHyperStateOverride = 4; // POST_HYPER_EXIT
				log_debug("[DBG] State: %d, g_fHyperTimeOverride: %0.3f", g_iHyperStateOverride, g_fHyperTimeOverride);
				*/
				return 0;
			case VK_LEFT:
				switch (g_KeySet) {
				case 1:
					g_LightVector[0].x -= 0.1f;
					g_LightVector[0].normalize();
					PrintVector(g_LightVector[0]);
					break;
				case 2:
					IncreaseFOV(-50.0f);
					// First try to update the DC file. If that fails, then save the regular focal length
					if (!UpdateXWAHackerFOV())
						SaveFocalLength();
					break;
				case 3:
					g_LaserPointDebug.x -= 0.1f;
					log_debug("[DBG] g_LaserPointDebug: %0.3f, %0.3f, %0.3f",
						g_LaserPointDebug.x, g_LaserPointDebug.y, g_LaserPointDebug.z);
					break;
				case 4:
					g_fDebugFOVscale -= 0.01f;
					log_debug("[DBG] g_fDebugFOVscale: %0.3f", g_fDebugFOVscale);
					//ComputeHyperFOVParams();
					//(*g_hudScale) -= 0.1f;
					//log_debug("[DBG] g_hudScale: %0.3f", *g_hudScale);
					break;
				case 5:
					g_fCockpitTranslationScale -= 0.0005f;
					log_debug("[DBG] g_fCockpitTranslationScale: %0.6f", g_fCockpitTranslationScale);
					break;
				case 6:
					g_fShadowMapAngleY -= 7.5f;
					log_debug("[DBG] [SHW] g_fLightMapAngleY: %0.3f", g_fShadowMapAngleY);
					break;
				case 8:
					g_fOBJGlobalMetricMult -= 0.05f;
					log_debug("[DBG] g_fOBJGlobalMetricMult: %0.3f", g_fOBJGlobalMetricMult);
					break;
				case 9:
					g_fReticleScale -= 0.1f;
					if (g_fReticleScale < 0.1f)
						g_fReticleScale = 0.1f;
					log_debug("[DBG] g_fReticleScale: %0.3f", g_fReticleScale);
					break;
				/*case 9:
					g_fReticleOfsX -= 0.01f;
					log_debug("[DBG] g_fReticleOfsX: %0.3f", g_fReticleOfsX);
					break;*/
				case 10:
					g_bSteamVRMirrorWindowLeftEye = !g_bSteamVRMirrorWindowLeftEye;
					break;
				case 11:
					g_contOriginWorldSpace[0].z -= 0.02f;
					/*log_debug("[DBG] g_contOriginWorldSpace.xyz: %0.3f, %0.3f, %0.3f",
						g_contOriginWorldSpace.x, g_contOriginWorldSpace.y, g_contOriginWorldSpace.z);*/
					break;
				case 13:
					g_fBlastMarkOfsX -= 0.01f;
					log_debug("[DBG] g_fBlastMarkOfsX: %0.6f", g_fBlastMarkOfsX);
					break;
				case 15:
					//g_fRTSoftShadowThresholdMult -= 0.05f;
					//if (g_fRTSoftShadowThresholdMult < 0.01f) g_fRTSoftShadowThresholdMult = 0.01f;
					//log_debug("[DBG] g_fRTSoftShadowThresholdMult: %0.6f", g_fRTSoftShadowThresholdMult);

					//g_fRTShadowSharpness -= 0.1f;
					//if (g_fRTShadowSharpness < 0.1f) g_fRTShadowSharpness = 0.1f;
					//log_debug("[DBG] g_fRTShadowSharpness: %0.6f", g_fRTShadowSharpness);

					g_fRTGaussFactor -= 0.1f;
					if (g_fRTGaussFactor < 0.1f) g_fRTGaussFactor = 0.01f;
					log_debug("[DBG] g_fRTGaussFactor: %0.6f", g_fRTGaussFactor);
					break;
				}

				/*
				g_fHyperTimeOverride -= 0.1f;
				if (g_fHyperTimeOverride < 0.0f)
					g_fHyperTimeOverride = 0.0f;
				if (g_fHyperTimeOverride >= 1.5f)
					g_iHyperStateOverride = 3; // HYPER_EXIT
				else
					g_iHyperStateOverride = 4; // POST_HYPER_EXIT
				log_debug("[DBG] State: %d, g_fHyperTimeOverride: %0.3f", g_iHyperStateOverride, g_fHyperTimeOverride);
				*/
				return 0;

			case VK_UP:
				switch (g_KeySet) {
				case 1:
					g_LightVector[0].y += 0.1f;
					g_LightVector[0].normalize();
					PrintVector(g_LightVector[0]);
					break;
				case 2:
					//IncreaseMetricMult(0.1f);
					//SaveVRParams();
					break;
				case 3:
					g_LaserPointDebug.y += 0.1f;
					log_debug("[DBG] g_LaserPointDebug: %0.3f, %0.3f, %0.3f",
						g_LaserPointDebug.x, g_LaserPointDebug.y, g_LaserPointDebug.z);
					break;
				case 4:
					//g_fDebugYCenter += 0.01f;
					//log_debug("[DBG] g_fDebugYCenter: %0.3f", g_fDebugYCenter);
					//ComputeHyperFOVParams();
					g_fReticleScale += 0.02f;
					log_debug("[DBG] g_fReticleScale: %0.3f", g_fReticleScale);
					break;
				case 6:
					g_fShadowMapAngleX += 7.5f;
					log_debug("[DBG] [SHW] g_fLightMapAngleX: %0.3f", g_fShadowMapAngleX);
					break;

					/*g_fLightMapDistance += 1.0f;
					log_debug("[DBG] [SHW] g_fLightMapDistance: %0.3f", g_fLightMapDistance);
					break;*/
				/*case 9:
					g_fReticleOfsY -= 0.1f;
					log_debug("[DBG] g_fReticleOfsY: %0.3f", g_fReticleOfsY);
					break;*/
				case 13:
					g_fBlastMarkOfsY -= 0.01f;
					log_debug("[DBG] g_fBlastMarkOfsY: %0.6f", g_fBlastMarkOfsY);
					break;
				case 14:
					g_fGlowMarkZOfs += 0.5f;
					log_debug("[DBG] g_fGlowMarkZOfs: %0.3f", g_fGlowMarkZOfs);
					break;
				}

				return 0;
			case VK_DOWN:
				switch (g_KeySet) {
				case 1:
					g_LightVector[0].y -= 0.1f;
					g_LightVector[0].normalize();
					PrintVector(g_LightVector[0]);
					break;
				case 2:
					//IncreaseMetricMult(-0.1f);
					//SaveVRParams();
					break;
				case 3:
					g_LaserPointDebug.y -= 0.1f;
					log_debug("[DBG] g_LaserPointDebug: %0.3f, %0.3f, %0.3f",
						g_LaserPointDebug.x, g_LaserPointDebug.y, g_LaserPointDebug.z);
					break;
				case 4:
					//g_fDebugYCenter -= 0.01f;
					//log_debug("[DBG] g_fDebugYCenter: %0.3f", g_fDebugYCenter);
					//ComputeHyperFOVParams();
					g_fReticleScale -= 0.02f;
					log_debug("[DBG] g_fReticleScale: %0.3f", g_fReticleScale);
					break;
				case 6:
					g_fShadowMapAngleX -= 7.5f;
					log_debug("[DBG] [SHW] g_fLightMapAngleX: %0.3f", g_fShadowMapAngleX);
					break;

					/*g_fLightMapDistance -= 1.0f;
					log_debug("[DBG] [SHW] g_fLightMapDistance: %0.3f", g_fLightMapDistance);
					break;*/
				/*case 9:
					g_fReticleOfsY += 0.1f;
					log_debug("[DBG] g_fReticleOfsY: %0.3f", g_fReticleOfsY);
					break;*/
				case 13:
					g_fBlastMarkOfsY += 0.01f;
					log_debug("[DBG] g_fBlastMarkOfsY: %0.6f", g_fBlastMarkOfsY);
					break;
				case 14:
					g_fGlowMarkZOfs -= 0.5f;
					log_debug("[DBG] g_fGlowMarkZOfs: %0.3f", g_fGlowMarkZOfs);
					break;
				}

				return 0;

			case 'G':
				g_bSpecularMappingEnabled = !g_bSpecularMappingEnabled;
				if (g_bSpecularMappingEnabled)
					DisplayTimedMessage(3, 0, "Specular Mapping Enabled");
				else
					DisplayTimedMessage(3, 0, "Specular Mapping Disabled");
				return 0;
			}
		}

		// Ctrl + Alt
		if (AltKey && CtrlKey && !ShiftKey) {
			switch (wParam) {

#if DBG_VR
			case 'C':
				log_debug("[DBG] Capture 3D");
				//g_bDo3DCapture = true;
				g_bStart3DCapture = true;
				return 0;

			case 'D':
				// Force a refresh of the display width
				g_bDisplayWidth = true;
				return 0;
			case 'D':
				log_debug("[DBG] Dumping specific texture");
				g_bDumpSpecificTex = true;
				return 0;
				/*case 'T':
					g_bSkipText = !g_bSkipText;
					if (g_bSkipText)
						log_debug("[DBG] Skipping Text GUI");
					else
						log_debug("[DBG] Drawing Text GUI");*/
						/*
						g_bSkipAfterTargetComp = !g_bSkipAfterTargetComp;
						if (g_bSkipAfterTargetComp)
							log_debug("[DBG] Skipping draw calls after TargetComp");
						else
							log_debug("[DBG] NOT Skipping draw calls after TargetComp");
						return 0;
						*/
			case 'F':
				g_bSkipSkyBox = !g_bSkipSkyBox;
				if (g_bFixSkyBox)
					log_debug("[DBG] Skip Skybox ON");
				else
					log_debug("[DBG] Skip Skybox OFF");
				return 0;

			case 'G':
				g_bSkipGUI = !g_bSkipGUI;
				log_debug("[DBG] g_bSkipGUI: %d", g_bSkipGUI);
				return 0;
#endif
			// Ctrl + Alt + Key
			// Toggle Debug buffers
			case 'D':
				g_bEnableCubeMaps = !g_bEnableCubeMaps;
				if (g_bEnableCubeMaps)
					DisplayTimedMessage(3, 0, "CubeMaps ENABLED");
				else
					DisplayTimedMessage(3, 0, "CubeMaps disabled");

				/*
				// g_bRenderDefaultStarfield is used to render DefaultStarfield.dds. It's set
				// to "true" by default. This switch toggles that DDS render:
				g_bRenderDefaultStarfield = !g_bRenderDefaultStarfield;
				if (g_bRenderDefaultStarfield)
					DisplayTimedMessage(3, 0, "RENDER Default Starfield");
				else
					DisplayTimedMessage(3, 0, "NO Default Starfield");
				*/

				//g_bDumpOptNodes = !g_bDumpOptNodes;
				//log_debug("[DBG] g_bDumpOptNodes: %d", g_bDumpOptNodes);

				//g_bFadeLights = !g_bFadeLights;
				//log_debug("[DBG] g_bFadeLights: %d", g_bFadeLights);

				//g_bDisplayGlowMarks = !g_bDisplayGlowMarks;
				//log_debug("[DBG] g_bDisplayGlowMarks: %d", g_bDisplayGlowMarks);
				//g_bShowSSAODebug = !g_bShowSSAODebug;
				//log_debug("[DBG] g_bShowSSAODebug: %d", g_bShowSSAODebug);
				return 0;
			// Toggle FXAA
			case 'F':
				g_config.FXAAEnabled = !g_config.FXAAEnabled;
				if (g_config.FXAAEnabled)
					DisplayTimedMessage(3, 0, "FXAA Enabled");
				else
					DisplayTimedMessage(3, 0, "FXAA Disabled");
				return 0;
			// Dump Debug buffers
			case 'X':
				g_bDumpSSAOBuffers = true;
				return 0;
			//case 'G':
			//	DumpGlobalLights();
			//	return 0;
			case 'G':
				//g_bDumpLaserPointerDebugInfo = true;
				/*
				g_bEnableGimbalLockFix = !g_bEnableGimbalLockFix;
				DisplayTimedMessage(3, 0, g_bEnableGimbalLockFix ? "Gimbal Lock Fix ON" : "Regular Joystick Controls");
				log_debug(g_bEnableGimbalLockFix ? "[DBG] Gimbal Lock Fix ON" : "[DBG] Regular Joystick Controls");
				*/

				g_bAutoGreeblesEnabled = !g_bAutoGreeblesEnabled;
				DisplayTimedMessage(3, 0, g_bAutoGreeblesEnabled ? "Greebles Enabled" : "Greebles Disabled");
				return 0;
				// DEBUG
			// Ctrl + Alt + P
			case 'P':
				g_bEnableIndirectSSDO = !g_bEnableIndirectSSDO;
				if (g_bEnableIndirectSSDO)
					DisplayTimedMessage(3, 0, "Indirect SSDO Enabled");
				else
					DisplayTimedMessage(3, 0, "Indirect SSDO Disabled");
				return 0;
			// Ctrl+Alt+A: Toggle Bloom
			case 'A':
				g_bBloomEnabled = !g_bBloomEnabled;
				if (g_bBloomEnabled)
					DisplayTimedMessage(3, 0, "Bloom Enabled");
				else
					DisplayTimedMessage(3, 0, "Bloom Disabled");
				/*
				g_bEnableAnimations = !g_bEnableAnimations;
				if (g_bEnableAnimations)
					DisplayTimedMessage(3, 0, "Animations Enabled");
				else
					DisplayTimedMessage(3, 0, "Animations Disabled");
				*/
				return 0;
			// Ctrl+Alt+O
			case 'O':
				g_bAOEnabled = !g_bAOEnabled;
				return 0;
			case 'N':
				// Toggle Normal Mapping
				g_bFNEnable = !g_bFNEnable;
				if (g_bFNEnable)
					DisplayTimedMessage(3, 0, "Normal Mapping Enabled");
				else
					DisplayTimedMessage(3, 0, "Normal Mapping Disabled");
				return 0;
			// Ctrl+Alt+H
			case 'H':
				//g_bHDREnabled = !g_bHDREnabled;
				g_bDCApplyEraseRegionCommands = !g_bDCApplyEraseRegionCommands;
				//g_bGlobalDebugFlag = !g_bGlobalDebugFlag;
				return 0;

			case 'B':
				g_bDisableBarrelEffect = !g_bDisableBarrelEffect;
				SaveVRParams();
				return 0;
			case 'R':
				ResetVRParams();
				DCResetSubRegions();
				return 0;

			case 'V':
				// We can't just switch to VR mode if the game was started with no VR support
				if (!g_bEnableVR) {
					if (g_bSteamVRInitialized || g_bDirectSBSInitialized)
						g_bEnableVR = true;
				}
				else { // VR mode can always be disabled
					g_bEnableVR = !g_bEnableVR;
				}
				return 0;
			// Ctrl+Alt+S
			case 'S':
				SaveVRParams();
				DisplayTimedMessage(3, 0, "VRParams.cfg Saved");
				return 0;
			//case 'T':
			//	g_bToggleSkipDC = !g_bToggleSkipDC;
			//	return 0;
			case 'L':
				// Force the re-application of the focal_length
				g_bCustomFOVApplied = false;
				LoadVRParams();
				return 0;
			
			// Ctrl+Alt+W
			case 'W':
				//ToggleCSM();

				g_iDelayedDumpDebugBuffers = 160;
				log_debug("[DBG] Delayed debug dump set");

				//g_bGlobalSpecToggle = !g_bGlobalSpecToggle;
				/*
				if (g_fSpecIntensity > 0.5f) {
					g_fSpecIntensity = 0.0f;
					g_fSpecBloomIntensity = 0.0f;
				} 
				else {
					g_fSpecIntensity = 1.0f;
					g_fSpecBloomIntensity = 1.25f;
				}
				*/
				return 0;
			case 'K':
				g_vrKeybState.ToggleState();
				log_debug("[DBG] [AC] VR Keyboard state: %d", (int)g_vrKeybState.state);
				return 0;
			// Ctrl+Alt+E is used by the Custom OpenVR driver to toggle emulation modes
			/*
			case 'E':
				//g_bEnableSSAOInShader = !g_bEnableSSAOInShader;
				g_bEdgeDetectorEnabled = !g_bEdgeDetectorEnabled;
				return 0;
			*/
			/*
			case 'Q':
				g_bActiveCockpitEnabled = !g_bActiveCockpitEnabled;
				return 0;
			*/

			/*
			
			*/

			case 0xbb:
				IncreaseScreenScale(0.1f);
				return 0;
			case 0xbd:
				IncreaseScreenScale(-0.1f);
				return 0;

			// Ctrl + Alt + Key
			case VK_UP:
				IncreaseLensK1(0.1f);
				SaveVRParams();
				return 0;
			case VK_DOWN:
				IncreaseLensK1(-0.1f);
				SaveVRParams();
				return 0;
			case VK_LEFT:
				IncreaseLensK2(-0.1f);
				SaveVRParams();
				/*g_fLPdebugPointOffset -= 0.05f;
				if (g_fLPdebugPointOffset < 0.0f)
					g_fLPdebugPointOffset = 0.0f;*/
				//log_debug("[DBG] [AC] g_fDebugZCenter: %0.4f", g_fDebugZCenter);
				//IncreaseFocalDist(-0.1f);
				return 0;
			case VK_RIGHT:
				IncreaseLensK2(0.1f);
				SaveVRParams();
				//g_fLPdebugPointOffset += 0.05f;
				//log_debug("[DBG] [AC] g_fDebugZCenter: %0.4f", g_fDebugZCenter);
				//IncreaseFocalDist(0.1f);
				return 0;
			}
		}

		// Ctrl
		if (CtrlKey && !AltKey && !ShiftKey) {
			switch (wParam) {
			case 0xbb:
				IncreaseD3DExecuteCounterSkipLo(1);
				return 0;
			case 0xbd:
				IncreaseD3DExecuteCounterSkipLo(-1);
				return 0;
			case 'Z':
				ToggleZoomOutMode();
				return 0;
			// Ctrl+H will toggle the headlights now
			case 'H':
				if (g_bInTechRoom)
					g_config.TechRoomHolograms = !g_config.TechRoomHolograms;
				else
					g_bDCHologramsVisible = !g_bDCHologramsVisible;
				return 0;
			// Headlights must be automatic now. They are automatically turned on in the Death Star mission
			/*
			case 'H':
				g_bEnableHeadLights = !g_bEnableHeadLights;
				if (g_bEnableHeadLights)
					DisplayTimedMessage(3, 0, "Headlights ON");
				else
					DisplayTimedMessage(3, 0, "Headlights OFF");
				return 0;
			*/
			// Ctrl+O
			//case 'O':
			//	g_bAOEnabled = !g_bAOEnabled;
			//	return 0;

			/*
			case 'K': {
				WORD action[MAX_ACTION_LEN];
				action[0] = 0x46; // Scroll Lock
				action[1] = 0x0;
				log_debug("[DBG] Sending Scroll Lock");
				RunAction(action);
				return 0;
			}
			*/

			// Ctrl+C: Force cockpit damage (helps test the cockpit damage animations)
			case 'C':
				g_bApplyCockpitDamage = true;
				return 0;

			// Ctrl+D --> Toggle dynamic lights
			case 'D': {
				g_bEnableLaserLights = !g_bEnableLaserLights;
				return 0;
			}

			// Ctrl+K --> Toggle Mouse Look
			case 'K': {
				*mouseLook = !*mouseLook;
				DisplayTimedMessage(3, 0, *mouseLook ? "Mouse Look ON" : "Mouse Look OFF");
				log_debug("[DBG] mouseLook: %d", *mouseLook);
				return 0;
			}

			// Ctrl+R
			case 'R': {
				//g_bResetDC = true;
				//g_bProceduralSuns = !g_bProceduralSuns;
				//g_bShadowMapDebug = !g_bShadowMapDebug;
				//g_config.EnableSoftHangarShadows = !g_config.EnableSoftHangarShadows;
				//log_debug("[DBG] EnableSoftHangarShadows: %d", g_config.EnableSoftHangarShadows);

#undef DEBUG_RT
#ifdef DEBUG_RT
				if (g_bInTechRoom) {
					g_bEnableQBVHwSAH = !g_bEnableQBVHwSAH;
					log_debug("[DBG] [BVH] g_bEnableQBVHwSAH: %d", g_bEnableQBVHwSAH);
				}
#endif

				g_bRTEnabled = !g_bRTEnabled;
				log_debug("[DBG] [BVH] g_bRTEnabled: %d", g_bRTEnabled);
				DisplayTimedMessage(3, 0, g_bRTEnabled ? "Raytracing Enabled" : "Raytracing Disabled");
				if (!g_bRTEnabled)
				{
					g_bShadowMapEnable = true;
					g_bRTEnabledInCockpit = false;
				}
				else
				{
					g_bShadowMapEnable = false;
					g_bRTEnabledInCockpit = true;
				}
				//SetHDRState(g_bRTEnabled);
				// Force the Deferred rendering mode when RT is enabled
				//if (g_bRTEnabled) g_SSAO_Type = SSO_DEFERRED;

				return 0;
			}
			// There's a hook by Justagai that uses Ctrl+T to toggle the CMD, so let's use another key
			//case 'T': {
				//g_bShadowMapEnablePCSS = !g_bShadowMapEnablePCSS;
				//g_bDCHologramsVisible = !g_bDCHologramsVisible;
				//return 0;
			//}
			// Ctrl+S
			case 'S': {
#define BENCHMARK_MODE 0
#if BENCHMARK_MODE == 0
				g_bRTEnabledInTechRoom = !g_bRTEnabledInTechRoom;
				log_debug("[DBG] [BVH] g_bRTEnabledInTechRoom: %d", g_bRTEnabledInTechRoom);

				/*g_bRTEnabled = !g_bRTEnabled;
				log_debug("[DBG] [BVH] g_bRTEnabled: %s", g_bRTEnabled ? "Enabled" : "Disabled");
				DisplayTimedMessage(3, 0, g_bRTEnabled ? "Raytracing Enabled" : "Raytracing Disabled");*/

				g_bShadowMapEnable = !g_bShadowMapEnable;
				DisplayTimedMessage(3, 0, g_bShadowMapEnable ? "Shadow Mapping Enabled" : "Shadow Mapping Disabled");
#elif BENCHMARK_MODE == 1
				g_BLASBuilderType = (BLASBuilderType)(((int)g_BLASBuilderType + 1) % (int)BLASBuilderType::MAX);
				log_debug("[DBG] [BVH] Builder type set to: %s", g_sBLASBuilderTypeNames[(int)g_BLASBuilderType]);
#elif BENCHMARK_MODE == 2
				g_TLASBuilderType = (TLASBuilderType)(((int)g_TLASBuilderType + 1) % (int)TLASBuilderType::MAX);
				log_debug("[DBG] [BVH] TLAS Builder type set to: %s", g_sTLASBuilderTypeNames[(int)g_TLASBuilderType]);
#elif BENCHMARK_MODE == 3
				g_bUseCentroids = !g_bUseCentroids;
				if (g_bUseCentroids)
				{
					DisplayTimedMessage(3, 0, "Using Centroids for TLAS");
					log_debug("[DBG] [BVH] Using Centroids for TLAS");
				}
				else
				{
					DisplayTimedMessage(3, 0, "Using Center-of-Mass for TLAS");
					log_debug("[DBG] [BVH] Using Center-of-Mass for TLAS");
				}
#endif
				return 0;
			}
			//case 'E': {
			//	g_bShadowMapHardwarePCF = !g_bShadowMapHardwarePCF;
			//	log_debug("[DBG] [SHW] g_bShadowMapHardwarePCF: %d", g_bShadowMapHardwarePCF);
			//	return 0;
			//}

			case 'F': {
				CycleFOVSetting();
				return 0;
			}
			// Ctrl+W
			case 'W': {
				if (g_bEnableDeveloperMode)
					g_config.OnlyGrayscale = !g_config.OnlyGrayscale;
				return 0;
			}
			case 'V':
			{
				g_bTriggerReticleCapture = true;
				g_bCustomFOVApplied = false; // Force reapplication/recomputation of FOV
				return 0;
			}

			// Ctrl+L is the landing gear

			// Ctrl+P Toggle PBR Shading
			//case 'P':
				//g_bEnablePBRShading = !g_bEnablePBRShading;
				//DisplayTimedMessage(3, 0, g_bEnablePBRShading ? "PBR Shading Enabled" : "Regular Shading");
				//log_debug("[DBG] PBR Shading %s", g_bEnablePBRShading ? "Enabled" : "Disabled");
				//g_bTogglePostPresentHandoff = !g_bTogglePostPresentHandoff;
				//log_debug("[DBG] PostPresentHandoff: %d", g_bTogglePostPresentHandoff);

				/*
				// Ctrl+P SteamVR screenshot (doesn't seem to work terribly well, though...)
				if (g_bUseSteamVR && g_pVRScreenshots != NULL) {
					static int scrCounter = 0;
					char prevFileName[80], scrFileName[80];
					sprintf_s(prevFileName, 80, "./preview%d", scrCounter);
					sprintf_s(scrFileName, 80, "./screenshot%d", scrCounter);

					vr::ScreenshotHandle_t scr;
					vr::EVRScreenshotError error = g_pVRScreenshots->TakeStereoScreenshot(&scr, prevFileName, scrFileName);
					if (error)
						log_debug("[DBG] error %d when taking SteamVR screenshot", error);
					else
						log_debug("[DBG] Screenshot %d taken", scrCounter);
					scrCounter++;
				}
				else {
					log_debug("[DBG] !g_bUseSteamVR || g_pVRScreenshots is NULL");
				}
				*/
				//break;

#if DBR_VR
			case 'X':
				g_bCapture2DOffscreenBuffer = true;
				return 0;
			case 'D':
				g_bDumpDebug = !g_bDumpDebug;
				return 0;
#endif

			// Ctrl + Up
			case VK_UP:
				switch (g_KeySet) {
				case 1:
					g_LightVector[0].z += 0.1f;
					g_LightVector[0].normalize();
					PrintVector(g_LightVector[0]);
					break;
				case 2:
					//IncreaseScreenScale(0.1f);
					//SaveVRParams();

					//IncreaseAspectRatio(0.05f);
					//IncreaseReticleScale(0.1f);
					//SaveVRParams();
					break;
				case 3:
					g_LaserPointDebug.z += 0.1f;
					log_debug("[DBG] g_LaserPointDebug: %0.3f, %0.3f, %0.3f",
						g_LaserPointDebug.x, g_LaserPointDebug.y, g_LaserPointDebug.z);
					break;
				case 6:
					g_fShadowMapScale += 0.1f;
					log_debug("[DBG] [SHW] g_fLightMapScale: %0.3f", g_fShadowMapScale);
					break;
				case 7:
					g_fHDRWhitePoint *= 2.0f;
					log_debug("[DBG] white point: %0.3f", g_fHDRWhitePoint);
					break;
				case 8:
					g_fOBJ_Z_MetricMult += 5.0f;
					log_debug("[DBG] g_fOBJMetricMult: %0.3f", g_fOBJ_Z_MetricMult);
					break;
				case 11:
					g_contOriginWorldSpace[0].y += 0.01f;
					//log_debug("[DBG] g_contOriginWorldSpace.xy: %0.3f, %0.3f", g_contOriginWorldSpace.x, g_contOriginWorldSpace.y);
					break;
				}
				return 0;
				// Ctrl + Down
			case VK_DOWN:
				switch (g_KeySet) {
				case 1:
					g_LightVector[0].z -= 0.1f;
					g_LightVector[0].normalize();
					PrintVector(g_LightVector[0]);
					break;
				case 2:
					//IncreaseScreenScale(-0.1f);
					//SaveVRParams();

					//IncreaseAspectRatio(-0.05f);
					//IncreaseReticleScale(-0.1f);
					//SaveVRParams();
					break;
				case 3:
					g_LaserPointDebug.z -= 0.1f;
					log_debug("[DBG] g_LaserPointDebug: %0.3f, %0.3f, %0.3f",
						g_LaserPointDebug.x, g_LaserPointDebug.y, g_LaserPointDebug.z);
					break;
				case 6:
					g_fShadowMapScale -= 0.1f;
					if (g_fShadowMapScale < 0.1f) g_fShadowMapScale = 0.2f;
					log_debug("[DBG] [SHW] g_fLightMapScale: %0.3f", g_fShadowMapScale);
					break;
				case 7:
					g_fHDRWhitePoint /= 2.0f;
					if (g_fHDRWhitePoint < 0.125f)
						g_fHDRWhitePoint = 0.125f;
					log_debug("[DBG] white point: %0.3f", g_fHDRWhitePoint);
					break;
				case 8:
					g_fOBJ_Z_MetricMult -= 5.0f;
					log_debug("[DBG] g_fOBJMetricMult: %0.3f", g_fOBJ_Z_MetricMult);
					break;
				case 11:
					g_contOriginWorldSpace[0].y -= 0.01f;
					//log_debug("[DBG] g_contOriginWorldSpace.xy: %0.3f, %0.3f", g_contOriginWorldSpace.x, g_contOriginWorldSpace.y);
					break;
				}
				return 0;
			// Ctrl + Left
			case VK_LEFT:
				switch (g_KeySet) {
				case 2:
					break;
				case 11:
					g_contOriginWorldSpace[0].x -= 0.01f;
					//log_debug("[DBG] g_contOriginWorldSpace.xy: %0.3f, %0.3f", g_contOriginWorldSpace.x, g_contOriginWorldSpace.y);
					break;
				}
				return 0;
			// Ctrl + Right
			case VK_RIGHT:
				switch (g_KeySet) {
				case 2:
					break;
				case 11:
					g_contOriginWorldSpace[0].x += 0.01f;
					//log_debug("[DBG] g_contOriginWorldSpace.xy: %0.3f, %0.3f", g_contOriginWorldSpace.x, g_contOriginWorldSpace.y);
					break;
				}
				return 0;

			}
		}

		// Ctrl + Shift
		if (CtrlKey && !AltKey && ShiftKey)
		{
			switch (wParam)
			{
			case 0xbb:
				//IncreaseNoExecIndices(0, 1);
				IncreaseD3DExecuteCounterSkipHi(1);
				return 0;
			case 0xbd:
				//IncreaseNoExecIndices(0, -1);
				IncreaseD3DExecuteCounterSkipHi(-1);
				return 0;
				// Ctrl+Shift+C: Reset the cockpit damage
			case 'C':
				g_bResetCockpitDamage = true;
				return 0;
				/*
				case 'T': {
					g_bDCHologramsVisible = !g_bDCHologramsVisible;
					return 0;
				}
				*/
			// Ctrl+Shift+R: Toggle Raytraced Cockpit Shadows
			case 'R':
				g_bRTEnabledInCockpit = !g_bRTEnabledInCockpit;
				log_debug("[DBG] Raytraced Cockpit Shadows: %d", g_bRTEnabledInCockpit);
				DisplayTimedMessage(3, 0, g_bRTEnabledInCockpit ?
					"Raytraced Cockpit Shadows" : "Shadow Mapped Cockpit Shadows");
				return 0;
			// Ctrl+Shift+W: Toggle keyboard joystick emulation
			case 'W':
				if (g_config.KbdSensitivity > 0.0f) {
					g_config.KbdSensitivity = 0.0f;
					DisplayTimedMessage(3, 0, "Joystick Emul Paused");
				}
				else {
					g_config.KbdSensitivity = 1.0f;
					DisplayTimedMessage(3, 0, "Joystick Emul Resumed");
				}
				log_debug("[DBG] Keyboard enabled: %d", (bool)g_config.KbdSensitivity);
				return 0;
			case VK_UP:
				IncreaseFloatingGUIParallax(0.05f);
				return 0;
			case VK_DOWN:
				IncreaseFloatingGUIParallax(-0.05f);
				return 0;
			case VK_LEFT:
				IncreaseTextParallax(-0.05f);
				return 0;
			case VK_RIGHT:
				IncreaseTextParallax(0.05f);
				return 0;
			}
		}

		// Shift
		if (ShiftKey && !AltKey && !CtrlKey) {
			switch (wParam) {
			// Shift + Arrow Keys
			case VK_LEFT:
				// Adjust the POV in VR, see CockpitLook
				POVBackwards();
				return 0;
			case VK_RIGHT:
				// Adjust the POV in VR, see CockpitLook
				POVForwards();
				return 0;
			case VK_UP:
				// Adjust the POV in VR, see CockpitLook
				POVUp();
				return 0;
			case VK_DOWN:
				// Adjust the POV in VR, see CockpitLook
				POVDown();
				return 0;

			case VK_OEM_PERIOD:
				log_debug("[DBG] Resetting POVOffset for %s", g_sCurrentCockpit);
				if (g_pSharedDataCockpitLook != NULL && g_SharedMemCockpitLook.IsDataReady()) {
					if (!InGunnerTurret())
						g_CockpitPOVOffset = { 0, 0, 0 };
					else
						g_GunnerTurretPOVOffset = { 0, 0, 0 };
					SavePOVOffsetToIniFile();
				}

				return 0;
			}
		}

		// Plain key: no Shift, Ctrl, Alt
		if (!ShiftKey && !AltKey && !CtrlKey) {
			switch (wParam) {
			case VK_SPACE:
				// Exit hyperspace
				g_bKeybExitHyperspace = true;

				// ACTIVE COCKPIT:
				// Use the spacebar to activate the cursor on the current active element
				if (g_bActiveCockpitEnabled) {
					//g_bACActionTriggered = true;
					g_bACTriggerState[0] = false;
					g_bACTriggerState[1] = false;
				}

				// Custom HUD color. Turns out that pressing H followed by the Spacebar restarts
				// the current mission. The clock isn't reset and nothing else changes. So the only
				// way I could find to detect this change is by catching the Spacebar event itself.
				// When a mission is reset this way, the custom HUD color is also reset. So, let's
				// reapply it here (if there is no custom HUD color, the following call does nothing)
				ApplyCustomHUDColor();
				break;

			case VK_NUMPAD5:
				ResetCockpitShake();
				break;

			case VK_OEM_PERIOD:
				POVReset();
				ResetCockpitShake();
				break;

			case 'D' :
				if (g_bDCEnabled)
					g_bDCApplyEraseRegionCommands = !g_bDCApplyEraseRegionCommands;
				else
					// Never erase the HUD if DC is off!
					g_bDCApplyEraseRegionCommands = false;
				break;
			}
		}

		// Alt doesn't seem to work by itself: wParam is messed up. See the beginning
		// of this function for an example of how to handle the Alt key properly
		/*
		if (!ShiftKey && AltKey && !CtrlKey) {
			switch (wParam) {
			case VK_LEFT:
				log_debug("[DBG] [AC] Alt+Left");
				return 0;
			case VK_RIGHT:
				log_debug("[DBG] [AC] Alt+Right");
				return 0;
			}
		}
		*/
		
		break;
	}

	case WM_KEYDOWN: {
		// Plain key: no Shift, Ctrl, Alt
		if (!ShiftKey && !AltKey && !CtrlKey) {
			switch (wParam) {
			case VK_SPACE:
				if (g_bActiveCockpitEnabled)
				{
					g_bACTriggerState[0] = true;
					g_bACTriggerState[1] = true;
				}
				return 0;
			}

		}

		/*
		// Shift
		if (ShiftKey && !AltKey && !CtrlKey) {
			switch (wParam) {
			case VK_UP:
				g_bUpKeyDownShift = true;
				return 0;
			case VK_DOWN:
				g_bDownKeyDownShift = true;
				return 0;
			}
		}
		*/
		return 0;
	}

	/*
	case WM_INPUT:
	{
		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];

		// GetRawInputBuffer() can't be read inside the main message loop
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)lpb;

		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			g_iMouseDeltaX += raw->data.mouse.lLastX;
			g_iMouseDeltaY += raw->data.mouse.lLastY;
			//log_debug("[DBG] raw delta: %d, %d", raw->data.mouse.lLastX, raw->data.mouse.lLastY);
			//g_bMouseDeltaReady = true;
			//InsertMouseDelta(g_iMouseDeltaX, g_iMouseDeltaY);
		}
		break;
	}
	*/

	}
	
	// Call the previous WindowProc handler
	return CallWindowProc(OldWindowProc, hwnd, uMsg, wParam, lParam);
}

bool ReplaceWindowProc(HWND hwnd)
{
	RECT rect;

	// Register the mouse for raw input. This will allow us to receive low-level mouse deltas
	// DISABLED: I suspect that registering for low-level events messes the main message pump
#ifdef DISABLED
	RAWINPUTDEVICE Rid[1];
	Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	Rid[0].hwndTarget = hwnd;
	if (!RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])))
		log_debug("[DBG] Failed to register raw input device");
#endif

	g_ThisWindow = hwnd;
	OldWindowProc = (WNDPROC )SetWindowLong(g_ThisWindow, GWL_WNDPROC, (LONG )MyWindowProc);
	if (OldWindowProc != NULL) {
		// SetProcessDPIAware(); // Reimar suggested using this to fix the DPI
		GetWindowRect(g_ThisWindow, &rect);
		log_debug("[DBG] RECT size: (%d, %d)-(%d, %d)", rect.left, rect.top, rect.right, rect.bottom);
		g_WindowWidth = rect.right - rect.left;
		g_WindowHeight = rect.bottom - rect.top;
		return true;
	}
	
	return false;
}

bool IsXwaExe()
{
	char filename[4096];

	if (GetModuleFileName(nullptr, filename, sizeof(filename)) == 0)
	{
		return false;
	}

	int length = strlen(filename);

	if (length < 17)
	{
		return false;
	}

	return _stricmp(filename + length - 17, "xwingalliance.exe") == 0;
}

void PatchWithValue(uint32_t address, unsigned char value, int size) {
	DWORD old, dummy;
	BOOL res;
	res = VirtualProtect((void *)address, 1, PAGE_READWRITE, &old);
	if (res) {
		//log_debug("[DBG] Patching address 0x%x", address);
		memset((unsigned char *)address, value, size);
		VirtualProtect((void *)address, 1, old, &dummy);
	}
	else
		log_debug("[DBG] Could not patch address 0x%x", address);
}

void LoadPOVOffsets() {
	log_debug("[DBG] [POV] Loading POVOffsets.cfg...");
	FILE *file;
	char buf[160], param[80], svalue[80];
	float fValue;
	bool bApplyPOVs = false;
	int slot, x, y, z, num_params, entries_applied = 0, error = 0;
	const char *CraftTableBase = (char *)0x5BB480;
	const int16_t EntrySize = 0x3DB, POVOffset = 0x238;
	// 0x5BB480 + (n-1) * 0x3DB + 0x238

	// POV Offsets will only be applied if VR is enabled
	//if (!g_bEnableVR) return;

	try {
		error = fopen_s(&file, "./POVOffsets.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] [POV] Could not load POVOffsets.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] [POV] Error %d when loading POVOffsets.cfg", error);
		return;
	}

	while (fgets(buf, 160, file) != NULL) {
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (strstr(buf, "=") != NULL) 
		{
			if (sscanf_s(buf, "%s = %s", param, 80, svalue, 80) > 0) {
				fValue = (float)atof(svalue);
				
				if (_stricmp(param, "apply_custom_VR_POVs") == 0) {
					if ((bool)fValue && g_bEnableVR) {
						bApplyPOVs = true;
						log_debug("[DBG] [POV] Applying Custom VR POVs: %d", bApplyPOVs);
					}
				}

				if (_stricmp(param, "force_apply_custom_POVs") == 0) {
					if ((bool)fValue) {
						bApplyPOVs = true;
						log_debug("[DBG] [POV] Applying Custom POVs: %d", bApplyPOVs);
					}
				}

			}
		}
		else 
		{
			num_params = sscanf_s(buf, "%d %d %d %d", &slot, &x, &z, &y);
			if (num_params == 4) 
			{
				// I know it's weird to apply the exit here, but this works because
				// *this* is when we start reading the POV lines, otherwise, blank lines, etc
				// will also take the "else" path and if we put the exit there, we won't
				// read any POVs.
				if (!bApplyPOVs) {
					log_debug("[DBG] [POV] POVOffsets will NOT be applied");
					goto out;
				}

				int16_t *pov = (int16_t *)(CraftTableBase + (slot - 1) * EntrySize + POVOffset);
				// Y, Z, X
				*pov += y; pov++;
				*pov += z; pov++;
				*pov += x;
				entries_applied++;
			}
		}
	}

out:
	fclose(file);
	log_debug("[DBG] [POV] %d POV entries modified", entries_applied);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		log_debug("[DBG] **********************");
		log_debug("[DBG] Initializing VR ddraw.dll");
		QueryPerformanceFrequency(&(g_HiResTimer.PC_Frequency));
		// Initialize the libraries needed to dump DirectX Textures
		CoInitialize(NULL);
		// Load vrparams.cfg if present
		LoadVRParams();
		// Initialize SteamVR
		if (g_bSteamVREnabled && !g_bSteamVRInitialized) {
			g_bUseSteamVR = InitSteamVR();
			log_debug("[DBG] g_bUseSteamVR: %d", g_bUseSteamVR);
			// Do not attempt initalization again, no matter what happened
			g_bSteamVRInitialized = true;
		}
		else if (g_bEnableVR && !g_bUseSteamVR) {
			log_debug("[DBG] Initializing DirectSBS mode");
			InitDirectSBS();
			g_bDirectSBSInitialized = true;
		}
		g_fDefaultFOVDist = *g_fRawFOVDist;
		log_debug("[DBG] [FOV] Default FOV Dist: %0.3f", g_fDefaultFOVDist);

		InitSharedMem();

		// Embree appears to cause random lockups in the Briefing Room and the
		// Tech Room. Not sure why, as I can't break into the process to see what's
		// going on, and since it's not a crash, there isn't a crash dump either.
		// So let's disable this altogether while I think about what to do next
		//if (g_bRTEnableEmbree)
		//{
		//	LoadEmbree();
		//}
		{
			g_bRTEnableEmbree = false;
			g_BLASBuilderType = DEFAULT_BLAS_BUILDER;
			log_debug("[DBG] [BVH] [EMB] Embree was not loaded. Using DEFAULT_BVH_BUILDER instead");
		}

		if (IsXwaExe())
		{
			{
				// Hook the Xwa3dRenderEngineGlow() function. This function is located
				// at 0x044B590 and it's called from two places 0x0042D91A and 0x0042DB0E
				// (we know this from looking at IDA and pressing X to see the cross-references).
				// I think only 0x0042D91A needs to be hooked, but here we're hooking both:
				uint32_t addr;
				addr = 0x0042D91A;
				*(unsigned char*)(addr + 0x00) = 0xE8;
				*(int*)(addr + 0x01) = (int)RenderEngineGlowHook - (addr + 0x05);

				addr = 0x0042DB0E;
				*(unsigned char*)(addr + 0x00) = 0xE8;
				*(int*)(addr + 0x01) = (int)RenderEngineGlowHook - (addr + 0x05);
			}

			if (g_EnhancedHUDData.Enabled)
			{
				uint32_t addr;

				// Locations 0x00401648 and 0x00401691 are calls to TargetBox()
				// for a non-subcomponent bracket.
				addr = 0x00401648;
				*(unsigned char*)(addr + 0x00) = 0xE8;
				*(int*)(addr + 0x01) = (int)TargetBoxHook - (addr + 0x05);

				addr = 0x00401691;
				*(unsigned char*)(addr + 0x00) = 0xE8;
				*(int*)(addr + 0x01) = (int)TargetBoxHook - (addr + 0x05);

				// Location 0x40157C is a call to TargetBox for a subcomponent bracket.
				// Here we make sure the instruction is a call (0xE8):
				addr = 0x0040157C;
				*(unsigned char*)(addr + 0x00) = 0xE8;
				// Now we replace the destination of the call with our hook:
				*(int*)(addr + 0x01) = (int)TargetBoxHook - (addr + 0x05);
			}

			// Backdrops hook. Doesn't do much for now, but we could render
			// the cubemaps in this hook.
			{
				// 0x0405FE0: void XwaRenderBackdrops()
				uint32_t addr;

				addr = 0x045A5B7; // Hangar render loop calls RenderBackdrops()
				// Here we make sure the instruction is a call (0xE8):
				*(unsigned char*)(addr + 0x00) = 0xE8;
				*(int*)(addr + 0x01) = (int)RenderBackdropsHook - (addr + 0x05);

				addr = 0x04F00CE; // The main render loop ("RelatedToPlayer") calls RenderBackdrops()
				// Here we make sure the instruction is a call (0xE8):
				*(unsigned char*)(addr + 0x00) = 0xE8;
				*(int*)(addr + 0x01) = (int)RenderBackdropsHook - (addr + 0x05);
			}

			if (g_config.Text2DRendererEnabled) 
			{
				// RenderCharHook
				*(unsigned char*)(0x00450A47 + 0x00) = 0xE8;
				*(int*)(0x00450A47 + 0x01) = (int)RenderCharHook - (0x00450A47 + 0x05);

				// ComputeMetricsHook
				*(unsigned char*)(0x00510385 + 0x00) = 0xE8;
				*(int*)(0x00510385 + 0x01) = (int)ComputeMetricsHook - (0x00510385 + 0x05);
			}
		
			if (g_config.Radar2DRendererEnabled) 
			{
				// DrawRadarHook
				*(int*)(0x00434977 + 0x06) = (int)DrawRadarHook;
				*(int*)(0x00434995 + 0x06) = (int)DrawRadarSelectedHook;

				// DrawBracketInFlightHook (this includes the sub-components rendered in OPT space)
				*(unsigned char*)(0x00503D46 + 0x00) = 0xE8;
				*(int*)(0x00503D46 + 0x01) = (int)DrawBracketInFlightHook - (0x00503D46 + 0x05);

				if (!g_bEnableVR)
				{
					// DrawBracketInFlightHook CMD
					*(unsigned char*)(0x00478E44 + 0x00) = 0xE8;
					*(int*)(0x00478E44 + 0x01) = (int)DrawBracketInFlightHook - (0x00478E44 + 0x05);
				}

				// DrawBracketMapHook
				*(unsigned char*)(0x00503CFE + 0x00) = 0xE8;
				*(int*)(0x00503CFE + 0x01) = (int)DrawBracketMapHook - (0x00503CFE + 0x05);
			}

			if (g_config.D3dRendererHookEnabled)
			{
				// D3dRenderLasersHook - call 0042BBA0
				*(unsigned char*)(0x004F0B7E + 0x00) = 0xE8;
				*(int*)(0x004F0B7E + 0x01) = (int)D3dRenderLasersHook - (0x004F0B7E + 0x05);

				// D3dRenderMiniatureHook - call 00478490
				*(unsigned char*)(0x00478412 + 0x00) = 0xE8;
				*(int*)(0x00478412 + 0x01) = (int)D3dRenderMiniatureHook - (0x00478412 + 0x05);
				*(unsigned char*)(0x00478483 + 0x00) = 0xE8;
				*(int*)(0x00478483 + 0x01) = (int)D3dRenderMiniatureHook - (0x00478483 + 0x05);

				// D3dRenderHyperspaceLinesHook - call 00480A80
				*(unsigned char*)(0x0047DCB6 + 0x00) = 0xE8;
				*(int*)(0x0047DCB6 + 0x01) = (int)D3dRenderHyperspaceLinesHook - (0x0047DCB6 + 0x05);

				// D3dRendererHook - call 00480370
				*(unsigned char*)(0x004829C5 + 0x00) = 0xE8;
				*(int*)(0x004829C5 + 0x01) = (int)D3dRendererMainHook - (0x004829C5 + 0x05);
				*(unsigned char*)(0x004829DF + 0x00) = 0xE8;
				*(int*)(0x004829DF + 0x01) = (int)D3dRendererMainHook - (0x004829DF + 0x05);

				// D3dRendererShadowHook - call 0044FD10
				*(unsigned char*)(0x004847DE + 0x00) = 0xE8;
				*(int*)(0x004847DE + 0x01) = (int)D3dRendererShadowHook - (0x004847DE + 0x05);
				*(unsigned char*)(0x004847F3 + 0x00) = 0xE8;
				*(int*)(0x004847F3 + 0x01) = (int)D3dRendererShadowHook - (0x004847F3 + 0x05);

				// D3dRendererOptLoadHook - call 0050E3B0
				*(int*)(0x004CC965 + 0x01) = (int)D3dRendererOptLoadHook - (0x004CC965 + 0x05);

				// D3dRendererOptNodeHook - call 00482000
				*(unsigned char*)(0x004815BF + 0x03) = 0x10; // esp+10
				*(int*)(0x004815CA + 0x01) = (int)D3dRendererOptNodeHook - (0x004815CA + 0x05);
				*(unsigned char*)(0x00481F9E + 0x00) = 0x57; // push edi
				*(int*)(0x00481FA5 + 0x01) = (int)D3dRendererOptNodeHook - (0x00481FA5 + 0x05);
				*(unsigned char*)(0x00481FC7 + 0x00) = 0x57; // push edi
				*(int*)(0x00481FC9 + 0x01) = (int)D3dRendererOptNodeHook - (0x00481FC9 + 0x05);
			}

			if (g_config.D3dRendererTexturesHookEnabled)
			{
				*(int*)(0x004CCA9F + 0x01) = (int)D3dReleaseD3DINFO - (0x004CCA9F + 0x05);
				*(int*)(0x004CD35C + 0x01) = (int)D3dReleaseD3DINFO - (0x004CD35C + 0x05);
				*(int*)(0x004CD147 + 0x01) = (int)D3dOptCreateD3DfromTexture - (0x004CD147 + 0x05);
				*(unsigned char*)(0x004CD146 + 0x00) = 0x56; // push esi

				*(unsigned char*)(0x004CD7DF + 0x00) = 0xE8; // call
				*(int*)(0x004CD7DF + 0x01) = (int)DatGlobalAllocHook - (0x004CD7DF + 0x05);
				*(unsigned char*)(0x004CD7DF + 0x05) = 0x90; // nop

				*(unsigned char*)(0x004CDD8C + 0x00) = 0xE8; // call
				*(int*)(0x004CDD8C + 0x01) = (int)DatGlobalReAllocHook - (0x004CDD8C + 0x05);
				*(unsigned char*)(0x004CDD8C + 0x05) = 0x90; // nop

				log_debug("[DBG] [PATCH] Applied D3dRendererTexturesHook");
			}

			// FlightTakeScreenshot
			*(unsigned char*)(0x004D4650 + 0x00) = 0xE8;
			*(int*)(0x004D4650 + 0x01) = (int)FlightTakeScreenshot - (0x004D4650 + 0x05);
			*(unsigned char*)(0x004D4650 + 0x05) = 0xC3;

			if (g_config.HDConcourseEnabled)
			{
				// ConcourseTakeScreenshot - call 0053FA70 - XwaTakeFrontScreenShot
				*(unsigned char*)(0x0053E479 + 0x00) = 0x90;
				*(int*)(0x0053E479 + 0x01) = 0x90909090;

				// Draw Cursor
				*(int*)(0x0053E94C + 0x01) = (int)DrawCursor - (0x0053E94C + 0x05);
				*(int*)(0x0053FF75 + 0x01) = (int)DrawCursor - (0x0053FF75 + 0x05);

				// Play Video Clear
				*(int*)(0x0055BE94 + 0x01) = (int)PlayVideoClear - (0x0055BE94 + 0x05);
			}

			// Remove the text next to the triangle pointer
			// At offset 072B4A, replace BF48BD6800 with 9090909090.
			// Offset 072B4A corresponds to address 0047374A
			if (!g_config.TriangleTextEnabled)
			{
				DWORD old, dummy;
				BOOL res;
				res = VirtualProtect((void *)0x047374A, 5, PAGE_READWRITE, &old);
				if (res) {
					log_debug("[DBG] Patching address 0x047374A");
					//memset((unsigned char *)0x047374A, 0x90, 5);
					unsigned char *ptr = (unsigned char *)0x047374A;
					for (int i = 0; i < 5; i++) {
						//log_debug("[DBG] 0x%x", *(ptr + i));
						*(ptr + i) = 0x90; // NOP
					}
					if (VirtualProtect((void *)0x047374A, 5, old, &dummy)) {
						log_debug("[DBG] Address 0x047374A patched!");
						if (FlushInstructionCache(GetModuleHandle(NULL), NULL, 0))
							log_debug("[DBG] Instructions flushed");
					}
				}
				else
					log_debug("[DBG] Could not get address 0x047374A");
			}

			// Remove the triangle pointer completely:
			// At offset 06B970 (address 0046C570), replace 83 with C3.
			if (!g_config.TrianglePointerEnabled) {
				DWORD old, dummy;
				BOOL res;
				res = VirtualProtect((void *)0x046C570, 1, PAGE_READWRITE, &old);
				if (res) {
					log_debug("[DBG] Patching address 0x046C570");
					//memset((unsigned char *)0x046C570, 0xC3, 1);
					unsigned char *ptr = (unsigned char *)0x046C570;
					for (int i = 0; i < 1; i++) {
						*(ptr + i) = 0xC3; // RET (?)
					}
					if (VirtualProtect((void *)0x046C570, 1, old, &dummy)) {
						log_debug("[DBG] Address 0x046C570 patched!");
						if (FlushInstructionCache(GetModuleHandle(NULL), NULL, 0))
							log_debug("[DBG] Instructions flushed");
					}
				}
				else
					log_debug("[DBG] Could not get address 0x046C570");
			}

			/*
				Remove the text next to the triangle pointer; but leave the triangle:

				// disable text
				At offset 06BE26 (address 0046CA26), replace E88585FCFF with 9090909090.
				At offset 06C092 (address 0046CC92), replace E81983FCFF with 9090909090.
				At offset 06C515 (address 0046D115), replace E8967EFCFF with 9090909090.

				// disable text digits
				At offset 06C0D4 (address 0046CCD4), replace E84785FCFF with 9090909090.
				At offset 06C10C (address 0046CD0C), replace E80F85FCFF with 9090909090.
				At offset 06C563 (address 0046D163), replace E8B880FCFF with 9090909090.
				At offset 06C59B (address 0046D19B), replace E88080FCFF with 9090909090.

				// disable dot
				At offset 06C0EB (address 0046CCEB), replace FF15B09D7C00 with 909090909090.
				At offset 06C57A (address 0046D17A), replace FF15B09D7C00 with 909090909090.
			*/
			if (g_config.SimplifiedTrianglePointer) {
				log_debug("[DBG] Applying SimplifiedTrianglePointer patch");
				// At offset 072B4A(address 0047374A), replace BF48BD6800 with 90 90 90 90 90. (5 bytes)
				//PatchWithValue(0x047374A, 0x90, 5); <-- This will remove the triangle; but also the FG name in the target box!

				// Disable Text
				// At offset 06BE26(address 0046CA26), replace E88585FCFF with 90 90 90 90 90. (5 bytes)
				PatchWithValue(0x046CA26, 0x90, 5);
				// At offset 06C092(address 0046CC92), replace E81983FCFF with 9090909090.
				PatchWithValue(0x046CC92, 0x90, 5);
				// At offset 06C515(address 0046D115), replace E8967EFCFF with 9090909090.
				PatchWithValue(0x046D115, 0x90, 5);

				// Disable text digits
				// At offset 06C0D4(address 0046CCD4), replace E84785FCFF with 90 90 90 90 90.
				PatchWithValue(0x046CCD4, 0x90, 5);
				// At offset 06C10C(address 0046CD0C), replace E80F85FCFF with 90 90 90 90 90.
				PatchWithValue(0x046CD0C, 0x90, 5);
				// At offset 06C563(address 0046D163), replace E8B880FCFF with 90 90 90 90 90.
				PatchWithValue(0x046D163, 0x90, 5);
				// At offset 06C59B(address 0046D19B), replace E88080FCFF with 90 90 90 90 90.
				PatchWithValue(0x046D19B, 0x90, 5);

				// disable dot
				// At offset 06C0EB(address 0046CCEB), replace FF15B09D7C00 with 90 90 90 90 90 90. (6 bytes)
				PatchWithValue(0x046CCEB, 0x90, 6);
				// At offset 06C57A(address 0046D17A), replace FF15B09D7C00 with 90 90 90 90 90 90.
				PatchWithValue(0x046D17A, 0x90, 6);
				// Flush the instruction cache
				if (FlushInstructionCache(GetModuleHandle(NULL), NULL, 0))
					log_debug("[DBG] Instructions flushed");
			}

			// Patch POV offsets for each flyable ship
			{
				//short *POV_Y0 = (short *)(0x5BB480 + 0x238); // = 0x5BB6B8, 0x5BB480 + 0x32 = Craft name
				//short *POV_Z0 = (short *)(0x5BB480 + 0x23A);
				//short *POV_X0 = (short *)(0x5BB480 + 0x23C);
				//log_debug("[DBG] [POV] X,Z,Y: %d, %d, %d", *POV_X0, *POV_Z0, *POV_Y0);
				//*POV_Z0 += 10; // Moves the POV up
				//*POV_Y0 += 5;  // Moves the POV forward
				//*POV_Y0 += 10; // This makes the X-Wing look good in VR
				LoadPOVOffsets();

				// DEBUG
				/*
				const char *CraftTableBase = (char *)0x5BB480;
				const int16_t EntrySize = 0x3DB, POVOffset = 0x238;
				for (int i = 1; i <= 3; i++) {
					int16_t *pov = (int16_t *)(CraftTableBase + (i - 1) * EntrySize + POVOffset);
					log_debug("[DBG] [POV] (%d): %d, %d, %d",
						i, *pov, *(pov+1), *(pov+2));
				}
				*/
			}

			// Enable recording in Melee missions (PPG/Yard)
			{
				uint32_t XWABase = 0x400C00;
				// At offset 100AF8 (address 400C00 + 100AF8), replace 0F 85 86 01 00 00 with 9090 9090 9090 (6 bytes)
				PatchWithValue(XWABase + 0x100AF8, 0x90, 6);
				log_debug("[DBG] [PATCH] Enabled Melee Recordings");
			}

			// DATReader debug block
			/*
			if (false)
			{
				HMODULE hDATReader = LoadLibrary("DATReader.dll");
				LoadDATFileFun LoadDATFile = (LoadDATFileFun)GetProcAddress(hDATReader, "LoadDATFile");
				GetDATImageMetadataFun GetDATImageMetadata = (GetDATImageMetadataFun)GetProcAddress(hDATReader, "GetDATImageMetadata");
				ReadDATImageDataFun ReadDATImageData = (ReadDATImageDataFun)GetProcAddress(hDATReader, "ReadDATImageData");
				short Width = 0, Height = 0;
				uint8_t Format = 0;
				uint8_t *buf = NULL;
				int buf_len = 0;

				if (LoadDATFile != NULL) {
					if (LoadDATFile("Resdata\\HUD.dat"))
						log_debug("[DBG] [C++] Loaded DAT file");
					else
						log_debug("[DBG] [C++] Could not load DAT file");
				}

				if (GetDATImageMetadata != NULL) {
					if (GetDATImageMetadata(10000, 300, &Width, &Height, &Format))
						log_debug("[DBG] [C++] Image found: W,H: (%d, %d), Format: %d", Width, Height, Format);
					else
						log_debug("[DBG] [C++] Image not found");
				}

				if (ReadDATImageData != NULL) {
					buf_len = Width * Height * 4;
					buf = new uint8_t[buf_len];
					if (ReadDATImageData(buf, buf_len))
						log_debug("[DBG] [C++] Read Image Data");
					else
						log_debug("[DBG] [C++] Failed to read image data");

					delete[] buf;
				}
				FreeLibrary(hDATReader);
			}
			*/

			// ZIPReader: Load and erase any dangling temporary directories
			{
				if (InitZIPReader())
					//DeleteAllTempZIPDirectories();
					break;
			}
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		// Release Embree objects
		if (g_bRTEnableEmbree)
		{
			UnloadEmbree();
		}

		CloseDATReader(); // Idempotent: does nothing if the DATReader wasn't loaded.
		CloseZIPReader(); // Idempotent: does nothing if the ZIPReader wasn't loaded.

#ifdef _DEBUG
		IDXGIDebug1* dxgiDebug;
		HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
		if (SUCCEEDED(hr))
		{
			log_debug("[DBG] Reporting Live Objects...");
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
		else
		{
			log_debug("[DBG] Could NOT get DXGI DEBUG object");
		}
#endif

		if (g_bUseSteamVR)
			ShutDownSteamVR();
		else if (g_bEnableVR)
			ShutDownDirectSBS();
		// Generic FreePIE shutdown, just in case...
		if (bFreePIEAlreadyInitialized)
			ShutdownFreePIE();
		break;
	}

	return TRUE;
}
