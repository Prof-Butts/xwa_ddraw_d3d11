#pragma once

// Tell OpenXR what platform code we'll be using
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11

#include "StereoRenderer.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

extern bool g_bOpenXREnabled; // The user sets this flag to true to request support for OpenXR
extern bool g_bUseOpenXR; //The systems sets this flag when an OpenXR is available and compatible
extern bool g_bOpenXRInitialized; //The systems sets this flag when OpenXR has been initialized correctly

class VRRendererOpenXR :
    public StereoRenderer
{
public:
	static std::vector<const char*> use_extensions;
	static const char* ask_extensions[];
	static bool is_available();

	const XrPosef  xr_pose_identity = { {0,0,0,1}, {0,0,0} };
	XrInstance     xr_instance = {};
	XrSession      xr_session = {};
	XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
	bool           xr_running = false;
	XrSpace        xr_app_space = {};
	XrSystemId     xr_system_id = XR_NULL_SYSTEM_ID;
	XrFormFactor            app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

	// Function pointers for some OpenXR extension methods we'll use.
	PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR = nullptr;
	PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
	PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;

	bool init(DeviceResources *deviceResources);
	void RenderLeft(ID3D11Texture2D* LeftEyeBuffer);
	void RenderRight(ID3D11Texture2D* LeftEyeBuffer);
};

