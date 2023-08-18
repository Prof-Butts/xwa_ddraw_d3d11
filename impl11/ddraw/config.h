// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt

#pragma once

#include <string>

class Config
{
public:
	Config();

	bool AspectRatioPreserved;
	bool MultisamplingAntialiasingEnabled;
	int MSAACount;
	bool AnisotropicFilteringEnabled;
	bool VSyncEnabled;
	bool VSyncEnabledInHangar;
	bool WireframeFillMode;
	int JoystickEmul;
	bool SwapJoystickXZAxes;
	int XInputTriggerAsThrottle;
	bool InvertYAxis;
	bool InvertThrottle;
	float MouseSensitivity;
	float KbdSensitivity;

	float Concourse3DScale;

	int ProcessAffinityCore;

	bool D3dHookExists;
	std::wstring TextFontFamily;
	int TextWidthDelta;

	bool EnhanceLasers;
	bool EnhanceIllumination;
	bool EnhanceEngineGlow;
	bool EnhanceExplosions;

	bool FXAAEnabled;
	bool StayInHyperspace;
	bool TriangleTextEnabled;
	bool TrianglePointerEnabled;
	bool SimplifiedTrianglePointer;
	// Direct2D settings
	bool Text2DRendererEnabled;
	bool Radar2DRendererEnabled;
	bool D3dRendererHookEnabled;
	bool Text2DAntiAlias;
	bool Geometry2DAntiAlias;

	bool MusicSyncFix;
	bool HangarShadowsEnabled;
	bool EnableSoftHangarShadows;
	bool OnlyGrayscaleInTechRoom;
	bool TechRoomHolograms;
	bool CullBackFaces;
	bool FlipDATImages;

	bool HDConcourseEnabled;

	float ProjectionParameterA;
	float ProjectionParameterB;
	float ProjectionParameterC;

	std::string ScreenshotsDirectory;
};

extern Config g_config;
