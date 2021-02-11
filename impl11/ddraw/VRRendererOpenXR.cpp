#include "VRRendererOpenXR.h"
#include "EffectsCommon.h"
#include <algorithm> // any_of

using namespace std;

bool g_bOpenXREnabled = false; // The user sets this flag to true to request support for OpenXR
bool g_bUseOpenXR = false; //The systems sets this flag when an OpenXR is available and compatible
bool g_bOpenXRInitialized = false; //The systems sets this flag when OpenXR has been initialized correctly

const char* VRRendererOpenXR::ask_extensions[] = {
	XR_KHR_D3D11_ENABLE_EXTENSION_NAME, // Use Direct3D11 for rendering
	XR_EXT_DEBUG_UTILS_EXTENSION_NAME,  // Debug utils for extra info
};
std::vector<const char*> VRRendererOpenXR::use_extensions;

bool VRRendererOpenXR::is_available()
{
	// OpenXR will fail to initialize if we ask for an extension that OpenXR
	// can't provide! So we need to check our all extensions before 
	// initializing OpenXR with them. Note that even if the extension is 
	// present, it's still possible you may not be able to use it. For 
	// example: the hand tracking extension may be present, but the hand
	// sensor might not be plugged in or turned on. There are often 
	// additional checks that should be made before using certain features!


	// We'll get a list of extensions that OpenXR provides using this 
	// enumerate pattern. OpenXR often uses a two-call enumeration pattern 
	// where the first call will tell you how much memory to allocate, and
	// the second call will provide you with the actual data!
	uint32_t ext_count = 0;
	XrResult xrResult = XR_ERROR_RUNTIME_FAILURE;
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
	vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

	if (xrResult != XR_SUCCESS)
		return false;

	log_debug("[DBG][OpenXR] Extensions available");
	for (size_t i = 0; i < xr_exts.size(); i++) {
		log_debug("- %s\n", xr_exts[i].extensionName);
		// Check if we're asking for this extensions, and add it to our use 
		// list!
		for (int32_t ask = 0; ask < _countof(ask_extensions); ask++) {
			if (strcmp(ask_extensions[ask], xr_exts[i].extensionName) == 0) {
				use_extensions.push_back(ask_extensions[ask]);
				break;
			}
		}
	}
	// If a required extension isn't present, you want to ditch out here!
	// It's possible something like your rendering API might not be provided
	// by the active runtime. APIs like OpenGL don't have universal support.
	if (!any_of(use_extensions.begin(), use_extensions.end(),
		[](const char* ext) {
			return strcmp(ext, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0;
		}))
	{
		return false;
	}

	//If it didn't fail yet, we consider OpenXR is available
	return true;
		
}

bool VRRendererOpenXR::init(DeviceResources *deviceResources)
{
		XrResult xrResult = XR_SUCCESS;
		// Initialize OpenXR with the extensions we've found!
		XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
		createInfo.enabledExtensionCount = use_extensions.size();
		createInfo.enabledExtensionNames = use_extensions.data();
		createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
		strcpy_s(createInfo.applicationInfo.applicationName, "X-Wing Alliance VR");
		xrCreateInstance(&createInfo, &xr_instance);

		// Check if OpenXR is on this system, if this is null here, the user 
		// needs to install an OpenXR runtime and ensure it's active!
		if (&xr_instance == nullptr)
			return false;

		// Load extension methods that we'll need for this application! There's a
		// couple ways to do this, and this is a fairly manual one. Chek out this
		// file for another way to do it:
		// https://github.com/maluoi/StereoKit/blob/master/StereoKitC/systems/platform/openxr_extensions.h
		xrGetInstanceProcAddr(xr_instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrCreateDebugUtilsMessengerEXT));
		xrGetInstanceProcAddr(xr_instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&ext_xrDestroyDebugUtilsMessengerEXT));
		xrGetInstanceProcAddr(xr_instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&ext_xrGetD3D11GraphicsRequirementsKHR));

		// Request a form factor from the device (HMD, Handheld, etc.)
		XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
		systemInfo.formFactor = app_config_form;
		xrGetSystem(xr_instance, &systemInfo, &xr_system_id);

		/*
		// OpenXR wants to ensure apps are using the correct graphics card, so this MUST be called 
		// before xrCreateSession. This is crucial on devices that have multiple graphics cards, 
		// like laptops with integrated graphics chips in addition to dedicated graphics cards.
		XrGraphicsRequirementsD3D11KHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
		ext_xrGetD3D11GraphicsRequirementsKHR(xr_instance, xr_system_id, &requirement);
		if (!d3d_init(requirement.adapterLuid))
			return false;

		*/

		// A session represents this application's desire to display things! This is where we hook up our graphics API.
		// This does not start the session, for that, you'll need a call to xrBeginSession, which we do in openxr_poll_events
		XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
		binding.device = deviceResources->_d3dDevice;
		XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
		sessionInfo.next = &binding;
		sessionInfo.systemId = xr_system_id;
		if (XR_SUCCESS != xrCreateSession(xr_instance, &sessionInfo, &xr_session))
		{
			// Unable to start a session, may not have a VR device attached or ready
			log_debug("[DBG][OpenXR] Unable to create session");
			return false;
		}

		// OpenXR uses a couple different types of reference frames for positioning content, we need to choose one for
		// displaying our content! STAGE would be relative to the center of your guardian system's bounds, and LOCAL
		// would be relative to your device's starting location.
		XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
		ref_space.poseInReferenceSpace = xr_pose_identity;
		ref_space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		xrCreateReferenceSpace(xr_session, &ref_space, &xr_app_space);
}

void VRRendererOpenXR::RenderLeft(ID3D11Texture2D* LeftEyeBuffer)
{
}

void VRRendererOpenXR::RenderRight(ID3D11Texture2D* LeftEyeBuffer)
{
}

