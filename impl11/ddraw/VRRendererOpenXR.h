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

struct swapchain_t {
	XrSwapchain handle;
	int32_t     width;
	int32_t     height;
	std::vector<XrSwapchainImageD3D11KHR> surface_images;
};

///<inheritdoc/>
class VRRendererOpenXR :
    public StereoRenderer
{
public:
	static std::vector<const char*> use_extensions;
	static const char* ask_extensions[];
	static bool is_available(); //Returns true if there is a valid OpenXR runtime available

	VRRendererOpenXR::VRRendererOpenXR();
	bool init(DeviceResources *deviceResources);
	bool is_ready();
	void WaitFrame();
	void BeginFrame();
	void UpdateViewMatrices();
	void EndFrame(ID3D11Device* d3dDevice);
	void Submit(ID3D11DeviceContext* context, ID3D11Texture2D* eyeBuffer, VREye vrEye);
	virtual void ShutDown();

private:
	const XrPosef  xr_pose_identity = { {0,0,0,1}, {0,0,0} };
	XrInstance     xr_instance = {};
	XrSession      xr_session = {};
	XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
	bool           xr_running = false;
	XrSpace        xr_app_space = {};
	XrSpace        xr_hmd_space = {};
	XrSystemId     xr_system_id = XR_NULL_SYSTEM_ID;
	XrFormFactor            app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	XrFrameState	frame_state = { XR_TYPE_FRAME_STATE };
	XrEnvironmentBlendMode   xr_blend = { XR_ENVIRONMENT_BLEND_MODE_OPAQUE };
	XrCompositionLayerBaseHeader* layer = nullptr;
	XrCompositionLayerProjection layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	std::vector<XrCompositionLayerProjectionView> layer_proj_views;

	std::vector<XrView> xr_views;
	std::vector<XrViewConfigurationView> xr_config_views;
	std::vector<swapchain_t> xr_swapchains;

	// Function pointers for some OpenXR extension methods we'll use.
	PFN_xrGetD3D11GraphicsRequirementsKHR ext_xrGetD3D11GraphicsRequirementsKHR = nullptr;
	PFN_xrCreateDebugUtilsMessengerEXT    ext_xrCreateDebugUtilsMessengerEXT = nullptr;
	PFN_xrDestroyDebugUtilsMessengerEXT   ext_xrDestroyDebugUtilsMessengerEXT = nullptr;
	//TODO add XR_KHR_composition_layer_depth extension to submit depth to the compositor
	//TODO add XR_KHR_visibility_mask extension to provide the render a visibility mask used for culling objects not visible due to HMD optics design.
};

