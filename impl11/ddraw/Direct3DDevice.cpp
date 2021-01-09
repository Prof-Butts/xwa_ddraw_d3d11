// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes, 2019

// Shaders by Marty McFly (used with permission from the author)
// https://github.com/martymcmodding/qUINT/tree/master/Shaders

// _deviceResources->_backbufferWidth, _backbufferHeight: 3240, 2160 -- Screen/Backbuffer Resolution
// resources->_displayWidth, resources->_displayHeight -- in-game resolution
// g_WindowWidth, g_WindowHeight --> Actual Windows screen as returned by GetWindowRect

/*

Jeremy: @blue_max An unsigned short at offset 0x00F0 in the XwaCraft struct.

In function L00473D00:
// L00473D00
void UpdateHUDText(  )

	Ptr<XwaCraft> ebp20 = 0;

	int eax0 = s_XwaPlayers[s_XwaCurrentPlayerId].ObjectIndex;

	if( eax0 != 0xFFFF )
	{
		Ptr<XwaMobileObject> eax1 = s_XwaObjects[eax0].pMobileObject;

		if( eax1 != 0 )
		{
			ebp20 = eax1->pCraft;
		}
	}

...

	if( (ebp20->XwaCraft_m181 & 64) != 0 )
	{
		unsigned short edi = ebp20->PresetThrottle / 0x28F;

		if( ebp20->IsSlamEnabled != 0 )
		{
			edi *= 2;
		}
	}

*/

/*
Jeremy: In the player struct, it seems there are 12 unused bytes at offset 0x0B9 and 6 unused
		bytes at offset 0x0E2.

 0x0B9 byte unk0B9[12];
 0x0E2 byte unk0E2[6];
*/

/*
TODO:
	[5128] [DBG] s_XwaGlobalLightsCount: 0 <-- When the reactor explodes.
	DSReactorCylinder is the glow around the reactor core in the Death Star.

	Hotkeys to adjust the aspect ratio for the SteamVR mirror window.

	The speed effect doesn't work in the external camera anymore.

	Sun flares are still not right in SBS mode.

	What's wrong with the map in VR?

	Fixed, To Verify (Check again in SteamVR mode):
		Fix the FOV in the mirror Window in SteamVR -- approximate fix

	Auto-turn on headlights in the last mission.

	Automatic Eye Adaptation
	Tonemapping/Whiteout in HDR mode
*/

/*

HOW TO ADD NEW DC SOURCE ELEMENTS:

LoadDCInternalCoordinates loads the source areas into g_DCElemSrcBoxes.
g_DCElemSrcBoxes must be pre-populated.

1. Add new constant indices in DeviceResources.h.
   Look for MAX_DC_SRC_ELEMENTS and increase it. Then add the new *_DC_ELEM_SRC_IDX
   constants above it.

2. Add the new slots in Dynamic_Cockpit_Internal_Areas.cfg.
   Select a suitable *_HUD_BOX_IDX entry, find the corresponding image in HUD.dat and use
   a program like Photoshop to write down coordinates in this image. Then add an entry
   in Dynamic_Cockpit_Internal_Areas.cfg with the coordinates you wish to capture.
   Each entry looks like this:

	source_def = width,height, x0,y0, x1,y1

   Remember that you can use negative numbers for (x0,y0) and (x1,y1)

3. Add the code that captures the new slots in Direct3DDevice.cpp.
   Find the HUD_BOX_IDX entry in the Execute() method. The code should look like
   this:

   if (!g_DCHUDRegions.boxes[SHIELDS_HUD_BOX_IDX].bLimitsComputed)
   ...

   Add the code to capture the new slot(s) in the case where it belongs. There
   are plenty of examples in this area, just take a look at other cases.

4. Go to g_DCElemSrcNames in DeviceResources.cpp and add the labels that will
   be used for the new slots in the DC files. This is the text that cockpit
   authors will use to load the element.

   MAKE SURE YOU PLACE THE NEW LABELS ON THE SAME INDEX USED FOR THE *_DC_ELEM_SRC_IDX


HOW TO ADD NEW ERASE REGION COMMANDS:

1. Go to DeviceResources.h and add the new region indices.
   Add new *_HUD_BOX_IDX indices and make sure MAX_HUD_BOXES has the total
   number of boxes.

2. Add the regions in Dynamic_Cockpit_areas.cfg. Make sure the indices match with the
   constants added in step 1.

3. Add the new region names in g_HUDRegionNames, in DeviceResources.cpp.
   Make sure the indices match. These names will be used in DC files to erase the
   new regions.

4. Add the code that computes the erase_region in Direct3DDevice.cpp.
   Look for the calls to ComputeCoordsFromUV()

*/

/*
	The current HUD color is stored in these variables:

	// V0x005B5318
	unsigned int s_XwaFlightHudColor;

	// V0x005B531C
	unsigned int s_XwaFlightHudBorderColor;
*/

/*

********************************************************************************************

@JeremyaFr Hey Jeremy, do you know if there's a way to tell which craft is currently the focus of the external camera?
@blue_max Hello, you can read the int value at offset 0B54 in the player struct and compare it with the player object index

********************************************************************************************

SLAM SYSTEM:

There is a short value at offset 0xF6 (short IsSlamEnabled) in the craft struct.
When it is 0, SLAM is not activated. When it is 1, SLAM is activated.
You can access to the variable via the object struct: object->pMobileObject->pCraft->IsSlamEnabled

********************************************************************************************

XWA's DEPTH BUFFER CODE:
From: https://xwaupgrade.com/phpBB3/viewtopic.php?f=16&t=12462

	float st0 = A8[A4].Position.z;

	if( st0 < 0.0f )
	{
		st0 = s_V0x08B94CC;
	}

	float st1 = ( st0 * s_V0x05B46B4 ) / ( st0 * s_V0x05B46B4 + s_V0x08B94CC );

	if( st1 < 1.52590219E-05f )
	{
		st1 = 1.52590219E-05f;
	}

	s_V0x064D1A8[s_V0x06628E0].z = st1;
	s_V0x064D1A8[s_V0x06628E0].rhw = st0;

  <Patch Name="[WIP] depth fix: s_V0x05B46B4 = 32.0f">
	<Item Offset="0415C0" From="8B442404A3B4465B00C390" To="C705B4465B0000000042C3" />
	<Item Offset="0415D0" From="C705B4465B0000000045C3" To="C705B4465B0000000042C3" />
  </Patch>

  The patch sets the value 32.0f to s_V0x05B46B4 and it's included in the opt limit hook.

********************************************************************************************

https://xwaupgrade.com/phpBB3/viewtopic.php?f=9&t=12436

Justagai:

The cockpit camera position is in the CraftDefinition struct/table. Table starts at 0x5BB480
and offsets (2 bytes) 0x238, 0x23A and 0x23C for the cockpit Y, Z and X positions. This is per
craft by the way and that's how MXVTED read/writes them.

EDIT: Actually I suspect the player's current cockpit position might be stored on the 
PlayerDataTable in offsets (4 bytes) 0x201, 0x205 and 0x209. But I'm not entirely sure.

Jeremy:

The cockpit position is stored in offsets 0x20D, 0x211, 0x215 (of the PlayerDataTable)
After it has been transformed with the transform matrix in the MobileObject struct (offset 0xC7),
the transformed position is stored in offsets 0x201, 0x205, 0x209.

X-Wing POV (current): X: 0, Z: 37: Y: -31


************************************************************************************************************
From: https://xwaupgrade.com/phpBB3/viewtopic.php?f=9&t=12359&start=75

FOV

For 1920x1080:

HUD scale:
value: 1080 / 600.0f = 1.8

FOV:
value: 1080 * 1.0666f + 0.5f = 1152
angle: atan(1080 / 1152) * 2 / pi * 180 = 86.3°

focal_length = in-game-height * 1.0666f + 0.5f
angle = atan(in-game-height / focal_length) * 2

The formula for the angle becomes:
	focal_length = in-game-height / tan(angle / 2)

This is my default focal_length for 2560x1920 in-game: 1600x1200:
	focal_length = 1280.0
The formula above yields 1280.42, close enough
	angle = atan(1200 / 1280) * 2 = 86.3


The above becomes:
in-game-height / focal_length = tan(angle / 2.0)
focal_length = in-game-height / tan(angle / 2.0)

************************************************************************************************************

HANGAR

You can get the hangar object index via this variable:

// V0x009C6E38
dword& s_V0x09C6E38 = *(dword*)0x009C6E38;
When the value is different of 0xFFFF, the player craft is in a hangar.

With that index, you can get the object, mobile object and craft structs. From that, you can get the orientation of the mothership.
You can get the hyperspace phase with the craft state offset (00B).

					Ptr<XwaCraft> ebp = s_XwaObjects[s_V0x09C6E38].pMobileObject->pCraft;

					XwaGetCraftIndex( s_XwaObjects[s_V0x09C6E38].ModelIndex );

					strcpy_s( esp18, "" );

					if( ebp->XwaCraft_m00B == 0x05 )
					{
						// "%s %s"
						sprintf( esp18, "%s %s", s_XwaTieFlightGroups[s_XwaObjects[s_V0x09C6E38].TieFlightGroupIndex].FlightGroup.Name, s_StringsHangarAdditional[StringsHangarAdditional_ENTERING_HYPERSPACE] );
					}
					else if( ebp->XwaCraft_m00B == 0x06 )
					{
						// "%s %s"
						sprintf( esp18, "%s %s", s_XwaTieFlightGroups[s_XwaObjects[s_V0x09C6E38].TieFlightGroupIndex].FlightGroup.Name, s_StringsHangarAdditional[StringsHangarAdditional_LEAVING_HYPERSPACE] );
					}
					else if( ebp->XwaCraft_m00B == 0x07 )
					{
						// "%s %s"
						sprintf( esp18, "%s %s", s_XwaTieFlightGroups[s_XwaObjects[s_V0x09C6E38].TieFlightGroupIndex].FlightGroup.Name, s_StringsHangarAdditional[StringsHangarAdditional_IN_HYPERSPACE] );
					}

************************************************************************************************************

The HUD scale is stored here:
// V0x006002B8
float s_XwaHudScale = 1.0f;

	The variable that defines if the HUDs are visible is a byte situated at offset 0x0064 in the player table.

		s_XwaPlayers[playerIndex].IsHudVisible
		0x0064  byte IsHudVisible;

	The variable that defines if the left MFD is visible is a byte situated at offset 0x0067 in the player table.

		!KEY_DELETE!169 Delete Toggle left MFD
		Key_DELETE
		s_XwaPlayers[playerIndex].IsHudMfd1Visible
		0x0067 byte IsHudMfd1Visible;

	The variable that defines if the right MFD is visible is a byte situated at offset 0x0068 in the player table.

		!KEY_PAGEDOWN!173 PageDown Toggle right MFD
		Key_NEXT
		s_XwaPlayers[playerIndex].IsHudMfd2Visible
		0x0068 byte IsHudMfd2Visible;

	The variable that defines if the left sensor / indicator is visible is a byte situated at offset 0x005B5338.

		!KEY_INSERT!168 Insert Toggle Left Sensor / Shield Indicator
		Key_INSERT
		byte s_V0x05B5338 = (byte)0x01;

	The variable that defines if the right sensor / indicator is visible is a byte situated at offset 0x005B533C.

		!KEY_PAGEUP!172 PageUp Toggle Right Sensor / Beam Indicator
		Key_PRIOR
		byte s_V0x05B533C = (byte)0x01;

	The variable that defines if the center indicators are visible is a byte situated at offset 0x005B5340.

		!KEY_HOME!170 Home Center indicators
		Key_HOME
		byte s_V0x05B5340 = (byte)0x01;

	The variable that defines if the toggle CMD is visible is a byte situated at offset 0x005B5334.

		!KEY_END!171 End Toggle CMD
		Key_END
		byte s_V0x05B5334 = (byte)0x01;
*/

#include "common.h"
#include "..\shaders\material_defs.h"
#include "DeviceResources.h"
#include "Direct3DDevice.h"
#include "Direct3DExecuteBuffer.h"
#include "Direct3DTexture.h"
#include "BackbufferSurface.h"
#include "ExecuteBufferDumper.h"
#include "PrimarySurface.h"
// TODO: Remove later
#include "TextureSurface.h"

#include <ScreenGrab.h>
#include <WICTextureLoader.h>
#include <wincodec.h>
#include <vector>
//#include <assert.h>

#include "FreePIE.h"
#include <headers/openvr.h>
#include "Matrices.h"

#include "XWAObject.h"

#include "..\shaders\shader_common.h"

#define DBG_MAX_PRESENT_LOGS 0

#include "SharedMem.h"
SharedData *g_pSharedData = NULL;

FILE *g_HackFile = NULL;

LARGE_INTEGER g_PC_Frequency;

int *s_XwaGlobalLightsCount = (int*)0x00782848;
XwaGlobalLight* s_XwaGlobalLights = (XwaGlobalLight*)0x007D4FA0;

extern ObjectEntry **objects;
PlayerDataEntry *PlayerDataTable = (PlayerDataEntry *)0x8B94E0;
uint32_t *g_playerInHangar = (uint32_t *)0x09C6E40;
uint32_t *g_playerIndex = (uint32_t *)0x8C1CC8;
const auto numberOfPlayersInGame = (int*)0x910DEC;
// The current HUD color
uint32_t *g_XwaFlightHudColor = (uint32_t *)0x005B5318;
// The current HUD border color
uint32_t *g_XwaFlightHudBorderColor = (uint32_t *)0x005B531C;

const float DEG2RAD = 3.141593f / 180.0f;
FOVtype g_CurrentFOVType = GLOBAL_FOV;

// xwahacker computes the FOV like this: FOV = 2.0 * atan(height/focal_length). This formula is questionable, the actual
// FOV seems to be: 2.0 * atan((height/2)/focal_length), same for the horizontal FOV. I confirmed this by geometry
// and by computing the angle between the lights and the current forward point.
// Data provided by keiranhalcyon7:
uint32_t *g_rawFOVDist = (uint32_t *)0x91AB6C; // raw FOV dist(dword int), copy of one of the six values hard-coded with the resolution slots, which are what xwahacker edits
float *g_fRawFOVDist   = (float *)0x8B94CC; // FOV dist(float), same value as above
float *g_cachedFOVDist = (float *)0x8B94BC; // cached FOV dist / 512.0 (float), seems to be used for some sprite processing
float g_fDefaultFOVDist = 1280.0f; // Original FOV dist
// Global y_center and FOVscale parameters. These are updated only in ComputeHyperFOVParams.
float g_fYCenter = 0.0f, g_fFOVscale = 0.75f;
Vector2 g_ReticleCentroid(-1.0f, -1.0f);
bool g_bTriggerReticleCapture = false, g_bYCenterHasBeenFixed = false;

float g_fRealHorzFOV = 0.0f; // The real Horizontal FOV, in radians
float g_fRealVertFOV = 0.0f; // The real Vertical FOV, in radians
#define PSVR_VERT_FOV 106.53f
float RealVertFOVToRawFocalLength(float real_FOV);
void ApplyFocalLength(float focal_length);
void SaveFocalLength();

void GetCraftViewMatrix(Matrix4 *result);

inline void backProjectMetric(float sx, float sy, float rhw, Vector3 *P);
inline void backProjectMetric(UINT index, Vector3 *P);
inline Vector3 projectMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR = false);
inline Vector3 projectToInGameOrPostProcCoordsMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR = false);

float g_fVR_FOV = PSVR_VERT_FOV;
float g_fCurrentShipFocalLength = 0.0f; // Gets populated from the current DC "xwahacker_fov" file (if one is provided).
float g_fCurrentShipLargeFocalLength = 0.0f; // Gets populated from the current "xwahacker_large_fov" DC file (if one is provided).
bool g_bCustomFOVApplied = false;  // Becomes true in PrimarySurface::Flip once the custom FOV has been applied. Reset to false in DeviceResources::OnSizeChanged
bool g_bLastFrameWasExterior = false; // Keeps track of the state of the exterior camera on the last frame
/*
The current ship-heading + viewmatrix transform. Transforms from XWA's 3D (forward = Y+) into Shader system (forward = Z+)
Used to transform XWA's lights into shader coord system.
Computed at the beginning of each frame, inside BeginScene(), by calling GetCurrentHeadingViewMatrix()
*/
Matrix4 g_CurrentHeadingViewMatrix;

#define MOVE_LIGHTS_KEY_SET 1
#define CHANGE_FOV_KEY_SET 2
#define MOVE_POINT_LIGHT_KEY_SET 3
int g_KeySet = CHANGE_FOV_KEY_SET; // Default setting: let users adjust the FOV, I can always override this with the "key_set" SSAO.cfg param

// This value (2.0f) was determined experimentally. It provides an almost 1:1 metric reconstruction when compared with the original models
//const float DEFAULT_FOCAL_DIST = 2.0f; 
const float DEFAULT_IPD = 6.5f; // Ignored in SteamVR mode.
const float DEFAULT_METRIC_MULT = 1.0f;

const float DEFAULT_HUD_PARALLAX = 1.7f;
const float DEFAULT_TEXT_PARALLAX = 0.45f;
const float DEFAULT_FLOATING_GUI_PARALLAX = 0.450f;
const float DEFAULT_FLOATING_OBJ_PARALLAX = -0.025f;

const float DEFAULT_TECH_LIB_PARALLAX = -2.0f;
const float DEFAULT_TRIANGLE_POINTER_DIST = 0.120f;
const float DEFAULT_GUI_ELEM_PZ_THRESHOLD = 0.0008f;
const float DEFAULT_ZOOM_OUT_SCALE = 0.5f;
const bool DEFAULT_ZOOM_OUT_INITIAL_STATE = false;
//const float DEFAULT_ASPECT_RATIO = 1.33f;
const float DEFAULT_ASPECT_RATIO = 1.25f;
//const float DEFAULT_CONCOURSE_SCALE = 0.4f;
const float DEFAULT_CONCOURSE_SCALE = 12.0f;
//const float DEFAULT_CONCOURSE_ASPECT_RATIO = 2.0f; // Default for non-SteamVR
const float DEFAULT_CONCOURSE_ASPECT_RATIO = 1.33f; // Default for non-SteamVR
const float DEFAULT_GLOBAL_SCALE = 1.0f;
const float DEFAULT_GUI_SCALE = 0.7f;
/*
const float DEFAULT_LENS_K1 = 2.0f;
const float DEFAULT_LENS_K2 = 0.22f;
const float DEFAULT_LENS_K3 = 0.0f;
*/
const float DEFAULT_LENS_K1 =  3.80f;
const float DEFAULT_LENS_K2 = -0.28f;
const float DEFAULT_LENS_K3 =  100.0f;

//const float DEFAULT_COCKPIT_PZ_THRESHOLD = 0.166f; // I used 0.13f for a long time until I jumped on a TIE-Interceptor
const float DEFAULT_COCKPIT_PZ_THRESHOLD = 10.0f; // De-activated
const int DEFAULT_SKYBOX_INDEX = 2;
const bool DEFAULT_INTERLEAVED_REPROJECTION = false;
const bool DEFAULT_BARREL_EFFECT_STATE = true;
const bool DEFAULT_BARREL_EFFECT_STATE_STEAMVR = false; // SteamVR provides its own lens correction, only enable it if the user really wants it
const float DEFAULT_BRIGHTNESS = 0.95f;
const float MAX_BRIGHTNESS = 1.0f;
const bool DEFAULT_FLOATING_AIMING_HUD = true;
const int DEFAULT_NATURAL_CONCOURSE_ANIM = 1;
const bool DEFAULT_DYNAMIC_COCKPIT_ENABLED = false;
const bool DEFAULT_FIXED_GUI_STATE = true;
// 6dof
const int DEFAULT_FREEPIE_SLOT = 0;
const float DEFAULT_ROLL_MULTIPLIER =  -1.0f;
const float DEFAULT_POS_X_MULTIPLIER =  1.0f;
const float DEFAULT_POS_Y_MULTIPLIER =  1.0f;
const float DEFAULT_POS_Z_MULTIPLIER = -1.0f;
const float DEFAULT_MIN_POS_X = -1.5f;
const float DEFAULT_MAX_POS_X =  1.5f;
const float DEFAULT_MIN_POS_Y = -1.5f;
const float DEFAULT_MAX_POS_Y =  1.5f;
const float DEFAULT_MIN_POS_Z = -0.1f;
const float DEFAULT_MAX_POS_Z =  2.5f;
const bool DEFAULT_STEAMVR_POS_FROM_FREEPIE = false;
const bool DEFAULT_RESHADE_ENABLED_STATE = false;
const bool DEFAULT_BLOOM_ENABLED_STATE = false;
// TODO: Make this toggleable later
const bool DEFAULT_AO_ENABLED_STATE = false;
// cockpit look constants
const float DEFAULT_YAW_MULTIPLIER   = 1.0f;
const float DEFAULT_PITCH_MULTIPLIER = 1.0f;
const float DEFAULT_YAW_OFFSET   = 0.0f;
const float DEFAULT_PITCH_OFFSET = 0.0f;
const float DEFAULT_RETICLE_SCALE = 0.8f;

const char *FOCAL_DIST_VRPARAM = "focal_dist";
const char *IPD_VRPARAM = "IPD";
//const char *METRIC_MULT_VRPARAM = "stereoscopy_multiplier";
//const char *SIZE_3D_WINDOW_VRPARAM = "3d_window_size";
const char *SIZE_3D_WINDOW_ZOOM_OUT_VRPARAM = "3d_window_zoom_out_size";
const char *WINDOW_ZOOM_OUT_INITIAL_STATE_VRPARAM = "zoomed_out_on_startup";
const char *CONCOURSE_WINDOW_SCALE_VRPARAM = "concourse_window_scale";
const char *COCKPIT_Z_THRESHOLD_VRPARAM = "cockpit_z_threshold";
//const char *ASPECT_RATIO_VRPARAM = "3d_aspect_ratio";
const char *CONCOURSE_ASPECT_RATIO_VRPARAM = "concourse_aspect_ratio";
const char *K1_VRPARAM = "k1";
const char *K2_VRPARAM = "k2";
const char *K3_VRPARAM = "k3";
const char *HUD_PARALLAX_VRPARAM = "HUD_depth";
const char *GUI_PARALLAX_VRPARAM = "GUI_depth";
const char *GUI_OBJ_PARALLAX_VRPARAM = "GUI_target_relative_depth";
const char *TEXT_PARALLAX_VRPARAM = "Text_depth";
const char *TECH_LIB_PARALLAX_VRPARAM = "Tech_Library_relative_depth";
const char *BRIGHTNESS_VRPARAM = "brightness";
const char *VR_MODE_VRPARAM = "VR_Mode"; // Select "None", "DirectSBS" or "SteamVR"
const char *VR_MODE_NONE_SVAL = "None";
const char *VR_MODE_DIRECT_SBS_SVAL = "DirectSBS";
const char *VR_MODE_STEAMVR_SVAL = "SteamVR";
const char *INTERLEAVED_REPROJ_VRPARAM = "SteamVR_Interleaved_Reprojection";
const char *STEAMVR_DISTORTION_ENABLED_VRPARAM = "steamvr_distortion_enabled";
const char *BARREL_EFFECT_STATE_VRPARAM = "apply_lens_correction";
const char *INVERSE_TRANSPOSE_VRPARAM = "alternate_steamvr_eye_inverse";
const char *FLOATING_AIMING_HUD_VRPARAM = "floating_aiming_HUD";
const char *NATURAL_CONCOURSE_ANIM_VRPARAM = "concourse_animations_at_25fps";
const char *DYNAMIC_COCKPIT_ENABLED_VRPARAM = "dynamic_cockpit_enabled";
const char *FIXED_GUI_VRPARAM = "fixed_GUI";
const char *STICKY_ARROW_KEYS_VRPARAM = "sticky_arrow_keys";
const char *RETICLE_SCALE_VRPARAM = "reticle_scale";
const char *TRIANGLE_POINTER_DIST_VRPARAM = "triangle_pointer_distance";
// 6dof vrparams
const char *ROLL_MULTIPLIER_VRPARAM = "roll_multiplier";
const char *FREEPIE_SLOT_VRPARAM = "freepie_slot";
const char *STEAMVR_POS_FROM_FREEPIE_VRPARAM = "steamvr_pos_from_freepie";
// Cockpitlook params
const char *YAW_MULTIPLIER_CLPARAM   = "yaw_multiplier";
const char *PITCH_MULTIPLIER_CLPARAM = "pitch_multiplier";
const char *YAW_OFFSET_CLPARAM       = "yaw_offset";
const char *PITCH_OFFSET_CLPARAM     = "pitch_offset";

// Dynamic Cockpit vrparams
const char *UV_COORDS_DCPARAM			= "uv_coords";
const char *COVER_TEX_NAME_DCPARAM		= "cover_texture";
const char *COVER_TEX_SIZE_DCPARAM		= "cover_texture_size";
const char *ERASE_REGION_DCPARAM		= "erase_region";
const char *MOVE_REGION_DCPARAM			= "move_region";
const char *CT_BRIGHTNESS_DCPARAM		= "cover_texture_brightness";
const char *DC_TARGET_COMP_UV_COORDS_VRPARAM   = "dc_target_comp_uv_coords";
const char *DC_LEFT_RADAR_UV_COORDS_VRPARAM    = "dc_left_radar_uv_coords";
const char *DC_RIGHT_RADAR_UV_COORDS_VRPARAM   = "dc_right_radar_uv_coords";
const char *DC_SHIELDS_PANEL_UV_COORDS_VRPARAM = "dc_shields_panel_uv_coords";
const char *DC_LASERS_PANEL_UV_COORDS_VRPARAM  = "dc_lasers_panel_uv_coords";
const char *DC_FRONT_PANEL_UV_COORDS_VRPARAM   = "dc_front_panel_uv_coords";

/*
typedef enum {
	VR_MODE_NONE,
	VR_MODE_DIRECT_SBS,
	VR_MODE_STEAMVR
} VRModeEnum;
VRModeEnum g_VRMode = VR_MODE_DIRECT_SBS;
*/

// ***************************************************************
// SteamVR HMD
vr::IVRSystem *g_pHMD = NULL;
vr::IVRCompositor *g_pVRCompositor = NULL;
vr::IVRScreenshots *g_pVRScreenshots = NULL;
vr::TrackedDevicePose_t g_rTrackedDevicePose;
uint32_t g_steamVRWidth = 0, g_steamVRHeight = 0; // The resolution recommended by SteamVR is stored here
bool g_bSteamVREnabled = false; // The user sets this flag to true to request support for SteamVR.
bool g_bSteamVRInitialized = false; // The system will set this flag after SteamVR has been initialized
bool g_bUseSteamVR = false; // The system will set this flag if the user requested SteamVR and SteamVR was initialized properly
bool g_bInterleavedReprojection = DEFAULT_INTERLEAVED_REPROJECTION;
bool g_bResetHeadCenter = true; // Reset the head center on startup
bool g_bSteamVRDistortionEnabled = true, g_bSteamVRYawPitchRollFromMouseLook = false;
vr::HmdMatrix34_t g_EyeMatrixLeft, g_EyeMatrixRight;
Matrix4 g_EyeMatrixLeftInv, g_EyeMatrixRightInv;
Matrix4 g_projLeft, g_projRight;
Matrix4 g_FullProjMatrixLeft, g_FullProjMatrixRight, g_viewMatrix;
//float g_fMetricMult = DEFAULT_METRIC_MULT, 
float g_fFrameTimeRemaining = 0.005f;
int g_iSteamVR_Remaining_ms = 3, g_iSteamVR_VSync_ms = 11;
float g_fSteamVRMirrorWindow3DScale = 0.7f, g_fSteamVRMirrorWindowAspectRatio = 0.0f;
// Set to true in PrimarySurface Present 2D (Flip)
extern bool g_bInTechRoom;

bool g_bExternalHUDEnabled = false, g_bEdgeDetectorEnabled = true, g_bStarDebugEnabled = false;

// METRIC 3D RECONSTRUCTION
// The following values were determined by comparing the back-projected 3D reconstructed
// with ddraw against the OBJ exported from the OPT. The values were tweaked until a
// proper match was found.
float g_fOBJ_Z_MetricMult = 44.72f, g_fOBJGlobalMetricMult = 1.432f, g_fOBJCurMetricScale;

float g_f2DYawMul = 1.0f, g_f2DPitchMul = 1.0f, g_f2DRollMul = 1.0f;

int g_iNaturalConcourseAnimations = DEFAULT_NATURAL_CONCOURSE_ANIM;
bool g_bDynCockpitEnabled = DEFAULT_DYNAMIC_COCKPIT_ENABLED;
float g_fYawMultiplier   = DEFAULT_YAW_MULTIPLIER;
float g_fPitchMultiplier = DEFAULT_PITCH_MULTIPLIER;
float g_fRollMultiplier  = DEFAULT_ROLL_MULTIPLIER;
float g_fYawOffset   = DEFAULT_YAW_OFFSET;
float g_fPitchOffset = DEFAULT_PITCH_OFFSET;
float g_fPosXMultiplier  = DEFAULT_POS_X_MULTIPLIER;
float g_fPosYMultiplier  = DEFAULT_POS_Y_MULTIPLIER;
float g_fPosZMultiplier  = DEFAULT_POS_Z_MULTIPLIER;
float g_fMinPositionX = DEFAULT_MIN_POS_X, g_fMaxPositionX = DEFAULT_MAX_POS_X;
float g_fMinPositionY = DEFAULT_MIN_POS_Y, g_fMaxPositionY = DEFAULT_MAX_POS_Y;
float g_fMinPositionZ = DEFAULT_MIN_POS_Z, g_fMaxPositionZ = DEFAULT_MAX_POS_Z;
bool g_bStickyArrowKeys = false, g_bYawPitchFromMouseOverride = false;
Vector3 g_headCenter; // The head's center: this value should be re-calibrated whenever we set the headset
void projectSteamVR(float X, float Y, float Z, vr::EVREye eye, float &x, float &y, float &z);

/* Vertices that will be used for the VertexBuffer. */
D3DTLVERTEX *g_OrigVerts = NULL;
/* Indices used when D3D hook is present */
uint32_t *g_OrigIndex = NULL;

// Counter for calls to DrawIndexed() (This helps us know where were are in the rendering process)
// Gets reset everytime the backbuffer is presented and is increased only after BOTH the left and
// right images have been rendered.
int g_iDrawCounter = 0, g_iNoDrawBeforeIndex = 0, g_iNoDrawAfterIndex = -1, g_iDrawCounterAfterHUD = -1;
// Similar to the above, but only get incremented after each Execute() is finished.
int g_iNoExecBeforeIndex = 0, g_iNoExecAfterIndex = -1, g_iNoDrawAfterHUD = -1;
// The Skybox cannot be detected using g_iExecBufCounter anymore when using the hook_d3d because the whole frame is
// rendered with a single Execute call
//int g_iSkyBoxExecIndex = DEFAULT_SKYBOX_INDEX; // This gives us the threshold for the Skybox
// g_iSkyBoxExecIndex is compared against g_iExecBufCounter to determine when the SkyBox is rendered
// This is important because in XWAU the SkyBox is *not* rendered at infinity and causes a lot of
// visual contention if not handled properly.
bool g_bIsSkyBox = false, g_bPrevIsSkyBox = false, g_bSkyBoxJustFinished = false;

bool g_bFixSkyBox = true; // Fix the skybox (send it to infinity: use original vertices without parallax)
bool g_bSkipSkyBox = false;
bool g_bStartedGUI = false; // Set to false at the beginning of each frame. Set to true when the GUI has begun rendering.
bool g_bPrevStartedGUI = false; // Keeps the last value of g_bStartedGUI -- helps detect when the GUI is about to be rendered.
bool g_bIsScaleableGUIElem = false; // Set to false at the beginning of each frame. Set to true when the scaleable GUI has begun rendering.
bool g_bPrevIsScaleableGUIElem = false; // Keeps the last value of g_bIsScaleableGUIElem -- helps detect when scaleable GUI is about to start rendering.
bool g_bScaleableHUDStarted = false; // Becomes true as soon as the scaleable HUD is about to be rendered. Reset to false at the end of each frame
bool g_bSkipGUI = false; // Skip non-skybox draw calls with disabled Z-Buffer
bool g_bSkipText = false; // Skips text draw calls
bool g_bSkipAfterTargetComp = false; // Skip all draw calls after the targetting computer has been drawn
bool g_bTargetCompDrawn = false; // Becomes true after the targetting computer has been drawn
bool g_bPrevIsFloatingGUI3DObject = false; // Stores the last value of g_bIsFloatingGUI3DObject -- useful to detect when the targeted craft is about to be drawn
bool g_bIsFloating3DObject = false; // true when rendering the targeted 3D object.
bool g_bIsTrianglePointer = false, g_bLastTrianglePointer = false;
bool g_bIsPlayerObject = false, g_bPrevIsPlayerObject = false, g_bHyperspaceEffectRenderedOnCurrentFrame = false;
bool g_bIsTargetHighlighted = false; // True if the target can  be fired upon, gets reset every frame
bool g_bPrevIsTargetHighlighted = false; // The value of g_bIsTargetHighlighted for the previous frame

//bool g_bLaserBoxLimitsUpdated = false; // Set to true whenever the laser/ion charge limit boxes are updated
unsigned int g_iFloatingGUIDrawnCounter = 0;
int g_iPresentCounter = 0, g_iNonZBufferCounter = 0, g_iSkipNonZBufferDrawIdx = -1;
float g_fZBracketOverride = 65530.0f; // 65535 is probably the maximum Z value in XWA
bool g_bResetDC = false;

// DS2 Effects
int g_iReactorExplosionCount = 0;

/*********************************************************/
// HYPERSPACE
HyperspacePhaseEnum g_HyperspacePhaseFSM = HS_INIT_ST;
int g_iHyperExitPostFrames = 0;
//Vector3 g_fCameraCenter(0.0f, 0.0f, 0.0f);
float g_fHyperTunnelSpeed = 5.5f, g_fHyperShakeRotationSpeed = 1.0f, g_fHyperLightRotationSpeed = 1.0f, g_fHyperspaceRand = 0.0f;
float g_fCockpitCameraYawOnFirstHyperFrame, g_fCockpitCameraPitchOnFirstHyperFrame, g_fCockpitCameraRollOnFirstHyperFrame;
short g_fLastCockpitCameraYaw, g_fLastCockpitCameraPitch;
int g_lastCockpitXReference, g_lastCockpitYReference, g_lastCockpitZReference;
bool g_bHyperspaceFirstFrame = false, g_bHyperHeadSnapped = false, g_bClearedAuxBuffer = false, g_bSwitchedToGUI = false;
bool g_bHyperExternalToCockpitTransition = false;
bool g_bHyperspaceTunnelLastFrame = false, g_bHyperspaceLastFrame = false;
// DEBUG
//#define HYPER_OVERRIDE
bool g_bHyperDebugMode = false;
float g_fHyperTimeOverride = 0.0f; // Only used to debug the post-hyper-exit effect. I should remove this later.
int g_iHyperStateOverride = HS_HYPER_ENTER_ST;
//int g_iHyperStateOverride = HS_HYPER_TUNNEL_ST;
//int g_iHyperStateOverride = HS_HYPER_EXIT_ST;
//int g_iHyperStateOverride = HS_POST_HYPER_EXIT_ST;
// DEBUG

/*********************************************************/
// ACTIVE COCKPIT
Vector4 g_contOriginWorldSpace = Vector4(0.0f, 0.0f, 0.05f, 1.0f); // This is the origin of the controller in 3D, in world-space coords
Vector4 g_contDirWorldSpace = Vector4(0.0f, 0.0f, 1.0f, 0.0f); // This is the direction in which the controller is pointing in world-space coords
Vector4 g_contOriginViewSpace = Vector4(0.0f, 0.0f, 0.05f, 1.0f); // This is the origin of the controller in 3D, in view-space coords
Vector4 g_contDirViewSpace = Vector4(0.0f, 0.0f, 1.0f, 0.0f); // The direction in which the controller is pointing, in view-space coords
Vector3 g_LaserPointer3DIntersection = Vector3(0.0f, 0.0f, 10000.0f);
float g_fBestIntersectionDistance = 10000.0f, g_fLaserPointerLength = 0.0f;
float g_fContMultiplierX, g_fContMultiplierY, g_fContMultiplierZ;
int g_iBestIntersTexIdx = -1; // The index into g_ACElements where the intersection occurred
bool g_bActiveCockpitEnabled = false, g_bACActionTriggered = false, g_bACLastTriggerState = false, g_bACTriggerState = false;
bool g_bOriginFromHMD = false, g_bCompensateHMDRotation = false, g_bCompensateHMDPosition = false, g_bFullCockpitTest = true;
bool g_bFreePIEControllerButtonDataAvailable = false;
ac_element g_ACElements[MAX_AC_TEXTURES_PER_COCKPIT] = { 0 };
int g_iNumACElements = 0, g_iLaserDirSelector = 3;
// DEBUG vars
Vector3 g_debug_v0, g_debug_v1, g_debug_v2;
bool g_bDumpLaserPointerDebugInfo = false;
Vector3 g_LPdebugPoint;
float g_fLPdebugPointOffset = 0.0f;
// DEBUG vars

/*********************************************************/
// DYNAMIC COCKPIT
char g_sCurrentCockpit[128] = { 0 };
DCHUDRegions g_DCHUDRegions;
DCElemSrcBoxes g_DCElemSrcBoxes;
dc_element g_DCElements[MAX_DC_SRC_ELEMENTS] = { 0 };

float g_fCoverTextureBrightness = 1.0f;
float g_fDCBrightness = 1.0f;
int g_iNumDCElements = 0;
move_region_coords g_DCMoveRegions = { 0 };
float g_fCurInGameWidth = 1, g_fCurInGameHeight = 1, g_fCurInGameAspectRatio = 1, g_fCurScreenWidth = 1, g_fCurScreenHeight = 1, g_fCurScreenWidthRcp = 1, g_fCurScreenHeightRcp = 1;
bool g_bDCManualActivate = true, g_bDCApplyEraseRegionCommands = false, g_bReRenderMissilesNCounterMeasures = false;
bool g_bGlobalDebugFlag = false, g_bInhibitCMDBracket = false;
bool g_bHUDVisibleOnStartup = false;
bool g_bCompensateFOVfor1920x1080 = true;
bool g_bDCWasClearedOnThisFrame = false;
int g_iHUDOffscreenCommandsRendered = 0;
bool g_bEdgeEffectApplied = false;
extern int g_WindowWidth, g_WindowHeight;
float4 g_DCTargetingColor, g_DCWireframeLuminance;
float4 g_DCTargetingIFFColors[6];
float g_DCWireframeContrast = 3.0f;
float g_fReticleScale = DEFAULT_RETICLE_SCALE;
extern Vector2 g_SubCMDBracket; // Populated in XwaDrawBracketHook for the sub-CMD bracket when the enhanced 2D renderer is on
// HOLOGRAMS
float g_fDCHologramFadeIn = 0.0f, g_fDCHologramFadeInIncr = 0.04f, g_fDCHologramTime = 0.0f;
bool g_bDCHologramsVisible = true, g_bDCHologramsVisiblePrev = true;

Vector2 g_TriangleCentroid;

/*********************************************************/
// SHADOW MAPPING
ShadowMappingData g_ShadowMapping;
float g_fShadowMapAngleY = 0.0f, g_fShadowMapAngleX = 0.0f, g_fShadowMapDepthTrans = 0.0f, g_fShadowMapScale = 0.5f;

// TODO: The VR path doesn't need this scale. We should get rid of it
// in the regular path too. This constant factor will affect how the POV
// translation is computed for both paths, and it may also affect the
// projection formulas in the regular path.
float SHADOW_OBJ_SCALE = 1.64f; // TODO: These scale params should be in g_ShadowMapping

bool g_bShadowMapDebug = false, g_bShadowMappingInvertCameraMatrix = false, g_bShadowMapEnablePCSS = false, g_bShadowMapEnable = false;
std::vector<Vector4> g_OBJLimits; // Box limits of the OBJ loaded. This is used to compute the Z range of the shadow map

Vector3 g_SunCentroids[MAX_XWA_LIGHTS]; // Stores all the sun centroids seen in this frame in in-game coords
Vector2 g_SunCentroids2D[MAX_XWA_LIGHTS]; // Stores all the 2D sun centroids seen in this frame in in-game coords
int g_iNumSunCentroids = 0;

/*********************************************************/

extern bool g_bRendering3D; // Used to distinguish between 2D (Concourse/Menus) and 3D rendering (main in-flight game)
// g_fZOverride is activated when it's greater than -0.9f, and it's used for bracket rendering so that 
// objects cover the brackets. In this way, we avoid visual contention from the brackets.
bool g_bCockpitPZHackEnabled = true;
bool g_bOverrideAspectRatio = false;
/*
true if either DirectSBS or SteamVR are enabled. false for original display mode
*/
bool g_bEnableVR = true;
TrackerType g_TrackerType = TRACKER_NONE;

extern std::vector<Direct3DTexture *> g_AuxTextureVector;
XWALightInfo g_XWALightInfo[MAX_XWA_LIGHTS];
//void InitHeadingMatrix();
//Matrix4 GetCurrentHeadingMatrix(Vector4 &Rs, Vector4 &Us, Vector4 &Fs, bool invert, bool debug);
Matrix4 GetCurrentHeadingViewMatrix();
Matrix4 GetSimpleDirectionMatrix(Vector4 Fs, bool invert);
float g_fDebugFOVscale = 1.0f;
float g_fDebugYCenter = 0.0f;

// Bloom
const int MAX_BLOOM_PASSES = 9;
const int DEFAULT_BLOOM_PASSES = 5;
bool g_bReshadeEnabled = DEFAULT_RESHADE_ENABLED_STATE;
bool g_bBloomEnabled = DEFAULT_BLOOM_ENABLED_STATE;
extern BloomPixelShaderCBStruct g_BloomPSCBuffer;
BloomConfig g_BloomConfig = { 1 };
extern float g_fBloomLayerMult[MAX_BLOOM_PASSES + 1], g_fBloomSpread[MAX_BLOOM_PASSES + 1];
extern int g_iBloomPasses[MAX_BLOOM_PASSES + 1];

// SSAO
SSAOTypeEnum g_SSAO_Type = SSO_AMBIENT;
extern PSShadingSystemCB	  g_ShadingSys_PSBuffer;
extern SSAOPixelShaderCBuffer g_SSAO_PSCBuffer;
float g_fHangarAmbient = 0.05f, g_fGlobalAmbient = 0.005f;

extern float g_fMoireOffsetDir, g_fMoireOffsetInd;
bool g_bAOEnabled = DEFAULT_AO_ENABLED_STATE, g_bDisableDiffuse = false;
int g_iSSDODebug = 0, g_iSSAOBlurPasses = 1;
float g_fSSAOZoomFactor = 2.0f, g_fSSAOZoomFactor2 = 4.0f, g_fSSAOWhitePoint = 0.7f, g_fNormWeight = 1.0f, g_fNormalBlurRadius = 0.01f;
float g_fSSAOAlphaOfs = 0.5f;
//float g_fViewYawSign = 1.0f, g_fViewPitchSign = -1.0f; // Old values for SSAO.cfg-based lightsf
float g_fViewYawSign = -1.0f, g_fViewPitchSign = 1.0f; // New values for XwaLights
float g_fSpecIntensity = 1.0f, g_fSpecBloomIntensity = 1.25f, g_fXWALightsSaturation = 0.8f, g_fXWALightsIntensity = 1.0f;
bool g_bApplyXWALightsIntensity = true, g_bProceduralSuns = true, g_bEnableHeadLights = false, g_bProceduralLava = true;
bool g_bBlurSSAO = true, g_bDepthBufferResolved = false; // g_bDepthBufferResolved gets reset to false at the end of each frame
bool g_bShowSSAODebug = false, g_bDumpSSAOBuffers = false, g_bEnableIndirectSSDO = false, g_bFNEnable = true;
bool g_bDisableDualSSAO = false, g_bEnableSSAOInShader = true, g_bEnableBentNormalsInShader = true;
bool g_bOverrideLightPos = false, g_bShadowEnable = true, g_bEnableSpeedShader = true, g_bEnableAdditionalGeometry = false;
float g_fSpeedShaderScaleFactor = 35.0f, g_fSpeedShaderParticleSize = 0.0075f, g_fSpeedShaderMaxIntensity = 0.6f, g_fSpeedShaderTrailSize = 0.1f;
float g_fSpeedShaderParticleRange = 50.0f; // This used to be 10.0
float g_fCockpitTranslationScale = 0.0025f; // 1.0f / 400.0f;
int g_iSpeedShaderMaxParticles = MAX_SPEED_PARTICLES;
Vector4 g_LightVector[2], g_TempLightVector[2];
Vector4 g_LightColor[2], g_TempLightColor[2];
//float g_fFlareAspectMult = 1.0f; // DEBUG: Fudge factor to place the flares on the right spot...

// white_point = 1 --> OK
// white_point = 0.5 --> Makes everything bright
// white_point = 4.0 --> Makes everything dark
// So, a bright scene should cause the white point to go up, and a dark scence should cause
// the white point to go down... but not by much in either direction.
float g_fHDRLightsMultiplier = 2.0f, g_fHDRWhitePoint = 1.0f;
bool g_bHDREnabled = false;

bool g_bDumpOBJEnabled = false;
FILE *g_DumpOBJFile = NULL;
int g_iDumpOBJFaceIdx = 0, g_iDumpOBJIdx = 1;

bool g_bDumpSpecificTex = false;
int g_iDumpSpecificTexIdx = 0;
bool g_bDisplayWidth = false;
extern bool g_bDumpDebug;
//bool g_bDumpBloomBuffers = false;

// This is the current resolution of the screen:
float g_fLensK1 = DEFAULT_LENS_K1;
float g_fLensK2 = DEFAULT_LENS_K2;
float g_fLensK3 = DEFAULT_LENS_K3;

// GUI elements seem to be in the range 0..0.0005, so 0.0008 sounds like a good threshold:
float g_fGUIElemPZThreshold = DEFAULT_GUI_ELEM_PZ_THRESHOLD;
float g_fTrianglePointerDist = DEFAULT_TRIANGLE_POINTER_DIST;
float g_fGlobalScale = DEFAULT_GLOBAL_SCALE;
//float g_fPostProjScale = 1.0f;
float g_fGlobalScaleZoomOut = DEFAULT_ZOOM_OUT_SCALE;
float g_fConcourseScale = DEFAULT_CONCOURSE_SCALE;
float g_fConcourseAspectRatio = DEFAULT_CONCOURSE_ASPECT_RATIO;
float g_fHUDDepth = DEFAULT_HUD_PARALLAX; // The aiming HUD is rendered at this depth
bool g_bFloatingAimingHUD = DEFAULT_FLOATING_AIMING_HUD; // The aiming HUD can be fixed to the cockpit glass or floating
float g_fTextDepth = DEFAULT_TEXT_PARALLAX; // All text gets rendered at this parallax
float g_fFloatingGUIDepth = DEFAULT_FLOATING_GUI_PARALLAX; // Floating GUI elements are rendered at this depth
float g_fFloatingGUIObjDepth = DEFAULT_FLOATING_OBJ_PARALLAX; // The targeted object must be rendered above the Floating GUI
float g_fTechLibraryParallax = DEFAULT_TECH_LIB_PARALLAX;
float g_fAspectRatio = DEFAULT_ASPECT_RATIO;
bool g_bZoomOut = DEFAULT_ZOOM_OUT_INITIAL_STATE;
bool g_bZoomOutInitialState = DEFAULT_ZOOM_OUT_INITIAL_STATE;
float g_fBrightness = DEFAULT_BRIGHTNESS;
float g_fGUIElemsScale = DEFAULT_GUI_SCALE; // Used to reduce the size of all the GUI elements
int g_iFreePIESlot = DEFAULT_FREEPIE_SLOT;
int g_iFreePIEControllerSlot = -1;
bool g_bFixedGUI = DEFAULT_FIXED_GUI_STATE;
bool g_bSteamVRPosFromFreePIE = DEFAULT_STEAMVR_POS_FROM_FREEPIE;
bool g_bDirectSBSInitialized = false;
//float g_fXWAScale = 1.0f; // This is the scale value as computed during Execute()
D3D11_VIEWPORT g_nonVRViewport{};

VertexShaderMatrixCB g_VSMatrixCB;
VertexShaderCBuffer  g_VSCBuffer;
PixelShaderCBuffer   g_PSCBuffer;
DCPixelShaderCBuffer g_DCPSCBuffer;
ShadertoyCBuffer	 g_ShadertoyBuffer;
LaserPointerCBuffer	 g_LaserPointerBuffer;
ShadowMapVertexShaderMatrixCB g_ShadowMapVSCBuffer;
MetricReconstructionCB g_MetricRecCBuffer;

float g_fCockpitPZThreshold = DEFAULT_COCKPIT_PZ_THRESHOLD; // The TIE-Interceptor needs this thresold!
float g_fBackupCockpitPZThreshold = g_fCockpitPZThreshold; // Backup of the cockpit threshold, used when toggling this effect on or off.

const float IPD_SCALE_FACTOR = 100.0f; // Transform centimeters to meters (IPD = 6.5 becomes 0.065)
const float GAME_SCALE_FACTOR = 60.0f; // Estimated empirically
const float GAME_SCALE_FACTOR_Z = 60.0f; // Estimated empirically

// In reality, there should be a different factor per in-game resolution; but for now this should be enough
const float C = 1.0f, Z_FAR = 1.0f;
const float LOG_K = 1.0f;

float g_fIPD = DEFAULT_IPD / IPD_SCALE_FACTOR;
float g_fHalfIPD = g_fIPD / 2.0f;
float g_fFocalDist = DEFAULT_FOCAL_DIST;
float g_fFakeRoll = 0.0f;

/*
 * Control/Debug variables
 */
bool g_bDisableZBuffer = false;
bool g_bDisableBarrelEffect = false;
bool g_bStart3DCapture = false, g_bDo3DCapture = false, g_bSkipTexturelessGUI = false;
// g_bSkipTexturelessGUI skips draw calls that don't have a texture associated (lastTextureSelected == NULL)
bool g_bDumpGUI = false;
int g_iHUDTexDumpCounter = 0;
int g_iDumpGUICounter = 0, g_iHUDCounter = 0;

bool LoadGeneric3DCoords(char *buf, float *x, float *y, float *z);
bool LoadGeneric4DCoords(char *buf, float *x, float *y, float *z, float *w);
void LoadCockpitLookParams();
bool isInVector(uint32_t crc, std::vector<uint32_t> &vector);
int isInVector(char *name, dc_element *dc_elements, int num_elems);
int isInVector(char *name, ac_element *ac_elements, int num_elems);
bool isInVector(char *OPTname, std::vector<OPTNameType> &vector);
bool InitDirectSBS();
bool LoadFocalLength();

SmallestK g_LaserList;
bool g_bEnableLaserLights = false;
bool g_b3DSunPresent = false;
bool g_b3DSkydomePresent = false;
extern Vector3 g_HeadLightsPosition, g_HeadLightsColor;
extern float g_fHeadLightsAmbient, g_fHeadLightsDistance, g_fHeadLightsAngleCos;
extern bool g_bHeadLightsAutoTurnOn;

bool g_bReloadMaterialsEnabled = false;
Material g_DefaultGlobalMaterial;

void GetScreenLimitsInUVCoords(float *x0, float *y0, float *x1, float *y1, bool UseNonVR = false);
void InGameToScreenCoords(UINT left, UINT top, UINT width, UINT height, float x, float y, float *x_out, float *y_out);
void ScreenCoordsToInGame(float left, float top, float width, float height, float x, float y, float *x_out, float *y_out);

/*
 * Converts a metric depth value to in-game (sz, rhw) values, copying the behavior of the game
 */
void ZToDepthRHW(float Z, float *sz_out, float *rhw_out)
{
	float sz = Z;
	float *Znear = (float *)0x08B94CC;
	float *Zfar = (float *)0x05B46B4;
	//log_debug("[DBG] nearZ: %0.3f, farZ: %0.3f", *nearZ, *farZ);
	if (sz < 0.0f)
		sz = *Znear;
	
	float rhw = (sz * *Zfar) / (sz * *Zfar + *Znear);

	if (rhw < 1.52590219E-05f)
		rhw = 1.52590219E-05f;

	// s_V0x064D1A8[s_V0x06628E0].z = st1;
	*sz_out = rhw;
	//s_V0x064D1A8[s_V0x06628E0].rhw = st0;
	*rhw_out = sz;
}

/*
 * Resets the g_XWALightInfo array to untagged, all suns.
 */
void ResetXWALightInfo()
{
	log_debug("[DBG] [SHW] Resetting g_XWALightInfo");
	g_ShadowMapping.bAllLightsTagged = false;
	for (int i = 0; i < MAX_XWA_LIGHTS; i++) {
		g_XWALightInfo[i].Reset();
		g_ShadowMapVSCBuffer.sm_black_levels[i] = g_ShadowMapping.black_level;
	}
}

void SmallestK::insert(Vector3 P, Vector3 col) {
	int i = _size - 1;
	while (i >= 0 && P.z < _elems[i].P.z) {
		// Copy the i-th element to the (i+1)-th index to make space at i
		if (i + 1 < MAX_CB_POINT_LIGHTS)
			_elems[i + 1] = _elems[i];
		i--;
	}

	// Insert at i + 1 (if possible) and we're done
	if (i + 1 < MAX_CB_POINT_LIGHTS) {
		_elems[i + 1].P = P;
		_elems[i + 1].col = col;
		if (_size < MAX_CB_POINT_LIGHTS)
			_size++;
	}
}

// NewIPD is in cms
void EvaluateIPD(float NewIPD) {
	if (NewIPD < 0.0f)
		NewIPD = 0.0f;

	g_fIPD = NewIPD / IPD_SCALE_FACTOR;
	log_debug("[DBG] NewIPD: %0.3f, Actual g_fIPD: %0.6f", NewIPD, g_fIPD);
	g_fHalfIPD = g_fIPD / 2.0f;
}

// Delta is in cms here
void IncreaseIPD(float Delta) {
	float NewIPD = g_fIPD * IPD_SCALE_FACTOR + Delta;
	EvaluateIPD(NewIPD);
}

void ToggleCockpitPZHack() {
	g_bCockpitPZHackEnabled = !g_bCockpitPZHackEnabled;
	//log_debug("[DBG] CockpitHackEnabled: %d", g_bCockpitPZHackEnabled);
	if (!g_bCockpitPZHackEnabled)
		g_fCockpitPZThreshold = 2.0f;
	else
		g_fCockpitPZThreshold = g_fBackupCockpitPZThreshold;
}

void ToggleZoomOutMode() {
	g_bZoomOut = !g_bZoomOut;
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;
}

void IncreaseZOverride(float Delta) {
	g_fZBracketOverride += Delta;
	//log_debug("[DBG] g_fZOverride: %f", g_fZOverride);
}

void IncreaseZoomOutScale(float Delta) {
	g_fGlobalScaleZoomOut += Delta;
	if (g_fGlobalScaleZoomOut < 0.2f)
		g_fGlobalScaleZoomOut = 0.2f;

	// Apply this change by modifying the global scale:
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;

	g_fConcourseScale += Delta;
	if (g_fConcourseScale < 0.2f)
		g_fConcourseScale = 0.2f;
}

void IncreaseHUDParallax(float Delta) {
	g_fHUDDepth += Delta;
	log_debug("[DBG] HUD parallax: %f", g_fHUDDepth);
}

void IncreaseFloatingGUIParallax(float Delta) {
	g_fFloatingGUIDepth += Delta;
	log_debug("[DBG] GUI parallax: %f", g_fFloatingGUIDepth);
}

void IncreaseTextParallax(float Delta) {
	g_fTextDepth += Delta;
	log_debug("[DBG] Text parallax: %f", g_fTextDepth);
}

void IncreaseCockpitThreshold(float Delta) {
	g_fCockpitPZThreshold += Delta;
	g_fBackupCockpitPZThreshold = g_fCockpitPZThreshold;
	log_debug("[DBG] New cockpit threshold: %f", g_fCockpitPZThreshold);
}

void IncreaseGUIThreshold(float Delta) {
	g_fGUIElemPZThreshold += Delta;
	//log_debug("[DBG] New GUI threshold: %f", GUI_elem_pz_threshold);
}

void IncreaseScreenScale(float Delta) {
	g_fGlobalScale += Delta;
	if (g_fGlobalScale < 0.2f)
		g_fGlobalScale = 0.2f;
	log_debug("[DBG] New g_fGlobalScale: %0.3f", g_fGlobalScale);
}

/*
void IncreasePostProjScale(float Delta) {
	g_fPostProjScale += Delta;
	if (g_fPostProjScale < 0.2f)
		g_fPostProjScale = 0.2f;
	log_debug("[DBG] New g_fPostProjScale: %0.3ff", g_fPostProjScale);
}
*/

/*
void IncreaseFocalDist(float Delta) {
	g_fFocalDist += Delta;
	if (g_fFocalDist < 0.01f)
		g_fFocalDist = 0.01f;
	log_debug("[DBG] g_fFocalDist: %f", g_fFocalDist);
}
*/

void IncreaseNoDrawBeforeIndex(int Delta) {
	g_iNoDrawBeforeIndex += Delta;
	log_debug("[DBG] NoDraw BeforeIdx, AfterIdx: %d, %d", g_iNoDrawBeforeIndex, g_iNoDrawAfterIndex);
}

void IncreaseNoDrawAfterIndex(int Delta) {
	g_iNoDrawAfterIndex += Delta;
	log_debug("[DBG] NoDraw BeforeIdx, AfterIdx: %d, %d", g_iNoDrawBeforeIndex, g_iNoDrawAfterIndex);
}

void IncreaseNoExecIndices(int DeltaBefore, int DeltaAfter) {
	g_iNoExecBeforeIndex += DeltaBefore;
	g_iNoExecAfterIndex += DeltaAfter;
	if (g_iNoExecBeforeIndex < -1)
		g_iNoExecBeforeIndex = -1;

	log_debug("[DBG] NoExec BeforeIdx, AfterIdx: %d, %d", g_iNoExecBeforeIndex, g_iNoExecAfterIndex);
}

void IncreaseNoDrawAfterHUD(int Delta) {
	g_iNoDrawAfterHUD += Delta;
	log_debug("[DBG] NoDrawAfterHUD: %d", g_iNoDrawAfterHUD);
}


void IncreaseSkipNonZBufferDrawIdx(int Delta) {
	g_iSkipNonZBufferDrawIdx += Delta;
	log_debug("[DBG] New g_iSkipNonZBufferDrawIdx: %d", g_iSkipNonZBufferDrawIdx);
}

//void IncreaseSkyBoxIndex(int Delta) {
//	g_iSkyBoxExecIndex += Delta;
//	log_debug("[DBG] New g_iSkyBoxExecIndex: %d", g_iSkyBoxExecIndex);
//}

void IncreaseLensK1(float Delta) {
	g_fLensK1 += Delta;
	log_debug("[DBG] New k1: %f", g_fLensK1);
}

void IncreaseLensK2(float Delta) {
	g_fLensK2 += Delta;
	log_debug("[DBG] New k2: %f", g_fLensK2);
}

/* Restores the various VR parameters to their default values. */
void ResetVRParams() {
	//g_fFocalDist = g_bSteamVREnabled ? DEFAULT_FOCAL_DIST_STEAMVR : DEFAULT_FOCAL_DIST;
	g_fFocalDist = DEFAULT_FOCAL_DIST;
	//g_fMetricMult = DEFAULT_METRIC_MULT;
	EvaluateIPD(DEFAULT_IPD);
	g_bCockpitPZHackEnabled = true;
	g_fGUIElemPZThreshold = DEFAULT_GUI_ELEM_PZ_THRESHOLD;
	g_fTrianglePointerDist = DEFAULT_TRIANGLE_POINTER_DIST;
	//g_fGlobalScale = g_bSteamVREnabled ? DEFAULT_GLOBAL_SCALE_STEAMVR : DEFAULT_GLOBAL_SCALE;
	g_fGlobalScale = DEFAULT_GLOBAL_SCALE;
	//g_fPostProjScale = 1.0f;
	g_fGlobalScaleZoomOut = DEFAULT_ZOOM_OUT_SCALE;
	g_bZoomOut = g_bZoomOutInitialState;
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;
	g_fConcourseScale = DEFAULT_CONCOURSE_SCALE;
	g_fCockpitPZThreshold = DEFAULT_COCKPIT_PZ_THRESHOLD;
	g_fBackupCockpitPZThreshold = g_fCockpitPZThreshold;
	g_fReticleScale = DEFAULT_RETICLE_SCALE;

	g_fAspectRatio = DEFAULT_ASPECT_RATIO;
	g_fConcourseAspectRatio = DEFAULT_CONCOURSE_ASPECT_RATIO;
	g_fLensK1 = DEFAULT_LENS_K1;
	g_fLensK2 = DEFAULT_LENS_K2;
	g_fLensK3 = DEFAULT_LENS_K3;

	g_bFixSkyBox = true;
	//g_iSkyBoxExecIndex = DEFAULT_SKYBOX_INDEX;
	g_bSkipText = false;
	g_bSkipGUI = false;
	g_bSkipSkyBox = false;

	g_iNoDrawBeforeIndex = 0; g_iNoDrawAfterIndex = -1;
	g_iNoExecBeforeIndex = 0; g_iNoExecAfterIndex = -1;
	g_fHUDDepth = DEFAULT_HUD_PARALLAX;
	g_bFloatingAimingHUD = DEFAULT_FLOATING_AIMING_HUD;
	g_fTextDepth = DEFAULT_TEXT_PARALLAX;
	g_fFloatingGUIDepth = DEFAULT_FLOATING_GUI_PARALLAX;
	g_fTechLibraryParallax = DEFAULT_TECH_LIB_PARALLAX;
	g_fFloatingGUIObjDepth = DEFAULT_FLOATING_OBJ_PARALLAX;
	g_iNaturalConcourseAnimations = DEFAULT_NATURAL_CONCOURSE_ANIM;

	g_fBrightness = DEFAULT_BRIGHTNESS;
	//g_bStickyArrowKeys = false;
	//g_bYawPitchFromMouseOverride = false;

	g_bInterleavedReprojection = DEFAULT_INTERLEAVED_REPROJECTION;
	if (g_bUseSteamVR) {
		g_pVRCompositor->ForceInterleavedReprojectionOn(g_bInterleavedReprojection);
		g_bSteamVRDistortionEnabled = true;
	}

	//g_bDisableBarrelEffect = g_bUseSteamVR ? !DEFAULT_BARREL_EFFECT_STATE_STEAMVR : !DEFAULT_BARREL_EFFECT_STATE;

	g_bFixedGUI = DEFAULT_FIXED_GUI_STATE;

	g_iFreePIESlot = DEFAULT_FREEPIE_SLOT;
	g_fYawMultiplier   = DEFAULT_YAW_MULTIPLIER;
	g_fPitchMultiplier = DEFAULT_PITCH_MULTIPLIER;
	g_fRollMultiplier  = DEFAULT_ROLL_MULTIPLIER;
	g_fYawOffset   = DEFAULT_YAW_OFFSET;
	g_fPitchOffset = DEFAULT_PITCH_OFFSET;
	g_fPosXMultiplier  = DEFAULT_POS_X_MULTIPLIER;
	g_fPosYMultiplier  = DEFAULT_POS_Y_MULTIPLIER;
	g_fPosZMultiplier  = DEFAULT_POS_Z_MULTIPLIER;
	g_fMinPositionX = DEFAULT_MIN_POS_X; g_fMaxPositionX = DEFAULT_MAX_POS_X;
	g_fMinPositionY = DEFAULT_MIN_POS_Y; g_fMaxPositionY = DEFAULT_MAX_POS_Y;
	g_fMinPositionZ = DEFAULT_MIN_POS_Z; g_fMaxPositionZ = DEFAULT_MAX_POS_Z;

	g_iSteamVR_Remaining_ms = 3; g_iSteamVR_VSync_ms = 11;

	// Recompute the eye and projection matrices
	if (!g_bUseSteamVR)
		InitDirectSBS();

	//g_bReshadeEnabled = DEFAULT_RESHADE_ENABLED_STATE;
	//g_bBloomEnabled = DEFAULT_BLOOM_ENABLED_STATE;
	//g_bDynCockpitEnabled = DEFAULT_DYNAMIC_COCKPIT_ENABLED;

	/**g_fRawFOVDist = g_fDefaultFOVDist;
	*g_cachedFOVDist = g_fDefaultFOVDist / 512.0f;
	*g_rawFOVDist = (uint32_t)g_fDefaultFOVDist;*/
	// Load CRCs
	LoadCockpitLookParams();
}

/* Saves the current view parameters to vrparams.cfg */
void SaveVRParams() {
	FILE *file;
	int error = 0;
	
	try {
		error = fopen_s(&file, "./VRParams.cfg", "wt");
	} catch (...) {
		log_debug("[DBG] Could not save VRParams.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when saving VRParams.cfg", error);
		return;
	}
	fprintf(file, "; VR parameters. Write one parameter per line.\n");
	fprintf(file, "; Always make a backup copy of this file before modifying it.\n");
	fprintf(file, "; If you want to restore it to its default settings, delete the\n");
	fprintf(file, "; file and restart the game. Then press Ctrl+Alt+S to save a\n");
	fprintf(file, "; new config file with the default parameters -- however the\n");
	fprintf(file, "; VR mode may need to be set manually.\n");
	fprintf(file, "; To reload this file during game (at any point) just press Ctrl+Alt+L.\n");
	fprintf(file, "; Most parameters can be re-applied when reloading.\n");
	//fprintf(file, "; You can also press Ctrl+Alt+R to reset the viewing params to default values.\n\n");

	fprintf(file, "; VR Mode. Select from None, DirectSBS and SteamVR.\n");
	if (!g_bEnableVR)
		fprintf(file, "%s = %s\n", VR_MODE_VRPARAM, VR_MODE_NONE_SVAL);
	else {
		if (!g_bSteamVREnabled)
			fprintf(file, "%s = %s\n", VR_MODE_VRPARAM, VR_MODE_DIRECT_SBS_SVAL);
		else
			fprintf(file, "%s = %s\n", VR_MODE_VRPARAM, VR_MODE_STEAMVR_SVAL);
	}
	fprintf(file, "\n");

	//fprintf(file, "focal_dist = %0.6f # Try not to modify this value, change IPD instead.\n", focal_dist);

	fprintf(file, "; %s is measured in cms. Set it to 0 to remove the stereoscopy effect.\n", IPD_VRPARAM);
	fprintf(file, "; This setting is ignored in SteamVR mode. Configure the IPD through SteamVR instead.\n");
	fprintf(file, "%s = %0.1f\n\n", IPD_VRPARAM, g_fIPD * IPD_SCALE_FACTOR);
	//fprintf(file, "; %s amplifies the stereoscopy of objects in the game. Never set it to 0\n", METRIC_MULT_VRPARAM);
	//fprintf(file, "%s = %0.3f\n", METRIC_MULT_VRPARAM, g_fMetricMult);
	//fprintf(file, "%s = %0.3f\n", SIZE_3D_WINDOW_VRPARAM, g_fGlobalScale);

	fprintf(file, "; Scale of the reticle in VR mode.\n");
	fprintf(file, "%s = %0.3f\n\n", RETICLE_SCALE_VRPARAM, g_fReticleScale);
	
	fprintf(file, "; The following setting will reduce the scale of the HUD in VR mode.\n");
	fprintf(file, "%s = %0.3f\n\n", SIZE_3D_WINDOW_ZOOM_OUT_VRPARAM, g_fGlobalScaleZoomOut);

	fprintf(file, "; Set the following to 1 to start the HUD in zoomed-out mode:\n");
	fprintf(file, "%s = %d\n\n", WINDOW_ZOOM_OUT_INITIAL_STATE_VRPARAM, g_bZoomOutInitialState);

	fprintf(file, "%s = %0.3f\n\n", CONCOURSE_WINDOW_SCALE_VRPARAM, g_fConcourseScale);

	fprintf(file, "; The concourse animations can be played as fast as possible, or at its original\n");
	fprintf(file, "; 25fps setting:\n");
	fprintf(file, "%s = %d\n\n", NATURAL_CONCOURSE_ANIM_VRPARAM, g_iNaturalConcourseAnimations);
	/*
	fprintf(file, "; The following is a hack to increase the stereoscopy on objects. Unfortunately it\n");
	fprintf(file, "; also causes some minor artifacts: this is basically the threshold between the\n");
	fprintf(file, "; cockpit and the 'outside' world in normalized coordinates (0 is ZNear 1 is ZFar).\n");
	fprintf(file, "; Set it to 2 to disable this hack (stereoscopy will be reduced).\n");
	fprintf(file, "%s = %0.3f\n\n", COCKPIT_Z_THRESHOLD_VRPARAM, g_fCockpitPZThreshold);
	*/

	//fprintf(file, "\n; Specify the aspect ratio here to override the aspect ratio computed by the library.\n");
	//fprintf(file, "; ALWAYS specify BOTH the Concourse and 3D window aspect ratio.\n");
	//fprintf(file, "; You can also edit ddraw.cfg and set 'PreserveAspectRatio = 1' to get the library to\n");
	//fprintf(file, "; estimate the aspect ratio for you (this is the preferred method).\n");
	//fprintf(file, "%s = %0.3f\n", ASPECT_RATIO_VRPARAM, g_fAspectRatio);
	fprintf(file, "%s = %0.3f\n\n", CONCOURSE_ASPECT_RATIO_VRPARAM, g_fConcourseAspectRatio);

	fprintf(file, "; DirectSBS Lens correction parameters -- ignored in SteamVR mode.\n");
	fprintf(file, "; k2 has the biggest effect and k1 fine-tunes the effect.\n");
	fprintf(file, "; Positive values = convex warping; negative = concave warping.\n");
	fprintf(file, "%s = %0.6f\n", K1_VRPARAM, g_fLensK1);
	fprintf(file, "%s = %0.6f\n", K2_VRPARAM, g_fLensK2);
	fprintf(file, "%s = %0.6f\n", K3_VRPARAM, g_fLensK3);
	fprintf(file, "%s = %d\n\n", BARREL_EFFECT_STATE_VRPARAM, !g_bDisableBarrelEffect);

	/*
	fprintf(file, "; The following parameter will enable/disable SteamVR's lens distortion correction\n");
	fprintf(file, "; The default is 1, only set it to 0 if you're seeing distortion in SteamVR.\n");
	fprintf(file, "; If you set it to 0, I suggest you enable %s above to use the internal lens\n", BARREL_EFFECT_STATE_VRPARAM);
	fprintf(file, "; distortion correction instead\n");
	fprintf(file, "%s = %d\n\n", STEAMVR_DISTORTION_ENABLED_VRPARAM, g_bSteamVRDistortionEnabled);
	*/

	fprintf(file, "; Depth for various GUI elements in meters from the head's origin.\n");
	fprintf(file, "; Positive depth is forwards, negative is backwards (towards you).\n");
	fprintf(file, "; As a reference, the background starfield is 65km meters away.\n");
	fprintf(file, "%s = %0.3f\n\n", HUD_PARALLAX_VRPARAM, g_fHUDDepth);

	fprintf(file, "; If 6dof is enabled, the aiming HUD can be fixed to the cockpit or it can \"float\"\n");
	fprintf(file, "; and follow the lasers. When it's fixed, it's probably more realistic; but it will\n");
	fprintf(file, "; be harder to aim when you lean.\n");
	fprintf(file, "; When the aiming HUD is floating, it will follow the lasers when you lean,\n");
	fprintf(file, "; making it easier to aim properly.\n");
	fprintf(file, "%s = %d\n\n", FLOATING_AIMING_HUD_VRPARAM, g_bFloatingAimingHUD);

	fprintf(file, "%s = %0.3f\n\n", GUI_PARALLAX_VRPARAM, g_fFloatingGUIDepth);

	fprintf(file, "%s = %0.3f\n\n", GUI_OBJ_PARALLAX_VRPARAM, g_fFloatingGUIObjDepth);

	fprintf(file, "; %s is relative and it's always added to %s\n", GUI_OBJ_PARALLAX_VRPARAM, GUI_PARALLAX_VRPARAM);
	fprintf(file, "; This has the effect of making the targeted object \"hover\" above the targeting computer\n");
	fprintf(file, "; As a rule of thumb always make %s <= %s so that\n", TEXT_PARALLAX_VRPARAM, GUI_PARALLAX_VRPARAM);
	fprintf(file, "; the text hovers above the targeting computer\n\n");
	fprintf(file, "%s = %0.3f\n\n", TEXT_PARALLAX_VRPARAM, g_fTextDepth);

	fprintf(file, "; This is the depth added to the controls in the tech library. Make it negative to bring the\n");
	fprintf(file, "; controls towards you. Objects in the tech library are obviously scaled by XWA, because there's\n");
	fprintf(file, "; otherwise no way to visualize both a Star Destroyer and an A-Wing in the same volume.\n");
	fprintf(file, "%s = %0.3f\n\n", TECH_LIB_PARALLAX_VRPARAM, g_fTechLibraryParallax);

	fprintf(file, "; The HUD/GUI can be fixed in space now. If this setting is enabled, you'll be\n");
	fprintf(file, "; able to see all the HUD simply by looking around. You may also lean forward to\n");
	fprintf(file, "; zoom-in on the text messages to make them more readable.\n");
	fprintf(file, "%s = %d\n\n", FIXED_GUI_VRPARAM, g_bFixedGUI);

	fprintf(file, "; Set the following parameter to lower the brightness of the text,\n");
	fprintf(file, "; Concourse and 2D menus (avoids unwanted bloom when using ReShade).\n");
	fprintf(file, "; A value of 1 is normal brightness, 0 will render everything black.\n");
	fprintf(file, "%s = %0.3f\n\n", BRIGHTNESS_VRPARAM, g_fBrightness);

	fprintf(file, "; Interleaved Reprojection is a SteamVR setting that locks the framerate at 45fps.\n");
	fprintf(file, "; In some cases, it may help provide a smoother experience. Try toggling it\n");
	fprintf(file, "; to see what works better for your specific case.\n");
	fprintf(file, "%s = %d\n\n", INTERLEAVED_REPROJ_VRPARAM, g_bInterleavedReprojection);

	//fprintf(file, "\n");
	//fprintf(file, "%s = %d\n", INVERSE_TRANSPOSE_VRPARAM, g_bInverseTranspose);
	fprintf(file, "; Cockpit roll multiplier. Set it to 0 to de-activate this axis.\n");
	fprintf(file, "; The settings for pitch, yaw and positional tracking are in CockpitLook.cfg\n");
	fprintf(file, "%s = %0.3f\n\n", ROLL_MULTIPLIER_VRPARAM,  g_fRollMultiplier);

	// STEAMVR_POS_FROM_FREEPIE_VRPARAM is not saved because it's kind of a hack -- I'm only
	// using it because the PSMoveServiceSteamVRBridge is a bit tricky to setup and why would
	// I do that when my current FreePIEBridgeLite is working properly -- and faster.

	fprintf(file, "; Places the triangle pointer at the specified distance from the center of the\n");
	fprintf(file, "; screen. A value of 0 places it right at the center, a value of 0.5 puts it\n");
	fprintf(file, "; near the edge of the screen.\n");
	fprintf(file, "%s = %0.3f\n\n", TRIANGLE_POINTER_DIST_VRPARAM, g_fTrianglePointerDist);

	fclose(file);
	log_debug("[DBG] vrparams.cfg saved");
}

/* Loads cockpitlook params that are relevant to tracking */
void LoadCockpitLookParams() {
	log_debug("[DBG] Loading cockpit look params...");
	FILE *file;
	int error = 0;

	try {
		error = fopen_s(&file, "./cockpitlook.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load cockpitlook.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading cockpitlook.cfg", error);
		return;
	}

	char buf[160], param[80], svalue[80];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 160, file) != NULL) {
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 80, svalue, 80) > 0) {
			fValue = (float)atof(svalue);
			if (_stricmp(param, YAW_MULTIPLIER_CLPARAM) == 0) {
				g_fYawMultiplier = fValue;
			}
			else if (_stricmp(param, PITCH_MULTIPLIER_CLPARAM) == 0) {
				g_fPitchMultiplier = fValue;
			}
			else if (_stricmp(param, YAW_OFFSET_CLPARAM) == 0) {
				g_fYawOffset = fValue;
			}
			else if (_stricmp(param, PITCH_OFFSET_CLPARAM) == 0) {
				g_fPitchOffset = fValue;
			}
			else if (_stricmp(param, "tracker_type") == 0) {
				if (_stricmp(svalue, "FreePIE") == 0) {
					log_debug("Using FreePIE for tracking");
					g_TrackerType = TRACKER_FREEPIE;
				}
				else if (_stricmp(svalue, "SteamVR") == 0) {
					log_debug("Using SteamVR for tracking");
					g_TrackerType = TRACKER_STEAMVR;
				}
				else if (_stricmp(svalue, "TrackIR") == 0) {
					log_debug("Using TrackIR for tracking");
					g_TrackerType = TRACKER_TRACKIR;
				}
				else if (_stricmp(svalue, "None") == 0) {
					log_debug("Tracking disabled");
					g_TrackerType = TRACKER_NONE;
				}
				
			}
			/*else if (_stricmp(param, "cockpit_inertia_enabled") == 0) {
				g_bCockpitInertiaEnabled = (bool)fValue;
				log_debug("[DBG] Cockpit Inertia: %d", g_bCockpitInertiaEnabled);
			}*/
			// 6dof parameters
			else if (_stricmp(param, FREEPIE_SLOT_VRPARAM) == 0) {
				g_iFreePIESlot = (int)fValue;
			}
			param_read_count++;
		}
	} // while ... read file
	fclose(file);
	log_debug("[DBG] Loaded %d cockpitlook params", param_read_count);
}

/*
 * Loads a "source_def" or "erase_def" line from the global coordinates file
 */
bool LoadDCGlobalUVCoords(char *buf, Box *coords)
{
	float width, height;
	float x0, y0, x1, y1;
	int res = 0;
	char *c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f, %f, %f, %f",
				&width, &height, &x0, &y0, &x1, &y1);
			if (res < 6) {
				log_debug("[DBG] [DC] ERROR (skipping), expected at least 6 elements in '%s'", c);
				return false;
			}
			else {
				coords->x0 = x0 / width;
				coords->y0 = y0 / height;
				coords->x1 = x1 / width;
				coords->y1 = y1 / height;
			}
		}
		catch (...) {
			log_debug("[DBG] [DC] Could not read uv coords from: %s", buf);
			return false;
		}
	}
	return true;
}

/*
 * Loads a "screen_def" or "erase_screen_def" line from the global coordinates file. These
 * lines contain direct uv coords relative to the in-game screen. These coords are converted
 * to screen uv coords and returned.
 */
bool LoadDCScreenUVCoords(char *buf, Box *coords)
{
	float x0, y0, x1, y1;
	int res = 0;
	char *c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f, %f",
				&x0, &y0, &x1, &y1);
			if (res < 4) {
				log_debug("[DBG] [DC] ERROR (skipping), expected at least 4 elements in '%s'", c);
				return false;
			}
			else {
				coords->x0 = x0;
				coords->y0 = y0;
				coords->x1 = x1;
				coords->y1 = y1;
			}
		}
		catch (...) {
			log_debug("[DBG] [DC] Could not read uv coords from: %s", buf);
			return false;
		}
	}
	return true;
}

DCHUDRegions::DCHUDRegions() {
	Clear();
	//log_debug("[DBG] [DC] Adding g_HUDRegionNames.size(): %d", g_HUDRegionNames.size());
	for (int i = 0; i < MAX_HUD_BOXES; i++) {
		DCHUDRegion box = { 0 };
		box.bLimitsComputed = false;
		boxes.push_back(box);
	}
}

DCElemSrcBoxes::DCElemSrcBoxes() {
	Clear();
	//log_debug("[DBG] [DC] Adding g_DCElemSrcNames.size(): %d", g_DCElemSrcNames.size());
	for (int i = 0; i < MAX_DC_SRC_ELEMENTS; i++) {
		DCElemSrcBox src_box;
		src_box.bComputed = false;
		src_boxes.push_back(src_box);
	}
}

/* 
 * Load dynamic_cockpit_internal_areas.cfg 
 * This function will not grow g_DCElemSrcBoxes and it expects it to be populated
 * already.
 */
bool LoadDCInternalCoordinates() {
	
	log_debug("[DBG] [DC] Loading Internal Dynamic Cockpit coordinates...");
	FILE *file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;

	try {
		error = fopen_s(&file, "./DynamicCockpit/dynamic_cockpit_internal_areas.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] [DC] Could not load DynamicCockpit/dynamic_cockpit_internal_areas.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] [DC] ERROR %d when loading DynamicCockpit/dynamic_cockpit_internal_areas.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	unsigned int erase_slot = 0, source_slot = 0;
	float value = 0.0f;

	// TODO: Reset the global source and erase coordinates
	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			value = (float)atof(svalue);

			if (_stricmp(param, "source_def") == 0) {
				if (source_slot >= g_DCElemSrcBoxes.src_boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", source_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCGlobalUVCoords(buf, &box))
					g_DCElemSrcBoxes.src_boxes[source_slot].uv_coords = box;
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				source_slot++;
			}
			else if (_stricmp(param, "screen_def") == 0) {
				if (source_slot >= g_DCElemSrcBoxes.src_boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", source_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCScreenUVCoords(buf, &box))
					g_DCElemSrcBoxes.src_boxes[source_slot].uv_coords = box;
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				source_slot++;
			}
			else if (_stricmp(param, "erase_def") == 0) {
				if (erase_slot >= g_DCHUDRegions.boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", erase_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCGlobalUVCoords(buf, &box)) {
					g_DCHUDRegions.boxes[erase_slot].uv_erase_coords = box;
					g_DCHUDRegions.boxes[erase_slot].bLimitsComputed = false; // Force a recompute of the limits
				}
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				//log_debug("[DBG] [DC] Region %d = [%s] loaded", erase_slot, g_HUDRegionNames[erase_slot]);
				erase_slot++;
			}
			else if (_stricmp(param, "erase_screen_def") == 0) {
				if (erase_slot >= g_DCHUDRegions.boxes.size()) {
					log_debug("[DBG] [DC] Ignoring '%s' because slot: %d does not exist\n", erase_slot);
					continue;
				}
				Box box = { 0 };
				if (LoadDCScreenUVCoords(buf, &box)) {
					g_DCHUDRegions.boxes[erase_slot].uv_erase_coords = box;
					g_DCHUDRegions.boxes[erase_slot].bLimitsComputed = false; // Force a recompute of the limits
				}
				else
					log_debug("[DBG] [DC] WARNING: '%s' could not be loaded", buf);
				//log_debug("[DBG] [DC] Region %d = [%s] loaded", erase_slot, g_HUDRegionNames[erase_slot]);
				erase_slot++;
			}
		}
	}
	fclose(file);
	return true;
}

/*
 * Reads the name-slot from a string of the form:
 * name, x0, y0, x1, y1
 * returns the number of chars read
 */
int ReadNameFromLine(char *buf, char *name) 
{
	int res, len;

	res = sscanf_s(buf, "%s", name, 50);
	if (res < 1) {
		log_debug("[DBG] [DC] ERROR: Expected a slot name in '%s'", buf);
		return 0;
	}

	len = strlen(name);
	if (len <= 0) {
		log_debug("[DBG] [DC] ERROR: Empty slot name in '%s'", buf);
		return 0;
	}

	// Remove the trailing semi-colon from the name:
	if (name[len - 1] == ';' || name[len - 1] == ',')
		name[len - 1] = '\0';
	return len;
}

/* 
 * Loads a uv_coords row:
 * uv_coords = src-slot, x0,y0,x1,y1, [Hex-Color]
 */
bool LoadDCUVCoords(char *buf, float width, float height, uv_src_dst_coords *coords)
{
	float x0, y0, x1, y1, intensity;
	int src_slot;
	uint32_t uColor, hColor, wColor, uBitField, text_layer, obj_layer;
	int res = 0, idx = coords->numCoords;
	char *substr = NULL;
	char slot_name[50];

	if (idx >= MAX_DC_COORDS_PER_TEXTURE) {
		log_debug("[DBG] [DC] Too many coords already loaded");
		return false;
	}

	substr = strchr(buf, '=');
	if (substr == NULL) {
		log_debug("[DBG] [DC] Missing '=' in '%s', skipping", buf);
		return false;
	}
	// Skip the equal sign:
	substr++;
	// Skip white space chars:
	while (*substr == ' ' || *substr == '\t')
		substr++;

	try {
		int len;
		uColor = 0x121233;
		hColor = 0x0;
		wColor = 0x0;
		obj_layer = 1;
		text_layer = 1;
		intensity = 1.0f;
		uBitField = (0x00) /* intensity: 1 */ | (0x04) /* Bit 2 enables text */ | (0x08) /* Bit 3 enables objects */;

		src_slot = -1;
		slot_name[0] = 0;
		len = ReadNameFromLine(substr, slot_name);
		if (len == 0)
			return false;

		if (strstr(slot_name, "SRC") == NULL) {
			log_debug("[DBG] [DC] ERROR: Invalid source slot name: [%s]", slot_name);
			return false;
		}
				
		src_slot = DCSrcElemNameToIndex(slot_name);
		if (src_slot < 0) {
			log_debug("[DBG] [DC] ERROR: Could not find slot named: [%s]", slot_name);
			return false;
		}

		// Parse the rest of the parameters
		substr += len + 1;
		res = sscanf_s(substr, "%f, %f, %f, %f; 0x%x; %f; %d; %d; 0x%x; 0x%x", &x0, &y0, &x1, &y1, &uColor, &intensity, &text_layer, &obj_layer, &hColor, &wColor);
		//log_debug("[DBG] [DC] res: %d, slot_name: %s", res, slot_name);
		if (res < 4) {
			log_debug("[DBG] [DC] ERROR (skipping), expected at least 4 elements in '%s'", substr);
		} else {
			coords->src_slot[idx] = src_slot;
			coords->dst[idx].x0 = x0 / width;
			coords->dst[idx].y0 = y0 / height;
			coords->dst[idx].x1 = x1 / width;
			coords->dst[idx].y1 = y1 / height;
			// Process the custom intensity
			if (res >= 6)
			{
				uint32_t temp;
				// bits 0-1: An integer in the range 0..3 that specifies the intensity.
				if (intensity < 0.0f) intensity = 0.0f;
				temp = (uint32_t )(intensity) - 1;
				temp = (temp > 3) ? 3 : temp;
				uBitField |= temp;
			}
			// Process the text layer enable/disable
			if (res >= 7)
			{
				// bit 2 enables the text layer
				if (text_layer)
					uBitField |= 0x04;
				else
					uBitField &= 0xFB;
			}
			// Process the object layer enable/disable
			if (res >= 8)
			{
				// bit 3 enables the object layer
				if (obj_layer)
					uBitField |= 0x08;
				else
					uBitField &= 0xF7;
			}
			// Process the highlight color
			if (res < 9) 
			{
				hColor = uColor;
			}
			// Process the highlight color
			if (res < 10)
			{
				wColor = uColor;
			}
			// Store the regular and highlight colors
			coords->uBGColor[idx] = uColor | (uBitField << 24);
			coords->uHGColor[idx] = hColor | (uBitField << 24);
			coords->uWHColor[idx] = wColor | (uBitField << 24);
			coords->numCoords++;
			//log_debug("[DBG] uColor: 0x%x, hColor: 0x%x, [%s]",
			//	coords->uBGColor[idx], coords->uHGColor[idx], buf);
			//log_debug("[DBG] [DC] src_slot: %d, (%0.3f, %0.3f)-(%0.3f, %0.3f)",
			//	src_slot, x0 / width, y0 / height, x1 / width, y1 / height);
		}
	}
	catch (...) {
		log_debug("[DBG] [DC] Could not read uv coords from: %s", buf);
		return false;
	}
	return true;
}

/*
 * Converts a string representation of a hotkey to a series of scan codes
 */
void TranslateACAction(WORD *scanCodes, char *action) {
	// XWA keyboard reference:
	// http://isometricland.net/keyboard/keyboard-diagram-star-wars-x-wing-alliance.php?sty=15&lay=1&fmt=0&ten=1
	// Scan code tables:
	// http://www.philipstorr.id.au/pcbook/book3/scancode.htm
	// https://www.shsu.edu/~csc_tjm/fall2000/cs272/scan_codes.html
	// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
	int len = strlen(action);
	const char *ptr = action, *cursor;
	int i, j;
	// Translate to uppercase
	for (i = 0; i < len; i++)
		action[i] = toupper(action[i]);
	//log_debug("[DBG] [AC] Translating [%s]", action);

	// Stop further parsing if this is a void action
	if (strstr(action, "VOID") != NULL)
	{
		scanCodes[0] = 0; 
		return;
	}

	j = 0;
	/*
	// Function keys must be handled separately because SHIFT+Fn have unique
	// scan codes
	if (strstr("F1", action) != NULL) {
		if      (strstr(action, "SHIFT") != NULL)	scanCodes[j] = 0x54;
		else if (strstr(action, "CTRL") != NULL)	scanCodes[j] = 0x5E;
		else if (strstr(action, "ALT") != NULL)		scanCodes[j] = 0x68;
		else										scanCodes[j] = 0x3B;
		return;
	}
	// End of function keys
	*/

	// Composite keys
	ptr = action;
	if ((cursor = strstr(action, "SHIFT")) != NULL) { 	scanCodes[j++] = 0x2A; ptr = cursor + strlen("SHIFT "); }
	if ((cursor = strstr(action, "CTRL")) != NULL) {	scanCodes[j++] = 0x1D; ptr = cursor + strlen("CTRL "); }
	if ((cursor = strstr(action, "ALT")) != NULL) {		scanCodes[j++] = 0x38; ptr = cursor + strlen("ALT "); }

	// Process the function keys
	if (strstr(ptr, "F") != NULL) {
		char next = *(ptr + 1);
		if (isdigit(next)) {
			// This is a function key, convert all the digits after the F
			int numkey = atoi(ptr + 1);
			scanCodes[j++] = 0x3B + numkey - 1;
			scanCodes[j] = 0; // Terminate the scan code list
			return;
		}
	}

	// Process the arrow keys
	if (strstr(ptr, "ARROW") != NULL) 
	{
		scanCodes[j++] = 0xE0;
		if (strstr(ptr, "LEFT")  != NULL)	scanCodes[j++] = 0x4B;
		if (strstr(ptr, "RIGHT") != NULL)	scanCodes[j++] = 0x4D;
		if (strstr(ptr, "UP")    != NULL)	scanCodes[j++] = 0x48;
		if (strstr(ptr, "DOWN")  != NULL) 	scanCodes[j++] = 0x50;

		//if (strstr(ptr, "LEFT") != NULL)		scanCodes[j++] = MapVirtualKey(VK_LEFT, MAPVK_VK_TO_VSC);	// CONFIRMED: 0x4B
		//if (strstr(ptr, "RIGHT") != NULL)	scanCodes[j++] = MapVirtualKey(VK_RIGHT, MAPVK_VK_TO_VSC);	// CONFIRMED: 0x4D
		//if (strstr(ptr, "UP") != NULL)  		scanCodes[j++] = MapVirtualKey(VK_UP, MAPVK_VK_TO_VSC);		// CONFIRMED: 0x48
		//if (strstr(ptr, "DOWN") != NULL) 	scanCodes[j++] = MapVirtualKey(VK_DOWN, MAPVK_VK_TO_VSC);	// CONFIRMED: 0x50
		scanCodes[j] = 0;
		return;
	}

	// Process other special keys
	{
		if (strstr(ptr, "TAB") != NULL) // CONFIRMED: 0x0F
		{
			scanCodes[j++] = MapVirtualKey(VK_TAB, MAPVK_VK_TO_VSC);
			scanCodes[j] = 0;
			return;
		}

		if (strstr(ptr, "ENTER") != NULL)
		{
			scanCodes[j++] = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
			scanCodes[j] = 0;
			return;
		}

		if (isdigit(*ptr)) { // CONFIRMED
			int digit = *ptr - '0';
			if (digit == 0) digit += 10;
			scanCodes[j++] = 0x02 - 1 + digit;
			scanCodes[j] = 0;
			return;
		}

		if (strstr(ptr, ";") != NULL) {
			scanCodes[j++] = 0x27;
			scanCodes[j] = 0;
			return;
		}

		if (strstr(ptr, "'") != NULL) {
			scanCodes[j++] = 0x28;
			scanCodes[j] = 0;
			return;
		}

		if (strstr(ptr, "COMMA") != NULL) {
			scanCodes[j++] = 0x33;
			scanCodes[j] = 0;
			return;
		}

		if (strstr(ptr, "PERIOD") != NULL) {
			scanCodes[j++] = 0x34;
			scanCodes[j] = 0;
			return;
		}

		if (strstr(ptr, "SPACE") != NULL) {
			scanCodes[j++] = 0x39;
			scanCodes[j] = 0;
			return;
		}

		if (strstr(ptr, "HOLOGRAM") != NULL) {
			scanCodes[0] = 0xFF;
			scanCodes[1] = AC_HOLOGRAM_FAKE_VK_CODE;
			return;
		}
	}

	// Regular single-char keys
	scanCodes[j++] = (WORD)MapVirtualKey(*ptr, MAPVK_VK_TO_VSC);
	if (j < MAX_AC_ACTION_LEN)
		scanCodes[j] = 0; // Terminate the scan code list
}

void DisplayACAction(WORD *scanCodes) {
	std::stringstream ss;
	int i = 0;
	while (scanCodes[i] && i < MAX_AC_ACTION_LEN) {
		ss << std::hex << scanCodes[i] << ", ";
		i++;
	}
	std::string s = ss.str();
	log_debug("[DBG] [AC] %s", s.c_str());
}

/*
 * Loads an "action" row:
 * "action" = ACTION, x0,y0, x1,y1
 */
bool LoadACAction(char *buf, float width, float height, ac_uv_coords *coords)
{
	float x0, y0, x1, y1;
	int res = 0, idx = coords->numCoords;
	char *substr = NULL;
	char action[50];

	if (idx >= MAX_AC_COORDS_PER_TEXTURE) {
		log_debug("[DBG] [AC] Too many actions already loaded for this texture");
		return false;
	}

	substr = strchr(buf, '=');
	if (substr == NULL) {
		log_debug("[DBG] [AC] Missing '=' in '%s', skipping", buf);
		return false;
	}
	// Skip the equal sign:
	substr++;
	// Skip white space chars:
	while (*substr == ' ' || *substr == '\t')
		substr++;

	try {
		int len;

		//src_slot = -1;
		action[0] = 0;
		len = ReadNameFromLine(substr, action);
		if (len == 0)
			return false;

		// Parse the rest of the parameters
		substr += len + 1;
		res = sscanf_s(substr, "%f, %f, %f, %f", &x0, &y0, &x1, &y1);
		if (res < 4) {
			log_debug("[DBG] [DC] ERROR (skipping), expected at least 4 elements in '%s'", substr);
		}
		else {
			strcpy_s(&(coords->action_name[idx][0]), 16, action);
			TranslateACAction(&(coords->action[idx][0]), action);
			//DisplayACAction(&(coords->action[idx][0]));

			coords->area[idx].x0 = x0 / width;
			coords->area[idx].y0 = y0 / height;
			coords->area[idx].x1 = x1 / width;
			coords->area[idx].y1 = y1 / height;
			// Flip the coordinates if necessary
			if (x0 != -1.0f && x1 != -1.0f) {
				if (x0 > x1) 
				{
					// Swap coords in the X-axis:
					//float x = coords->area[idx].x0;
					//coords->area[idx].x0 = coords->area[idx].x1;
					//coords->area[idx].x1 = x;
					// Mirror the X-axis:
					coords->area[idx].x0 = 1.0f - coords->area[idx].x0;
					coords->area[idx].x1 = 1.0f - coords->area[idx].x1;
				}
			}
			if (y0 != -1.0f && y1 != -1.0f) {
				if (y0 > y1) 
				{
					// Swap coords in the Y-axis:
					//float y = coords->area[idx].y0;
					//coords->area[idx].y0 = coords->area[idx].y1;
					//coords->area[idx].y1 = y;

					// Mirror the Y-axis:
					coords->area[idx].y0 = 1.0f - coords->area[idx].y0;
					coords->area[idx].y1 = 1.0f - coords->area[idx].y1;
				}
			}
			coords->numCoords++;
		}
	}
	catch (...) {
		log_debug("[DBG] [AC] Could not read uv coords from: %s", buf);
		return false;
	}
	return true;
}

/*
 * Loads a cover_texture_size row
 */
bool LoadDCCoverTextureSize(char *buf, float *width, float *height)
{
	int res = 0;
	char *c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f", width, height);
			if (res < 2) {
				log_debug("[DBG] [DC] Error (skipping), expected at least 2 elements in '%s'", c);
			}
		}
		catch (...) {
			log_debug("[DBG] [DC] Could not read 'width, height' from: %s", buf);
			return false;
		}
	}
	return true;
}

/*
 * Clears all the move_region commands
 */
static inline void ClearDCMoveRegions() {
	g_DCMoveRegions.numCoords = 0;
}

/*
 * Loads a move_region row into g_DCMoveRegions:
 * move_region = region, x0,y0,x1,y1
 */
bool LoadDCMoveRegion(char *buf)
{
	float x0, y0, x1, y1;
	int region_slot;
	int res = 0, idx = g_DCMoveRegions.numCoords;
	char *substr = NULL;
	char region_name[50];

	if (idx >= MAX_HUD_BOXES) {
		log_debug("[DBG] [DC] Too many move_region commands");
		return false;
	}

	substr = strchr(buf, '=');
	if (substr == NULL) {
		log_debug("[DBG] [DC] Missing '=' in '%s', skipping", buf);
		return false;
	}
	// Skip the equal sign:
	substr++;
	// Skip white space chars:
	while (*substr == ' ' || *substr == '\t')
		substr++;

	try {
		int len;

		region_slot = -1;
		region_name[0] = 0;
		len = ReadNameFromLine(substr, region_name);
		if (len == 0)
			return false;

		if (strstr(region_name, "REGION") == NULL) {
			log_debug("[DBG] [DC] ERROR: Invalid region name: [%s]", region_name);
			return false;
		}

		region_slot = HUDRegionNameToIndex(region_name);
		if (region_slot < 0) {
			log_debug("[DBG] [DC] ERROR: Could not find region named: [%s]", region_name);
			return false;
		}

		// Parse the rest of the parameters
		substr += len + 1;
		res = sscanf_s(substr, "%f, %f, %f, %f", &x0, &y0, &x1, &y1);
		if (res < 4) {
			log_debug("[DBG] [DC] ERROR (skipping), expected at least 5 elements in '%s'", substr);
		}
		else {
			g_DCMoveRegions.region_slot[idx] = region_slot;
			g_DCMoveRegions.dst[idx].x0 = x0;
			g_DCMoveRegions.dst[idx].y0 = y0;
			g_DCMoveRegions.dst[idx].x1 = x1;
			g_DCMoveRegions.dst[idx].y1 = y1;
			g_DCMoveRegions.numCoords++;

			log_debug("[DBG] [DC] move_region [%s=%d], (%0.3f, %0.3f)-(%0.3f, %0.3f)",
				region_name, region_slot, x0, y0, x1, y1);
		}
	}
	catch (...) {
		log_debug("[DBG] [DC] Could not read coords from: %s", buf);
		return false;
	}
	return true;
}

/*
 * Convert a cockpit name into a DC params file of the form:
 * DynamicCockpit\<CockpitName>.dc
 */
void CockpitNameToDCParamsFile(char *CockpitName, char *sFileName, int iFileNameSize) {
	snprintf(sFileName, iFileNameSize, "DynamicCockpit\\%s.dc", CockpitName);
}

/*
 * Convert a cockpit name into an AC params file of the form:
 * DynamicCockpit\<CockpitName>.ac
 */
void CockpitNameToACParamsFile(char *CockpitName, char *sFileName, int iFileNameSize) {
	snprintf(sFileName, iFileNameSize, "DynamicCockpit\\%s.ac", CockpitName);
}

/*
 * Convert an OPT name into a MAT params file of the form:
 * Materials\<OPTName>.mat
 */
void OPTNameToMATParamsFile(char *OPTName, char *sFileName, int iFileNameSize) {
	snprintf(sFileName, iFileNameSize, "Materials\\%s.mat", OPTName);
}

bool GetGroupIdImageIdFromDATName(char *DATName, int *GroupId, int *ImageId) {
	// Extract the Group Id and Image Id from the dat name:
	// Sample dat name:
	// [dat,9002,1200,0,0]
	char *idx = strstr(DATName, "dat,");
	if (idx == NULL) {
		log_debug("[DBG] [MAT] Could not find 'dat,' substring in [%s]", DATName);
		return false;
	}
	idx += 4; // Skip the "dat," token, we're now pointing to the first char of the Group Id
	*GroupId = atoi(idx); // Convert the current string to the Group Id

	// Advance to the next comma
	idx = strstr(idx, ",");
	if (idx == NULL) {
		log_debug("[DBG] [MAT] Error while parsing [%s], could not find ImageId", DATName);
		return false;
	}
	idx += 1; // Skip the comma
	*ImageId = atoi(idx); // Convert the current string to the Image Id
	return true;
}

/*
 * Convert a DAT name into two MAT params files of the form:
 *
 * sFileName:		Materials\dat-<GroupId>-<ImageId>.mat
 * sFileNameShort:	Materials\dat-<GroupId>.mat
 *
 * Both sFileName and sFileNameShort must have iFileNameSize bytes to store the string.
 * sFileNameShort can be used to load mat files for animations, like explosions.
 */
void DATNameToMATParamsFile(char *DATName, char *sFileName, char *sFileNameShort, int iFileNameSize) {
	int GroupId, ImageId;
	// Get the GroupId and ImageId from the DATName, nullify sFileName if we can't extract
	// the data from the name
	if (!GetGroupIdImageIdFromDATName(DATName, &GroupId, &ImageId)) {
		sFileName[0] = 0;
		return;
	}
	// Build the material filename for the DAT texture:
	snprintf(sFileName, iFileNameSize, "Materials\\dat-%d-%d.mat", GroupId, ImageId);
	snprintf(sFileNameShort, iFileNameSize, "Materials\\dat-%d.mat", GroupId);
}

/*
 * Loads a Light color row
 */
bool LoadLightColor(char *buf, Vector3 *Light)
{
	int res = 0;
	char *c = NULL;
	float r, g, b;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f", &r, &g, &b);
			if (res < 3) {
				log_debug("[DBG] [DC] Error (skipping), expected at least 3 elements in '%s'", c);
			}
			Light->x = r;
			Light->y = g;
			Light->z = b;
		}
		catch (...) {
			log_debug("[DBG] [DC] Could not read 'r, g, b' from: %s", buf);
			return false;
		}
	}
	return true;
}

/*
 * Loads an UV coord row
 */
bool LoadUVCoord(char *buf, Vector2 *UVCoord)
{
	int res = 0;
	char *c = NULL;
	float x, y;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f", &x, &y);
			if (res < 2) {
				log_debug("[DBG] [MAT] Error (skipping), expected at least 2 elements in '%s'", c);
			}
			UVCoord->x = x;
			UVCoord->y = y;
		}
		catch (...) {
			log_debug("[DBG] [MAT] Could not read 'x, y' from: %s", buf);
			return false;
		}
	}
	return true;
}

void ReadMaterialLine(char *buf, Material *curMaterial) {
	char param[256], svalue[256]; // texname[MAX_TEXNAME];
	float fValue = 0.0f;

	// Skip comments and blank lines
	if (buf[0] == ';' || buf[0] == '#')
		return;
	if (strlen(buf) == 0)
		return;

	// Read the parameter
	if (sscanf_s(buf, "%s = %s", param, 256, svalue, 256) > 0) {
		fValue = (float)atof(svalue);
	}

	if (_stricmp(param, "Metallic") == 0) {
		//log_debug("[DBG] [MAT] Metallic: %0.3f", fValue);
		curMaterial->Metallic = fValue;
	}
	else if (_stricmp(param, "Intensity") == 0) {
		//log_debug("[DBG] [MAT] Intensity: %0.3f", fValue);
		curMaterial->Intensity = fValue;
	}
	else if (_stricmp(param, "Glossiness") == 0) {
		//log_debug("[DBG] [MAT] Glossiness: %0.3f", fValue);
		curMaterial->Glossiness = fValue;
	}
	else if (_stricmp(param, "NMIntensity") == 0) {
		curMaterial->NMIntensity = fValue;
	}
	else if (_stricmp(param, "SpecularVal") == 0) {
		curMaterial->SpecValue = fValue;
	}
	else if (_stricmp(param, "Shadeless") == 0) {
		curMaterial->IsShadeless = (bool)fValue;
		log_debug("[DBG] Shadeless texture loaded");
	}
	else if (_stricmp(param, "Light") == 0) {
		LoadLightColor(buf, &(curMaterial->Light));
		//log_debug("[DBG] [MAT] Light: %0.3f, %0.3f, %0.3f",
		//	curMaterialTexDef.material.Light.x, curMaterialTexDef.material.Light.y, curMaterialTexDef.material.Light.z);
	}
	else if (_stricmp(param, "light_uv_coord_pos") == 0) {
		LoadUVCoord(buf, &(curMaterial->LightUVCoordPos));
		//log_debug("[DBG] [MAT] LightUVCoordPos: %0.3f, %0.3f",
		//	curMaterial->LightUVCoordPos.x, curMaterial->LightUVCoordPos.y);
	}
	else if (_stricmp(param, "NoBloom") == 0) {
		curMaterial->NoBloom = (bool)fValue;
		log_debug("[DBG] NoBloom: %d", curMaterial->NoBloom);
	}
	// Shadow Mapping settings
	else if (_stricmp(param, "shadow_map_mult_x") == 0) {
		g_ShadowMapping.shadow_map_mult_x = fValue;
		log_debug("[DBG] [SHW] shadow_map_mult_x: %0.3f", fValue);
	}
	else if (_stricmp(param, "shadow_map_mult_y") == 0) {
		g_ShadowMapping.shadow_map_mult_y = fValue;
		log_debug("[DBG] [SHW] shadow_map_mult_y: %0.3f", fValue);
	}
	else if (_stricmp(param, "shadow_map_mult_z") == 0) {
		g_ShadowMapping.shadow_map_mult_z = fValue;
		log_debug("[DBG] [SHW] shadow_map_mult_z: %0.3f", fValue);
	}
	// Lava Settings
	else if (_stricmp(param, "Lava") == 0) {
		curMaterial->IsLava = (bool)fValue;
	}
	else if (_stricmp(param, "LavaSpeed") == 0) {
		curMaterial->LavaSpeed = fValue;
	}
	else if (_stricmp(param, "LavaSize") == 0) {
		curMaterial->LavaSize = fValue;
	}
	else if (_stricmp(param, "EffectBloom") == 0) {
		// Used for specific effects where specifying bloom is difficult, like the Lava
		// and the AlphaToBloom shaders.
		curMaterial->EffectBloom = fValue;
	}
	else if (_stricmp(param, "LavaColor") == 0) {
		LoadLightColor(buf, &(curMaterial->LavaColor));
	}
	else if (_stricmp(param, "LavaTiling") == 0) {
		curMaterial->LavaTiling = (bool)fValue;
	}
	else if (_stricmp(param, "AlphaToBloom") == 0) {
		// Uses the color alpha to apply bloom. Can be used to force surfaces to glow
		// when they don't have an illumination texture.
		curMaterial->AlphaToBloom = (bool)fValue;
	}
	else if (_stricmp(param, "NoColorAlpha") == 0) {
		// Forces color alpha to 0. Only takes effect when AlphaToBloom is set.
		curMaterial->NoColorAlpha = (bool)fValue;
	}
	else if (_stricmp(param, "AlphaIsntGlass") == 0) {
		// When set, semi-transparent areas aren't converted to glass
		curMaterial->AlphaIsntGlass = (bool)fValue;
	}
	else if (_stricmp(param, "Ambient") == 0) {
		// Additional ambient component. Only used in PixelShaderNoGlass
		curMaterial->Ambient = fValue;
	}
	else if (_stricmp(param, "TotalFrames") == 0) {
		curMaterial->TotalFrames = (int)fValue;
	}
	else if (_stricmp(param, "ExplosionScale") == 0) {
		// The user-facing explosion scale must be translated into the scale that the
		// ExplosionShader users. For some reason (the code isn't mine) the scale in
		// the shader works like this:
		// ExplosionScale: 4.0 == small, 2.0 == normal, 1.0 == big.
		// So the formula is:
		// ExplosionScale = 4.0 / UserExplosionScale
		// Because:
		// 4.0 / UserExplosionScale yields:
		// 4.0 / 1.0 -> 4 = small
		// 4.0 / 2.0 -> 2 = medium
		// 4.0 / 4.0 -> 1 = big
		float UserExplosionScale = max(0.5f, fValue); // Disallow values smaller than 0.5
		curMaterial->ExplosionScale = 4.0f / UserExplosionScale;
	}
	else if (_stricmp(param, "ExplosionSpeed") == 0) {
		curMaterial->ExplosionSpeed = fValue;
	}
	else if (_stricmp(param, "ExplosionBlendMode") == 0) {
		// 0: No Blending, use the original texture
		// 1: Blend with the original texture
		// 2: Replace original texture with procedural explosion
		curMaterial->ExplosionBlendMode = (int)fValue;
	}
	/*
	else if (_stricmp(param, "LavaNormalMult") == 0) {
		LoadLightColor(buf, &(curMaterial->LavaNormalMult));
		log_debug("[DBG] [MAT] LavaNormalMult: %0.3f, %0.3f, %0.3f",
			curMaterial->LavaNormalMult.x, curMaterial->LavaNormalMult.y, curMaterial->LavaNormalMult.z);
	}
	else if (_stricmp(param, "LavaPosMult") == 0) {
		LoadLightColor(buf, &(curMaterial->LavaPosMult));
		log_debug("[DBG] [MAT] LavaPosMult: %0.3f, %0.3f, %0.3f",
			curMaterial->LavaPosMult.x, curMaterial->LavaPosMult.y, curMaterial->LavaPosMult.z);
	}
	else if (_stricmp(param, "LavaTranspose") == 0) {
		curMaterial->LavaTranspose = (bool)fValue;
	}
	*/
}

/*
 * Load the material parameters for an individual OPT or DAT
 */
bool LoadIndividualMATParams(char *OPTname, char *sFileName, bool verbose=true) {
	// I may have to use std::array<char, DIM> and std::vector<std::array<char, Dim>> instead
	// of TexnameType
	// https://stackoverflow.com/questions/21829451/push-back-on-a-vector-of-array-of-char
	FILE *file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		//log_debug("[DBG] [MAT] Could not load [%s]", sFileName);
	}

	if (error != 0) {
		//log_debug("[DBG] [MAT] Error %d when loading [%s]", error, sFileName);
		return false;
	}

	if (verbose) log_debug("[DBG] [MAT] Loading Craft Material params for [%s]...", sFileName);
	char buf[256], param[256], svalue[256]; // texname[MAX_TEXNAME];
	std::vector<TexnameType> texnameList;
	int param_read_count = 0;
	float fValue = 0.0f;
	MaterialTexDef curMaterialTexDef;

	// Find this OPT in the global materials and clear it if necessary...
	int craftIdx = FindCraftMaterial(OPTname);
	if (craftIdx < 0) {
		// New Craft Material
		if (verbose) log_debug("[DBG] [MAT] New Craft Material (%s)", OPTname);
		//craftIdx = g_Materials.size();
	}
	else {
		// Existing Craft Material, clear it
		if (verbose) log_debug("[DBG] [MAT] Existing Craft Material, clearing %s", OPTname);
		g_Materials[craftIdx].MaterialList.clear();
	}
	CraftMaterials craftMat;
	// Clear the materials for this craft and add a default material
	craftMat.MaterialList.clear();
	strncpy_s(craftMat.OPTname, OPTname, MAX_OPT_NAME);

	curMaterialTexDef.material = g_DefaultGlobalMaterial;
	strncpy_s(curMaterialTexDef.texname, "Default", MAX_TEXNAME);
	// The default material will always be in slot 0:
	craftMat.MaterialList.push_back(curMaterialTexDef);
	//craftMat.MaterialList.insert(craftMat.MaterialList.begin(), materialTexDef);

	// We always start the craft material with one material: the default material in slot 0
	bool MaterialSaved = true;
	texnameList.clear();
	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 256, svalue, 256) > 0) {
			fValue = (float)atof(svalue);

			if (buf[0] == '[') {

				//strcpy_s(texname, MAX_TEXNAME, buf + 1);
				// Get rid of the trailing ']'
				//char *end = strstr(texname, "]");
				//if (end != NULL)
				//	*end = 0;
				//log_debug("[DBG] [MAT] texname: %s", texname);

				if (!MaterialSaved) {
					// There's an existing material that needs to be saved before proceeding
					for (TexnameType texname : texnameList) {
						// Copy the texture name from the list to the current material
						strncpy_s(curMaterialTexDef.texname, texname.name, MAX_TEXNAME);
						// Special case: overwrite the default material
						if (_stricmp("default", curMaterialTexDef.texname) == 0) {
							//log_debug("[DBG] [MAT] Overwriting the default material for this craft");
							craftMat.MaterialList[0] = curMaterialTexDef;
						}
						else {
							//log_debug("[DBG] [MAT] Adding new material: %s", curMaterialTexDef.texname);
							craftMat.MaterialList.push_back(curMaterialTexDef);
						}
					}
				}
				texnameList.clear();

				// Extract the name(s) of the texture(s)
				{
					char *start = buf + 1; // Skip the '[' bracket
					char *end;
					while (*start && *start != ']') {
						end = start;
						// Skip chars until we find ',' ']' or EOS
						while (*end && *end != ',' && *end != ']')
							end++;
						// If we didn't hit EOS, then add the token to the list
						if (*end)
						{
							TexnameType texname;
							strncpy_s(texname.name, start, end - start);
							//log_debug("[DBG] [MAT] Adding texname: [%s]", texname.texname);
							texnameList.push_back(texname);
							start = end + 1;
						}
						else
							start = end;
					}

					// DEBUG
					/*log_debug("[DBG] [MAT] ---------------");
					for (uint32_t i = 0; i < texnameList.size(); i++)
						log_debug("[DBG] [MAT] %s, ", texnameList[i].name);
					log_debug("[DBG] [MAT] ===============");*/
					// DEBUG
				}

				// Start a new material
				//strncpy_s(curMaterialTexDef.texname, texname, MAX_TEXNAME);
				curMaterialTexDef.texname[0] = 0;
				curMaterialTexDef.material = g_DefaultGlobalMaterial;
				MaterialSaved = false;
			}
			else
				ReadMaterialLine(buf, &(curMaterialTexDef.material));
		}
	}
	fclose(file);

	// Save the last material if necessary...
	if (!MaterialSaved) {
		// There's an existing material that needs to be saved before proceeding
		for (TexnameType texname : texnameList) {
			// Copy the texture name from the list to the current material
			strncpy_s(curMaterialTexDef.texname, texname.name, MAX_TEXNAME);
			// Special case: overwrite the default material
			if (_stricmp("default", curMaterialTexDef.texname) == 0) {
				//log_debug("[DBG] [MAT] (last) Overwriting the default material for this craft");
				craftMat.MaterialList[0] = curMaterialTexDef;
			}
			else {
				//log_debug("[DBG] [MAT] (last) Adding new material: %s", curMaterialTexDef.texname);
				craftMat.MaterialList.push_back(curMaterialTexDef);
			}
		}
	}
	texnameList.clear();

	// Replace the craft material in g_Materials
	if (craftIdx < 0) {
		//log_debug("[DBG] [MAT] Adding new craft material %s", OPTname);
		g_Materials.push_back(craftMat);
		craftIdx = g_Materials.size() - 1;
	}
	else {
		//log_debug("[DBG] [MAT] Replacing existing craft material %s", OPTname);
		g_Materials[craftIdx] = craftMat;
	}

	// DEBUG
	// Print out the materials for this craft
	/*log_debug("[DBG] [MAT] *********************");
	log_debug("[DBG] [MAT] Craft Materials for OPT: %s", g_Materials[craftIdx].OPTname);
	for (uint32_t i = 0; i < g_Materials[craftIdx].MaterialList.size(); i++) {
		Material defMat = g_Materials[craftIdx].MaterialList[i].material;
		log_debug("[DBG] [MAT] %s, M:%0.3f, I:%0.3f, G:%0.3f",
			g_Materials[craftIdx].MaterialList[i].texname,
			defMat.Metallic, defMat.Intensity, defMat.Glossiness);
	}
	log_debug("[DBG] [MAT] *********************");*/
	// DEBUG
	return true;
}

void CycleFOVSetting()
{
	// Don't change the FOV in VR mode: use your head to look around!
	if (g_bEnableVR) {
		g_CurrentFOVType = GLOBAL_FOV;
	}
	else {
		switch (g_CurrentFOVType) {
		case GLOBAL_FOV:
			g_CurrentFOVType = XWAHACKER_FOV;
			log_debug("[DBG] [FOV] Current FOV: xwahacker_fov");
			DisplayTimedMessage(4, 0, "Current Craft FOV");
			break;
		case XWAHACKER_FOV:
			g_CurrentFOVType = XWAHACKER_LARGE_FOV;
			log_debug("[DBG] [FOV] Current FOV: xwahacker_large_fov");
			DisplayTimedMessage(4, 0, "Current Craft Large FOV");
			break;
		case XWAHACKER_LARGE_FOV:
			g_CurrentFOVType = GLOBAL_FOV;
			log_debug("[DBG] [FOV] Current FOV: GLOBAL");
			DisplayTimedMessage(4, 0, "Global FOV");
			break;
		}

		// Apply the current FOV and recompute FOV-related parameters
		g_bYCenterHasBeenFixed = false;
		g_bCustomFOVApplied = false;
	}
}

/*
 * Saves the current FOV to the current dc file -- if it exists.
 * Depending on the current g_CurrentFOV, it will write "xwahacker_fov" or
 * "xwahacker_large_fov".
 */
bool UpdateXWAHackerFOV()
{
	char sFileName[256], *sTempFileName = "./TempDCFile.dc";
	FILE *in_file, *out_file;
	int error = 0, line = 0;
	char buf[256];
	char *FOVname = NULL;

	switch (g_CurrentFOVType) {
	case GLOBAL_FOV:
		return false;
	case XWAHACKER_FOV:
		FOVname = "xwahacker_fov";
		break;
	case XWAHACKER_LARGE_FOV:
		FOVname = "xwahacker_large_fov";
		break;
	}
	if (FOVname == NULL) {
		log_debug("[DBG] [FOV] FOVname is NULL! Aborting UpdateXWAHackerFOV");
		return false;
	}

	if (strlen(g_sCurrentCockpit) <= 0) {
		log_debug("[DBG] [DC] No DC-enabled cockpit has been loaded, will not write current FOV");
		return false;
	}

	snprintf(sFileName, 256, ".\\DynamicCockpit\\%s.dc", g_sCurrentCockpit);
	log_debug("[DBG] Saving current FOV to Dynamic Cockpit params file [%s]...", sFileName);
	// Open sFileName for reading
	try {
		error = fopen_s(&in_file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not read [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when reading [%s]", error, sFileName);
		return false;
	}

	// Create sTempFileName
	try {
		error = fopen_s(&out_file, sTempFileName, "wt");
	}
	catch (...) {
		log_debug("[DBG] Could not create temporary file: [%s]", sTempFileName);
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when creating [%s]", error, sTempFileName);
		return false;
	}

	bool bFOVWritten = false;
	float focal_length = *g_fRawFOVDist;
	float FOV = atan2(g_fCurInGameHeight, focal_length) * 2.0f;
	// Convert radians to degrees:
	FOV *= 180.0f / 3.141592f;
	log_debug("[DBG] [FOV] FOVType: %d, FOV that will be saved: %s = %0.3f", g_CurrentFOVType, FOVname, FOV);
	while (fgets(buf, 256, in_file) != NULL) {
		line++;
		// Commented lines are automatically pass-through
		if (buf[0] == ';') {
			fprintf(out_file, buf);
			continue;
		}

		if (strstr(buf, FOVname) != NULL) {
			fprintf(out_file, "%s = %0.3f\n", FOVname, FOV);
			bFOVWritten = true;
		}
		else
			fprintf(out_file, buf);
	}

	// This DC file may not have the "xwahacker_fov" line, so let's add it:
	if (!bFOVWritten)
		fprintf(out_file, "\n%s = %0.3f\n", FOVname, FOV);
	
	fclose(out_file);
	fclose(in_file);

	// Swap the files
	remove(sFileName);
	rename(sTempFileName, sFileName);
	return true;
}

/*
 * bOverwriteCurrentShipFOV should be disabled when reading the DC file, so that
 * we don't overwrite both xwahacker FOVs with the same value.
 * bCompensateFOVfor1920x1080 should be true only when reading the DC file: compensating
 * every time the FOV is adjusted has some adverse effects when the in-game resolution is
 * lower than 1920x1080
 */
float SetCurrentShipFOV(float FOV, bool bOverwriteCurrentShipFOV, bool bCompensateFOVfor1920x1080)
{
	float FocalLength;
	// Prevent nonsensical values:
	if (FOV < 15.0f) FOV = 15.0f;
	if (FOV > 170.0f) FOV = 170.0f;
	//DisplayTimedMessageV(3, 1, "FOV: %0.1f", FOV);
	// Convert to radians
	FOV = FOV * 3.141592f / 180.0f;
	// This formula matches what Jeremy posted:
	FocalLength = g_fCurInGameHeight / tan(FOV / 2.0f);
	if (bCompensateFOVfor1920x1080) {
		// Compute the focal length that would be applied in 1920x1080
		float DFocalLength = 1080.0f / tan(FOV / 2.0f);
		// Compute the real HFOV and desired HFOV:
		float HFOV = 2.0f * atan2(0.5f * g_fCurInGameWidth, FocalLength);
		float DHFOV = 2.0f * atan2(0.5f * 1920.0f, DFocalLength);
		log_debug("[DBG] [FOV] [DC] HFOV: %0.3f, DHFOV: %0.3f", HFOV / DEG2RAD, DHFOV / DEG2RAD);
		// If the actual HFOV is lower than the desired HFOV, then we need to adjust:
		if (HFOV < DHFOV) {
			log_debug("[DBG] [FOV] [DC] ADJUSTING FOV. Original regular focal length: %0.3f", FocalLength);
			FocalLength = 0.5f * g_fCurInGameWidth / tan(0.5f * DHFOV);
			log_debug("[DBG] [FOV] [DC] ADJUSTING FOV. ADJUSTED regular focal length: %0.3f", FocalLength);
		}
	}

	if (bOverwriteCurrentShipFOV) {
		switch (g_CurrentFOVType) {
		case XWAHACKER_FOV:
			g_fCurrentShipFocalLength = FocalLength;
			break;
		case XWAHACKER_LARGE_FOV:
			g_fCurrentShipLargeFocalLength = FocalLength;
			break;
		}
	}
	return FocalLength;
}

/*
 * Load the DC params for an individual cockpit. 
 * Resets g_DCElements (if we're not rendering in 3D), and the move regions.
 */
bool LoadIndividualDCParams(char *sFileName) {
	log_debug("[DBG] Loading Dynamic Cockpit params for [%s]...", sFileName);
	FILE *file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;
	float cover_tex_width = 1, cover_tex_height = 1;
	const float DEG2RAD = 3.141593f / 180.0f;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading [%s]", error, sFileName);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Reset the dynamic cockpit vector if we're not rendering in 3D
	//if (!g_bRendering3D && g_DCElements.size() > 0) {
	//	log_debug("[DBG] [DC] Clearing g_DCElements");
	//	ClearDynCockpitVector(g_DCElements);
	//}
	//ClearDCMoveRegions();
	g_DCTargetingColor.w = 0.0f; // Reset the targeting mesh color
	// Do not re-render Missiles/Counters from first principles by default:
	g_bReRenderMissilesNCounterMeasures = false;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (buf[0] == '[') {
				// This is a new DC element.
				dc_element dc_elem;
				strcpy_s(dc_elem.name, MAX_TEXTURE_NAME, buf + 1);
				// Get rid of the trailing ']'
				char *end = strstr(dc_elem.name, "]");
				if (end != NULL)
					*end = 0;
				// See if we have this DC element already
				lastDCElemSelected = isInVector(dc_elem.name, g_DCElements, g_iNumDCElements);
				//log_debug("[DBG] [DC] New dc_elem.name: [%s], idx: %d",
				//	dc_elem.name, lastDCElemSelected);
				if (lastDCElemSelected > -1) {
					g_DCElements[lastDCElemSelected].coords.numCoords = 0;
					g_DCElements[lastDCElemSelected].num_erase_slots = 0;
					log_debug("[DBG] [DC] Resetting coords of existing DC elem @ idx: %d", lastDCElemSelected);
				}
				else if (g_iNumDCElements < MAX_DC_SRC_ELEMENTS) {
					// Initialize this dc_elem:
					dc_elem.coverTextureName[0] = 0;
					// TODO: Replace the line below with the proper command now that the
					// coverTextures live inside DeviceResources
					//dc_elem.coverTexture = nullptr;
					dc_elem.coords = { 0 };
					dc_elem.num_erase_slots = 0;
					dc_elem.bActive = false;
					dc_elem.bNameHasBeenTested = false;
					dc_elem.bHologram = false;
					dc_elem.bNoisyHolo = false;
					dc_elem.bTransparent = false;
					//g_DCElements.push_back(dc_elem);
					g_DCElements[g_iNumDCElements] = dc_elem;
					//lastDCElemSelected = (int)g_DCElements.size() - 1;
					lastDCElemSelected = g_iNumDCElements;
					g_iNumDCElements++;
					//log_debug("[DBG] [DC] Added new dc_elem, count: %d", g_iNumDCElements);
				}
				else {
					if (g_iNumDCElements >= MAX_DC_SRC_ELEMENTS)
						log_debug("[DBG] [DC] ERROR: Max g_iNumDCElements: %d reached", g_iNumDCElements);
				}
			}
			else if (_stricmp(param, UV_COORDS_DCPARAM) == 0) {
				//if (g_DCElements.size() == 0) {
				if (g_iNumDCElements == 0) {
					log_debug("[DBG] [DC] ERROR. Line %d, g_DCElements is empty, cannot add %s", line, param, UV_COORDS_DCPARAM);
					continue;
				}
				if (lastDCElemSelected == -1) {
					log_debug("[DBG] [DC] ERROR. Line %d, %s without a corresponding texture section.", line, UV_COORDS_DCPARAM);
					continue;
				}
				LoadDCUVCoords(buf, cover_tex_width, cover_tex_height, &(g_DCElements[lastDCElemSelected].coords));
			}
			/*
			else if (_stricmp(param, MOVE_REGION_DCPARAM) == 0) {
				LoadDCMoveRegion(buf);
			}
			*/
			else if (_stricmp(param, ERASE_REGION_DCPARAM) == 0) {
				//if (g_DCElements.size() == 0) {
				if (g_iNumDCElements == 0) {
					log_debug("[DBG] [DC] ERROR. Line %d, g_DCElements is empty, cannot add %s", line, param, ERASE_REGION_DCPARAM);
					continue;
				}
				if (lastDCElemSelected == -1) {
					log_debug("[DBG] [DC] ERROR. Line %d, %s without a corresponding texture section.", line, ERASE_REGION_DCPARAM);
					continue;
				}
				// The name of the region must contain the word "REGION":
				if (strstr(svalue, "REGION") == NULL) {
					log_debug("[DBG] [DC] ERROR: Invalid region name: [%s]", svalue);
					continue;
				}

				int slot = HUDRegionNameToIndex(svalue);
				if (slot < 0) {
					log_debug("[DBG] [DC] ERROR: Unknown region: [%s]", svalue);
					continue;
				}

				if (slot < (int)g_DCHUDRegions.boxes.size()) {
					int next_idx = g_DCElements[lastDCElemSelected].num_erase_slots;
					g_DCElements[lastDCElemSelected].erase_slots[next_idx] = slot;
					g_DCElements[lastDCElemSelected].num_erase_slots++;
					//log_debug("[DBG] [DC] Added erase slot [%s] to DCElem %d", svalue,
					//	lastDCElemSelected);
				}
				else
					log_debug("[DBG] [DC] WARNING: erase_region = %d IGNORED: Not enough g_DCElemSrcBoxes", slot);
			}
			else if (_stricmp(param, COVER_TEX_NAME_DCPARAM) == 0) {
				if (lastDCElemSelected == -1) {
					log_debug("[DBG] [DC] ERROR. Line %d, %s without a corresponding texture section.", line, COVER_TEX_NAME_DCPARAM);
					continue;
				}
				strcpy_s(g_DCElements[lastDCElemSelected].coverTextureName, MAX_TEXTURE_NAME, svalue);
			}
			else if (_stricmp(param, COVER_TEX_SIZE_DCPARAM) == 0) {
				LoadDCCoverTextureSize(buf, &cover_tex_width, &cover_tex_height);
			}
			else if (_stricmp(param, "xwahacker_fov") == 0) {
				log_debug("[DBG] [FOV] [DC] XWA HACKER FOV: %0.3f", fValue);
				g_fCurrentShipFocalLength = SetCurrentShipFOV(fValue, false, g_bCompensateFOVfor1920x1080);
				log_debug("[DBG] [FOV] [DC] XWA HACKER FOCAL LENGTH: %0.3f", g_fCurrentShipFocalLength);
				g_CurrentFOVType = g_bEnableVR ? GLOBAL_FOV : XWAHACKER_FOV;
				// Force the new FOV to be applied
				g_bCustomFOVApplied = false;
			}
			else if (_stricmp(param, "xwahacker_large_fov") == 0) {
				log_debug("[DBG] [FOV] [DC] XWA HACKER LARGE FOV: %0.3f", fValue);
				// SetCurrentShipFOV can overwrite g_fCurrentShipFocalLength and g_fCurrentShipLargeFocalLength because
				// we need that when increasing the FOV using hotkeys. However, we must not overwrite those values here
				// because the current FOV at this point is XWAHACKER, so we end up writing the same value to both FOVs
				g_fCurrentShipLargeFocalLength = SetCurrentShipFOV(fValue, false, g_bCompensateFOVfor1920x1080);
				log_debug("[DBG] [FOV] [DC] XWA HACKER LARGE FOCAL LENGTH: %0.3f", g_fCurrentShipLargeFocalLength);
				g_CurrentFOVType = g_bEnableVR ? GLOBAL_FOV : XWAHACKER_FOV; // This is *NOT* an error, I want the default to be XWAHACKER_FOV
				// Force the new FOV to be applied
				g_bCustomFOVApplied = false;
			}
			else if (_stricmp(param, "wireframe_mesh_color") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingColor.x = x;
					g_DCTargetingColor.y = y;
					g_DCTargetingColor.z = z;
					g_DCTargetingColor.w = 1.0f;
				}
			}
			else if (_stricmp(param, "hologram") == 0) {
				g_DCElements[lastDCElemSelected].bHologram = (bool)fValue;
			}
			else if (_stricmp(param, "noisy_hologram") == 0) {
				g_DCElements[lastDCElemSelected].bNoisyHolo = (bool)fValue;
				log_debug("[DBG] noisy hologram");
			}
			else if (_stricmp(param, "fix_missles_countermeasures_text") == 0) {
				g_bReRenderMissilesNCounterMeasures = (bool)fValue;
			}
			else if (_stricmp(param, "transparent") == 0) {
				g_DCElements[lastDCElemSelected].bTransparent = (bool)fValue;
			}
		}
	}
	fclose(file);
	return true;
}

/*
 * Load the DC params for an individual cockpit.
 * Resets g_DCElements (if we're not rendering in 3D), and the move regions.
 */
bool LoadIndividualACParams(char *sFileName) {
	log_debug("[DBG] [AC] Loading Active Cockpit params for [%s]...", sFileName);
	FILE *file;
	int error = 0, line = 0;
	static int lastACElemSelected = -1;
	float tex_width = 1, tex_height = 1;

	// Reset the Active Cockpit elements? We probably don't need this, because the code modifies existing
	// AC slots
	//g_iNumACElements = 0;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load [%s]", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading [%s]", error, sFileName);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float value = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			value = (float)atof(svalue);

			if (buf[0] == '[') {
				// This is a new AC element.
				ac_element ac_elem;
				strcpy_s(ac_elem.name, MAX_TEXTURE_NAME, buf + 1);
				// Get rid of the trailing ']'
				char *end = strstr(ac_elem.name, "]");
				if (end != NULL)
					*end = 0;
				// See if we have this AC element already
				lastACElemSelected = isInVector(ac_elem.name, g_ACElements, g_iNumACElements);
				if (lastACElemSelected > -1) {
					g_ACElements[lastACElemSelected].coords.numCoords = 0;
					log_debug("[DBG] [AC] Resetting coords of existing AC elem @ idx: %d", lastACElemSelected);
				}
				else if (g_iNumACElements < MAX_AC_TEXTURES_PER_COCKPIT) {
					//log_debug("[DBG] [AC] New ac_elem.name: [%s], id: %d",
					//	ac_elem.name, g_iNumACElements);
					//ac_elem.idx = g_iNumACElements; // Generate a unique ID
					ac_elem.coords = { 0 };
					ac_elem.bActive = false;
					ac_elem.bNameHasBeenTested = false;
					g_ACElements[g_iNumACElements] = ac_elem;
					lastACElemSelected = g_iNumACElements;
					g_iNumACElements++;
					//log_debug("[DBG] [AC] Added new ac_elem, count: %d", g_iNumACElements);
				}
				else {
					if (g_iNumACElements >= MAX_AC_TEXTURES_PER_COCKPIT)
						log_debug("[DBG] [AC] ERROR: Max g_iNumACElements: %d reached", g_iNumACElements);
				}
			}
			else if (_stricmp(param, "texture_size") == 0) {
				// We can re-use LoadDCCoverTextureSize here, it's the same format (but different tag)
				LoadDCCoverTextureSize(buf, &tex_width, &tex_height);
				//log_debug("[DBG] [AC] texture size: %0.3f, %0.3f", tex_width, tex_height);
			}
			else if (_stricmp(param, "action") == 0) {
				if (g_iNumACElements == 0) {
					log_debug("[DBG] [AC] ERROR. Line %d, g_ACElements is empty, cannot add action", line);
					continue;
				}
				if (lastACElemSelected == -1) {
					log_debug("[DBG] [AC] ERROR. Line %d, 'action' tag without a corresponding texture section.", line);
					continue;
				}
				LoadACAction(buf, tex_width, tex_height, &(g_ACElements[lastACElemSelected].coords));
				// DEBUG
				/*g_ACElements[lastACElemSelected].width = (short)tex_width;
				g_ACElements[lastACElemSelected].height = (short)tex_height;
				ac_uv_coords *coords = &(g_ACElements[lastACElemSelected].coords);
				int idx = coords->numCoords - 1;
				log_debug("[DBG] [AC] Action: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
					coords->area[idx].x0, coords->area[idx].y0,
					coords->area[idx].x1, coords->area[idx].y1);*/
				// DEBUG
			}
			
		}
	}
	fclose(file);
	return true;
}

/* Loads the dynamic_cockpit.cfg file */
bool LoadDCParams() {
	log_debug("[DBG] Loading Dynamic Cockpit params...");
	FILE *file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;
	float cover_tex_width = 1, cover_tex_height = 1;

	try {
		error = fopen_s(&file, "./dynamic_cockpit.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load dynamic_cockpit.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading dynamic_cockpit.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Initialize the IFF colors
	// Rebel
	g_DCTargetingIFFColors[0].x = 0.1f;
	g_DCTargetingIFFColors[0].y = 0.7f;
	g_DCTargetingIFFColors[0].z = 0.1f;
	// Imperial
	g_DCTargetingIFFColors[1].x = 0.5f;
	g_DCTargetingIFFColors[1].y = 0.1f;
	g_DCTargetingIFFColors[1].z = 0.1f;
	// Neutral
	g_DCTargetingIFFColors[2].x = 0.1f;
	g_DCTargetingIFFColors[2].y = 0.1f;
	g_DCTargetingIFFColors[2].z = 0.5f;
	// Viraxo
	g_DCTargetingIFFColors[3].x = 0.3f;
	g_DCTargetingIFFColors[3].y = 0.2f;
	g_DCTargetingIFFColors[3].z = 0.1f;
	// Backdrop (?)
	g_DCTargetingIFFColors[4].x = 0.1f;
	g_DCTargetingIFFColors[4].y = 0.1f;
	g_DCTargetingIFFColors[4].z = 0.1f;
	// Azzameen
	g_DCTargetingIFFColors[5].x = 0.5f;
	g_DCTargetingIFFColors[5].y = 0.1f;
	g_DCTargetingIFFColors[5].z = 0.5f;
	// Other wireframe initialization
	g_DCWireframeLuminance.x = 0.33f;
	g_DCWireframeLuminance.y = 0.50f;
	g_DCWireframeLuminance.z = 0.16f;
	g_DCWireframeLuminance.w = 0.05f;

	g_DCWireframeContrast = 3.0f;

	// Reset the dynamic cockpit vector if we're not rendering in 3D
	//if (!g_bRendering3D && g_DCElements.size() > 0) {
	//	log_debug("[DBG] [DC] Clearing g_DCElements");
	//	ClearDynCockpitVector(g_DCElements);
	//}
	ClearDCMoveRegions();

	/* Reload individual cockit parameters if the current cockpit is set */
	bool bCockpitParamsLoaded = false;
	if (g_sCurrentCockpit[0] != 0) {
		char sFileName[80];
		CockpitNameToDCParamsFile(g_sCurrentCockpit, sFileName, 80);
		bCockpitParamsLoaded = LoadIndividualDCParams(sFileName);
	}

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, DYNAMIC_COCKPIT_ENABLED_VRPARAM) == 0) {
				g_bDynCockpitEnabled = (bool)fValue;
				log_debug("[DBG] [DC] g_bDynCockpitEnabled: %d", g_bDynCockpitEnabled);
				if (!g_bDynCockpitEnabled) {
					// Early abort: stop reading coordinates if the dynamic cockpit is disabled
					fclose(file);
					return false;
				}
			}
			else if (_stricmp(param, MOVE_REGION_DCPARAM) == 0) {
				// Individual cockpit move_region commands override the global move_region commands:
				// EDIT: I don't remember very well why I did this; but maybe it was when I thought the move_region
				// commands would go into each cockpit. So the per-cockpit move_region commands would override
				// any global move_regions. However, at this point, the move_region commands seem to be global
				// and apply to all cockpits, so let's reload these commands to make it easier to edit the HUD
				// with Ctrl+Alt+L
				//if (!bCockpitParamsLoaded)
					LoadDCMoveRegion(buf);
			}
			else if (_stricmp(param, CT_BRIGHTNESS_DCPARAM) == 0) {
				g_fCoverTextureBrightness = fValue;
			}
			/*else if (_stricmp(param, "ignore_erase_commands") == 0) {
				//g_bDCApplyEraseRegionCommands = (bool)fValue;
			}*/
			else if (_stricmp(param, "HUD_visible_on_startup") == 0) {
				g_bHUDVisibleOnStartup = (bool)fValue;
				//g_bDCApplyEraseRegionCommands = (bool)fValue;
			}
			else if (_stricmp(param, "compensate_FOV_for_1920x1080") == 0) {
				g_bCompensateFOVfor1920x1080 = (bool)fValue;
			}

			else if (_stricmp(param, "dc_brightness") == 0) {
				g_fDCBrightness = fValue;
			}

			else if (_stricmp(param, "enable_wireframe_CMD") == 0) {
				g_bEdgeDetectorEnabled = (bool)fValue;
			}
			else if (_stricmp(param, "wireframe_IFF_color_0") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[0].x = x;
					g_DCTargetingIFFColors[0].y = y;
					g_DCTargetingIFFColors[0].z = z;
					g_DCTargetingIFFColors[0].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_1") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[1].x = x;
					g_DCTargetingIFFColors[1].y = y;
					g_DCTargetingIFFColors[1].z = z;
					g_DCTargetingIFFColors[1].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_2") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[2].x = x;
					g_DCTargetingIFFColors[2].y = y;
					g_DCTargetingIFFColors[2].z = z;
					g_DCTargetingIFFColors[2].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_3") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[3].x = x;
					g_DCTargetingIFFColors[3].y = y;
					g_DCTargetingIFFColors[3].z = z;
					g_DCTargetingIFFColors[3].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_4") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[4].x = x;
					g_DCTargetingIFFColors[4].y = y;
					g_DCTargetingIFFColors[4].z = z;
					g_DCTargetingIFFColors[4].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_IFF_color_5") == 0) {
				float x, y, z;
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCTargetingIFFColors[5].x = x;
					g_DCTargetingIFFColors[5].y = y;
					g_DCTargetingIFFColors[5].z = z;
					g_DCTargetingIFFColors[5].w = 1.0f;
				}
			}
			else if (_stricmp(param, "wireframe_luminance_vector") == 0) {
				float x, y, z;
				log_debug("[DBG] [DC] Loading wireframe luminance vector...");
				if (LoadGeneric3DCoords(buf, &x, &y, &z)) {
					g_DCWireframeLuminance.x = x;
					g_DCWireframeLuminance.y = y;
					g_DCWireframeLuminance.z = z;
					g_DCWireframeLuminance.w = 0.0f;
					log_debug("[DBG] [DC] WireframeLuminance: %0.3f, %0.3f, %0.3f, %0.3f",
						g_DCWireframeLuminance.x, g_DCWireframeLuminance.y, g_DCWireframeLuminance.z, g_DCWireframeLuminance.w);
				}
			}
			else if (_stricmp(param, "wireframe_contrast") == 0) {
				g_DCWireframeContrast = fValue;
				log_debug("[DBG] [DC] Wireframe contrast: %0.3f", g_DCWireframeContrast);
			}

		}
	}
	fclose(file);
	return true;
}

/* Loads the dynamic_cockpit.cfg file */
bool LoadACParams() {
	log_debug("[DBG] [AC] Loading Active Cockpit params...");
	FILE *file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./active_cockpit.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] [AC] Could not load active_cockpit.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] [AC] Error %d when loading active_cockpit.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	/* Reload individual cockpit parameters if the current cockpit is set */
	// TODO: Reset AC params!
	bool bActiveCockpitParamsLoaded = false;
	if (g_sCurrentCockpit[0] != 0) {
		char sFileName[80];
		CockpitNameToACParamsFile(g_sCurrentCockpit, sFileName, 80);
		bActiveCockpitParamsLoaded = LoadIndividualACParams(sFileName);
	}

	g_LaserPointerBuffer.bDebugMode = 0;
	g_LaserPointerBuffer.cursor_radius = 0.01f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "active_cockpit_enabled") == 0) {
				g_bActiveCockpitEnabled = (bool)fValue;
				log_debug("[DBG] [AC] g_bActiveCockpitEnabled: %d", g_bActiveCockpitEnabled);
				if (!g_bActiveCockpitEnabled) {
					// Early abort: stop reading coordinates if the active cockpit is disabled
					fclose(file);
					return false;
				}
			}
			else if (_stricmp(param, "freepie_controller_slot") == 0) {
				g_iFreePIEControllerSlot = (int)fValue;
				InitFreePIE();
			}
			else if (_stricmp(param, "button_data_available") == 0) {
				g_bFreePIEControllerButtonDataAvailable = (bool)fValue;
			}
			else if (_stricmp(param, "controller_multiplier_x") == 0) {
				g_fContMultiplierX = fValue;
			}
			else if (_stricmp(param, "controller_multiplier_y") == 0) {
				g_fContMultiplierY = fValue;
			}
			else if (_stricmp(param, "controller_multiplier_z") == 0) {
				g_fContMultiplierZ = fValue;
			}
			else if (_stricmp(param, "origin_from_HMD_position") == 0) {
				g_bOriginFromHMD = (bool)fValue;
			}
			else if (_stricmp(param, "cursor_origin_init_x") == 0) {
				g_contOriginWorldSpace.x = fValue;
			}
			else if (_stricmp(param, "cursor_origin_init_y") == 0) {
				g_contOriginWorldSpace.y = fValue;
			}
			else if (_stricmp(param, "cursor_origin_init_z") == 0) {
				g_contOriginWorldSpace.z = fValue;
			}
			else if (_stricmp(param, "compensate_HMD_rotation") == 0) {
				g_bCompensateHMDRotation = (bool)fValue;
			}
			else if (_stricmp(param, "compensate_HMD_position") == 0) {
				g_bCompensateHMDPosition = (bool)fValue;
			}
			else if (_stricmp(param, "compensate_HMD_motion") == 0) {
				g_bCompensateHMDRotation = (bool)fValue;
				g_bCompensateHMDPosition = (bool)fValue;
			}
			else if (_stricmp(param, "full_cockpit_test") == 0) {
				g_bFullCockpitTest = (bool)fValue;
			}
			else if (_stricmp(param, "laser_pointer_length") == 0) {
				g_fLaserPointerLength = fValue;
			}
			else if (_stricmp(param, "debug_laser_dir") == 0) {
				g_iLaserDirSelector = (int)fValue;
			}
			else if (_stricmp(param, "debug") == 0) {
				g_LaserPointerBuffer.bDebugMode = (bool)fValue;
			}
			else if (_stricmp(param, "cursor_radius") == 0) {
				g_LaserPointerBuffer.cursor_radius = fValue;
			}
		}
	}
	fclose(file);
	return true;
}

/* Loads the Bloom parameters from bloom.cfg */
bool LoadBloomParams() {
	log_debug("[DBG] Loading Bloom params...");
	FILE *file;
	int error = 0, line = 0;

	try {
		error = fopen_s(&file, "./bloom.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load bloom.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading bloom.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;
	// Set some default values
	g_BloomConfig.uvStepSize1 = 3.0f;
	g_BloomConfig.uvStepSize2 = 2.0f;
	g_BloomConfig.fLasersStrength     = 4.0f;
	g_BloomConfig.fEngineGlowStrength = 0.5f;
	g_BloomConfig.fSparksStrength	  = 0.5f;
	g_BloomConfig.fSkydomeLightStrength = 0.1f;
	g_BloomConfig.fBracketStrength    = 0.0f;
	g_BloomPSCBuffer.general_bloom_strength = 1.0f;
	// TODO: Complete the list of default values...
	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			// ReShade state
			if (_stricmp(param, "bloom_enabled") == 0) {
				bool state = (bool)fValue;
				g_bReshadeEnabled |= state;
				g_bBloomEnabled = state;
			}

			// Bloom
			/*
			else if (_stricmp(param, "general_bloom_strength") == 0) {
				g_BloomPSCBuffer.general_bloom_strength = fValue;
				log_debug("[DBG] [Bloom] general bloom strength: %0.3f",
					g_BloomPSCBuffer.general_bloom_strength);
			}
			*/
			else if (_stricmp(param, "saturation_strength") == 0) {
				g_BloomConfig.fSaturationStrength = fValue;
			}
			else if (_stricmp(param, "bloom_levels") == 0) {
				g_BloomConfig.iNumPasses = (int)fValue;
				if (g_BloomConfig.iNumPasses > MAX_BLOOM_PASSES)
					g_BloomConfig.iNumPasses = MAX_BLOOM_PASSES;
				log_debug("[DBG] [Bloom] iNumPasses: %d", g_BloomConfig.iNumPasses);
			}
			else if (_stricmp(param, "uv_step_size_1") == 0) {
				g_BloomConfig.uvStepSize1 = fValue;
			}
			else if (_stricmp(param, "uv_step_size_2") == 0) {
				g_BloomConfig.uvStepSize2 = fValue;
			}
			else if (_stricmp(param, "background_suns_strength") == 0) {
				g_BloomConfig.fSunsStrength = fValue;
			}
			else if (_stricmp(param, "lens_flare_strength") == 0) {
				g_BloomConfig.fLensFlareStrength = fValue;
			}
			else if (_stricmp(param, "cockpit_lights_strength") == 0) {
				g_BloomConfig.fCockpitStrength = fValue;
			}
			else if (_stricmp(param, "light_map_strength") == 0) {
				g_BloomConfig.fLightMapsStrength = fValue;
			}
			else if (_stricmp(param, "lasers_strength") == 0) {
				g_BloomConfig.fLasersStrength = fValue;
			}
			else if (_stricmp(param, "turbolasers_strength") == 0) {
				g_BloomConfig.fTurboLasersStrength = fValue;
			}
			else if (_stricmp(param, "engine_glow_strength") == 0) {
				g_BloomConfig.fEngineGlowStrength = fValue;
			}
			else if (_stricmp(param, "explosions_strength") == 0) {
				g_BloomConfig.fExplosionsStrength = fValue;
			}
			else if (_stricmp(param, "sparks_strength") == 0) {
				g_BloomConfig.fSparksStrength = fValue;
			}
			else if (_stricmp(param, "cockpit_sparks_strength") == 0) {
				g_BloomConfig.fCockpitSparksStrength = fValue;
			}
			else if (_stricmp(param, "missile_strength") == 0) {
				g_BloomConfig.fMissileStrength = fValue;
			}
			else if (_stricmp(param, "hyper_streak_strength") == 0) {
				g_BloomConfig.fHyperStreakStrength = fValue;
			}
			else if (_stricmp(param, "hyper_tunnel_strength") == 0) {
				g_BloomConfig.fHyperTunnelStrength = fValue;
			}
			else if (_stricmp(param, "skydome_light_strength") == 0) {
				g_BloomConfig.fSkydomeLightStrength = fValue;
			}
			else if (_stricmp(param, "bracket_strength") == 0) {
				g_BloomConfig.fBracketStrength = fValue;
			}
			
			// Bloom strength per pyramid level
			else if (_stricmp(param, "bloom_layer_mult_0") == 0) {
				g_fBloomLayerMult[0] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_1") == 0) {
				g_fBloomLayerMult[1] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_2") == 0) {
				g_fBloomLayerMult[2] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_3") == 0) {
				g_fBloomLayerMult[3] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_4") == 0) {
				g_fBloomLayerMult[4] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_5") == 0) {
				g_fBloomLayerMult[5] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_6") == 0) {
				g_fBloomLayerMult[6] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_7") == 0) {
				g_fBloomLayerMult[7] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_8") == 0) {
				g_fBloomLayerMult[8] = fValue;
			}
			else if (_stricmp(param, "bloom_layer_mult_9") == 0) {
				g_fBloomLayerMult[9] = fValue;
			}

			// Bloom Spread
			else if (_stricmp(param, "bloom_spread_1") == 0) {
				g_fBloomSpread[1] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_2") == 0) {
				g_fBloomSpread[2] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_3") == 0) {
				g_fBloomSpread[3] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_4") == 0) {
				g_fBloomSpread[4] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_5") == 0) {
				g_fBloomSpread[5] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_6") == 0) {
				g_fBloomSpread[6] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_7") == 0) {
				g_fBloomSpread[7] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_8") == 0) {
				g_fBloomSpread[8] = fValue;
			}
			else if (_stricmp(param, "bloom_spread_9") == 0) {
				g_fBloomSpread[9] = fValue;
			}

			// Bloom Passes
			else if (_stricmp(param, "bloom_passes_1") == 0) {
				g_iBloomPasses[1] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_2") == 0) {
				g_iBloomPasses[2] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_3") == 0) {
				g_iBloomPasses[3] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_4") == 0) {
				g_iBloomPasses[4] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_5") == 0) {
				g_iBloomPasses[5] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_6") == 0) {
				g_iBloomPasses[6] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_7") == 0) {
				g_iBloomPasses[7] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_8") == 0) {
				g_iBloomPasses[8] = (int)fValue;
			}
			else if (_stricmp(param, "bloom_passes_9") == 0) {
				g_iBloomPasses[9] = (int)fValue;
			}
		}
	}
	fclose(file);

	log_debug("[DBG] Reshade Enabled: %d", g_bReshadeEnabled);
	log_debug("[DBG] Bloom Enabled: %d", g_bBloomEnabled);
	return true;
}

bool LoadGeneric3DCoords(char *buf, float *x, float *y, float *z)
{
	int res = 0;
	char *c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f", x, y, z);
			if (res < 3) {
				log_debug("[DBG] ERROR (skipping), expected at least 3 elements in [%s], read: %d", c, res);
				return false;
			}
		}
		catch (...) {
			log_debug("[DBG] Could not read 3D from: %s", buf);
			return false;
		}
	}
	return true;
}

bool LoadGeneric4DCoords(char *buf, float *x, float *y, float *z, float *w)
{
	int res = 0;
	char *c = NULL;

	c = strchr(buf, '=');
	if (c != NULL) {
		c += 1;
		try {
			res = sscanf_s(c, "%f, %f, %f, %f", x, y, z, w);
			if (res < 4) {
				log_debug("[DBG] ERROR (skipping), expected at least 4 elements in [%s], read %d", c, res);
				return false;
			}
		}
		catch (...) {
			log_debug("[DBG] Could not read 4D from: %s", buf);
			return false;
		}
	}
	return true;
}

bool LoadSSAOParams() {
	log_debug("[DBG] Loading SSAO params...");
	FILE *file;
	int error = 0, line = 0;

	// Provide some default values in case they are missing in the config file
	g_SSAO_PSCBuffer.bias = 0.05f;
	g_SSAO_PSCBuffer.intensity = 4.0f;
	g_SSAO_PSCBuffer.indirect_intensity = 1.5f;
	g_SSAO_PSCBuffer.power = 1.0f;
	g_SSAO_PSCBuffer.black_level = 0.2f;
	g_SSAO_PSCBuffer.bentNormalInit = 0.1f; // 0.2f // TODO: Check if we need to update this now that the reconstruction uses DEFAULT_FOCAL_DIST
	g_SSAO_PSCBuffer.near_sample_radius = 0.005f;
	g_SSAO_PSCBuffer.far_sample_radius = 0.005f;
	g_SSAO_PSCBuffer.z_division = 0;
	g_SSAO_PSCBuffer.samples = 8;
	g_fMoireOffsetDir = 0.02f;
	g_fMoireOffsetInd = 0.1f;
	g_SSAO_PSCBuffer.moire_offset = g_fMoireOffsetDir;
	g_SSAO_PSCBuffer.nm_intensity_near = 0.2f;
	g_SSAO_PSCBuffer.nm_intensity_far = 0.001f;
	g_SSAO_PSCBuffer.fn_sharpness = 1.0f;
	g_SSAO_PSCBuffer.fn_scale = 0.03f;
	g_SSAO_PSCBuffer.fn_max_xymult = 0.4f;
	g_SSAO_PSCBuffer.shadow_epsilon = 0.0f;
	g_SSAO_PSCBuffer.Bz_mult = 0.05f;
	g_SSAO_PSCBuffer.debug = 0;
	g_SSAO_PSCBuffer.moire_scale = 0.5f; // Previous: 0.1f
	g_fSSAOAlphaOfs = 0.5;
	g_SSAO_Type = SSO_AMBIENT;
	// Default position of the global light (the sun)
	g_LightVector[0].x = 0;
	g_LightVector[0].y = 1;
	g_LightVector[0].z = 0;
	g_LightVector[0].w = 0;
	g_LightVector[0].normalize();

	g_LightVector[1].x = 1;
	g_LightVector[1].y = 1;
	g_LightVector[1].z = 0;
	g_LightVector[1].w = 0;
	g_LightVector[0].normalize();

	// Default view-yaw/pitch signs for SSAO.cfg-based lights
	//g_fViewYawSign = 1.0f;
	//g_fViewPitchSign = -1.0f;

	// Default values for the shading system CB
	g_ShadingSys_PSBuffer.spec_intensity = 1.0f;
	g_ShadingSys_PSBuffer.spec_bloom_intensity = 1.25f;
	g_ShadingSys_PSBuffer.glossiness = 128.0f;
	g_ShadingSys_PSBuffer.bloom_glossiness_mult = 3.0f;
	g_ShadingSys_PSBuffer.saturation_boost = 0.75f;
	g_ShadingSys_PSBuffer.lightness_boost  = 2.0f;
	g_ShadingSys_PSBuffer.sqr_attenuation  = 0.001f; // Smaller numbers fade less
	g_ShadingSys_PSBuffer.laser_light_intensity = 6.0f;
	g_bHDREnabled = false;
	g_fHDRWhitePoint = 1.0f;
	g_fHDRLightsMultiplier = 2.8f;
	g_ShadingSys_PSBuffer.HDREnabled = g_bHDREnabled;
	g_ShadingSys_PSBuffer.HDR_white_point = g_fHDRWhitePoint;
	g_fGlobalAmbient = 0.005f;
	g_fHangarAmbient = 0.05f;

	g_ShadertoyBuffer.flare_intensity = 2.0f;

	g_ShadowMapping.bEnabled = false;
	g_bShadowMapDebug = false;
	g_ShadowMapVSCBuffer.sm_bias = 0.01f;
	g_ShadowMapVSCBuffer.sm_debug = g_bShadowMapDebug;
	g_ShadowMapVSCBuffer.sm_pcss_radius = 1.0f / SHADOW_MAP_SIZE;
	g_ShadowMapVSCBuffer.sm_light_size = 0.1f;

	for (int i = 0; i < MAX_XWA_LIGHTS; i++)
		if (!g_XWALightInfo[i].bTagged)
			g_ShadowMapVSCBuffer.sm_black_levels[i] = 0.2f;

	g_bDumpOBJEnabled = false;

	try {
		error = fopen_s(&file, "./ssao.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load ssao.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading ssao.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "ssao_enabled") == 0) {
				bool state = (bool)fValue;
				g_bReshadeEnabled |= state;
				g_bAOEnabled = state;
			}

			else if (_stricmp(param, "ssao_type") == 0) {
				if (_stricmp(svalue, "ambient") == 0)
					g_SSAO_Type = SSO_AMBIENT;
				else if (_stricmp(svalue, "directional") == 0)
					g_SSAO_Type = SSO_DIRECTIONAL;
				else if (_stricmp(svalue, "bent_normals") == 0)
					g_SSAO_Type = SSO_BENT_NORMALS;
				else if (_stricmp(svalue, "deferred") == 0)
					g_SSAO_Type = SSO_DEFERRED;
			}
			if (_stricmp(param, "disable_xwa_diffuse") == 0) {
				g_bDisableDiffuse = (bool)fValue;
			}
			else if (_stricmp(param, "bias") == 0) {
				g_SSAO_PSCBuffer.bias = fValue;
			}
			else if (_stricmp(param, "intensity") == 0) {
				g_SSAO_PSCBuffer.intensity = fValue;
			}
			else if (_stricmp(param, "near_sample_radius") == 0) { // "sample_radius" was previously read here too
				g_SSAO_PSCBuffer.near_sample_radius = fValue;
			}
			else if (_stricmp(param, "far_sample_radius") == 0) {
				g_SSAO_PSCBuffer.far_sample_radius = fValue;
			}
			else if (_stricmp(param, "samples") == 0) {
				g_SSAO_PSCBuffer.samples = (int )fValue;
			}
			else if (_stricmp(param, "ssao_buffer_scale_divisor") == 0) {
				g_fSSAOZoomFactor = (float)fValue;
			}
			/*else if (_stricmp(param, "ssao2_buffer_scale_divisor") == 0) {
				g_fSSAOZoomFactor2 = (float)fValue;
			}*/
			else if (_stricmp(param, "enable_blur") == 0) {
				g_bBlurSSAO = (bool)fValue;
			}
			else if (_stricmp(param, "black_level") == 0) {
				g_SSAO_PSCBuffer.black_level = fValue;
			}
			else if (_stricmp(param, "perspective_correct") == 0) {
				g_SSAO_PSCBuffer.z_division = (bool)fValue;
			}
			else if (_stricmp(param, "bent_normal_init") == 0) {
				g_SSAO_PSCBuffer.bentNormalInit = fValue; // Default: 0.2f
			}
			else if (_stricmp(param, "max_dist") == 0) {
				g_SSAO_PSCBuffer.max_dist = fValue;
			}
			else if (_stricmp(param, "power") == 0) {
				g_SSAO_PSCBuffer.power = fValue;
			}
			/*else if (_stricmp(param, "enable_dual_ssao") == 0) {
				g_bDisableDualSSAO = !(bool)fValue;
			}
			else if (_stricmp(param, "enable_ssao_in_shader") == 0) {
				g_bEnableSSAOInShader = (bool)fValue;
			}
			else if (_stricmp(param, "enable_bent_normals_in_shader") == 0) {
				g_bEnableBentNormalsInShader = (bool)fValue;
			}*/
			else if (_stricmp(param, "debug") == 0) {
				g_SSAO_PSCBuffer.debug = (int)fValue;
				g_BloomPSCBuffer.debug = (int)fValue;
			}
			else if (_stricmp(param, "indirect_intensity") == 0) {
				g_SSAO_PSCBuffer.indirect_intensity = fValue;
			}
			/*else if (_stricmp(param, "normal_blur_radius") == 0) {
				g_fNormalBlurRadius = fValue;
			}*/
			else if (_stricmp(param, "debug_ssao") == 0) {
				g_iSSDODebug = (int)fValue;
			}
			else if (_stricmp(param, "blur_passes") == 0) {
				g_iSSAOBlurPasses = (int)fValue;
			}
			else if (_stricmp(param, "enable_indirect_ssdo") == 0) {
				g_bEnableIndirectSSDO = (bool)fValue;
			}
			else if (_stricmp(param, "moire_offset") == 0) {
				g_fMoireOffsetDir = fValue;
			}
			else if (_stricmp(param, "moire_offset_ind") == 0) {
				g_fMoireOffsetInd = fValue;
			}
			else if (_stricmp(param, "moire_scale") == 0) {
				g_SSAO_PSCBuffer.moire_scale = fValue;
			}
			/* else if (_stricmp(param, "add_ssdo_to_indirect_pass") == 0) {
				g_SSAO_PSCBuffer.addSSDO = (int)fValue;
			} */
			else if (_stricmp(param, "normal_mapping_enable") == 0) {
				g_SSAO_PSCBuffer.fn_enable = (int)fValue;
				g_bFNEnable = g_SSAO_PSCBuffer.fn_enable;
			}
			else if (_stricmp(param, "nm_max_xymult") == 0) {
				g_SSAO_PSCBuffer.fn_max_xymult = fValue;
			}
			else if (_stricmp(param, "nm_scale") == 0) {
				g_SSAO_PSCBuffer.fn_scale = fValue;
			}
			else if (_stricmp(param, "nm_sharpness") == 0) {
				g_SSAO_PSCBuffer.fn_sharpness = fValue;
			}
			else if (_stricmp(param, "nm_intensity_near") == 0) {
				g_SSAO_PSCBuffer.nm_intensity_near = fValue;
			}
			else if (_stricmp(param, "nm_intensity_far") == 0) {
				g_SSAO_PSCBuffer.nm_intensity_far = fValue;
			}
			else if (_stricmp(param, "override_game_light_pos") == 0) {
				g_bOverrideLightPos = (bool)fValue;
			}
			else if (_stricmp(param, "alpha_to_solid_offset") == 0) {
				g_fSSAOAlphaOfs = fValue;
			}
			else if (_stricmp(param, "ssdo_ambient") == 0 ||
					 _stricmp(param, "ambient") == 0) {
				g_ShadingSys_PSBuffer.ambient = fValue;
				g_fGlobalAmbient = fValue;
			}
			else if (_stricmp(param, "hangar_ambient") == 0) {
				g_fHangarAmbient = fValue;
			}
			else if (_stricmp(param, "xwa_lights_saturation") == 0) {
				g_fXWALightsSaturation = fValue;
			}
			else if (_stricmp(param, "xwa_lights_apply_original_intensity") == 0) {
				g_bApplyXWALightsIntensity = (bool)fValue;
			}
			else if (_stricmp(param, "xwa_lights_global_intensity") == 0) {
				g_fXWALightsIntensity = fValue;
			}
			else if (_stricmp(param, "procedural_suns") == 0) {
				g_bProceduralSuns = (bool)fValue;
			}
			else if (_stricmp(param, "flare_intensity") == 0) {
				g_ShadertoyBuffer.flare_intensity = fValue;
			}
			else if (_stricmp(param, "enable_speed_shader") == 0) {
				g_bEnableSpeedShader = (bool)fValue;
			}
			else if (_stricmp(param, "speed_shader_scale_factor") == 0) {
				g_fSpeedShaderScaleFactor = fValue;
			}
			else if (_stricmp(param, "speed_shader_particle_size") == 0) {
				g_fSpeedShaderParticleSize = fValue;
			}
			else if (_stricmp(param, "speed_shader_max_intensity") == 0) {
				g_fSpeedShaderMaxIntensity = fValue;
			}
			else if (_stricmp(param, "speed_shader_trail_length") == 0) {
				g_fSpeedShaderTrailSize = fValue;
			}
			else if (_stricmp(param, "speed_shader_max_particles") == 0) {
				g_iSpeedShaderMaxParticles = (int )fValue;
				if (g_iSpeedShaderMaxParticles < 0) g_iSpeedShaderMaxParticles = 0;
				g_iSpeedShaderMaxParticles = min(g_iSpeedShaderMaxParticles, MAX_SPEED_PARTICLES);
			}
			else if (_stricmp(param, "speed_shader_particle_range") == 0) {
				g_fSpeedShaderParticleRange = fValue;
			}
			// Additional Geometry Shader
			else if (_stricmp(param, "enable_additional_geometry") == 0) {
				g_bEnableAdditionalGeometry = (bool)fValue;
			}
			else if (_stricmp(param, "add_geom_trans_scale") == 0) {
				g_fCockpitTranslationScale = fValue;
			}
			// Shadow Mapping
			else if (_stricmp(param, "shadow_mapping_enable") == 0) {
				g_bShadowMapEnable = (bool)fValue;
				g_ShadowMapping.bEnabled = g_bShadowMapEnable;
				g_ShadowMapVSCBuffer.sm_enabled = g_ShadowMapping.bEnabled;
				log_debug("[DBG] [SHW] g_ShadowMapping.Enabled: %d", g_ShadowMapping.bEnabled);
			}
			else if (_stricmp(param, "shadow_mapping_anisotropic_scale") == 0) {
				g_ShadowMapping.bAnisotropicMapScale = (bool)fValue;
			}
			else if (_stricmp(param, "shadow_mapping_angle_x") == 0) {
				g_fShadowMapAngleX = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapAngleX: %0.3f", g_fShadowMapAngleX);
			}
			else if (_stricmp(param, "shadow_mapping_angle_y") == 0) {
				g_fShadowMapAngleY = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapAngleY: %0.3f", g_fShadowMapAngleY);
			}
			else if (_stricmp(param, "shadow_mapping_scale") == 0) {
				g_fShadowMapScale = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapScale: %0.3f", g_fShadowMapScale);
			}

			else if (_stricmp(param, "shadow_mapping_OBJ_scale") == 0) {
				SHADOW_OBJ_SCALE = fValue;
				log_debug("[DBG] [SHW] g_fShadowMapScale: %0.3f", SHADOW_OBJ_SCALE);
			}

			else if (_stricmp(param, "shadow_mapping_z_factor") == 0) {
				g_ShadowMapVSCBuffer.sm_z_factor = fValue;
				log_debug("[DBG] [SHW] sm_z_factor: %0.3f", g_ShadowMapVSCBuffer.sm_z_factor);
			}

			else if (_stricmp(param, "shadow_mapping_pcss_samples") == 0) {
				g_ShadowMapVSCBuffer.sm_pcss_samples = (int)fValue;
			}
			
			else if (_stricmp(param, "shadow_mapping_black_level") == 0) {
				g_ShadowMapping.black_level = fValue;
				for (int i = 0; i < MAX_XWA_LIGHTS; i++)
					if (!g_XWALightInfo[i].bTagged)
						g_ShadowMapVSCBuffer.sm_black_levels[i] = fValue;
			}

			else if (_stricmp(param, "shadow_mapping_black_level_0") == 0) {
				if (!g_XWALightInfo[0].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[0] = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_black_level_1") == 0) {
				if (!g_XWALightInfo[1].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[1] = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_black_level_2") == 0) {
				if (!g_XWALightInfo[2].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[2] = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_black_level_3") == 0) {
				if (!g_XWALightInfo[3].bTagged) g_ShadowMapVSCBuffer.sm_black_levels[3] = fValue;
			}

			else if (_stricmp(param, "shadow_mapping_depth_bias") == 0) {
				g_ShadowMapping.DepthBias = (int)fValue;
			}
			else if (_stricmp(param, "shadow_mapping_depth_bias_clamp") == 0) {
				g_ShadowMapping.DepthBiasClamp = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_slope_scale_depth_bias") == 0) {
				g_ShadowMapping.SlopeScaledDepthBias = fValue;
			}

			else if (_stricmp(param, "shadow_mapping_POV_XY_FACTOR") == 0) {
				g_ShadowMapping.POV_XY_FACTOR = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_POV_Z_FACTOR") == 0) {
				g_ShadowMapping.POV_Z_FACTOR = fValue;
			}
			

			else if (_stricmp(param, "shadow_mapping_depth_trans") == 0) {
				g_fShadowMapDepthTrans = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_invert_camera_matrix") == 0) {
				g_bShadowMappingInvertCameraMatrix = (bool)fValue;
			}
			
			else if (_stricmp(param, "shadow_mapping_bias_sw") == 0) {
				//g_ShadowMapVSCBuffer.sm_bias = fValue;
				g_ShadowMapping.sw_pcf_bias = fValue;
				log_debug("[DBG] [SHW] sw_pcf_bias: %0.3f", g_ShadowMapping.sw_pcf_bias);
			}
			else if (_stricmp(param, "shadow_mapping_bias_hw") == 0) {
				//g_ShadowMapVSCBuffer.sm_bias = fValue;
				g_ShadowMapping.hw_pcf_bias = fValue;
				log_debug("[DBG] [SHW] hw_pcf_bias: %0.3f", g_ShadowMapping.hw_pcf_bias);
			}

			else if (_stricmp(param, "shadow_mapping_debug") == 0) {
				g_bShadowMapDebug = (bool)fValue;
				g_ShadowMapVSCBuffer.sm_debug = g_bShadowMapDebug;
			}
			else if (_stricmp(param, "shadow_mapping_pcss_radius") == 0) {
				g_ShadowMapVSCBuffer.sm_pcss_radius = fValue;
			}
			//else if (_stricmp(param, "shadow_mapping_light_size") == 0) {
			//	g_ShadowMapVSCBuffer.sm_light_size = fValue;
			//}
			//else if (_stricmp(param, "shadow_mapping_blocker_radius") == 0) {
			//	g_ShadowMapVSCBuffer.sm_blocker_radius = fValue;
			//}
			else if (_stricmp(param, "shadow_mapping_POV_X") == 0) {
				g_ShadowMapVSCBuffer.POV.x = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_POV_Y") == 0) {
				g_ShadowMapVSCBuffer.POV.y = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_POV_Z") == 0) {
				g_ShadowMapVSCBuffer.POV.z = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_OBJrange_override") == 0) {
				g_ShadowMapping.bOBJrange_override = (bool)fValue;
			}
			else if (_stricmp(param, "shadow_mapping_OBJrange_value") == 0) {
				g_ShadowMapping.fOBJrange_override_value = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_fovdist_scale") == 0) {
				g_ShadowMapping.FOVDistScale = fValue;
			}
			else if (_stricmp(param, "shadow_mapping_enable_multiple_suns") == 0) {
				g_ShadowMapping.bMultipleSuns = (bool)fValue;
			}

			/*
			else if (_stricmp(param, "shadow_mapping_XWA_LIGHT_Y_CONV_SCALE") == 0) {
				g_ShadowMapping.XWA_LIGHT_Y_CONV_SCALE = fValue;
			}
			*/
			

			else if (_stricmp(param, "dump_OBJ_enabled") == 0) {
				g_bDumpOBJEnabled = (bool)fValue;
			}
			
			else if (_stricmp(param, "HDR_enabled") == 0) {
				g_bHDREnabled = (bool)fValue;
				g_ShadingSys_PSBuffer.HDREnabled = g_bHDREnabled;
			}
			else if (_stricmp(param, "HDR_lights_intensity") == 0) {
				g_fHDRLightsMultiplier = fValue;
			}
			
			/*
			else if (_stricmp(param, "flare_aspect_mult") == 0) {
				g_fFlareAspectMult = fValue;
				log_debug("[DBG] g_fFlareAspectMult: %0.3f", g_fFlareAspectMult);
			}
			*/

			/*else if (_stricmp(param, "viewYawSign") == 0) {
				g_fViewYawSign = fValue;
			}
			else if (_stricmp(param, "viewPitchSign") == 0) {
				g_fViewPitchSign = fValue;
			}*/

			/*
			else if (_stricmp(param, "gamma") == 0) {
				g_SSAO_PSCBuffer.gamma = fValue;
			}
			*/
			/*else if (_stricmp(param, "shadow_step_size") == 0) {
				g_SSAO_PSCBuffer.shadow_step_size = fValue;
			}
			else if (_stricmp(param, "shadow_steps") == 0) {
				g_SSAO_PSCBuffer.shadow_steps = fValue;
			}
			else if (_stricmp(param, "shadow_k") == 0) {
				g_SSAO_PSCBuffer.shadow_k = fValue;
			}
			else if (_stricmp(param, "shadow_epsilon") == 0) {
				g_SSAO_PSCBuffer.shadow_epsilon = fValue;
			}
			else if (_stricmp(param, "shadow_enable") == 0) {
				g_bShadowEnable = (bool)fValue;
				g_SSAO_PSCBuffer.shadow_enable = g_bShadowEnable;
			}*/
			/*
			else if (_stricmp(param, "white_point") == 0) {
				g_SSAO_PSCBuffer.white_point = fValue;
			}
			*/
			else if (_stricmp(param, "Bz_mult") == 0) {
				g_SSAO_PSCBuffer.Bz_mult = fValue;
			}
			/*else if (_stricmp(param, "light_vector") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightVector[0].x = x;
				g_LightVector[0].y = y;
				g_LightVector[0].z = z;
				g_LightVector[0].w = 0.0f;
				g_LightVector[0].normalize();
				log_debug("[DBG] [AO] Light vec: [%0.3f, %0.3f, %0.3f]",
					g_LightVector[0].x, g_LightVector[0].y, g_LightVector[0].z);
			}
			else if (_stricmp(param, "light_vector2") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightVector[1].x = x;
				g_LightVector[1].y = y;
				g_LightVector[1].z = z;
				g_LightVector[1].w = 0.0f;
				g_LightVector[1].normalize();
				log_debug("[DBG] [AO] Light vec2: [%0.3f, %0.3f, %0.3f]",
					g_LightVector[1].x, g_LightVector[1].y, g_LightVector[1].z);
			}
			else if (_stricmp(param, "light_color") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightColor[0].x = x;
				g_LightColor[0].y = y;
				g_LightColor[0].z = z;
				g_LightColor[0].w = 0.0f;
				log_debug("[DBG] [AO] Light Color: [%0.3f, %0.3f, %0.3f]",
					g_LightColor[0].x, g_LightColor[0].y, g_LightColor[0].z);
			}
			else if (_stricmp(param, "light_color2") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_LightColor[1].x = x;
				g_LightColor[1].y = y;
				g_LightColor[1].z = z;
				g_LightColor[1].w = 0.0f;
				log_debug("[DBG] [AO] Light Color2: [%0.3f, %0.3f, %0.3f]",
					g_LightColor[1].x, g_LightColor[1].y, g_LightColor[1].z);
			}*/
			/*
			else if (_stricmp(param, "shadow_color") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_SSAO_PSCBuffer.invLightR = x;
				g_SSAO_PSCBuffer.invLightG = y;
				g_SSAO_PSCBuffer.invLightB = z;
			}
			*/

			/* Shading System Settings */
			else if (_stricmp(param, "specular_intensity") == 0) {
				g_ShadingSys_PSBuffer.spec_intensity = fValue;
				g_fSpecIntensity = fValue;
			}
			else if (_stricmp(param, "specular_bloom_intensity") == 0) {
				g_ShadingSys_PSBuffer.spec_bloom_intensity = fValue;
				g_fSpecBloomIntensity = fValue;
			}
			else if (_stricmp(param, "lightness_boost") == 0) {
				g_ShadingSys_PSBuffer.lightness_boost = fValue;
			}
			else if (_stricmp(param, "saturation_boost") == 0) {
				g_ShadingSys_PSBuffer.saturation_boost = fValue;
			}
			else if (_stricmp(param, "glossiness") == 0) {
				g_ShadingSys_PSBuffer.glossiness = fValue;
			}
			else if (_stricmp(param, "bloom_glossiness_multiplier") == 0) {
				g_ShadingSys_PSBuffer.bloom_glossiness_mult = fValue;
			}
			else if (_stricmp(param, "ss_debug") == 0) {
				g_ShadingSys_PSBuffer.ss_debug = (int)fValue;
			}
			else if (_stricmp(param, "key_set") == 0) {
				g_KeySet = (int)fValue;
				log_debug("[DBG] key_set: %d", g_KeySet);
			}
			else if (_stricmp(param, "enable_laser_lights") == 0) {
				g_bEnableLaserLights = (bool)fValue;
			}
			else if (_stricmp(param, "sqr_attenuation") == 0) {
				g_ShadingSys_PSBuffer.sqr_attenuation = fValue;
			}
			else if (_stricmp(param, "laser_light_intensity") == 0) {
				// The way the laser light is attached to the laser texture has changed. I'm now using
				// barycentric coords. I believe the old method may have attached two lights per laser
				// so the new method now looks too dim. To compensate, we probably need to double the
				// laser light intensity here.
				g_ShadingSys_PSBuffer.laser_light_intensity = 2.0f * fValue;
			}
			else if (_stricmp(param, "headlights_pos") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_HeadLightsPosition.x = x;
				g_HeadLightsPosition.y = y;
				g_HeadLightsPosition.z = z;
			}
			else if (_stricmp(param, "headlights_col") == 0) {
				float x, y, z;
				LoadGeneric3DCoords(buf, &x, &y, &z);
				g_HeadLightsColor.x = x;
				g_HeadLightsColor.y = y;
				g_HeadLightsColor.z = z;
			}
			else if (_stricmp(param, "headlights_ambient") == 0) {
				g_fHeadLightsAmbient = fValue;
			}
			else if (_stricmp(param, "headlights_distance") == 0) {
				// Let's make the headlights a bit more powerful -- this helps in the core room in the DS2
				// By multiplying by 3.33, we make the headlights good enough for the Death Star
				g_fHeadLightsDistance = 3.33f * fValue; 
			}
			else if (_stricmp(param, "headlights_angle") == 0) {
				g_fHeadLightsAngleCos = cos(0.01745f * fValue);
			}
			else if (_stricmp(param, "headlights_auto_turn_on") == 0) {
				g_bHeadLightsAutoTurnOn = (bool)fValue;
			}
			else if (_stricmp(param, "reload_materials_enabled") == 0) {
				g_bReloadMaterialsEnabled = (bool)fValue;
				log_debug("[DBG] [MAT] Material Reloading Enabled? %d", g_bReloadMaterialsEnabled);
			}

			/*else if (_stricmp(param, "g_fOBJZMetricMult") == 0) {
				g_fOBJ_Z_MetricMult = fValue;
				log_debug("[DBG] [SHOW] g_fOBJZMetricMult: %0.3f", g_fOBJ_Z_MetricMult);
			}
			else if (_stricmp(param, "g_fOBJGlobalMetricMult") == 0) {
				g_fOBJGlobalMetricMult = fValue;
				log_debug("[DBG] [SHOW] g_fOBJGlobalMetricMult: %0.3f", g_fOBJGlobalMetricMult);
			}*/

			/*else if (_stricmp(param, "emission_intensity") == 0) {
				g_ShadingSys_PSBuffer.emission_intensity = fValue;
			}*/
			
			else if (_stricmp(param, "external_hud_enabled") == 0) {
				g_bExternalHUDEnabled = (bool)fValue;
			}
			else if (_stricmp(param, "ext_hud_ar0") == 0) {
				g_ShadertoyBuffer.preserveAspectRatioComp[0] = fValue;
			}
			else if (_stricmp(param, "ext_hud_ar1") == 0) {
				g_ShadertoyBuffer.preserveAspectRatioComp[1] = fValue;
			}

			if (_stricmp(param, "star_debug_enabled") == 0) {
				g_bStarDebugEnabled = (bool)fValue;
			}
		}
	}
	fclose(file);

	log_debug("[DBG] [AO] SSAO Enabled: %d", g_bAOEnabled);
	log_debug("[DBG] [AO] SSAO bias: %0.3f", g_SSAO_PSCBuffer.bias);
	log_debug("[DBG] [AO] SSAO intensity: %0.3f", g_SSAO_PSCBuffer.intensity);
	log_debug("[DBG] [AO] SSAO near_sample_radius: %0.6f", g_SSAO_PSCBuffer.near_sample_radius);
	log_debug("[DBG] [AO] SSAO far_sample_radius: %0.6f", g_SSAO_PSCBuffer.far_sample_radius);
	log_debug("[DBG] [AO] SSAO samples: %d", g_SSAO_PSCBuffer.samples);
	log_debug("[DBG] [AO] SSAO black_level: %f", g_SSAO_PSCBuffer.black_level);
	if (g_SSAO_Type == SSO_AMBIENT)
		log_debug("[DBG] [AO] SSO: AMBIENT OCCLUSION");
	else if (g_SSAO_Type == SSO_DIRECTIONAL)
		log_debug("[DBG] [AO] SSO: DIRECTIONAL OCCLUSION");
	return true;
}

bool LoadHyperParams() {
	log_debug("[DBG] Loading Hyperspace params...");
	FILE *file;
	int error = 0, line = 0;

	// Provide some default values in case they are missing in the config file
	g_ShadertoyBuffer.y_center = 0.15f;
	g_ShadertoyBuffer.FOVscale = 1.0f;
	g_ShadertoyBuffer.viewMat.identity();
	g_ShadertoyBuffer.bDisneyStyle = 1;
	g_ShadertoyBuffer.tunnel_speed = 5.5f;
	g_ShadertoyBuffer.twirl = 1.0f;
	g_fHyperLightRotationSpeed = 50.0f;
	g_fHyperShakeRotationSpeed = 50.0f;
	g_bHyperDebugMode = false;

	try {
		error = fopen_s(&file, "./hyperspace.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load hyperspace.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading hyperspace.cfg", error);
		return false;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);

			if (_stricmp(param, "disney_style") == 0) {
				g_ShadertoyBuffer.bDisneyStyle = (bool)fValue;
			}
			else if (_stricmp(param, "tunnel_speed") == 0) {
				g_fHyperTunnelSpeed = fValue;
			}
			else if (_stricmp(param, "twirl") == 0) {
				g_ShadertoyBuffer.twirl = fValue;
			}
			else if (_stricmp(param, "debug_mode") == 0) {
				g_bHyperDebugMode = (bool)fValue;
			}
			else if (_stricmp(param, "debug_fsm") == 0) {
				g_iHyperStateOverride = (int)fValue;
			}
			else if (_stricmp(param, "light_rotation_speed") == 0) {
				g_fHyperLightRotationSpeed = fValue;
			}
			else if (_stricmp(param, "shake_speed") == 0) {
				g_fHyperShakeRotationSpeed = fValue;
			}
		}
	}
	fclose(file);

	return true;
}

bool LoadDefaultGlobalMaterial() {
	FILE *file;
	int error = 0, line = 0;
	char buf[256];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Failsafe: populate with constant defaults:
	g_DefaultGlobalMaterial.Metallic    = DEFAULT_METALLIC;
	g_DefaultGlobalMaterial.Intensity   = DEFAULT_SPEC_INT;
	g_DefaultGlobalMaterial.Glossiness  = DEFAULT_GLOSSINESS;
	g_DefaultGlobalMaterial.NMIntensity = DEFAULT_NM_INT;
	g_DefaultGlobalMaterial.SpecValue   = DEFAULT_SPEC_VALUE;
	g_DefaultGlobalMaterial.IsShadeless = false;
	g_DefaultGlobalMaterial.Light.set(0.0f, 0.0f, 0.0f);

	try {
		error = fopen_s(&file, "./Materials/DefaultGlobalMaterial.mat", "rt");
	}
	catch (...) {
		//log_debug("[DBG] [MAT] Could not load [%s]", sFileName);
	}

	if (error != 0) {
		//log_debug("[DBG] [MAT] Error %d when loading [%s]", error, sFileName);
		return false;
	}
	log_debug("[DBG] [MAT] Loading DefaultGlobalMaterial.mat");
	
	// Now, try to load the global materials file
	while (fgets(buf, 256, file) != NULL) {
		line++;
		ReadMaterialLine(buf, &g_DefaultGlobalMaterial);
	}
	fclose(file);
	return true;
}

void ReloadMaterials() 
{
	char *surface_name;
	char texname[MAX_TEXNAME];
	bool bIsDat;
	OPTNameType OPTname;

	if (!g_bReloadMaterialsEnabled) {
		log_debug("[DBG] [MAT] Material Reloading is not enabled");
		return;
	}
	
	log_debug("[DBG] [MAT] Reloading materials.");
	ClearCraftMaterials();
	ClearOPTnames();

	for (Direct3DTexture *texture : g_AuxTextureVector) {
		OPTname.name[0] = 0;
		surface_name = texture->_surface->_name;
		bIsDat = false;
		//bIsDat = strstr(surface_name, "dat,") != NULL;

		// Capture the OPT/DAT name and load the material file
		char *start = strstr(surface_name, "\\");
		char *end = strstr(surface_name, ".opt");
		char sFileName[180], sFileNameShort[180];
		if (start != NULL && end != NULL) {
			start += 1; // Skip the backslash
			int size = end - start;
			strncpy_s(OPTname.name, MAX_OPT_NAME, start, size);
			if (!isInVector(OPTname.name, g_OPTnames)) {
				//log_debug("[DBG] [MAT] OPT Name Captured: '%s'", OPTname.name);
				// Add the name to the list of OPTnames so that we don't try to process it again
				g_OPTnames.push_back(OPTname);
				OPTNameToMATParamsFile(OPTname.name, sFileName, 180);
				log_debug("[DBG] [MAT] [OPT] Reloading file %s...", sFileName);
				LoadIndividualMATParams(OPTname.name, sFileName); // OPT material
			}
		}
		else if (strstr(surface_name, "dat,") != NULL) {
			bIsDat = true;
			// For DAT images, OPTname.name is the full DAT name:
			strncpy_s(OPTname.name, MAX_OPT_NAME, surface_name, strlen(surface_name));
			DATNameToMATParamsFile(OPTname.name, sFileName, sFileNameShort, 180);
			if (sFileName[0] != 0) {
				log_debug("[DBG] [MAT] [DAT] Reloading file %s...", sFileName);
				// Reload a regular DAT material file:
				if (!LoadIndividualMATParams(OPTname.name, sFileName)) {
					// If the above failed, try reloading the animated mat file:
					if (sFileNameShort[0] != 0)
						LoadIndividualMATParams(OPTname.name, sFileNameShort, false);
				}
			}
		}

		int craftIdx = FindCraftMaterial(OPTname.name);
		//log_debug("[DBG] [MAT] craftIdx: %d", craftIdx);
		if (bIsDat)
			texname[0] = 0; // Retrieve the default material
		else
		{
			//log_debug("[DBG] [MAT] Craft Material %s found", OPTname.name);
			char *start = strstr(surface_name, ".opt");
			// Skip the ".opt," part
			start += 5;
			// Find the next comma
			char *end = strstr(start, ",");
			int size = end - start;
			strncpy_s(texname, MAX_TEXNAME, start, size);
		}
		//log_debug("[DBG] [MAT] Looking for material for %s", texname);
		//bool Debug = strstr(surface_name, "CalamariLulsa") != NULL || bIsDat;
		texture->material = FindMaterial(craftIdx, texname, false);
		//if (Debug)
		//	log_debug("[DBG] Re-Applied Mat: %0.3f, %0.3f, %0.3f", texture->material.Metallic, texture->material.Intensity, texture->material.Glossiness);
		texture->bHasMaterial = true;
	}
}

/* Loads the VR parameters from vrparams.cfg */
void LoadVRParams() {
	log_debug("[DBG] Loading view params...");
	FILE *file;
	int error = 0, line = 0;
	static int lastDCElemSelected = -1;

	try {
		error = fopen_s(&file, "./vrparams.cfg", "rt");
	}
	catch (...) {
		log_debug("[DBG] Could not load vrparams.cfg");
	}

	if (error != 0) {
		log_debug("[DBG] Error %d when loading vrparams.cfg", error);
		goto next;
	}

	char buf[256], param[128], svalue[128];
	int param_read_count = 0;
	float fValue = 0.0f;

	// Reset the dynamic cockpit vector if we're not rendering in 3D
	//if (!g_bRendering3D && g_DCElements.size() > 0) {
	//	log_debug("[DBG] [DC] Clearing g_DCElements");
	//	ClearDynCockpitVector(g_DCElements);
	//}
	g_iSteamVR_VSync_ms = 11;
	g_iSteamVR_Remaining_ms = 3;

	while (fgets(buf, 256, file) != NULL) {
		line++;
		// Skip comments and blank lines
		if (buf[0] == ';' || buf[0] == '#')
			continue;
		if (strlen(buf) == 0)
			continue;

		if (sscanf_s(buf, "%s = %s", param, 128, svalue, 128) > 0) {
			fValue = (float)atof(svalue);
			if (_stricmp(param, FOCAL_DIST_VRPARAM) == 0) {
				g_fFocalDist = fValue;
				log_debug("[DBG] Focal Distance: %0.3f", g_fFocalDist);
			}
			else if (_stricmp(param, IPD_VRPARAM) == 0) {
				EvaluateIPD(fValue);
			}
			/*else if (_stricmp(param, METRIC_MULT_VRPARAM) == 0) {
				g_fMetricMult = fValue;
				log_debug("[DBG] [FOV] g_fMetricMult: %0.6f", g_fMetricMult);
			}
			else if (_stricmp(param, SIZE_3D_WINDOW_VRPARAM) == 0) {
				// Size of the window while playing the game
				g_fGlobalScale = fValue;
			}*/
			else if (_stricmp(param, SIZE_3D_WINDOW_ZOOM_OUT_VRPARAM) == 0) {
				// Size of the window while playing the game; but zoomed out to see all the GUI
				g_fGlobalScaleZoomOut = fValue;
			}
			else if (_stricmp(param, WINDOW_ZOOM_OUT_INITIAL_STATE_VRPARAM) == 0) {
				g_bZoomOutInitialState = (bool)fValue;
				g_bZoomOut = (bool)fValue;
			}
			else if (_stricmp(param, CONCOURSE_WINDOW_SCALE_VRPARAM) == 0) {
				// Concourse and 2D menus scale
				g_fConcourseScale = fValue;
			}
			/*
			else if (_stricmp(param, COCKPIT_Z_THRESHOLD_VRPARAM) == 0) {
				g_fCockpitPZThreshold = fValue;
			}
			*/
			/* else if (_stricmp(param, ASPECT_RATIO_VRPARAM) == 0) {
				g_fAspectRatio = fValue;
				g_bOverrideAspectRatio = true;
			} */
			else if (_stricmp(param, CONCOURSE_ASPECT_RATIO_VRPARAM) == 0) {
				g_fConcourseAspectRatio = fValue;
				g_bOverrideAspectRatio = true;
			}
			else if (_stricmp(param, K1_VRPARAM) == 0) {
				g_fLensK1 = fValue;
			}
			else if (_stricmp(param, K2_VRPARAM) == 0) {
				g_fLensK2 = fValue;
			}
			else if (_stricmp(param, K3_VRPARAM) == 0) {
				g_fLensK3 = fValue;
			}
			else if (_stricmp(param, BARREL_EFFECT_STATE_VRPARAM) == 0) {
				g_bDisableBarrelEffect = !((bool)fValue);
			}
			else if (_stricmp(param, HUD_PARALLAX_VRPARAM) == 0) {
				g_fHUDDepth = fValue;
			}
			else if (_stricmp(param, FLOATING_AIMING_HUD_VRPARAM) == 0) {
				g_bFloatingAimingHUD = (bool)fValue;
			}
			else if (_stricmp(param, GUI_PARALLAX_VRPARAM) == 0) {
				// "Floating" GUI elements: targetting computer and the like
				g_fFloatingGUIDepth = fValue;
			}
			else if (_stricmp(param, GUI_OBJ_PARALLAX_VRPARAM) == 0) {
				// "Floating" GUI targeted elements
				g_fFloatingGUIObjDepth = fValue;
			}
			else if (_stricmp(param, TEXT_PARALLAX_VRPARAM) == 0) {
				g_fTextDepth = fValue;
			}
			else if (_stricmp(param, TECH_LIB_PARALLAX_VRPARAM) == 0) {
				g_fTechLibraryParallax = fValue;
			}
			else if (_stricmp(param, BRIGHTNESS_VRPARAM) == 0) {
				g_fBrightness = fValue;
			}
			else if (_stricmp(param, STICKY_ARROW_KEYS_VRPARAM) == 0) {
				g_bStickyArrowKeys = (bool)fValue;
			}
			else if (_stricmp(param, "yaw_pitch_from_mouse_override") == 0) {
				g_bYawPitchFromMouseOverride = (bool)fValue;
			}
			else if (_stricmp(param, VR_MODE_VRPARAM) == 0) {
				if (_stricmp(svalue, VR_MODE_NONE_SVAL) == 0) {
					//g_VRMode = VR_MODE_NONE;
					g_bEnableVR = false;
					g_bSteamVREnabled = false;
					log_debug("[DBG] Disabling VR");
				}
				else if (_stricmp(svalue, VR_MODE_DIRECT_SBS_SVAL) == 0) {
					//g_VRMode = VR_MODE_DIRECT_SBS;
					g_bSteamVREnabled = false;
					g_bEnableVR = true;
					// Let's force AspectRatioPreserved in VR mode. The aspect ratio is easier to compute that way
					g_config.AspectRatioPreserved = true;
					log_debug("[DBG] Using Direct SBS mode");
				}
				else if (_stricmp(svalue, VR_MODE_STEAMVR_SVAL) == 0) {
					//g_VRMode = VR_MODE_STEAMVR;
					g_bSteamVREnabled = true;
					g_bEnableVR = true;
					// Let's force AspectRatioPreserved in VR mode. The aspect ratio is easier to compute that way
					g_config.AspectRatioPreserved = true;
					log_debug("[DBG] Using SteamVR");
				}
			}
			else if (_stricmp(param, INTERLEAVED_REPROJ_VRPARAM) == 0) {
				g_bInterleavedReprojection = (bool)fValue;
				if (g_bUseSteamVR) {
					log_debug("[DBG] Setting Interleaved Reprojection to: %d", g_bInterleavedReprojection);
					g_pVRCompositor->ForceInterleavedReprojectionOn(g_bInterleavedReprojection);
				}
			}
			else if (_stricmp(param, STEAMVR_DISTORTION_ENABLED_VRPARAM) == 0) {
				g_bSteamVRDistortionEnabled = (bool)fValue;
			}
			else if (_stricmp(param, NATURAL_CONCOURSE_ANIM_VRPARAM) == 0) {
				g_iNaturalConcourseAnimations = (int)fValue;
			}
			else if (_stricmp(param, FIXED_GUI_VRPARAM) == 0) {
				g_bFixedGUI = (bool)fValue;
			}

			else if (_stricmp(param, "manual_dc_activate") == 0) {
				g_bDCManualActivate = (bool)fValue;
			}

			else if (_stricmp(param, "frame_time_remaining") == 0) {
				g_fFrameTimeRemaining = fValue;
			}

			else if (_stricmp(param, "SteamVR_submit_frame_threshold_ms") == 0) {
				g_iSteamVR_Remaining_ms = (int)fValue;
			}
			else if (_stricmp(param, "SteamVR_VSync_ms") == 0) {
				g_iSteamVR_VSync_ms = (int)fValue;
			}
			else if (_stricmp(param, RETICLE_SCALE_VRPARAM) == 0) {
				g_fReticleScale = fValue;
			}
			else if (_stricmp(param, TRIANGLE_POINTER_DIST_VRPARAM) == 0) {
				g_fTrianglePointerDist = fValue;
			}

			else if (_stricmp(param, "2D_yaw_mul") == 0) {
				g_f2DYawMul = fValue;
			}
			else if (_stricmp(param, "2D_pitch_mul") == 0) {
				g_f2DPitchMul = fValue;
			}
			else if (_stricmp(param, "2D_roll_mul") == 0) {
				g_f2DRollMul = fValue;
			}

			else if (_stricmp(param, "steamvr_mirror_window_scale") == 0) {
				// This one is used to zoom in the view in the mirror window to avoid showing
				// wide-FOV-related artifacts
				g_fSteamVRMirrorWindow3DScale = fValue;
			}
			else if (_stricmp(param, "steamvr_mirror_aspect_ratio") == 0) {
				// A value greater than 0 overrides the automatic aspect ratio computed by ddraw in
				// resizeForSteamVR().
				g_fSteamVRMirrorWindowAspectRatio = fValue;
			}
			else if (_stricmp(param, "steamvr_yaw_pitch_roll_from_mouse_look") == 0) {
				g_bSteamVRYawPitchRollFromMouseLook = (bool)fValue;
			}
			
			
			param_read_count++;
		}
	} // while ... read file
	// Apply the initial Zoom Out state:
	g_fGUIElemsScale = g_bZoomOut ? g_fGlobalScaleZoomOut : DEFAULT_GUI_SCALE;
	fclose(file);

next:
	/**g_fRawFOVDist = g_fDefaultFOVDist;
	*g_cachedFOVDist = g_fDefaultFOVDist / 512.0f;
	*g_rawFOVDist = (uint32_t)g_fDefaultFOVDist;*/

	// Load cockpit look params
	LoadCockpitLookParams();
	// Load the global dynamic cockpit coordinates
	LoadDCInternalCoordinates();
	// Load Dynamic Cockpit params
	LoadDCParams();
	// Load Active Cockpit params
	LoadACParams();
	// Load the Bloom params
	LoadBloomParams();
	// Load the SSAO params
	LoadSSAOParams();
	// Load the Hyperspace params
	LoadHyperParams();
	// Load FOV params
	LoadFocalLength();
	// Load the default global material
	LoadDefaultGlobalMaterial();
	// Reload the materials
	ReloadMaterials();
}

////////////////////////////////////////////////////////////////
// SteamVR functions
////////////////////////////////////////////////////////////////
char *GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
	char *pchBuffer = NULL;
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return pchBuffer;

	pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	return pchBuffer;
}

void DumpMatrix34(FILE *file, const vr::HmdMatrix34_t &m) {
	if (file == NULL)
		return;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			fprintf(file, "%0.6f", m.m[i][j]);
			if (j < 3)
				fprintf(file, ", ");
		}
		fprintf(file, "\n");
	}
}

void DumpMatrix44(FILE *file, const vr::HmdMatrix44_t &m) {
	if (file == NULL)
		return;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			fprintf(file, "%0.6f", m.m[i][j]);
			if (j < 3)
				fprintf(file, ", ");
		}
		fprintf(file, "\n");
	}
}

void DumpMatrix4(FILE *file, const Matrix4 &mat) {
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[0], mat[4], mat[8], mat[12]);
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[1], mat[5], mat[9], mat[13]);
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[2], mat[6], mat[10], mat[14]);
	fprintf(file, "%0.6f, %0.6f, %0.6f, %0.6f\n", mat[3], mat[7], mat[11], mat[15]);
}

void ShowMatrix4(const Matrix4 &mat, char *name) {
	log_debug("[DBG] -----------------------------------------");
	if (name != NULL)
		log_debug("[DBG] %s", name);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[0], mat[4], mat[8], mat[12]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[1], mat[5], mat[9], mat[13]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[2], mat[6], mat[10], mat[14]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat[3], mat[7], mat[11], mat[15]);
	log_debug("[DBG] =========================================");
}

void ShowHmdMatrix34(const vr::HmdMatrix34_t &mat, char *name) {
	log_debug("[DBG] -----------------------------------------");
	if (name != NULL)
		log_debug("[DBG] %s", name);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3]);
	log_debug("[DBG] =========================================");
}

void ShowHmdMatrix44(const vr::HmdMatrix44_t &mat, char *name) {
	log_debug("[DBG] -----------------------------------------");
	if (name != NULL)
		log_debug("[DBG] %s", name);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f", mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
	log_debug("[DBG] =========================================");
}

void Matrix4toHmdMatrix44(const Matrix4 &m4, vr::HmdMatrix44_t &mat) {
	mat.m[0][0] = m4[0];  mat.m[1][0] = m4[1];  mat.m[2][0] = m4[2];  mat.m[3][0] = m4[3];
	mat.m[0][1] = m4[4];  mat.m[1][1] = m4[5];  mat.m[2][1] = m4[6];  mat.m[3][1] = m4[7];
	mat.m[0][2] = m4[8];  mat.m[1][2] = m4[9];  mat.m[2][2] = m4[10]; mat.m[3][2] = m4[11];
	mat.m[0][3] = m4[12]; mat.m[1][3] = m4[13]; mat.m[2][3] = m4[14]; mat.m[3][3] = m4[14];
}

Matrix4 HmdMatrix44toMatrix4(const vr::HmdMatrix44_t &mat) {
	Matrix4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
	return matrixObj;
}

Matrix4 HmdMatrix34toMatrix4(const vr::HmdMatrix34_t &mat) {
	Matrix4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	);
	return matrixObj;
}

void ProcessSteamVREyeMatrices(vr::EVREye eye) {
	if (g_pHMD == NULL) {
		log_debug("[DBG] Cannot process SteamVR matrices because g_pHMD == NULL");
		return;
	}

	vr::HmdMatrix34_t eyeMatrix = g_pHMD->GetEyeToHeadTransform(eye);
	//ShowHmdMatrix34(eyeMatrix, "HmdMatrix34_t eyeMatrix");
	if (eye == vr::EVREye::Eye_Left)
		g_EyeMatrixLeft = eyeMatrix;
	else
		g_EyeMatrixRight = eyeMatrix;

	Matrix4 matrixObj = HmdMatrix34toMatrix4(eyeMatrix);
	/*
	// Pimax matrix: 11.11 degrees on the Y axis and 6.64 IPD
	Matrix4 matrixObj(
		0.984808, 0.000000, 0.173648, -0.033236,
		0.000000, 1.000000, 0.000000, 0.000000,
		-0.173648, 0.000000, 0.984808, 0.000000,
		0, 0, 0, 1);
	matrixObj.transpose();
	*/

	/*
	if (g_bInverseTranspose) {
		log_debug("[DBG] Computing Inverse Transpose");
		matrixObj.transpose();
	}
	*/
	// Invert the matrix and store it
	matrixObj.invertGeneral();
	if (eye == vr::EVREye::Eye_Left)
		g_EyeMatrixLeftInv = matrixObj;
	else
		g_EyeMatrixRightInv = matrixObj;
}

void ShowVector4(const Vector4 &X, char *name) {
	if (name != NULL)
		log_debug("[DBG] %s = %0.6f, %0.6f, %0.6f, %0.6f",
			name, X[0], X[1], X[2], X[3]);
	else
		log_debug("[DBG] %0.6f, %0.6f, %0.6f, %0.6f",
			X[0], X[1], X[2], X[3]);
}

void TestPimax() {
	vr::HmdMatrix34_t eyeLeft;
	eyeLeft.m[0][0] =  0.984808f; eyeLeft.m[0][1] = 0.000000f, eyeLeft.m[0][2] = 0.173648f; eyeLeft.m[0][3] = -0.033236f;
	eyeLeft.m[1][0] =  0.000000f; eyeLeft.m[1][1] = 1.000000f; eyeLeft.m[1][2] = 0.000000f; eyeLeft.m[1][3] =  0.000000f;
	eyeLeft.m[2][0] = -0.173648f; eyeLeft.m[2][1] = 0.000000f; eyeLeft.m[2][2] = 0.984808f; eyeLeft.m[2][3] =  0.000000f;

	vr::HmdMatrix34_t eyeRight;
	eyeRight.m[0][0] = 0.984808f; eyeRight.m[0][1] = 0.000000f, eyeRight.m[0][2] = -0.173648f; eyeRight.m[0][3] = 0.033236f;
	eyeRight.m[1][0] = 0.000000f; eyeRight.m[1][1] = 1.000000f; eyeRight.m[1][2] =  0.000000f; eyeRight.m[1][3] = 0.000000f;
	eyeRight.m[2][0] = 0.173648f; eyeRight.m[2][1] = 0.000000f; eyeRight.m[2][2] =  0.984808f; eyeRight.m[2][3] = 0.000000f;

	g_EyeMatrixLeftInv = HmdMatrix34toMatrix4(eyeLeft);
	g_EyeMatrixRightInv = HmdMatrix34toMatrix4(eyeRight);
	g_EyeMatrixLeftInv.invertGeneral();
	g_EyeMatrixRightInv.invertGeneral();

	vr::HmdMatrix44_t projLeft;
	projLeft.m[0][0] = 0.647594f; projLeft.m[0][1] = 0.000000f; projLeft.m[0][2] = -0.128239f; projLeft.m[0][3] =  0.000000f;
	projLeft.m[1][0] = 0.000000f; projLeft.m[1][1] = 0.787500f; projLeft.m[1][2] =  0.000000f; projLeft.m[1][3] =  0.000000f;
	projLeft.m[2][0] = 0.000000f; projLeft.m[2][1] = 0.000000f; projLeft.m[2][2] = -1.010101f; projLeft.m[2][3] = -0.505050f;
	projLeft.m[3][0] = 0.000000f; projLeft.m[3][1] = 0.000000f; projLeft.m[3][2] = -1.000000f; projLeft.m[3][3] =  0.000000f;

	vr::HmdMatrix44_t projRight;
	projRight.m[0][0] = 0.647594f; projRight.m[0][1] = 0.000000f; projRight.m[0][2] =  0.128239f; projRight.m[0][3] =  0.000000f;
	projRight.m[1][0] = 0.000000f; projRight.m[1][1] = 0.787500f; projRight.m[1][2] =  0.000000f; projRight.m[1][3] =  0.000000f;
	projRight.m[2][0] = 0.000000f; projRight.m[2][1] = 0.000000f; projRight.m[2][2] = -1.010101f; projRight.m[2][3] = -0.505050f;
	projRight.m[3][0] = 0.000000f; projRight.m[3][1] = 0.000000f; projRight.m[3][2] = -1.000000f; projRight.m[3][3] =  0.000000f;

	g_projLeft = HmdMatrix44toMatrix4(projLeft);
	g_projRight = HmdMatrix44toMatrix4(projRight);
}

void Test2DMesh() {
	/*
	MainVertex vertices[4] =
	{
		MainVertex(-1, -1, 0, 1),
		MainVertex(1, -1, 1, 1),
		MainVertex(1,  1, 1, 0),
		MainVertex(-1,  1, 0, 0),
	};
	*/
	Matrix4 fullMatrixLeft = g_FullProjMatrixLeft;
	//fullMatrixLeft.invertGeneral();
	Vector4 P, Q;

	P.set(-12, -12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);

	P.set(12, -12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);

	P.set(-12, 12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);

	P.set(12, 12, -30, 0);
	Q = fullMatrixLeft * P;
	Q = Q / Q[3];
	log_debug("[DBG] (%0.3f, %0.3f) --> (%0.3f, %0.3f)", P[0], P[1], Q[0], Q[1]);
}

bool InitSteamVR()
{
	/*
	How to enable the null driver:
	https://www.reddit.com/r/Vive/comments/6uo053/how_to_use_steamvr_tracked_devices_without_a_hmd/

	steamvr.vrsettings file:
	C:\Program Files (x86)\Steam\config

	null driver config file:
	C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\null\resources\settings
	*/
	char *strDriver = NULL;
	char *strDisplay = NULL;
	FILE *file = NULL;
	//Matrix4 RollTest;
	bool result = true;

	int file_error = fopen_s(&file, "./steamvr_mat.txt", "wt");
	log_debug("[DBG] Initializing SteamVR");
	vr::EVRInitError eError = vr::VRInitError_None;
	g_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	g_headCenter.set(0, 0, 0);

	if (eError != vr::VRInitError_None)
	{
		g_pHMD = NULL;
		log_debug("[DBG] Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		result = false;
		goto out;
	}
	log_debug("[DBG] SteamVR Initialized");
	g_pHMD->GetRecommendedRenderTargetSize(&g_steamVRWidth, &g_steamVRHeight);
	log_debug("[DBG] Recommended steamVR width, height: %d, %d", g_steamVRWidth, g_steamVRHeight);

	strDriver = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	if (strDriver) {
		log_debug("[DBG] Driver: %s", strDriver);
		strDisplay = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
	}

	if (strDisplay)
		log_debug("[DBG] Display: %s", strDisplay);

	g_pVRCompositor = vr::VRCompositor();
	if (g_pVRCompositor == NULL)
	{
		log_debug("[DBG] SteamVR Compositor Initialization failed.");
		result = false;
		goto out;
	}
	log_debug("[DBG] SteamVR Compositor Initialized");

	g_pVRCompositor->SetTrackingSpace(vr::TrackingUniverseSeated);
	g_pVRCompositor->ForceInterleavedReprojectionOn(g_bInterleavedReprojection);
	log_debug("[DBG] InterleavedReprojection: %d", g_bInterleavedReprojection);

	g_pVRScreenshots = vr::VRScreenshots();
	if (g_pVRScreenshots != NULL) {
		log_debug("[DBG] SteamVR screenshot system enabled");
	}

	// Reset the seated pose
	g_pHMD->ResetSeatedZeroPose();

	// Pre-multiply and store the eye and projection matrices:
	ProcessSteamVREyeMatrices(vr::EVREye::Eye_Left);
	ProcessSteamVREyeMatrices(vr::EVREye::Eye_Right);

	vr::HmdMatrix44_t projLeft = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Left, 0.001f, 100.0f);
	vr::HmdMatrix44_t projRight = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Right, 0.001f, 100.0f);

	g_projLeft  = HmdMatrix44toMatrix4(projLeft);
	g_projRight = HmdMatrix44toMatrix4(projRight);

	// Override all of the above with the Pimax matrices
	//TestPimax();

	g_FullProjMatrixLeft  = g_projLeft  * g_EyeMatrixLeftInv;
	g_FullProjMatrixRight = g_projRight * g_EyeMatrixRightInv;

	//Test2DMesh();

	ShowMatrix4(g_EyeMatrixLeftInv, "g_EyeMatrixLeftInv");
	ShowMatrix4(g_projLeft, "g_projLeft");

	ShowMatrix4(g_EyeMatrixRightInv, "g_EyeMatrixRightInv");
	ShowMatrix4(g_projRight, "g_projRight");
	
	// Initialize FreePIE if we're going to use it to read the position.
	if (g_bSteamVRPosFromFreePIE)
		InitFreePIE();

	// Compute the FOV from the left eye vertical params. This FOV will be applied
	// and saved in PrimarySurface::Flip()
	float left, right, top, bottom;
	g_pHMD->GetProjectionRaw(vr::EVREye::Eye_Left, &left, &right, &top, &bottom);
	g_fVR_FOV = (atan(fabs(top)) + atan(fabs(bottom))) / DEG2RAD;

	// Dump information about the view matrices
	if (file_error == 0) {
		Matrix4 eye, test;

		if (strDriver != NULL)
			fprintf(file, "Driver: %s\n", strDriver);
		if (strDisplay != NULL)
			fprintf(file, "Display: %s\n", strDisplay);
		fprintf(file, "\n");

		// Left Eye matrix
		fprintf(file, "eyeLeft:\n");
		DumpMatrix34(file, g_EyeMatrixLeft);
		fprintf(file, "\n");

		fprintf(file, "eyeLeftInv:\n");
		DumpMatrix4(file, g_EyeMatrixLeftInv);
		fprintf(file, "\n");

		eye = HmdMatrix34toMatrix4(g_EyeMatrixLeft);
		test = eye * g_EyeMatrixLeftInv;
		fprintf(file, "Left Eye inverse test:\n");
		DumpMatrix4(file, test);
		fprintf(file, "\n");

		// Right Eye matrix
		fprintf(file, "eyeRight:\n");
		DumpMatrix34(file, g_EyeMatrixRight);
		fprintf(file, "\n");

		fprintf(file, "eyeRightInv:\n");
		DumpMatrix4(file, g_EyeMatrixRightInv);
		fprintf(file, "\n");

		eye = HmdMatrix34toMatrix4(g_EyeMatrixRight);
		test = eye * g_EyeMatrixRightInv;
		fprintf(file, "Right Eye inverse test:\n");
		DumpMatrix4(file, test);
		fprintf(file, "\n");

		// Z_FAR was 50 for version 0.9.6, and I believe Z_Near was 0.5 (focal dist)
		//vr::HmdMatrix44_t projLeft = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Left, DEFAULT_FOCAL_DIST, 50.0f);
		//vr::HmdMatrix44_t projRight = g_pHMD->GetProjectionMatrix(vr::EVREye::Eye_Right, DEFAULT_FOCAL_DIST, 50.0f);

		fprintf(file, "projLeft:\n");
		DumpMatrix44(file, projLeft);
		fprintf(file, "\n");

		fprintf(file, "projRight:\n");
		DumpMatrix44(file, projRight);
		fprintf(file, "\n");

		g_pHMD->GetProjectionRaw(vr::EVREye::Eye_Left, &left, &right, &top, &bottom);
		fprintf(file, "Raw data (Left eye):\n");
		fprintf(file, "Left: %0.6f, Right: %0.6f, Top: %0.6f, Bottom: %0.6f\n",
			left, right, top, bottom);

		g_pHMD->GetProjectionRaw(vr::EVREye::Eye_Right, &left, &right, &top, &bottom);
		fprintf(file, "Raw data (Right eye):\n");
		fprintf(file, "Left: %0.6f, Right: %0.6f, Top: %0.6f, Bottom: %0.6f\n",
			left, right, top, bottom);
		fclose(file);
	}

out:
	g_bSteamVRInitialized = result;

	if (strDriver != NULL) {
		delete[] strDriver;
		strDriver = NULL;
	}
	if (strDisplay != NULL) {
		delete[] strDisplay;
		strDisplay = NULL;
	}
	if (file != NULL) {
		fclose(file);
		file = NULL;
	}
	return result;
}

void ShutDownSteamVR() {
	// If the user sets g_bSteamVRPosFromFreePIE when XWA is starting, they may
	// also reload the vrparams and then unset this flag. In that case, we won't
	// shut FreePIE down. Well, this is a private hack, so I'm not going to care
	// about that case.
	if (g_bSteamVRPosFromFreePIE)
		ShutdownFreePIE();

	// We can't shut down SteamVR twice, we either shut it down here or in the cockpit look hook.
	// It looks like the right order is to shut SteamVR down in the cockpit look hook
	return;
	//log_debug("[DBG] Not shutting down SteamVR, just returning...");
	//return;
	/*
	log_debug("[DBG] Shutting down SteamVR...");
	vr::VR_Shutdown();
	g_pHMD = NULL;
	g_pVRCompositor = NULL;
	g_pVRScreenshots = NULL;
	log_debug("[DBG] SteamVR shut down");
	*/
}

bool InitDirectSBS()
{
	InitFreePIE();
	g_headCenter.set(0, 0, 0);

	g_EyeMatrixLeftInv.set
	(
		1.0f, 0.0f, 0.0f, g_fHalfIPD,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	// Matrices are stored in column-major format, so we have to transpose them to
	// match the format above:
	g_EyeMatrixLeftInv.transpose();

	g_EyeMatrixRightInv.set
	(
		1.0f, 0.0f, 0.0f, -g_fHalfIPD,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	// Matrices are stored in column-major format, so we have to transpose them to
	// match the format above:
	g_EyeMatrixRightInv.transpose();

	/*
	Matrix from Trinus PSVR, June 11, 2020:
	0.847458, 0.000000, 0.000000, 0.000000
	0.000000, 0.746269, 0.000000, 0.000000
	0.000000, 0.000000, -1.000010, -0.001000
	0.000000, 0.000000, -1.000000, 0.000000

	iVRy reported these values:

	projLeft:
	0.847458, 0.000000,  0.000000,  0.000000
	0.000000, 0.746269,  0.000000,  0.000000
	0.000000, 0.000000, -1.010101, -0.505050
	0.000000, 0.000000, -1.000000,  0.000000

	Raw data (Left eye):
	Left: -1.180000, Right: 1.180000, Top: -1.340000, Bottom: 1.340000
	Raw data (Right eye):
	Left: -1.180000, Right: 1.180000, Top: -1.340000, Bottom: 1.340000

	atan(1.34) = 53.26 deg, so that's 106.53 degrees vertical FOV
	*/
	g_projLeft.set
	(
		0.847458f, 0.0f,       0.0f,  0.0f,
		0.0f,      0.746269f,  0.0f,  0.0f,
		0.0f,      0.0f,      -1.000010f, -0.001f,
		0.0f,      0.0f,      -1.0f,  0.0f
	);
	g_projLeft.transpose();
	g_projRight = g_projLeft;

	g_FullProjMatrixLeft  = g_projLeft  * g_EyeMatrixLeftInv;
	g_FullProjMatrixRight = g_projRight * g_EyeMatrixRightInv;

	//ShowMatrix4(g_EyeMatrixLeftInv, "g_EyeMatrixLeftInv");
	//ShowMatrix4(g_projLeft, "g_projLeft");
	//ShowMatrix4(g_EyeMatrixRightInv, "g_EyeMatrixRightInv");
	//ShowMatrix4(g_projRight, "g_projRight");

	// Set the vertical FOV to 106.53. This will be applied after the first frame is
	// rendered, in PrimarySurface::Flip
	g_fVR_FOV = PSVR_VERT_FOV;
	log_debug("[DBG] DirectSBS mode initialized");
	g_bDirectSBSInitialized = true;
	return true;
}

bool ShutDownDirectSBS() {
	ShutdownFreePIE();
	return true;
}

/*
void ProcessVREvent(const vr::VREvent_t & event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		log_debug("[DBG] Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		log_debug("[DBG] Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}
*/

/*
bool HandleVRInput()
{
	bool bRet = false;

	// Process SteamVR events
	vr::VREvent_t event;
	while (g_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		ProcessVREvent(event);
	}

	return bRet;
}
*/

////////////////////////////////////////////////////////////////
// End of SteamVR functions
////////////////////////////////////////////////////////////////

int g_ExecuteCount;
int g_ExecuteVertexCount;
int g_ExecuteIndexCount;
int g_ExecuteStateCount;
int g_ExecuteTriangleCount;

class RenderStates
{
public:
	RenderStates(DeviceResources* deviceResources)
	{
		this->_deviceResources = deviceResources;

		this->TextureAddress = D3DTADDRESS_WRAP;

		this->AlphaBlendEnabled = FALSE;
		this->TextureMapBlend = D3DTBLEND_MODULATE;
		this->SrcBlend = D3DBLEND_ONE;
		this->DestBlend = D3DBLEND_ZERO;

		this->ZEnabled = TRUE;
		this->ZWriteEnabled = TRUE;
		this->ZFunc = D3DCMP_GREATER;
	}
	
	static D3D11_TEXTURE_ADDRESS_MODE TextureAddressMode(D3DTEXTUREADDRESS address)
	{
		switch (address)
		{
		case D3DTADDRESS_WRAP:
			return D3D11_TEXTURE_ADDRESS_WRAP;
		case D3DTADDRESS_MIRROR:
			return D3D11_TEXTURE_ADDRESS_MIRROR;
		case D3DTADDRESS_CLAMP:
			return D3D11_TEXTURE_ADDRESS_CLAMP;
		}

		return D3D11_TEXTURE_ADDRESS_WRAP;
	}

	static D3D11_BLEND Blend(D3DBLEND blend)
	{
		switch (blend)
		{
		case D3DBLEND_ZERO:
			return D3D11_BLEND_ZERO; // 1
		case D3DBLEND_ONE:
			return D3D11_BLEND_ONE; // 2
		case D3DBLEND_SRCCOLOR:
			return D3D11_BLEND_SRC_COLOR; // 3
		case D3DBLEND_INVSRCCOLOR:
			return D3D11_BLEND_INV_SRC_COLOR; // 4
		case D3DBLEND_SRCALPHA:
			return D3D11_BLEND_SRC_ALPHA; // 5
		case D3DBLEND_INVSRCALPHA:
			return D3D11_BLEND_INV_SRC_ALPHA; // 6
		case D3DBLEND_DESTALPHA:
			return D3D11_BLEND_DEST_ALPHA;
		case D3DBLEND_INVDESTALPHA:
			return D3D11_BLEND_INV_DEST_ALPHA;
		case D3DBLEND_DESTCOLOR:
			return D3D11_BLEND_DEST_COLOR;
		case D3DBLEND_INVDESTCOLOR:
			return D3D11_BLEND_INV_DEST_COLOR;
		case D3DBLEND_SRCALPHASAT:
			return D3D11_BLEND_SRC_ALPHA_SAT;
		case D3DBLEND_BOTHSRCALPHA:
			return D3D11_BLEND_SRC1_ALPHA;
		case D3DBLEND_BOTHINVSRCALPHA:
			return D3D11_BLEND_INV_SRC1_ALPHA;
		}

		return D3D11_BLEND_ZERO;
	}

	static D3D11_COMPARISON_FUNC ComparisonFunc(D3DCMPFUNC func)
	{
		switch (func)
		{
		case D3DCMP_NEVER:
			return D3D11_COMPARISON_NEVER;
		case D3DCMP_LESS:
			return D3D11_COMPARISON_LESS;
		case D3DCMP_EQUAL:
			return D3D11_COMPARISON_EQUAL;
		case D3DCMP_LESSEQUAL:
			return D3D11_COMPARISON_LESS_EQUAL;
		case D3DCMP_GREATER:
			return D3D11_COMPARISON_GREATER;
		case D3DCMP_NOTEQUAL:
			return D3D11_COMPARISON_NOT_EQUAL;
		case D3DCMP_GREATEREQUAL:
			return D3D11_COMPARISON_GREATER_EQUAL;
		case D3DCMP_ALWAYS:
			return D3D11_COMPARISON_ALWAYS;
		}

		return D3D11_COMPARISON_ALWAYS;
	}

	D3D11_SAMPLER_DESC GetSamplerDesc()
	{
		D3D11_SAMPLER_DESC desc;
		desc.Filter = this->_deviceResources->_useAnisotropy ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.MaxAnisotropy = this->_deviceResources->_useAnisotropy ? this->_deviceResources->GetMaxAnisotropy() : 1;
		desc.AddressU = TextureAddressMode(this->TextureAddress);
		desc.AddressV = TextureAddressMode(this->TextureAddress);
		desc.AddressW = TextureAddressMode(this->TextureAddress);
		desc.MipLODBias = 0.0f;
		desc.MinLOD = 0;
		desc.MaxLOD = FLT_MAX;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.BorderColor[0] = 0.0f;
		desc.BorderColor[1] = 0.0f;
		desc.BorderColor[2] = 0.0f;
		desc.BorderColor[3] = 0.0f;

		return desc;
	}

	D3D11_BLEND_DESC GetBlendDesc()
	{
		D3D11_BLEND_DESC desc{};

		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;
		desc.RenderTarget[0].BlendEnable = this->AlphaBlendEnabled;
		desc.RenderTarget[0].SrcBlend = Blend(this->SrcBlend);
		desc.RenderTarget[0].DestBlend = Blend(this->DestBlend);
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		desc.RenderTarget[0].SrcBlendAlpha = this->TextureMapBlend == D3DTBLEND_MODULATEALPHA ? D3D11_BLEND_SRC_ALPHA : D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = this->TextureMapBlend == D3DTBLEND_MODULATEALPHA ? D3D11_BLEND_INV_SRC_ALPHA : D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		return desc;
	}

	D3D11_DEPTH_STENCIL_DESC GetDepthStencilDesc()
	{
		D3D11_DEPTH_STENCIL_DESC desc{};

		desc.DepthEnable = this->ZEnabled;
		desc.DepthWriteMask = this->ZWriteEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = ComparisonFunc(this->ZFunc);
		desc.StencilEnable = FALSE;

		return desc;
	}

	inline void SetTextureAddress(D3DTEXTUREADDRESS textureAddress)
	{
		this->TextureAddress = textureAddress;
	}

	inline void SetAlphaBlendEnabled(BOOL alphaBlendEnabled)
	{
		this->AlphaBlendEnabled = alphaBlendEnabled;
	}

	inline void SetTextureMapBlend(D3DTEXTUREBLEND textureMapBlend)
	{
		this->TextureMapBlend = textureMapBlend;
	}

	inline void SetSrcBlend(D3DBLEND srcBlend)
	{
		this->SrcBlend = srcBlend;
	}

	inline void SetDestBlend(D3DBLEND destBlend)
	{
		this->DestBlend = destBlend;
	}

	inline void SetZEnabled(BOOL zEnabled)
	{
		this->ZEnabled = zEnabled;
	}

	inline bool GetZEnabled()
	{
		return this->ZEnabled;
	}

	inline void SetZWriteEnabled(BOOL zWriteEnabled)
	{
		this->ZWriteEnabled = zWriteEnabled;
	}

	inline bool GetZWriteEnabled()
	{
		return this->ZWriteEnabled;
	}

	inline void SetZFunc(D3DCMPFUNC zFunc)
	{
		this->ZFunc = zFunc;
	}

	inline D3DCMPFUNC GetZFunc() {
		return this->ZFunc;
	}

public: // HACK: Return this to private after the Dynamic Cockpit is stable
	DeviceResources* _deviceResources;

	D3DTEXTUREADDRESS TextureAddress;

	BOOL AlphaBlendEnabled;
	D3DTEXTUREBLEND TextureMapBlend;
	D3DBLEND SrcBlend;
	D3DBLEND DestBlend;

	BOOL ZEnabled;
	BOOL ZWriteEnabled;
	D3DCMPFUNC ZFunc;
};

Direct3DDevice::Direct3DDevice(DeviceResources* deviceResources)
{
	this->_refCount = 1;
	this->_deviceResources = deviceResources;

	this->_renderStates = new RenderStates(this->_deviceResources);

	this->_maxExecuteBufferSize = 0;
}

Direct3DDevice::~Direct3DDevice()
{
	delete this->_renderStates;
}

HRESULT Direct3DDevice::QueryInterface(
	REFIID riid,
	LPVOID* obp
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tE_NOINTERFACE");
	LogText(str.str());
#endif

	return E_NOINTERFACE;
}

ULONG Direct3DDevice::AddRef()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount++;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	return this->_refCount;
}

ULONG Direct3DDevice::Release()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount--;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	if (this->_refCount == 0)
	{
		delete this;
		return 0;
	}

	return this->_refCount;
}

HRESULT Direct3DDevice::Initialize(
	LPDIRECT3D lpd3d,
	LPGUID lpGUID,
	LPD3DDEVICEDESC lpd3ddvdesc
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::GetCaps(
	LPD3DDEVICEDESC lpD3DHWDevDesc,
	LPD3DDEVICEDESC lpD3DHELDevDesc
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::SwapTextureHandles(
	LPDIRECT3DTEXTURE lpD3DTex1,
	LPDIRECT3DTEXTURE lpD3DTex2
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::CreateExecuteBuffer(
	LPD3DEXECUTEBUFFERDESC lpDesc,
	LPDIRECT3DEXECUTEBUFFER *lplpDirect3DExecuteBuffer,
	IUnknown *pUnkOuter
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpDesc == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	if ((lpDesc->dwFlags & D3DDEB_BUFSIZE) == 0)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	if (lplpDirect3DExecuteBuffer == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	if (lpDesc->dwBufferSize > this->_maxExecuteBufferSize || lpDesc->dwBufferSize < this->_maxExecuteBufferSize / 4)
	{
		auto& device = this->_deviceResources->_d3dDevice;

		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.ByteWidth = lpDesc->dwBufferSize;
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		if (FAILED(device->CreateBuffer(&vertexBufferDesc, nullptr, &this->_vertexBuffer)))
			return DDERR_INVALIDOBJECT;

		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.ByteWidth = lpDesc->dwBufferSize;
		indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		if (FAILED(device->CreateBuffer(&indexBufferDesc, nullptr, &this->_indexBuffer)))
			return DDERR_INVALIDOBJECT;

		this->_maxExecuteBufferSize = lpDesc->dwBufferSize;
	}

	*lplpDirect3DExecuteBuffer = new Direct3DExecuteBuffer(this->_deviceResources, lpDesc->dwBufferSize * 2, this);

#if LOGGER
	str.str("");
	str << "\t" << *lplpDirect3DExecuteBuffer;
	LogText(str.str());
#endif

	return D3D_OK;
}

HRESULT Direct3DDevice::GetStats(
	LPD3DSTATS lpD3DStats
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

#ifdef DBG_VR
void DumpOrigVertices(FILE *file, int numVerts)
{
	char buf[256];
	float px, py, pz, rhw;

	// Don't catpure things we are not drawing
	if (g_iDrawCounter < g_iNoDrawBeforeIndex)
		return;
	if (g_iNoDrawAfterIndex > -1 && g_iDrawCounter > g_iNoDrawAfterIndex)
		return;

	for (register int i = 0; i < numVerts; i++) {
		px = g_OrigVerts[i].sx;
		py = g_OrigVerts[i].sy;
		pz = g_OrigVerts[i].sz;
		rhw = g_OrigVerts[i].rhw;

		// 1/rhw is the linear Z depth times a scale factor
		sprintf_s(buf, 256, "%f %f %f %f %f\n", px, py, pz, rhw, 1.0f/rhw);
		fprintf(file, buf);
	}
}
#endif

/* 
 * In SteamVR, the coordinate system is as follows:
 * +x is right
 * +y is up
 * -z is forward
 * Distance is meters
 * This function is not used anymore and the projection is done in the vertex shader
 */
void projectSteamVR(float X, float Y, float Z, vr::EVREye eye, float &x, float &y, float &z) {
	Vector4 PX; // PY;

	PX.set(X, -Y, -Z, 1.0f);
	if (eye == vr::EVREye::Eye_Left) {
		PX = g_FullProjMatrixLeft * PX;
	} else {
		PX = g_FullProjMatrixRight * PX;
	}
	// Project
	PX /= PX[3];
	// Convert to 2D
	x =  PX[0];
	y = -PX[1];
	z =  PX[2];
}

/* Function to quickly enable/disable ZWrite. Currently only used for brackets */
HRESULT Direct3DDevice::QuickSetZWriteEnabled(BOOL Enabled) {
	HRESULT hr;
	D3D11_DEPTH_STENCIL_DESC desc = this->_renderStates->GetDepthStencilDesc();
	ComPtr<ID3D11DepthStencilState> depthState;
	auto &resources = this->_deviceResources;

	desc.DepthEnable = Enabled;
	desc.DepthWriteMask = Enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = Enabled ? D3D11_COMPARISON_GREATER : D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	hr = resources->_d3dDevice->CreateDepthStencilState(&desc, &depthState);
	if (SUCCEEDED(hr))
		resources->_d3dDeviceContext->OMSetDepthStencilState(depthState, 0);
	return hr;
}

inline float lerp(float x, float y, float s) {
	return x + s * (y - x);
}

/*
// Should this function be inlined?
// See:
// xwa_ddraw_d3d11-sm4\impl11\ddraw-May-6-2019-Functionality-Complete\Direct3DDevice.cpp
// For a working implementation of the index buffer read (spoiler alert: it's the same we have here)
void Direct3DDevice::GetBoundingBox(LPD3DINSTRUCTION instruction, UINT curIndex,
	float *minX, float *minY, float *maxX, float *maxY, bool debug) {
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	D3DTLVERTEX v;
	WORD index;
	//int aux_idx = curIndex;
	float px, py;
	*maxX = -1; *maxY = -1;
	*minX = 1000000; *minY = 1000000;
	for (WORD i = 0; i < instruction->wCount; i++)
	{
		index = triangle->v1;
		px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		if (px < *minX) *minX = px; if (px > *maxX) *maxX = px;
		if (py < *minY) *minY = py; if (py > *maxY) *maxY = py;
		if (debug) {
			v = g_OrigVerts[index];
			log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f, tu: %0.3f, tv: %0.3f", v.sx, v.sy, v.sz, v.rhw, v.tu, v.tv);
		}

		index = triangle->v2;
		px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		if (px < *minX) *minX = px; if (px > *maxX) *maxX = px;
		if (py < *minY) *minY = py; if (py > *maxY) *maxY = py;
		if (debug) {
			v = g_OrigVerts[index];
			log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f, tu: %0.3f, tv: %0.3f", v.sx, v.sy, v.sz, v.rhw, v.tu, v.tv);
		}

		index = triangle->v3;
		px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		if (px < *minX) *minX = px; if (px > *maxX) *maxX = px;
		if (py < *minY) *minY = py; if (py > *maxY) *maxY = py;
		if (debug) {
			v = g_OrigVerts[index];
			log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f, tu: %0.3f, tv: %0.3f", v.sx, v.sy, v.sz, v.rhw, v.tu, v.tv);
		}

		triangle++;
	}
}
*/

void DisplayCoords(LPD3DINSTRUCTION instruction, UINT curIndex) {
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	D3DTLVERTEX vert;
	uint32_t index;
	UINT idx = curIndex;
	
	log_debug("[DBG] START Geom");
	for (WORD i = 0; i < instruction->wCount; i++)
	{
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		vert = g_OrigVerts[index];
		log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f", vert.sx, vert.sy, vert.sz, vert.rhw);
		// , tu: %0.3f, tv: %0.3f, vert.tu, vert.tv

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f", vert.sx, vert.sy, vert.sz, vert.rhw);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f", vert.sx, vert.sy, vert.sz, vert.rhw);
		if (!g_config.D3dHookExists) triangle++;
	}
	log_debug("[DBG] END Geom");
}

void Direct3DDevice::GetBoundingBoxUVs(LPD3DINSTRUCTION instruction, UINT curIndex,
	float *minX, float *minY, float *maxX, float *maxY, 
	float *minU, float *minV, float *maxU, float *maxV,
	bool debug) 
{
	/* 
	 * If the D3D hook is active, we need to use the index buffer directly and 
	 * switch to 32-bit indices. If not, we can continue to use the triangle and
	 * 16-bit indices.
	 */
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	D3DTLVERTEX vert;
	uint32_t index;
	UINT idx = curIndex;
	float px, py, u, v;
	*maxX = -1000000; *maxY = -1000000;
	*minX =  1000000; *minY =  1000000;
	*minU =  100; *minV =  100;
	*maxU = -100; *maxV = -100;
	if (debug)
		log_debug("[DBG] START Geom");
	for (WORD i = 0; i < instruction->wCount; i++)
	{
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		u  = g_OrigVerts[index].tu; v  = g_OrigVerts[index].tv;
		if (px < *minX) *minX = px; if (px > *maxX) *maxX = px;
		if (py < *minY) *minY = py; if (py > *maxY) *maxY = py;
		if (u < *minU) *minU = u; if (u > *maxU) *maxU = u;
		if (v < *minV) *minV = v; if (v > *maxV) *maxV = v;
		if (debug) {
			vert = g_OrigVerts[index];
			log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f, tu: %0.3f, tv: %0.3f", vert.sx, vert.sy, vert.sz, vert.rhw, vert.tu, vert.tv);
		}

		//index = triangle->v2;
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		u  = g_OrigVerts[index].tu; v  = g_OrigVerts[index].tv;
		if (px < *minX) *minX = px; if (px > *maxX) *maxX = px;
		if (py < *minY) *minY = py; if (py > *maxY) *maxY = py;
		if (u < *minU) *minU = u; if (u > *maxU) *maxU = u;
		if (v < *minV) *minV = v; if (v > *maxV) *maxV = v;
		if (debug) {
			vert = g_OrigVerts[index];
			log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f, tu: %0.3f, tv: %0.3f", vert.sx, vert.sy, vert.sz, vert.rhw, vert.tu, vert.tv);
		}

		//index = triangle->v3;
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		u  = g_OrigVerts[index].tu; v  = g_OrigVerts[index].tv;
		if (px < *minX) *minX = px; if (px > *maxX) *maxX = px;
		if (py < *minY) *minY = py; if (py > *maxY) *maxY = py;
		if (u < *minU) *minU = u; if (u > *maxU) *maxU = u;
		if (v < *minV) *minV = v; if (v > *maxV) *maxV = v;
		if (debug) {
			vert = g_OrigVerts[index];
			log_debug("[DBG] sx: %0.6f, sy: %0.6f, sz: %0.6f, rhw: %0.6f, tu: %0.3f, tv: %0.3f", vert.sx, vert.sy, vert.sz, vert.rhw, vert.tu, vert.tv);
		}
		if (!g_config.D3dHookExists) triangle++;
	}
	if (debug)
		log_debug("[DBG] END Geom");
}

inline void InGameToScreenCoords(UINT left, UINT top, UINT width, UINT height, float x, float y, float *x_out, float *y_out)
{
	*x_out = left + x / g_fCurInGameWidth * width;
	*y_out = top + y / g_fCurInGameHeight * height;
}

void ScreenCoordsToInGame(float left, float top, float width, float height, float x, float y, float *x_out, float *y_out)
{
	*x_out = g_fCurInGameWidth * (x - left) / width;
	*y_out = g_fCurInGameHeight * (y - top) / height;
}

/*
bool rayTriangleIntersect_old(
	const Vector3 &orig, const Vector3 &dir,
	const Vector3 &v0, const Vector3 &v1, const Vector3 &v2,
	float &t, Vector3 &P, float &u, float &v)
{
	// From https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates
	// compute plane's normal
	Vector3 v0v1 = v1 - v0;
	Vector3 v0v2 = v2 - v0;
	// no need to normalize
	Vector3 N = v0v1.cross(v0v2); // N 
	float denom = N.dot(N);

	// Step 1: finding P

	// check if ray and plane are parallel ?
	float NdotRayDirection = N.dot(dir);
	if (fabs(NdotRayDirection) < 0.00001) // almost 0 
		return false; // they are parallel so they don't intersect ! 

	// compute d parameter using equation 2
	float d = N.dot(v0);

	// compute t (equation 3)
	t = (N.dot(orig) + d) / NdotRayDirection;
	// check if the triangle is in behind the ray
	if (t < 0) return false; // the triangle is behind

	// compute the intersection point using equation 1
	P = orig + t * dir;

	// Step 2: inside-outside test
	Vector3 C; // vector perpendicular to triangle's plane 

	// edge 0
	Vector3 edge0 = v1 - v0;
	Vector3 vp0 = P - v0;
	C = edge0.cross(vp0);
	if (N.dot(C) < 0) return false; // P is on the right side 

	// edge 1
	Vector3 edge1 = v2 - v1;
	Vector3 vp1 = P - v1;
	C = edge1.cross(vp1);
	if ((u = N.dot(C)) < 0)  return false; // P is on the right side

	// edge 2
	Vector3 edge2 = v0 - v2;
	Vector3 vp2 = P - v2;
	C = edge2.cross(vp2);
	if ((v = N.dot(C)) < 0) return false; // P is on the right side; 

	u /= denom;
	v /= denom;

	return true; // this ray hits the triangle 
}
*/

#define MOLLER_TRUMBORE
//#undef MOLLER_TRUMBORE
#undef CULLING
bool rayTriangleIntersect(
	const Vector3 &orig, const Vector3 &dir,
	const Vector3 &v0, const Vector3 &v1, const Vector3 &v2,
	float &t, Vector3 &P, float &u, float &v)
{
	// From: https://www.scratchapixel.com/code.php?id=9&origin=/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle
#ifdef MOLLER_TRUMBORE 
	Vector3 v0v1 = v1 - v0;
	Vector3 v0v2 = v2 - v0;
	Vector3 pvec = dir.cross(v0v2);
	float det = v0v1.dot(pvec);
#ifdef CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < 0.00001 /* kEpsilon */) return false;
#else 
	// ray and triangle are parallel if det is close to 0
	if (fabs(det) < 0.00001 /* kEpsilon */) return false;
#endif 
	float invDet = 1.0f / det;

	Vector3 tvec = orig - v0;
	u = tvec.dot(pvec) * invDet;
	if (u < 0 || u > 1) return false;

	Vector3 qvec = tvec.cross(v0v1);
	v = dir.dot(qvec) * invDet;
	if (v < 0 || u + v > 1) return false;

	t = v0v2.dot(qvec) * invDet;
	P = orig + t * dir;

	// Compute u-v again to make them consistent with tex coords
	Vector3 N = v0v1.cross(v0v2); // N 
	Vector3 C;
	// edge 1
	Vector3 edge1 = v2 - v1;
	Vector3 vp1 = P - v1;
	C = edge1.cross(vp1);
	u = N.dot(C);
	
	// edge 2
	Vector3 edge2 = v0 - v2;
	Vector3 vp2 = P - v2;
	C = edge2.cross(vp2);
	v = N.dot(C);

	float denom = N.dot(N);
	u /= denom;
	v /= denom;
	return true;
#else 
	// compute plane's normal
	Vector3 v0v1 = v1 - v0;
	Vector3 v0v2 = v2 - v0;
	// no need to normalize
	Vector3 N = v0v1.cross(v0v2); // N 
	float denom = N.dot(N);

	// Step 1: finding P

	// check if ray and plane are parallel ?
	float NdotRayDirection = N.dot(dir);
	if (fabs(NdotRayDirection) < 0.00001 /* kEpsilon */) // almost 0 
		return false; // they are parallel so they don't intersect ! 

	// compute d parameter using equation 2
	float d = N.dot(v0);

	// compute t (equation 3)
	t = (N.dot(orig) + d) / NdotRayDirection;
	// check if the triangle is in behind the ray
	if (t < 0) return false; // the triangle is behind 

	// compute the intersection point using equation 1
	P = orig + t * dir;

	// Step 2: inside-outside test
	Vector3 C; // vector perpendicular to triangle's plane 

	// edge 0
	Vector3 edge0 = v1 - v0;
	Vector3 vp0 = P - v0;
	C = edge0.cross(vp0);
	if (N.dot(C) < 0) return false; // P is on the right side 

	// edge 1
	Vector3 edge1 = v2 - v1;
	Vector3 vp1 = P - v1;
	C = edge1.cross(vp1);
	if ((u = N.dot(C)) < 0)  return false; // P is on the right side 

	// edge 2
	Vector3 edge2 = v0 - v2;
	Vector3 vp2 = P - v2;
	C = edge2.cross(vp2);
	if ((v = N.dot(C)) < 0) return false; // P is on the right side; 

	u /= denom;
	v /= denom;

	return true; // this ray hits the triangle 
#endif 
}

/*
 * Fully-metric back-projection. This should write OBJ files that match the scale
 * of OPT files 1:1 regardless of FOV and in-game resolution. This should help fix
 * the distortion reported by users in SteamVR.
 * This code is based on AddGeometryVertexShader -- it's basically the inverse of that
 * code up to a scale factor.
 *
 * INPUT:
 *		in-game 2D coordinates + rhw
 * OUTPUT:
 *		OPT-scale metric 3D centered on the current camera.
 *	    VR path: same as non-VR but with Y flipped
 */
inline void backProjectMetric(float sx, float sy, float rhw, Vector3 *P) {
	float3 temp;
	float FOVscaleZ;
	float sm_FOVscale = g_MetricRecCBuffer.mr_FOVscale;
	float sm_aspect_ratio = g_MetricRecCBuffer.mr_aspect_ratio;
	float sm_y_center = g_MetricRecCBuffer.mr_y_center;

	temp.z = sm_FOVscale * (1.0f / rhw) * g_MetricRecCBuffer.mr_z_metric_mult;
	// temp.z is now metric 3D minus the g_fOBJCurMetricScale scale factor
	FOVscaleZ = sm_FOVscale / temp.z;

	// Normalize into the 0.0..1.0 range
	temp.x = sx * g_VSCBuffer.viewportScale[0];
	temp.y = sy * g_VSCBuffer.viewportScale[1];

	if (g_bEnableVR)
	{
		temp.x *=  2.0f;
		temp.y *= -2.0f;
	}
	temp.x += -1.0f;
	temp.y +=  1.0f;
	// temp.x is now in the range -1.0 .. 1.0 and
	// temp.y is now in the range  1.0 ..-1.0
	// temp.xy is now in DirectX coords [-1..1]

	// P.x = sm_aspect_ratio * P2D.x / (FOVscale/P.z)
	temp.x = temp.x / FOVscaleZ * sm_aspect_ratio;
	// P.y = P2D.y / (FOVscale/P.z) - P.z * y_center / FOVscale
	temp.y = temp.y / FOVscaleZ - temp.z * sm_y_center / sm_FOVscale;
	// temp.xyz is now "straight" 3D up to a scale factor

	// g_fOBJCurMetricScale depends on the current in-game height and is computed
	// whenever a new FOV is applied. See ComputeHyperFOVParams()
	temp.x *= g_MetricRecCBuffer.mr_cur_metric_scale;
	temp.y *= g_MetricRecCBuffer.mr_cur_metric_scale;
	temp.z *= g_MetricRecCBuffer.mr_cur_metric_scale;

	// DEBUG
	// Final axis flip to get something viewable nicely in Blender:
	//temp.z = -temp.z;
	// DEBUG

	P->x = temp.x;
	P->y = temp.y;
	P->z = temp.z;

	if (g_bEnableVR)
		// Further adjustment of the coordinates for the DirectSBS/SteamVR case:
		P->y = -P->y;
}

// Back-project a 2D vertex (specified in in-game coords) stored in g_OrigVerts 
// into OBJ METRIC 3D
inline void backProjectMetric(UINT index, Vector3 *P) {
	backProjectMetric(g_OrigVerts[index].sx, g_OrigVerts[index].sy, g_OrigVerts[index].rhw, P);
}

/*
 * Fully-metric projection: This is simply the inverse of the backProjectMetric code
 * INPUT: 
 *		OPT-scale metric 3D centered on the current camera.
 * OUTPUT:
 *		(regular and VR paths): post-proc UV coords. (Confirmed by rendering the XWA lights
 *		in the external HUD shader).
 */
inline Vector3 projectMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR) {
	Vector3 P, temp = pos3D;

	if (!bForceNonVR && g_bEnableVR) {
		float w;
		// We need to invert the sign of the z coord because the matrices are defined in the SteamVR
		// coord system
		Vector4 Q = Vector4(temp.x, temp.y, -temp.z, 1.0f);
		Q = projEyeMatrix * viewMatrix * Q;

		// output.pos = mul(projEyeMatrix, output.pos);
		P.x = Q.x;
		P.y = Q.y;
		P.z = Q.z;
		w   = Q.w;

		// DirectX divides by w internally after the PixelShader output is written. We don't
		// see that division in the shader; but we have to do it explicitly here because
		// that's what actually accomplishes the 3D -> 2D projection (it's like a weighed 
		// division by Z)
		P.x /= w;
		P.y /= w;

		// P is now in the internal DirectX coord sys: xy in (-1..1)
		// So let's transform to the range 0..1 for post-proc coords:
		P.x = P.x * 0.5f + 0.5f;
		P.y = P.y * 0.5f + 0.5f;
	}
	else {
		float sm_FOVscale = g_MetricRecCBuffer.mr_FOVscale;
		float sm_aspect_ratio = g_MetricRecCBuffer.mr_aspect_ratio;
		float sm_y_center = g_MetricRecCBuffer.mr_y_center;
		float FOVscaleZ;

		// (x0,y0)-(x1,y1) are the viewport limits
		float x0, y0, x1, y1;
		GetScreenLimitsInUVCoords(&x0, &y0, &x1, &y1);

		// g_fOBJCurMetricScale depends on the current in-game height and is computed
		// whenever a new FOV is applied. See ComputeHyperFOVParams()
		temp /= g_fOBJCurMetricScale;
		FOVscaleZ = sm_FOVscale / temp.z;

		temp.x = FOVscaleZ * (temp.x / sm_aspect_ratio);
		temp.y = FOVscaleZ * (temp.y + temp.z * sm_y_center / sm_FOVscale);
		// temp.xy is now in DirectX coords [-1..1]:
		// temp.x is now in the range -1.0 .. 1.0 and
		// temp.y is now in the range  1.0 ..-1.0

		temp.x -= -1.0f;
		temp.y -=  1.0f;
		// Normalize into the 0..2 or 0.0..1.0 range
		temp.x /= g_VSCBuffer.viewportScale[0];
		temp.y /= g_VSCBuffer.viewportScale[1];
		if (bForceNonVR && g_bEnableVR) {
			temp.x /=  2.0f;
			temp.y /= -2.0f;
		}
		// temp.xy is now (sx,sy): in-game screen coords
		// sx = temp.x; sy = temp.y;

		// The following lines are simply implementing the following formulas used from the VertexShader:
		//output.pos.x = (input.pos.x * vpScale.x - 1.0f) * vpScale.z;
		//output.pos.y = (input.pos.y * vpScale.y + 1.0f) * vpScale.z;
		if (bForceNonVR && g_bEnableVR) {
			temp.x = ( 2.0f * temp.x * g_VSCBuffer.viewportScale[0] - 1.0f) * g_VSCBuffer.viewportScale[2];
			temp.y = (-2.0f * temp.y * g_VSCBuffer.viewportScale[1] + 1.0f) * g_VSCBuffer.viewportScale[2];
		}
		else {
			temp.x = (temp.x * g_VSCBuffer.viewportScale[0] - 1.0f) * g_VSCBuffer.viewportScale[2];
			temp.y = (temp.y * g_VSCBuffer.viewportScale[1] + 1.0f) * g_VSCBuffer.viewportScale[2];
		}
		// temp.x is now in -1 ..  1 (left ... right)
		// temp.y is now in  1 .. -1 (up ... down)

		// Now convert to UV coords: (0, 1)-(1, 0):
		// By using x0,y0-x1,y1 as limits, we're now converting to post-proc viewport UVs
		P.x = lerp(x0, x1, (temp.x + 1.0f) / 2.0f);
		P.y = lerp(y1, y0, (temp.y + 1.0f) / 2.0f);
		P.z = temp.z;
		// Reconstruct w:
		//w = temp.z / (sm_FOVscale * g_fOBJZMetricMult);
	}
	return P;
}

/*
 * INPUT:
 *		OPT-scale metric 3D centered on the current camera.
 * OUTPUT :
 *		Regular path: in-game 2D coords (sx, sy)
 *	    VR path: Post-proc coords
 */
inline Vector3 projectToInGameOrPostProcCoordsMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR)
{
	Vector3 P = pos3D;
	//float w;
	// Whatever is placed in P is returned at the end of this function

	if (!bForceNonVR && g_bEnableVR) {
		float w;
		// We need to invert the sign of the z coord because the matrices are defined in the SteamVR
		// coord system
		Vector4 Q = Vector4(P.x, P.y, -P.z, 1.0f);
		Q = projEyeMatrix * viewMatrix * Q;

		// output.pos = mul(projEyeMatrix, output.pos);
		P.x = Q.x;
		P.y = Q.y;
		P.z = Q.z;
		w   = Q.w;
		// Multiply by focal_dist? The focal dist should be in the projection matrix...

		// DirectX divides by w internally after the PixelShader output is written. We don't
		// see that division in the shader; but we have to do it explicitly here because
		// that's what actually accomplishes the 3D -> 2D projection (it's like a weighed 
		// division by Z)
		P.x /= w;
		P.y /= w;

		// P is now in the internal DirectX coord sys: xy in (-1..1)
		// So let's transform to the range 0..1 for post-proc coords:
		P.x = P.x * 0.5f + 0.5f;
		P.y = P.y * 0.5f + 0.5f;
	}
	else {
		Vector3 temp = pos3D;
		float sm_FOVscale = g_MetricRecCBuffer.mr_FOVscale;
		float sm_aspect_ratio = g_MetricRecCBuffer.mr_aspect_ratio;
		float sm_y_center = g_MetricRecCBuffer.mr_y_center;
		float FOVscaleZ;

		// g_fOBJCurMetricScale depends on the current in-game height and is computed
		// whenever a new FOV is applied. See ComputeHyperFOVParams()
		temp /= g_fOBJCurMetricScale;
		FOVscaleZ = sm_FOVscale / temp.z;

		temp.x = FOVscaleZ * (temp.x / sm_aspect_ratio);
		temp.y = FOVscaleZ * (temp.y + temp.z * sm_y_center / sm_FOVscale);

		// temp.xy is now in DirectX coords [-1..1]:
		// temp.x is now in the range -1.0 .. 1.0 and
		// temp.y is now in the range  1.0 ..-1.0

		temp.x -= -1.0f;
		temp.y -=  1.0f;

		// Normalize into the 0..2 or 0.0..1.0 range
		temp.x /= g_VSCBuffer.viewportScale[0];
		temp.y /= g_VSCBuffer.viewportScale[1];
		if (bForceNonVR && g_bEnableVR) {
			temp.x /=  2.0f;
			temp.y /= -2.0f;
		}
		// temp.xy is now (sx,sy): in-game screen coords (CONFIRMED)
		// sx = temp.x; sy = temp.y;
		P.x = temp.x;
		P.y = temp.y;
		P.z = temp.z;
	}
	return P;
}

// Back-project a 2D vertex (specified in in-game coords)
// into 3D, just like we do in the VertexShader or SBS VertexShader:
inline void backProject(float sx, float sy, float rhw, Vector3 *P) {
	// The code to back-project is slightly different in DirectSBS/SteamVR
	// This code comes from VertexShader.hlsl/SBSVertexShader.hlsl
	float3 temp;
	// float3 temp = input.pos.xyz;
	temp.x = sx;
	temp.y = sy;

	// Normalize into the 0..2 or 0.0..1.0 range
	//temp.xy *= vpScale.xy;
	temp.x *= g_VSCBuffer.viewportScale[0];
	temp.y *= g_VSCBuffer.viewportScale[1];
	// Non-VR case:
	//		temp.x is now normalized to the range (0,  2)
	//		temp.y is now normalized to the range (0, -2) (viewPortScale[1] is negative for nonVR)
	// Direct-SBS:
	//		temp.xy is now normalized to the range [0..1] (notice how the Y-axis is swapped w.r.t to the Non-VR case

	if (g_bEnableVR)
	{
		// SBSVertexShader
		// temp.xy -= 0.5;
		temp.x += -0.5f;
		temp.y += -0.5f;
		// temp.xy is now in the range -0.5 ..  0.5

		// temp.xy *= vpScale.w * vpScale.z * float2(aspect_ratio, 1);
		temp.x *= g_VSCBuffer.viewportScale[3] * g_VSCBuffer.viewportScale[2] * g_VSCBuffer.aspect_ratio;
		temp.y *= g_VSCBuffer.viewportScale[3] * g_VSCBuffer.viewportScale[2];

		// TODO: The code below hasn't been tested in VR:
		temp.z = (float)METRIC_SCALE_FACTOR * /*g_fMetricMult*/ (1.0f / rhw);
	}
	else
	{
		// Code from VertexShader
		temp.x += -1.0f; // For nonVR, vpScale is mult by 2, so we need to add/substract with 1.0, not 0.5 to center the coords
		temp.y +=  1.0f;
		// temp.x is now in the range -1.0 ..  1.0 and
		// temp.y is now in the range  1.0 .. -1.0

		// The center of the screen may not be the center of projection. The y-axis seems to be a bit off.
		// temp.xy *= vpScale.z * float2(aspect_ratio, 1);
		temp.x *= g_VSCBuffer.viewportScale[2] * g_VSCBuffer.aspect_ratio;
		temp.y *= g_VSCBuffer.viewportScale[2];
		// temp.x is now in the range -0.5 ..  0.5 and (?)
		// temp.y is now in the range  0.5 .. -0.5 (?)

		// temp.z = METRIC_SCALE_FACTOR * w;
		temp.z = (float)METRIC_SCALE_FACTOR * (1.0f / rhw);
	}

	// The back-projection into 3D is now very simple:
	P->x = temp.z * temp.x / g_fFocalDist;
	P->y = temp.z * temp.y / g_fFocalDist;
	P->z = temp.z;
	if (g_bEnableVR)
	{
		// Further adjustment of the coordinates for the DirectSBS case:
		//output.pos3D = float4(P.x, -P.y, P.z, 1);
		P->y = -P->y;
		// Adjust the coordinate system for SteamVR:
		//P.yz = -P.yz;
	}
}

// Back-project a 2D vertex (specified in in-game coords) stored in g_OrigVerts 
// into 3D, just like we do in the VertexShader or SBS VertexShader:
inline void backProject(UINT index, Vector3 *P) {
	backProject(g_OrigVerts[index].sx, g_OrigVerts[index].sy, g_OrigVerts[index].rhw, P);
}

// The return values sx,sy are the screen coords (in in-game coords). Use them for debug purposes only
Vector3 project(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix /*, float *sx, float *sy */)
{
	Vector3 P = pos3D;
	//float w;
	// Whatever is placed in P is returned at the end of this function
	
	if (g_bEnableVR) {
		float w;
		// We need to invert the sign of the z coord because the matrices are defined in the SteamVR
		// coord system
		Vector4 Q = Vector4(g_fFocalDist * P.x, g_fFocalDist * -P.y, -P.z, 1.0f);
		Q = projEyeMatrix * viewMatrix * Q;

		// output.pos = mul(projEyeMatrix, output.pos);
		P.x = Q.x;
		P.y = Q.y;
		P.z = Q.z;
		w   = Q.w;
		// Multiply by focal_dist? The focal dist should be in the projection matrix...

		// DirectX divides by w internally after the PixelShader output is written. We don't
		// see that division in the shader; but we have to do it explicitly here because
		// that's what actually accomplishes the 3D -> 2D projection (it's like a weighed 
		// division by Z)
		P.x /= w;
		P.y /= w;
		
		// P is now in the internal DirectX coord sys: xy in (-1..1)
		// So let's transform to the range 0..1 for post-proc coords:
		P.x = P.x * 0.5f + 0.5f;
		P.y = P.y * 0.5f + 0.5f;
	} 
	else {
		// Non-VR processing from this point on:
		// (x0,y0)-(x1,y1) are the viewport limits
		float x0 = g_LaserPointerBuffer.x0;
		float y0 = g_LaserPointerBuffer.y0;
		float x1 = g_LaserPointerBuffer.x1;
		float y1 = g_LaserPointerBuffer.y1;
		//w = P.z / ((float)METRIC_SCALE_FACTOR * g_fMetricMult);
		
		// According to the code in back_project, P.z = METRIC_SCALE_FACTOR * w;
		// P.xy = P.xy / P.z;
		P.x *= g_fFocalDist / P.z;
		P.y *= g_fFocalDist / P.z;

		// Convert to vertex pos:
		// P.xy /= (vpScale.z * float2(aspect_ratio, 1));
		P.x /= (g_VSCBuffer.viewportScale[2] * g_VSCBuffer.aspect_ratio);
		P.y /= (g_VSCBuffer.viewportScale[2]);
		P.x -= -1.0f;
		P.y -=  1.0f;
		// P.xy /= vpScale.xy;
		P.x /= g_VSCBuffer.viewportScale[0];
		P.y /= g_VSCBuffer.viewportScale[1];
		//P.z = 1.0f / w; // Not necessary, this computes the original rhw

		// *******************************************************
		// P is now in in-game coordinates (CONFIRMED!), where:
		// (0,0) is the upper-left *original* viewport corner, and:
		// (g_fCurInGameWidth, g_fCurInGameHeight) is the lower-right *original* viewport corner
		// Note that the *original* viewport does not necessarily cover the whole screen
		//return P; // Return in-game coords
		//if (sx != NULL) *sx = P.x;
		//if (sy != NULL) *sy = P.y;

		// At this point, P.x * viewPortScale[0] converts P.x to the range 0..2, in the original viewport (?)
		// P.y * viewPortScale[1] converts P.y to the range 0..-2 in the original viewport (?)

		// The following lines are simply implementing the following formulas used in the VertexShader:
		//output.pos.x = (input.pos.x * vpScale.x - 1.0f) * vpScale.z;
		//output.pos.y = (input.pos.y * vpScale.y + 1.0f) * vpScale.z;
		P.x = (P.x * g_VSCBuffer.viewportScale[0] - 1.0f) * g_VSCBuffer.viewportScale[2];
		P.y = (P.y * g_VSCBuffer.viewportScale[1] + 1.0f) * g_VSCBuffer.viewportScale[2];
		// P.x is now in -1 ..  1
		// P.y is now in  1 .. -1

		// Now convert to UV coords: (0, 1)-(1, 0):
		// By using x0,y0-x1,y1 as limits, we're now converting to post-proc viewport UVs
		P.x = lerp(x0, x1, (P.x + 1.0f) / 2.0f);
		P.y = lerp(y1, y0, (P.y + 1.0f) / 2.0f);
	}
	return P;
}

Vector3 projectToInGameCoords(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix)
{
	Vector3 P = pos3D;
	//float w;
	// Whatever is placed in P is returned at the end of this function

	if (g_bEnableVR) {
		float w;
		// We need to invert the sign of the z coord because the matrices are defined in the SteamVR
		// coord system
		//Vector4 Q = Vector4(g_fFocalDist * P.x, g_fFocalDist * -P.y, -P.z, 1.0f);
		Vector4 Q = Vector4(P.x, P.y, P.z, 1.0f); // MetricVR (?)
		Q = projEyeMatrix * viewMatrix * Q;

		// output.pos = mul(projEyeMatrix, output.pos);
		P.x = Q.x;
		P.y = Q.y;
		P.z = Q.z;
		w = Q.w;
		// Multiply by focal_dist? The focal dist should be in the projection matrix...

		// DirectX divides by w internally after the PixelShader output is written. We don't
		// see that division in the shader; but we have to do it explicitly here because
		// that's what actually accomplishes the 3D -> 2D projection (it's like a weighed 
		// division by Z)
		P.x /= w;
		P.y /= w;

		// P is now in the internal DirectX coord sys: xy in (-1..1)
		// So let's transform to the range 0..1 for post-proc coords:
		P.x = P.x * 0.5f + 0.5f;
		P.y = P.y * 0.5f + 0.5f;
	}
	else {
		// (x0,y0)-(x1,y1) are the viewport limits
		float x0 = g_LaserPointerBuffer.x0;
		float y0 = g_LaserPointerBuffer.y0;
		float x1 = g_LaserPointerBuffer.x1;
		float y1 = g_LaserPointerBuffer.y1;
		//w = P.z / ((float)METRIC_SCALE_FACTOR * g_fMetricMult);

		// Non-VR processing from this point on:
		// P.xy = P.xy / P.z;
		P.x *= g_fFocalDist / P.z;
		P.y *= g_fFocalDist / P.z;

		// Convert to vertex pos:
		// P.xy /= (vpScale.z * float2(aspect_ratio, 1));
		P.x /= (g_VSCBuffer.viewportScale[2] * g_VSCBuffer.aspect_ratio);
		P.y /= (g_VSCBuffer.viewportScale[2]);
		P.x -= -1.0f;
		P.y -= 1.0f;
		// P.xy /= vpScale.xy;
		P.x /= g_VSCBuffer.viewportScale[0];
		P.y /= g_VSCBuffer.viewportScale[1];
		//P.z = 1.0f / w; // Not necessary, this computes the original rhw

		// *******************************************************
		// P is now in in-game coordinates (CONFIRMED!), where:
		// (0,0) is the upper-left *original* viewport corner, and:
		// (g_fCurInGameWidth, g_fCurInGameHeight) is the lower-right *original* viewport corner
		// Note that the *original* viewport does not necessarily cover the whole screen
		//return P; // Return in-game coords
	}
	return P;
}

/*
 * Dumps the vertices in the current instruction to the given file after back-projecting them
 * into 3D space.
 * After dumping solid polygons, I realized the back-project code skews the Y axis... considerably.
 */
void DumpVerticesToOBJ(FILE *file, LPD3DINSTRUCTION instruction, UINT curIndex)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector3 tempv0, tempv1, tempv2;
	std::vector<int> indices;

	if (file == NULL) {
		log_debug("[DBG] Cannot dump vertices, NULL file ptr");
		return;
	}

	// Start a new object
	fprintf(file, "o obj_%d\n", g_iDumpOBJIdx);
	indices.clear();
	for (WORD i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		backProjectMetric(index, &tempv0);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv0.x, tempv0.y, tempv0.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		backProjectMetric(index, &tempv1);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv1.x, tempv1.y, tempv1.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		backProjectMetric(index, &tempv2);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv2.x, tempv2.y, tempv2.z);

		if (!g_config.D3dHookExists) triangle++;

		indices.push_back(g_iDumpOBJFaceIdx++);
		indices.push_back(g_iDumpOBJFaceIdx++);
		indices.push_back(g_iDumpOBJFaceIdx++);
	}
	fprintf(file, "\n");

	// Write the face data
	for (uint32_t i = 0; i < indices.size(); i += 3)
	{
		fprintf(file, "f %d %d %d\n", indices[i], indices[i + 1], indices[i + 2]);
	}
	fprintf(file, "\n");

	g_iDumpOBJIdx++;
}

// From: https://blackpawn.com/texts/pointinpoly/default.html
bool IsInsideTriangle(Vector2 P, Vector2 A, Vector2 B, Vector2 C, float *u, float *v) {
	// Compute vectors        
	Vector2 v0 = C - A;
	Vector2 v1 = B - A;
	Vector2 v2 = P - A;

	// Compute dot products
	float dot00 = v0.dot(v0);
	float dot01 = v0.dot(v1);
	float dot02 = v0.dot(v2);
	float dot11 = v1.dot(v1);
	float dot12 = v1.dot(v2);

	// Compute barycentric coordinates
	float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	*u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	*v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Points in this scheme are described as:
	// P = A + u * (C - A) + v * (B - A)

	// Check if point is in triangle
	//return (u >= 0.0f) && (v >= 0.0f) && (u + v < 1.0f);
	return (*u >= -0.000001f) && (*v >= -0.000001f) && (*u + *v < 1.000001f);
}

/*
 Computes the centroid of the given texture, returns a metric 3D point in space,
 and 2D centroid in in-game coordinates.
 We're using this to find the center of the Suns to add lens flare, etc.

 Returns true if the centroid could be computed (i.e. if the centroid is visible)
 
 ------------------------------------------------------------------------------------
 Perspective-correct interpolation, from: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/perspective-correct-interpolation-vertex-attributes

 A = Z * (A0/Z0 * (1 - q) + A1/Z1 * q)

 Where:

	 A0 is the attribute at vertex 0
	 A1 is the attribute at vertex 1
	 Z0 is the Z value at vertex 0
	 Z1 is the Z value at vertex 1
	 Z  is the Z value at the point we're interpolating
	 q  is the cursor that goes from 0 to 1
	 A  is the attribute at the current point we're interpolating
 */
bool Direct3DDevice::ComputeCentroid(LPD3DINSTRUCTION instruction, UINT curIndex, Vector3 *Centroid, Vector2 *Centroid2D)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector2 P, UV0, UV1, UV2, UV = Vector2(0.5f, 0.5f);

	Vector3 tempv0, tempv1, tempv2, tempP;
	Vector2 v0, v1, v2;

	for (WORD i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		UV0.x = g_OrigVerts[index].tu; UV0.y = g_OrigVerts[index].tv;
		v0.x = g_OrigVerts[index].sx; v0.y = g_OrigVerts[index].sy;
		backProjectMetric(index, &tempv0);
		if (g_bEnableVR) tempv0.y = -tempv0.y;
		//log_debug("[DBG] tempv0: %0.3f, %0.3f, %0.3f", tempv0.x, tempv0.y, tempv0.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		UV1.x = g_OrigVerts[index].tu; UV1.y = g_OrigVerts[index].tv;
		v1.x = g_OrigVerts[index].sx; v1.y = g_OrigVerts[index].sy;
		backProjectMetric(index, &tempv1);
		if (g_bEnableVR) tempv1.y = -tempv1.y;
		//log_debug("[DBG] tempv0: %0.3f, %0.3f, %0.3f", tempv1.x, tempv1.y, tempv1.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		UV2.x = g_OrigVerts[index].tu; UV2.y = g_OrigVerts[index].tv;
		v2.x = g_OrigVerts[index].sx; v2.y = g_OrigVerts[index].sy;
		backProjectMetric(index, &tempv2);
		if (g_bEnableVR) tempv2.y = -tempv2.y;
		//log_debug("[DBG] tempv0: %0.3f, %0.3f, %0.3f", tempv2.x, tempv2.y, tempv2.z);

		float u, v;
		if (IsInsideTriangle(UV, UV0, UV1, UV2, &u, &v)) {
			// Confirm:
			//Vector2 Q = UV0 + u * (UV2 - UV0) + v * (UV1 - UV0);
			//log_debug("[DBG] Q: %0.3f, %0.3f", Q.x, Q.y); // This should *always* display 0.5, 0.5
			
			// Compute the 3D vertex where the UV coords are 0.5, 0.5. By using the back-projected
			// 3D vertices, we automatically get perspective-correct results when projecting back to 2D:
			*Centroid = tempv0 + u * (tempv2 - tempv0) + v * (tempv1 - tempv0);
			*Centroid2D = v0 + u * (v2 - v0) + v * (v1 - v0);

			/*
			q = projectToInGameCoords(tempP, g_viewMatrix, g_fullMatrixLeft);
			
			float X, Y;
			InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
				(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, q.x, q.y, &X, &Y);
			Centroid->x = X;
			Centroid->y = Y;
			*/
			return true;
		}
		
		if (!g_config.D3dHookExists) triangle++;
	}

	return false;
}

/*
 Computes the 2D centroid of the given instruction as specified by its UV coordinates.
 i.e. if UV = (0.5, 0.5) is visible on the screen, this function will return true and
 a 2D point in in-game coordinates that corresponds to that UV.
*/
bool Direct3DDevice::ComputeCentroid2D(LPD3DINSTRUCTION instruction, UINT curIndex, Vector2 *Centroid)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector2 P, UV0, UV1, UV2, UV = Vector2(0.5f, 0.5f);

	Vector2 tempv0, tempv1, tempv2;

	for (WORD i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		UV0.x = g_OrigVerts[index].tu; UV0.y = g_OrigVerts[index].tv;
		tempv0.x = g_OrigVerts[index].sx; tempv0.y = g_OrigVerts[index].sy;

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		UV1.x = g_OrigVerts[index].tu; UV1.y = g_OrigVerts[index].tv;
		tempv1.x = g_OrigVerts[index].sx; tempv1.y = g_OrigVerts[index].sy;

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		UV2.x = g_OrigVerts[index].tu; UV2.y = g_OrigVerts[index].tv;
		tempv2.x = g_OrigVerts[index].sx; tempv2.y = g_OrigVerts[index].sy;

		float u, v;
		if (IsInsideTriangle(UV, UV0, UV1, UV2, &u, &v)) {
			// Confirm:
			//Vector2 Q = UV0 + u * (UV2 - UV0) + v * (UV1 - UV0);
			//log_debug("[DBG] Q: %0.3f, %0.3f", Q.x, Q.y); // This should *always* display 0.5, 0.5

			// Compute the 2D vertex where the UV coords are 0.5, 0.5.
			*Centroid = tempv0 + u * (tempv2 - tempv0) + v * (tempv1 - tempv0);
			return true;
		}

		if (!g_config.D3dHookExists) triangle++;
	}
	return false;
}

// DEBUG
//FILE *colorFile = NULL, *lightFile = NULL;
// DEBUG

bool Direct3DDevice::IntersectWithTriangles(LPD3DINSTRUCTION instruction, UINT curIndex, int textureIdx, bool isACTex,
	Vector3 orig, Vector3 dir, bool debug)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	D3DTLVERTEX vert;
	uint32_t index;
	UINT idx = curIndex;
	float u, v, U0, V0, U1, V1, U2, V2; // , dx, dy;
	float best_t = 10000.0f;
	bool bIntersection = false;

	Vector3 tempv0, tempv1, tempv2, tempP;
	float tempt, tu, tv;

	/*
	FILE *outFile = NULL;
	if (g_bDumpSSAOBuffers) {
		if (colorFile == NULL)
			fopen_s(&colorFile, "./colorVertices.obj", "wt");
		if (lightFile == NULL)
			fopen_s(&lightFile, "./lightVertices.obj", "wt");
		outFile = texture->is_LightTexture ? lightFile : colorFile;
	}
	*/

	if (debug)
		log_debug("[DBG] START Geom");

	for (WORD i = 0; i < instruction->wCount; i++)
	{
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		//px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		U0 = g_OrigVerts[index].tu; V0 = g_OrigVerts[index].tv;
		backProjectMetric(index, &tempv0);
		/* if (g_bDumpSSAOBuffers) {
			//fprintf(outFile, "v %0.6f %0.6f %0.6f\n", tempv0.x, tempv0.y, tempv0.z);
			Vector3 q = project(tempv0, g_viewMatrix, g_fullMatrixLeft);
			fprintf(outFile, "v %0.6f %0.6f %0.6f\n", q.x, q.y, q.z);
		} */
		if (debug) {
			vert = g_OrigVerts[index];
			Vector3 q = projectMetric(tempv0, g_viewMatrix, g_FullProjMatrixLeft /*, &dx, &dy */);
			log_debug("[DBG] 2D: (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f, %0.3f)",//=(%0.3f, %0.3f)",
				vert.sx, vert.sy, 1.0f/vert.rhw, 
				tempv0.x, tempv0.y, tempv0.z,
				q.x, q.y, 1.0f/q.z /*, dx, dy */);
		}

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		//px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		U1 = g_OrigVerts[index].tu; V1 = g_OrigVerts[index].tv;
		backProjectMetric(index, &tempv1);
		/* if (g_bDumpSSAOBuffers) {
			//fprintf(outFile, "v %0.6f %0.6f %0.6f\n", tempv1.x, tempv1.y, tempv1.z);
			Vector3 q = project(tempv1, g_viewMatrix, g_fullMatrixLeft);
			fprintf(outFile, "v %0.6f %0.6f %0.6f\n", q.x, q.y, q.z);
		} */
		if (debug) {
			vert = g_OrigVerts[index];
			Vector3 q = projectMetric(tempv1, g_viewMatrix, g_FullProjMatrixLeft /*, &dx, &dy */);
			log_debug("[DBG] 2D: (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f, %0.3f)", //=(%0.3f, %0.3f)",
				vert.sx, vert.sy, 1.0f/vert.rhw, 
				tempv1.x, tempv1.y, tempv1.z,
				q.x, q.y, 1.0f/q.z /*, dx, dy */);
		}

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		//px = g_OrigVerts[index].sx; py = g_OrigVerts[index].sy;
		U2 = g_OrigVerts[index].tu; V2 = g_OrigVerts[index].tv;
		backProjectMetric(index, &tempv2);
		/* if (g_bDumpSSAOBuffers) {
			//fprintf(outFile, "v %0.6f %0.6f %0.6f\n", tempv2.x, tempv2.y, tempv2.z);
			Vector3 q = project(tempv2, g_viewMatrix, g_fullMatrixLeft);
			fprintf(outFile, "v %0.6f %0.6f %0.6f\n", q.x, q.y, q.z);
		} */
		if (debug) {
			vert = g_OrigVerts[index];
			Vector3 q = projectMetric(tempv2, g_viewMatrix, g_FullProjMatrixLeft /*, &dx, &dy */);
			log_debug("[DBG] 2D: (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f, %0.3f)", //=(%0.3f, %0.3f)",
				vert.sx, vert.sy, 1.0f/vert.rhw,
				tempv2.x, tempv2.y, tempv2.z,
				q.x, q.y, 1.0f/q.z /*, dx, dy */);
		}

		// Check the intersection with this triangle
		// (tu, tv) are barycentric coordinates in the tempv0,v1,v2 triangle
		if (rayTriangleIntersect(orig, dir, tempv0, tempv1, tempv2, tempt, tempP, tu, tv))
		{
			//if (isACTex) tempt -= 0.01f; // Make AC elements a little more likely to be considered before other textures
			//if (g_bDumpLaserPointerDebugInfo)
			//	log_debug("[DBG] [AC] %s intersected, idx: %d, t: %0.6f", texName, textureIdx, tempt);
			if (tempt < g_fBestIntersectionDistance)
			{
				//if (g_bDumpLaserPointerDebugInfo)
				//	log_debug("[DBG] [AC] %s is best intersection with idx: %d, t: %0.6f", texName, textureIdx, tempt);
				// Update the best intersection so far
				g_fBestIntersectionDistance = tempt;
				g_LaserPointer3DIntersection = tempP;
				g_iBestIntersTexIdx = textureIdx;
				g_debug_v0 = tempv0;
				g_debug_v1 = tempv1;
				g_debug_v2 = tempv2;

				//float v0 = tempv0; *v1 = tempv1; *v2 = tempv2;
				//*P = tempP;

				// Interpolate the texture UV using the barycentric (tu, tv) coords:
				u = tu * U0 + tv * U1 + (1.0f - tu - tv) * U2;
				v = tu * V0 + tv * V1 + (1.0f - tu - tv) * V2;

				g_LaserPointerBuffer.uv[0] = u;
				g_LaserPointerBuffer.uv[1] = v;

				bIntersection = true;
				g_LaserPointerBuffer.bIntersection = 1;
			}
		}
		/*else {
			if (g_bDumpLaserPointerDebugInfo && strstr(texName, "AwingCockpit.opt,TEX00080,color") != NULL)
				log_debug("[DBG] [AC] %s considered; but no intersection found!", texName);
		}*/
		if (!g_config.D3dHookExists) triangle++;
	}

	if (debug)
		log_debug("[DBG] END Geom");
	return bIntersection;
}

void Direct3DDevice::AddLaserLightsOld(LPD3DINSTRUCTION instruction, UINT curIndex, Direct3DTexture *texture)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector3 pos3D;
	float u, v;

	// XWA batch renders all lasers that share the same texture, so we may see several
	// lasers when parsing this instruction. So, to detect each individual laser, we need
	// to look at the uv's: if the current uv is close to (1,1), then we know that's the
	// tip of one individual laser and we add it to the current list.
	for (WORD i = 0; i < instruction->wCount; i++)
	{
		
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		u = g_OrigVerts[index].tu;
		v = g_OrigVerts[index].tv;
		if (u > 0.9f && v > 0.9f)
		{
			backProject(index, &pos3D);
			g_LaserList.insert(pos3D, texture->material.Light);
		}

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		u = g_OrigVerts[index].tu;
		v = g_OrigVerts[index].tv;
		if (u > 0.9f && v > 0.9f)
		{
			backProject(index, &pos3D);
			g_LaserList.insert(pos3D, texture->material.Light);
		}

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		u = g_OrigVerts[index].tu;
		v = g_OrigVerts[index].tv;
		if (u > 0.9f && v > 0.9f)
		{
			backProject(index, &pos3D);
			g_LaserList.insert(pos3D, texture->material.Light);
		}

		if (!g_config.D3dHookExists) triangle++;
	}
}

void Direct3DDevice::AddLaserLights(LPD3DINSTRUCTION instruction, UINT curIndex, Direct3DTexture *texture)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector3 tempv0, tempv1, tempv2, P;
	//Vector2 v0, v1, v2;
	Vector2 UV0, UV1, UV2, UV = texture->material.LightUVCoordPos;

	// XWA batch renders all lasers that share the same texture, so we may see several
	// lasers when parsing this instruction. To detect each individual laser, we need
	// to look at the uv's and see if the current triangle contains the uv coord we're
	// looking for (the default is (0.1, 0.9)). If the uv is contained, then we compute
	// the 3D point using its barycentric coords and add it to the current list.
	for (WORD i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		UV0.x = g_OrigVerts[index].tu; UV0.y = g_OrigVerts[index].tv;
		//v0.x = g_OrigVerts[index].sx; v0.y = g_OrigVerts[index].sy;
		backProjectMetric(index, &tempv0);
		if (g_bEnableVR) tempv0.y = -tempv0.y;
		//log_debug("[DBG] tempv0: %0.3f, %0.3f, %0.3f", tempv0.x, tempv0.y, tempv0.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		UV1.x = g_OrigVerts[index].tu; UV1.y = g_OrigVerts[index].tv;
		//v1.x = g_OrigVerts[index].sx; v1.y = g_OrigVerts[index].sy;
		backProjectMetric(index, &tempv1);
		if (g_bEnableVR) tempv1.y = -tempv1.y;
		//log_debug("[DBG] tempv0: %0.3f, %0.3f, %0.3f", tempv1.x, tempv1.y, tempv1.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		UV2.x = g_OrigVerts[index].tu; UV2.y = g_OrigVerts[index].tv;
		//v2.x = g_OrigVerts[index].sx; v2.y = g_OrigVerts[index].sy;
		backProjectMetric(index, &tempv2);
		if (g_bEnableVR) tempv2.y = -tempv2.y;
		//log_debug("[DBG] tempv0: %0.3f, %0.3f, %0.3f", tempv2.x, tempv2.y, tempv2.z);

		float u, v;
		if (IsInsideTriangle(UV, UV0, UV1, UV2, &u, &v)) {
			P = tempv0 + u * (tempv2 - tempv0) + v * (tempv1 - tempv0);
			g_LaserList.insert(P, texture->material.Light);
		}

		if (!g_config.D3dHookExists) triangle++;
	}
}

/*
 * Compute the effective screen limits in uv coords when rendering effects using a
 * full-screen quad. This is used in SSDO to get the effective viewport limits in
 * uv-coords. Pixels outside the uv-coords computed here should be black.
 */
void GetScreenLimitsInUVCoords(float *x0, float *y0, float *x1, float *y1, bool UseNonVR) 
{
	if (!UseNonVR && g_bEnableVR) {
		// In VR mode we can't see the edges of the screen anyway, so don't bother
		// computing the effective viewport for the left and right eyes... or the
		// viewport for the SteamVR mode.
		*x0 = 0.0f;
		*y0 = 0.0f;
		*x1 = 1.0f;
		*y1 = 1.0f;
	}
	else {
		UINT left   = (UINT)g_nonVRViewport.TopLeftX;
		UINT top    = (UINT)g_nonVRViewport.TopLeftY;
		UINT width  = (UINT)g_nonVRViewport.Width;
		UINT height = (UINT)g_nonVRViewport.Height;
		float x, y;
		InGameToScreenCoords(left, top, width, height, 0, 0, &x, &y);
		*x0 = x / g_fCurScreenWidth;
		*y0 = y / g_fCurScreenHeight;
		InGameToScreenCoords(left, top, width, height, g_fCurInGameWidth, g_fCurInGameHeight, &x, &y);
		*x1 = x / g_fCurScreenWidth;
		*y1 = y / g_fCurScreenHeight;
		//if (g_bDumpSSAOBuffers)
		//log_debug("[DBG] [DBG] (x0,y0)-(x1,y1): (%0.3f, %0.3f)-(%0.3f, %0.3f)",
		//	*x0, *y0, *x1, *y1);
	}
}

/*
 * Converts normalized uv_coords with respect to a HUD texture into actual pixel
 * coordinates.
 */
Box ComputeCoordsFromUV(UINT left, UINT top, UINT width, UINT height,
	Box uv_minmax, Box HUD_coords, const Box uv_coords) {
	Box result = { 0 };
	// These are coordinates in pixels in texture space
	float u0 = uv_coords.x0;
	float v0 = uv_coords.y0;
	float u1 = uv_coords.x1;
	float v1 = uv_coords.y1;
	// We expect normalized uv coords here
	// Now we need to interpolate, if u == minU --> minX, if u == maxU --> maxX
	result.x0 = lerp(HUD_coords.x0, HUD_coords.x1, (u0 - uv_minmax.x0) / (uv_minmax.x1 - uv_minmax.x0));
	result.y0 = lerp(HUD_coords.y0, HUD_coords.y1, (v0 - uv_minmax.y0) / (uv_minmax.y1 - uv_minmax.y0));
	result.x1 = lerp(HUD_coords.x0, HUD_coords.x1, (u1 - uv_minmax.x0) / (uv_minmax.x1 - uv_minmax.x0));
	result.y1 = lerp(HUD_coords.y0, HUD_coords.y1, (v1 - uv_minmax.y0) / (uv_minmax.y1 - uv_minmax.y0));
	// Normalize to uv coords wrt screen space:
	result.x0 /= g_fCurScreenWidth; result.y0 /= g_fCurScreenHeight;
	result.x1 /= g_fCurScreenWidth; result.y1 /= g_fCurScreenHeight;
	return result;
}

void ComputeCoordsFromUV(UINT left, UINT top, UINT width, UINT height,
	Box uv_minmax, Box HUD_coords, const Box uv_coords, uvfloat4 *result) {
	Box box = ComputeCoordsFromUV(left, top, width, height,
		uv_minmax, HUD_coords, uv_coords);
	result->x0 = box.x0; result->y0 = box.y0;
	result->x1 = box.x1; result->y1 = box.y1;
}

void DisplayBox(char *name, Box box) {
	log_debug("[DBG] %s (%0.3f, %0.3f)-(%0.3f, %0.3f)", name,
		box.x0, box.y0, box.x1, box.y1);
}

/*
 If the game is rendering the hyperspace effect, this function will select shaderToyBuf
 when rendering the cockpit. Otherwise it will select the regular offscreenBuffer
 */
inline ID3D11RenderTargetView *Direct3DDevice::SelectOffscreenBuffer(bool bIsMaskable, bool bSteamVRRightEye = false) {
	auto& resources = this->_deviceResources;
	//auto& context = resources->_d3dDeviceContext;

	/*
	// DEBUG
	static bool bFirstTime = true;
	if (bFirstTime) {
		DirectX::SaveDDSTextureToFile(context, resources->_offscreenBufferPost,
			L"C:\\Temp\\_offscreenBufferPost-First-Time.dds");
		bFirstTime = false;
	}
	// DEBUG
	*/

	ID3D11RenderTargetView *regularRTV = bSteamVRRightEye ? resources->_renderTargetViewR.Get() : resources->_renderTargetView.Get();
	ID3D11RenderTargetView *shadertoyRTV = bSteamVRRightEye ? resources->_shadertoyRTV_R.Get() : resources->_shadertoyRTV.Get();
	//if (g_HyperspacePhaseFSM != HS_POST_HYPER_EXIT_ST || !bIsCockpit)
	if (g_HyperspacePhaseFSM != HS_INIT_ST && bIsMaskable)
		// If we reach this point, then the game is in hyperspace AND this is a cockpit texture
		return shadertoyRTV;
	else
		// Normal output buffer (_offscreenBuffer)
		return regularRTV;
}

inline void Direct3DDevice::EnableTransparency() {
	auto& resources = this->_deviceResources;
	D3D11_BLEND_DESC blendDesc{};

	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	resources->InitBlendState(nullptr, &blendDesc);
}

inline void Direct3DDevice::EnableHoloTransparency() {
	auto& resources = this->_deviceResources;
	D3D11_BLEND_DESC blendDesc{};

	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	resources->InitBlendState(nullptr, &blendDesc);
}

/*
 * Saves the current blend state to m_SavedBlendDesc
 */
inline void Direct3DDevice::SaveBlendState() {
	m_SavedBlendDesc = this->_renderStates->GetBlendDesc();
}

/*
 * Restore a previously-saved blend state in m_SavedBlendDesc
 */
inline void Direct3DDevice::RestoreBlendState() {
	auto& resources = this->_deviceResources;
	resources->InitBlendState(nullptr, &m_SavedBlendDesc);
}

inline void Direct3DDevice::RestoreSamplerState() {
	auto& resources = this->_deviceResources;
	auto& context = resources->_d3dDeviceContext;
	context->PSSetSamplers(1, 1, resources->_mainSamplerState.GetAddressOf());
}

inline void UpdateDCHologramState() {
	if (!g_bDCHologramsVisiblePrev && g_bDCHologramsVisible) {
		g_fDCHologramFadeIn = 0.0f;
		g_fDCHologramFadeInIncr = 0.04f;
	} else if (g_bDCHologramsVisiblePrev && !g_bDCHologramsVisible) {
		g_fDCHologramFadeIn = 1.0f;
		g_fDCHologramFadeInIncr = -0.04f;
	}
	g_fDCHologramTime += 0.0166f;
	g_fDCHologramFadeIn += g_fDCHologramFadeInIncr;
	if (g_fDCHologramFadeIn > 1.0f) g_fDCHologramFadeIn = 1.0f;
	if (g_fDCHologramFadeIn < 0.0f) g_fDCHologramFadeIn = 0.0f;
	g_bDCHologramsVisiblePrev = g_bDCHologramsVisible;
}

HRESULT Direct3DDevice::Execute(
	LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
	LPDIRECT3DVIEWPORT lpDirect3DViewport,
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif
	/*
	// DEBUG: This will print the message that the CockpitLook hook has set.
	if (g_pSharedData != NULL && g_pSharedData->bDataReady) {
		// Here we're using g_pSharedData->pDataPtr to access the shared data, but
		// we can also save pDataPtr to a global variable and use that instead.
		log_debug("[DBG] msg: %s", (char *)g_pSharedData->pDataPtr);
	}
	*/

/*
	if (g_bUseSteamVR && g_ExecuteCount == 0) {//only wait once per frame
		// Synchronization point to wait for vsync before we start to send work to the GPU
		// This avoids blocking the CPU while the compositor waits for the pixel shader effects to run in the GPU
		// (that's what happens if we sync after Submit+Present)
		//vr::EVRCompositorError error = g_pVRCompositor->WaitGetPoses(&g_rTrackedDevicePose, 0, NULL, 0);
		CalculateViewMatrix();
		g_bRunningStartAppliedOnThisFrame = true;
	}
*/
	g_ExecuteCount++;

	//log_debug("[DBG] Execute (1)");

	if (lpDirect3DExecuteBuffer == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	Direct3DExecuteBuffer* executeBuffer = (Direct3DExecuteBuffer*)lpDirect3DExecuteBuffer;

#if LOGGER
	DumpExecuteBuffer(executeBuffer);
#endif

	// DEBUG
	//g_HyperspacePhaseFSM = HS_POST_HYPER_EXIT_ST;
//#ifdef HYPER_OVERRIDE
	if (g_bHyperDebugMode)
		g_HyperspacePhaseFSM = (HyperspacePhaseEnum )g_iHyperStateOverride;
//#endif
	//g_iHyperExitPostFrames = 0;
	// DEBUG

	// Applying the custom FOV doesn't work here, it has to be done after the first frame is rendered

	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;

	if (g_bDumpSSAOBuffers)
	{
		DirectX::SaveWICTextureToFile(context, resources->_offscreenAsInputDynCockpit, GUID_ContainerFormatJpeg, L"c:\\temp\\_DC-FG-Input.jpg");
		DirectX::SaveWICTextureToFile(context, resources->_offscreenAsInputDynCockpitBG, GUID_ContainerFormatJpeg, L"c:\\temp\\_DC-BG-Input.jpg");
		DirectX::SaveWICTextureToFile(context, resources->_DCTextAsInput, GUID_ContainerFormatJpeg, L"c:\\temp\\_DC-Text-Input.jpg");
		if (g_bDumpOBJEnabled) {
			if (g_DumpOBJFile != NULL)
				fclose(g_DumpOBJFile);
			fopen_s(&g_DumpOBJFile, "./DumpVertices.OBJ", "wt");
			g_iDumpOBJIdx = 1;
			g_iDumpOBJFaceIdx = 1;
			log_debug("[DBG] [SHW] sm_FOVscale: %0.3f", g_ShadowMapVSCBuffer.sm_FOVscale);
			log_debug("[DBG] [SHW] sm_y_center: %0.3f", g_ShadowMapVSCBuffer.sm_y_center);
			log_debug("[DBG] [SHW] g_fOBJMetricMult: %0.3f", g_fOBJ_Z_MetricMult);
		}
		// The following texture has the sub-component bracket in in-game coordinates:
		DirectX::SaveWICTextureToFile(context, resources->_mainDisplayTexture, GUID_ContainerFormatPng,
			L"C:\\Temp\\_DCTexViewSubCMD.png");
	}

	// Apply the Edge Detector effect to the DC foreground texture
	if (g_bEdgeDetectorEnabled && g_bDynCockpitEnabled && g_bRendering3D && !g_bEdgeEffectApplied)
		RenderEdgeDetector();

	HRESULT hr = S_OK;
	UINT width, height, left, top;
	float scale;
	UINT vertexBufferStride = sizeof(D3DTLVERTEX), vertexBufferOffset = 0;
	D3D11_VIEWPORT viewport;
	bool bModifiedShaders = false, bModifiedPixelShader = false, bZWriteEnabled = false, bModifiedBlendState = false, bModifiedSamplerState = false;
	float FullTransform = g_bEnableVR && g_bInTechRoom ? 1.0f : 0.0f;

	g_VSCBuffer = { 0 };
	g_VSCBuffer.aspect_ratio	  =  g_bRendering3D ? g_fAspectRatio : g_fConcourseAspectRatio;
	g_SSAO_PSCBuffer.aspect_ratio =  g_VSCBuffer.aspect_ratio;
	g_VSCBuffer.z_override		  = -1.0f;
	g_VSCBuffer.sz_override		  = -1.0f;
	g_VSCBuffer.mult_z_override	  = -1.0f;
	g_VSCBuffer.apply_uv_comp     =  false;
	g_VSCBuffer.bPreventTransform =  0.0f;
	g_VSCBuffer.bFullTransform	  =  FullTransform;
	g_VSCBuffer.scale_override    =  1.0f;

	g_PSCBuffer = { 0 };
	g_PSCBuffer.brightness      = MAX_BRIGHTNESS;
	g_PSCBuffer.fBloomStrength  = 1.0f;
	g_PSCBuffer.fPosNormalAlpha = 1.0f;
	g_PSCBuffer.fSSAOAlphaMult  = g_fSSAOAlphaOfs;
	g_PSCBuffer.fSSAOMaskVal    = g_DefaultGlobalMaterial.Metallic * 0.5f;
	g_PSCBuffer.fGlossiness     = g_DefaultGlobalMaterial.Glossiness;
	g_PSCBuffer.fSpecInt        = g_DefaultGlobalMaterial.Intensity;  // DEFAULT_SPEC_INT;
	g_PSCBuffer.fNMIntensity    = g_DefaultGlobalMaterial.NMIntensity;
	
	g_DCPSCBuffer = { 0 };
	g_DCPSCBuffer.ct_brightness	= g_fCoverTextureBrightness;
	g_DCPSCBuffer.dc_brightness = g_fDCBrightness;

	char* step = "";

	this->_deviceResources->InitInputLayout(resources->_inputLayout);
	if (g_bEnableVR)
		this->_deviceResources->InitVertexShader(resources->_sbsVertexShader);
	else
		// The original code used _vertexShader:
		this->_deviceResources->InitVertexShader(resources->_vertexShader);	
	this->_deviceResources->InitPixelShader(resources->_pixelShaderTexture);
	this->_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->_deviceResources->InitRasterizerState(resources->_rasterizerState);
	ID3D11PixelShader *lastPixelShader = resources->_pixelShaderTexture;

	float displayWidth  = (float)resources->_displayWidth;
	float displayHeight = (float)resources->_displayHeight;

	// Constant Buffer step (and aspect ratio)
	if (SUCCEEDED(hr))
	{
		step = "ConstantBuffer";

		if (g_config.AspectRatioPreserved)
		{
			if (resources->_backbufferHeight * resources->_displayWidth <= this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight)
			{
				width = this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth / this->_deviceResources->_displayHeight;
				height = this->_deviceResources->_backbufferHeight;
			}
			else
			{
				width = this->_deviceResources->_backbufferWidth;
				height = this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight / this->_deviceResources->_displayWidth;
			}
			// In this version, we're stretching the viewports to cover the full backbuffer. If no aspect correction is done in the
			// block above, then the SBS aspect_ratio is simply 2 because we're halving the screen.
			// However, if some aspect ratio was performed above, we need to compensate for that. The SBS viewports will already
			// stretch "width/2" to "backBufferWidth/2", (e.g. with displayWidth,Height set to 1280x1025 in the game and an actual
			// screen res of 3240x2160, the code will take the first case and set width to 2700, which will be stretched to 3240)
			// Thus, we need to revert that stretching and then multiply by 2 (because we're halving the screen size)
			if (!g_bOverrideAspectRatio) { // Only compute the aspect ratio if we didn't read it off of the config file
				g_fAspectRatio = 2.0f * width / this->_deviceResources->_backbufferWidth;
			}
		}
		else
		{
			width  = resources->_backbufferWidth;
			height = resources->_backbufferHeight;

			if (!g_bOverrideAspectRatio) {
				float original_aspect = (float)this->_deviceResources->_displayWidth / (float)this->_deviceResources->_displayHeight;
				float actual_aspect = (float)this->_deviceResources->_backbufferWidth / (float)this->_deviceResources->_backbufferHeight;
				g_fAspectRatio = actual_aspect / original_aspect;
			}
		}

		left = (this->_deviceResources->_backbufferWidth - width) / 2;
		top = (this->_deviceResources->_backbufferHeight - height) / 2;

		if (this->_deviceResources->_frontbufferSurface == nullptr)
		{
			scale = 1.0f;
		}
		else
		{
			if (this->_deviceResources->_backbufferHeight * this->_deviceResources->_displayWidth <= this->_deviceResources->_backbufferWidth * this->_deviceResources->_displayHeight)
			{
				scale = (float)this->_deviceResources->_backbufferHeight / (float)this->_deviceResources->_displayHeight;
			}
			else
			{
				scale = (float)this->_deviceResources->_backbufferWidth / (float)this->_deviceResources->_displayWidth;
			}

			scale *= g_config.Concourse3DScale;
		}

		//const float viewportScale[4] = { 2.0f / (float)this->_deviceResources->_displayWidth, -2.0f / (float)this->_deviceResources->_displayHeight, scale, 0 };
		if (g_bEnableVR) {
			g_VSCBuffer.viewportScale[0] = 1.0f / displayWidth;
			g_VSCBuffer.viewportScale[1] = 1.0f / displayHeight;
		} else {
			g_VSCBuffer.viewportScale[0] =  2.0f / displayWidth;
			g_VSCBuffer.viewportScale[1] = -2.0f / displayHeight;
			//log_debug("[DBG] [AC] displayWidth,Height: %0.3f,%0.3f", displayWidth, displayHeight);
			//[15860] [DBG] [AC] displayWidth, Height: 1600.000, 1200.000
		}
		g_VSCBuffer.viewportScale[2]  =  scale;
		//log_debug("[DBG] [AC] scale: %0.3f", scale); The scale seems to be 1 for unstretched nonVR
		g_VSCBuffer.viewportScale[3]  =  g_fGlobalScale;
		// If we're rendering to the Tech Library, then we should use the Concourse Aspect Ratio
		g_VSCBuffer.aspect_ratio	  =  g_bRendering3D ? g_fAspectRatio : g_fConcourseAspectRatio;
		g_SSAO_PSCBuffer.aspect_ratio =  g_VSCBuffer.aspect_ratio;
		g_VSCBuffer.apply_uv_comp     =  false;
		g_VSCBuffer.z_override        = -1.0f;
		g_VSCBuffer.sz_override       = -1.0f;
		g_VSCBuffer.mult_z_override   = -1.0f;

		g_SSAO_PSCBuffer.aspect_ratio = g_VSCBuffer.aspect_ratio;
		g_SSAO_PSCBuffer.vpScale[0] = g_VSCBuffer.viewportScale[0];
		g_SSAO_PSCBuffer.vpScale[1] = g_VSCBuffer.viewportScale[1];
		g_SSAO_PSCBuffer.vpScale[2] = g_VSCBuffer.viewportScale[2];
		g_SSAO_PSCBuffer.vpScale[3] = g_VSCBuffer.viewportScale[3];
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
		//this->_deviceResources->InitConstantBuffer(this->_deviceResources->_constantBuffer.GetAddressOf(), viewportScale);
		//g_fXWAScale = scale; // Store the current scale, the Dynamic Cockpit will use this value

		/*
		// This is the original monoscopic viewport:
		viewport.TopLeftX = (float)left;
		viewport.TopLeftY = (float)top;
		viewport.Width    = (float)width;
		viewport.Height   = (float)height;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);
		*/
	}

	// VertexBuffer Step
	if (SUCCEEDED(hr))
	{
		step = "VertexBuffer";
		
		//g_OrigVerts = (D3DTLVERTEX *)executeBuffer->_buffer;
		g_ExecuteVertexCount += executeBuffer->_executeData.dwVertexCount;

		if (!g_config.D3dHookExists)
		{
			D3D11_MAPPED_SUBRESOURCE map;
			hr = context->Map(this->_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

			if (SUCCEEDED(hr))
			{
				size_t length = sizeof(D3DTLVERTEX) * executeBuffer->_executeData.dwVertexCount;
				g_OrigVerts = (D3DTLVERTEX *)executeBuffer->_buffer;
				memcpy(map.pData, executeBuffer->_buffer, length);
				//memset((char*)map.pData + length, 0, this->_maxExecuteBufferSize - length);

				context->Unmap(this->_vertexBuffer, 0);
			}
		}

		if (SUCCEEDED(hr))
		{
			UINT stride = sizeof(D3DTLVERTEX);
			UINT offset = 0;

			this->_deviceResources->InitVertexBuffer(this->_vertexBuffer.GetAddressOf(), &stride, &offset);
		}
	}

	// IndexBuffer Step
	if (SUCCEEDED(hr))
	{
		step = "IndexBuffer";

		if (!g_config.D3dHookExists)
		{
			D3D11_MAPPED_SUBRESOURCE map;
			hr = context->Map(this->_indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

			if (SUCCEEDED(hr))
			{
				char* pData = executeBuffer->_buffer + executeBuffer->_executeData.dwInstructionOffset;
				char* pDataEnd = pData + executeBuffer->_executeData.dwInstructionLength;

				WORD* indice = (WORD*)map.pData;

				while (pData < pDataEnd)
				{
					LPD3DINSTRUCTION instruction = (LPD3DINSTRUCTION)pData;
					pData += sizeof(D3DINSTRUCTION) + instruction->bSize * instruction->wCount;

					switch (instruction->bOpcode)
					{
					case D3DOP_TRIANGLE:
					{
						LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);

						for (WORD i = 0; i < instruction->wCount; i++)
						{
							*indice = triangle->v1;
							indice++;

							*indice = triangle->v2;
							indice++;

							*indice = triangle->v3;
							indice++;

							triangle++;
						}
					}
					}
				}

				context->Unmap(this->_indexBuffer, 0);
			}
		}
		
		if (SUCCEEDED(hr))
		{
			this->_deviceResources->InitIndexBuffer(this->_indexBuffer, g_config.D3dHookExists);
		}
	}

	/*
	 * When the game is displaying 3D geometry in the tech room, we have a hybrid situation.
	 * First, the game will call UpdateViewMatrix() at the beginning of 2D content, like the Concourse
	 * and 2D menus, but if the Tech Room is displayed, then it will come down this path to render 3D
	 * content before going back to the 2D path and executing the final Flip().
	 * In other words, when the Tech Room is displayed, we already called UpdateViewMatrix(), so we
	 * don't need to call it again here.
	 */
	if (g_ExecuteCount == 1 && !g_bInTechRoom) { //only wait once per frame
		// Synchronization point to wait for vsync before we start to send work to the GPU
		// This avoids blocking the CPU while the compositor waits for the pixel shader effects to run in the GPU
		// (that's what happens if we sync after Submit+Present)
		UpdateViewMatrix(); // g_ExecuteCount == 1 && !g_bInTechRoom
	}

	// Render images
	if (SUCCEEDED(hr))
	{
		char* pData = executeBuffer->_buffer + executeBuffer->_executeData.dwInstructionOffset;
		char* pDataEnd = pData + executeBuffer->_executeData.dwInstructionLength;
		// lastTextureSelected is used to predict what is going to be rendered next
		Direct3DTexture* lastTextureSelected = NULL;

		UINT currentIndexLocation = 0;

		while (pData < pDataEnd)
		{
			LPD3DINSTRUCTION instruction = (LPD3DINSTRUCTION)pData;
			pData += sizeof(D3DINSTRUCTION) + instruction->bSize * instruction->wCount;

			switch (instruction->bOpcode)
			{
			// lastTextureSelected is updated here, in the TEXTUREHANDLE subcase
			case D3DOP_STATERENDER:
			{
				g_ExecuteStateCount += instruction->wCount;

				LPD3DSTATE state = (LPD3DSTATE)(instruction + 1);

				for (WORD i = 0; i < instruction->wCount; i++)
				{
					switch (state->drstRenderStateType)
					{
					case D3DRENDERSTATE_TEXTUREHANDLE:
					{
						Direct3DTexture* texture = g_config.WireframeFillMode ? nullptr : (Direct3DTexture*)state->dwArg[0];
						ID3D11PixelShader* pixelShader;

						if (texture == nullptr)
						{
							resources->InitPSShaderResourceView(nullptr);
							pixelShader = resources->_pixelShaderSolid;
							// Nullify the last texture selected
							lastTextureSelected = NULL;
						}
						else
						{
							texture->_refCount++;
							this->_deviceResources->InitPSShaderResourceView(texture->_textureView.Get());
							texture->_refCount--;

							pixelShader = resources->_pixelShaderTexture;
							// Keep the last texture selected and tag it (classify it) if necessary
							lastTextureSelected = texture;
							if (!lastTextureSelected->is_Tagged)
								lastTextureSelected->TagTexture();
						}

						resources->InitPixelShader(pixelShader);
						// Save the last pixel shader set (in this case, that's either the
						// pixelshadertexture or the pixelshadersolid
						lastPixelShader = pixelShader;
						bModifiedPixelShader = false;
						break;
					}

					case D3DRENDERSTATE_TEXTUREADDRESS:
						this->_renderStates->SetTextureAddress((D3DTEXTUREADDRESS)state->dwArg[0]);
						break;

					case D3DRENDERSTATE_ALPHABLENDENABLE:
						this->_renderStates->SetAlphaBlendEnabled(state->dwArg[0]);
						break;
					case D3DRENDERSTATE_TEXTUREMAPBLEND:
						this->_renderStates->SetTextureMapBlend((D3DTEXTUREBLEND)state->dwArg[0]);
						break;
					case D3DRENDERSTATE_SRCBLEND:
						this->_renderStates->SetSrcBlend((D3DBLEND)state->dwArg[0]);
						break;
					case D3DRENDERSTATE_DESTBLEND:
						this->_renderStates->SetDestBlend((D3DBLEND)state->dwArg[0]);
						break;

					case D3DRENDERSTATE_ZENABLE:
						this->_renderStates->SetZEnabled(state->dwArg[0]);
						break;
					case D3DRENDERSTATE_ZWRITEENABLE:
						this->_renderStates->SetZWriteEnabled(state->dwArg[0]);
						break;
					case D3DRENDERSTATE_ZFUNC:
						this->_renderStates->SetZFunc((D3DCMPFUNC)state->dwArg[0]);
						break;
					}

					state++;
				}

				break;
			}

			case D3DOP_TRIANGLE:
			{
				g_ExecuteTriangleCount++;
				g_ExecuteIndexCount += instruction->wCount * 3;

				step = "SamplerState";
				D3D11_SAMPLER_DESC samplerDesc = this->_renderStates->GetSamplerDesc();
				if (FAILED(hr = this->_deviceResources->InitSamplerState(nullptr, &samplerDesc)))
					break;

				step = "BlendState";
				D3D11_BLEND_DESC blendDesc = this->_renderStates->GetBlendDesc();
				if (FAILED(hr = this->_deviceResources->InitBlendState(nullptr, &blendDesc))) {
					log_debug("[DBG] Caught error at Execute:BlendState");
					hr = device->GetDeviceRemovedReason();
					log_debug("[DBG] Reason: 0x%x", hr);
					break;
				}

				step = "DepthStencilState";
				D3D11_DEPTH_STENCIL_DESC depthDesc = this->_renderStates->GetDepthStencilDesc();
				if (FAILED(hr = this->_deviceResources->InitDepthStencilState(nullptr, &depthDesc)))
					break;

				// Capture the non-VR viewport that is used with the non-VR vertexshader:
				g_nonVRViewport.TopLeftX = (float)left;
				g_nonVRViewport.TopLeftY = (float)top;
				g_nonVRViewport.Width    = (float)width;
				g_nonVRViewport.Height   = (float)height;
				g_nonVRViewport.MinDepth = D3D11_MIN_DEPTH;
				g_nonVRViewport.MaxDepth = D3D11_MAX_DEPTH;

				step = "Draw";

				/*********************************************************************
					 State management begins here
				 *********************************************************************

				   Unfortunately, we need to maintain a number of flags to tell what the
				   engine is about to render. If we had access to the engine, we could do
				   away with all these flags.

				   The sequence of events goes like this:
					  - Draw the Skybox with ZWrite disabled, this is done in the first few draw calls.
					  - Draw 3D objects, including the cockpit.
					  - Disable ZWrite to draw engine glow and brackets.
					  - Disable ZWrite to draw the GUI elements.
					  - Enable ZWrite to draw the targeting computer HUD.
					  - Enable ZWrite to draw the targeted 3D object.

					NOTE: "Floating GUI elements" is what I call the GUI elements drawn along the bottom half of the
						  screen: the left and right panels, the main targeting computer and the laser charge state.
						  I call these "Floating GUI elements" because they are expected to "float" before the cockpit
						  or they will just cause visual contention. The rest of the GUI (radars, HUD, etc) is not in
						  this category -- I just call them GUI.
						  Floating GUI elems have their own list of CRCs independent of the GUI CRCs

					At the beginning of every frame:
					  - g_bTargetCompDrawn will be set to false
					  - g_iFloating_GUI_drawn_counter is set to 0
					  - g_bPrevIsFloatingGUI3DObject && g_bIsFloating3DObject are set to false

					We're using a non-exhaustive list of GUI CRCs to tell when the 3D content has finished drawing.
				*/

				const bool bExternalCamera = (bool)PlayerDataTable[*g_playerIndex].externalCamera;
				/* 
				 * ZWriteEnabled is false when rendering the background starfield or when
				 * rendering the GUI elements -- except that the targetting computer GUI
				 * is rendered with ZWriteEnabled == true. This is probably done to clear
				 * the depth stencil in the area covered by the targeting computer, so that
				 * later, when the targeted object is rendered, it can render without
				 * interfering with the z-values of the cockpit.
				 */
				bZWriteEnabled = this->_renderStates->GetZWriteEnabled();
				/* If we have drawn at least one Floating GUI element and now the ZWrite has been enabled
				   again, then we're about to draw the floating 3D element. Although, g_bTargetCompDrawn
				   isn't fully semantically correct because it should be set to true *after* it has actually
				   been drawn. Here it's being set *before* it's drawn. */
				if (!g_bTargetCompDrawn && g_iFloatingGUIDrawnCounter > 0 && bZWriteEnabled)
					g_bTargetCompDrawn = true;
				// lastTextureSelected can be NULL. This happens when drawing the square
				// brackets around the currently-selected object (and maybe other situations)
				bool bSunCentroidComputed = false;
				Vector3 SunCentroid;
				Vector2 SunCentroid2D;
				const bool bLastTextureSelectedNotNULL = (lastTextureSelected != NULL);
				bool bIsLaser = false, bIsLightTexture = false, bIsText = false, bIsReticle = false, bIsReticleCenter = false;
				bool bIsGUI = false, bIsLensFlare = false, bIsHyperspaceTunnel = false, bIsSun = false;
				bool bIsCockpit = false, bIsGunner = false, bIsExterior = false, bIsDAT = false;
				bool bIsActiveCockpit = false, bIsBlastMark = false, bIsTargetHighlighted = false;
				bool bIsHologram = false, bIsNoisyHolo = false, bIsTransparent = false, bIsDS2CoreExplosion = false;
				bool bWarheadLocked = PlayerDataTable[*g_playerIndex].warheadArmed && PlayerDataTable[*g_playerIndex].warheadLockState == 3;
				bool bIsElectricity = false, bIsExplosion = false, bHasMaterial = false;
				if (bLastTextureSelectedNotNULL) {
					if (g_bDynCockpitEnabled && lastTextureSelected->is_DynCockpitDst) 
					{
						int idx = lastTextureSelected->DCElementIndex;
						if (idx >= 0 && idx < g_iNumDCElements) {
							bIsHologram |= g_DCElements[idx].bHologram;
							bIsNoisyHolo |= g_DCElements[idx].bNoisyHolo;
							bIsTransparent |= g_DCElements[idx].bTransparent;
						}
					}

					bIsLaser = lastTextureSelected->is_Laser || lastTextureSelected->is_TurboLaser;
					bIsLightTexture = lastTextureSelected->is_LightTexture;
					bIsText = lastTextureSelected->is_Text;
					bIsReticle = lastTextureSelected->is_Reticle;
					bIsReticleCenter = lastTextureSelected->is_ReticleCenter;
					g_bIsTargetHighlighted |= lastTextureSelected->is_HighlightedReticle;
					//g_bIsTargetHighlighted |= (PlayerDataTable[*g_playerIndex].warheadArmed && PlayerDataTable[*g_playerIndex].warheadLockState == 3);
					bIsTargetHighlighted = g_bIsTargetHighlighted || g_bPrevIsTargetHighlighted;
					bIsGUI = lastTextureSelected->is_GUI;
					bIsLensFlare = lastTextureSelected->is_LensFlare;
					bIsHyperspaceTunnel = lastTextureSelected->is_HyperspaceAnim;
					bIsSun = lastTextureSelected->is_Sun;
					bIsCockpit = lastTextureSelected->is_CockpitTex;
					bIsGunner = lastTextureSelected->is_GunnerTex;
					bIsExterior = lastTextureSelected->is_Exterior;
					bIsDAT = lastTextureSelected->is_DAT;
					bIsActiveCockpit = lastTextureSelected->ActiveCockpitIdx > -1;
					bIsBlastMark = lastTextureSelected->is_BlastMark;
					//bIsSkyDome = lastTextureSelected->is_SkydomeLight;
					bIsDS2CoreExplosion = lastTextureSelected->is_DS2_Reactor_Explosion;
					bIsElectricity = lastTextureSelected->is_Electricity;
					bIsExplosion = lastTextureSelected->is_Explosion;
					bHasMaterial = lastTextureSelected->bHasMaterial;
				}
				g_bPrevIsSkyBox = g_bIsSkyBox;
				// bIsSkyBox is true if we're about to render the SkyBox
				//g_bIsSkyBox = !bZWriteEnabled && g_iExecBufCounter <= g_iSkyBoxExecIndex;
				g_bIsSkyBox = !bZWriteEnabled && !g_bSkyBoxJustFinished && bLastTextureSelectedNotNULL && lastTextureSelected->is_DAT;
				g_bIsTrianglePointer = bLastTextureSelectedNotNULL && lastTextureSelected->is_TrianglePointer;
				g_bPrevIsPlayerObject = g_bIsPlayerObject;
				g_bIsPlayerObject = bIsCockpit || bIsExterior || bIsGunner;
				const bool bIsFloatingGUI = bLastTextureSelectedNotNULL && lastTextureSelected->is_Floating_GUI;
				//bool bIsTranspOrGlow = bIsNoZWrite && _renderStates->GetZFunc() == D3DCMP_GREATER;
				// Hysteresis detection (state is about to switch to render something different, like the HUD)
				g_bPrevIsFloatingGUI3DObject = g_bIsFloating3DObject;
				g_bIsFloating3DObject = g_bTargetCompDrawn && bLastTextureSelectedNotNULL &&
					!lastTextureSelected->is_Text && !lastTextureSelected->is_TrianglePointer &&
					!lastTextureSelected->is_Reticle && !lastTextureSelected->is_Floating_GUI &&
					!lastTextureSelected->is_TargetingComp && !bIsLensFlare;

				if (g_bPrevIsSkyBox && !g_bIsSkyBox && !g_bSkyBoxJustFinished) {
					// The skybox just finished, capture it, replace it, etc
					g_bSkyBoxJustFinished = true;
					// Capture the background; but only if we're not in hyperspace -- we don't want to
					// capture the black background used by the game!
				}
				// bIsNoZWrite is true if ZWrite is disabled and the SkyBox has been rendered.
				// bIsBracket depends on bIsNoZWrite 
				//const bool bIsNoZWrite = !bZWriteEnabled && g_iExecBufCounter > g_iSkyBoxExecIndex;
				const bool bIsNoZWrite = !bZWriteEnabled && g_bSkyBoxJustFinished;
				// In the hangar, shadows are enabled. Shadows don't have a texture and are rendered with
				// ZWrite disabled. So, how can we tell if a bracket is being rendered or a shadow?
				// Brackets are rendered with ZFunc D3DCMP_ALWAYS (8),
				// Shadows  are rendered with ZFunc D3DCMP_GREATEREQUAL (7)
				// Cockpit Glass & Engine Glow are rendered with ZFunc D3DCMP_GREATER (5)
				const bool bIsBracket = bIsNoZWrite && !bLastTextureSelectedNotNULL &&
					this->_renderStates->GetZFunc() == D3DCMP_ALWAYS;
				const bool bIsXWAHangarShadow = *g_playerInHangar && bIsNoZWrite && !bLastTextureSelectedNotNULL &&
					this->_renderStates->GetZFunc() == D3DCMP_GREATEREQUAL;
				// The GUI starts rendering whenever we detect a GUI element, or Text, or a bracket.
				// ... or not at all if we're in external view mode with nothing targeted.
				g_bPrevStartedGUI = g_bStartedGUI;
				// Apr 10, 2020: g_bDisableDiffuse will make the reticle look white when the HUD is
				// hidden. To prevent this, I added bIsAimingHUD to g_bStartedGUI; but I don't know
				// if this breaks VR. If it does, then I need to add !bIsAimingHUD around line 6425,
				// where I'm setting fDisableDiffuse = 1.0f
				g_bStartedGUI |= bIsGUI || bIsText || bIsBracket || bIsFloatingGUI || bIsReticle;
				// bIsScaleableGUIElem is true when we're about to render a HUD element that can be scaled down with Ctrl+Z
				g_bPrevIsScaleableGUIElem = g_bIsScaleableGUIElem;
				g_bIsScaleableGUIElem = g_bStartedGUI && !bIsReticle && !bIsBracket && !g_bIsTrianglePointer && !bIsLensFlare;

				//if (g_bReshadeEnabled && !g_bPrevStartedGUI && g_bStartedGUI) {
					// We're about to start rendering *ALL* the GUI: including the triangle pointer and text
					// This is where we can capture the current frame for post-processing effects
					//	context->ResolveSubresource(resources->_offscreenBufferAsInputReshade, 0,
					//		resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
				//}

				// Clear the Dynamic Cockpit foreground and background RTVs
				// We are now clearing the DC RTVs in two places. The other place is in
				// PrimarySurface, if we reach the Present() path and 
				// g_bDCWasClearedOnThisFrame is false
				if (!g_bPrevIsScaleableGUIElem && g_bIsScaleableGUIElem && !g_bScaleableHUDStarted) 
				{
					g_bScaleableHUDStarted = true;
					g_iDrawCounterAfterHUD = 0;
					// We're about to render the scaleable HUD, time to clear the dynamic cockpit textures
					if (g_bDynCockpitEnabled || g_bReshadeEnabled) 
					{
						float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
						context->ClearRenderTargetView(resources->_renderTargetViewDynCockpit, bgColor);
						context->ClearRenderTargetView(resources->_renderTargetViewDynCockpitBG, bgColor);
						context->ClearRenderTargetView(resources->_DCTextRTV, bgColor);
						g_bDCWasClearedOnThisFrame = true;
						//log_debug("[DBG] DC Clear RTVs");

						// I think (?) we need to clear the depth buffer here so that the targeted craft is drawn properly
						//context->ClearDepthStencilView(this->_deviceResources->_depthStencilViewL,
						//	D3D11_CLEAR_DEPTH, resources->clearDepth, 0);

						/*
						[14656] [DBG] PRESENT *******************
						[14656] [DBG] Render CMD sub-bracket
						[14656] [DBG] Execute (1)
						[14656] [DBG] Replacing cockpit textures with DC
						[14656] [DBG] Replacing cockpit textures with DC
						[14656] [DBG] Replacing cockpit textures with DC
						...
						[14656] [DBG] Replacing cockpit textures with DC
						[14656] [DBG] DC RTV CLEARED
						[14656] [DBG] Render to DC RTV
						[14656] [DBG] Render to DC RTV
						...
						[14656] [DBG] Render to DC RTV
						[14656] [DBG] Execute (2)
						[14656] [DBG] PRESENT *******************
						*/
					}
				}

				const bool bRenderToDynCockpitBuffer = (g_bDCManualActivate || bExternalCamera) && (g_bDynCockpitEnabled || g_bReshadeEnabled) &&
					(bLastTextureSelectedNotNULL && g_bScaleableHUDStarted && g_bIsScaleableGUIElem);
				bool bRenderToDynCockpitBGBuffer = false;

				// Render HUD backgrounds to their own layer (HUD BG)
				if ((g_bDCManualActivate || bExternalCamera) && (g_bDynCockpitEnabled || g_bReshadeEnabled) &&
					(bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_HUDRegionSrc))
					bRenderToDynCockpitBGBuffer = true;

				/*
				Justagai shared the following information. Some phases are extremely quick and may not
				be visible unless breakpoints are set.
				0x6 may not be used in XWA -- it may be there from a previous game in the series
				enum HyperspacePhase
				{
				  HyperspacePhase_None = 0x0,
				  HyperspacePhase_Preparing = 0x1,
				  HyperspacePhase_StartStreaks = 0x2,
				  HyperspacePhase_Streaks = 0x3,
				  HyperspacePhase_InTunnel = 0x4,
				  HyperspacePhase_Exiting = 0x5,
				  HyperspacePhase_Exited = 0x6,
				};
				*/
				// Update the Hyperspace FSM -- but only update it exactly once per frame
				// Also clear the shaderToyAuxBuf if any tracker is enabled
				if (!g_bHyperspaceEffectRenderedOnCurrentFrame) {
					switch (g_HyperspacePhaseFSM) {
					case HS_INIT_ST:
						g_PSCBuffer.bInHyperspace = 0;
						g_bHyperExternalToCockpitTransition = false;
						g_bHyperspaceLastFrame = false;
						g_bHyperspaceTunnelLastFrame = false;
						if (PlayerDataTable[*g_playerIndex].hyperspacePhase == 2) {
							// Hyperspace has *just* been engaged. Save the current cockpit camera heading so we can restore it
							g_bHyperspaceFirstFrame = true;
							g_PSCBuffer.bInHyperspace = 1;
							g_PSCBuffer.fBloomStrength = g_BloomConfig.fHyperStreakStrength;
							g_bClearedAuxBuffer = false; // We use this flag to clear the aux buffer if the cockpit camera moves
							if (PlayerDataTable[*g_playerIndex].cockpitCameraYaw != g_fLastCockpitCameraYaw ||
								PlayerDataTable[*g_playerIndex].cockpitCameraPitch != g_fLastCockpitCameraPitch)
								g_bHyperHeadSnapped = true;
							if (*numberOfPlayersInGame == 1) {
								PlayerDataTable[*g_playerIndex].cockpitCameraYaw = g_fLastCockpitCameraYaw;
								PlayerDataTable[*g_playerIndex].cockpitCameraPitch = g_fLastCockpitCameraPitch;
								// Restoring the cockpitX/Y/Z/Reference didn't seem to have any effect when cockpit inertia was on:
								//PlayerDataTable[*g_playerIndex].cockpitXReference = g_lastCockpitXReference;
								//PlayerDataTable[*g_playerIndex].cockpitYReference = g_lastCockpitYReference;
								//PlayerDataTable[*g_playerIndex].cockpitZReference = g_lastCockpitZReference;
							}
							g_fCockpitCameraYawOnFirstHyperFrame = g_fLastCockpitCameraYaw;
							g_fCockpitCameraPitchOnFirstHyperFrame = g_fLastCockpitCameraPitch;
							g_HyperspacePhaseFSM = HS_HYPER_ENTER_ST;
							// Compute a new random seed for this hyperspace jump
							g_fHyperspaceRand = (float)rand() / (float)RAND_MAX;
							if (g_bLastFrameWasExterior)
								// External view --> Cockpit transition for the hyperspace jump
								g_bHyperExternalToCockpitTransition = true;
						}
						break;
					case HS_HYPER_ENTER_ST:
						g_PSCBuffer.bInHyperspace = 1;
						g_bHyperspaceLastFrame = false;
						g_bHyperspaceTunnelLastFrame = false;
						// UPDATE 3/30/2020:
						// This whole block was removed to support cockpit inertia. The aux buffer can't be cleared
						// on the first hyperspace frame because it will otherwise "blink". We have to clear this
						// buffer *after* the first hyperspace frame has been rendered. This is now done in
						// PrimarySurface::Flip() after the first hyperspace frame has been presented. I also removed
						// all the conditions, so this buffer is now cleared all the time. Later I'll have to fix
						// that to allow blending the background with the trails; but... later

						// Clear the captured offscreen buffer if the cockpit camera has changed from the pose
						// it had when entering hyperspace, or clear it if we're using any VR mode, because chances
						// are the user's head position moved anyway if 6dof is enabled.
						// If we clear this buffer *at this point* when cockpit inertia is enabled, then we'll see
						// the screen blink. We need to preserve this buffer for the first hyperspace frame: it can
						// be cleared later. When rendering non-VR, we inhibit the first hyperspace frame, so we need
						// this.
						// This whole block may have to be removed for TrackIR/VR; but we also need to make sure we're
						// inhibiting the first frame in those cases.
						/*
						if (!g_bClearedAuxBuffer &&
							  (g_bEnableVR || g_TrackerType == TRACKER_TRACKIR || // g_bCockpitInertiaEnabled ||
							     (
							        PlayerDataTable[*g_playerIndex].cockpitCameraYaw != g_fCockpitCameraYawOnFirstHyperFrame ||
							        PlayerDataTable[*g_playerIndex].cockpitCameraPitch != g_fCockpitCameraPitchOnFirstHyperFrame
							     )
							  )
						   ) 
						{
							float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
							g_bClearedAuxBuffer = true;
							// DEBUG save the aux buffer for analysis
							//DirectX::SaveWICTextureToFile(context, resources->_shadertoyBuf, GUID_ContainerFormatJpeg,
							//	L"C:\\Temp\\_shadertoyBuf.jpg");
							// DEBUG

							context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
							context->ResolveSubresource(resources->_shadertoyAuxBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
							if (g_bUseSteamVR)
								context->CopyResource(resources->_shadertoyAuxBufR, resources->_shadertoyAuxBuf);
						}
						*/

						if (PlayerDataTable[*g_playerIndex].hyperspacePhase == 4) {
							g_HyperspacePhaseFSM = HS_HYPER_TUNNEL_ST;
							g_PSCBuffer.fBloomStrength = g_BloomConfig.fHyperTunnelStrength;
							// We're about to enter the hyperspace tunnel, change the color of the lights:
							float fade = 1.0f;
							for (int i = 0; i < 2; i++, fade *= 0.5f) {
								memcpy(&g_TempLightVector[i], &g_LightVector[i], sizeof(Vector4));
								memcpy(&g_TempLightColor[i], &g_LightColor[i], sizeof(Vector4));
								g_LightColor[i].x = /* fade * */ 0.10f;
								g_LightColor[i].y = /* fade * */ 0.15f;
								g_LightColor[i].z = /* fade * */ 1.50f;
							}
							// Reset the Sun --> XWA light association every time we enter hyperspace
							//for (uint32_t i = 0; i < g_AuxTextureVector.size(); i++)
							//	g_AuxTextureVector[i]->AssociatedXWALight = -1;
						}
						break;
					case HS_HYPER_TUNNEL_ST:
						g_PSCBuffer.bInHyperspace = 1;
						g_bHyperspaceLastFrame = false;
						g_bHyperspaceTunnelLastFrame = false;
						if (PlayerDataTable[*g_playerIndex].hyperspacePhase == 3) {
							//log_debug("[DBG] [FSM] HS_HYPER_TUNNEL_ST --> HS_HYPER_EXIT_ST");
							g_bHyperspaceTunnelLastFrame = true;
							g_HyperspacePhaseFSM = HS_HYPER_EXIT_ST;
							g_PSCBuffer.fBloomStrength = g_BloomConfig.fHyperStreakStrength;
							// Restore the previous color of the lights
							for (int i = 0; i < 2; i++) {
								memcpy(&g_LightVector[i], &g_TempLightVector[i], sizeof(Vector4));
								memcpy(&g_LightColor[i], &g_TempLightColor[i], sizeof(Vector4));
							}

							// Clear the previously-captured offscreen buffer: we don't want to display it again when exiting
							// hyperspace
							if (!g_bClearedAuxBuffer) 
							{
								float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
								g_bClearedAuxBuffer = true;
								context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
								context->ResolveSubresource(resources->_shadertoyAuxBuf, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
								if (g_bUseSteamVR)
									context->CopyResource(resources->_shadertoyAuxBufR, resources->_shadertoyAuxBuf);
							}
						}
						break;
					case HS_HYPER_EXIT_ST:
						g_PSCBuffer.bInHyperspace = 1;
						g_bHyperspaceLastFrame = false;
						g_bHyperspaceTunnelLastFrame = false;
						if (PlayerDataTable[*g_playerIndex].hyperspacePhase == 0) {
							//log_debug("[DBG] [FSM] HS_HYPER_EXIT_ST --> HS_POST_HYPER_EXIT_ST");
							g_iHyperExitPostFrames = 0;
							g_HyperspacePhaseFSM = HS_POST_HYPER_EXIT_ST;
							g_bHyperspaceLastFrame = true;
							// Reset the Sun --> XWA light association every time we exit hyperspace
							ResetXWALightInfo();
						}
						break;
					case HS_POST_HYPER_EXIT_ST:
						g_PSCBuffer.bInHyperspace = 1;
						g_bHyperspaceLastFrame = false;
						g_bHyperspaceTunnelLastFrame = false;
						if (g_iHyperExitPostFrames > MAX_POST_HYPER_EXIT_FRAMES) {
							//log_debug("[DBG] [FSM] HS_POST_HYPER_EXIT_ST --> HS_INIT_ST");
							g_HyperspacePhaseFSM = HS_INIT_ST;
						}
						break;
					}

//#ifdef HYPER_OVERRIDE
					// DEBUG
					if (g_bHyperDebugMode)
						g_HyperspacePhaseFSM = (HyperspacePhaseEnum)g_iHyperStateOverride;
					// DEBUG
//#endif
				}
				//if (g_bHyperspaceFirstFrame)
				//	goto out;

				/*************************************************************************
					State management ends here, special state management starts
				 *************************************************************************/
				
				// Resolve the depth buffers. Capture the current screen to shadertoyAuxBuf
				if (!g_bPrevStartedGUI && g_bStartedGUI) {
					g_bSwitchedToGUI = true;
					// We're about to start rendering *ALL* the GUI: including the triangle pointer and text
					// This is where we can capture the current frame for post-processing effects

					// Resolve the Depth Buffers (we have dual SSAO, so there are two depth buffers)
					if (g_bAOEnabled) {
						g_bDepthBufferResolved = true;
						context->ResolveSubresource(resources->_depthBufAsInput, 0, resources->_depthBuf, 0, AO_DEPTH_BUFFER_FORMAT);
						if (g_bUseSteamVR)
							context->ResolveSubresource(resources->_depthBufAsInputR, 0,
								resources->_depthBufR, 0, AO_DEPTH_BUFFER_FORMAT);

						// DEBUG
						//if (g_iPresentCounter == 100) {
						   ////DirectX::SaveWICTextureToFile(context, resources->_depthBufAsInput, GUID_ContainerFormatJpeg,
						   ////	L"c:\\temp\\_depthBuf.jpg");
						   //// //DirectX::SaveWICTextureToFile(context, resources->_normBufAsInput, GUID_ContainerFormatJpeg,
						   //// //	 L"c:\\temp\\_normBuf.jpg");
						   //DirectX::SaveDDSTextureToFile(context, resources->_depthBufAsInput, L"c:\\temp\\_depthBuf.dds");
						   //log_debug("[DBG] [AO] _depthBuf.dds dumped");
						//}
						// DEBUG
					}

					// Capture the current-frame-so-far (cockpit+background) for the new hyperspace effect; but only if we're
					// not travelling through hyperspace:
					{
						if (g_bHyperDebugMode || g_HyperspacePhaseFSM == HS_INIT_ST || g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST)
						{
							g_fLastCockpitCameraYaw   = PlayerDataTable[*g_playerIndex].cockpitCameraYaw;
							g_fLastCockpitCameraPitch = PlayerDataTable[*g_playerIndex].cockpitCameraPitch;
							g_lastCockpitXReference   = PlayerDataTable[*g_playerIndex].cockpitXReference;
							g_lastCockpitYReference   = PlayerDataTable[*g_playerIndex].cockpitYReference;
							g_lastCockpitZReference   = PlayerDataTable[*g_playerIndex].cockpitZReference;

							context->ResolveSubresource(resources->_shadertoyAuxBuf, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
							if (g_bUseSteamVR) {
								context->ResolveSubresource(resources->_shadertoyAuxBufR, 0, resources->_offscreenBufferR, 0, BACKBUFFER_FORMAT);
							}
						}
					}
				}

				/*************************************************************************
					Special state management ends here
				 *************************************************************************/

				if (!g_bAOEnabled && bIsLightTexture) {
					// The EngineGlow texture is also used to render the smoke, so it will glow. I
					// tried enabling the transparency when the engine glow is about to be rendered,
					// but the smoke still doesn't look quite right when the effects are off -- I
					// think transparency was already enabled, so that's why it didn't make any difference
					//|| (bLastTextureSelectedNotNULL && lastTextureSelected->is_EngineGlow)) {

					// We need to set the blend state properly for light textures when AO is disabled.
					// Not sure why; but here you go: This fixes the black textures bug
					EnableTransparency();
				}

				// Before the exterior (HUD) hook, the HUD was never rendered in the exterior view, so
				// switching to the cockpit while entering hyperspace was not a problem. Now, the HUD
				// can be rendered in the exterior view and when we switch to the cockpit for the
				// hyperspace effect we apparently keep rendering it partially, causing artifacts in
				// the cockpit while in hyperspace. It's better to skip those calls instead.
				if (g_bHyperExternalToCockpitTransition && g_bIsScaleableGUIElem && g_PSCBuffer.bInHyperspace)
					goto out;

				//if (bLastTextureSelectedNotNULL && lastTextureSelected->is_LightTexture)
				//	goto out;

				//if (PlayerDataTable[*g_playerIndex].cockpitDisplayed)
				//if (PlayerDataTable[*g_playerIndex].cockpitDisplayed2)
				//	goto out;

				//if (bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_LeftSensorSrc && debugTexture == NULL) {
				//	log_debug("[DBG] [DC] lastTextureSelected is_DC_LeftSensorSrc TRUE");
				//	debugTexture = lastTextureSelected;
				//}
				//if (debugTexture != NULL) {
				//	log_debug("[DBG] [DC] debugTexture->is_DC_LeftSensorSrc: %d", debugTexture->is_DC_LeftSensorSrc);
				//}

				// Dynamic Cockpit: Reset the DC HUD Regions
				if (g_bResetDC) {
					for (unsigned int i = 0; i < g_DCHUDRegions.boxes.size(); i++) {
						g_DCHUDRegions.boxes[i].bLimitsComputed = false;
					}
					g_bResetDC = false;
					log_debug("[DBG] [DC] DC HUD Regions reset");
				}

				// **************************************************************
				// Dynamic Cockpit: Capture the bounds for all the HUD elements
				// **************************************************************
				{
					// Compute the limits for the "screen_def" and "erase_screen_def" areas -- these areas are
					// not related to any single HUD element
					if (g_bDCManualActivate && g_bDynCockpitEnabled) 
					{
						DCHUDRegion *dcSrcBox = NULL;
						DCElemSrcBox *dcElemSrcBox = NULL;
						float x0, y0, x1, y1, sx, sy;

						dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TEXT_RADIO_DC_ELEM_SRC_IDX];
						x0 = dcElemSrcBox->uv_coords.x0;
						y0 = dcElemSrcBox->uv_coords.y0;
						x1 = dcElemSrcBox->uv_coords.x1;
						y1 = dcElemSrcBox->uv_coords.y1;
						// These coords are relative to in-game resolution, we need to convert them to
						// screen/desktop uv coords
						x0 *= g_fCurInGameWidth;
						y0 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x0, y0, &sx, &sy);
						dcElemSrcBox->coords.x0 = sx / g_fCurScreenWidth;
						dcElemSrcBox->coords.y0 = sy / g_fCurScreenHeight;

						x1 *= g_fCurInGameWidth;
						y1 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x1, y1, &sx, &sy);
						dcElemSrcBox->coords.x1 = sx / g_fCurScreenWidth;
						dcElemSrcBox->coords.y1 = sy / g_fCurScreenHeight;
						dcElemSrcBox->bComputed = true;
						/*log_debug("[DBG] [DC] RADIO (x0,y0)-(x1,y1): (%0.1f, %0.1f)-(%0.1f, %0.1f)",
							g_fCurScreenWidth * dcElemSrcBox->coords.x0, g_fCurScreenHeight * dcElemSrcBox->coords.y0,
							g_fCurScreenWidth * dcElemSrcBox->coords.x1, g_fCurScreenHeight * dcElemSrcBox->coords.y1);*/


						dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TEXT_SYSTEM_DC_ELEM_SRC_IDX];
						x0 = dcElemSrcBox->uv_coords.x0;
						y0 = dcElemSrcBox->uv_coords.y0;
						x1 = dcElemSrcBox->uv_coords.x1;
						y1 = dcElemSrcBox->uv_coords.y1;
						// These coords are relative to in-game resolution, we need to convert them to
						// screen/desktop uv coords
						x0 *= g_fCurInGameWidth;
						y0 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x0, y0, &sx, &sy);
						dcElemSrcBox->coords.x0 = sx / g_fCurScreenWidth;
						dcElemSrcBox->coords.y0 = sy / g_fCurScreenHeight;

						x1 *= g_fCurInGameWidth;
						y1 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x1, y1, &sx, &sy);
						dcElemSrcBox->coords.x1 = sx / g_fCurScreenWidth;
						dcElemSrcBox->coords.y1 = sy / g_fCurScreenHeight;
						dcElemSrcBox->bComputed = true;
						/*log_debug("[DBG] [DC] SYSTEM (x0,y0)-(x1,y1): (%0.1f, %0.1f)-(%0.1f, %0.1f)",
							g_fCurScreenWidth * dcElemSrcBox->coords.x0, g_fCurScreenHeight * dcElemSrcBox->coords.y0,
							g_fCurScreenWidth * dcElemSrcBox->coords.x1, g_fCurScreenHeight * dcElemSrcBox->coords.y1);*/


						dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TEXT_CMD_DC_ELEM_SRC_IDX];
						x0 = dcElemSrcBox->uv_coords.x0;
						y0 = dcElemSrcBox->uv_coords.y0;
						x1 = dcElemSrcBox->uv_coords.x1;
						y1 = dcElemSrcBox->uv_coords.y1;
						// These coords are relative to in-game resolution, we need to convert them to
						// screen/desktop uv coords
						x0 *= g_fCurInGameWidth;
						y0 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x0, y0, &sx, &sy);
						dcElemSrcBox->coords.x0 = sx / g_fCurScreenWidth;
						dcElemSrcBox->coords.y0 = sy / g_fCurScreenHeight;

						x1 *= g_fCurInGameWidth;
						y1 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x1, y1, &sx, &sy);
						dcElemSrcBox->coords.x1 = sx / g_fCurScreenWidth;
						dcElemSrcBox->coords.y1 = sy / g_fCurScreenHeight;
						dcElemSrcBox->bComputed = true;
						/*log_debug("[DBG] [DC] CMD (x0,y0)-(x1,y1): (%0.1f, %0.1f)-(%0.1f, %0.1f)",
							g_fCurScreenWidth * dcElemSrcBox->coords.x0, g_fCurScreenHeight * dcElemSrcBox->coords.y0,
							g_fCurScreenWidth * dcElemSrcBox->coords.x1, g_fCurScreenHeight * dcElemSrcBox->coords.y1);*/

							// *********************************************************
							// Compute the coords for the "erase_screen_def" commands:
							// *********************************************************
						dcSrcBox = &g_DCHUDRegions.boxes[TEXT_RADIOSYS_HUD_BOX_IDX];
						x0 = dcSrcBox->uv_erase_coords.x0;
						y0 = dcSrcBox->uv_erase_coords.y0;
						x1 = dcSrcBox->uv_erase_coords.x1;
						y1 = dcSrcBox->uv_erase_coords.y1;
						// These coords are relative to in-game resolution, we need to convert them to
						// screen/desktop uv coords
						x0 *= g_fCurInGameWidth;
						y0 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x0, y0, &sx, &sy);
						dcSrcBox->erase_coords.x0 = sx / g_fCurScreenWidth;
						dcSrcBox->erase_coords.y0 = sy / g_fCurScreenHeight;

						x1 *= g_fCurInGameWidth;
						y1 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x1, y1, &sx, &sy);
						dcSrcBox->erase_coords.x1 = sx / g_fCurScreenWidth;
						dcSrcBox->erase_coords.y1 = sy / g_fCurScreenHeight;
						dcSrcBox->bLimitsComputed = true;
						/*log_debug("[DBG] [DC] RADIOSYS ERASE (x0,y0)-(x1,y1): (%0.1f, %0.1f)-(%0.1f, %0.1f)",
							g_fCurScreenWidth * dcSrcBox->erase_coords.x0, g_fCurScreenHeight * dcSrcBox->erase_coords.y0,
							g_fCurScreenWidth * dcSrcBox->erase_coords.x1, g_fCurScreenHeight * dcSrcBox->erase_coords.y1);*/

						dcSrcBox = &g_DCHUDRegions.boxes[TEXT_CMD_HUD_BOX_IDX];
						x0 = dcSrcBox->uv_erase_coords.x0;
						y0 = dcSrcBox->uv_erase_coords.y0;
						x1 = dcSrcBox->uv_erase_coords.x1;
						y1 = dcSrcBox->uv_erase_coords.y1;
						// These coords are relative to in-game resolution, we need to convert them to
						// screen/desktop uv coords
						x0 *= g_fCurInGameWidth;
						y0 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x0, y0, &sx, &sy);
						dcSrcBox->erase_coords.x0 = sx / g_fCurScreenWidth;
						dcSrcBox->erase_coords.y0 = sy / g_fCurScreenHeight;

						x1 *= g_fCurInGameWidth;
						y1 *= g_fCurInGameHeight;
						InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
							(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, x1, y1, &sx, &sy);
						dcSrcBox->erase_coords.x1 = sx / g_fCurScreenWidth;
						dcSrcBox->erase_coords.y1 = sy / g_fCurScreenHeight;
						dcSrcBox->bLimitsComputed = true;
						/*
						log_debug("[DBG] [DC] CMD ERASE (x0,y0)-(x1,y1): (%0.1f, %0.1f)-(%0.1f, %0.1f)",
							g_fCurScreenWidth * dcSrcBox->erase_coords.x0, g_fCurScreenHeight * dcSrcBox->erase_coords.y0,
							g_fCurScreenWidth * dcSrcBox->erase_coords.x1, g_fCurScreenHeight * dcSrcBox->erase_coords.y1);
						*/
					}

					// Capture the bounds for the left sensor:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_LeftSensorSrc)
					{
						if (!g_DCHUDRegions.boxes[LEFT_RADAR_HUD_BOX_IDX].bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[LEFT_RADAR_HUD_BOX_IDX];
							DCElemSrcBox *dcElemSrcBox = NULL;
							float minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box;
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							//log_debug("[DBG] [DC] Left Radar. in-game coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
							//	minX, minY, maxX, maxY);

							InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
							InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
							// Store the pixel coordinates
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);

							dcSrcBox->bLimitsComputed = true;
							//log_debug("[DBG] [DC] Left Sensor Region captured");

							// Get the limits for the left radar:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[LEFT_RADAR_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the laser recharge rate:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[LASER_RECHARGE_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the shield recharge rate:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SHIELD_RECHARGE_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							//Box elem_coords = g_DCElemSrcBoxes.src_boxes[LEFT_RADAR_DC_ELEM_SRC_IDX].coords;
							//log_debug("[DBG] [DC] g_iPresentCounter: %d, currentIndexLocation: %d", g_iPresentCounter, currentIndexLocation);
							//log_debug("[DBG] [DC] Left Radar HUD CAPTURED. screen coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
							//	box.x0, box.y0, box.x1, box.y1);
							//log_debug("[DBG] [DC] Left Radar ELEMENT screen coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
							//	elem_coords.x0, elem_coords.y0, elem_coords.x1, elem_coords.y1);
							/*uvfloat4 e = g_DCHUDRegions.boxes[LEFT_RADAR_HUD_BOX_IDX].erase_coords;
							log_debug("[DBG] [DC] Left Radar HUD erase coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
								e.x0 * g_fCurScreenWidth, e.y0 * g_fCurScreenHeight,
								e.x1 * g_fCurScreenWidth, e.y1 * g_fCurScreenHeight);*/
						}
					}

					// Capture the bounds for the right sensor:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_RightSensorSrc)
					{
						if (!g_DCHUDRegions.boxes[RIGHT_RADAR_HUD_BOX_IDX].bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[RIGHT_RADAR_HUD_BOX_IDX];
							DCElemSrcBox *dcElemSrcBox = NULL;
							float minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box;
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
							InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
							// Store the pixel coordinates
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
							dcSrcBox->bLimitsComputed = true;
							//log_debug("[DBG] [DC] Right Radar Region limits captured");

							// Get the limits for the right radar:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[RIGHT_RADAR_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the engine recharge rate:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[ENGINE_RECHARGE_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the beam recharge rate:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[BEAM_RECHARGE_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;
						}
					}

					// Capture the bounds for the shields:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_ShieldsSrc)
					{
						if (!g_DCHUDRegions.boxes[SHIELDS_HUD_BOX_IDX].bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[SHIELDS_HUD_BOX_IDX];
							DCElemSrcBox *dcElemSrcBox = NULL;
							float minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box;
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
							InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
							// Store the pixel coordinates
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
							dcSrcBox->bLimitsComputed = true;
							//log_debug("[DBG] [DC] Shields Region limits captured");

							// Get the limits for the shields:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SHIELDS_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the front shields strength:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SHIELDS_FRONT_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the back shields strength:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SHIELDS_BACK_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;
						}
					}

					// Capture the bounds for the tractor beam:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_BeamBoxSrc)
					{
						if (!g_DCHUDRegions.boxes[BEAM_HUD_BOX_IDX].bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[BEAM_HUD_BOX_IDX];
							DCElemSrcBox *dcElemSrcBox = NULL;
							float minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box;
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
							InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
							// Store the pixel coordinates
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
							dcSrcBox->bLimitsComputed = true;
							//log_debug("[DBG] [DC] Beam Region limits captured");

							// Get the limits for the shields:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[BEAM_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;
						}
					}

					// Capture the bounds for the targeting computer:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_TargetCompSrc)
					{
						if (!g_DCHUDRegions.boxes[TARGET_HUD_BOX_IDX].bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[TARGET_HUD_BOX_IDX];
							DCElemSrcBox *dcElemSrcBox = NULL;
							float minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box;
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
							InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
							// Store the pixel coordinates
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
							dcSrcBox->bLimitsComputed = true;
							//log_debug("[DBG] [DC] Target Comp Region limits captured");

							// Get the limits for the targeting computer:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGET_COMP_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the quad lasers, left side:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[QUAD_LASERS_L_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the quad lasers, left side:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[QUAD_LASERS_R_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the quad lasers, both sides:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[QUAD_LASERS_BOTH_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the dual lasers:
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[DUAL_LASERS_L_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[DUAL_LASERS_R_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[DUAL_LASERS_BOTH_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the B-Wing lasers
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[B_WING_LASERS_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the TIE-Defender lasers
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SIX_LASERS_BOTH_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SIX_LASERS_L_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SIX_LASERS_R_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the various elements in the CMD (mostly text):
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_NAME_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_SHD_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_HULL_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_CARGO_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_SYS_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_DIST_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGETED_OBJ_SUBCMP_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;
						}
					}

					// Capture the bounds for the left/right message boxes:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_SolidMsgSrc)
					{
						//if (lastTextureSelected->is_DC_BorderMsgSrc ||
						//	)

						DCHUDRegion *dcSrcBoxL = &g_DCHUDRegions.boxes[LEFT_MSG_HUD_BOX_IDX];
						DCHUDRegion *dcSrcBoxR = &g_DCHUDRegions.boxes[RIGHT_MSG_HUD_BOX_IDX];
						if (!dcSrcBoxL->bLimitsComputed || !dcSrcBoxR->bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = NULL;
							DCElemSrcBox *dcElemSrcBox = NULL;
							bool bLeft = false;
							float midX, minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box = { 0 };
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							// This HUD is used for both the left and right message boxes, so we need to check
							// which one is being rendered
							midX = (minX + maxX) / 2.0f;
							if (midX < g_fCurInGameWidth / 2.0f && !dcSrcBoxL->bLimitsComputed) {
								bLeft = true;
								dcSrcBox = dcSrcBoxL;
								dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[LEFT_MSG_DC_ELEM_SRC_IDX];
								//log_debug("[DBG] [DC] Left Msg Region captured");
							}
							else if (midX > g_fCurInGameWidth / 2.0f && !dcSrcBoxR->bLimitsComputed) {
								bLeft = false;
								dcSrcBox = dcSrcBoxR;
								dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[RIGHT_MSG_DC_ELEM_SRC_IDX];
								//log_debug("[DBG] [DC] Right Msg Region captured");
							}

							if (dcSrcBox != NULL) {
								InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
								InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
								//if (bLeft) DisplayBox("LEFT MSG: ", box); else DisplayBox("RIGHT MSG: ", box);
								// Store the pixel coordinates
								dcSrcBox->coords = box;
								// Compute and store the erase coordinates for this HUD Box
								ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
									dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
								dcSrcBox->bLimitsComputed = true;

								// Get the limits for the text contents itself
								dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
									uv_minmax, box, dcElemSrcBox->uv_coords);
								dcElemSrcBox->bComputed = true;
							}
						}
					}

					// Capture the bounds for the top-left bracket:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_TopLeftSrc)
					{
						if (!g_DCHUDRegions.boxes[TOP_LEFT_HUD_BOX_IDX].bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[TOP_LEFT_HUD_BOX_IDX];
							DCElemSrcBox *dcElemSrcBox = NULL;
							float minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box;
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
							InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
							// Store the pixel coordinates
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
							dcSrcBox->bLimitsComputed = true;
							//log_debug("[DBG] [DC] Top Left Region captured");

							// Get the limits for Speed & Throttle
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[SPEED_N_THROTTLE_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for Missiles
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[MISSILES_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for the CMD text
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[KW_TEXT_CMD_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							/*
							log_debug("[DBG] [DC] CMD Text coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
								g_fCurScreenWidth * dcElemSrcBox->coords.x0, g_fCurScreenHeight * dcElemSrcBox->coords.y0,
								g_fCurScreenWidth * dcElemSrcBox->coords.x1, g_fCurScreenHeight * dcElemSrcBox->coords.y1);
							*/

							// Get the limits for the top 2 rows of text (missiles, countermeasures, time, speed, throttle, etc)
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[KW_TEXT_TOP_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							/*
							log_debug("[DBG] [DC] Text Line 2 coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
								g_fCurScreenWidth * dcElemSrcBox->coords.x0, g_fCurScreenHeight * dcElemSrcBox->coords.y0,
								g_fCurScreenWidth * dcElemSrcBox->coords.x1, g_fCurScreenHeight * dcElemSrcBox->coords.y1);
							*/
						}
					}

					// Capture the bounds for the top-right bracket:
					if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_TopRightSrc)
					{
						if (!g_DCHUDRegions.boxes[TOP_RIGHT_HUD_BOX_IDX].bLimitsComputed)
						{
							DCHUDRegion *dcSrcBox = &g_DCHUDRegions.boxes[TOP_RIGHT_HUD_BOX_IDX];
							DCElemSrcBox *dcElemSrcBox = NULL;
							float minX, minY, maxX, maxY;
							Box uv_minmax = { 0 };
							Box box;
							GetBoundingBoxUVs(instruction, currentIndexLocation, &minX, &minY, &maxX, &maxY,
								&uv_minmax.x0, &uv_minmax.y0, &uv_minmax.x1, &uv_minmax.y1);
							InGameToScreenCoords(left, top, width, height, minX, minY, &box.x0, &box.y0);
							InGameToScreenCoords(left, top, width, height, maxX, maxY, &box.x1, &box.y1);
							// Store the pixel coordinates
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
							dcSrcBox->bLimitsComputed = true;

							/*
							// Store the pixel coordinates
							dcSrcBox = &g_DCHUDRegions.boxes[TEXT_RADIOSYS_BOX_IDX];
							dcSrcBox->coords = box;
							// Compute and store the erase coordinates for this HUD Box
							ComputeCoordsFromUV(left, top, width, height, uv_minmax, box,
								dcSrcBox->uv_erase_coords, &dcSrcBox->erase_coords);
							dcSrcBox->bLimitsComputed = true;
							*/

							//log_debug("[DBG] [DC] Top Right Region captured");

							// Get the limits for Name & Time
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[NAME_TIME_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for Number of Crafts -- Countermeasures
							// When I wrote this, I *thought* this was the number of ships; but (much) later I realized this
							// is wrong: this is the number of countermeasures!
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[NUM_CRAFTS_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							// Get the limits for Text Line 3
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[KW_TEXT_RADIOSYS_DC_ELEM_SRC_IDX];
							dcElemSrcBox->coords = ComputeCoordsFromUV(left, top, width, height,
								uv_minmax, box, dcElemSrcBox->uv_coords);
							dcElemSrcBox->bComputed = true;

							/*
							log_debug("[DBG] [DC] Text Line 3 coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
								g_fCurScreenWidth * dcElemSrcBox->coords.x0, g_fCurScreenHeight * dcElemSrcBox->coords.y0,
								g_fCurScreenWidth * dcElemSrcBox->coords.x1, g_fCurScreenHeight * dcElemSrcBox->coords.y1);
							*/

							// DCElemSrcBox dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[NUM_CRAFTS_DC_ELEM_SRC_IDX]
							/*log_debug("[DBG] [DC] Countermeasures coords: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
								g_fCurScreenWidth * dcElemSrcBox->coords.x0, g_fCurScreenHeight * dcElemSrcBox->coords.y0,
								g_fCurScreenWidth * dcElemSrcBox->coords.x1, g_fCurScreenHeight * dcElemSrcBox->coords.y1);*/
						}
					}
				}

				// Dynamic Cockpit: Remove all the alpha overlays in hi-res mode
				if (g_bDCManualActivate && g_bDynCockpitEnabled &&
					bLastTextureSelectedNotNULL && lastTextureSelected->is_DynCockpitAlphaOverlay)
					goto out;

				// Avoid rendering explosions on the CMD if we're rendering edges.
				// This didn't make much difference: the real problem is that the explosions and smoke aren't doing
				// correct alpha blending, so we see the square edges overlapping even without an edge detector.
				/*
				if (g_bEdgeDetectorEnabled && g_bTargetCompDrawn && bLastTextureSelectedNotNULL &&
					(lastTextureSelected->is_Explosion || lastTextureSelected->is_Electricity || lastTextureSelected->is_Spark || lastTextureSelected->is_Smoke)) {
					goto out;
				}
				*/

				// Reticle processing
				//if (!g_bYCenterHasBeenFixed && bIsReticleCenter && !bExternalCamera)
				if (bIsReticleCenter)
				{
					Vector2 ReticleCentroid;
					if (ComputeCentroid2D(instruction, currentIndexLocation, &ReticleCentroid)) {
						g_ReticleCentroid = ReticleCentroid;
						// The reticle centroid can be used to compute y_center:
						if (!bExternalCamera && !g_bYCenterHasBeenFixed) {
							// Force the re-application of the focal_length, this will make use the reticle centroid
							// to compute a new y_center.
							g_bCustomFOVApplied = false;
						}
						// DEBUG
						/*{
							float x, y;
							InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
								(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height,
								g_ReticleCentroid.x, g_ReticleCentroid.y, &x, &y);
							log_debug("[DBG] Reticle Centroid: %0.3f, %0.3f, Screen: %0.3f, %0.3f", g_ReticleCentroid.x, g_ReticleCentroid.y, x, y);
						}*/
						// DEBUG
					}
				}

				// Hide the text boxes for the X-Wing (this was a little experiment to see if this is possible)
				/*
				if (g_bToggleSkipDC && bLastTextureSelectedNotNULL &&
					((strstr(lastTextureSelected->_surface->_name, "TEX00150") != NULL) ||
					 (strstr(lastTextureSelected->_surface->_name, "TEX00151") != NULL) ||
					 (strstr(lastTextureSelected->_surface->_name, "TEX00129") != NULL)
				    )
				   )
				{
					goto out;
				}
				*/

				// DEBUG
				//if (bLastTextureSelectedNotNULL && lastTextureSelected->is_3DSun)
				//	goto out;
				// DEBUG
				
				// Active Cockpit: Intersect the current texture with the controller
				if (g_bActiveCockpitEnabled && bLastTextureSelectedNotNULL &&
					(bIsActiveCockpit || bIsCockpit && g_bFullCockpitTest && !bIsHologram))
				{
					Vector3 orig, dir, v0, v1, v2, P;
					//bool debug = false;
					//bool bIntersection;
					//log_debug("[DBG] [AC] Testing for intersection...");
					//if (bIsActiveCockpit) log_debug("[DBG] [AC] Testing %s", lastTextureSelected->_surface->_name);
					
					// DEBUG
					/*
					if (strstr(lastTextureSelected->_surface->_name, "TEX00061") != NULL &&
						strstr(lastTextureSelected->_surface->_name, "AwingCockpit") != NULL) {
						debug = g_bDumpSSAOBuffers;
						if (debug)
							log_debug("[DBG] [AC] %s is being tested for inters", lastTextureSelected->_surface->_name);
					}
					*/
					// DEBUG

					orig.x = g_contOriginViewSpace.x;
					orig.y = g_contOriginViewSpace.y;
					orig.z = g_contOriginViewSpace.z;

					dir.x = g_contDirViewSpace.x;
					dir.y = g_contDirViewSpace.y;
					dir.z = g_contDirViewSpace.z;

					//bool debug = g_bDumpLaserPointerDebugInfo && (strstr(lastTextureSelected->_surface->_name, "AwingCockpit.opt,TEX00080,color") != NULL);
					IntersectWithTriangles(instruction, currentIndexLocation, lastTextureSelected->ActiveCockpitIdx, 
						bIsActiveCockpit, orig, dir /*, debug */);

					// Commented block follows (debug block for LaserPointer):
					{
						//if (bIntersection) {
							//Vector3 pos2D;

							//if (t < g_fBestIntersectionDistance)
							//{

								//g_fBestIntersectionDistance = t;
								//g_LaserPointer3DIntersection = P;
								// Project to 2D
								//pos2D = project(g_LaserPointer3DIntersection);
								//g_LaserPointerBuffer.intersection[0] = pos2D.x;
								//g_LaserPointerBuffer.intersection[1] = pos2D.y;
								//g_LaserPointerBuffer.uv[0] = u;
								//g_LaserPointerBuffer.uv[1] = v;
								//g_LaserPointerBuffer.bIntersection = 1;
								//g_debug_v0 = v0;
								//g_debug_v1 = v1;
								//g_debug_v2 = v2;

								// DEBUG
								//{
									/*Vector3 q;
									q = project(v0); g_LaserPointerBuffer.v0[0] = q.x; g_LaserPointerBuffer.v0[1] = q.y;
									q = project(v1); g_LaserPointerBuffer.v1[0] = q.x; g_LaserPointerBuffer.v1[1] = q.y;
									q = project(v2); g_LaserPointerBuffer.v2[0] = q.x; g_LaserPointerBuffer.v2[1] = q.y;*/
									/*
									log_debug("[DBG] [AC] Intersection: (%0.3f, %0.3f, %0.3f) --> (%0.3f, %0.3f)",
										g_LaserPointer3DIntersection.x, g_LaserPointer3DIntersection.y, g_LaserPointer3DIntersection.z,
										pos2D.x, pos2D.y);
									*/
									//}
									// DEBUG
								//}
							//}
					}
				}

				//if (bIsNoZWrite && _renderStates->GetZFunc() == D3DCMP_GREATER) {
				//	goto out;
					//log_debug("[DBG] NoZWrite, ZFunc: %d", _renderStates->GetZFunc());
				//}


				// Skip specific draw calls for debugging purposes.
#ifdef DBG_VR
				if (!bZWriteEnabled)
					g_iNonZBufferCounter++;
				if (!bZWriteEnabled && g_iSkipNonZBufferDrawIdx > -1 && g_iNonZBufferCounter >= g_iSkipNonZBufferDrawIdx)
					goto out;

				//if (bIsText && g_bSkipText)
				//	goto out;

				// Skip the draw calls after the targetting computer has been drawn?
				//if (bIsFloating3DObject && g_bSkipAfterTargetComp)
				//	goto out;

				if (bIsSkyBox && g_bSkipSkyBox)
					goto out;

				if (g_bStartedGUI && g_bSkipGUI)
					goto out;
				// Engine glow:
				//if (bIsNoZWrite && bLastTextureSelectedNotNULL && g_bSkipGUI)
				//	goto out;

				/* if (bIsBracket) {
					log_debug("[DBG] ZEnabled: %d, ZFunc: %d", this->_renderStates->GetZEnabled(),
						this->_renderStates->GetZFunc());
				} */

				// DBG HACK: Skip draw calls after the HUD has started rendering
				if (g_iDrawCounterAfterHUD > -1) {
					if (g_iNoDrawAfterHUD > -1 && g_iDrawCounterAfterHUD > g_iNoDrawAfterHUD)
						goto out;
				}
#endif

				// We will be modifying the regular render state from this point on. The state and the Pixel/Vertex
				// shaders are already set by this point; but if we modify them, we'll set bModifiedShaders to true
				// so that we can restore the state at the end of the draw call.
				bModifiedShaders      = false;
				bModifiedPixelShader  = false;
				bModifiedBlendState   = false;
				bModifiedSamplerState = false;

				// Skip rendering light textures in VR or bind the light texture if we're rendering the color tex
#ifdef DISABLED
				if (false && g_bEnableVR && bLastTextureSelectedNotNULL) {
					if (bIsLightTexture)
						goto out;
					else {
						// Bind the light texture too
						if (lastTextureSelected->lightTexture != nullptr) {
							bModifiedShaders = true;
							lastTextureSelected->lightTexture->_refCount++;
							context->PSSetShaderResources(1, 1, lastTextureSelected->lightTexture->_textureView.GetAddressOf());
							lastTextureSelected->lightTexture->_refCount--;
							g_PSCBuffer.bLightTextureAvailable = 1;
						}
					}
				}
#endif DISABLED

				// DEBUG
				//if (bIsActiveCockpit && strstr(lastTextureSelected->_surface->_name, "AwingCockpit.opt,TEX00080,color") != NULL)
				/*if (bIsActiveCockpit && lastTextureSelected->ActiveCockpitIdx == 9)
				{
					bModifiedShaders = true;
					g_PSCBuffer.AC_debug = 1;
				}*/
				// DEBUG

				//if (g_bDynCockpitEnabled && !g_bPrevIsFloatingGUI3DObject && g_bIsFloating3DObject) {
					// The targeted craft is about to be drawn!
					// Let's clear the render target view for the dynamic cockpit
					//float bgColor[4] = { 0.1f, 0.1f, 0.3f, 0.0f };
					//context->ClearRenderTargetView(resources->_renderTargetViewDynCockpit, bgColor);
					//context->ClearDepthStencilView(this->_deviceResources->_depthStencilViewL, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
				//}

				// _offscreenAsInputSRVDynCockpit is resolved in PrimarySurface.cpp, right before we
				// present the backbuffer. That prevents resolving the texture multiple times (and we
				// also don't have to resolve it here).

				// If we're rendering 3D contents in the Tech Room and some form of SSAO is enabled, 
				// then disable the pre-computed diffuse component:
				//if (g_bAOEnabled && !g_bRendering3D) {
				// Only disable the diffuse component during regular flight. 
				// The tech room is unchanged (the tech room makes g_bRendering = false)
				// We should also avoid touching the GUI elements
				// When the Death Star is destroyed s_XwaGlobalLightsCount becomes 0, we can use the original illumination in that case.
				if (/* (*s_XwaGlobalLightsCount == 0) || */
					(g_bRendering3D && g_bDisableDiffuse && !g_bStartedGUI && !g_bIsTrianglePointer)) {
					bModifiedShaders = true;
					g_PSCBuffer.fDisableDiffuse = 1.0f;
				}

				if (bIsXWAHangarShadow) {
					// For hangar shadows we need to change the blending operation (it's set to force alpha = 1)
					// EDIT (after a few months) I'm not sure the fix below works very well. This was paired with
					// an alpha of 0.25 in PixelShaderSolid, but that caused overlapping shadows to look darker.
					// I changed the blend op to min and max and that only caused shadows to go full black. I ended
					// up disabling this block and using the original settings -- except I changed the color of the
					// shadow to black (because otherwise it's rendered blue). See PixelShaderSolid.hlsl.
					/*
					D3D11_BLEND_DESC blendDesc{};
					blendDesc.AlphaToCoverageEnable = FALSE;
					blendDesc.IndependentBlendEnable = FALSE;
					blendDesc.RenderTarget[0].BlendEnable = TRUE;
					blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
					blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
					//blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
					blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MIN;
					blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
					blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
					blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
					blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
					hr = resources->InitBlendState(nullptr, &blendDesc);
					*/

					//D3D11_BLEND_DESC desc = this->_renderStates->GetBlendDesc();
					/*
					log_debug("[DBG] ******************************");
					log_debug("[DBG] BlendEnable: %d, BlendOp: %d, BlendOpAlpha: %d", 
						desc.RenderTarget->BlendEnable, desc.RenderTarget->BlendOp, desc.RenderTarget->BlendOpAlpha);
					log_debug("[DBG] SrcBlend: %d, SrcBlendAlpha: %d",
						desc.RenderTarget->SrcBlend, desc.RenderTarget->SrcBlendAlpha);
					log_debug("[DBG] DestBlend: %d, DestBlendAlpha: %d",
						desc.RenderTarget->DestBlend, desc.RenderTarget->DestBlendAlpha);
					log_debug("[DBG] ******************************");*/
					/*
					This is the blend state for hangar shadows (they are forced to be alpha 1):
					[16360] [DBG] BlendEnable: 1, BlendOp: 1, BlendOpAlpha: 1
					[16360] [DBG] SrcBlend: 5, SrcBlendAlpha: 2
					[16360] [DBG] DestBlend: 6, DestBlendAlpha: 1
					*/
					//D3D11_BLEND_OP_ADD == 1
					//D3D11_BLEND_ZERO == 1
					//D3D11_BLEND_ONE == 2
					//D3D11_BLEND_SRC_ALPHA == 5
					//D3D11_BLEND_INV_SRC_ALPHA == 6
					bModifiedShaders = true;
					g_PSCBuffer.special_control = SPECIAL_CONTROL_XWA_SHADOW;
				}

				if (bLastTextureSelectedNotNULL && lastTextureSelected->is_Smoke) {
					bModifiedShaders = true;
					//EnableTransparency();
					g_PSCBuffer.special_control = SPECIAL_CONTROL_SMOKE;
				}

				if (bIsDS2CoreExplosion) {
					g_iReactorExplosionCount++;
					// The reactor's core explosion is rendered 4 times per frame. We only need one now:
					if (g_iReactorExplosionCount > 1)
						goto out;

					static float iTime = 0.0f;
					iTime += 0.05f;
					/*
					static float ExplosionFrameCounter = 0.0f;
					float selector = ExplosionFrameCounter / 3.0f;
					if (selector > 1.0f) selector = 1.0f;
					// A scale of 4.0 is small, a scale of 2.0 is normal and 1.0 is large:
					float ExplosionScale = lerp(4.0f, 2.0f, selector);
					ExplosionFrameCounter += 1.0f;
					*/

					bModifiedShaders = true;
					bModifiedPixelShader = true;
					bModifiedSamplerState = true;
					resources->InitPixelShader(resources->_explosionPS);
					// Set the noise texture and sampler state with wrap/repeat enabled.
					context->PSSetShaderResources(1, 1, resources->_grayNoiseSRV.GetAddressOf());
					// bModifiedSamplerState restores this sampler state at the end of this instruction.
					context->PSSetSamplers(1, 1, resources->_repeatSamplerState.GetAddressOf());
					
					g_ShadertoyBuffer.iTime = iTime;
					//g_ShadertoyBuffer.iResolution[0] = lastTextureSelected->material.LavaSize;
					g_ShadertoyBuffer.iResolution[1] = lastTextureSelected->material.EffectBloom;
					g_ShadertoyBuffer.bDisneyStyle = false; // AlphaBlendEnabled: Do not blend the explosion with the original texture
					g_ShadertoyBuffer.tunnel_speed = 1.0f; // ExplosionTime: Always set to 1 -- the animation is performed by iTime in VolumetricExplosion()
					//g_ShadertoyBuffer.twirl = ExplosionScale; // 2.0 is the normal size, 4.0 is small, 1.0 is big.
					g_ShadertoyBuffer.twirl = 2.0f; // ExplosionScale: 2.0 is the normal size, 4.0 is small, 1.0 is big.
					// Set the constant buffer
					resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
				}

				// Render the procedural explosions
				if (bIsExplosion && bHasMaterial && lastTextureSelected->material.ExplosionBlendMode > 0)
				{
					static float iTime = 0.0f;
					//iTime += 0.05f;
					iTime += lastTextureSelected->material.ExplosionSpeed;

					bModifiedShaders = true;
					bModifiedPixelShader = true;
					bModifiedSamplerState = true;
					resources->InitPixelShader(resources->_explosionPS);
					// Set the noise texture and sampler state with wrap/repeat enabled.
					context->PSSetShaderResources(1, 1, resources->_grayNoiseSRV.GetAddressOf());
					// bModifiedSamplerState restores this sampler state at the end of this instruction.
					context->PSSetSamplers(1, 1, resources->_repeatSamplerState.GetAddressOf());

					int GroupId = 0, ImageId = 0;
					if (!lastTextureSelected->material.DATGroupImageIdParsed) {
						// TODO: Maybe I can extract the group Id and image Id from the DAT's name during tagging and
						// get rid of the DATGroupImageIdParsed field in the material property.
						GetGroupIdImageIdFromDATName(lastTextureSelected->_surface->_name, &GroupId, &ImageId);
						lastTextureSelected->material.GroupId = GroupId;
						lastTextureSelected->material.ImageId = ImageId;
						lastTextureSelected->material.DATGroupImageIdParsed = true;
					}
					else {
						GroupId = lastTextureSelected->material.GroupId;
						ImageId = lastTextureSelected->material.ImageId;
					}
					// TODO: The following time will increase in steps, but I'm not sure it's possible to do a smooth
					// increase because there's no way to uniquely identify two different explosions on the same frame.
					// The same GroupId can appear mutiple times on the screen belonging to different craft and they
					// may be at different stages of the animation.
					float ExplosionTime = min(1.0f, (float)ImageId / (float)lastTextureSelected->material.TotalFrames);
					//log_debug("[DBG] Explosion Id: %d, Frame: %d, TotalFrames: %d, Time: %0.3f",
					//	GroupId, ImageId, lastTextureSelected->material.TotalFrames, ExplosionTime);
					//log_debug("[DBG] Explosion Id: %d", GroupId);

					g_ShadertoyBuffer.iTime = iTime;
					// ExplosionBlendMode:
					// 0: Original texture, 
					// 1: Blend with procedural explosion, 
					// 2: Use procedural explosions only
					g_ShadertoyBuffer.bDisneyStyle = lastTextureSelected->material.ExplosionBlendMode; // AlphaBlendEnabled: true blend with original texture, false: replace original texture
					//g_ShadertoyBuffer.tunnel_speed = lerp(4, -1, ExplosionTime); // ExplosionTime: 4..-1 The animation is performed by iTime in VolumetricExplosion()
					g_ShadertoyBuffer.tunnel_speed = lerp(3.0f, -1.0f, ExplosionTime); // ExplosionTime: 4..-1 The animation is performed by iTime in VolumetricExplosion()
					// ExplosionScale: 4.0 == small, 2.0 == normal, 1.0 == big.
					// The value from ExplosionScale is translated from user-facing units to shader units in the ReadMaterialLine() function
					g_ShadertoyBuffer.twirl = lastTextureSelected->material.ExplosionScale;
					// Set the constant buffer
					resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
				}

				// Capture the centroid of the current sun texture and store it.
				// Sun Centroids appear to be around 50m away in metric 3D space
				if (bIsSun) 
				{
					// Get the centroid of the current sun. The 2D centroid may suffer from perspective-incorrect interpolation
					// so it jumps a little when the containing polygons are clipped near the edges of the screen.
					// TODO: SunCentroid2D is not useful because the centroid jumps due to perspective-incorrect interpolation.
					//		 Remove it later...
					bSunCentroidComputed = ComputeCentroid(instruction, currentIndexLocation, &SunCentroid, &SunCentroid2D);
					if (bSunCentroidComputed && g_iNumSunCentroids < MAX_XWA_LIGHTS) {
						//if (g_bEnableVR) SunCentroid.y = -SunCentroid.y;
						// Looks like don't get multiple centroids for the same Sun. Seems to be a 1-1 correspondence
						g_SunCentroids[g_iNumSunCentroids++] = SunCentroid;
						//if (g_iNumSunCentroids == 1)
						//	log_debug("[DBG] [SHW] SunCentroid: %0.3f, %0.3f",
						//		SunCentroid2D.x, SunCentroid2D.y);
					}
				}

				// Replace the sun textures with procedurally-generated suns
				if (g_bProceduralSuns && !g_b3DSunPresent && !g_b3DSkydomePresent && bIsSun &&
					g_ShadertoyBuffer.SunFlareCount < MAX_SUN_FLARES) 
				{
					static float iTime = 0.0f;
					//Matrix4 H = GetCurrentHeadingViewMatrix();

					bModifiedShaders = true;
					// Change the pixel shader to the SunShader:
					bModifiedPixelShader = true;
					resources->InitPixelShader(resources->_sunShaderPS);
					// The Sun's texture will be displayed, so let's update some values
					g_PSCBuffer.fBloomStrength = g_BloomConfig.fSunsStrength;
					g_ShadertoyBuffer.iTime = iTime;
					iTime += 0.01f;

					// The 3D centroid of the current sun should be computed already, so let's just use it
					int SunFlareIdx = g_ShadertoyBuffer.SunFlareCount;
					// By default suns don't have any color. We specify that by setting the alpha component to 0:
					g_ShadertoyBuffer.SunColor[SunFlareIdx].w = 0.0f;
					// Use the material properties of this Sun -- if it has any associated with it
					if (bHasMaterial) {
						g_ShadertoyBuffer.SunColor[SunFlareIdx].x = lastTextureSelected->material.Light.x;
						g_ShadertoyBuffer.SunColor[SunFlareIdx].y = lastTextureSelected->material.Light.y;
						g_ShadertoyBuffer.SunColor[SunFlareIdx].z = lastTextureSelected->material.Light.z;
						// We have a color for this sun, let's set w to 1 to signal that
						g_ShadertoyBuffer.SunColor[SunFlareIdx].w = 1.0f;
					}

					//if (ComputeCentroid(instruction, currentIndexLocation, &SunCentroid))
					if (bSunCentroidComputed)
					{
						// If the centroid is visible, then let's display the sun flare:
						g_ShadertoyBuffer.SunFlareCount++;
						if (g_bEnableVR) {
							//log_debug("[DBG] 3D centroid: %0.3f, %0.3f, %0.3f",
							//	SunCentroid.x, SunCentroid.y, SunCentroid.z);
							// In VR mode, we store the Metric 3D position of the Centroid
							g_ShadertoyBuffer.SunCoords[SunFlareIdx].x = SunCentroid.x;
							g_ShadertoyBuffer.SunCoords[SunFlareIdx].y = SunCentroid.y;
							g_ShadertoyBuffer.SunCoords[SunFlareIdx].z = SunCentroid.z;
							g_ShadertoyBuffer.VRmode = 1;
							// DEBUG
							// Project the centroid to the left image and log the coords
							//Vector3 q = project(Centroid, g_viewMatrix, g_FullProjMatrixLeft);
							//log_debug("[DBG] Centroid: %0.3f, %0.3f, %0.3f --> %0.3f, %0.3f",
							//	Centroid.x, Centroid.y, Centroid.z, q.x, q.y);
							// DEBUG
						}
						
						{
							// In regular mode, we store the 2D screen coords of the centroid
							float X, Y;
							Vector3 q = projectToInGameOrPostProcCoordsMetric(SunCentroid, g_viewMatrix, g_FullProjMatrixLeft, true);
							
							// Store the in-game 2D centroid coords for later use, during sun flare rendering
							g_SunCentroids2D[SunFlareIdx].x = q.x;
							g_SunCentroids2D[SunFlareIdx].y = q.y;

							if (!g_bEnableVR) {
								InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
									(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, q.x, q.y, &X, &Y);
								g_ShadertoyBuffer.SunCoords[SunFlareIdx].x = X;
								g_ShadertoyBuffer.SunCoords[SunFlareIdx].y = Y;
								g_ShadertoyBuffer.SunCoords[SunFlareIdx].z = 0.0f;
								g_ShadertoyBuffer.VRmode = 0;
							}
						}
					}
					// Set the constant buffer
					resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
				}

				if (g_bProceduralLava && bLastTextureSelectedNotNULL && bHasMaterial && lastTextureSelected->material.IsLava)
				{
					// lastT is not properly initialized on the very first frame; but nothing much seems
					// to happen. So: ignoring for now.
					static LARGE_INTEGER curT, lastT, elapsed_us;
					QueryPerformanceCounter(&curT);
					elapsed_us.QuadPart = curT.QuadPart - lastT.QuadPart;
					elapsed_us.QuadPart *= 1000000;
					elapsed_us.QuadPart /= g_PC_Frequency.QuadPart;

					float elapsed_s = ((float)elapsed_us.QuadPart / 1000000.0f);
					//log_debug("[DBG] elapsed_us.Q: %llu, elapsed_s: %0.6f", elapsed_us.QuadPart, elapsed_s);
					static float iTime = 0.0f;
					//iTime += 0.01f * lastTextureSelected->material.LavaSpeed;
					iTime += elapsed_s * lastTextureSelected->material.LavaSpeed;
					lastT = curT;

					bModifiedShaders = true;
					bModifiedPixelShader = true;
					bModifiedSamplerState = true;

					g_ShadertoyBuffer.iTime = iTime;
					g_ShadertoyBuffer.bDisneyStyle = lastTextureSelected->material.LavaTiling;
					g_ShadertoyBuffer.iResolution[0] = lastTextureSelected->material.LavaSize;
					g_ShadertoyBuffer.iResolution[1] = lastTextureSelected->material.EffectBloom;
					// SunColor[0] --> Color
					g_ShadertoyBuffer.SunColor[0].x = lastTextureSelected->material.LavaColor.x;
					g_ShadertoyBuffer.SunColor[0].y = lastTextureSelected->material.LavaColor.y;
					g_ShadertoyBuffer.SunColor[0].z = lastTextureSelected->material.LavaColor.z;
					/*
					// SunColor[1] --> LavaNormalMult
					g_ShadertoyBuffer.SunColor[1].x = lastTextureSelected->material.LavaNormalMult.x;
					g_ShadertoyBuffer.SunColor[1].y = lastTextureSelected->material.LavaNormalMult.y;
					g_ShadertoyBuffer.SunColor[1].z = lastTextureSelected->material.LavaNormalMult.z;
					// SunColor[2] --> LavaPosMult
					g_ShadertoyBuffer.SunColor[2].x = lastTextureSelected->material.LavaPosMult.x;
					g_ShadertoyBuffer.SunColor[2].y = lastTextureSelected->material.LavaPosMult.y;
					g_ShadertoyBuffer.SunColor[2].z = lastTextureSelected->material.LavaPosMult.z;

					g_ShadertoyBuffer.bDisneyStyle = lastTextureSelected->material.LavaTranspose;
					*/

					resources->InitPixelShader(resources->_lavaPS);
					// Set the noise texture and sampler state with wrap/repeat enabled.
					context->PSSetShaderResources(1, 1, resources->_grayNoiseSRV.GetAddressOf());
					// bModifiedSamplerState restores this sampler state at the end of this instruction.
					context->PSSetSamplers(1, 1, resources->_repeatSamplerState.GetAddressOf());

					// Set the constant buffer
					// TODO (?): Set the g_PSCBuffer
					resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
				}

				// Do not render pos3D or normal outputs for specific objects (used for SSAO)
				// If these outputs are not disabled, then the aiming HUD gets AO as well!
				if (g_bStartedGUI || g_bIsSkyBox || bIsBracket /* || bIsSkyDome */) {
					bModifiedShaders = true;
					g_PSCBuffer.fPosNormalAlpha = 0.0f;
					g_PSCBuffer.bIsShadeless = 1;
					//g_PSCBuffer.bIsBackground = g_bIsSkyBox;
				}

				// Dim all the GUI elements
				if (g_bStartedGUI && !g_bIsFloating3DObject) {
					bModifiedShaders = true;
					g_PSCBuffer.brightness = g_fBrightness;
				}

				// For some reason, the DS2 reactor core explosion is confused with the SkyBox... fun times!
				if (g_bIsSkyBox && !bIsDS2CoreExplosion) {
					bModifiedShaders = true;
					// DEBUG: Get a sample of how the vertexbuffer for the skybox looks like
					//DisplayCoords(instruction, currentIndexLocation);
					// DEBUG

					// If we make the skybox a bit bigger to enable roll, it "swims" -- it's probably not going to work.
					//g_VSCBuffer.viewportScale[3] = g_fGlobalScale + 0.2f;
					// Send the skybox to infinity:
					g_VSCBuffer.sz_override = 0.01f;
					g_VSCBuffer.mult_z_override = 5000.0f; // Infinity is probably at 65535, we can probably multiply by something bigger here.
					g_PSCBuffer.bIsShadeless = 1;
					g_PSCBuffer.special_control = SPECIAL_CONTROL_BACKGROUND;
					// Suns are pushed to infinity too:
					//if (bIsSun) log_debug("[DBG] Sun pushed to infinity");
				}

				// Apply specific material properties for the current texture
				if (bHasMaterial) {
					bModifiedShaders = true;
					// DEBUG
					/*
					bool Debug = strstr(lastTextureSelected->_surface->_name, "CalamariLulsa") != NULL;
					//if (strstr(lastTextureSelected->_surface->_name, "TieInterceptor") != NULL) {
					if (Debug)
					{
						log_debug("[DBG] [MAT] Applying: [%s], %0.3f, %0.3f, %0.3f, %d",
							lastTextureSelected->_surface->_name,
							lastTextureSelected->material.Metallic,
							lastTextureSelected->material.Glossiness,
							lastTextureSelected->material.Intensity,
							lastTextureSelected->material.IsShadeless
						);
					}
					*/
					// DEBUG

					if (lastTextureSelected->material.IsShadeless)
						g_PSCBuffer.fSSAOMaskVal = SHADELESS_MAT;
					else
						g_PSCBuffer.fSSAOMaskVal = lastTextureSelected->material.Metallic * 0.5f; // Metallicity is encoded in the range 0..0.5 of the SSAOMask
					g_PSCBuffer.fGlossiness  = lastTextureSelected->material.Glossiness;
					g_PSCBuffer.fSpecInt     = lastTextureSelected->material.Intensity;
					g_PSCBuffer.fNMIntensity = lastTextureSelected->material.NMIntensity;
					g_PSCBuffer.fSpecVal	 = lastTextureSelected->material.SpecValue;
					g_PSCBuffer.fAmbient	 = lastTextureSelected->material.Ambient;

					if (lastTextureSelected->material.AlphaToBloom) {
						bModifiedPixelShader = true;
						bModifiedShaders = true;
						resources->InitPixelShader(resources->_alphaToBloomPS);
						if (lastTextureSelected->material.NoColorAlpha)
							g_PSCBuffer.special_control = SPECIAL_CONTROL_NO_COLOR_ALPHA;
						g_PSCBuffer.fBloomStrength = lastTextureSelected->material.EffectBloom;
					}

					if (lastTextureSelected->material.AlphaIsntGlass && !bIsLightTexture) {
						bModifiedPixelShader = true;
						bModifiedShaders = true;
						resources->InitPixelShader(resources->_noGlassPS);
					}
				}

				// Apply the SSAO mask/Special materials, like lasers and HUD
				//if (g_bAOEnabled && bLastTextureSelectedNotNULL) 
				if (bLastTextureSelectedNotNULL)
				{
					if (g_bIsScaleableGUIElem || bIsReticle || bIsText || g_bIsTrianglePointer || 
						lastTextureSelected->is_Debris || lastTextureSelected->is_GenericSSAOMasked ||
						lastTextureSelected->is_Electricity || lastTextureSelected->is_Explosion ||
						lastTextureSelected->is_Smoke)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fSSAOMaskVal = SHADELESS_MAT;
						g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
						g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
						g_PSCBuffer.fNMIntensity = 0.0f;
						g_PSCBuffer.fSpecVal     = 0.0f;
						g_PSCBuffer.bIsShadeless = 1;

						g_PSCBuffer.fPosNormalAlpha = 0.0f;

						// DEBUG
						//g_PSCBuffer.bIsBackground = bIsAimingHUD;
						// DEBUG
					} 
					else if (lastTextureSelected->is_Debris || lastTextureSelected->is_Trail ||
						lastTextureSelected->is_CockpitSpark || lastTextureSelected->is_Spark || 
						lastTextureSelected->is_Chaff || lastTextureSelected->is_Missile /* || lastTextureSelected->is_GenericSSAOTransparent */
						/* || (lastTextureSelected->is_Explosion && !g_bTransparentExplosions) */
						)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fSSAOMaskVal = PLASTIC_MAT;
						g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
						g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
						g_PSCBuffer.fNMIntensity = 0.0f;
						g_PSCBuffer.fSpecVal     = 0.0f;

						g_PSCBuffer.fPosNormalAlpha = 0.0f;
					}
					else if (lastTextureSelected->is_Laser) {
						bModifiedShaders = true;
						g_PSCBuffer.fSSAOMaskVal = EMISSION_MAT;
						g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
						g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
						g_PSCBuffer.fNMIntensity = 0.0f;
						g_PSCBuffer.fSpecVal     = 0.0f;
						g_PSCBuffer.bIsShadeless = 1;

						g_PSCBuffer.fPosNormalAlpha = 0.0f;
					}
				}

				// When exiting hyperspace, the light textures will overwrite the alpha component. Fixing this
				// requires changing the alpha blend state; but if I modify that, chances are something else will
				// break. So instead of fixing it, how about skipping those draw calls since it's only going
				// to be a few frames after exiting hyperspace.
				//if (g_HyperspacePhaseFSM != HS_INIT_ST && g_bIsPlayerObject && lastTextureSelected->is_LightTexture)
				//	goto out;
				// FIXED by using discard and setting alpha to 1 when DC is active

				// EARLY EXIT 1: Render the HUD/GUI to the Dynamic Cockpit RTVs and continue
				bool bRenderReticleToBuffer = g_bEnableVR && bIsReticle; // && !bExternalCamera;
				if (
					 (g_bDCManualActivate || bExternalCamera) && (g_bDynCockpitEnabled || g_bReshadeEnabled) &&
					 (bRenderToDynCockpitBuffer || bRenderToDynCockpitBGBuffer) || bRenderReticleToBuffer
				   )
				{					
					// Looks like we don't need to restore the blend/depth state???
					//D3D11_BLEND_DESC curBlendDesc = _renderStates->GetBlendDesc();
					//D3D11_DEPTH_STENCIL_DESC curDepthDesc = _renderStates->GetDepthStencilDesc();
					//if (!g_bPrevIsFloatingGUI3DObject && g_bIsFloating3DObject) {
					//	// The targeted craft is about to be drawn! Clear both depth stencils?
					//	context->ClearDepthStencilView(resources->_depthStencilViewL, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
					//}
					
					//log_debug("[DBG] RENDER to DC RTV");

					// Restore the non-VR dimensions:
					g_VSCBuffer.viewportScale[0] =  2.0f / displayWidth;
					g_VSCBuffer.viewportScale[1] = -2.0f / displayHeight;
					// Apply the brightness settings to the pixel shader
					g_PSCBuffer.brightness = g_fBrightness;
					resources->InitViewport(&g_nonVRViewport);
					resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
					resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
					// Set the original vertex buffer and dynamic cockpit RTV:
					resources->InitVertexShader(resources->_vertexShader);
					// Select the proper RTV
					if (bRenderReticleToBuffer) {
						context->OMSetRenderTargets(1, resources->_ReticleRTV.GetAddressOf(), NULL);
					}
					else if (bRenderToDynCockpitBGBuffer) {
						context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpitBG.GetAddressOf(),
							resources->_depthStencilViewL.Get());
					}
					else {
						//context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpit.GetAddressOf(),
						//	resources->_depthStencilViewL.Get());

						if (g_config.Text2DRendererEnabled)
							context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpit.GetAddressOf(),
								resources->_depthStencilViewL.Get());
						else 
						{
							// The new Text Renderer is not enabled; but we can still render the text to its own
							// layer so that we can do a more specialized processing, like splitting the text
							// from the targeted object
							if (bIsText)
								context->OMSetRenderTargets(1, resources->_DCTextRTV.GetAddressOf(),
									resources->_depthStencilViewL.Get());
							else
								context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpit.GetAddressOf(),
									resources->_depthStencilViewL.Get());
						}
					}
					// Enable Z-Buffer if we're drawing the targeted craft
					if (g_bIsFloating3DObject) {
						//log_debug("[DBG] Render 3D Floating OBJ to DC RTV");
						QuickSetZWriteEnabled(TRUE);
					}

					// Render
					context->DrawIndexed(3 * instruction->wCount, currentIndexLocation, 0);
					g_iHUDOffscreenCommandsRendered++;

					// Restore the regular texture, RTV, shaders, etc:
					context->PSSetShaderResources(0, 1, lastTextureSelected->_textureView.GetAddressOf());
					context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(),
						resources->_depthStencilViewL.Get());
					if (g_bEnableVR) {
						resources->InitVertexShader(resources->_sbsVertexShader);
						// Restore the right constants in case we're doing VR rendering
						g_VSCBuffer.viewportScale[0] = 1.0f / displayWidth;
						g_VSCBuffer.viewportScale[1] = 1.0f / displayHeight;
						resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
					}
					else {
						resources->InitVertexShader(resources->_vertexShader);
					}
					// Restore the Pixel Shader constant buffers:
					g_PSCBuffer.brightness = MAX_BRIGHTNESS;
					resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
					// Restore the previous pixel shader
					//bModifiedPixelShader = true;
					//resources->InitPixelShader(lastPixelShader);

					// Restore the original blend state
					/*if (g_bIsFloating3DObject)
						hr = resources->InitBlendState(nullptr, &curBlendDesc);*/
					goto out;
				}

				// Modify the state for both VR and regular game modes...

				// Maintain the k-closest lasers to the camera
				if (g_bEnableLaserLights && bIsLaser && bHasMaterial)
					AddLaserLights(instruction, currentIndexLocation, lastTextureSelected);

				// Apply BLOOM flags and 32-bit mode enhancements
				if (bLastTextureSelectedNotNULL)
				{
					if (bIsLaser) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = lastTextureSelected->is_Laser ?
							g_BloomConfig.fLasersStrength : g_BloomConfig.fTurboLasersStrength;
						g_PSCBuffer.bIsLaser = g_config.EnhanceLasers ? 2 : 1;
					}
					// Send the flag for light textures (enhance them in 32-bit mode, apply bloom)
					else if (bIsLightTexture) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = lastTextureSelected->is_CockpitTex ?
							g_BloomConfig.fCockpitStrength : g_BloomConfig.fLightMapsStrength;
						g_PSCBuffer.bIsLightTexture = g_config.EnhanceIllumination ? 2 : 1;
					}
					// Set the flag for EngineGlow and Explosions (enhance them in 32-bit mode, apply bloom)
					else if (lastTextureSelected->is_EngineGlow) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fEngineGlowStrength;
						g_PSCBuffer.bIsEngineGlow = g_config.EnhanceEngineGlow ? 2 : 1;
					}
					else if (lastTextureSelected->is_Electricity || bIsExplosion)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fExplosionsStrength;
						g_PSCBuffer.bIsEngineGlow = g_config.EnhanceExplosions ? 2 : 1;
					}
					else if (lastTextureSelected->is_LensFlare) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fLensFlareStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (bIsSun) {
						bModifiedShaders = true;
						// If there's a 3D sun in the scene, then we shouldn't apply bloom to Sun textures  they should be invisible 
						g_PSCBuffer.fBloomStrength = g_b3DSunPresent ? 0.0f : g_BloomConfig.fSunsStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (lastTextureSelected->is_Spark) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fSparksStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (lastTextureSelected->is_CockpitSpark) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fCockpitSparksStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (lastTextureSelected->is_Chaff)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fSparksStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (lastTextureSelected->is_Missile)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fMissileStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (lastTextureSelected->is_SkydomeLight) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fSkydomeLightStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}

					// Remove Bloom for all textures with materials tagged as "NoBloom"
					if (bHasMaterial && lastTextureSelected->material.NoBloom)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = 0.0f;
						g_PSCBuffer.bIsEngineGlow = 0;
					}
				}

				// Apply BLOOM flags for textureless objects
				if (bIsBracket) {
					bModifiedShaders = true;
					g_PSCBuffer.fBloomStrength = g_BloomConfig.fBracketStrength;
				}

				// Transparent textures are currently used with DC to render floating text. However, if the erase region
				// commands are being ignored, then this will cause the text messages to be rendered twice. To avoid
				// having duplicated messages, we're removing these textures here when the erase_region commmands are
				// not being applied.
				if (g_bDCManualActivate && g_bDynCockpitEnabled && bIsTransparent && !g_bDCApplyEraseRegionCommands)
					goto out;

				// Dynamic Cockpit: Replace textures at run-time:
				if (g_bDCManualActivate && g_bDynCockpitEnabled && bLastTextureSelectedNotNULL && lastTextureSelected->is_DynCockpitDst)
				{
					int idx = lastTextureSelected->DCElementIndex;
					//log_debug("[DBG] Replacing cockpit textures with DC");

					// Check if this idx is valid before rendering
					if (idx >= 0 && idx < g_iNumDCElements) {
						dc_element *dc_element = &g_DCElements[idx];
						if (dc_element->bActive) {
							bModifiedShaders = true;
							g_PSCBuffer.fBloomStrength = g_BloomConfig.fCockpitStrength;
							int numCoords = 0;
							for (int i = 0; i < dc_element->coords.numCoords; i++)
							{
								int src_slot = dc_element->coords.src_slot[i];
								// Skip invalid src slots
								if (src_slot < 0)
									continue;

								if (src_slot >= (int)g_DCElemSrcBoxes.src_boxes.size()) {
									//log_debug("[DBG] [DC] src_slot: %d bigger than src_boxes.size! %d",
									//	src_slot, g_DCElemSrcBoxes.src_boxes.size());
									continue;
								}

								DCElemSrcBox *src_box = &g_DCElemSrcBoxes.src_boxes[src_slot];
								// Skip src boxes that haven't been computed yet
								if (!src_box->bComputed)
									continue;

								uvfloat4 uv_src;
								uv_src.x0 = src_box->coords.x0; uv_src.y0 = src_box->coords.y0;
								uv_src.x1 = src_box->coords.x1; uv_src.y1 = src_box->coords.y1;
								g_DCPSCBuffer.src[numCoords] = uv_src;
								g_DCPSCBuffer.dst[numCoords] = dc_element->coords.dst[i];
								g_DCPSCBuffer.noisy_holo = bIsNoisyHolo;
								g_DCPSCBuffer.transparent = bIsTransparent;
								if (bWarheadLocked)
									g_DCPSCBuffer.bgColor[numCoords] = dc_element->coords.uWHColor[i];
								else
									g_DCPSCBuffer.bgColor[numCoords] = bIsTargetHighlighted ? dc_element->coords.uHGColor[i] : dc_element->coords.uBGColor[i];
								// The hologram property will make *all* uvcoords in this DC element
								// holographic as well:
								//bIsHologram |= (dc_element->bHologram);
								numCoords++;
							} // for
							// g_bDCHologramsVisible is a hard switch, let's use g_fDCHologramFadeIn instead to
							// provide a softer ON/OFF animation
							if (bIsHologram && g_fDCHologramFadeIn <= 0.01f) goto out;
							g_PSCBuffer.DynCockpitSlots = numCoords;
							//g_PSCBuffer.bUseCoverTexture = (dc_element->coverTexture != nullptr) ? 1 : 0;
							g_PSCBuffer.bUseCoverTexture = (resources->dc_coverTexture[idx] != nullptr) ? 1 : 0;
							
							// slot 0 is the cover texture
							// slot 1 is the HUD offscreen buffer
							// slot 2 is the text buffer
							context->PSSetShaderResources(1, 1, resources->_offscreenAsInputDynCockpitSRV.GetAddressOf());
							context->PSSetShaderResources(2, 1, resources->_DCTextSRV.GetAddressOf());
							// Set the cover texture:
							if (g_PSCBuffer.bUseCoverTexture) {
								//log_debug("[DBG] [DC] Setting coverTexture: 0x%x", resources->dc_coverTexture[idx].GetAddressOf());
								//context->PSSetShaderResources(0, 1, dc_element->coverTexture.GetAddressOf());
								//context->PSSetShaderResources(0, 1, &dc_element->coverTexture);
								context->PSSetShaderResources(0, 1, resources->dc_coverTexture[idx].GetAddressOf());
								//resources->InitPSShaderResourceView(resources->dc_coverTexture[idx].Get());
							} else
								context->PSSetShaderResources(0, 1, lastTextureSelected->_textureView.GetAddressOf());
								//resources->InitPSShaderResourceView(lastTextureSelected->_textureView.Get());
							// No need for an else statement, slot 0 is already set to:
							// context->PSSetShaderResources(0, 1, texture->_textureView.GetAddressOf());
							// See D3DRENDERSTATE_TEXTUREHANDLE, where lastTextureSelected is set.
							if (g_PSCBuffer.DynCockpitSlots > 0) {
								bModifiedPixelShader = true;
								//bModifiedBlendState = true;
								// Holograms require alpha blending to be enabled, but we also need to save the current
								// blending state so that it gets restored at the end of this draw call.
								//SaveBlendState();
								//EnableTransparency();
								if (bIsHologram) {
									uint32_t hud_color = (*g_XwaFlightHudColor) & 0x00FFFFFF;
									//log_debug("[DBG] hud_color, border, inside: 0x%x, 0x%x", *g_XwaFlightHudBorderColor, *g_XwaFlightHudColor);
									g_ShadertoyBuffer.iTime = g_fDCHologramTime;
									g_ShadertoyBuffer.twirl = g_fDCHologramFadeIn;
									// Override the background color if the current DC element is a hologram:
									g_DCPSCBuffer.bgColor[0] = hud_color;
									resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
								}
								resources->InitPixelShader(bIsHologram ? resources->_pixelShaderDCHolo : resources->_pixelShaderDC);
							}
							else if (g_PSCBuffer.bUseCoverTexture) {
								bModifiedPixelShader = true;
								resources->InitPixelShader(resources->_pixelShaderEmptyDC);
							}
						} // if dc_element->bActive
					}
					// TODO: I should probably put an assert here since this shouldn't happen
					/*else if (idx >= (int)g_DCElements.size()) {
						log_debug("[DBG] [DC] ****** idx: %d outside the bounds of g_DCElements (%d)", idx, g_DCElements.size());
					}*/
				}

				// Count the number of *actual* DC commands sent to the GPU:
				//if (g_PSCBuffer.bUseCoverTexture != 0 || g_PSCBuffer.DynCockpitSlots > 0)
				//	g_iDCElementsRendered++;

				// Don't render the first hyperspace frame: use all the buffers from the previous frame instead. Otherwise
				// the craft will jerk or blink because XWA resets the cockpit camera and the craft's orientation on this
				// frame.
				if (g_bHyperspaceFirstFrame)
					goto out;

				// DEBUG: Dump an OBJ for the current cockpit
				if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled && bIsCockpit)
					DumpVerticesToOBJ(g_DumpOBJFile, instruction, currentIndexLocation);

				// EARLY EXIT 2: RENDER NON-VR. Here we only need the state; but not the extra
				// processing needed for VR.
				if (!g_bEnableVR) {
					resources->InitViewport(&g_nonVRViewport);

					// Don't render the very first frame when entering hyperspace if we were not looking forward:
					// the game resets the yaw/pitch on the first frame and we don't want that
					//if (g_bHyperspaceFirstFrame && g_bHyperHeadSnapped)
					//	goto out;

					if (bModifiedShaders) {
						resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
						resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
						if (g_PSCBuffer.DynCockpitSlots > 0)
							resources->InitPSConstantBufferDC(resources->_PSConstantBufferDC.GetAddressOf(), &g_DCPSCBuffer);
					}

					if (!g_bReshadeEnabled) {
						// The original 2D vertices are already in the GPU, so just render as usual
						ID3D11RenderTargetView *rtvs[1] = {
							SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle),
						};
						context->OMSetRenderTargets(1, 
							//resources->_renderTargetView.GetAddressOf(),
							rtvs, resources->_depthStencilViewL.Get());
					} else {
						// Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
						// NON-VR with effects:
						ID3D11RenderTargetView *rtvs[6] = {
							SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle), //resources->_renderTargetView.Get(),

							resources->_renderTargetViewBloomMask.Get(),
							//g_bIsPlayerObject || g_bDisableDualSSAO ? resources->_renderTargetViewDepthBuf.Get() : resources->_renderTargetViewDepthBuf2.Get(),
							g_bAOEnabled ? resources->_renderTargetViewDepthBuf.Get() : NULL, // Don't use dual depth anymore
							// The normals hook should not be allowed to write normals for light textures
							bIsLightTexture ? NULL : resources->_renderTargetViewNormBuf.Get(),
							// Blast Marks are confused with glass because they are not shadeless; but they have transparency
							bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMask.Get(),
							bIsBlastMark ? NULL : resources->_renderTargetViewSSMask.Get(),
						};
						context->OMSetRenderTargets(6, rtvs, resources->_depthStencilViewL.Get());
					}

					context->DrawIndexed(3 * instruction->wCount, currentIndexLocation, 0);

					// Old screen-space shadow mapping code. Obsolete now that OBJs can be side-loaded
					/*
					if (g_ShadowMapping.Enabled && !g_ShadowMapping.UseShadowOBJ && bIsCockpit) 
					{
						Matrix4 T, Ry, Rx, S;
						Rx.rotateX(g_fShadowMapAngleX);
						Ry.rotateY(g_fShadowMapAngleY);
						T.translate(0, 0, g_fShadowMapDepthTrans);

						resources->InitViewport(&g_ShadowMapping.ViewPort);
						
						// Initialize the Constant Buffer
						// T * R does rotation first, then translation: so the object rotates around the origin
						// and then gets pushed away along the Z axis
						//g_ShadowMapVSCBuffer.lightWorldMatrix = T * Rx * Ry * S;
						g_ShadowMapVSCBuffer.lightWorldMatrix = Rx * Ry;
						g_ShadowMapVSCBuffer.sm_aspect_ratio = g_VSCBuffer.aspect_ratio;
						// Set the constant buffer
						resources->InitVSConstantBufferShadowMap(resources->_shadowMappingVSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);

						// Set the Vertex and Pixel Shaders
						resources->InitVertexShader(resources->_shadowMapVS);
						resources->InitPixelShader(resources->_shadowMapPS);

						if (g_ShadowMapping.UseShadowOBJ) {
							// Set the vertex and index buffers
							UINT stride = sizeof(D3DTLVERTEX), ofs = 0;
							resources->InitVertexBuffer(resources->_shadowVertexBuffer.GetAddressOf(), &stride, &ofs);
							resources->InitIndexBuffer(resources->_shadowIndexBuffer.Get(), false);
						}

						// Set the Shadow Map DSV
						context->OMSetRenderTargets(0, 0, resources->_shadowMapDSV.Get());
						// Render the Shadow Map
						if (g_ShadowMapping.UseShadowOBJ)
							context->DrawIndexed(g_ShadowMapping.NumIndices, 0, 0);
						else
							context->DrawIndexed(3 * instruction->wCount, currentIndexLocation, 0);

						// Restore the previous viewport, etc
						resources->InitViewport(&g_nonVRViewport);
						resources->InitVertexShader(resources->_vertexShader);
						resources->InitPixelShader(lastPixelShader);
						context->OMSetRenderTargets(0, 0, resources->_depthStencilViewL.Get());

						if (g_ShadowMapping.UseShadowOBJ) {
							// Set the vertex and index buffers
							UINT stride = sizeof(D3DTLVERTEX), ofs = 0;
							resources->InitVertexBuffer(this->_vertexBuffer.GetAddressOf(), &stride, &ofs);
							resources->InitIndexBuffer(this->_indexBuffer.Get(), g_config.D3dHookExists);
						}
					}
					*/

					goto out;
				}

				/********************************************************************
				   Modify the state of the render for VR
				 ********************************************************************/

				// Elements that are drawn with ZBuffer disabled:
				// * All GUI HUD elements except for the targetting computer (why?)
				// * Lens flares.
				// * All the text and brackets around objects. The brackets have their own draw call.
				// * Glasses on other cockpits and engine glow <-- Good candidate for bloom!
				// * Maybe explosions and other animations? I think explosions are actually rendered at depth (?)
				// * Cockpit sparks?

				// Reduce the scale for GUI elements, except for the HUD and Lens Flare
				// I'm not sure when this path is hit anymore. Maybe when DC is off?
				if (g_bIsScaleableGUIElem) {
					bModifiedShaders = true;
					g_VSCBuffer.scale_override = g_fGUIElemsScale;
					// Enable the fixed GUI
					if (g_bFixedGUI)
						g_VSCBuffer.bFullTransform = 1.0f;
				}

				// Enable the full transform for the hyperspace tunnel
				//if (bIsHyperspaceTunnel) {
					//bModifiedShaders = true; // TODO: Check the hyperspace tunnel in VR mode
					////g_VSCBuffer.bFullTransform = 1.0f; // This was already commented out! Do we need to set bModifiedShaders?
				//}

				// The game renders brackets with ZWrite disabled; but we need to enable it temporarily so that we
				// can place the brackets at infinity and avoid visual contention
				if (bIsBracket) {
					bModifiedShaders = true;
					QuickSetZWriteEnabled(TRUE);
					g_VSCBuffer.sz_override = 0.05f;
					g_VSCBuffer.z_override = g_fZBracketOverride;
					//g_PSCBuffer.bIsBackground = 1;
				}

				/* // Looks like we no longer need to clear the depth buffers for the targeted object
				if (!g_bPrevIsFloatingGUI3DObject && g_bIsFloating3DObject) {
					// The targeted craft is about to be drawn! Clear both depth stencils?
					//context->ClearDepthStencilView(this->_deviceResources->_depthStencilViewL, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
					//context->ClearDepthStencilView(this->_deviceResources->_depthStencilViewR, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
				}
				*/

				// if (bIsSkyBox) was here originally.

				/*
				if (bIsTranspOrGlow) {
					bModifiedShaders = true;
				}
				*/

				// Add an extra depth to HUD elements
				if (bIsReticle) {
					bModifiedShaders = true;
					g_VSCBuffer.z_override = g_fHUDDepth;
					// The Aiming HUD is now visible in external view using the exterior hook, let's put it at
					// infinity
					if (bExternalCamera)
						g_VSCBuffer.z_override = 65536.0f;
					if (g_bFloatingAimingHUD)
						g_VSCBuffer.bPreventTransform = 1.0f;
				}

				// Let's render the triangle pointer closer to the center so that we can see it all the time,
				// and let's put it at text depth so that it doesn't cause visual contention against the
				// cockpit
				if (g_bIsTrianglePointer) {
					//bModifiedShaders = true;
					//g_VSCBuffer.scale_override = 0.25f;
					//g_VSCBuffer.z_override = g_fTextDepth;
					
					(void)ComputeCentroid2D(instruction, currentIndexLocation, &g_TriangleCentroid);
					// Don't render the triangle pointer anymore, we'll do it later, when rendering the
					// reticle
					goto out;
				}

				// Add extra depth to Floating GUI elements and Lens Flare
				if (bIsFloatingGUI || g_bIsFloating3DObject || g_bIsScaleableGUIElem || bIsLensFlare) {
					bModifiedShaders = true;
					if (!bIsBracket)
						g_VSCBuffer.z_override = g_fFloatingGUIDepth;
					if (g_bIsFloating3DObject && !bIsBracket) {
						g_VSCBuffer.z_override += g_fFloatingGUIObjDepth;
					}
				}

				// Add an extra depth to Text elements
				if (bIsText) {
					bModifiedShaders = true;
					g_VSCBuffer.z_override = g_fTextDepth;
				}

				// Apply the changes to the vertex and pixel shaders
				if (bModifiedShaders) {
					resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
					resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
					if (g_PSCBuffer.DynCockpitSlots > 0)
						resources->InitPSConstantBufferDC(resources->_PSConstantBufferDC.GetAddressOf(), &g_DCPSCBuffer);
				}

				// Skip the draw call for debugging purposes depending on g_iNoDrawBeforeIndex and g_iNoDrawAfterIndex
#ifdef DBG_VR
				if (g_iDrawCounter < g_iNoDrawBeforeIndex)
					goto out;
				if (g_iNoDrawAfterIndex > -1 && g_iDrawCounter > g_iNoDrawAfterIndex)
					goto out;
#endif

				// ****************************************************************************
				// Render the left image
				// ****************************************************************************
				{
					// SteamVR probably requires independent ZBuffers; for non-Steam we can get away
					// with just using one, though... but why would we use just one? To make AO 
					// computation faster? On the other hand, having always 2 z-buffers makes the code
					// easier.
					if (g_bUseSteamVR) {
						if (!g_bReshadeEnabled) {
							ID3D11RenderTargetView *rtvs[1] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle),
							};
							context->OMSetRenderTargets(1, rtvs, resources->_depthStencilViewL.Get());
						} else {
							// SteamVR, left image. Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
							ID3D11RenderTargetView *rtvs[6] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle), //resources->_renderTargetView.Get(),
								resources->_renderTargetViewBloomMask.Get(),
								resources->_renderTargetViewDepthBuf.Get(), // Don't use dual SSAO anymore
								// The normals hook should not be allowed to write normals for light textures
								bIsLightTexture ? NULL : resources->_renderTargetViewNormBuf.Get(),
								// Blast Marks are confused with glass because they are not shadeless; but they have transparency
								bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMask.Get(),
								bIsBlastMark ? NULL : resources->_renderTargetViewSSMask.Get(),
							};
							context->OMSetRenderTargets(6, rtvs, resources->_depthStencilViewL.Get());
						}
					} else {
						// Direct SBS mode
						if (!g_bReshadeEnabled) {
							ID3D11RenderTargetView *rtvs[1] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle),
							};
							context->OMSetRenderTargets(1, rtvs, resources->_depthStencilViewL.Get());
						} else {
							// DirectSBS, Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
							ID3D11RenderTargetView *rtvs[6] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle), //resources->_renderTargetView.Get(),
								resources->_renderTargetViewBloomMask.Get(),
								//g_bIsPlayerObject || g_bDisableDualSSAO ? resources->_renderTargetViewDepthBuf.Get() : resources->_renderTargetViewDepthBuf2.Get(),
								resources->_renderTargetViewDepthBuf.Get(),
								// The normals hook should not be allowed to write normals for light textures
								bIsLightTexture ? NULL : resources->_renderTargetViewNormBuf.Get(),
								// Blast Marks are confused with glass because they are not shadeless; but they have transparency
								bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMask.Get(),
								bIsBlastMark ? NULL : resources->_renderTargetViewSSMask.Get(),
							};
							context->OMSetRenderTargets(6, rtvs, resources->_depthStencilViewL.Get());
						}
					}

					// VIEWPORT-LEFT
					if (g_bUseSteamVR) {
						viewport.Width = (float)resources->_backbufferWidth;
					} else {
						viewport.Width = (float)resources->_backbufferWidth / 2.0f;
					}
					viewport.Height = (float)resources->_backbufferHeight;
					viewport.TopLeftX = 0.0f;
					viewport.TopLeftY = 0.0f;
					viewport.MinDepth = D3D11_MIN_DEPTH;
					viewport.MaxDepth = D3D11_MAX_DEPTH;
					resources->InitViewport(&viewport);
					// Set the left projection matrix
					g_VSMatrixCB.projEye = g_FullProjMatrixLeft;
					// The viewMatrix is set at the beginning of the frame
					resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);
					// Draw the Left Image
					context->DrawIndexed(3 * instruction->wCount, currentIndexLocation, 0);
				}

				// ****************************************************************************
				// Render the right image
				// ****************************************************************************
				{
					// For SteamVR, we probably need two ZBuffers; but for non-Steam VR content we
					// just need one ZBuffer.
					//context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(),
					//	resources->_depthStencilViewR.Get());
					if (g_bUseSteamVR) {
						if (!g_bReshadeEnabled) {
							ID3D11RenderTargetView *rtvs[1] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle, true),
							};
							context->OMSetRenderTargets(1, rtvs, resources->_depthStencilViewR.Get());
						} else {
							// SteamVR, Reshade is enabled, render to multiple output targets
							ID3D11RenderTargetView *rtvs[6] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle, true),
								resources->_renderTargetViewBloomMaskR.Get(),
								resources->_renderTargetViewDepthBufR.Get(),
								// The normals hook should not be allowed to write normals for light textures
								bIsLightTexture ? NULL : resources->_renderTargetViewNormBufR.Get(),
								// Blast Marks are confused with glass because they are not shadeless; but they have transparency
								bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMaskR.Get(),
								bIsBlastMark ? NULL : resources->_renderTargetViewSSMaskR.Get(),
							};
							context->OMSetRenderTargets(6, rtvs, resources->_depthStencilViewR.Get());
						}
					} else {
						// DirectSBS Mode
						if (!g_bReshadeEnabled) {
							ID3D11RenderTargetView *rtvs[1] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle),
							};
							context->OMSetRenderTargets(1, rtvs, resources->_depthStencilViewL.Get());
						} else {
							// Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
							ID3D11RenderTargetView *rtvs[6] = {
								SelectOffscreenBuffer(bIsCockpit || bIsGunner || bIsReticle),
								resources->_renderTargetViewBloomMask.Get(),
								resources->_renderTargetViewDepthBuf.Get(),
								// The normals hook should not be allowed to write normals for light textures
								bIsLightTexture ? NULL : resources->_renderTargetViewNormBuf.Get(),
								// Blast Marks are confused with glass because they are not shadeless; but they have transparency
								bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMask.Get(),
								bIsBlastMark ? NULL : resources->_renderTargetViewSSMask.Get(),
							};
							context->OMSetRenderTargets(6, rtvs, resources->_depthStencilViewL.Get());
						}
					}

					// VIEWPORT-RIGHT
					if (g_bUseSteamVR) {
						viewport.Width = (float)resources->_backbufferWidth;
						viewport.TopLeftX = 0.0f;
					} else {
						viewport.Width = (float)resources->_backbufferWidth / 2.0f;
						viewport.TopLeftX = viewport.Width;
					}
					viewport.Height = (float)resources->_backbufferHeight;
					viewport.TopLeftY = 0.0f;
					viewport.MinDepth = D3D11_MIN_DEPTH;
					viewport.MaxDepth = D3D11_MAX_DEPTH;
					resources->InitViewport(&viewport);
					// Set the right projection matrix
					g_VSMatrixCB.projEye = g_FullProjMatrixRight;
					resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);
					// Draw the Right Image
					context->DrawIndexed(3 * instruction->wCount, currentIndexLocation, 0);
				}

			out:
				// Update counters
				g_iDrawCounter++;
				if (g_iDrawCounterAfterHUD > -1)
					g_iDrawCounterAfterHUD++;
				// Have we just finished drawing the targetting computer?
				if (bLastTextureSelectedNotNULL && lastTextureSelected->is_Floating_GUI)
					g_iFloatingGUIDrawnCounter++;

				if (g_bIsTrianglePointer)
					g_bLastTrianglePointer = true;

				// Restore the No-Z-Write state for bracket elements
				if (bIsBracket && bModifiedShaders) {
					QuickSetZWriteEnabled(bZWriteEnabled);
					g_VSCBuffer.z_override = -1.0f;
					g_VSCBuffer.sz_override = -1.0f;
					g_VSCBuffer.mult_z_override = -1.0f;
					resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
				}

				//g_PSCBuffer = { 0 };
				//g_PSCBuffer.brightness = MAX_BRIGHTNESS;
				//g_PSCBuffer.ct_brightness = g_fCoverTextureBrightness;
				//resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);

				// Restore the normal state of the render; but only if we altered it previously.
				if (bModifiedShaders) {
					g_VSCBuffer.viewportScale[3]  =  g_fGlobalScale;
					g_VSCBuffer.z_override        = -1.0f;
					g_VSCBuffer.sz_override       = -1.0f;
					g_VSCBuffer.mult_z_override   = -1.0f;
					g_VSCBuffer.bPreventTransform =  0.0f;
					g_VSCBuffer.bFullTransform    =  FullTransform;
					g_VSCBuffer.scale_override    =  1.0f;

					g_PSCBuffer = { 0 };
					g_PSCBuffer.brightness		= MAX_BRIGHTNESS;
					g_PSCBuffer.fBloomStrength	= 1.0f;
					g_PSCBuffer.fPosNormalAlpha = 1.0f;
					g_PSCBuffer.fSSAOAlphaMult  = g_fSSAOAlphaOfs;
					g_PSCBuffer.fSSAOMaskVal    = g_DefaultGlobalMaterial.Metallic * 0.5f;
					g_PSCBuffer.fGlossiness     = g_DefaultGlobalMaterial.Glossiness;
					g_PSCBuffer.fSpecInt        = g_DefaultGlobalMaterial.Intensity; // DEFAULT_SPEC_INT;
					g_PSCBuffer.fNMIntensity    = g_DefaultGlobalMaterial.NMIntensity;

					if (g_PSCBuffer.DynCockpitSlots > 0) {
						g_DCPSCBuffer = { 0 };
						g_DCPSCBuffer.ct_brightness = g_fCoverTextureBrightness;
						g_DCPSCBuffer.dc_brightness = g_fDCBrightness;
						// Restore the regular pixel shader (disable the PixelShaderDC)
						//resources->InitPixelShader(lastPixelShader);
					}

					// Remove the cover texture
					//context->PSSetShaderResources(1, 1, NULL);
					//g_PSCBuffer.bUseCoverTexture  = 0;
					//g_PSCBuffer.DynCockpitSlots   = 0;
					//g_PSCBuffer.bRenderHUD		  = 0;
					//g_PSCBuffer.bEnhaceLasers	  = 0;
					//g_PSCBuffer.bIsLaser = 0;
					//g_PSCBuffer.bIsLightTexture   = 0;
					//g_PSCBuffer.bIsEngineGlow = 0;
					resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
					resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
				}

				if (bModifiedPixelShader)
					resources->InitPixelShader(lastPixelShader);

				if (bModifiedBlendState) {
					RestoreBlendState();
					bModifiedBlendState = false;
				}

				if (bModifiedSamplerState) {
					RestoreSamplerState();
					bModifiedSamplerState = false;
				}

				currentIndexLocation += 3 * instruction->wCount;
				break;
			}
			}

			if (FAILED(hr))
				break;
		}
	}

//noexec:
	//g_iExecBufCounter++; // This variable is used to find when the SkyBox has been rendered
	// This variable is useless with the hook_d3d: it stays at 1, meaning that this function is called exactly *once* per frame.

	if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled) {
		if (g_DumpOBJFile != NULL) {
			fflush(g_DumpOBJFile);
			fclose(g_DumpOBJFile);
		}
		g_DumpOBJFile = NULL;
		log_debug("[DBG] Vertices dumped to OBJ file");
	}
	//log_debug("[DBG] Execute (2)");

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			char text[512];
			strcpy_s(text, step);
			strcat_s(text, "\n");
			strcat_s(text, _com_error(hr).ErrorMessage());

			//MessageBox(nullptr, text, __FUNCTION__, MB_ICONERROR);
			log_debug("[DBG] %s, %s", text, __FUNCTION__);
		}

		messageShown = true;

#if LOGGER
		str.str("\tD3DERR_EXECUTE_FAILED");
		LogText(str.str());
#endif

		return D3DERR_EXECUTE_FAILED;
	}

#if LOGGER
	str.str("\tD3D_OK");
	LogText(str.str());
#endif

	return D3D_OK;
}

HRESULT Direct3DDevice::AddViewport(
	LPDIRECT3DVIEWPORT lpDirect3DViewport
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tD3D_OK");
	LogText(str.str());
#endif

	return D3D_OK;
}

HRESULT Direct3DDevice::DeleteViewport(
	LPDIRECT3DVIEWPORT lpDirect3DViewport
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tD3D_OK");
	LogText(str.str());
#endif

	return D3D_OK;
}

HRESULT Direct3DDevice::NextViewport(
	LPDIRECT3DVIEWPORT lpDirect3DViewport,
	LPDIRECT3DVIEWPORT *lplpDirect3DViewport,
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::Pick(
	LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
	LPDIRECT3DVIEWPORT lpDirect3DViewport,
	DWORD dwFlags,
	LPD3DRECT lpRect
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::GetPickRecords(
	LPDWORD lpCount,
	LPD3DPICKRECORD lpD3DPickRec
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::EnumTextureFormats(
	LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc,
	LPVOID lpArg
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpd3dEnumTextureProc == nullptr)
	{
		return DDERR_INVALIDPARAMS;
	}

	DDSURFACEDESC sd{};
	sd.dwSize = sizeof(DDSURFACEDESC);
	sd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
	sd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

	// 0555
	sd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	sd.ddpfPixelFormat.dwRGBBitCount = 16;
	sd.ddpfPixelFormat.dwRBitMask = 0x7C00;
	sd.ddpfPixelFormat.dwGBitMask = 0x03E0;
	sd.ddpfPixelFormat.dwBBitMask = 0x001F;
	sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0000;

	if (lpd3dEnumTextureProc(&sd, lpArg) != D3DENUMRET_OK)
		return D3D_OK;

	// 1555
	sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	sd.ddpfPixelFormat.dwRGBBitCount = 16;
	sd.ddpfPixelFormat.dwRBitMask = 0x7C00;
	sd.ddpfPixelFormat.dwGBitMask = 0x03E0;
	sd.ddpfPixelFormat.dwBBitMask = 0x001F;
	sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;

	if (lpd3dEnumTextureProc(&sd, lpArg) != D3DENUMRET_OK)
		return D3D_OK;

	// 4444
	sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	sd.ddpfPixelFormat.dwRGBBitCount = 16;
	sd.ddpfPixelFormat.dwRBitMask = 0x0F00;
	sd.ddpfPixelFormat.dwGBitMask = 0x00F0;
	sd.ddpfPixelFormat.dwBBitMask = 0x000F;
	sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xF000;

	if (lpd3dEnumTextureProc(&sd, lpArg) != D3DENUMRET_OK)
		return D3D_OK;

	// 0565
	sd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	sd.ddpfPixelFormat.dwRGBBitCount = 16;
	sd.ddpfPixelFormat.dwRBitMask = 0xF800;
	sd.ddpfPixelFormat.dwGBitMask = 0x07E0;
	sd.ddpfPixelFormat.dwBBitMask = 0x001F;
	sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0000;

	if (lpd3dEnumTextureProc(&sd, lpArg) != D3DENUMRET_OK)
		return D3D_OK;

	// 0888
	sd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	sd.ddpfPixelFormat.dwRGBBitCount = 32;
	sd.ddpfPixelFormat.dwRBitMask = 0x0FF0000;
	sd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	sd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
	sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x00000000;

	if (lpd3dEnumTextureProc(&sd, lpArg) != D3DENUMRET_OK)
		return D3D_OK;

	// 8888
	sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	sd.ddpfPixelFormat.dwRGBBitCount = 32;
	sd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
	sd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
	sd.ddpfPixelFormat.dwBBitMask = 0x000000FF;
	sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;

	if (lpd3dEnumTextureProc(&sd, lpArg) != D3DENUMRET_OK)
		return D3D_OK;

	return D3D_OK;
}

HRESULT Direct3DDevice::CreateMatrix(
	LPD3DMATRIXHANDLE lpD3DMatHandle
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::SetMatrix(
	D3DMATRIXHANDLE d3dMatHandle,
	LPD3DMATRIX lpD3DMatrix
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::GetMatrix(
	D3DMATRIXHANDLE d3dMatHandle,
	LPD3DMATRIX lpD3DMatrix
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DDevice::DeleteMatrix(
	D3DMATRIXHANDLE d3dMatHandle
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

void Direct3DDevice::RenderEdgeDetector()
{
	auto& resources = this->_deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	D3D11_VIEWPORT viewport;
	float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const bool bExternalView = PlayerDataTable[*g_playerIndex].externalCamera;
	D3D11_DEPTH_STENCIL_DESC desc;
	D3D11_BLEND_DESC blendDesc{};
	ComPtr<ID3D11DepthStencilState> depthState;

	DCElemSrcBox *dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[TARGET_COMP_DC_ELEM_SRC_IDX];
	if (dcElemSrcBox->bComputed) {
		float W = dcElemSrcBox->coords.x1 - dcElemSrcBox->coords.x0;
		float H = dcElemSrcBox->coords.y1 - dcElemSrcBox->coords.y0;
		
		// Send the UV coords down to the shader as well
		g_ShadertoyBuffer.x0 = dcElemSrcBox->coords.x0;
		g_ShadertoyBuffer.y0 = dcElemSrcBox->coords.y0;
		g_ShadertoyBuffer.x1 = dcElemSrcBox->coords.x1;
		g_ShadertoyBuffer.y1 = dcElemSrcBox->coords.y1;
		
		viewport.Width    = g_fCurScreenWidth * W;
		viewport.Height   = g_fCurScreenHeight * H;
		viewport.TopLeftX = g_fCurScreenWidth * dcElemSrcBox->coords.x0;
		viewport.TopLeftY = g_fCurScreenHeight * dcElemSrcBox->coords.y0;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		// Convert the UVs into in-game UVs for the subCMD bracket
		float x0, y0, x1, y1;
		// Convert screen coords to in-game coords:
		ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
			g_nonVRViewport.Width, g_nonVRViewport.Height, 
			viewport.TopLeftX, viewport.TopLeftY, 
			&x0, &y0);
		ScreenCoordsToInGame(g_nonVRViewport.TopLeftX, g_nonVRViewport.TopLeftY,
			g_nonVRViewport.Width, g_nonVRViewport.Height, 
			viewport.TopLeftX + viewport.Width, viewport.TopLeftY + viewport.Height, 
			&x1, &y1);
		//log_debug("[DBG] subCMD box: (%0.3f, %0.3f)-(%0.3f, %0.3f)", x0, y0, x1, y1);
		// Convert to UVs:
		g_ShadertoyBuffer.SunCoords[1].x = x0 / g_fCurInGameWidth;
		g_ShadertoyBuffer.SunCoords[1].y = y0 / g_fCurInGameHeight;
		g_ShadertoyBuffer.SunCoords[1].z = x1 / g_fCurInGameWidth;
		g_ShadertoyBuffer.SunCoords[1].w = y1 / g_fCurInGameHeight;
		// Send the in-game resolution:
		g_ShadertoyBuffer.SunCoords[2].x = 1.0f / g_fCurInGameWidth;
		g_ShadertoyBuffer.SunCoords[2].y = 1.0f / g_fCurInGameHeight;
		// If the Radar2DRenderer is enabled, then we'll put the subCMD bracket coords in
		// SunCoords[3] and set SunCoords[3].w to 1.0. If not, then SunCoords[3].w will be set to 0.
		if (g_config.Radar2DRendererEnabled) {
			float x, y;
			static float pulse = 3.0f;
			pulse += 0.5f;
			if (pulse > 12.0f)
				pulse = 3.0f;
			//log_debug("[DBG] g_SubCMDBracket: %0.3f, %0.3f", g_SubCMDBracket.x, g_SubCMDBracket.y);
			// Convert In-game coords to post-proc UVs
			InGameToScreenCoords((UINT)g_nonVRViewport.TopLeftX, (UINT)g_nonVRViewport.TopLeftY,
				(UINT)g_nonVRViewport.Width, (UINT)g_nonVRViewport.Height, g_SubCMDBracket.x, g_SubCMDBracket.y, &x, &y);
			g_ShadertoyBuffer.SunCoords[3].x = x / g_fCurScreenWidth;
			g_ShadertoyBuffer.SunCoords[3].y = y / g_fCurScreenHeight;
			g_ShadertoyBuffer.SunCoords[3].w = 1.0f;
			g_ShadertoyBuffer.iTime = pulse;
		} else
			g_ShadertoyBuffer.SunCoords[3].w = 0.0f;
	}
	else
		return;

	// SunCoords[1] are the UV coords for the buffer that has the SubCMD when the 2D renderer is disabled
	// SunCoords[2].xy is the in-game resolution
	// SunCoords[3].xy is the center of the SubCMD component if the 2D renderer is enabled
	
	// SunColor[0].xyz is the color of the wireframe
	// SunColor[0].w is the contrast enhancer
	// SunColor[1] is the luminance vector
	g_ShadertoyBuffer.SunColor[1] = g_DCWireframeLuminance;

	// We need to set the blend state properly
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO; // This will replace the Dest alpha with the Src alpha, overwriting it
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	resources->InitBlendState(nullptr, &blendDesc);

	// Temporarily disable ZWrite
	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);

	g_ShadertoyBuffer.iResolution[0] = 1.0f / g_fCurScreenWidth;
	g_ShadertoyBuffer.iResolution[1] = 1.0f / g_fCurScreenHeight;
	// Set a default color for the wireframe
	g_ShadertoyBuffer.SunColor[0].x = 0.1f;
	g_ShadertoyBuffer.SunColor[0].y = 0.1f;
	g_ShadertoyBuffer.SunColor[0].z = 0.5f;
	// Read the IFF of the current target and use it to colorize the wireframe display
	short currentTargetIndex = PlayerDataTable[*g_playerIndex].currentTargetIndex;
	// I think I remember that when currentTargetIndex is 0, the game crashed; but Jeremy
	// just told me that currentTargetIndex can be 0 (it's -1 when no target is selected).
	// So, updating changing this code to allow target 0:
	if (currentTargetIndex >= 0) {
		ObjectEntry *object = &((*objects)[currentTargetIndex]);
		if (object == NULL) goto nocolor;
		MobileObjectEntry *mobileObject = object->MobileObjectPtr;
		if (mobileObject == NULL) goto nocolor;
		int IFF = mobileObject->IFF;
		if (IFF >= 0 && IFF <= 5)
			g_ShadertoyBuffer.SunColor[0] = g_DCTargetingIFFColors[IFF];
	}
nocolor:
	// Override all of the above if the current DC file has a wireframe color set:
	if (g_DCTargetingColor.w > 0.0f) {
		g_ShadertoyBuffer.SunColor[0] = g_DCTargetingColor;
	}
	// Send the contrast data
	g_ShadertoyBuffer.SunColor[0].w = g_DCWireframeContrast;
	// Set the time
	static float time = 0.0f;
	time += 0.1f;
	if (time > 2.0f) time = 0.0f;
	g_ShadertoyBuffer.iTime = time;

	/*
	// The noise effect doesn't look that great on all cockpits, and in VR it becomes really low-res
	// I'm going to disable it, but this block can be used to re-enable it in the future.
	// Check the state of the targeted craft. If it's destroyed, then add some noise to the screen...
	static float destroyedTimer = 0.0f;
	if (currentTargetIndex > -1) {
		ObjectEntry *object = &((*objects)[currentTargetIndex]);
		if (object == NULL) goto reset;
		MobileObjectEntry *mobileObject = object->MobileObjectPtr;
		if (mobileObject == NULL) goto reset;
		CraftInstance *craftInstance = mobileObject->craftInstancePtr;
		if (craftInstance == NULL) goto reset;
		if (craftInstance->CraftState == 3 && !bExternalView) {
			destroyedTimer += 0.005f;
			destroyedTimer = min(destroyedTimer, 1.0f);
		}
		else
			destroyedTimer = 0.0f;
	}
	else {
reset:
		destroyedTimer = 0.0f;
	}
	g_ShadertoyBuffer.twirl = destroyedTimer;
	*/

	// Shut down the CMD if the target has been destroyed. Otherwise we might see some artifacts
	// due to explosions and gas.
	g_ShadertoyBuffer.twirl = 1.0f; // Display the CMD
	if (currentTargetIndex > -1) {
		ObjectEntry *object = &((*objects)[currentTargetIndex]);
		if (object == NULL) goto nochange;
		MobileObjectEntry *mobileObject = object->MobileObjectPtr;
		if (mobileObject == NULL) goto nochange;
		CraftInstance *craftInstance = mobileObject->craftInstancePtr;
		if (craftInstance == NULL) goto nochange;
		if (craftInstance->CraftState == 3)
			g_ShadertoyBuffer.twirl = 0.0f; // Shut down the CMD
	}
nochange:
	
	resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);

	resources->InitPixelShader(resources->_edgeDetectorPS);
	if (g_bDumpSSAOBuffers) {
		DirectX::SaveWICTextureToFile(context, resources->_offscreenAsInputDynCockpit, GUID_ContainerFormatJpeg,
			L"C:\\Temp\\_edgeDetectorInput.jpg");
	}

	// Apply the edge detector to the DC foreground buffer
	{
		// We don't need to clear the current vertex and pixel constant buffers.
		// Since we've just finished rendering 3D, they should contain values that
		// can be reused. So let's just overwrite the values that we need.
		g_VSCBuffer.aspect_ratio      =  g_fAspectRatio;
		g_VSCBuffer.z_override        = -1.0f;
		g_VSCBuffer.sz_override       = -1.0f;
		g_VSCBuffer.mult_z_override   = -1.0f;
		g_VSCBuffer.apply_uv_comp     =  false;
		g_VSCBuffer.bPreventTransform =  0.0f;
		g_VSCBuffer.bFullTransform    =  0.0f;
		g_VSCBuffer.viewportScale[0]  =  2.0f / resources->_displayWidth;
		g_VSCBuffer.viewportScale[1]  = -2.0f / resources->_displayHeight;

		// Since the HUD is all rendered on a flat surface, we lose the vrparams that make the 3D object
		// and text float
		g_VSCBuffer.z_override = 65535.0f;
		g_VSCBuffer.scale_override = 1.0f;

		// Set the left projection matrix (the viewMatrix is set at the beginning of the frame)
		g_VSMatrixCB.projEye = g_FullProjMatrixLeft;
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		UINT stride = sizeof(D3DTLVERTEX), offset = 0;
		resources->InitVertexBuffer(resources->_hyperspaceVertexBuffer.GetAddressOf(), &stride, &offset);
		resources->InitInputLayout(resources->_inputLayout);
		resources->InitVertexShader(resources->_vertexShader);
		resources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//context->ClearRenderTargetView(resources->_renderTargetViewPost, bgColor);
		// Instead of clearing the RTV, we copy the DC FG buffer to the offscreenBufferPost, that way the
		// viewport only overwrites the section we're going to process.
		context->CopyResource(resources->_offscreenBufferPost, resources->_offscreenBufferDynCockpit);
		// Set the RTV:
		ID3D11RenderTargetView *rtvs[1] = {
			resources->_renderTargetViewPost.Get(), // Render to offscreenBufferPost instead of offscreenBuffer
		};
		context->OMSetRenderTargets(1, rtvs, NULL);
		// Set the SRVs:
		resources->InitPSShaderResourceView(resources->_offscreenAsInputDynCockpitSRV);
		context->PSSetShaderResources(1, 1, resources->_mainDisplayTextureView.GetAddressOf());
		context->Draw(6, 0);

		if (g_bDumpSSAOBuffers) {
			DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferPost, GUID_ContainerFormatJpeg,
				L"C:\\Temp\\_edgeDetectorOutput.jpg");
		}
	}

	// Copy or resolve the result
	if (g_config.MultisamplingAntialiasingEnabled)
		context->ResolveSubresource(resources->_offscreenAsInputDynCockpit, 0, resources->_offscreenBufferPost, 0, BACKBUFFER_FORMAT);
	else
		context->CopyResource(resources->_offscreenAsInputDynCockpit, resources->_offscreenBufferPost);
	
	// Restore previous rendertarget: this line is necessary or the 2D content won't be displayed
	// after applying this effect.
	// The reason we need this is because the original ddraw never expects another RTV other than
	// _renderTargetView, so there isn't an InitRenderTargetView() function -- there's no need since
	// this RTV is always going to be set. If we break that assumption here, things will stop working.
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), NULL);
}

HRESULT Direct3DDevice::BeginScene()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif
	static bool bPrevHyperspaceState = false, bCurHyperspaceState = false;
	bool bTransitionToHyperspace = false;
	bPrevHyperspaceState = bCurHyperspaceState;
	bCurHyperspaceState = PlayerDataTable[*g_playerIndex].hyperspacePhase != 0;
	bTransitionToHyperspace = !bPrevHyperspaceState && bCurHyperspaceState;
	// We want to capture the transition to hyperspace because we don't want to clear some buffers
	// when this happens. The problem is that the game snaps the camera to the forward position as soon
	// as we jump into hyperspace; but that causes glitches with the new hyperspace effect. To solve this
	// I'm storing the heading of the camera right before the jump and then I restore it as soon as possible
	// while I inhibit the draw calls for the very first hyperspace frame. However, this means that I must 
	// also inhibit clearing these buffers on that same frame or the effect will "blink"

	g_ExecuteCount = 0;
	g_ExecuteVertexCount = 0;
	g_ExecuteIndexCount = 0;
	g_ExecuteStateCount = 0;
	g_ExecuteTriangleCount = 0;
	g_CurrentHeadingViewMatrix = GetCurrentHeadingViewMatrix();
	UpdateDCHologramState();

	if (!this->_deviceResources->_renderTargetView)
	{
#if LOGGER
		str.str("\tD3DERR_SCENE_BEGIN_FAILED");
		LogText(str.str());
#endif

		return D3DERR_SCENE_BEGIN_FAILED;
	}
	
	this->_deviceResources->inScene = true;
	this->_deviceResources->inSceneBackbufferLocked = false;

	auto& device = this->_deviceResources->_d3dDevice;
	auto& context = this->_deviceResources->_d3dDeviceContext;
	auto& resources = this->_deviceResources;

	if (!bTransitionToHyperspace) {
		context->ClearRenderTargetView(this->_deviceResources->_renderTargetView, this->_deviceResources->clearColor);
		context->ClearRenderTargetView(resources->_shadertoyRTV, resources->clearColorRGBA);
		if (g_bEnableVR) {
			context->ClearRenderTargetView(resources->_ReticleRTV, resources->clearColorRGBA);
		}
		if (g_bUseSteamVR) {
			context->ClearRenderTargetView(this->_deviceResources->_renderTargetViewR, this->_deviceResources->clearColor);
			context->ClearRenderTargetView(resources->_shadertoyRTV_R, resources->clearColorRGBA);
		}
	}

	// Clear the Bloom Mask RTVs -- SSDO also uses the bloom mask (and maybe SSAO should too), so we have to clear them
	// even if the Bloom effect is disabled
	if (g_bBloomEnabled || resources->_renderTargetViewBloomMask != NULL) 
		if (!bTransitionToHyperspace) {
			context->ClearRenderTargetView(resources->_renderTargetViewBloomMask, resources->clearColor);
			if (g_bUseSteamVR)
				context->ClearRenderTargetView(resources->_renderTargetViewBloomMaskR, resources->clearColor);
		}

	if (g_bReshadeEnabled && !bTransitionToHyperspace) {
		float infinity[4] = { 0, 0, 32000.0f, 0 };
		float zero[4] = { 0, 0, 0, 0 };
		context->ClearRenderTargetView(resources->_renderTargetViewNormBuf, infinity);
		context->ClearRenderTargetView(resources->_renderTargetViewSSAOMask, zero);
		if (g_bUseSteamVR) {
			context->ClearRenderTargetView(resources->_renderTargetViewNormBufR, infinity);
			context->ClearRenderTargetView(resources->_renderTargetViewSSAOMaskR, zero);
		}
	}

	// Clear the AO RTVs
	if (g_bAOEnabled && !bTransitionToHyperspace) {
		// Filling up the ZBuffer with large values prevents artifacts in SSAO when black bars are drawn
		// on the sides of the screen
		float infinity[4] = { 0, 0, 32000.0f, 0 };
		float zero[4] = { 0, 0, 0, 0 };

		context->ClearRenderTargetView(resources->_renderTargetViewDepthBuf, infinity);
		if (g_bUseSteamVR)
			context->ClearRenderTargetView(resources->_renderTargetViewDepthBufR, infinity);
	}

	if (!bTransitionToHyperspace) {
		context->ClearDepthStencilView(resources->_depthStencilViewL, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
		if (g_bUseSteamVR)
			context->ClearDepthStencilView(resources->_depthStencilViewR, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
	}

	//log_debug("[DBG] BeginScene RenderMain");
	if (FAILED(this->_deviceResources->RenderMain(resources->_backbufferSurface->_buffer, resources->_displayWidth,
		resources->_displayHeight, resources->_displayBpp)))
		return D3DERR_SCENE_BEGIN_FAILED;

	if (this->_deviceResources->_displayBpp == 2)
	{
		unsigned short* buffer = (unsigned short*)this->_deviceResources->_backbufferSurface->_buffer;
		int length = this->_deviceResources->_displayWidth * this->_deviceResources->_displayHeight;

		for (int i = 0; i < length; i++)
		{
			buffer[i] = 0x2000;
		}
	}
	else
	{
		unsigned int* buffer = (unsigned int*)this->_deviceResources->_backbufferSurface->_buffer;
		int length = this->_deviceResources->_displayWidth * this->_deviceResources->_displayHeight;

		for (int i = 0; i < length; i++)
		{
			buffer[i] = 0x200000;
		}
	}

	return D3D_OK;
}

HRESULT Direct3DDevice::EndScene()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif
	auto& resources = this->_deviceResources;
	auto& context = resources->_d3dDeviceContext;

	/*
	I can't render hyperspace here because it will break the display of the main menu after exiting a mission
	Looks like even the menu uses EndScene and I'm messing it up even when using the "g_bRendering3D" flag!
	*/

	this->_deviceResources->sceneRendered = true;

	this->_deviceResources->inScene = false;

	/*if (g_config.D3dHookExists)
	{
		OutputDebugString((
			std::string("EndScene")
			+ " E=" + std::to_string(g_ExecuteCount)
			+ " S=" + std::to_string(g_ExecuteStateCount)
			+ " T=" + std::to_string(g_ExecuteTriangleCount)
			+ " V=" + std::to_string(g_ExecuteVertexCount)
			+ " I=" + std::to_string(g_ExecuteIndexCount)).c_str());
	}*/

	g_SubCMDBracket.x = -1.0f;
	g_SubCMDBracket.y = -1.0f;

	return D3D_OK;
}

HRESULT Direct3DDevice::GetDirect3D(
	LPDIRECT3D *lplpD3D
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}
