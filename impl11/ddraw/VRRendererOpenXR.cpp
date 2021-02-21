#include "VRRendererOpenXR.h"
#include "EffectsCommon.h"
#include <algorithm> // any_of

using namespace std;

bool g_bOpenXREnabled = false; // The user sets this flag to true to request support for OpenXR
bool g_bUseOpenXR = false; //The systems sets this flag when an OpenXR is available and compatible
bool g_bOpenXRInitialized = false; //The systems sets this flag when OpenXR has been initialized correctly

const char* VRRendererOpenXR::ask_extensions[] = {
	XR_KHR_D3D11_ENABLE_EXTENSION_NAME, // Use Direct3D11 for rendering
	//XR_EXT_DEBUG_UTILS_EXTENSION_NAME,  // Debug utils for extra info
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
	if (xrResult != XR_SUCCESS)
		log_debug("[DBG][OpenXR] Unable to get the number of extensions available. Error %d", xrResult);
	return false;
	vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

	if (xrResult != XR_SUCCESS)
		log_debug("[DBG][OpenXR] Unable to get available Extensions from xrEnumerateInstanceExtensionProperties. Error %d", xrResult);
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
		log_debug("[DBG][OpenXR] One of the extensions is not available");
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

		// Check that the XR device is an HMD stereo device
		uint32_t view_count = 0;
		if (XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED == xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, 0, &view_count, nullptr))
		{
			log_debug("[DBG][OpenXR] OpenXR XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO not supported");
			return false;
		}
		xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, view_count, &view_count, xr_config_views.data());

		// TODO: Using texture array for better performance, so requiring left/right views have identical sizes.
		const XrViewConfigurationView& view = xr_config_views[0];
		if (xr_config_views[0].recommendedImageRectWidth != xr_config_views[1].recommendedImageRectWidth ||
			xr_config_views[0].recommendedImageRectHeight != xr_config_views[1].recommendedImageRectHeight ||
			xr_config_views[0].recommendedSwapchainSampleCount != xr_config_views[1].recommendedSwapchainSampleCount)
		{
			log_debug("[DBG][OpenXR] Left and right view configs are different");
			return false;
		}

		renderProperties.height = xr_config_views[0].recommendedImageRectHeight;
		renderProperties.width = xr_config_views[0].recommendedImageRectWidth;
		//FOV is not known at this time, only after calling XrLocateViews when inside the frame loop

		int64_t swapchain_format = BACKBUFFER_FORMAT;

		for (uint32_t i = 0; i < view_count; i++) {
			// Create a swapchain for this viewpoint!
			// OpenXR doesn't create a concrete image format for the texture, like DXGI_FORMAT_R8G8B8A8_UNORM.
			// Instead, it switches to the TYPELESS variant of the provided texture format, like 
			// DXGI_FORMAT_R8G8B8A8_TYPELESS. When creating an ID3D11RenderTargetView for the swapchain texture, we must specify
			// a concrete type like DXGI_FORMAT_R8G8B8A8_UNORM, as attempting to create a TYPELESS view will throw errors, so 
			// we do need to store the format separately and remember it later.
			XrViewConfigurationView& view = xr_config_views[i];
			XrSwapchainCreateInfo    swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
			XrSwapchain              handle;
			swapchain_info.arraySize = 1;
			swapchain_info.mipCount = 1;
			swapchain_info.faceCount = 1;
			swapchain_info.format = swapchain_format;
			swapchain_info.width = view.recommendedImageRectWidth;
			swapchain_info.height = view.recommendedImageRectHeight;
			swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
			swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
			xrCreateSwapchain(xr_session, &swapchain_info, &handle);

			// Find out how many textures were generated for the swapchain
			uint32_t surface_count = 0;
			xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

			swapchain_t swapchain = {};
			swapchain.width = swapchain_info.width;
			swapchain.height = swapchain_info.height;
			swapchain.handle = handle;
			swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });			
			xrEnumerateSwapchainImages(handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());

			if (i == 0)
				this->swapchainLeftEye = swapchain;
			else
				this->swapchainRightEye = swapchain;
		}
		return true;
}

void VRRendererOpenXR::WaitFrame()
{
	// Block until the next frame needs to start being rendered	
	xrWaitFrame(xr_session, nullptr, &frame_state);
}

void VRRendererOpenXR::UpdateViewMatrices()
{
	//TODO

	//To get the head pose locate the XR_REFERENCE_SPACE_TYPE_VIEW relative to XR_REFERENCE_SPACE_TYPE_LOCAL
	//xrLocateSpace();

	//This is the "head" pose relative to the workd space, to provide to XWA for rendering.
	rotViewMatrix.identity();
	fullViewMatrix.identity();
	//xrLocateViews relative to XR_REFERENCE_SPACE_TYPE_VIEW to get the head to eye transform.
	//Set eyeMatrixLeft, eyeMatrixRight;
	eyeMatrixLeft.identity();
	eyeMatrixRight.identity();
	//Set FOV
	renderProperties.vFOV = 105;
}

void VRRendererOpenXR::BeginFrame()
{
	xrBeginFrame(xr_session, nullptr);
}

void VRRendererOpenXR::Submit(ID3D11DeviceContext* context, ID3D11Texture2D* EyeBuffer, VREye vrEye, bool implicitEndFrame)
{
	XrCompositionLayerBaseHeader* layer = nullptr;
	XrCompositionLayerProjection             layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	vector<XrCompositionLayerProjectionView> views;

	//Acquire buffer from swapchain to copy the rendered texture on it
	uint32_t                    img_id;
	XrSwapchainImageAcquireInfo acquire_info = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
	swapchain_t* swapchain;
	if (vrEye == Eye_Left)
		swapchain = &swapchainLeftEye;
	else if (vrEye == Eye_Right)
		swapchain = &swapchainRightEye;
	xrAcquireSwapchainImage(swapchain->handle, &acquire_info, &img_id);

	// Wait until the image is available to render to. The compositor could still be
	// reading from it.
	XrSwapchainImageWaitInfo wait_info = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	wait_info.timeout = XR_INFINITE_DURATION;
	xrWaitSwapchainImage(swapchain->handle, &wait_info);

	//Resolve input buffer into the display swapchain buffer for the correct eye
	context->ResolveSubresource(swapchain->surface_images[img_id].texture,0,EyeBuffer,0,BACKBUFFER_FORMAT);

	//The swapchain buffer now contains the rendered content, we can release it
	xrReleaseSwapchainImage(swapchain->handle, nullptr);

	//If the calling app wants to immediately submit the frame, we do it here
	if (implicitEndFrame)
		EndFrame();
}

void VRRendererOpenXR::EndFrame()
{
	XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
	end_info.displayTime = frame_state.predictedDisplayTime;
	// Submit
	xrEndFrame(xr_session, &end_info);
}

void VRRendererOpenXR::ShutDown()
{
	xrDestroySwapchain(swapchainLeftEye.handle);
	xrDestroySwapchain(swapchainRightEye.handle);

	// Release all the other OpenXR resources that we've created!
	// What gets allocated, must get deallocated!

	if (xr_app_space != XR_NULL_HANDLE) xrDestroySpace(xr_app_space);
	if (xr_session != XR_NULL_HANDLE) xrDestroySession(xr_session);
	//if (xr_debug != XR_NULL_HANDLE) ext_xrDestroyDebugUtilsMessengerEXT(xr_debug);
	if (xr_instance != XR_NULL_HANDLE) xrDestroyInstance(xr_instance);
}
