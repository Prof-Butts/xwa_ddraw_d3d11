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
	bool D3dRendererTexturesHookEnabled;
	bool Text2DAntiAlias;
	bool Geometry2DAntiAlias;

	bool MusicSyncFix;
	bool HangarShadowsEnabled;
	bool EnableSoftHangarShadows;
	bool OnlyGrayscale;
	bool TechRoomHolograms;
	bool CullBackFaces;
	bool FlipDATImages;

	bool HDConcourseEnabled;

	float ProjectionParameterA;
	float ProjectionParameterB;
	float ProjectionParameterC;

	float TechRoomMetallicity;
	float TechRoomAmbient;

	std::string ScreenshotsDirectory;
};

extern Config g_config;

class CraftConfig
{
public:
	CraftConfig();

	int Craft_Size;
	int Craft_Offset_2DF;
};

extern CraftConfig g_craftConfig;