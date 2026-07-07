// Copyright (c) 2016-2018 Reimar Döffinger
// Licensed under the MIT license. See LICENSE.txt

#include "common.h"
#include "config.h"

#include <mmsystem.h>
#include <xinput.h>

#include "XWAObject.h"
#include "SharedMem.h"
#include "joystick.h"
#include "HiResTimer.h"
#include "SteamVR.h"
#include "ActiveCockpit.h"

extern PlayerDataEntry *PlayerDataTable;
extern uint32_t *g_playerIndex;

// Gimbal Lock Fix
bool g_bEnableGimbalLockFix = false, g_bGimbalLockFixActive = false;
bool g_bEnableRudder = true, g_bYTSeriesShip = false;

// How much roll is applied when the ship is doing yaw. Set this to 0
// to have a completely flat yaw, as in the YT-series ships.
float g_fRollFromYawScale = -0.8f;

// Acceleration and deceleration rates in degrees per second (this is the 2nd derivative)
float g_fYawAccelRate_s   = 5.0f;
float g_fPitchAccelRate_s = 5.0f;
float g_fRollAccelRate_s  = 5.0f;

// The maximum turn rate (in degrees per second) for each axis (1st derivative)
float g_fMaxYawRate_s   = 70.0f;
float g_fMaxPitchRate_s = 70.0f;
float g_fMaxRollRate_s  = 100.0f;

float g_fTurnRateScaleThr_0   = 0.3f;
float g_fTurnRateScaleThr_100 = 0.6f;
float g_fMaxTurnAccelRate_s   = 3.0f;
bool g_bThrottleModulationEnabled = true;

float g_fMouseRangeX = 256.0f, g_fMouseRangeY = 256.0f;

float g_fMouseDeltaX = 0.0f, g_fMouseDeltaY = 0.0f;
float g_fMouseDecelRate_s = 700.0f;

constexpr int MAX_RAW_INPUT_ENTRIES = 128;
RAWINPUT rawInputBuffer[MAX_RAW_INPUT_ENTRIES];

extern bool g_bRendering3D;
extern D3D11_VIEWPORT g_concourseViewport;
extern int g_WindowWidth, g_WindowHeight;
extern float g_fCurInGame2DWidth, g_fCurInGame2DHeight;

float GetCurrentPlayerThrottle();

#pragma comment(lib, "winmm")
#pragma comment(lib, "XInput9_1_0")

#undef min
#undef max
#include <algorithm>

inline float sign(float val)
{
	return (val >= 0.0f) ? 1.0f : -1.0f;
}

float SignedReduceClamp(float val, float delta, float abs_min, float abs_max)
{
	float s = sign(val);
	val = fabs(val);
	val -= delta;
	if (val < abs_min) val = abs_min;
	if (val > abs_max) val = abs_max;
	return s * val;
}

void SendMouseEvent(float dx, float dy, bool bLeft, bool bRight)
{
	constexpr int MAX_ACTION_LEN = 12;
	INPUT input[MAX_ACTION_LEN];

	int i = 0;
	input[i].ki.wScan = 0;
	input[i].type = INPUT_MOUSE;
	input[i].mi.dx = (LONG)dx;
	input[i].mi.dy = (LONG)dy;
	input[i].mi.dwFlags = MOUSEEVENTF_MOVE | (bLeft ? MOUSEEVENTF_LEFTDOWN : 0) | (bLeft ? MOUSEEVENTF_RIGHTDOWN : 0);
	input[i].mi.mouseData = 0;
	input[i].mi.dwExtraInfo = NULL;
	input[i].mi.time = 0;
	i++;

	input[i].ki.wScan = 0;
	input[i].type = INPUT_MOUSE;
	input[i].mi.dx = 0;
	input[i].mi.dy = 0;
	input[i].mi.dwFlags = (bLeft ? MOUSEEVENTF_LEFTUP : 0) | (bLeft ? MOUSEEVENTF_RIGHTUP : 0);
	input[i].mi.mouseData = 0;
	input[i].mi.dwExtraInfo = NULL;
	input[i].mi.time = 0;
	i++;

	SendInput(i, input, sizeof(INPUT));
}

void VREventPump(WORD* escScanCodes, WORD *periodScanCodes)
{
	const int ptrIdx = 0;
	const uvfloat4 coords = { -1, -1, -1, -1 };

	vr::VREvent_t vrEvent;
	while (vr::VROverlay()->PollNextOverlayEvent(g_VR2Doverlay, &vrEvent, sizeof(vrEvent)))
	{
		switch (vrEvent.eventType)
		{
			case vr::VREvent_MouseMove:
			{
				float left   = g_concourseViewport.TopLeftX;
				float top    = g_concourseViewport.TopLeftY;
				float width  = g_concourseViewport.Width;
				float height = g_concourseViewport.Height;

				// The mouse coords go from (0,0) = (left, bottom) to (1,1) = (right, top).
				// In pixels, the range of the mouse is (0,0)-(g_steamVRWidth, g_steamVRHeight)
				// because that's the size of the overlay texture. We need to invert the Y axis
				// because the regular mouse coords have (0,0) at the top-left corner of the
				// screen.
				float x = g_steamVRWidth * vrEvent.data.mouse.x;
				float y = g_steamVRHeight * (1.0f - vrEvent.data.mouse.y);

				float scaleX = g_fCurInGame2DWidth / width;
				// TODO: I don't understand why, but the VR pointer and the cursor are not
				// perfectly aligned. This is more evident near the bottom of the screen.
				// To try and help the vertical alignment, I'm doing "height + 20" here instead
				// of the more logical "height". I wish I knew why this is happening, but for
				// now, it's time to move on.
				float scaleY = g_fCurInGame2DHeight / (height + 20);
				x = (x - left) * scaleX;
				y = (y - top) * scaleY;

				SetCursorPos((int)x, (int)y);
				break;
			}
			case vr::VREvent_MouseButtonDown:
				SendMouseEvent(0, 0, true, false);
				break;
			case vr::VREvent_ButtonPress:
				if ((vrEvent.data.controller.button & 0x01) != 0)
					ACRunAction(periodScanCodes, coords, -1, ptrIdx, nullptr);
				else if ((vrEvent.data.controller.button & 0x02) != 0)
					ACRunAction(escScanCodes, coords, -1, ptrIdx, nullptr);
				break;
		}
	}
}

void EmulMouseWithVRControllers()
{
	const int ptrIdx = 0;
	const int auxIdx = (ptrIdx == 0) ? 1 : 0;
	const uvfloat4 coords = { -1, -1, -1, -1 };
	static WORD escScanCodes[2] = { 0 };
	static WORD periodScanCodes[2] = { 0 };
	if (escScanCodes[0] == 0)
	{
		char action1[] = "ESC";
		TranslateACAction(-1, escScanCodes, action1, nullptr);

		char action2[] = "PERIOD";
		TranslateACAction(-1, periodScanCodes, action2, nullptr);
	}

	const float dx =  g_contStates[ptrIdx].trackPadX * g_ACPointerData.mouseSpeedX;
	const float dy = -g_contStates[ptrIdx].trackPadY * g_ACPointerData.mouseSpeedY;
	const bool  bLeftClick = (!g_prevContStates[ptrIdx].buttons[VRButtons::TRIGGER] && g_contStates[ptrIdx].buttons[VRButtons::TRIGGER]);

	SendMouseEvent(dx, dy, bLeftClick, false);

	// Send the ESC command
	if (!g_prevContStates[ptrIdx].buttons[VRButtons::PAD_CLICK] && g_contStates[ptrIdx].buttons[VRButtons::PAD_CLICK])
		ACRunAction(escScanCodes, coords, -1, ptrIdx, nullptr);

	// Send the PERIOD command
	if (!g_prevContStates[auxIdx].buttons[VRButtons::PAD_CLICK] && g_contStates[auxIdx].buttons[VRButtons::PAD_CLICK])
		ACRunAction(periodScanCodes, coords, -1, ptrIdx, nullptr);

	if (g_bEnableVRPointerInConcourse)
		VREventPump(escScanCodes, periodScanCodes);

	// Actions have been processed, we can update the previous state now
	for (int i = 0; i < 2; i++)
		g_prevContStates[i] = g_contStates[i];
}

// timeGetTime emulation.
// if it is called in a tight loop it will call Sleep()
// prevents high CPU usage due to busy loop
DWORD emulGetTime()
{
	static DWORD oldtime;
	static DWORD count;
	DWORD time = timeGetTime();
	if (time != oldtime)
	{
		oldtime = time;
		count = 0;
	}
	// Trigger value and sleep value derived by trial and error.
	// Trigger values down to 10 with sleep value 8 do not seem to affect performance
	// even at 60 FPS, and trigger values up to 1000 with sleep value 1 still seem effective
	// to reduce CPU load on fast modern computers.
	if (++count >= 20)
	{
		Sleep(2);
		time = timeGetTime();
		count = 0;
	}
	return time;
}

static int needsJoyEmul()
{
	JOYCAPS caps = {};
	if (joyGetDevCaps(0, &caps, sizeof(caps)) != JOYERR_NOERROR ||
	    !(caps.wCaps & JOYCAPS_HASZ) || caps.wNumAxes <= 2 ||
	    // Assume for now that wMid == 0x45e means "Gamepad"
	    // Steam controller: pid == 0x28e
	    // Xbox controller: pid == 0x2ff
	    caps.wMid == 0x45e)
	{
		// Probably the joystick is just an emulation from a gamepad.
		// Rather try to use it as gamepad directly then.
		XINPUT_STATE state;
		if (XInputGetState(0, &state) == ERROR_SUCCESS)
		{
			log_debug("[DBG] [JOY] Gamepad found.");
			return 2;
		}
	}

	UINT cnt = joyGetNumDevs();
	for (unsigned i = 0; i < cnt; ++i)
	{
		/*JOYINFOEX jie;
		memset(&jie, 0, sizeof(jie));
		jie.dwSize = sizeof(jie);
		jie.dwFlags = JOY_RETURNALL;
		UINT res = joyGetPosEx(0, &jie);*/

		JOYCAPS caps;
		UINT res = joyGetDevCaps(i, &caps, sizeof(JOYCAPS));
		if (res == JOYERR_NOERROR)
		{
			log_debug("[DBG] [JOY] Found joystick in slot: %d", i);
			return 0;
		}
	}

	log_debug("[DBG] [JOY] Couldn't find any joysticks. Enabling keyb & mouse emulation.");
	return 1;
}

UINT WINAPI emulJoyGetNumDevs(void)
{
	if (g_config.JoystickEmul < 0) {
		log_debug("[DBG] [JOY] JoystickEmul is -1, looking for devices...");
		g_config.JoystickEmul = needsJoyEmul();
		log_debug("[DBG] [JOY] g_config.JoystickEmul got: %d", g_config.JoystickEmul);
	}
	if (!g_config.JoystickEmul) {
		return joyGetNumDevs();
	}
	return 1;
}

static UINT joyYmax, joyZmax, joyZmin;

UINT WINAPI emulJoyGetDevCaps(UINT_PTR joy, struct tagJOYCAPSA *pjc, UINT size)
{
	if (!g_config.JoystickEmul) {
		UINT res = joyGetDevCaps(joy, pjc, size);
		if (g_config.InvertYAxis && joy == 0 && pjc && size == 0x194) joyYmax = pjc->wYmax;
		if (joy == 0 && pjc && size == 0x194) {
			joyZmax = pjc->wZmax;
			joyZmin = pjc->wZmin;
			if (joyZmin > joyZmax) {
				UINT temp = joyZmin;
				joyZmin = joyZmax;
				joyZmax = temp;
			}
		}
		return res;
	}
	if (joy != 0) return MMSYSERR_NODRIVER;
	if (size != 0x194) return MMSYSERR_INVALPARAM;
	memset(pjc, 0, size);
	if (g_config.JoystickEmul == 2) {
		pjc->wXmax = 65536;
		pjc->wYmax = 65535;
		pjc->wZmax = 255;
		pjc->wRmax = 65536;
		pjc->wUmax = 65536;
		pjc->wVmax = 255;
		pjc->wNumButtons = 14;
		pjc->wMaxButtons = 14;
		pjc->wNumAxes = 6;
		pjc->wMaxAxes = 6;
		pjc->wCaps = JOYCAPS_HASZ | JOYCAPS_HASR | JOYCAPS_HASU | JOYCAPS_HASV | JOYCAPS_HASPOV | JOYCAPS_POV4DIR;
		return JOYERR_NOERROR;
	}
	pjc->wXmax = 512;
	pjc->wYmax = 512;
	// VR controllers can be used to emulate the throttle, so the emulated range must match
	// the range used in a real joystick. That way we can have both a real joystick with a
	// throttle and an emulated throttle from a VR controller.
	pjc->wZmax = 65536;
	pjc->wRmax = 512;
	pjc->wNumButtons = 5;
	pjc->wMaxButtons = 5;
	// wNumAxes should probably stay at 2 here because needsJoyEmul() compares against
	// 2 num axes to decide whether or not to use XInput.
	pjc->wNumAxes = 6;
	pjc->wMaxAxes = 6;
	pjc->wCaps = JOYCAPS_HASZ | JOYCAPS_HASR;
	return JOYERR_NOERROR;
}

static DWORD lastGetPos;

static const DWORD povmap[16] = {
	JOY_POVCENTERED, // nothing
	JOY_POVFORWARD, // DPAD_UP
	JOY_POVBACKWARD, // DPAD_DOWN
	JOY_POVCENTERED, // up and down
	JOY_POVLEFT, // DPAD_LEFT
	(270 + 45) * 100, // left and up
	(180 + 45) * 100, // down and left
	JOY_POVLEFT, // left and up and down
	JOY_POVRIGHT, // DPAD_RIGHT
	45 * 100, // up and right
	(90 + 45) * 100, // right and down
	JOY_POVRIGHT, // right and up and down

	// As from start, but  with both right and left pressed
	JOY_POVCENTERED, // nothing
	JOY_POVFORWARD, // DPAD_UP
	JOY_POVBACKWARD, // DPAD_DOWN
	JOY_POVCENTERED, // up and down
};

// See:
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getrawinputbuffer?redirectedfrom=MSDN
// https://learn.microsoft.com/en-us/windows/win32/inputdev/using-raw-input
// https://stackoverflow.com/questions/28879021/winapi-getrawinputbuffer
// https://stackoverflow.com/questions/59809804/rawinput-winapi-getrawinputbuffer-and-message-handling
// Returns 0 if no events were read
// DISABLED: I suspect that registering for low-level events messes the main message pump
#ifdef DISABLED
int ReadRawMouseData(int *iDeltaX, int *iDeltaY)
{
	uint32_t rawInputBufferSize = sizeof(RAWINPUT) * MAX_RAW_INPUT_ENTRIES;

	*iDeltaX = 0; *iDeltaY = 0;
	//bool nonzero = false;
	// MAGIC NUMBER WARNING:
	// For some reason, the actual data is shifted by 8 bytes.
	// This 8 is probably the diff between sizeof(RAWINPUT) (40) and header.dwSize (48)
	RAWINPUT* raw = (RAWINPUT*)((uint32_t)rawInputBuffer + 8);

	int res = GetRawInputBuffer(rawInputBuffer, &rawInputBufferSize, sizeof(RAWINPUTHEADER));
	int count = res;
	/*if (count) {
		log_debug("[DBG] count: %d, rawInputBufferSize: %d", count, rawInputBufferSize);
		nonzero = true;
	}*/

	while (count > 0) {
		// The trackpad becomes RIM_TYPEMOUSE, but additional mice have a different type
		// (probably RIM_TYPEHID). Since we're only registering mice, we can ignore the
		// type and move on...
		//if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			/*log_debug("[DBG] RMD.MOUSE event, %d, %d; %d, %d, %d",
				raw->data.mouse.lLastX, raw->data.mouse.lLastY,
				raw->data.mouse.ulButtons, raw->data.mouse.usButtonFlags,
				raw->data.mouse.ulExtraInformation);*/
			*iDeltaX += raw->data.mouse.lLastX;
			*iDeltaY += raw->data.mouse.lLastY;
		}

		raw = NEXTRAWINPUTBLOCK(raw);
		count--;
	}

	/*if (nonzero) {
		log_debug("[DBG] Delta: %d, %d", *iDeltaX, *iDeltaY);
		log_debug("[DBG] ---------------------------------");
	}*/
	return res;
}

void ResetRawMouseInput()
{
	int deltaX, deltaY;
	ReadRawMouseData(&deltaX, &deltaY);
	g_fMouseDeltaX = g_fMouseDeltaY = 0.0f;
}
#endif

/// <summary>
/// Computes the yaw, pitch, roll for the emulated joystick and returns normalized values
/// </summary>
void EmulYawPitchRollFromVR(const bool bResetCenter, float *yaw, float *pitch, float *roll)
{
	// yaw: left: 40, right: -40
	// pitch: forward: -40, backward: 40
	// roll : left : 40, right : -40
	const int joyIdx = g_ACJoyEmul.joyHandIdx;
	Matrix4 pose = g_contStates[joyIdx].pose;
	static Vector4 cUpYZ, cUpXY;
	if (bResetCenter)
	{
		// Capture the current forward vector (g_controllerForwardVector is already normalized)
		g_contStates[joyIdx].centerFwd = pose * g_controllerForwardVector;
		// Project F to the X-Z plane:
		g_contStates[joyIdx].centerFwd.y = 0;
		g_contStates[joyIdx].centerFwd.w = 0;
		g_contStates[joyIdx].centerFwd.normalize();

		// Capture the current up vector (g_controllerUpVector is already normalized)
		g_contStates[joyIdx].centerUp = pose * g_controllerUpVector;

		// Project U to the Y-Z plane:
		cUpYZ = g_contStates[joyIdx].centerUp;
		cUpYZ.x = cUpYZ.w = 0;
		cUpYZ.normalize();

		// Project U to the X-Y plane:
		cUpXY = g_contStates[joyIdx].centerUp;
		cUpXY.z = cUpXY.w = 0;
		cUpXY.normalize();
	}

	// g_controllerUpVector is already normalized
	Vector4 U = pose * g_controllerUpVector;

	// Project U to the Y-Z plane and find the angle wrt centerUp to find the pitch
	{
		// To compute the pitch, we're going to project the controller's Up dir to the Y-Z plane.
		// Then, in this Y-Z plane we'll compute the "x" and "y" components of the controller's
		// Up to see how far it is from the centerUp reference captured above. Once the "x" and
		// "y" components have been captured, we use atan2 to compute the final angle.
		Vector4 Uyz;
		Uyz.y = U.y; Uyz.z = U.z;
		Uyz.x = Uyz.w = 0;
		Uyz.normalize();

		// cUpPYZ is perpendicular to cUpYZ
		const Vector3 cUpPYZ = Vector3(0, -cUpYZ.z, cUpYZ.y);
		const float x = Uyz.dot(cUpYZ);
		const float y = Uyz.dot(cUpPYZ);
		*pitch = RAD2DEG * atan2(y, x);
	}

	// Project U to the X-Y plane and find the angle wrt centerUp to find the yaw
	{
		Vector4 Uxy;
		Uxy.x = U.x; Uxy.y = U.y;
		Uxy.z = Uxy.w = 0;
		Uxy.normalize();

		// cUpPXY is perpendicualr to cUpXY
		const Vector3 cUpPXY = Vector3(cUpXY.y, -cUpXY.x, 0);
		const float x = Uxy.dot(cUpXY);
		const float y = Uxy.dot(cUpPXY);
		*yaw = RAD2DEG * atan2(y, x);
	}

	// Project F to the X-Z plane and find the angle wrt centerFwd to find the roll
	{
		const Vector4 cFwd = g_contStates[joyIdx].centerFwd;
		// g_controllerForwardVector is already normalized
		Vector4 Fwd = pose * g_controllerForwardVector;
		// Project to the X-Z plane and find the angle wrt centerFwd
		Fwd.y = Fwd.w = 0;
		Fwd.normalize();

		// cFwdP is perpendicular to cFwd
		Vector3 cFwdP = Vector3(-cFwd.z, 0, cFwd.x);
		const float x = Fwd.dot(cFwd);
		const float y = Fwd.dot(cFwdP);
		*roll = RAD2DEG * atan2(y, x);
	}

	*yaw   = clamp(*yaw,   -g_ACJoyEmul.yawHalfRange,   g_ACJoyEmul.yawHalfRange)   / g_ACJoyEmul.yawHalfRange;
	*pitch = clamp(*pitch, -g_ACJoyEmul.pitchHalfRange, g_ACJoyEmul.pitchHalfRange) / g_ACJoyEmul.pitchHalfRange;
	*roll  = clamp(*roll,  -g_ACJoyEmul.rollHalfRange,  g_ACJoyEmul.rollHalfRange)  / g_ACJoyEmul.rollHalfRange;
}

void ProcessVRControllerButtons()
{
	const uvfloat4 coords = { -1, -1, -1, -1 };
	// Synthesize joystick button clicks and run AC actions associated with VR buttons
	for (int contIdx = 0; contIdx < 2; contIdx++)
	{
		for (int buttonIdx = 0; buttonIdx < VRButtons::MAX; buttonIdx++)
		{
			if (!g_prevContStates[contIdx].buttons[buttonIdx] && g_contStates[contIdx].buttons[buttonIdx])
				ACRunAction(g_ACJoyMappings[contIdx].action[buttonIdx], coords, -1, contIdx, nullptr);
		}

		// Actions have been processed, we can update the previous state now
		g_prevContStates[contIdx] = g_contStates[contIdx];
	}
}

UINT WINAPI emulJoyGetPosEx(UINT joy, struct joyinfoex_tag *pji)
{
	const bool bJoystickEmulationEnabled = (g_config.JoystickEmul != 0);

	// Tell the joystick hook when to disable the joystick
	if (g_pSharedDataJoystick != NULL) {
		g_pSharedDataJoystick->GimbalLockFixActive = g_bGimbalLockFixActive;
		g_pSharedDataJoystick->JoystickEmulationEnabled = bJoystickEmulationEnabled;
	}

	if (!bJoystickEmulationEnabled) {
		UINT res = joyGetPosEx(joy, pji);
		if (g_config.InvertYAxis && joyYmax > 0) pji->dwYpos = joyYmax - pji->dwYpos;

		// The X-Z joystick axes are now swapped below, at the end of this function

		if (g_config.InvertThrottle) pji->dwZpos = (joyZmax - pji->dwZpos) + joyZmin;
		// dwXpos: x-axis goes from 0 (left) to 32767 (center) to 65535 (right)
		// dwYpos: y-axis goes from 0 (up)   to 32767 (center) to 65535 (down)
		// dwRpos: z-axis goes from 0 (left) to 32767 (center) to 65535 (right)
		if (g_bGimbalLockFixActive && g_pSharedDataJoystick != NULL)
		{
			// Only run this section if the joystick hook isn't present
			if (!(g_pSharedDataJoystick->JoystickHookPresent)) {
				float normYaw   = 2.0f * (pji->dwXpos / 65535.0f - 0.5f);
				float normPitch = 2.0f * (pji->dwYpos / 65535.0f - 0.5f);
				float normRoll  = 2.0f * (pji->dwRpos / 65535.0f - 0.5f);
				g_pSharedDataJoystick->JoystickYaw   = normYaw;
				g_pSharedDataJoystick->JoystickPitch = normPitch;
				g_pSharedDataJoystick->JoystickRoll  = normRoll;

				// Nullify the joystick input
				pji->dwXpos = 32767;
				pji->dwYpos = 32767;
				pji->dwRpos = 32767;
			}
		}

		// There's code below to emulate the throttle when AC is on. We need to let this block fall through to reach that part
		//return res;
	}

	//if (joy != 0) return MMSYSERR_NODRIVER;
	//if (pji->dwSize != 0x34) return MMSYSERR_INVALPARAM;

	// XInput
	if (g_config.JoystickEmul == 2) {
		XINPUT_STATE state;
		XInputGetState(0, &state);
		pji->dwFlags = JOY_RETURNALL;
		pji->dwXpos = state.Gamepad.sThumbLX + 32768;
		pji->dwYpos = state.Gamepad.sThumbLY + 32768;
		if (!g_config.InvertYAxis) pji->dwYpos = 65536 - pji->dwYpos;
		// The 65536 value breaks XWA with in-game invert Y axis option
		pji->dwYpos = std::min(pji->dwYpos, DWORD(65535));
		if (g_config.XInputTriggerAsThrottle)
		{
			pji->dwZpos = g_config.XInputTriggerAsThrottle & 1 ? state.Gamepad.bLeftTrigger : state.Gamepad.bRightTrigger;
		}
		pji->dwRpos = state.Gamepad.sThumbRX + 32768;
		pji->dwUpos = state.Gamepad.sThumbRY + 32768;
		pji->dwVpos = state.Gamepad.bLeftTrigger;
		pji->dwButtons = 0;
		// Order matches XBox One controller joystick emulation default
		// A, B, X, Y first
		pji->dwButtons |= (state.Gamepad.wButtons & 0xf000) >> 12;
		// Shoulder buttons next
		pji->dwButtons |= (state.Gamepad.wButtons & 0x300) >> 4;
		// start and back, for some reason in flipped order compared to XINPUT
		pji->dwButtons |= (state.Gamepad.wButtons & 0x10) << 3;
		pji->dwButtons |= (state.Gamepad.wButtons & 0x20) << 1;
		// Thumb buttons
		pji->dwButtons |= (state.Gamepad.wButtons & 0xc0) << 2;
		// Triggers last, they are not mapped in the joystick emulation
		if (g_config.XInputTriggerAsThrottle != 1 &&
			state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) pji->dwButtons |= 0x400;
		if (g_config.XInputTriggerAsThrottle != 2 &&
			state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) pji->dwButtons |= 0x800;
		// These are not defined by XINPUT, and can't be remapped by the
		// XWA user interface as they will be buttons above 12, but map them just in case
		pji->dwButtons |= (state.Gamepad.wButtons & 0xc00) << 2;

		// XWA can map only 12 buttons, so map dpad to POV
		pji->dwPOV = povmap[state.Gamepad.wButtons & 15];

		pji->dwButtonNumber = 0;
		for (int i = 0; i < 32; i++)
		{
			if ((pji->dwButtons >> i) & 1) ++pji->dwButtonNumber;
		}
		return JOYERR_NOERROR;
	}

	DWORD now = GetTickCount();
	// Assume we started a new game
	if ((now - lastGetPos) > 5000)
	{
		SetCursorPos(240, 240);
		GetAsyncKeyState(VK_LBUTTON);
		GetAsyncKeyState(VK_RBUTTON);
		GetAsyncKeyState(VK_MBUTTON);
		GetAsyncKeyState(VK_XBUTTON1);
		GetAsyncKeyState(VK_XBUTTON2);
	}
	lastGetPos = now;
	POINT pos;
	GetCursorPos(&pos);

	// Reset pji, but only if emulation is enabled.
	if (bJoystickEmulationEnabled) {
		pji->dwXpos = 256; // This is the center position for this axis
		pji->dwYpos = 256;
		pji->dwZpos = 32768; // Throttle
		pji->dwRpos = 256; // Rudder (roll)
	}

	// Mouse input
	if (g_config.JoystickEmul == 1)
	{
		//if (!g_bGimbalLockFixActive)
		{
			// This is the original code by Reimar
			pji->dwXpos = static_cast<DWORD>(std::min(256.0f + (pos.x - 240.0f) * g_config.MouseSensitivity, 512.0f));
			pji->dwYpos = static_cast<DWORD>(std::min(256.0f + (pos.y - 240.0f) * g_config.MouseSensitivity, 512.0f));
		}
		// DISABLED: I suspect that registering for low-level events messes the main message pump
#ifdef DISABLED
		else // if (g_bRendering3D)
		{
			static float lastTime = g_HiResTimer.global_time_s;
			const float now = g_HiResTimer.global_time_s;
			const float elapsedTime = now - lastTime;

			int iDeltaX = 0, iDeltaY = 0;
			if (ReadRawMouseData(&iDeltaX, &iDeltaY))
			{
				g_fMouseDeltaX = clamp(g_fMouseDeltaX + (float)iDeltaX * g_config.MouseSensitivity,
					-g_fMouseRangeX, g_fMouseRangeX);
				g_fMouseDeltaY = clamp(g_fMouseDeltaY + (float)iDeltaY * g_config.MouseSensitivity,
					-g_fMouseRangeY, g_fMouseRangeY);
			}
			else
			{
				//g_fMouseDeltaX *= 0.5f;
				//g_fMouseDeltaY *= 0.5f;

				g_fMouseDeltaX = SignedReduceClamp(g_fMouseDeltaX, elapsedTime * g_fMouseDecelRate_s, 0.0f, g_fMouseRangeX);
				g_fMouseDeltaY = SignedReduceClamp(g_fMouseDeltaY, elapsedTime * g_fMouseDecelRate_s, 0.0f, g_fMouseRangeY);
			}
			const float deltaX = clamp(g_fMouseDeltaX / g_fMouseRangeX, -1.0f, 1.0f);
			const float deltaY = clamp(g_fMouseDeltaY / g_fMouseRangeY, -1.0f, 1.0f);

			pji->dwXpos = static_cast<DWORD>((deltaX + 1.0f) / 2.0f * 512.0f);
			pji->dwYpos = static_cast<DWORD>((deltaY + 1.0f) / 2.0f * 512.0f);

			lastTime = now;
		}
#endif

		pji->dwButtons = 0;
		pji->dwButtonNumber = 0;
		if (GetAsyncKeyState(VK_LBUTTON)) {
			pji->dwButtons |= 1;
			++pji->dwButtonNumber;
		}
		if (GetAsyncKeyState(VK_RBUTTON)) {
			pji->dwButtons |= 2;
			++pji->dwButtonNumber;
		}
		if (GetAsyncKeyState(VK_MBUTTON)) {
			pji->dwButtons |= 4;
			++pji->dwButtonNumber;
		}
		if (GetAsyncKeyState(VK_XBUTTON1)) {
			pji->dwButtons |= 8;
			++pji->dwButtonNumber;
		}
		if (GetAsyncKeyState(VK_XBUTTON2)) {
			pji->dwButtons |= 16;
			++pji->dwButtonNumber;
		}
	}

	float normYaw = 0, normPitch = 0, normRoll = 0;
	if (g_bUseSteamVR)
	{
		// Support VR controllers (quick-and-dirty approach, the proper way is probably
		// to add code to the joystick hook so that bindings can be configured).
		const int joyIdx = g_ACJoyEmul.joyHandIdx;
		const int thrIdx = g_ACJoyEmul.thrHandIdx;

		if (g_config.JoystickEmul == 3)
		{
			pji->dwButtons = 0;
			pji->dwButtonNumber = 0;

			static Vector4 rightAnchor;
			const bool bResetAnchor = !(g_prevContStates[joyIdx].buttons[VRButtons::GRIP]) &&
										g_contStates[joyIdx].buttons[VRButtons::GRIP];
			if (bResetAnchor)
			{
				rightAnchor = g_contStates[joyIdx].pose * Vector4(0, 0, 0, 1);;
			}

			if (g_contStates[joyIdx].buttons[VRButtons::GRIP])
			{
				Vector4 current = g_contStates[joyIdx].pose * Vector4(0, 0, 0, 1);
				float yaw   = clamp(current.x - rightAnchor.x, -g_ACJoyEmul.joyHalfRangeX, g_ACJoyEmul.joyHalfRangeX) / g_ACJoyEmul.joyHalfRangeX;
				float pitch = clamp(current.z - rightAnchor.z, -g_ACJoyEmul.joyHalfRangeZ, g_ACJoyEmul.joyHalfRangeZ) / g_ACJoyEmul.joyHalfRangeZ;
				float roll  = 0;
				float dummy;

				if (g_ACJoyEmul.yprEnabled)
					EmulYawPitchRollFromVR(bResetAnchor, &yaw, &pitch, &roll);
				else
					EmulYawPitchRollFromVR(bResetAnchor, &dummy, &dummy, &roll);

				// Apply deadzone
				const float s_yaw   = sign(yaw);
				const float s_pitch = sign(pitch);
				const float s_roll  = sign(roll);

				yaw   = fabs(yaw);
				pitch = fabs(pitch);
				roll  = fabs(roll);

				if (yaw < g_ACJoyEmul.deadZonePerc)
					yaw = 0.0f;
				else
					yaw = lerp(0.0f, 1.0f, clamp(yaw / (1.0f - g_ACJoyEmul.deadZonePerc), 0.0f, 1.0f));

				if (pitch < g_ACJoyEmul.deadZonePerc)
					pitch = 0.0f;
				else
					pitch = lerp(0.0f, 1.0f, clamp(pitch / (1.0f - g_ACJoyEmul.deadZonePerc), 0.0f, 1.0f));

				if (roll < g_ACJoyEmul.rollDeadZonePerc)
					roll = 0.0f;
				else
					roll = lerp(0.0f, 1.0f, clamp(roll / (1.0f - g_ACJoyEmul.rollDeadZonePerc), 0.0f, 1.0f));

				yaw   *= s_yaw;
				pitch *= s_pitch;
				roll  *= s_roll;

				normYaw     = yaw;
				normPitch   = pitch;
				normRoll    = roll;
				pji->dwXpos = (DWORD)(512.0f * ((normYaw   / 2.0f) + 0.5f));
				pji->dwYpos = (DWORD)(512.0f * ((normPitch / 2.0f) + 0.5f));
				pji->dwRpos = (DWORD)(512.0f * ((normRoll  / 2.0f) + 0.5f));
			}
		}

		if (g_ACJoyEmul.throttleEnabled)
		{
			static Vector4 s_anchor;
			static float s_normThrottle   = 0.0f;
			static float s_anchorThrottle = 0.0f;
			// Used to avoid sending multiple actions
			static int   s_fakeKeyPress   = 0;
			// s_state is used to select which axis will be enabled. As soon as the throttle moves
			// away from the anchor beyond the deazone, the axis with the largest motion will be
			// selected with s_state.
			static int   s_state          = 0;

			static WORD s_enterKeyScanCodes[4]     = { 0 };
			static WORD s_backslashKeyScanCodes[4] = { 0 };
			static WORD s_lBracketKeyScanCodes[4]  = { 0 };
			static WORD s_rBracketKeyScanCodes[4]  = { 0 };
			if (s_enterKeyScanCodes[0] == 0)
			{
				char action[] = "ENTER";
				TranslateACAction(-1, s_enterKeyScanCodes, action, nullptr);
			}
			if (s_backslashKeyScanCodes[0] == 0)
			{
				char action[] = "\\";
				TranslateACAction(-1, s_backslashKeyScanCodes, action, nullptr);
			}
			if (s_lBracketKeyScanCodes[0] == 0)
			{
				char action[] = "[";
				TranslateACAction(-1, s_lBracketKeyScanCodes, action, nullptr);
			}
			if (s_rBracketKeyScanCodes[0] == 0)
			{
				char action[] = "]";
				TranslateACAction(-1, s_rBracketKeyScanCodes, action, nullptr);
			}

			if (!(g_prevContStates[thrIdx].buttons[VRButtons::GRIP]) && g_contStates[thrIdx].buttons[VRButtons::GRIP])
			{
				s_anchor = g_contStates[thrIdx].pose * Vector4(0, 0, 0, 1);
				s_anchorThrottle = -1.0f * (GetCurrentPlayerThrottle() * 2.0f - 1.0f);
				s_fakeKeyPress = 0;
				s_state = 0;
			}

			if (g_contStates[thrIdx].buttons[VRButtons::GRIP])
			{
				Vector4 current = g_contStates[thrIdx].pose * Vector4(0, 0, 0, 1);
				// Use left-right motion to set the throttle to 1/3 and 2/3:
				const float Dx  = (current.x - s_anchor.x) / g_ACJoyEmul.thrHalfRange;
				// Use the up-down motion to match the target's speed:
				const float Dy  = (current.y - s_anchor.y) / (2.5f * g_ACJoyEmul.thrHalfRange);
				// Use forwards-backwards motion to compute the throttle:
				const float Dz  = (current.z - s_anchor.z) / g_ACJoyEmul.thrHalfRange;

				const float absDx = fabs(Dx), absDy = fabs(Dy), absDz = fabs(Dz);
				// Find the axis with the largest displacement:
				int   maxDim    = 0;
				float maxAbsDim = absDx;
				if (s_state == 0)
				{
					if (absDy > maxAbsDim)
					{
						maxAbsDim = absDy;
						maxDim = 1;
					}
					if (absDz > maxAbsDim)
					{
						maxAbsDim = absDz;
						maxDim = 2;
					}
				}
				else
				{
					maxDim = s_state - 1;
				}

				switch (maxDim)
				{
					case 0:
						// If the largest motion is along the x-axis, then set 1/3 or 2/3 throttle:
						if (absDx > 0.5f && s_fakeKeyPress == 0)
						{
							const uvfloat4 coords = { -1, -1, -1, -1 };
							if (Dx > 0.0f)
								ACRunAction(s_lBracketKeyScanCodes, coords, -1, thrIdx, pji);
							else
								ACRunAction(s_rBracketKeyScanCodes, coords, -1, thrIdx, pji);
							s_fakeKeyPress = 1;
							s_state = 1;
							break;
						}
					case 1:
						if (absDy > 0.5f && s_fakeKeyPress == 0)
						{
							const uvfloat4 coords = { -1, -1, -1, -1 };
							if (Dy > 0.0f)
								// If the largest motion is along the y-axis, then match the target's speed:
								ACRunAction(s_enterKeyScanCodes, coords, -1, thrIdx, pji);
							else
								// Pushing down stops the ship:
								ACRunAction(s_backslashKeyScanCodes, coords, -1, thrIdx, pji);
							s_fakeKeyPress = 1;
							s_state = 2;
							break;
						}
					case 2:
						// If the largest motion is along the z-axis, then apply the regular throttle:
						// But only apply throttle if no other keys have been "pressed" already
						if ((absDz > 0.5f && s_fakeKeyPress == 0) || s_state == 3)
						{
							s_normThrottle = clamp(s_anchorThrottle + Dz, -1.0f, 1.0f);
							s_state = 3;
						}
						break;
				}
			}

			pji->dwZpos = (DWORD)(65536.0f * ((s_normThrottle / 2.0f) + 0.5f));
		}

		// Synthesize mouse motion
		if (!g_bRendering3D)
		{
			EmulMouseWithVRControllers();
		}
		else if (g_bActiveCockpitEnabled)
		{
			const uvfloat4 coords = { -1, -1, -1, -1 };
			// Synthesize joystick button clicks and run AC actions associated with VR buttons
			for (int contIdx = 0; contIdx < 2; contIdx++)
			{
				for (int buttonIdx = 0; buttonIdx < VRButtons::MAX; buttonIdx++)
				{
					if (IsContinousAction(g_ACJoyMappings[contIdx].action[buttonIdx]))
					{
						if (g_contStates[contIdx].buttons[buttonIdx])
							ACRunAction(g_ACJoyMappings[contIdx].action[buttonIdx], coords, -1, contIdx, pji);
					}
					else
					{
						if (!g_prevContStates[contIdx].buttons[buttonIdx] && g_contStates[contIdx].buttons[buttonIdx])
							ACRunAction(g_ACJoyMappings[contIdx].action[buttonIdx], coords, -1, contIdx, pji);
					}
				}

				// Trigger the fire button on its own (it can only be triggered when pressing grip at the same time)
				if (g_config.JoystickEmul == 3 && contIdx == g_ACJoyEmul.joyHandIdx)
				{
					if (g_contStates[contIdx].buttons[VRButtons::TRIGGER] &&
						g_contStates[contIdx].buttons[VRButtons::GRIP])
					{
						pji->dwButtons |= 1;
						pji->dwButtonNumber = std::max(pji->dwButtonNumber, (DWORD)1);
					}
				}

				// Actions have been processed, we can update the previous state now
				g_prevContStates[contIdx] = g_contStates[contIdx];
			}
		}
	}

	// Joystick emulation through the keyboard
	if (g_config.JoystickEmul == 1)
	{
		bool bCtrlKey = (GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000;
		if (bCtrlKey)
		{
			if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
				pji->dwRpos = static_cast<DWORD>(std::max(256 - 256 * g_config.KbdSensitivity, 0.0f));
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
				pji->dwRpos = static_cast<DWORD>(std::min(256 + 256 * g_config.KbdSensitivity, 512.0f));
			}
		}
		else
		{
			if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
				pji->dwXpos = static_cast<DWORD>(std::max(256 - 256 * g_config.KbdSensitivity, 0.0f));
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
				pji->dwXpos = static_cast<DWORD>(std::min(256 + 256 * g_config.KbdSensitivity, 512.0f));
			}
		}

		if (GetAsyncKeyState(VK_UP) & 0x8000) {
			pji->dwYpos = static_cast<DWORD>(std::max(256 - 256 * g_config.KbdSensitivity, 0.0f));
		}
		if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
			pji->dwYpos = static_cast<DWORD>(std::min(256 + 256 * g_config.KbdSensitivity, 512.0f));
		}
		if (g_config.InvertYAxis) pji->dwYpos = 512 - pji->dwYpos;
		// Normalize each axis to the range -1..1:
		normYaw   = 2.0f * (pji->dwXpos / 512.0f - 0.5f);
		normPitch = 2.0f * (pji->dwYpos / 512.0f - 0.5f);
		normRoll  = 2.0f * (pji->dwRpos / 512.0f - 0.5f);
	}

	if (bJoystickEmulationEnabled && g_pSharedDataJoystick != NULL) {
		g_pSharedDataJoystick->JoystickYaw   = normYaw;
		g_pSharedDataJoystick->JoystickPitch = normPitch;
		g_pSharedDataJoystick->JoystickRoll  = normRoll;
	}

	if (g_bGimbalLockFixActive)
	{
		// Nullify the joystick input, but only if we're inside the cockpit
		// (that's what g_bGimbalLockActive does)
		pji->dwXpos = 256;
		pji->dwYpos = 256;
		pji->dwZpos = 256;
		pji->dwRpos = 256;
	}

	if (!PlayerDataTable[*g_playerIndex].gunnerTurretActive && g_config.SwapJoystickXZAxes) {
		DWORD X = pji->dwXpos;
		pji->dwXpos = pji->dwRpos;
		pji->dwRpos = X;
	}

	return JOYERR_NOERROR;
}
