// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt

#include "common.h"
#include "config.h"
#include "hook_config.h"

#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>

#include "joystick.h"

using namespace std;

Config g_config;

bool HookD3D_IsHookD3DEnabled()
{
	auto lines = GetFileLines("hook_d3d.cfg");

	if (lines.empty())
	{
		lines = GetFileLines("hooks.ini", "hook_d3d");
	}

	bool IsHookD3DEnabled = GetFileKeyValueInt(lines, "IsHookD3DEnabled", 1) != 0;

	return IsHookD3DEnabled;
}

Config::Config()
{
	this->AspectRatioPreserved = true;
	this->MultisamplingAntialiasingEnabled = false;
	this->MSAACount = -1;
	this->AnisotropicFilteringEnabled = true;
	this->VSyncEnabled = true;
	this->VSyncEnabledInHangar = true;
	this->WireframeFillMode = false;
	this->JoystickEmul = 0;

	this->XInputTriggerAsThrottle = 0;
	this->InvertYAxis = false;
	this->InvertThrottle = false;
	this->MouseSensitivity = 0.5f;
	this->KbdSensitivity = 1.0f;

	this->Concourse3DScale = 0.6f;

	this->ProcessAffinityCore = 0;

	this->D3dHookExists = false;

	this->TextFontFamily = L"Verdana";
	this->TextWidthDelta = 0;

	this->EnhanceLasers = false;
	this->EnhanceIllumination = false;
	this->EnhanceEngineGlow = false;

	this->SwapJoystickXZAxes = false;
	this->FXAAEnabled = false;
	this->StayInHyperspace = false;
	this->TriangleTextEnabled = true;
	this->TrianglePointerEnabled = true;
	this->SimplifiedTrianglePointer = false;
	this->Text2DRendererEnabled = true;
	this->Radar2DRendererEnabled = true;
	this->Text2DAntiAlias = true;
	this->Geometry2DAntiAlias = true;
	this->MusicSyncFix = false;

	this->D3dRendererHookEnabled = true;
	this->HangarShadowsEnabled = true;
	this->EnableSoftHangarShadows = true;
	this->OnlyGrayscaleInTechRoom = false;
	this->CullBackFaces = false;

	this->HDConcourseEnabled = false;

	this->ProjectionParameterA = 32.0f;
	this->ProjectionParameterB = 256.0f;
	this->ProjectionParameterC = 0.33f;

	CreateDirectory("Screenshots", nullptr);
	this->ScreenshotsDirectory = "Screenshots";

	if (ifstream("Hook_D3d.dll") && HookD3D_IsHookD3DEnabled())
	{
		this->D3dHookExists = true;
	}

	ifstream file("ddraw.cfg");

	if (file.is_open())
	{
		string line;

		while (std::getline(file, line))
		{
			if (!line.length())
			{
				continue;
			}

			if (line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/'))
			{
				continue;
			}

			int pos = line.find("=");

			string name = line.substr(0, pos);
			name.erase(remove_if(name.begin(), name.end(), std::isspace), name.end());

			string value = line.substr(pos + 1);
			value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));
			value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);

			if (!name.length() || !value.length())
			{
				continue;
			}

			if (name == "PreserveAspectRatio")
			{
				this->AspectRatioPreserved = stoi(value) != 0;
			}
			else if (name == "EnableMultisamplingAntialiasing")
			{
				this->MultisamplingAntialiasingEnabled = stoi(value) != 0;
			}
			else if (name == "MSAACount")
			{
				this->MSAACount = stoi(value);
			}
			else if (name == "EnableAnisotropicFiltering")
			{
				this->AnisotropicFilteringEnabled = stoi(value) != 0;
			}
			else if (name == "EnableVSync")
			{
				this->VSyncEnabled = stoi(value) != 0;
			}
			else if (name == "EnableVSyncInHangar")
			{
				this->VSyncEnabledInHangar = stoi(value) != 0;
			}
			else if (name == "FillWireframe")
			{
				this->WireframeFillMode = stoi(value) != 0;
			}
			else if (name == "Concourse3DScale")
			{
				this->Concourse3DScale = stof(value);
			}
			else if (name == "ProcessAffinityCore")
			{
				this->ProcessAffinityCore = stoi(value);
			}
			else if (name == "TextFontFamily")
			{
				this->TextFontFamily = string_towstring(value);
			}
			else if (name == "TextWidthDelta")
			{
				this->TextWidthDelta = stoi(value);
			}
			else if (name == "JoystickEmul")
			{
				this->JoystickEmul = stoi(value);
			}
			else if (name == "SwapJoystickXZAxes")
			{
				this->SwapJoystickXZAxes = (bool)stoi(value);
			}
			else if (name == "InvertThrottle")
			{
				this->InvertThrottle = (bool)stoi(value);
			}
			else if (name == "MouseSensitivity")
			{
				this->MouseSensitivity = stof(value);
			}
			else if (name == "KbdSensitivity")
			{
				this->KbdSensitivity = stof(value);
			}
			else if (name == "EnhanceLasers")
			{
				this->EnhanceLasers = (bool)stoi(value);
			}
			else if (name == "EnhanceIlluminationTextures")
			{
				this->EnhanceIllumination = (bool)stoi(value);
			}
			else if (name == "EnhanceEngineGlow")
			{
				this->EnhanceEngineGlow = (bool)stoi(value);
			}
			else if (name == "EnhanceExplosions")
			{
				this->EnhanceExplosions = (bool)stoi(value);
			}
			else if (name == "EnableFXAA")
			{
				this->FXAAEnabled = (bool)stoi(value);
			}
			else if (name == "StayInHyperspace")
			{
				this->StayInHyperspace = (bool)stoi(value);
			}
			else if (name == "TriangleTextEnabled")
			{
				this->TriangleTextEnabled = (bool)stoi(value);
			}
			else if (name == "TrianglePointerEnabled")
			{
				this->TrianglePointerEnabled = (bool)stoi(value);
			}
			else if (name == "SimplifiedTrianglePointer")
			{
				this->SimplifiedTrianglePointer = (bool)stoi(value);
			}
			else if (name == "Text2DRendererEnabled")
			{
				this->Text2DRendererEnabled = (bool)stoi(value);
			}
			else if (name == "Radar2DRendererEnabled") 
			{
				this->Radar2DRendererEnabled = (bool)stoi(value);
			}
			else if (name == "Text2DAntiAlias")
			{
				this->Text2DAntiAlias = (bool)stoi(value);
			}
			else if (name == "Geometry2DAntiAlias")
			{
				this->Geometry2DAntiAlias = (bool)stoi(value);
			}
			else if (name == "MusicSyncFix")
			{
				this->MusicSyncFix = (bool)stoi(value);
			}
			else if (name == "D3dRendererHookEnabled")
			{
				this->D3dRendererHookEnabled = stoi(value) != 0;
			}
			else if (name == "HangarShadowsEnabled")
			{
				this->HangarShadowsEnabled = stoi(value) != 0;
			}
			else if (name == "EnableSoftHangarShadows")
			{
				this->EnableSoftHangarShadows = stoi(value) != 0;
			}
			else if (name == "OnlyGrayscaleInTechRoom")
			{
				this->OnlyGrayscaleInTechRoom = stoi(value) != 0;
			}
			else if (name == "CullBackFaces")
			{
				this->CullBackFaces = stoi(value) != 0;
			}
			else if (name == "HDConcourseEnabled")
			{
				this->HDConcourseEnabled = stoi(value) != 0;
			}
			else if (name == "ProjectionParameterA")
			{
				this->ProjectionParameterA = stof(value);
			}
			else if (name == "ProjectionParameterB")
			{
				this->ProjectionParameterB = stof(value);
			}
			else if (name == "ProjectionParameterC")
			{
				this->ProjectionParameterC = stof(value);
			}
			else if (name == "ScreenshotsDirectory")
			{
				if (PathIsDirectory(value.c_str()))
				{
					this->ScreenshotsDirectory = value;
				}
			}
		}
	}

	if (this->D3dRendererHookEnabled && this->D3dHookExists)
	{
		this->D3dRendererHookEnabled = false;

		MessageBox(nullptr, "You must set [hook_d3d] IsHookD3DEnabled = 0 in Hooks.ini to use the D3d renderer hook.\nThe D3d renderer hook will be disabled.", "X-Wing Alliance DDraw", MB_ICONWARNING);
	}

	//if (this->JoystickEmul != 0 && isXWA)
	//if (this->JoystickEmul)
	{
		// TODO: How to check if this is a supported binary?
		DWORD old, dummy;
		VirtualProtect((void *)0x5a92a4, 0x5a92b8 - 0x5a92a4, PAGE_READWRITE, &old);
		*(unsigned *)0x5a92a4 = reinterpret_cast<unsigned>(emulJoyGetPosEx);
		*(unsigned *)0x5a92a8 = reinterpret_cast<unsigned>(emulJoyGetDevCaps);
		*(unsigned *)0x5a92b4 = reinterpret_cast<unsigned>(emulJoyGetNumDevs);
		VirtualProtect((void *)0x5a92a4, 0x5a92b8 - 0x5a92a4, old, &dummy);
	}

	if (this->ProcessAffinityCore > 0)
	{
		HANDLE process = GetCurrentProcess();
		//HANDLE process = GetCurrentThread();

		DWORD_PTR processAffinityMask;
		DWORD_PTR systemAffinityMask;

		if (GetProcessAffinityMask(process, &processAffinityMask, &systemAffinityMask))
		{
			int core = this->ProcessAffinityCore;
			DWORD_PTR mask = 0x01;

			for (int bit = 0, currentCore = 1; bit < 64; bit++)
			{
				if (mask & processAffinityMask)
				{
					if (currentCore != core)
					{
						processAffinityMask &= ~mask;
					}

					currentCore++;
				}

				mask = mask << 1;
			}

			SetProcessAffinityMask(process, processAffinityMask);
			//SetThreadAffinityMask(process, processAffinityMask);
		}
	}

	DisableProcessWindowsGhosting();
}
