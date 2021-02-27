#pragma once

#include "commonVR.h"

extern const float MAX_BRIGHTNESS;
#define MAX_BLOOM_PASSES 9
extern const float DEFAULT_RETICLE_SCALE;
extern const char* UV_COORDS_DCPARAM;
extern const char* COVER_TEX_NAME_DCPARAM;
extern const char* COVER_TEX_SIZE_DCPARAM;
extern const char* ERASE_REGION_DCPARAM;

// This is the current resolution of the screen:
extern float g_fLensK1;
extern float g_fLensK2;
extern float g_fLensK3;

// GUI elements seem to be in the range 0..0.0005, so 0.0008 sounds like a good threshold:
extern float g_fGUIElemPZThreshold ;
extern float g_fTrianglePointerDist ;
extern float g_fGlobalScale ;
//extern float g_fPostProjScale ;
extern float g_fGlobalScaleZoomOut ;
extern float g_fConcourseScale ;
extern float g_fConcourseAspectRatio ;
extern float g_fHUDDepth ; // The aiming HUD is rendered at this depth
extern bool g_bFloatingAimingHUD ; // The aiming HUD can be fixed to the cockpit glass or floating
extern float g_fTextDepth ; // All text gets rendered at this parallax
extern float g_fFloatingGUIDepth ; // Floating GUI elements are rendered at this depth
extern float g_fFloatingGUIObjDepth ; // The targeted object must be rendered above the Floating GUI
extern float g_fTechLibraryParallax ;
extern float g_fAspectRatio ;
extern bool g_bZoomOut ;
extern bool g_bZoomOutInitialState ;
extern float g_fBrightness ;
extern float g_fGUIElemsScale ; // Used to reduce the size of all the GUI elements
extern int g_iFreePIESlot ;
extern int g_iFreePIEControllerSlot ;
extern bool g_bFixedGUI ;
//extern float g_fXWAScale ; // This is the scale value as computed during Execute()

extern float g_fSSAOAlphaOfs;
extern bool g_bDisableDiffuse;
extern bool g_bProceduralLava;

extern float g_fBloomLayerMult[MAX_BLOOM_PASSES + 1];
extern float g_fBloomSpread[MAX_BLOOM_PASSES + 1];
extern int g_iBloomPasses[MAX_BLOOM_PASSES + 1];

extern bool g_bDynCockpitEnabled;

extern float g_fHangarAmbient, g_fGlobalAmbient;
extern float g_fSpecIntensity, g_fSpecBloomIntensity, g_fXWALightsSaturation, g_fXWALightsIntensity;
extern bool g_bApplyXWALightsIntensity;
extern float g_fHDRLightsMultiplier, g_fHDRWhitePoint;
extern bool g_bHDREnabled;
extern bool g_bEdgeDetectorEnabled;

extern bool g_bEnableVR;
extern bool g_bCockpitPZHackEnabled;
extern bool g_bOverrideAspectRatio;



// User-facing functions
void ResetVRParams(); // Restores default values for the view params
void SaveVRParams();
void LoadVRParams();
void LoadCockpitLookParams();
bool LoadGeneric3DCoords(char* buf, float* x, float* y, float* z);
bool LoadGeneric4DCoords(char* buf, float* x, float* y, float* z, float* w);

void ApplyFocalLength(float focal_length);
void SaveFocalLength();
bool LoadFocalLength();
void ComputeHyperFOVParams();

bool LoadDCParams();
bool LoadACParams();
bool LoadBloomParams();
bool LoadSSAOParams();
bool LoadHyperParams();
bool LoadDefaultGlobalMaterial();
void ReloadMaterials();
