#include "common.h"
#include <Windows.h>
#include <stdio.h>
#include "utils.h"
#include "commonVR.h"
#include "SteamVR.h"
#include "globals.h"
#include <openvr.h>
#include "FreePIE.h"
#include "SharedMem.h"
#include "XWAFramework.h"

vr::IVRSystem* g_pHMD = NULL;
vr::IVRChaperone* g_pChaperone = NULL;
vr::IVRCompositor* g_pVRCompositor = NULL;
vr::IVRScreenshots* g_pVRScreenshots = NULL;
vr::IVROverlay* g_pVROverlay = NULL;
vr::VROverlayHandle_t g_VR2Doverlay = vr::k_ulOverlayHandleInvalid;
vr::TrackedDevicePose_t g_rTrackedDevicePose;
uint32_t g_steamVRWidth = 0, g_steamVRHeight = 0; // The resolution recommended by SteamVR is stored here
bool g_bSteamVREnabled = false; // The user sets this flag to true to request support for SteamVR.
bool g_bSteamVRInitialized = false; // The system will set this flag after SteamVR has been initialized
bool g_bUseSteamVR = false; // The system will set this flag if the user requested SteamVR and SteamVR was initialized properly
bool g_bUseTextureCube = false;
bool g_bRenderDefaultStarfield = true;
bool g_bDebugDefaultStarfield = false;
bool g_bEnableCubeMaps = false;
bool g_bRenderCubeMapInThisRegion = false;
bool g_bReplaceBackdrops = false;
bool g_bDefaultStarfieldRendered = false; // Reset on every frame
const bool DEFAULT_INTERLEAVED_REPROJECTION = false;
const bool DEFAULT_STEAMVR_POS_FROM_FREEPIE = false;
bool g_bInterleavedReprojection = DEFAULT_INTERLEAVED_REPROJECTION;
bool g_bSteamVRDistortionEnabled = true;
bool g_bSteamVRYawPitchRollFromMouseLook = false;
bool g_bTogglePostPresentHandoff = false;
bool g_bSteamVRMirrorWindowLeftEye = true;
//bool g_bResetHeadCenter = true; // Reset the head center on startup
//vr::HmdMatrix34_t g_EyeMatrixLeft, g_EyeMatrixRight;
//Matrix4 g_EyeMatrixLeftInv, g_EyeMatrixRightInv;
//Matrix4 g_projLeft, g_projRight;
//Matrix4 g_FullProjMatrixLeft, g_FullProjMatrixRight, g_viewMatrix;
void* g_pSurface = NULL;

//float g_fMetricMult = DEFAULT_METRIC_MULT, 
float g_fFrameTimeRemaining = 0.005f;
int g_iSteamVR_Remaining_ms = 3, g_iSteamVR_VSync_ms = 11;
bool g_bSteamVRPosFromFreePIE = DEFAULT_STEAMVR_POS_FROM_FREEPIE;
float g_fSteamVRMirrorWindow3DScale = 0.7f, g_fSteamVRMirrorWindowAspectRatio = 0.0f;

ControllerState g_prevContStates[2];
ControllerState g_contStates[2];
VRKeybState     g_vrKeybState;

const char *VRButtonNames[VRButtons::MAX]
{
	"TRIGGER",
	"GRIP",
	"BUTTON_1",
	"BUTTON_2",

	"PAD_LEFT",
	"PAD_RIGHT",
	"PAD_UP",
	"PAD_DOWN",
	"PAD_CLICK",
};

void ProcessVRControllerButtons();

bool InitSteamVR()
{
	/*
	How to enable the null driver:
	https://www.reddit.com/r/Vive/comments/6uo053/how_to_use_steamvr_tracked_devices_without_a_hmd/

	steamvr.vrsettings file:
	C:\Program Files (x86)\Steam\config

	null driver config file:
	C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\null\resources\settings
	*/
	char* strDriver = NULL;
	char* strDisplay = NULL;
	FILE* file = NULL;
	//Matrix4 RollTest;
	bool result = true;

	int file_error = fopen_s(&file, "./steamvr_mat.txt", "wt");
	log_debug("[DBG] Initializing SteamVR");
	vr::EVRInitError eError = vr::VRInitError_None;
	g_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	g_headCenter.set(0, 0, 0);
	g_pChaperone = vr::VRChaperone();

	if (eError != vr::VRInitError_None)
	{
		g_pHMD = NULL;
		log_debug("[DBG] Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		result = false;
		goto out;
	}
	log_debug("[DBG] SteamVR Initialized");
	g_pHMD->GetRecommendedRenderTargetSize(&g_steamVRWidth, &g_steamVRHeight);
	log_debug("[DBG] Recommended steamVR width, height: %d, %d", g_steamVRWidth, g_steamVRHeight);

	strDriver = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	if (strDriver) {
		log_debug("[DBG] Driver: %s", strDriver);
		strDisplay = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
	}

	if (strDisplay)
		log_debug("[DBG] Display: %s", strDisplay);

	g_pVRCompositor = vr::VRCompositor();
	if (g_pVRCompositor == NULL)
	{
		log_debug("[DBG] SteamVR Compositor Initialization failed.");
		result = false;
		goto out;
	}
	log_debug("[DBG] SteamVR Compositor Initialized");

	g_pVRCompositor->SetTrackingSpace(vr::TrackingUniverseSeated);
	g_pVRCompositor->ForceInterleavedReprojectionOn(g_bInterleavedReprojection);
	log_debug("[DBG] InterleavedReprojection: %d", g_bInterleavedReprojection);

	g_pVRScreenshots = vr::VRScreenshots();
	if (g_pVRScreenshots != NULL) {
		log_debug("[DBG] SteamVR screenshot system enabled");
	}

	// Reset the seated pose
	//g_pHMD->ResetSeatedZeroPose();
	g_pChaperone->ResetZeroPose(vr::TrackingUniverseSeated);

	// Pre-multiply and store the eye and projection matrices:
	ProcessSteamVREyeMatrices(vr::EVREye::Eye_Left);
	ProcessSteamVREyeMatrices(vr::EVREye::Eye_Right);

	vr::HmdMatrix44_t projLeft = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Left, 0.001f, 100.0f);
	vr::HmdMatrix44_t projRight = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Right, 0.001f, 100.0f);

	g_projLeft = HmdMatrix44toMatrix4(projLeft);
	g_projRight = HmdMatrix44toMatrix4(projRight);

	// Override all of the above with the Pimax matrices
	//TestPimax();

	g_FullProjMatrixLeft = g_projLeft * g_EyeMatrixLeftInv;
	g_FullProjMatrixRight = g_projRight * g_EyeMatrixRightInv;

	//Test2DMesh();

	ShowMatrix4(g_EyeMatrixLeftInv, "g_EyeMatrixLeftInv");
	ShowMatrix4(g_projLeft, "g_projLeft");

	ShowMatrix4(g_EyeMatrixRightInv, "g_EyeMatrixRightInv");
	ShowMatrix4(g_projRight, "g_projRight");

	// Initialize FreePIE if we're going to use it to read the position.
	if (g_bSteamVRPosFromFreePIE)
		InitFreePIE();

	// Compute the FOV from the left eye vertical params. This FOV will be applied
	// and saved in PrimarySurface::Flip()
	float left, right, top, bottom;
	g_pHMD->GetProjectionRaw(vr::EVREye::Eye_Left, &left, &right, &top, &bottom);
	g_fVR_FOV = (atan(fabs(top)) + atan(fabs(bottom))) / DEG2RAD;

	// Dump information about the view matrices
	if (file_error == 0) {
		Matrix4 eye, test;

		if (strDriver != NULL)
			fprintf(file, "Driver: %s\n", strDriver);
		if (strDisplay != NULL)
			fprintf(file, "Display: %s\n", strDisplay);
		fprintf(file, "\n");

		// Left Eye matrix
		fprintf(file, "eyeLeft:\n");
		DumpMatrix34(file, g_EyeMatrixLeft);
		fprintf(file, "\n");

		fprintf(file, "eyeLeftInv:\n");
		DumpMatrix4(file, g_EyeMatrixLeftInv);
		fprintf(file, "\n");

		eye = HmdMatrix34toMatrix4(g_EyeMatrixLeft);
		test = eye * g_EyeMatrixLeftInv;
		fprintf(file, "Left Eye inverse test:\n");
		DumpMatrix4(file, test);
		fprintf(file, "\n");

		// Right Eye matrix
		fprintf(file, "eyeRight:\n");
		DumpMatrix34(file, g_EyeMatrixRight);
		fprintf(file, "\n");

		fprintf(file, "eyeRightInv:\n");
		DumpMatrix4(file, g_EyeMatrixRightInv);
		fprintf(file, "\n");

		eye = HmdMatrix34toMatrix4(g_EyeMatrixRight);
		test = eye * g_EyeMatrixRightInv;
		fprintf(file, "Right Eye inverse test:\n");
		DumpMatrix4(file, test);
		fprintf(file, "\n");

		// Z_FAR was 50 for version 0.9.6, and I believe Z_Near was 0.5 (focal dist)
		//vr::HmdMatrix44_t projLeft = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Left, DEFAULT_FOCAL_DIST, 50.0f);
		//vr::HmdMatrix44_t projRight = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Right, DEFAULT_FOCAL_DIST, 50.0f);

		fprintf(file, "projLeft:\n");
		DumpMatrix44(file, projLeft);
		fprintf(file, "\n");

		fprintf(file, "projRight:\n");
		DumpMatrix44(file, projRight);
		fprintf(file, "\n");

		g_pHMD->GetProjectionRaw(vr::EVREye::Eye_Left, &left, &right, &top, &bottom);
		fprintf(file, "Raw data (Left eye):\n");
		fprintf(file, "Left: %0.6f, Right: %0.6f, Top: %0.6f, Bottom: %0.6f\n",
			left, right, top, bottom);

		g_pHMD->GetProjectionRaw(vr::EVREye::Eye_Right, &left, &right, &top, &bottom);
		fprintf(file, "Raw data (Right eye):\n");
		fprintf(file, "Left: %0.6f, Right: %0.6f, Top: %0.6f, Bottom: %0.6f\n",
			left, right, top, bottom);
		fclose(file);
	}

	// Set a black background to draw the overlay over
	g_pVRCompositor->FadeGrid(2.0f, false);
	g_pVRCompositor->FadeToColor(2.0f, 0.0f, 0.0f, 0.0f, 1.0, 0);

	// Create Overlay to draw 2D content fixed in space
	g_pVROverlay = vr::VROverlay();
	if (g_pVROverlay != NULL) {
		log_debug("[DBG] SteamVR Overlay system initialized");
	}
	vr::HmdMatrix34_t overlay_transform;
	// This transform matrix puts the overlay at 5m in front of the user POV
	overlay_transform = {
		1.0f, 0.0f, 0.0f,  0.0f,
		0.0f, 1.0f, 0.0f,  0.0f,
		0.0f, 0.0f, 1.0f, -g_fSteamVROverlayDepthInMeters
	};
	g_pVROverlay->CreateOverlay("xwa_2d_window", "X-Wing Alliance VR", &g_VR2Doverlay);
	g_pVROverlay->SetOverlayWidthInMeters(g_VR2Doverlay, g_fSteamVROverlayWidthInMeters); // Make the overlay 5 meters wide.
	g_pVROverlay->SetOverlayTransformAbsolute(g_VR2Doverlay, vr::TrackingUniverseSeated, &overlay_transform);
	if (g_bEnableVRPointerInConcourse)
	{
		g_pVROverlay->SetOverlayInputMethod(g_VR2Doverlay, vr::VROverlayInputMethod_Mouse);
		g_pVROverlay->SetOverlayFlag(g_VR2Doverlay, vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible, true);
	}

out:
	g_bSteamVRInitialized = result;

	if (strDriver != NULL) {
		delete[] strDriver;
		strDriver = NULL;
	}
	if (strDisplay != NULL) {
		delete[] strDisplay;
		strDisplay = NULL;
	}
	if (file != NULL) {
		fclose(file);
		file = NULL;
	}
	return result;
}

void ShutDownSteamVR() {
	// If the user sets g_bSteamVRPosFromFreePIE when XWA is starting, they may
	// also reload the vrparams and then unset this flag. In that case, we won't
	// shut FreePIE down. Well, this is a private hack, so I'm not going to care
	// about that case.
	if (g_bSteamVRPosFromFreePIE)
		ShutdownFreePIE();

	// We can't shut down SteamVR twice, we either shut it down here or in the cockpit look hook.
	// It looks like the right order is to shut SteamVR down in the cockpit look hook
	return;
	//log_debug("[DBG] Not shutting down SteamVR, just returning...");
	//return;
	/*
	log_debug("[DBG] Shutting down SteamVR...");
	vr::VR_Shutdown();
	g_pHMD = NULL;
	g_pVRCompositor = NULL;
	g_pVRScreenshots = NULL;
	log_debug("[DBG] SteamVR shut down");
	*/
}

/*
 * In SteamVR, the coordinate system is as follows:
 * +x is right
 * +y is up
 * -z is forward
 * Distance is meters
 * This function is not used anymore and the projection is done in the vertex shader
 */
void projectSteamVR(float X, float Y, float Z, vr::EVREye eye, float& x, float& y, float& z) {
	Vector4 PX; // PY;

	PX.set(X, -Y, -Z, 1.0f);
	if (eye == vr::EVREye::Eye_Left) {
		PX = g_FullProjMatrixLeft * PX;
	}
	else {
		PX = g_FullProjMatrixRight * PX;
	}
	// Project
	PX /= PX[3];
	// Convert to 2D
	x = PX[0];
	y = -PX[1];
	z = PX[2];
}

char* GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError)
{
	char* pchBuffer = NULL;
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return pchBuffer;

	pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	return pchBuffer;
}

void DisplayController(int idx)
{
	std::string name = idx == 0 ? "LT," : "RT,";
	for (int i = 0; i < VRButtons::MAX; i++)
	{
		if (g_contStates[idx].buttons[i])
		{
			name += VRButtonNames[i];
			name += ",";
		}
	}
	name += "(" + std::to_string(g_contStates[idx].trackPadX) + ", " + std::to_string(g_contStates[idx].trackPadY) + "), ";
	log_debug("[DBG] [AC] %s", name.c_str());
}

void GetSteamVRPositionalData(float* yaw, float* pitch, float* roll, float* x, float* y, float* z, Matrix4* m4_hmdPose)
{
	vr::TrackedDeviceIndex_t unDevice = vr::k_unTrackedDeviceIndex_Hmd;

	if (!g_pHMD->IsTrackedDeviceConnected(unDevice))
		return;

	static vr::TrackedDeviceIndex_t leftId  = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
	static vr::TrackedDeviceIndex_t rightId = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);

	// Use GetControllerState() to detect when controllers are disconnected:
	vr::VRControllerState_t state;
	g_contStates[0].bIsValid = g_pHMD->GetControllerState(leftId, &state, sizeof(state));
	g_contStates[1].bIsValid = g_pHMD->GetControllerState(rightId, &state, sizeof(state));

	if (g_pHMD->GetControllerState(unDevice, &state, sizeof(state)))
	{
		// Pose array predicted for current frame N, to use by ddraw to render current frame in GPU (minimize latency)
		vr::TrackedDevicePose_t trackedDevicePoseArray[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t trackedDevicePose; // HMD pose to use for the current frame render
		vr::TrackedDevicePose_t trackedControllerPose[2];
		vr::HmdMatrix34_t m34_fullMatrix;
		vr::HmdQuaternionf_t q;
		vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(unDevice);

		if (g_bRendering3D)
		{
			// For legacy/stable tracking, WaitGetPoses() is run in CockpitLook. Get the last tracking pose obtained then.
			// This pose was just used to render the current frame in xwingaliance.exe and will provide consistent tracking.			
			vr::VRCompositor()->GetLastPoses(trackedDevicePoseArray, vr::k_unMaxTrackedDeviceCount, NULL, 0);
		}
		else
		{
			// CockpitLookHook is not running (we are in 2D mode). We do the vsync blocking here
			vr::VRCompositor()->WaitGetPoses(trackedDevicePoseArray, vr::k_unMaxTrackedDeviceCount, NULL, 0);
		}
		trackedDevicePose        = trackedDevicePoseArray[vr::k_unTrackedDeviceIndex_Hmd];
		trackedControllerPose[0] = trackedDevicePoseArray[leftId];
		trackedControllerPose[1] = trackedDevicePoseArray[rightId];

		if (trackedDevicePose.bPoseIsValid)
		{
			*m4_hmdPose = HmdMatrix34toMatrix4(trackedDevicePose.mDeviceToAbsoluteTracking);

			// We take the pose as it was returned by GetLastPoses() and used by CockpitLook.
			Matrix4toHmdMatrix34(*m4_hmdPose, m34_fullMatrix);

			// We extract the components of the composed matrix to use them later to correct the rotation
			// of 2D elements like reticle, hyperspace, speedeffect, shadows...
			q = rotationToQuaternion(m34_fullMatrix);
			quatToEuler(q, yaw, pitch, roll);

			*x = m34_fullMatrix.m[0][3];
			*y = m34_fullMatrix.m[1][3];
			*z = m34_fullMatrix.m[2][3];
		}

		for (int i = 0; i < 2; i++)
		{
			// trackedControllerPose[i].bPoseIsValid returns random 8-bit numbers when the controllers are off
			//g_contStates[i].bIsValid = trackedControllerPose[i].bPoseIsValid;
			g_contStates[i].pose     = HmdMatrix34toMatrix4(trackedControllerPose[i].mDeviceToAbsoluteTracking);

			// Get the state of the buttons
			vr::VRControllerState_t state;
			const int contIdx = i == 0 ? leftId : rightId;
			if (vr::VRSystem()->GetControllerState(contIdx, &state, sizeof(state)))
			{
				//if (g_contStates[i].packetNum != state.unPacketNum)
				{
					g_contStates[i].buttons[VRButtons::TRIGGER]   = (state.rAxis[1].x > 0.1f); // 0 is fully released
					g_contStates[i].buttons[VRButtons::GRIP]      = (state.ulButtonPressed & 0x04) != 0;
					g_contStates[i].buttons[VRButtons::BUTTON_1]  = (state.ulButtonPressed & 0x80) != 0;
					g_contStates[i].buttons[VRButtons::BUTTON_2]  = (state.ulButtonPressed & 0x02) != 0;
					g_contStates[i].buttons[VRButtons::PAD_CLICK] = (state.ulButtonPressed >> 32) & 1;
					g_contStates[i].buttons[VRButtons::PAD_LEFT]  = (state.rAxis[0].x < -0.8f);
					g_contStates[i].buttons[VRButtons::PAD_RIGHT] = (state.rAxis[0].x >  0.8f);
					g_contStates[i].buttons[VRButtons::PAD_UP]    = (state.rAxis[0].y >  0.8f);
					g_contStates[i].buttons[VRButtons::PAD_DOWN]  = (state.rAxis[0].y < -0.8f);

					g_contStates[i].trackPadX = state.rAxis[0].x;
					g_contStates[i].trackPadY = state.rAxis[0].y;
					g_contStates[i].packetNum = state.unPacketNum;

					//vr::HmdMatrix34_t m34_fullMatrix;
					//vr::HmdQuaternionf_t q;
					//Matrix4toHmdMatrix34(g_contStates[i].pose, m34_fullMatrix);
					//q = rotationToQuaternion(m34_fullMatrix);
					//quatToEuler(q, &(g_contStates[i].yaw), &(g_contStates[i].pitch), &(g_contStates[i].roll));

					//DisplayController(i);
				}
			}

		}
	}

	// When we're parked in the hangar, the joystick loop won't get called; so we need to manually
	// process the VR controller events here or the VR buttons won't work. This should at least
	// allow users to bring up the VR keyboard using the controller to do everything else.
	if (*g_playerInHangar)
		ProcessVRControllerButtons();
}

/*
   From: http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/index.htm
   yaw: left = +90, right = -90
   pitch: up = +90, down = -90
   roll: left = +90, right = -90

   if roll > 90, the axis will swap pitch and roll; but why would anyone do that?
*/
void quatToEuler(vr::HmdQuaternionf_t q, float* yaw, float* roll, float* pitch) {
	float test = q.x * q.y + q.z * q.w;

	if (test > 0.499f) { // singularity at north pole
		*yaw = 2 * atan2(q.x, q.w);
		*pitch = PI / 2.0f;
		*roll = 0;
		return;
	}
	if (test < -0.499f) { // singularity at south pole
		*yaw = -2 * atan2(q.x, q.w);
		*pitch = -PI / 2.0f;
		*roll = 0;
		return;
	}
	float sqx = q.x * q.x;
	float sqy = q.y * q.y;
	float sqz = q.z * q.z;
	*yaw = atan2(2.0f * q.y * q.w - 2.0f * q.x * q.z, 1.0f - 2.0f * sqy - 2.0f * sqz);
	*pitch = asin(2.0f * test);
	*roll = atan2(2.0f * q.x * q.w - 2.0f * q.y * q.z, 1.0f - 2.0f * sqx - 2.0f * sqz);
}

/*
 * From: http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/index.htm
 */
vr::HmdQuaternionf_t eulerToQuat(float yaw, float pitch, float roll) {
	vr::HmdQuaternionf_t q;
	// Assuming the angles are in radians.
	float c1 = cos(yaw / 2.0f);
	float s1 = sin(yaw / 2.0f);
	float c2 = cos(pitch / 2.0f);
	float s2 = sin(pitch / 2.0f);
	float c3 = cos(roll / 2.0f);
	float s3 = sin(roll / 2.0f);
	float c1c2 = c1 * c2;
	float s1s2 = s1 * s2;
	q.w = c1c2 * c3 - s1s2 * s3;
	q.x = c1c2 * s3 + s1s2 * c3;
	q.y = s1 * c2 * c3 + c1 * s2 * s3;
	q.z = c1 * s2 * c3 - s1 * c2 * s3;
	return q;
}

/*
 * From http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
 */
vr::HmdMatrix33_t quatToMatrix(vr::HmdQuaternionf_t q) {
	vr::HmdMatrix33_t m;
	float sqw = q.w * q.w;
	float sqx = q.x * q.x;
	float sqy = q.y * q.y;
	float sqz = q.z * q.z;

	// invs (inverse square length) is only required if quaternion is not already normalised
	float invs = 1 / (sqx + sqy + sqz + sqw);
	m.m[0][0] = (sqx - sqy - sqz + sqw) * invs; // since sqw + sqx + sqy + sqz =1/invs*invs
	m.m[1][1] = (-sqx + sqy - sqz + sqw) * invs;
	m.m[2][2] = (-sqx - sqy + sqz + sqw) * invs;

	float tmp1 = q.x * q.y;
	float tmp2 = q.z * q.w;
	m.m[1][0] = 2.0f * (tmp1 + tmp2) * invs;
	m.m[0][1] = 2.0f * (tmp1 - tmp2) * invs;

	tmp1 = q.x * q.z;
	tmp2 = q.y * q.w;
	m.m[2][0] = 2.0f * (tmp1 - tmp2) * invs;
	m.m[0][2] = 2.0f * (tmp1 + tmp2) * invs;
	tmp1 = q.y * q.z;
	tmp2 = q.x * q.w;
	m.m[2][1] = 2.0f * (tmp1 + tmp2) * invs;
	m.m[1][2] = 2.0f * (tmp1 - tmp2) * invs;
	return m;
}

/*
 * Convert a rotation matrix to a normalized quaternion.
 * From: http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
 */
vr::HmdQuaternionf_t rotationToQuaternion(vr::HmdMatrix34_t m) {
	float tr = m.m[0][0] + m.m[1][1] + m.m[2][2];
	vr::HmdQuaternionf_t q;

	if (tr > 0) {
		float S = sqrt(tr + 1.0f) * 2.0f; // S=4*qw
		q.w = 0.25f * S;
		q.x = (m.m[2][1] - m.m[1][2]) / S;
		q.y = (m.m[0][2] - m.m[2][0]) / S;
		q.z = (m.m[1][0] - m.m[0][1]) / S;
	}
	else if ((m.m[0][0] > m.m[1][1]) && (m.m[0][0] > m.m[2][2])) {
		float S = sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2.0f; // S=4*qx
		q.w = (m.m[2][1] - m.m[1][2]) / S;
		q.x = 0.25f * S;
		q.y = (m.m[0][1] + m.m[1][0]) / S;
		q.z = (m.m[0][2] + m.m[2][0]) / S;
	}
	else if (m.m[1][1] > m.m[2][2]) {
		float S = sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2.0f; // S=4*qy
		q.w = (m.m[0][2] - m.m[2][0]) / S;
		q.x = (m.m[0][1] + m.m[1][0]) / S;
		q.y = 0.25f * S;
		q.z = (m.m[1][2] + m.m[2][1]) / S;
	}
	else {
		float S = sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2.0f; // S=4*qz
		q.w = (m.m[1][0] - m.m[0][1]) / S;
		q.x = (m.m[0][2] + m.m[2][0]) / S;
		q.y = (m.m[1][2] + m.m[2][1]) / S;
		q.z = 0.25f * S;
	}
	float Q = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	q.x /= Q;
	q.y /= Q;
	q.z /= Q;
	q.w /= Q;
	return q;
}

Matrix3 HmdMatrix34toMatrix3(const vr::HmdMatrix34_t& mat) {
	Matrix3 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2]
	);
	return matrixObj;
}

Matrix3 HmdMatrix33toMatrix3(const vr::HmdMatrix33_t& mat) {
	Matrix3 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2]
	);
	return matrixObj;
}

Matrix4 HmdMatrix44toMatrix4(const vr::HmdMatrix44_t& mat) {
	Matrix4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
	return matrixObj;
}

Matrix4 HmdMatrix34toMatrix4(const vr::HmdMatrix34_t& mat) {
	Matrix4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	);
	return matrixObj;
}

void Matrix4toHmdMatrix44(const Matrix4& m4, vr::HmdMatrix44_t& mat) {
	mat.m[0][0] = m4[0];  mat.m[1][0] = m4[1];  mat.m[2][0] = m4[2];  mat.m[3][0] = m4[3];
	mat.m[0][1] = m4[4];  mat.m[1][1] = m4[5];  mat.m[2][1] = m4[6];  mat.m[3][1] = m4[7];
	mat.m[0][2] = m4[8];  mat.m[1][2] = m4[9];  mat.m[2][2] = m4[10]; mat.m[3][2] = m4[11];
	mat.m[0][3] = m4[12]; mat.m[1][3] = m4[13]; mat.m[2][3] = m4[14]; mat.m[3][3] = m4[15];
}

void Matrix4toHmdMatrix34(const Matrix4& m4, vr::HmdMatrix34_t& mat) {
	mat.m[0][0] = m4[0];  mat.m[1][0] = m4[1];  mat.m[2][0] = m4[2];
	mat.m[0][1] = m4[4];  mat.m[1][1] = m4[5];  mat.m[2][1] = m4[6];
	mat.m[0][2] = m4[8];  mat.m[1][2] = m4[9];  mat.m[2][2] = m4[10];
	mat.m[0][3] = m4[12]; mat.m[1][3] = m4[13]; mat.m[2][3] = m4[14];
}

void DumpMatrix34(FILE* file, const vr::HmdMatrix34_t& m) {
	if (file == NULL)
		return;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			fprintf(file, "%0.6f", m.m[i][j]);
			if (j < 3)
				fprintf(file, ", ");
		}
		fprintf(file, "\n");
	}
}

void DumpMatrix44(FILE* file, const vr::HmdMatrix44_t& m) {
	if (file == NULL)
		return;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			fprintf(file, "%0.6f", m.m[i][j]);
			if (j < 3)
				fprintf(file, ", ");
		}
		fprintf(file, "\n");
	}
}

void DumpMatrix4(FILE* file, const Matrix4& mat) {
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[0], mat[4], mat[8], mat[12]);
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[1], mat[5], mat[9], mat[13]);
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[2], mat[6], mat[10], mat[14]);
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[3], mat[7], mat[11], mat[15]);
}

void ShowMatrix4(const Matrix4& mat, char* name) {
	log_debug("[DBG] -----------------------------------------");
	if (name != NULL)
		log_debug("[DBG] %s", name);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[0], mat[4], mat[8], mat[12]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[1], mat[5], mat[9], mat[13]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[2], mat[6], mat[10], mat[14]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[3], mat[7], mat[11], mat[15]);
	log_debug("[DBG] =========================================");
}

void ShowHmdMatrix34(const vr::HmdMatrix34_t& mat, char* name) {
	log_debug("[DBG] -----------------------------------------");
	if (name != NULL)
		log_debug("[DBG] %s", name);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3]);
	log_debug("[DBG] =========================================");
}

void ShowHmdMatrix44(const vr::HmdMatrix44_t& mat, char* name) {
	log_debug("[DBG] -----------------------------------------");
	if (name != NULL)
		log_debug("[DBG] %s", name);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	log_debug("[DBG] =========================================");
}

void ProcessSteamVREyeMatrices(vr::EVREye eye) {
	if (g_pHMD == NULL) {
		log_debug("[DBG] Cannot process SteamVR matrices because g_pHMD == NULL");
		return;
	}

	vr::HmdMatrix34_t eyeMatrix = g_pHMD->GetEyeToHeadTransform(eye);
	//ShowHmdMatrix34(eyeMatrix, "HmdMatrix34_t eyeMatrix");
	if (eye == vr::EVREye::Eye_Left)
		g_EyeMatrixLeft = eyeMatrix;
	else
		g_EyeMatrixRight = eyeMatrix;

	Matrix4 matrixObj = HmdMatrix34toMatrix4(eyeMatrix);
	/*
	// Pimax matrix: 11.11 degrees on the Y axis and 6.64 IPD
	Matrix4 matrixObj(
		0.984808, 0.000000, 0.173648, -0.033236,
		0.000000, 1.000000, 0.000000, 0.000000,
		-0.173648, 0.000000, 0.984808, 0.000000,
		0, 0, 0, 1);
	matrixObj.transpose();
	*/

	/*
	if (g_bInverseTranspose) {
		log_debug("[DBG] Computing Inverse Transpose");
		matrixObj.transpose();
	}
	*/
	// Invert the matrix and store it
	matrixObj.invertGeneral();
	if (eye == vr::EVREye::Eye_Left)
		g_EyeMatrixLeftInv = matrixObj;
	else
		g_EyeMatrixRightInv = matrixObj;
}

void ShowVector4(const Vector4& X, char* name) {
	if (name != NULL)
		log_debug("[DBG] %s = %0.6f, %0.6f, %0.6f, %0.6f",
			name, X[0], X[1], X[2], X[3]);
	else
		log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f",
			X[0], X[1], X[2], X[3]);
}

/*
void ProcessVREvent(const vr::VREvent_t & event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		log_debug("[DBG] Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		log_debug("[DBG] Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}
*/

/*
bool HandleVRInput()
{
	bool bRet = false;

	// Process SteamVR events
	vr::VREvent_t event;
	while (g_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		ProcessVREvent(event);
	}

	return bRet;
}
*/

void TestPimax() {
	vr::HmdMatrix34_t eyeLeft;
	eyeLeft.m[0][0] = 0.984808f; eyeLeft.m[0][1] = 0.000000f, eyeLeft.m[0][2] = 0.173648f; eyeLeft.m[0][3] = -0.033236f;
	eyeLeft.m[1][0] = 0.000000f; eyeLeft.m[1][1] = 1.000000f; eyeLeft.m[1][2] = 0.000000f; eyeLeft.m[1][3] = 0.000000f;
	eyeLeft.m[2][0] = -0.173648f; eyeLeft.m[2][1] = 0.000000f; eyeLeft.m[2][2] = 0.984808f; eyeLeft.m[2][3] = 0.000000f;

	vr::HmdMatrix34_t eyeRight;
	eyeRight.m[0][0] = 0.984808f; eyeRight.m[0][1] = 0.000000f, eyeRight.m[0][2] = -0.173648f; eyeRight.m[0][3] = 0.033236f;
	eyeRight.m[1][0] = 0.000000f; eyeRight.m[1][1] = 1.000000f; eyeRight.m[1][2] = 0.000000f; eyeRight.m[1][3] = 0.000000f;
	eyeRight.m[2][0] = 0.173648f; eyeRight.m[2][1] = 0.000000f; eyeRight.m[2][2] = 0.984808f; eyeRight.m[2][3] = 0.000000f;

	g_EyeMatrixLeftInv = HmdMatrix34toMatrix4(eyeLeft);
	g_EyeMatrixRightInv = HmdMatrix34toMatrix4(eyeRight);
	g_EyeMatrixLeftInv.invertGeneral();
	g_EyeMatrixRightInv.invertGeneral();

	vr::HmdMatrix44_t projLeft;
	projLeft.m[0][0] = 0.647594f; projLeft.m[0][1] = 0.000000f; projLeft.m[0][2] = -0.128239f; projLeft.m[0][3] = 0.000000f;
	projLeft.m[1][0] = 0.000000f; projLeft.m[1][1] = 0.787500f; projLeft.m[1][2] = 0.000000f; projLeft.m[1][3] = 0.000000f;
	projLeft.m[2][0] = 0.000000f; projLeft.m[2][1] = 0.000000f; projLeft.m[2][2] = -1.010101f; projLeft.m[2][3] = -0.505050f;
	projLeft.m[3][0] = 0.000000f; projLeft.m[3][1] = 0.000000f; projLeft.m[3][2] = -1.000000f; projLeft.m[3][3] = 0.000000f;

	vr::HmdMatrix44_t projRight;
	projRight.m[0][0] = 0.647594f; projRight.m[0][1] = 0.000000f; projRight.m[0][2] = 0.128239f; projRight.m[0][3] = 0.000000f;
	projRight.m[1][0] = 0.000000f; projRight.m[1][1] = 0.787500f; projRight.m[1][2] = 0.000000f; projRight.m[1][3] = 0.000000f;
	projRight.m[2][0] = 0.000000f; projRight.m[2][1] = 0.000000f; projRight.m[2][2] = -1.010101f; projRight.m[2][3] = -0.505050f;
	projRight.m[3][0] = 0.000000f; projRight.m[3][1] = 0.000000f; projRight.m[3][2] = -1.000000f; projRight.m[3][3] = 0.000000f;

	g_projLeft = HmdMatrix44toMatrix4(projLeft);
	g_projRight = HmdMatrix44toMatrix4(projRight);
}

void Test2DMesh() {
	/*
	MainVertex vertices[4] =
	{
		MainVertex(-1, -1, 0, 1),
		MainVertex(1, -1, 1, 1),
		MainVertex(1,  1, 1, 0),
		MainVertex(-1,  1, 0, 0),
	};
	*/
	Matrix4 fullMatrixLeft = g_FullProjMatrixLeft;
	//fullMatrixLeft.invertGeneral();
	Vector4 P, Q;

	P.set(-12, -12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);

	P.set(12, -12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);

	P.set(-12, 12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);

	P.set(12, 12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);
}