// Copyright (c) 2014 J�r�my Ansel
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
	bool Text2DAntiAlias;
	bool Geometry2DAntiAlias;
	bool MusicSyncFix;
	bool GammaFixEnabled;
};

extern Config g_config;
