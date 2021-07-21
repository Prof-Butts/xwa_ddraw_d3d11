#pragma once

#include "DeviceResources.h"
#include "Matrices.h"

enum VREye
{
	Eye_Left = 0,
	Eye_Right = 1
};

enum RenderType {
	OpenVR,
	OpenXR,
	DirectSBS
};

struct RenderProperties {
	float vFOV; //Total vertical FOV in degrees
	float hFOV; //Total horizontal FOV in degrees, to be used for rendering in XWA
	uint32_t width;
	uint32_t height;
	RenderType renderType;
};

// Abstract class to define a common interface for stereo renderers (DirectSBS, SteamVR, OpenXR...)
class StereoRenderer {
	public:		
		RenderProperties renderProperties;
		Matrix4 rotViewMatrix;	// Transformation matrix with only roll, to apply on ddraw (g_VSMatrixCB.viewMat)
		Matrix4 fullViewMatrix; // Traslation + rotation matrix to be applied on CockpitLook (except roll) (g_VSMatrixCB.fullViewMat)
		Matrix4 eyeMatrixLeft; // Translation + projection matrix for left eye (g_VSMatrixCB.projEye)
		Matrix4 eyeMatrixRight;// Translation + projection matrix for right eye (g_VSMatrixCB.projEye)
							   
		/*
		* Initialize all the necessary resources for the VR runtime. To be called only once.
		*/
		virtual bool init(DeviceResources* deviceResources) = 0;	

		/*
		* Verifies if the VR runtime is ready to start rendering frames
		* To be called once per frame, will poll for relevant events from VR runtime
		* Returns true when the application should start the frame loop and synchronization
		*/
		virtual bool is_ready() = 0;

		/*
		* Takes the final rendered texture (eyeBuffer) for vrEye eye and sends it to the VR API.
		* ImplicitEndFrame:
		*     true: Submit frame already launches the VR composition.
		*     false: a subsequent call to EndFrame is needed to launch the VR composition
		*/
		virtual void Submit(ID3D11DeviceContext* context, ID3D11Texture2D *eyeBuffer, VREye vrEye) = 0;

		/*
		* To be called each frame, to synchronize with the HMD display timing, just before the pose is needed
		* In XWA, this should probably be called at the end of the frame rendering, or in CockpitLook.
		* To be tested for best performance.
		*/
		virtual void WaitFrame() = 0;

		/*
		* To be called each frame, just before starting the GPU rendering work.
		* TODO: check if frame needs to be rendered and return the info.
		*/		
		virtual void BeginFrame() = 0;

		/*
		* To be called each frame to update rotViewMatrix, fullViewMatrix, eyeMatrixLeft, eyeMatrixRight
		* that will be used by CockpitLook and to render
		*/
		virtual void UpdateViewMatrices() = 0;

		/*
		* To be called after the frame is submitted to the HMD swapchain.
		* Is it better to call it before or after Present in the 2D monitor swapchain?
		*/
		virtual void EndFrame() = 0;

		virtual void ShutDown() = 0;
};