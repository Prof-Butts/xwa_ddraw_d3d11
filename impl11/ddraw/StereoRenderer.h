#pragma once

#include "DeviceResources.h"

enum VREye
{
	Eye_Left = 0,
	Eye_Right = 1
};

// Abstract class to define a common interface for stereo renderers (DirectSBS, SteamVR, OpenXR...)
class StereoRenderer {
	public:		
		virtual bool init(DeviceResources* deviceResources) = 0;
		virtual void RenderLeft(ID3D11Texture2D *LeftEyeBuffer) = 0;
		virtual void RenderRight(ID3D11Texture2D *LeftEyeBuffer) = 0;
		virtual ~StereoRenderer() = 0;
	protected:
		virtual void Render(ID3D11Texture2D *buffer, VREye eye) = 0;
};