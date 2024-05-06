// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes, 2019

// Shaders by Marty McFly (used with permission from the author)
// https://github.com/martymcmodding/qUINT/tree/master/Shaders

// _deviceResources->_backbufferWidth, _backbufferHeight: 3240, 2160 -- Screen/Backbuffer Resolution
// resources->_displayWidth, resources->_displayHeight -- in-game resolution
// g_WindowWidth, g_WindowHeight --> Actual Windows screen as returned by GetWindowRect

// Important commits:
// Improve Depth Value:
// https://github.com/Prof-Butts/xwa_ddraw_d3d11/commit/f05c0ef6f65b2bf7f9e085baf9ebaf0e7207eb41
// One BLAS per FaceGroup:
// https://github.com/Prof-Butts/xwa_ddraw_d3d11/commit/673da14a4b717ded5296dc6e17b1d95c43c20147

/*
From you-know-who:

The function to create lights for backdrops look like that:
// L00439040
void XwaCreateGlobalLightsForBackdrops(  )
{
	dword ebx = s_XwaPlayers[s_XwaCurrentPlayerId].Region;

	for( dword edi = 0; edi < s_XwaBackdropsCountPerRegion[ebx]; edi++ )
	{
		if( s_XwaBackdrops[ebx * 0x20 + edi].ColorIntensity <= 0.0f )
			continue;

		XwaAddGlobalLight(
			s_XwaBackdrops[ebx * 0x20 + edi].WorldX / 256,
			s_XwaBackdrops[ebx * 0x20 + edi].WorldY / 256,
			s_XwaBackdrops[ebx * 0x20 + edi].WorldZ / 256,
			s_XwaBackdrops[ebx * 0x20 + edi].ColorIntensity,
			s_XwaBackdrops[ebx * 0x20 + edi].ColorR,
			s_XwaBackdrops[ebx * 0x20 + edi].ColorG,
			s_XwaBackdrops[ebx * 0x20 + edi].ColorB
			);
	}
}

Before the call to XwaCreateGlobalLightsForBackdrops the global lights count is set to 0.
The lights are created in the same order as the backdrops appear in the mission tie file.
In the XwaTieFlightGroups array you can get the PlanetId.
For backdrops the CraftId is CraftId_183_9001_1100_ResData_Backdrop.
With the PlanetId you can read the backdrop model index in the XwaPlanets.
With the model index you can read in the ExeObjectsTable the backdrop dat GroupId and ImageId.

*/

/*
From Jeremy, regarding LODs and how to parse the OPT structure:

There is no direct way to tell which lod is rendered.
There is int& s_XwaOptCurrentLodDistance = *(int*)0x007D4F8C;.
There is float& s_XwaFlightLod = *(float*)0x00600288;.
The distance is 1.0f / ( s_XwaOptCurrentLodDistance * s_XwaFlightLod );
You can access to the whole opt structure via a model index.
The SceneCompData struct contains a pObject member. The XwaObject has a model index.
At offset 0x007CA6E0 there is an array s_XwaOptModelFileMemHandles of 557 short. The index in this array is a model index.
There are these function to lock and unlock the handle.
// L0050E2F0
void* Lock_Handle( short A4 )
// L0050E350
void Unlock_Handle( short A4 )

The data struct of the locked handle is a OptHeader pointer.
With this header you can access to the whole opt.
*/

/*
From Jeremy, regarding how to replace the transform matrix in MobileObject

The function to recalculate the forward vector is:
// L0043FFB0
void XwaRecalculateForwardVector( word A4, word A8, Ptr<XwaObject> AC )

The function to recalculate the transform matrix is:
// L00440140
void XwaRecalculateTransformMatrix( word A4, word A8, Ptr<XwaObject> AC )

I think you can modify/replace the implementation of this function to define the matrix to whatever you want 

*/

/*
* Is there a field that tells us when a ship is under the effect of a beam weapon?
*
* RandomStarfighter:
* Offset 0x13 into the craft struct, there's an int32[5] array that tracks whether
* the craft is currently under the effect of a beam weapon. The first two slots are
* of note for tractor and jamming beams, in that order. Decoy beam isn't tracked here
* since it has a meta effect rather than a direct effect. The other slots are for
* unused beams and not implemented. If I recall, the array is cleared and re-examined
* every frame during the craft update logic.
*
*/
/*

Mission Index:
	Skirmish: 0
	PPG: 6 (looks like this is complicated in TFTC).
	Death Star: 52
	Lights are automatically turned on when there are no global lights. The only mission in XWA where this happens is mission 52.
*/

/*

// V0x007FFD80
dword s_XwaObjectsCount;

// V0x008C1CE4
dword s_XwaCraftsCount;

Use s_XwaObjectsCount to read the objects table.
Use s_XwaCraftsCount to read the crafts table.

*/

/*

craftInstance->CockpitInstrumentStatus damage bits:

 0x0001 is the CMD/Targeting computer.
 0x000E is the laser/ion display. Looks like all 3 bits must be on, but not sure what happens if the craft doesn't have ions
 0x0010 is the beam weapon
 0x0020 is the shields display
 0x0040 is the throttle (text) display <-- This matches the code that Jeremy provided (see m181 & 64 below)
 0x0180 both sensors. Both bits must be on, if either bit is off, both sensors will shut down
 0x0200 lasers recharge rate
 0x0400 engine level
 0x0800 shields recharge rate
 0x1000 beam recharge rate

Yes, the cockpit instrument damages seem to be at offset 0x17F and 0x181 in the craft struct. I don't know how to interpret these values.
But for the craft components damages, there is:
In XwaCraft struct:
 0x01F7  Array<byte, 11> DamageAssessmentsOrder;
 0x0202  Array<word, 11> DamageAssessmentsPercent; // 100 = not damaged
 0x0218  Array<word, 11> DamageAssessmentsTimeRemaining;

enum DamageAssessmentEnum : unsigned char
{
	DamageAssessment_Engines = 0,
	DamageAssessment_FlightControls = 1,
	DamageAssessment_CannonSystem = 2,
	DamageAssessment_ShieldSystem = 3,
	DamageAssessment_TargetingComputer = 4,
	DamageAssessment_WarheadLauncher = 5,
	DamageAssessment_BeamSystem = 6,
	DamageAssessment_Communications = 7,
	DamageAssessment_Countermeasures = 8,
	DamageAssessment_Hyperdrive = 9,
};

Also, from the "how to get the value of the throttle question", we know that offset 0x181
seems to behave like a bitfield:

if ((ebp20->XwaCraft_m181 & 64) != 0)
	<display throttle>

*/

/*

HOW TO CREATE A SIMPLE TRACKER (from user BenKenobi, I think):

https://forums.xwaupgrade.com/viewtopic.php?f=15&t=12551&p=166877&hilit=opentrack#p166877
To add to that, You can also use Opentrack with an Aruco Marker. In that way all you need
is a webcam and a piece of paper! I’m testing this while waiting for IR leds to arrive and
the results are amazing!

I found a video on this on the internet while waiting for the IR leds and thought I’d give
it a go, didn’t even print the Aruco Marker, just used a piece of paper and used a black
Edding marker to make it.

See the video here: https://www.youtube.com/watch?v=ajoUzwe1bT0

I do use it with a PS 3 Eye camera, tried others but the Eye camera perfoms the best, it’s
not the resolution that’s important but the frame rate.

*/

/*
Jeremy: Enable film recording in melee missions (PPG/Yard)
At offset 100AF8, replace 0F8586010000 with 909090909090.
*/

/*
How to get the current throttle setting:
Jeremy: @blue_max An unsigned short at offset 0x00F0 in the XwaCraft struct.

In function L00473D00:
// L00473D00
void UpdateHUDText()

	Ptr<XwaCraft> ebp20 = 0;

	int eax0 = s_XwaPlayers[s_XwaCurrentPlayerId].ObjectIndex;

	if (eax0 != 0xFFFF)
	{
		Ptr<XwaMobileObject> eax1 = s_XwaObjects[eax0].pMobileObject;

		if( eax1 != 0 )
		{
			ebp20 = eax1->pCraft;
		}
	}

...

	if ((ebp20->XwaCraft_m181 & 64) != 0)
	{
		unsigned short edi = ebp20->PresetThrottle / 0x28F;

		if( ebp20->IsSlamEnabled != 0 )
		{
			edi *= 2;
		}
	}
*/

/*
HOW TO SHARE DATA BETWEEN HOOKS:

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
angle: atan(1080 / 1152) * 2 / pi * 180 = 86.3�

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
#include "globals.h"
#include "..\shaders\material_defs.h"
#include "DeviceResources.h"
#include "Direct3DDevice.h"
#include "Direct3DExecuteBuffer.h"
#include "Direct3DTexture.h"
#include "BackbufferSurface.h"
#include "FrontbufferSurface.h"
#include "OffscreenSurface.h"
#include "ExecuteBufferDumper.h"
#include "XwaD3dRendererHook.h"
#include "PrimarySurface.h"
// TODO: Remove later
#include "TextureSurface.h"

#include <ScreenGrab.h>
#include <WICTextureLoader.h>
#include <wincodec.h>
#include <vector>
//#include <assert.h>

#include "effects.h"
#include "commonVR.h"
#include "FreePIE.h"
#include "SteamVR.h"
#include "DirectSBS.h"
#include "VRConfig.h"
#include "EffectsRenderer.h"
#include "Matrices.h"

#include "XWAObject.h"

#include "..\shaders\shader_common.h"


#define DBG_MAX_PRESENT_LOGS 0

#include "SharedMem.h"

#include "XWAFramework.h"

//Array<XwaPlanet, 104> s_XwaPlanets
XwaPlanet* g_XwaPlanets = (XwaPlanet*)0x005B1140;
//Array<TieFlightGroupEx, 192> s_XwaTieFlightGroups;
TieFlightGroup* g_XwaTieFlightGroups = (TieFlightGroup*)0x0080DC80;
//Array<ExeObjectEntry, 557> s_ExeObjectsTable;
const ExeEnableEntry* g_ExeObjectsTable = (ExeEnableEntry*)0x005FB240; // (Not sure about the type in this one)

FILE *g_HackFile = NULL;

float RealVertFOVToRawFocalLength(float real_FOV);

void GetCraftViewMatrix(Matrix4 *result);

inline void backProjectMetric(float sx, float sy, float rhw, Vector3 *P);
inline void backProjectMetric(UINT index, Vector3 *P);
Vector3 projectMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR);
inline Vector3 projectToInGameOrPostProcCoordsMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR = false);
float3 InverseTransformProjectionScreen(float4 pos);

void ResetObjectIndexMap();
void ReloadInterdictionMap();

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
// Replace regular explosions with procedural explosions during the DS2 mission to improve visibility.
bool g_bDS2ForceProceduralExplosions = false;

/*********************************************************/
// High Resolution Timers
HiResTimer g_HiResTimer;

/*********************************************************/
// HYPERSPACE
HyperspacePhaseEnum g_HyperspacePhaseFSM = HS_INIT_ST;
int g_iHyperExitPostFrames = 0;
//Vector3 g_fCameraCenter(0.0f, 0.0f, 0.0f);
float g_fHyperspaceTunnelSpeed = 5.5f, g_fHyperShakeRotationSpeed = 1.0f, g_fHyperLightRotationSpeed = 1.0f, g_fHyperspaceRand = 0.0f;
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
//int g_iHyperStateOverride = HS_HYPER_ENTER_ST;
int g_iHyperStateOverride = HS_HYPER_TUNNEL_ST;
//int g_iHyperStateOverride = HS_HYPER_EXIT_ST;
//int g_iHyperStateOverride = HS_POST_HYPER_EXIT_ST;
// DEBUG

XWALightInfo g_XWALightInfo[MAX_XWA_LIGHTS];
//void InitHeadingMatrix();
//Matrix4 GetCurrentHeadingMatrix(Vector4 &Rs, Vector4 &Us, Vector4 &Fs, bool invert, bool debug);
Matrix4 GetCurrentHeadingViewMatrix();
Matrix4 GetSimpleDirectionMatrix(Vector4 Fs, bool invert);

D3D11_VIEWPORT g_nonVRViewport{};

VertexShaderMatrixCB			g_VSMatrixCB;
VertexShaderCBuffer				g_VSCBuffer;
PixelShaderCBuffer				g_PSCBuffer;
DCPixelShaderCBuffer			g_DCPSCBuffer;
ShadertoyCBuffer				g_ShadertoyBuffer;
LaserPointerCBuffer				g_LaserPointerBuffer;
ShadowMapVertexShaderMatrixCB	g_ShadowMapVSCBuffer;
MetricReconstructionCB			g_MetricRecCBuffer;

// In reality, there should be a different factor per in-game resolution; but for now this should be enough
const float C = 1.0f, Z_FAR = 1.0f;
const float LOG_K = 1.0f;


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
bool g_bAutoGreeblesEnabled = true;
bool g_bShowBlastMarks = true;
float g_fBlastMarkOfsX = 0.0f, g_fBlastMarkOfsY = 0.0f;

SmallestK g_LaserList;
bool g_bEnableLaserLights = false;
bool g_bEnableExplosionLights = true;
bool g_b3DSunPresent = false;
bool g_b3DSkydomePresent = false;

// Force Cockpit Damage
bool g_bApplyCockpitDamage = false, g_bResetCockpitDamage = false, g_bResetCockpitDamageInHangar = false;

// Custom HUD colors
uint32_t g_iHUDInnerColor = 0, g_iHUDBorderColor = 0;
uint32_t g_iOriginalHUDInnerColor = 0, g_iOriginaHUDBorderColor = 0;

// Laser/Ion Cannon counting vars
bool g_bLasersIonsNeedCounting = false;
int g_iNumLaserCannons = 0, g_iNumIonCannons = 0;

// Sun Colors, to be used to apply colors to the flares later
float4 g_SunColors[MAX_SUN_FLARES];
int g_iSunFlareCount = 0;
void RenderSkyBox();

float clamp(float val, float min, float max);

/*
 * Converts a metric (OPT-scale?) depth value to in-game (sz, rhw) values, copying the behavior of the game.
 * This is the old formula. Jeremy modified it to improve the precision, but it's still a good reference
 * because this is what the game did originally.
 */
void ZToDepthRHW(float Z, float *sz_out, float *rhw_out)
{
	float rhw = Z;
	float *Znear = (float *)0x08B94CC;
	float *Zfar = (float *)0x05B46B4;
	//log_debug("[DBG] nearZ: %0.3f, farZ: %0.3f", *nearZ, *farZ);
	if (rhw < 0.0f)
		rhw = *Znear;
	
	float sz = (rhw * *Zfar) / (rhw * *Zfar + *Znear);

	if (sz < 1.52590219E-05f)
		sz = 1.52590219E-05f;

	// s_V0x064D1A8[s_V0x06628E0].z = st1;
	*sz_out = sz;
	//s_V0x064D1A8[s_V0x06628E0].rhw = st0;
	*rhw_out = rhw;
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
		g_ShadowMapVSCBuffer.sm_minZ[i] = 0.0f;
		g_ShadowMapVSCBuffer.sm_maxZ[i] = DEFAULT_COCKPIT_SHADOWMAP_MAX_Z; // Regular range for the cockpit
	}
	g_bBackdropsReset = true;
	g_iBackdropsTagged = 0;
	g_iBackdropsToTag = 0;
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

static bool IsInTechLibrary()
{
	int currentGameState = *(int*)(0x09F60E0 + 0x25FA9);
	int updateCallback = *(int*)(0x09F60E0 + 0x25FB1 + currentGameState * 0x850 + 0x0844);

	int XwaTechLibraryGameStateUpdate = 0x00574D70;
	bool isInTechLibrary = updateCallback == XwaTechLibraryGameStateUpdate;

	return isInTechLibrary;
}

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

		this->MonoRendering = FALSE;

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

	BOOL GetMonoRendering()
	{
		return this->MonoRendering;
	}

	inline void SetMonoRendering(BOOL monoRendering)
	{
		this->MonoRendering = monoRendering;
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

	BOOL MonoRendering;

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
	LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
	IUnknown* pUnkOuter
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

/* Function to quickly enable/disable ZWrite. */
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
		triangle++;
	}
	if (debug)
		log_debug("[DBG] END Geom");
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
	float &t, Vector3 &P, float &u, float &v, float margin)
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
	//if (u < 0 || u > 1) return false;
	if (u < 0.0f - margin || u > 1.0f + margin) return false;

	Vector3 qvec = tvec.cross(v0v1);
	v = dir.dot(qvec) * invDet;
	//if (v < 0 || u + v > 1) return false;
	if (v < 0.0f - margin || u + v > 1.0f + margin) return false;

	t = v0v2.dot(qvec) * invDet;
	// Prevent intersections behind the origin
	if (t < 0.0f) return false;

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

// Given segment AB and Point P, compute the closest point Q
// on segment AB. Also return t where Q = A + t * (B - A)
// from: https://gdbooks.gitbooks.io/3dcollisions/content/Chapter1/closest_point_on_line.html
inline Vector3 ClosestPointOnLine(const Vector3& a, const Vector3& b, Vector3 P, float& t)
{
	Vector3 ab = b - a;
	// Project c onto ab, computing the paramaterized position
	// d(t) = a + t * (b - a)
	t = (P - a).dot(ab) / ab.dot(ab);

	// Clamp T to a 0-1 range. If t was < 0 or > 1
	// then the closest point was outside the line!
	t = clamp(t, 0.0f, 1.0f);

	// Compute the projected position from the clamped t
	return a + t * ab;
}

/// <summary>
/// Variation of rayTriangleIntersect. This code will find the closest point on the triangle
/// to point orig and return its distance
/// </summary>
float ClosestPointOnTriangle(
	const Vector3& orig, const Vector3& v0, const Vector3& v1, const Vector3& v2,
	Vector3& P, float& u, float& v, float margin)
{
	// Variation of rayTriangleIntersect. This code will find the closest point on the triangle
	// to point orig and return its distance
	// From: https://www.scratchapixel.com/code.php?id=9&origin=/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle
	Vector3 v0v1 = v1 - v0;
	Vector3 v0v2 = v2 - v0;
	// Compute the normal
	Vector3 dir = v0v1.cross(v0v2);
	Vector3 pvec = dir.cross(v0v2);
	float det = v0v1.dot(pvec);
	// ray and triangle are parallel if det is close to 0
	if (fabs(det) < 0.00001f /* kEpsilon */) return FLT_MAX;

	bool inside = true;
	float t;
	float invDet = 1.0f / det;

	Vector3 tvec = orig - v0;
	u = tvec.dot(pvec) * invDet;
	//if (u < 0 || u > 1) return false;
	if (u < 0.0f - margin || u > 1.0f + margin) inside = false;

	Vector3 qvec = tvec.cross(v0v1);
	v = dir.dot(qvec) * invDet;
	//if (v < 0 || u + v > 1) return false;
	if (v < 0.0f - margin || u + v > 1.0f + margin) inside = false;

	t = v0v2.dot(qvec) * invDet;
	// Prevent intersections behind the origin
	//if (t < 0.0f) return false;
	P = orig + t * dir;

	if (!inside)
	{
		// The intersection is outside the triangle. Compute the distance between
		// P and all the line segments making up the triangle and return the closest
		// one to P.
		const Vector3 P01 = ClosestPointOnLine(v0, v1, P, t);
		const Vector3 P12 = ClosestPointOnLine(v1, v2, P, t);
		const Vector3 P20 = ClosestPointOnLine(v2, v0, P, t);
		const float d01 = (P - P01).length();
		const float d12 = (P - P12).length();
		const float d20 = (P - P20).length();
		const float d = min(d01, min(d12, d20));
		if (d == d01)
			P = P01;
		else if (d == d12)
			P = P12;
		else
			P = P20;
	}

	// P is now inside the triangle or on one of its edges. Let's compute its
	// barycentric coords
	Vector3 C;
	// edge 1
	Vector3 edge1 = v2 - v1;
	Vector3 vp1 = P - v1;
	C = edge1.cross(vp1);
	u = dir.dot(C);

	// edge 2
	Vector3 edge2 = v0 - v2;
	Vector3 vp2 = P - v2;
	C = edge2.cross(vp2);
	v = dir.dot(C);

	float denom = dir.dot(dir);
	u /= denom;
	v /= denom;

	return (orig - P).length();
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

inline void InverseTransformProjectionScreen(UINT index, Vector3 *P, bool invertZ=false) {
	float4 input;
	input.x = g_OrigVerts[index].sx;
	input.y = g_OrigVerts[index].sy;
	input.z = g_OrigVerts[index].sz;
	input.w = invertZ ? (1.0f - g_OrigVerts[index].rhw) : g_OrigVerts[index].rhw;
	float3 pos = InverseTransformProjectionScreen(input);
	P->x = pos.x * OPT_TO_METERS;
	P->y = pos.y * OPT_TO_METERS;
	P->z = pos.z * OPT_TO_METERS;
}

/*
 * Fully-metric projection: This is simply the inverse of the backProjectMetric code
 * INPUT: 
 *		OPT-scale metric 3D centered on the current camera.
 * OUTPUT:
 *		(regular and VR paths): post-proc UV coords. (Confirmed by rendering the XWA lights
 *		in the external HUD shader).
 */
Vector3 projectMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR) {
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
// Setting this flag to true will dump the explosions and engine glows. To set it
// add dump_OBJ_enabled = 1 in SSAO.cfg
bool g_bDumpOBJEnabled = false;
FILE *g_DumpOBJFile = NULL, *g_DumpLaserFile = NULL;
int g_iOBJFileIdx = 0;
int g_iDumpOBJIdx = 1, g_iDumpLaserOBJIdx = 1, g_iDumpFaceIdx = 1;

void DumpVerticesToOBJ(FILE *file, LPD3DINSTRUCTION instruction, UINT curIndex, int &OBJIdx, char *name=nullptr, bool invertZ=false)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector3 tempv0, tempv1, tempv2;
	//Vector2 v0, v1, v2;
	//float w0, w1, w2;
	std::vector<int> indices;

	if (file == NULL) {
		log_debug("[DBG] Cannot dump vertices, NULL file ptr");
		return;
	}

#define METRIC 1

	// DEPTH-BUFFER-CHANGE DONE
	// Map Icons have an "inverted Z". Look in VertexShader.hlsl for details. Here this "fix" is applied
	// when invertZ is true.
	// Start a new object
	if (name != nullptr) fprintf(file, "# %s\n", name);
	float *Znear = (float *)0x08B94CC;
	float *Zfar = (float *)0x05B46B4;
	float projectionDeltaX = *(float*)0x08C1600 + *(float*)0x0686ACC;
	float projectionDeltaY = *(float*)0x080ACF8 + *(float*)0x07B33C0 + *(float*)0x064D1AC;
#if METRIC == 1
	fprintf(file, "# NEW DDRAW, METRIC\n");
#else
	fprintf(file, "# NEW DDRAW, RAW\n");
#endif
	fprintf(file, "# (Znear) 0x08B94CC: %0.6f, (Zfar) 0x05B46B4: %0.6f\n", *Znear, *Zfar);
	fprintf(file, "# projDeltaX,Y: %0.6f, %0.6f\n", projectionDeltaX, projectionDeltaY);
	fprintf(file, "# viewportScale: %0.6f, %0.6f, %0.6f\n",
		g_VSCBuffer.viewportScale[0], g_VSCBuffer.viewportScale[1], g_VSCBuffer.viewportScale[2]);
	fprintf(file, "o OBJ_%d\n", OBJIdx);
	indices.clear();
	for (uint32_t i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		//v0.x = g_OrigVerts[index].sx; v0.y = g_OrigVerts[index].sy; w0 = 1.0f / g_OrigVerts[index].rhw;
#if METRIC == 1
		//backProjectMetric(index, &tempv0);
		InverseTransformProjectionScreen(index, &tempv0, invertZ);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv0.x, tempv0.y, tempv0.z);
#else
		fprintf(file, "v %0.6f %0.6f %0.6f  # %0.6f\n",
			g_OrigVerts[index].sx, g_OrigVerts[index].sy, g_OrigVerts[index].sz, g_OrigVerts[index].rhw);
#endif

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		//v1.x = g_OrigVerts[index].sx; v1.y = g_OrigVerts[index].sy; w1 = 1.0f / g_OrigVerts[index].rhw;
#if METRIC == 1
		//backProjectMetric(index, &tempv1);
		InverseTransformProjectionScreen(index, &tempv1, invertZ);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv1.x, tempv1.y, tempv1.z);
#else
		fprintf(file, "v %0.6f %0.6f %0.6f  # %0.6f\n",
			g_OrigVerts[index].sx, g_OrigVerts[index].sy, g_OrigVerts[index].sz, g_OrigVerts[index].rhw);
#endif

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		//v2.x = g_OrigVerts[index].sx; v2.y = g_OrigVerts[index].sy; w2 = 1.0f / g_OrigVerts[index].rhw;
#if METRIC == 1
		//backProjectMetric(index, &tempv2);
		InverseTransformProjectionScreen(index, &tempv2, invertZ);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv2.x, tempv2.y, tempv2.z);
#else
		fprintf(file, "v %0.6f %0.6f %0.6f  # %0.6f\n",
			g_OrigVerts[index].sx, g_OrigVerts[index].sy, g_OrigVerts[index].sz, g_OrigVerts[index].rhw);
#endif

		if (!g_config.D3dHookExists) triangle++;

		indices.push_back(g_iDumpFaceIdx++);
		indices.push_back(g_iDumpFaceIdx++);
		indices.push_back(g_iDumpFaceIdx++);
	}
	fprintf(file, "\n");

	// Write the face data
	for (uint32_t i = 0; i < indices.size(); i += 3)
	{
		fprintf(file, "f %d %d %d\n", indices[i], indices[i + 1], indices[i + 2]);
	}
	fprintf(file, "\n");

	OBJIdx++;
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

void Direct3DDevice::AddLaserLights(LPD3DINSTRUCTION instruction, UINT curIndex, Direct3DTexture *texture)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector3 tempv0, tempv1, tempv2, P;
	Vector2 UV0, UV1, UV2, UV = texture->material.LightUVCoordPos;

	// XWA batch renders all lasers that share the same texture, so we may see several
	// lasers when parsing this instruction. To detect each individual laser, we need
	// to look at the uv's and see if the current triangle contains the uv coord we're
	// looking for (the default is (0.1, 0.5)). If the uv is contained, then we compute
	// the 3D point using its barycentric coords and add it to the current list.
	for (uint32_t i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		UV0.x = g_OrigVerts[index].tu; UV0.y = g_OrigVerts[index].tv;
		backProjectMetric(index, &tempv0);
		if (g_bEnableVR) tempv0.y = -tempv0.y;

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		UV1.x = g_OrigVerts[index].tu; UV1.y = g_OrigVerts[index].tv;
		backProjectMetric(index, &tempv1);
		if (g_bEnableVR) tempv1.y = -tempv1.y;

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		UV2.x = g_OrigVerts[index].tu; UV2.y = g_OrigVerts[index].tv;
		backProjectMetric(index, &tempv2);
		if (g_bEnableVR) tempv2.y = -tempv2.y;

		float u, v;
		if (IsInsideTriangle(UV, UV0, UV1, UV2, &u, &v)) {
			P = tempv0 + u * (tempv2 - tempv0) + v * (tempv1 - tempv0);
			g_LaserList.insert(P, texture->material.Light);
		}

		if (!g_config.D3dHookExists) triangle++;
	}
}

void Direct3DDevice::AddExplosionLights(LPD3DINSTRUCTION instruction, UINT curIndex, Direct3DTexture *texture)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector3 tempv0, tempv1, tempv2, P;
	Vector2 UV0, UV1, UV2, UV = texture->material.LightUVCoordPos;
	Vector3 Light = texture->material.Light;
	float falloff = texture->material.LightFalloff;
	if (abs(Light.x) <= 0.0001f && abs(Light.y) <= 0.0001f && abs(Light.z) <= 0.0001f) {
		// This explosion's light and UV are not initialized. Provide some
		// default values...

		// We don't use (0.5, 0.5) because this coordinate will be contained in both
		// triangles making up the explosion quad
		UV = Vector2(0.45f, 0.45f);
		Light = DEFAULT_EXPLOSION_COLOR;

		// ... then write these values back to the texture
		texture->material.LightUVCoordPos = UV;
		texture->material.Light = Light;
	}

	// If the falloff isn't set for this explosion, then set a default value
	if (falloff == 0.0f) {
		falloff = DEFAULT_DYNAMIC_LIGHT_FALLOFF;
		texture->material.LightFalloff = falloff;
	}

	// XWA probably batch-renders all explosions that share the same texture, so we may
	// see several explosions when parsing this instruction. To detect each individual
	// explosion, we need to look at the uv's and see if the current triangle contains
	// the uv coord we're looking for (the default is (0.45, 0.45)). If the uv is contained,
	// then we compute the 3D point using its barycentric coords and add it to the current
	// list.
	for (uint32_t i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		UV0.x = g_OrigVerts[index].tu; UV0.y = g_OrigVerts[index].tv;
		InverseTransformProjectionScreen(index, &tempv0);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		UV1.x = g_OrigVerts[index].tu; UV1.y = g_OrigVerts[index].tv;
		InverseTransformProjectionScreen(index, &tempv1);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		UV2.x = g_OrigVerts[index].tu; UV2.y = g_OrigVerts[index].tv;
		InverseTransformProjectionScreen(index, &tempv2);

		float u, v;
		if (IsInsideTriangle(UV, UV0, UV1, UV2, &u, &v)) {
			P = tempv0 + u * (tempv2 - tempv0) + v * (tempv1 - tempv0);
			// Don't add lights that are too close to the origin: they'll produce an annoying
			// white flash
			if (fabs(P.x > 2.5f) && fabs(P.y > 2.5f) && fabs(P.z > 2.5f))
				g_LaserList.insert(P, Light, {}, falloff, 0.0f);
		}

		if (!g_config.D3dHookExists) triangle++;
	}
}

/*
 * Compute the effective screen limits in uv coords when rendering effects using a
 * full-screen quad. This is used in SSDO to get the effective viewport limits in
 * uv-coords (it's also used for other effects, like the hyperspace effect). Pixels
 * outside the uv-coords computed here should be black.
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
		// Some effects, like the hyperspace effect, cover a few more pixels on the last column and row
		// causing an artifact where the effect -- which should be in the background -- shows through
		// the edges of the cockpit. This is most likely due to roundoff errors. To compensate for that,
		// we're substracting 2 pixels from the right/bottom so that the effective viewport is slightly
		// smaller, preventing the background from "showing through". This problem was reported by Angel
		// from the TFTC project.
		*x1 = (x - 2.0f) / g_fCurScreenWidth;
		*y1 = (y - 2.0f) / g_fCurScreenHeight;
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
inline ID3D11RenderTargetView *Direct3DDevice::SelectOffscreenBuffer() {
	auto& resources = this->_deviceResources;

	ID3D11RenderTargetView* regularRTV = resources->_renderTargetView.Get();
	// Since we're now splitting the background and the 3D content, we don't need the shadertoyRTV
	// anymore. When hyperspace is activated, no external OPTs are rendered, so we still just get
	// the cockpit on the regularRTV
	if (resources->_overrideRTV == TRANSP_LYR_1) return resources->_transp1RTV;
	if (resources->_overrideRTV == TRANSP_LYR_2) return resources->_transp2RTV;
	if (resources->_overrideRTV == BACKGROUND_LYR) return resources->_backgroundRTV;
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

inline void UpdateDCHologramState() {
	if (!g_bDCHologramsVisiblePrev && g_bDCHologramsVisible) {
		g_fDCHologramFadeIn = 0.0f;
		g_fDCHologramFadeInIncr = 0.04f;
	} else if (g_bDCHologramsVisiblePrev && !g_bDCHologramsVisible) {
		g_fDCHologramFadeIn = 1.0f;
		g_fDCHologramFadeInIncr = -0.04f;
	}
	//g_fDCHologramTime += 0.0166f;
	g_fDCHologramTime += g_HiResTimer.elapsed_s;
	g_fDCHologramFadeIn += g_fDCHologramFadeInIncr;
	if (g_fDCHologramFadeIn > 1.0f) g_fDCHologramFadeIn = 1.0f;
	if (g_fDCHologramFadeIn < 0.0f) g_fDCHologramFadeIn = 0.0f;
	g_bDCHologramsVisiblePrev = g_bDCHologramsVisible;
}

uint32_t Direct3DDevice::GetWarningLightColor(LPD3DINSTRUCTION instruction, UINT curIndex, Direct3DTexture *texture)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index, color = 0;
	UINT idx = curIndex;

	for (uint32_t i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		color = (uint32_t)g_OrigVerts[index].color;
		break;
		//index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;

		//index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;

		//if (!g_config.D3dHookExists) triangle++;
	}
	//log_debug("[DBG] GetWarningLightColor, wCount: %d, color: 0x%x", instruction->wCount, color);
	//log_debug("[DBG] HUD colors: 0x%x, 0x%x", *g_XwaFlightHudColor, *g_XwaFlightHudBorderColor);

	// The green warning light is encoded as: 0xff 10bc00 (ARGB)
	// The yellow warning light is encoded as: 0xff fcd400 (ARGB)
	// The red warning light is encoded as: 0xff ff0000 (ARGB)
	// I haven't seen the color of the beam warning light yet.
	// Here we check the color of the current instruction against the current HUD color. If they are
	// different, then we know the event for the current warning light has been activated. Otherwise,
	// we'll just return 0 to signal that no event happened.
	if (color == *g_XwaFlightHudColor)
		color = 0;

	return color;
}

void CountLasersAndIons(CraftInstance *craftInstance) {
	g_iNumLaserCannons = 0;
	g_iNumIonCannons = 0;
	// A probably better way to count the cannons would be to figure out how many
	// cannons per set we have. Something like:
	// set 0: 4 lasers
	// set 1: 2 ions
	// set 2: 2 lasers
	// This would be a somewhat exotic setup, but we might need this and the only way
	// would be to detect when we switch from one type of cannon to another.
	for (int i = 0; i < craftInstance->NumberOfLasers; i++) {
		switch (craftInstance->Hardpoints[i].WeaponType) {
		case 1:
			g_iNumLaserCannons++;
			break;
		case 2:
			g_iNumIonCannons++;
			break;
		}
	}
	log_debug("[DBG] Num Laser Cannons: %d, Num Ion Cannons: %d, TotalLasers: %d",
		g_iNumLaserCannons, g_iNumIonCannons, craftInstance->NumberOfLasers);
	g_bLasersIonsNeedCounting = false;
}

void Direct3DDevice::UpdateReconstructionConstants()
{
	auto &resources = _deviceResources;

	g_VSMatrixCB.Znear = *(float*)0x08B94CC; // Znear, we know because g_fRawFOVDist is this exact same value!
	g_VSMatrixCB.Zfar = *(float*)0x05B46B4; // Zfar
	g_VSMatrixCB.DeltaX = *(float*)0x08C1600 + *(float*)0x0686ACC;
	g_VSMatrixCB.DeltaY = *(float*)0x080ACF8 + *(float*)0x07B33C0 + *(float*)0x064D1AC;

	/*
	log_debug("[DBG]   Znear,far: %0.3f, %0.3f, DeltaX,Y: %0.3f, %0.3f",
		g_VSMatrixCB.Znear, g_VSMatrixCB.Zfar,
		g_VSMatrixCB.DeltaX, g_VSMatrixCB.DeltaY);
	log_debug("[DBG]   vpScale: %0.6f, %0.6f, %0.6f", 
		g_VSMatrixCB.origViewport[0], g_VSMatrixCB.origViewport[1], g_VSMatrixCB.origViewport[2]);
	*/
}

HRESULT Direct3DDevice::Execute(
	LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
	LPDIRECT3DVIEWPORT lpDirect3DViewport,
	DWORD dwFlags
)
{
	bool bStateD3dAnnotationOpen = false; // To keep track of the d3dannotation event state so that we close it at the end of Execute()
	_deviceResources->BeginAnnotatedEvent(L"Execute");

#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

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
		g_HyperspacePhaseFSM = (HyperspacePhaseEnum)g_iHyperStateOverride;
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
			char sFileNameOBJ[80], sFileNameLaser[80];
			sprintf_s(sFileNameOBJ, 80, "./DumpVertices%03d.OBJ", g_iOBJFileIdx);
			sprintf_s(sFileNameLaser, 80, "./DumpVerticesLaser%03d.OBJ", g_iOBJFileIdx);
			log_debug("[DBG] [SHW] g_iOBJFileIdx: %03d", g_iOBJFileIdx);
			g_iOBJFileIdx++;

			if (g_DumpOBJFile != NULL)
				fclose(g_DumpOBJFile);
			if (g_DumpLaserFile != NULL)
				fclose(g_DumpLaserFile);

			fopen_s(&g_DumpOBJFile, sFileNameOBJ, "wt");
			fopen_s(&g_DumpLaserFile, sFileNameLaser, "wt");
			g_iDumpOBJIdx = 1; g_iDumpLaserOBJIdx = 1; g_iDumpFaceIdx = 1;
			log_debug("[DBG] [SHW] sm_FOVscale: %0.3f", g_ShadowMapVSCBuffer.sm_FOVscale);
			log_debug("[DBG] [SHW] sm_y_center: %0.3f", g_ShadowMapVSCBuffer.sm_y_center);
			log_debug("[DBG] [SHW] g_fOBJMetricMult: %0.3f", g_fOBJ_Z_MetricMult);
		}
		// The following texture has the sub-component bracket in in-game coordinates:
		DirectX::SaveWICTextureToFile(context, resources->_mainDisplayTexture, GUID_ContainerFormatPng,
			L"C:\\Temp\\_DCTexViewSubCMD.png");
	}

	// Apply the Edge Detector effect to the DC foreground texture
	// The Edge Detector is now executed in PrimarySurface.cpp::Flip(), right after
	// _offscreenAsInputDynCockpit is resolved.

	HRESULT hr = S_OK;
	UINT width, height, left, top;
	float scale;
	UINT vertexBufferStride = sizeof(D3DTLVERTEX), vertexBufferOffset = 0;
	D3D11_VIEWPORT viewport;
	bool bModifiedShaders = false, bModifiedPixelShader = false, bZWriteEnabled = false;
	bool bModifiedVertexShader = false;
	float FullTransform = g_bEnableVR && g_bInTechRoom ? 1.0f : 0.0f;

	g_VSCBuffer = { 0 };
	g_VSCBuffer.aspect_ratio      =  g_bRendering3D ? g_fAspectRatio : g_fConcourseAspectRatio;
	g_SSAO_PSCBuffer.aspect_ratio =  g_VSCBuffer.aspect_ratio;
	g_VSCBuffer.z_override        = -1.0f;
	g_VSCBuffer.sz_override       = -1.0f;
	g_VSCBuffer.mult_z_override	  = -1.0f;
	g_VSCBuffer.apply_uv_comp     =  false;
	g_VSCBuffer.bPreventTransform =  0.0f;
	g_VSCBuffer.bFullTransform    =  FullTransform;
	g_VSCBuffer.scale_override    =  1.0f;
	g_VSCBuffer.s_V0x08B94CC      = *(float*)0x08B94CC;
	g_VSCBuffer.s_V0x05B46B4      = *(float*)0x05B46B4;

	g_PSCBuffer = { 0 };
	g_PSCBuffer.brightness      = MAX_BRIGHTNESS;
	g_PSCBuffer.fBloomStrength  = 1.0f;
	g_PSCBuffer.fPosNormalAlpha = 1.0f;
	g_PSCBuffer.fSSAOAlphaMult  = g_fSSAOAlphaOfs;
	g_PSCBuffer.fSSAOMaskVal    = g_DefaultGlobalMaterial.Metallic * 0.5f;
	g_PSCBuffer.fGlossiness     = g_DefaultGlobalMaterial.Glossiness;
	g_PSCBuffer.fSpecInt        = g_DefaultGlobalMaterial.Intensity;  // DEFAULT_SPEC_INT;
	g_PSCBuffer.fNMIntensity    = g_DefaultGlobalMaterial.NMIntensity;
	g_PSCBuffer.AuxColor.x		= 1.0f;
	g_PSCBuffer.AuxColor.y		= 1.0f;
	g_PSCBuffer.AuxColor.z		= 1.0f;
	g_PSCBuffer.AuxColor.w		= 1.0f;
	g_PSCBuffer.AspectRatio		= 1.0f;
		
	g_DCPSCBuffer = { 0 };
	g_DCPSCBuffer.ct_brightness	= g_fCoverTextureBrightness;
	g_DCPSCBuffer.dc_brightness = g_fDCBrightness;

	const char* step = "";

	this->_deviceResources->InitInputLayout(resources->_inputLayout);
	if (g_bEnableVR)
		this->_deviceResources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
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
		/*
		// New constants added with the D3DRendererHook:
		const float viewportScale[8] =
		{
			2.0f / (float)this->_deviceResources->_displayWidth,
			-2.0f / (float)this->_deviceResources->_displayHeight,
			scale,
			0,
			_IsXwaExe ? *(float*)0x08B94CC : 0,
			_IsXwaExe ? *(float*)0x05B46B4 : 0,
			0,
			0
		};
		*/

		// New constants added with the D3DRendererHook:
		g_VSCBuffer.s_V0x08B94CC = *(float*)0x08B94CC;
		g_VSCBuffer.s_V0x05B46B4 = *(float*)0x05B46B4;
		// This setting is no longer applied here. Look below, after we're done with the
		// state management instead. The reason for this change is because engine glows
		// have a better depth if we add 0x05B46B4 -- but we can't add this for the GUI
		// elements, so we need to know when we're not rendering GUI. Hence, we can only
		// set this after we're done with state management.
		//g_VSCBuffer.s_V0x05B46B4_Offset = IsInTechLibrary() ? *(float*)0x05B46B4 : 0;
		g_VSCBuffer.ProjectionParameters.x = g_config.ProjectionParameterA;
		g_VSCBuffer.ProjectionParameters.y = g_config.ProjectionParameterB;
		g_VSCBuffer.ProjectionParameters.z = g_config.ProjectionParameterC;
		if (g_bEnableVR && !g_bInTechRoom) {
			// The Tech Room needs the regular viewportscale below in VR
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
		g_VSCBuffer.aspect_ratio      =  g_bRendering3D ? g_fAspectRatio : g_fConcourseAspectRatio;
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
					case D3DRENDERSTATE_MONOENABLE:
					{
						this->_renderStates->SetMonoRendering((BOOL)state->dwArg[0]);
						break;
					}

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
				// TODO
				//if (this->_renderStates->GetMonoRendering())
				//{
				//	currentIndexLocation += 3 * instruction->wCount;
				//	break;
				//}

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
					  - Draw 3D objects, including the cockpit (this is now done in the D3dRenderHook)
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

				const bool bExternalCamera = (bool)PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
				/* 
				 * ZWriteEnabled is false when rendering the background starfield or when
				 * rendering the GUI elements -- except that the targetting computer GUI
				 * is rendered with ZWriteEnabled == true. This is probably done to clear
				 * the depth stencil in the area covered by the targeting computer, so that
				 * later, when the targeted object is rendered, it can render without
				 * interfering with the z-values of the cockpit.
				 */
				bZWriteEnabled = this->_renderStates->GetZWriteEnabled();
				/* If we have drawn at least one Floating GUI element and now ZWrite has been enabled
				   again, then we're about to draw the floating 3D element. Although, g_bTargetCompDrawn
				   isn't fully semantically correct because it should be set to true *after* it has actually
				   been drawn. Here it's being set *before* it's drawn.
				   g_bTargetCompDrawn is also updated in EffectsRenderer::DoStateManagement() because
				   sometimes control jumps directly into the d3dhook from here.
				*/
				// You might be tempted to remove the bZWriteEnabled condition below, but doing so
				// messes up the state (the Hull and Shields indicator disappears).
				if (!g_bTargetCompDrawn && g_iFloatingGUIDrawnCounter > 0 && bZWriteEnabled)
					g_bTargetCompDrawn = true;
				// lastTextureSelected can be NULL. This happens when drawing the square
				// brackets around the currently-selected object (and maybe other situations)
				bool bSunCentroidComputed = false;
				Vector3 SunCentroid;
				Vector2 SunCentroid2D;
				const bool bLastTextureSelectedNotNULL = (lastTextureSelected != NULL);
				bool bIsLaser = false, bIsLightTexture = false, bIsText = false, bIsReticle = false, bIsReticleCenter = false;
				bool bIsGUI = false, bIsLensFlare = false, bIsSun = false, bIsMapIcon = false;
				bool bIsCockpit = false, bIsGunner = false, bIsExterior = false, bIsDAT = false;
				bool bIsActiveCockpit = false, bIsBlastMark = false, bIsTargetHighlighted = false;
				bool bIsHologram = false, bIsNoisyHolo = false, bIsTransparent = false, bIsDS2CoreExplosion = false;
				bool bWarheadLocked = PlayerDataTable[*g_playerIndex].warheadArmed && PlayerDataTable[*g_playerIndex].warheadLockState == 3;
				bool bIsElectricity = false, bIsExplosion = false, bHasMaterial = false, bIsEngineGlow = false;
				bool bIsHitEffect = false, bIsTrail = false;
				bool bDCElemAlwaysVisible = false;
				if (bLastTextureSelectedNotNULL) {
					if (g_bDynCockpitEnabled && lastTextureSelected->is_DynCockpitDst) 
					{
						int idx = lastTextureSelected->DCElementIndex;
						if (idx >= 0 && idx < g_iNumDCElements) {
							bIsHologram |= g_DCElements[idx].bHologram;
							bIsNoisyHolo |= g_DCElements[idx].bNoisyHolo;
							bIsTransparent |= g_DCElements[idx].bTransparent;
							bDCElemAlwaysVisible |= g_DCElements[idx].bAlwaysVisible;
						}
					}

					bIsLaser = lastTextureSelected->is_Laser || lastTextureSelected->is_TurboLaser;
					bIsLightTexture = lastTextureSelected->is_LightTexture;
					bIsText = lastTextureSelected->is_Text;
					bIsReticle = lastTextureSelected->is_Reticle;
					bIsReticleCenter = lastTextureSelected->is_ReticleCenter;
					g_bIsTargetHighlighted |= lastTextureSelected->is_HighlightedReticle;
					bIsTargetHighlighted = g_bIsTargetHighlighted || g_bPrevIsTargetHighlighted;
					if (bIsTargetHighlighted) g_GameEvent.TargetEvent = TGT_EVT_LASER_LOCK;
					if (PlayerDataTable[*g_playerIndex].warheadArmed) {
						char state = PlayerDataTable[*g_playerIndex].warheadLockState;
						switch (state) {
							// state == 0 warhead armed, no lock
						case 2:
							g_GameEvent.TargetEvent = TGT_EVT_WARHEAD_LOCKING;
							break;
						case 3:
							g_GameEvent.TargetEvent = TGT_EVT_WARHEAD_LOCKED;
							break;
						}
					}
					bIsGUI = lastTextureSelected->is_GUI;
					bIsLensFlare = lastTextureSelected->is_LensFlare;
					bIsSun = lastTextureSelected->is_Sun;
					bIsCockpit = lastTextureSelected->is_CockpitTex;
					bIsGunner = lastTextureSelected->is_GunnerTex;
					bIsExterior = lastTextureSelected->is_Exterior;
					bIsDAT = lastTextureSelected->is_DAT;
					bIsActiveCockpit = lastTextureSelected->ActiveCockpitIdx > -1;
					bIsBlastMark = lastTextureSelected->is_BlastMark;
					//bIsSkyDome = lastTextureSelected->is_SkydomeLight;
					bIsDS2CoreExplosion = lastTextureSelected->is_DS2_Reactor_Explosion;
					bIsMapIcon = lastTextureSelected->is_MapIcon;
					bIsElectricity = lastTextureSelected->is_Electricity;
					bHasMaterial = lastTextureSelected->bHasMaterial;
					bIsExplosion = lastTextureSelected->is_Explosion;
					if (bIsExplosion) g_bExplosionsDisplayedOnCurrentFrame = true;
					bIsEngineGlow = lastTextureSelected->is_EngineGlow;
					bIsHitEffect = lastTextureSelected->is_HitEffect;
					bIsTrail = lastTextureSelected->is_Trail;
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
					if ((g_bDynCockpitEnabled || g_bReshadeEnabled) && !g_bDCWasClearedOnThisFrame)
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

				const bool bRenderToDynCockpitBuffer = g_bDynCockpitEnabled &&
					bLastTextureSelectedNotNULL && g_bScaleableHUDStarted && g_bIsScaleableGUIElem;
				// Render HUD backgrounds to their own layer (HUD BG)
				const bool bRenderToDynCockpitBGBuffer = g_bDynCockpitEnabled &&
					bLastTextureSelectedNotNULL && lastTextureSelected->is_DC_HUDRegionSrc;

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
							if (PlayerDataTable[*g_playerIndex].MousePositionX != g_fLastCockpitCameraYaw ||
								PlayerDataTable[*g_playerIndex].MousePositionY != g_fLastCockpitCameraPitch)
								g_bHyperHeadSnapped = true;
							if (*numberOfPlayersInGame == 1) {
								PlayerDataTable[*g_playerIndex].MousePositionX = g_fLastCockpitCameraYaw;
								PlayerDataTable[*g_playerIndex].MousePositionY = g_fLastCockpitCameraPitch;
								// Restoring the cockpitX/Y/Z/Reference didn't seem to have any effect when cockpit inertia was on:
								//PlayerDataTable[*g_playerIndex].cockpitXReference = g_lastCockpitXReference;
								//PlayerDataTable[*g_playerIndex].cockpitYReference = g_lastCockpitYReference;
								//PlayerDataTable[*g_playerIndex].cockpitZReference = g_lastCockpitZReference;
							}
							g_fCockpitCameraYawOnFirstHyperFrame = g_fLastCockpitCameraYaw;
							g_fCockpitCameraPitchOnFirstHyperFrame = g_fLastCockpitCameraPitch;
							g_HyperspacePhaseFSM = HS_HYPER_ENTER_ST;
							//log_debug("[DBG] [FSM] HS_INIT_ST --> HS_HYPER_ENTER_ST");
							// Compute a new random seed for this hyperspace jump
							g_fHyperspaceRand = (float)rand() / (float)RAND_MAX;
							if (g_bLastFrameWasExterior) {
								// External view --> Cockpit transition for the hyperspace jump
								g_bHyperExternalToCockpitTransition = true;
							}
							//int region = PlayerDataTable[*g_playerIndex].currentRegion;
							// criticalMessageObjectIndex holds the destination region when a hyperjump is
							// initiated
							int region = PlayerDataTable[*g_playerIndex].criticalMessageObjectIndex;
							g_bInterdictionActive = false;
							g_ShadertoyBuffer.Style = g_iHyperStyle;
							ReloadInterdictionMap();
							if (g_iInterdictionBitfield)
							{
								log_debug("[DBG] [INT] Interdictions in current mission: 0x%x, dest region: %d",
									g_iInterdictionBitfield, region);
								if ((0x1 & (g_iInterdictionBitfield >> region)) != 0x0) {
									g_bInterdictionActive = true;
									log_debug("[DBG] [INT] INTERDICTION ACTIVATED");
								}
							}
						}
						break;
					case HS_HYPER_ENTER_ST:
						g_PSCBuffer.bInHyperspace = 1;
						g_bHyperspaceLastFrame = false;
						g_bHyperspaceTunnelLastFrame = false;
						g_ShadertoyBuffer.Style = g_iHyperStyle;
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
							//log_debug("[DBG] [FSM] HS_HYPER_ENTER_ST --> HS_HYPER_TUNNEL_ST");
							g_PSCBuffer.fBloomStrength = g_BloomConfig.fHyperTunnelStrength;
							// We're about to enter the hyperspace tunnel, change the color of the lights:
							float fade = 1.0f;
							for (int i = 0; i < 2; i++, fade *= 0.5f) {
								memcpy(&g_TempLightVector[i], &g_LightVector[i], sizeof(Vector4));
								memcpy(&g_TempLightColor[i], &g_LightColor[i], sizeof(Vector4));
								// This color is modified in RenderHyperspaceEffect if there's an interdiction
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
						g_ShadertoyBuffer.Style = g_bInterdictionActive ? HYPER_INTERDICTION_STYLE : g_iHyperStyle;
						if (PlayerDataTable[*g_playerIndex].hyperspacePhase == 3) {
							//log_debug("[DBG] [FSM] HS_HYPER_TUNNEL_ST --> HS_HYPER_EXIT_ST");
							g_bHyperspaceTunnelLastFrame = true;
							g_HyperspacePhaseFSM = HS_HYPER_EXIT_ST;
							//log_debug("[DBG] [FSM] HS_HYPER_TUNNEL_ST --> HS_HYPER_EXIT_ST");
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
						g_ShadertoyBuffer.Style = g_bInterdictionActive ? HYPER_INTERDICTION_STYLE : g_iHyperStyle;
						if (PlayerDataTable[*g_playerIndex].hyperspacePhase == 0) {
							g_iHyperExitPostFrames = 0;
							g_HyperspacePhaseFSM = HS_POST_HYPER_EXIT_ST;
							//log_debug("[DBG] [FSM] HS_HYPER_EXIT_ST --> HS_POST_HYPER_EXIT_ST");
							g_bHyperspaceLastFrame = true;
							// Reset the Sun --> XWA light association every time we exit hyperspace
							ResetXWALightInfo();
							ResetObjectIndexMap();
						}
						break;
					case HS_POST_HYPER_EXIT_ST:
						g_PSCBuffer.bInHyperspace = 1;
						g_bHyperspaceLastFrame = false;
						g_bHyperspaceTunnelLastFrame = false;
						g_ShadertoyBuffer.Style = g_bInterdictionActive ? HYPER_INTERDICTION_STYLE : g_iHyperStyle;
						if (g_iHyperExitPostFrames > MAX_POST_HYPER_EXIT_FRAMES) {
							g_HyperspacePhaseFSM = HS_INIT_ST;
							//log_debug("[DBG] [FSM] HS_POST_HYPER_EXIT_ST --> HS_INIT_ST");
						}
						break;
					}

//#ifdef HYPER_OVERRIDE
					// DEBUG
					if (g_bHyperDebugMode) {
						bModifiedShaders = true;
						g_HyperspacePhaseFSM = (HyperspacePhaseEnum)g_iHyperStateOverride;
						g_PSCBuffer.bInHyperspace = 1;
					}
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

					// For the D3DRendererHook, the lasers are rendered in a deferred fashion (through a draw call list)
					// The Execute() method is now called to render the background, the HUD and other stuff. So, if we reach
					// this point, we're about to render the HUD. We should render the lasers now, before we punch a hole in
					// the depth stencil to draw the miniature; and if there's nothing to render yet, no harm done!
					RenderDeferredDrawCalls();

					// Resolve the Depth Buffers
					_deviceResources->BeginAnnotatedEvent(L"ResolveDepthBuffers");
					if (g_bAOEnabled) {
						g_bDepthBufferResolved = true;
						context->ResolveSubresource(resources->_depthBufAsInput, 0, resources->_depthBuf, 0, AO_DEPTH_BUFFER_FORMAT);
						if (g_bUseSteamVR)
							context->ResolveSubresource(resources->_depthBufAsInput, D3D11CalcSubresource(0, 1, 1),
								resources->_depthBuf, D3D11CalcSubresource(0, 1, 1), AO_DEPTH_BUFFER_FORMAT);

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
					_deviceResources->EndAnnotatedEvent();

					// Capture the current-frame-so-far (cockpit+background) for the new hyperspace effect; but only if we're
					// not travelling through hyperspace:
					_deviceResources->BeginAnnotatedEvent(L"CaptureFrameForHyperspace");
					{
						if (g_bHyperDebugMode || g_HyperspacePhaseFSM == HS_INIT_ST || g_HyperspacePhaseFSM == HS_POST_HYPER_EXIT_ST)
						{
							g_fLastCockpitCameraYaw   = PlayerDataTable[*g_playerIndex].MousePositionX;
							g_fLastCockpitCameraPitch = PlayerDataTable[*g_playerIndex].MousePositionY;
							g_lastCockpitXReference   = PlayerDataTable[*g_playerIndex].Camera.ShakeX;
							g_lastCockpitYReference   = PlayerDataTable[*g_playerIndex].Camera.ShakeY;
							g_lastCockpitZReference   = PlayerDataTable[*g_playerIndex].Camera.ShakeZ;

							context->ResolveSubresource(resources->_shadertoyAuxBuf, 0, resources->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
							if (g_bUseSteamVR) {
								context->ResolveSubresource(
									resources->_shadertoyAuxBuf, D3D11CalcSubresource(0, 1, 1),
									resources->_offscreenBuffer, D3D11CalcSubresource(0, 1, 1), BACKBUFFER_FORMAT);
							}
						}
					}
					_deviceResources->EndAnnotatedEvent();
				}
				/*************************************************************************
					Special state management ends here
				 *************************************************************************/

				// Adding 0x05B46B4 to the engine glows, makes them easier to see. This is
				// probably restoring old behavior in the pre-d3d_hook ddraw anyway.
				g_VSCBuffer.s_V0x05B46B4_Offset = (IsInTechLibrary() || !g_bStartedGUI) ? *(float*)0x05B46B4 : 0;

				// Enable transparency for light textures when AO is disabled
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

				// Update the state of the warning lights
				if (bLastTextureSelectedNotNULL && lastTextureSelected->WarningLightType != NONE_WLIGHT) {
					uint32_t color = GetWarningLightColor(instruction, currentIndexLocation, lastTextureSelected);
					switch (lastTextureSelected->WarningLightType) {
						case RETICLE_LEFT_WLIGHT:
						case WARHEAD_RETICLE_LEFT_WLIGHT:
							g_GameEvent.WLightLLEvent = (color != 0);
							//log_debug("[DBG] WLightLLEvent --> %d", g_GameEvent.WLightLLEvent);
							break;
						case RETICLE_MID_LEFT_WLIGHT:
						case WARHEAD_RETICLE_MID_LEFT_WLIGHT:
							g_GameEvent.WLightMLEvent = (color != 0);
							//log_debug("[DBG] WLightMLEvent --> %d", g_GameEvent.WLightMLEvent);
							break;
						case RETICLE_MID_RIGHT_WLIGHT:
						case WARHEAD_RETICLE_MID_RIGHT_WLIGHT:
							g_GameEvent.WLightMREvent = (color != 0);
							//log_debug("[DBG] WLightMREvent --> %d", g_GameEvent.WLightMREvent);
							break;
						case RETICLE_RIGHT_WLIGHT:
						case WARHEAD_RETICLE_RIGHT_WLIGHT: {
							g_GameEvent.WLightRREvent = 0;
							//log_debug("[DBG] WLIGHT RR color: 0x%x", color);
							// If color is yellow (0xff fcd400) (ARGB), then assign 1
							if (color == 0xfffcd400) {
								g_GameEvent.WLightRREvent = 1;
								//log_debug("[DBG] WLightRREvent --> YELLOW");
							}
							// If color is red (0xff ff0000) (ARGB), assign 2
							else if (color == 0xffff0000) {
								g_GameEvent.WLightRREvent = 2;
								//log_debug("[DBG] WLightRREvent --> RED");
							}
							break;
						}
					}
				}

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

							// Get the limits for the throttle bar (as rendered by first principles)
							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[THROTTLE_BAR_DC_SRC_IDX];
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

							dcElemSrcBox = &g_DCElemSrcBoxes.src_boxes[EIGHT_LASERS_BOTH_SRC_IDX];
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

				// Dynamic Cockpit: Remove all the alpha overlays (lightmaps), unless there's an active
				// lightmap animation.
				if (g_bDCManualActivate && g_bDynCockpitEnabled &&
					bLastTextureSelectedNotNULL && lastTextureSelected->is_DynCockpitAlphaOverlay) 
				{
					if (!bHasMaterial) goto out;
					if (lastTextureSelected->material.GetCurrentATCIndex(NULL, LIGHTMAP_ATC_IDX) < 0) goto out;
					// This lightmap has a material and an animated light map, we can't skip this texture.
				}

				// Skip rendering the laser/ion energy levels and brackets if we're rendering them
				// from first principles.
				if (g_bRenderLaserIonEnergyLevels && bLastTextureSelectedNotNULL &&
					lastTextureSelected->is_LaserIonEnergy)
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
				bModifiedVertexShader = false;

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

				resources->_overrideRTV = TRANSP_LYR_NONE;
				// Missiles are rendered in two phases. There's an OPT which gets rendered in EffectsRenderer,
				// and there's a trail that gets rendered here. Let's put the trail on the transp layer.
				// bIsElectricity contains group 22000 which is also an explosion that is used like "flak"
				if (bIsHitEffect || bIsEngineGlow || bIsExplosion || bIsElectricity ||
					bIsTrail || g_bIsTrianglePointer)
				{
					// Override the RTV for hit effects, engine glow, explosions and other shadeless/transparent
					// objects --> let's render them directly on the transparent layer!
					resources->_overrideRTV = TRANSP_LYR_1;
					if (bIsTrail)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fMissileStrength;
						g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_MISSILE;
					}
				}

				// If we're rendering 3D contents in the Tech Room and some form of SSAO is enabled, 
				// then disable the pre-computed diffuse component:
				//if (g_bAOEnabled && !g_bRendering3D) {
				// Only disable the diffuse component during regular flight. 
				// The tech room is unchanged (the tech room makes g_bRendering = false)
				// We should also avoid touching the GUI elements
				// When the Death Star is destroyed s_XwaGlobalLightsCount becomes 0, we can use the original illumination in that case.
				if (/* (*s_XwaGlobalLightsCount == 0) || */
					(g_bRendering3D && !g_bStartedGUI && !g_bIsTrianglePointer && !bIsMapIcon)) {
					bModifiedShaders = true;
					g_PSCBuffer.fDisableDiffuse = 1.0f;
				}

				if (bIsXWAHangarShadow) {
					bModifiedShaders = true;
					g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_XWA_SHADOW;
				}

				// Set bits in the constant buffers for Smoke and Blast Marks
				if (bLastTextureSelectedNotNULL) {
					if (lastTextureSelected->is_Smoke) {
						//log_debug("[DBG] Smoke: %s", lastTextureSelected->_surface->_name);
						bModifiedShaders = true;
						//EnableTransparency();
						g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_SMOKE;
					}
					else if (bIsBlastMark) {
						// Blast Marks are rendered on top of the original texture, after greebles have been added. Greebles are
						// rendered with PixelShaderGreeble, and then the blast mark is rendered with PixelShaderTexture on its
						// own draw call, using textures like the following:
						// dat, 3050, 0, 0, 0
						// dat, 3050, 1, 0, 0
						// dat, 3050, 3, 0, 0
						// Now, blast marks have transparency. PixelShaderTexture renders transparency as a glass material, and
						// we can't have that, in part because that erases the normal-mapped greebles that were rendered in a previous
						// draw call. So, here we enable this flag so that we *don't* render blast marks as glass.
						bModifiedShaders = true;
						g_PSCBuffer.special_control.bBlastMark = 1;
						//g_PSCBuffer.GreebleDist1 = g_fBlastMarkOfsX;
						//g_PSCBuffer.GreebleDist2 = g_fBlastMarkOfsY;
						resources->_overrideRTV = TRANSP_LYR_1;
					}
				}

				if (bIsDS2CoreExplosion) {
					resources->_overrideRTV = TRANSP_LYR_1;

					if (!bStateD3dAnnotationOpen) {
						_deviceResources->BeginAnnotatedEvent(L"DrawDS2CoreExplosion");
						bStateD3dAnnotationOpen = true;
					}
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
					resources->InitPixelShader(resources->_explosionPS);
					// Set the noise texture and sampler state with wrap/repeat enabled.
					context->PSSetShaderResources(9, 1, resources->_grayNoiseSRV.GetAddressOf());
					context->PSSetSamplers(9, 1, resources->_repeatSamplerState.GetAddressOf());
					
					g_ShadertoyBuffer.iTime = iTime;
					//g_ShadertoyBuffer.iResolution[0] = lastTextureSelected->material.LavaSize;
					g_ShadertoyBuffer.iResolution[1] = lastTextureSelected->material.EffectBloom;
					g_ShadertoyBuffer.Style = 0; // AlphaBlendEnabled: Do not blend the explosion with the original texture
					g_ShadertoyBuffer.tunnel_speed = 1.0f; // ExplosionTime: Always set to 1 -- the animation is performed by iTime in VolumetricExplosion()
					//g_ShadertoyBuffer.twirl = ExplosionScale; // 2.0 is the normal size, 4.0 is small, 1.0 is big.
					g_ShadertoyBuffer.twirl = 2.0f; // ExplosionScale: 2.0 is the normal size, 4.0 is small, 1.0 is big.
					// Set the constant buffer
					resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
				}

				// Add explosion lights
				if (g_bEnableExplosionLights && bIsExplosion)
				{
					AddExplosionLights(instruction, currentIndexLocation, lastTextureSelected);
				}

				// Render the procedural explosions
				if (bIsExplosion && bHasMaterial)
				{
					g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_EXPLOSION;
					if (lastTextureSelected->material.ExplosionBlendMode > 0 ||
						// The newest explosions by MechDonald are bigger and longer. Unfortunately, during the DS2 mission, this can
						// block a lot of visibility in the tunnel. So if we're in the DS2 mission, let's use procedural explosions
						// instead.
						(g_bDS2ForceProceduralExplosions && *missionIndexLoaded == DEATH_STAR_MISSION_INDEX))
					{
						static float iTime = 0.0f;
						iTime += lastTextureSelected->material.ExplosionSpeed;

						bModifiedShaders = true;
						bModifiedPixelShader = true;
						resources->InitPixelShader(resources->_explosionPS);
						// Set the noise texture and sampler state with wrap/repeat enabled.
						context->PSSetShaderResources(9, 1, resources->_grayNoiseSRV.GetAddressOf());
						context->PSSetSamplers(9, 1, resources->_repeatSamplerState.GetAddressOf());

						int GroupId = 0, ImageId = 0;
						if (!lastTextureSelected->material.DATGroupImageIdParsed) {
							// TODO: Maybe I can extract the group Id and image Id from the DAT's name during tagging and
							// get rid of the DATGroupImageIdParsed field in the material property.
							GetGroupIdImageIdFromDATName(lastTextureSelected->_surface->_cname, &GroupId, &ImageId);
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
						// The same GroupId can appear mutiple times on the screen belonging to different crafts and they
						// may even be at different stages of the animation.
						float ExplosionTime = min(1.0f, (float)ImageId / (float)lastTextureSelected->material.TotalFrames);
						//log_debug("[DBG] Explosion Id: %d, Frame: %d, TotalFrames: %d, Time: %0.3f",
						//	GroupId, ImageId, lastTextureSelected->material.TotalFrames, ExplosionTime);
						//log_debug("[DBG] Explosion Id: %d", GroupId);

						g_ShadertoyBuffer.iTime = iTime;
						// ExplosionBlendMode:
						// 0: Original texture, 
						// 1: Blend with procedural explosion, 
						// 2: Use procedural explosions only
						// AlphaBlendEnabled: true blend with original texture, false: replace original texture
						g_ShadertoyBuffer.Style = lastTextureSelected->material.ExplosionBlendMode;
						g_ShadertoyBuffer.tunnel_speed = lerp(3.0f, -1.0f, ExplosionTime); // ExplosionTime: 3..-1 The animation is performed by iTime in VolumetricExplosion()
						// ExplosionScale: 4.0 == small, 2.0 == normal, 1.0 == big.
						// The value from ExplosionScale is translated from user-facing units to shader units in the ReadMaterialLine() function
						g_ShadertoyBuffer.twirl = lastTextureSelected->material.ExplosionScale;
						// Set the constant buffer
						resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
					}
					else { // ExplosionBlendMode == 0
						int AltExplosionSelectorIdx = lastTextureSelected->DATGroupId - 2000; // 2000 is the first explosion GroupId
						if (AltExplosionSelectorIdx >= 0 && AltExplosionSelectorIdx < MAX_XWA_EXPLOSIONS) {
							int RandomAltIdx = g_AltExplosionSelector[AltExplosionSelectorIdx];
							// Debug override: force a specific index so that we can see each alternate explosion set if we want to.
							if (g_iForceAltExplosion > -1 && g_iForceAltExplosion < MAX_ALT_EXPLOSIONS)
								RandomAltIdx = g_iForceAltExplosion;
							if (RandomAltIdx > -1) {
								Material *material = &lastTextureSelected->material;
								// If we're in the DS2 mission, we need to load the alternate explosion designed for it.
								// Otherwise we select one of the alternatives at random.
								int AltExpIndex = (*missionIndexLoaded == DEATH_STAR_MISSION_INDEX) ? DS2_ALT_EXPLOSION_IDX : RandomAltIdx;
								int ATCIndex = material->AltExplosionIdx[AltExpIndex];
								if (ATCIndex > -1) {
									AnimatedTexControl *atc = &(g_AnimatedMaterials[ATCIndex]);
									int frame = lastTextureSelected->DATImageId;
									if (-1 < frame && frame < (int)atc->Sequence.size()) {
										int extraTexIdx = atc->Sequence[frame].ExtraTextureIndex;
										if (extraTexIdx > -1)
											resources->InitPSShaderResourceView(resources->_extraTextures[extraTexIdx]);
										else // Skip this frame
											goto out;
									}
									else
										// The alternate explosion has less frame than the original. Skip this frame.
										// If we *don't* skip this frame, then the original animation will play after the alternate
										// animation. That usually looks weird.
										goto out;
								}
								// else: ATCIndex is -1: there's no alternate explosion, use the original version
							}
							// else: RandomAltIdx is -1, use the original version
						}
						// else: Unknown Explosion group!
					}
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
					g_SunColors[SunFlareIdx].w = 0.0f;
					// Use the material properties of this Sun -- if it has any associated with it
					if (bHasMaterial) {
						g_SunColors[SunFlareIdx].x = lastTextureSelected->material.Light.x;
						g_SunColors[SunFlareIdx].y = lastTextureSelected->material.Light.y;
						g_SunColors[SunFlareIdx].z = lastTextureSelected->material.Light.z;
						// We have a color for this sun, let's set w to 1 to signal that
						g_SunColors[SunFlareIdx].w = 1.0f;
						// With the D3DRendererHook, the order of the draw calls is slightly different.
						// In this pass, we render the Sun and store its color for later use. Originally,
						// we stored the color directly in g_ShadertoyBuffer.SunColor. However, this same
						// color is used for other things, so now it's overwritten with other stuff by
						// the time we add the flare, in PrimarySurface::Flip(). We can't render the flare
						// here because we need a fully-populated depth buffer, so that has to be added in
						// post. So, we store the color in g_SunColors and then we copy those colors back
						// into g_ShadertoyBuffer when rendering the flare. However, we still need the color
						// here, to render the procedural sun.
						g_ShadertoyBuffer.SunColor[SunFlareIdx] = g_SunColors[SunFlareIdx];
					}

					//if (ComputeCentroid(instruction, currentIndexLocation, &SunCentroid))
					if (bSunCentroidComputed)
					{
						// If the centroid is visible, then let's display the sun flare:
						g_ShadertoyBuffer.SunFlareCount++;
						g_iSunFlareCount = g_ShadertoyBuffer.SunFlareCount;
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

					// Send the skybox to infinity:
					g_VSCBuffer.sz_override = 1.52590219E-05f;
					g_VSCBuffer.mult_z_override = 5000.0f; // Infinity is probably at 65535, we can probably multiply by something bigger here.
					g_PSCBuffer.bIsShadeless = 1;
					g_PSCBuffer.fPosNormalAlpha = 0.0f;
					g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_BACKGROUND;
					// Redirect all background objects to the proper layer:
					resources->_overrideRTV = BACKGROUND_LYR;
				}

				// Apply specific material properties for the current texture
				// This is now implemented in ApplyMaterialProperties()

				// Apply the SSAO mask/Special materials, like lasers and HUD
				if (bLastTextureSelectedNotNULL)
				{
					if (g_bIsScaleableGUIElem || bIsReticle || bIsText || g_bIsTrianglePointer || 
						lastTextureSelected->is_Debris || lastTextureSelected->is_GenericSSAOMasked ||
						lastTextureSelected->is_Electricity || bIsExplosion || bIsMapIcon ||
						lastTextureSelected->is_Smoke)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fSSAOMaskVal = 0;
						g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
						g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
						g_PSCBuffer.fNMIntensity = 0.0f;
						g_PSCBuffer.fSpecVal     = 0.0f;
						g_PSCBuffer.bIsShadeless = 1;
						g_PSCBuffer.fPosNormalAlpha = 0.0f;
					} 
					else if (lastTextureSelected->is_Debris || lastTextureSelected->is_Trail ||
						lastTextureSelected->is_CockpitSpark || lastTextureSelected->is_Spark || 
						lastTextureSelected->is_Chaff)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fSSAOMaskVal = 0;
						g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
						g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
						g_PSCBuffer.fNMIntensity = 0.0f;
						g_PSCBuffer.fSpecVal     = 0.0f;
						g_PSCBuffer.fPosNormalAlpha = 0.0f;
					}
					else if (lastTextureSelected->is_Laser) {
						bModifiedShaders = true;
						g_PSCBuffer.fSSAOMaskVal = 0;
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

				// Animated Light Maps/Textures (DAT-related animations)
				// We need to apply DAT animations here, before we capture the reticle. That way, animated reticles
				// are possible in VR mode.
				if (bHasMaterial && lastTextureSelected->material.GreebleDataIdx == -1) {
					if ((bIsLightTexture && lastTextureSelected->material.GetCurrentATCIndex(NULL, LIGHTMAP_ATC_IDX) > -1) ||
						(!bIsLightTexture && lastTextureSelected->material.GetCurrentATCIndex(NULL, TEXTURE_ATC_IDX) > -1))
					{
						bool bIsDamageTex = false;
						bModifiedShaders = true;
						bModifiedPixelShader = true;
						int TexATCIndex = lastTextureSelected->material.GetCurrentATCIndex(&bIsDamageTex, TEXTURE_ATC_IDX);
						int LightATCIndex = lastTextureSelected->material.GetCurrentATCIndex(NULL, LIGHTMAP_ATC_IDX);

						// This path doesn't render any DC elements. So, compared with EffectsRenderer::ApplyAnimatedTextures(),
						// this code lacks all the DC-related checks (we assume bRenderingDC is false)

						// If we reach this point then one of LightMapATCIndex or TextureATCIndex must be > -1 or both!
						// If we're rendering a DC element, we don't want to replace the shader
						resources->InitPixelShader(resources->_pixelShaderAnimDAT);

						// Let's do a quick update of the hyperspace flags here:
						g_PSCBuffer.bInHyperspace = PlayerDataTable[*g_playerIndex].hyperspacePhase != 0 || g_HyperspacePhaseFSM != HS_INIT_ST;
						g_PSCBuffer.AuxColor.x = 1.0f;
						g_PSCBuffer.AuxColor.y = 1.0f;
						g_PSCBuffer.AuxColor.z = 1.0f;
						g_PSCBuffer.AuxColor.w = 1.0f;

						g_PSCBuffer.AuxColorLight.x = 1.0f;
						g_PSCBuffer.AuxColorLight.y = 1.0f;
						g_PSCBuffer.AuxColorLight.z = 1.0f;
						g_PSCBuffer.AuxColorLight.w = 1.0f;

						int extraTexIdx = -1, extraLightIdx = -1;
						if (TexATCIndex > -1) {
							AnimatedTexControl *atc = &(g_AnimatedMaterials[TexATCIndex]);
							int idx = atc->AnimIdx;
							extraTexIdx = atc->Sequence[idx].ExtraTextureIndex;
							if (atc->BlackToAlpha)
								g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_BLACK_TO_ALPHA;
							else if (atc->AlphaIsBloomMask)
								g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_ALPHA_IS_BLOOM_MASK;
							else
								g_PSCBuffer.special_control.ExclusiveMask = 0;
							g_PSCBuffer.AuxColor = atc->Tint;
							g_PSCBuffer.Offset = atc->Offset;
							g_PSCBuffer.AspectRatio = atc->AspectRatio;
							g_PSCBuffer.Clamp = atc->Clamp;
							g_PSCBuffer.fBloomStrength = atc->Sequence[idx].intensity;
							g_current_renderer->SetRenderTypeIllum(0);
							// We cannot use InitPSShaderResourceView here because that will set slots 0 and 1, thus changing
							// the DC foreground SRV
							context->PSSetShaderResources(0, 1, &(resources->_extraTextures[extraTexIdx]));
						}

						if (LightATCIndex > -1) {
							AnimatedTexControl *atc = &(g_AnimatedMaterials[LightATCIndex]);
							int idx = atc->AnimIdx;
							extraLightIdx = atc->Sequence[idx].ExtraTextureIndex;
							if (atc->BlackToAlpha)
								g_PSCBuffer.special_control_light.ExclusiveMask = SPECIAL_CONTROL_BLACK_TO_ALPHA;
							else if (atc->AlphaIsBloomMask)
								g_PSCBuffer.special_control_light.ExclusiveMask = SPECIAL_CONTROL_ALPHA_IS_BLOOM_MASK;
							else
								g_PSCBuffer.special_control_light.ExclusiveMask = 0;
							g_PSCBuffer.AuxColorLight = atc->Tint;
							// TODO: We might need two of these settings below, one for the regular tex and one for the lightmap
							g_PSCBuffer.Offset = atc->Offset;
							g_PSCBuffer.AspectRatio = atc->AspectRatio;
							g_PSCBuffer.Clamp = atc->Clamp;
							g_PSCBuffer.fBloomStrength = atc->Sequence[idx].intensity;
							g_current_renderer->SetRenderTypeIllum(1);
							// We cannot use InitPSShaderResourceView here because that will set slots 0 and 1, thus changing
							// the DC foreground SRV
							context->PSSetShaderResources(1, 1, &(resources->_extraTextures[extraLightIdx]));
						}
					}
				}

				// EARLY EXIT 1: Render the HUD/GUI to the Dynamic Cockpit RTVs and continue
				bool bRenderReticleToBuffer = bIsReticle; // && !bExternalCamera;
				if (g_bDynCockpitEnabled &&
					(bRenderToDynCockpitBuffer || bRenderToDynCockpitBGBuffer) || bRenderReticleToBuffer
				   )
				{	
					ID3D11DepthStencilView *ds = g_bUseSteamVR ? NULL : resources->_depthStencilViewL.Get();

					if (!bStateD3dAnnotationOpen) {
						_deviceResources->BeginAnnotatedEvent(L"RenderHUD");
						bStateD3dAnnotationOpen = true;
					}
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
						context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpitBG.GetAddressOf(), ds);
					}
					else {
						//context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpit.GetAddressOf(),
						//	resources->_depthStencilViewL.Get());

						if (g_config.Text2DRendererEnabled)
							context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpit.GetAddressOf(), ds);
						else 
						{
							// The new Text Renderer is not enabled; but we can still render the text to its own
							// layer so that we can do a more specialized processing, like splitting the text
							// from the targeted object
							if (bIsText)
								context->OMSetRenderTargets(1, resources->_DCTextRTV.GetAddressOf(),
									resources->_depthStencilViewL.Get());
							else
								context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpit.GetAddressOf(), ds);
						}
					}
					// Enable Z-Buffer if we're drawing the targeted craft
					// The targeted craft is no longer rendered in this path when the D3DRendererHook is
					// enabled. See XwaD3dRendererHook::DCCaptureMiniature instead.
					if (g_bIsFloating3DObject) {
						//log_debug("[DBG] Render 3D Floating OBJ to DC RTV");
						QuickSetZWriteEnabled(TRUE);
					}

					// Render
					if (g_bUseSteamVR)
						context->DrawIndexedInstanced(3 * instruction->wCount, 1, currentIndexLocation, 0, 0); // if (g_bUseSteamVR)
					else
						context->DrawIndexed(3 * instruction->wCount, currentIndexLocation, 0);
					g_iHUDOffscreenCommandsRendered++;

					// Restore the regular texture, RTV, shaders, etc:
					context->PSSetShaderResources(0, 1, lastTextureSelected->_textureView.GetAddressOf());
					context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(),
						resources->_depthStencilViewL.Get());
					if (g_bEnableVR) {
						resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
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
				// No longer done here, look in XwaD3dRendererHook

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
						int anim_idx = lastTextureSelected->material.GetCurrentATCIndex(NULL, LIGHTMAP_ATC_IDX);
						g_PSCBuffer.fBloomStrength = lastTextureSelected->is_CockpitTex ?
							g_BloomConfig.fCockpitStrength : g_BloomConfig.fLightMapsStrength;
						// If this is an animated light map, then use the right intensity setting
						// TODO: Make the following code more efficient
						if (anim_idx > -1) {
							AnimatedTexControl *atc = &(g_AnimatedMaterials[anim_idx]);
							g_PSCBuffer.fBloomStrength = atc->Sequence[atc->AnimIdx].intensity;
						}
						//g_PSCBuffer.bIsLightTexture = g_config.EnhanceIllumination ? 2 : 1;
					}
					// Set the flag for EngineGlow and Explosions (enhance them in 32-bit mode, apply bloom)
					else if (lastTextureSelected->is_EngineGlow) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fEngineGlowStrength;
						g_PSCBuffer.bIsEngineGlow = g_config.EnhanceEngineGlow ? 2 : 1;
						if (!bStateD3dAnnotationOpen) {
							_deviceResources->BeginAnnotatedEvent(L"EngineGlow");
							bStateD3dAnnotationOpen = true;
						}
					}
					else if (lastTextureSelected->is_Electricity || bIsExplosion)
					{
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fExplosionsStrength;
						g_PSCBuffer.bIsEngineGlow = g_config.EnhanceExplosions ? 2 : 1;
						if (!bStateD3dAnnotationOpen) {
							_deviceResources->BeginAnnotatedEvent(L"ElectricityOrExplosion");
							bStateD3dAnnotationOpen = true;
						}
					}
					else if (lastTextureSelected->is_LensFlare) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fLensFlareStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
						if (!bStateD3dAnnotationOpen) {
							_deviceResources->BeginAnnotatedEvent(L"LensFlare");
							bStateD3dAnnotationOpen = true;
						}
					}
					else if (bIsSun) {
						bModifiedShaders = true;
						// If there's a 3D sun in the scene, then we shouldn't apply bloom to Sun textures  they should be invisible 
						g_PSCBuffer.fBloomStrength = g_b3DSunPresent ? 0.0f : g_BloomConfig.fSunsStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (lastTextureSelected->is_Spark || lastTextureSelected->is_HitEffect) {
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
						g_PSCBuffer.fBloomStrength = 4.0f * g_BloomConfig.fSparksStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (lastTextureSelected->is_SkydomeLight) {
						bModifiedShaders = true;
						g_PSCBuffer.fBloomStrength = g_BloomConfig.fSkydomeLightStrength;
						g_PSCBuffer.bIsEngineGlow = 1;
					}
					else if (!bIsLightTexture && lastTextureSelected->material.GetCurrentATCIndex(NULL) > -1) {
						bModifiedShaders = true;
						int anim_idx = lastTextureSelected->material.GetCurrentATCIndex(NULL);
						// If this is an animated light map, then use the right intensity setting
						// TODO: Make the following code more efficient
						if (anim_idx > -1) {
							AnimatedTexControl *atc = &(g_AnimatedMaterials[anim_idx]);
							g_PSCBuffer.fBloomStrength = atc->Sequence[atc->AnimIdx].intensity;
						}
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

				if (bIsMapIcon) {
					bModifiedShaders = true;
					g_VSCBuffer.bIsMapIcon = true;
				}

				// Transparent textures are currently used with DC to render floating text. However, if the erase region
				// commands are being ignored, then this will cause the text messages to be rendered twice. To avoid
				// having duplicated messages, we're removing these textures here when the erase_region commmands are
				// not being applied.
				// The above behavior is overridden if the DC element is set as "always_visible". In that case, the
				// transparent layer will remain visible even when the HUD is displayed.
				if (g_bDCManualActivate && g_bDynCockpitEnabled && bIsTransparent && !g_bDCApplyEraseRegionCommands && !bDCElemAlwaysVisible)
					goto out;

				// Dynamic Cockpit: Replace textures at run-time:
				// No longer done here, look in XwaD3dRendererHook

				// Animated Light Maps/Textures (DAT-related animations)
				// This block was moved to the section where the reticle is captured. Thatw way, animated
				// reticles can also work in VR mode

				// Don't render the first hyperspace frame: use all the buffers from the previous frame instead. Otherwise
				// the craft will jerk or blink because XWA resets the cockpit camera and the craft's orientation on this
				// frame.
				if (g_bHyperspaceFirstFrame)
					goto out;

				// DEBUG: Dump an OBJ for the current cockpit
				/*
				if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled && bIsCockpit) {
					log_debug("[DBG] Dumping OBJ (Cockpit): %s", lastTextureSelected->_surface->_cname);
					DumpVerticesToOBJ(g_DumpOBJFile, instruction, currentIndexLocation, g_iDumpOBJIdx);
				}
				if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled && bIsLaser) {
					log_debug("[DBG] Dumping OBJ (Laser): %s", lastTextureSelected->_surface->_cname);
					DumpVerticesToOBJ(g_DumpLaserFile, instruction, currentIndexLocation, g_iDumpLaserOBJIdx);
				}
				*/
				if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled && (bIsEngineGlow || bIsMapIcon)) {
					log_debug("[DBG] Dumping OBJ (bIsEngineGlow|bIsMapIcon): obj_%d, %s", g_iDumpOBJIdx, lastTextureSelected->_surface->_cname);
					DumpVerticesToOBJ(g_DumpOBJFile, instruction, currentIndexLocation,
						g_iDumpOBJIdx, lastTextureSelected->_surface->_cname, bIsMapIcon);
				}
				if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled && bIsExplosion) {
					log_debug("[DBG] Dumping OBJ (bIsExplosion): obj_%d, %s", g_iDumpLaserOBJIdx, lastTextureSelected->_surface->_cname);
					DumpVerticesToOBJ(g_DumpLaserFile, instruction, currentIndexLocation,
						g_iDumpLaserOBJIdx, lastTextureSelected->_surface->_cname);
				}

				// Tech Room Hologram control (this is only used for the engine glows)
				if (g_bInTechRoom && g_config.TechRoomHolograms)
				{
					bModifiedShaders = 1;
					g_PSCBuffer.rand0 = 8.0f * g_fDCHologramTime;
					g_PSCBuffer.rand1 = 1.0f;
				}

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

					// DEBUG: Disable blast marks!
					//if (!g_bShowBlastMarks && bIsBlastMark)
					//	goto out;

					if (!g_bReshadeEnabled) {
						// The original 2D vertices are already in the GPU, so just render as usual
						ID3D11RenderTargetView *rtvs[1] = {
							SelectOffscreenBuffer(),
						};
						context->OMSetRenderTargets(1,
							//resources->_renderTargetView.GetAddressOf(),
							rtvs, resources->_depthStencilViewL.Get());
					} else {
						// Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
						// NON-VR with effects:
						ID3D11RenderTargetView *rtvs[6] = {
							SelectOffscreenBuffer(), //resources->_renderTargetView.Get(),

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
				
				if (bIsEngineGlow || bIsExplosion /* || bIsDS2CoreExplosion? */ ) {
					bModifiedVertexShader = true;
					UpdateReconstructionConstants();
					resources->InitVertexShader(resources->_datVertexShaderVR);
				}

				// Apply the changes to the vertex and pixel shaders
				//if (bModifiedShaders)
				{
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
					if (g_bUseSteamVR) {
						if (!g_bReshadeEnabled) {
							ID3D11RenderTargetView *rtvs[1] = {
								SelectOffscreenBuffer(),
							};
							context->OMSetRenderTargets(1, rtvs, resources->_depthStencilViewL.Get());
						} else {
							// SteamVR, left image. Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
							ID3D11RenderTargetView *rtvs[6] = {
								SelectOffscreenBuffer(), //resources->_renderTargetView.Get(),
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
								SelectOffscreenBuffer(),
							};
							context->OMSetRenderTargets(1, rtvs, resources->_depthStencilViewL.Get());
						} else {
							// DirectSBS, Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
							ID3D11RenderTargetView *rtvs[6] = {
								SelectOffscreenBuffer(), //resources->_renderTargetView.Get(),
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
					g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
					g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
					// The viewMatrix is set at the beginning of the frame
					resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);
					// Draw the Left Image (and the right one if were using instanced stereo)
					if (g_bUseSteamVR)
						context->DrawIndexedInstanced(3 * instruction->wCount, 2, currentIndexLocation, 0, 0); // if (g_bUseSteamVR)
					else
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
						goto out;
					} else {
						// DirectSBS Mode
						if (!g_bReshadeEnabled) {
							ID3D11RenderTargetView *rtvs[1] = {
								SelectOffscreenBuffer(),
							};
							context->OMSetRenderTargets(1, rtvs, resources->_depthStencilViewL.Get());
						} else {
							// Reshade is enabled, render to multiple output targets (bloom mask, depth buffer)
							ID3D11RenderTargetView *rtvs[6] = {
								SelectOffscreenBuffer(),
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
					viewport.Width = (float)resources->_backbufferWidth / 2.0f;
					viewport.TopLeftX = viewport.Width;
					viewport.Height = (float)resources->_backbufferHeight;
					viewport.TopLeftY = 0.0f;
					viewport.MinDepth = D3D11_MIN_DEPTH;
					viewport.MaxDepth = D3D11_MAX_DEPTH;
					resources->InitViewport(&viewport);
					// Set the right projection matrix
					g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
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

				// Reset the _overrideRTV target
				resources->_overrideRTV = TRANSP_LYR_NONE;

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
					g_PSCBuffer.AuxColor.x		= 1.0f;
					g_PSCBuffer.AuxColor.y		= 1.0f;
					g_PSCBuffer.AuxColor.z		= 1.0f;
					g_PSCBuffer.AuxColor.w		= 1.0f;
					g_PSCBuffer.AspectRatio		= 1.0f;

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

				if (bModifiedVertexShader) {
					if (g_bEnableVR)
						resources->InitVertexShader(resources->_sbsVertexShader); // if (g_bEnableVR)
					else
						resources->InitVertexShader(resources->_vertexShader);
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
		if (g_DumpLaserFile != NULL) {
			fflush(g_DumpLaserFile);
			fclose(g_DumpLaserFile);
		}
		g_DumpOBJFile = NULL; g_DumpLaserFile = NULL;
		log_debug("[DBG] Vertices dumped to OBJ file");
	}
	//log_debug("[DBG] Execute (2)");
	// DEBUG
	g_iDumpOBJIdx = 1; g_iDumpLaserOBJIdx = 1;
	// DEBUG

	if (bStateD3dAnnotationOpen) {
		_deviceResources->EndAnnotatedEvent(); //We need to close the special state annotation event
		bStateD3dAnnotationOpen = false;
	}
	_deviceResources->EndAnnotatedEvent();

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			char text[512];
			strcpy_s(text, step);
			strcat_s(text, "\n");
			strcat_s(text, _com_error(hr).ErrorMessage());
			strcat_s(text, "\n");
			strcat_s(text, _com_error(this->_deviceResources->_d3dDevice->GetDeviceRemovedReason()).ErrorMessage());

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
	LPDIRECT3DVIEWPORT* lplpDirect3DViewport,
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

HRESULT Direct3DDevice::BeginScene()
{

	_deviceResources->BeginAnnotatedEvent(L"Direct3DDeviceScene");

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
	if (g_iPresentCounter >= PLAYERDATATABLE_MIN_SAFE_FRAME && g_playerIndex != nullptr)
		g_bMapMode = (PlayerDataTable[*g_playerIndex].mapState != 0);
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

	// Update the hi-res timer
	g_HiResTimer.GetElapsedTime();

	g_CurrentHeadingViewMatrix = GetCurrentHeadingViewMatrix();
	UpdateDCHologramState();

	// Update the global game event
	int16_t currentTargetIndex = (int16_t)PlayerDataTable[*g_playerIndex].currentTargetIndex;
	g_GameEvent.TargetEvent = currentTargetIndex < 0 ? EVT_NONE : TGT_EVT_SELECTED;

	CraftInstance *craftInstance = GetPlayerCraftInstanceSafe();
	if (craftInstance != NULL) {
		CockpitInstrumentState prevDamage = g_GameEvent.CockpitInstruments;

		if (!g_GameEvent.bCockpitInitialized)
		{
			g_GameEvent.InitialCockpitInstruments.FromXWADamage(craftInstance->InitialCockpitInstruments);
			g_GameEvent.CockpitInstruments = g_GameEvent.InitialCockpitInstruments;
			g_GameEvent.bCockpitInitialized = true;
			prevDamage = g_GameEvent.CockpitInstruments;
		}

		// Restore the cockpit and hull damage
		if (g_bResetCockpitDamage) {
			log_debug("[DBG] [CPT] Restoring Cockpit and Hull Damage");
			craftInstance->CockpitInstrumentStatus = craftInstance->InitialCockpitInstruments;
			craftInstance->HullDamageReceived = 0;
			g_bResetCockpitDamage = false;
			prevDamage.FromXWADamage(craftInstance->InitialCockpitInstruments);
		}

		// Apply the cockpit damage if requested
		if (g_bApplyCockpitDamage) {
			FILE *MaskFile = NULL;
			fopen_s(&MaskFile, "CockpitDamage.txt", "rt");
			if (MaskFile != NULL) {
				int num_read = 0;
				uint32_t Mask = 0x0;
				float hull;
				// First line: the cockpit damage hex mask
				num_read = fscanf_s(MaskFile, "0x%x\n", &Mask);
				if (num_read == 0) Mask = 0x0;
				// Second line: hull health percentage. A number in [1..100].
				num_read = fscanf_s(MaskFile, "%f\n", &hull);
				if (num_read == 0) hull = 100.0f;
				fclose(MaskFile);

				log_debug("[DBG] InitialCockpitInstruments: 0x%x, CockpitInstrumentStatus: 0x%x",
					craftInstance->InitialCockpitInstruments, craftInstance->CockpitInstrumentStatus);
				craftInstance->CockpitInstrumentStatus = craftInstance->InitialCockpitInstruments & Mask;
				craftInstance->HullDamageReceived = (int)((float)craftInstance->HullStrength - (float)craftInstance->HullStrength * (hull / 100.0f));
				log_debug("[DBG] New Cockpit Instruments: 0x%x", craftInstance->CockpitInstrumentStatus);
				log_debug("[DBG] New Hull value: %0.3f", hull);
			}
			g_bApplyCockpitDamage = false;
		}

		// Update the cockpit instrument status
		g_GameEvent.CockpitInstruments.FromXWADamage(craftInstance->CockpitInstrumentStatus);
		if (prevDamage.CMD && !g_GameEvent.CockpitInstruments.CMD)
			log_debug("[DBG] [CPT] CMD Damaged!");
		if (prevDamage.LaserIon && !g_GameEvent.CockpitInstruments.LaserIon)
			log_debug("[DBG] [CPT] LaserIon Damaged!");
		if (prevDamage.BeamWeapon && !g_GameEvent.CockpitInstruments.BeamWeapon)
			log_debug("[DBG] [CPT] BeamWeapon Damaged!");
		if (prevDamage.Shields && !g_GameEvent.CockpitInstruments.Shields)
			log_debug("[DBG] [CPT] Shields Damaged!");
		if (prevDamage.Sensors && !g_GameEvent.CockpitInstruments.Sensors)
			log_debug("[DBG] [CPT] Sensors Damaged!");
		if (prevDamage.LaserRecharge && !g_GameEvent.CockpitInstruments.LaserRecharge)
			log_debug("[DBG] [CPT] LaserRecharge Damaged!");
		if (prevDamage.ShieldRecharge && !g_GameEvent.CockpitInstruments.ShieldRecharge)
			log_debug("[DBG] [CPT] ShieldRecharge Damaged!");
		if (prevDamage.EnginePower && !g_GameEvent.CockpitInstruments.EnginePower)
			log_debug("[DBG] [CPT] EnginePower Damaged!");
		if (prevDamage.BeamRecharge && !g_GameEvent.CockpitInstruments.BeamRecharge)
			log_debug("[DBG] [CPT] BeamRecharge Damaged!");
		if (prevDamage.Throttle && !g_GameEvent.CockpitInstruments.Throttle)
			log_debug("[DBG] [CPT] Throttle Damaged!");

		float hull = 100.0f * (1.0f - (float)craftInstance->HullDamageReceived / (float)craftInstance->HullStrength);
		hull = max(0.0f, hull);
		if (g_bDumpSSAOBuffers) log_debug("[DBG] Hull health: %0.3f", hull);
		// Update the Hull event
		g_GameEvent.HullEvent = EVT_NONE;
		if (hull > 75.0f)
			g_GameEvent.HullEvent = EVT_NONE;
		else if (50.0f < hull && hull <= 75.0f)
			g_GameEvent.HullEvent = HULL_EVT_DAMAGE_75;
		else if (25.0f < hull && hull <= 50.0f)
			g_GameEvent.HullEvent = HULL_EVT_DAMAGE_50;
		else if (hull <= 25.0f)
			g_GameEvent.HullEvent = HULL_EVT_DAMAGE_25;

		/*
		LaserNextHardpoint tells us which laser cannon is ready to fire.
		LaserLinkStatus: 0x1 -- Single fire.
		LaserLinkStatus: 0x2 -- Dual fire. LaserHardpoint will now be either 0x0 or 0x1 and it will skip one laser
		LaserLinkStatus: 0x3 -- Fire all lasers in the current group. LaserHardpoint stays at 0x0 -- all cannons ready to fire
		LaserLinkStatus: 0x4 -- Fire all lasers in all groups. All LaserLinkStatus indices become 0x4 as soon as one of them is 0x4

		X/W, single fire:
		[808][DBG][Cockpitlook] WarheadArmed: 0, NumberOfLaserSets : 1, NumberOfLasers : 4, NumWarheadLauncherGroups : 1, Countermeasures : 0
		[808][DBG][Cockpitlook] LaserNextHardpoint[0] : 0x0,
		[808][DBG][Cockpitlook] LaserNextHardpoint[0] : 0x1,
		[808][DBG][Cockpitlook] LaserNextHardpoint[0] : 0x2,
		[808][DBG][Cockpitlook] LaserNextHardpoint[0] : 0x3, ... then it goes back to 0
		The other indices in LaserNextHardpoint remain at 0:
		[808] [DBG] [Cockpitlook] LaserNextHardpoint[1]: 0x0, <-- This is the second set of lasers/ions (when it exists)
		[808] [DBG] [Cockpitlook] LaserNextHardpoint[2]: 0x0, <-- Maybe the third set of lasers, if it exists?

		For the B-Wing the first LaserNextHardpoint group goes 0x0, 0x1, 0x2
		The second group (ions) goes 0x3, 0x4, 0x5.
		LaserLinkStatus is either 0x1 or 0x3. There's no dual fire
		The B-Wing has 6 lasers and 2 groups

		For the T/D the first LaserNextHardpoint group goes 0x0, 0x1, 0x2, 0x3
		The second group goes: 0x4, 0x5
		LaserLinkStatus goes 0x1, 0x2, 0x3 and 0x4. There's single fire, dual fire, current group fire and all groups fire.
		The T/D has 6 lasers and 2 groups.
		*/
		// Update the cannon ready events
		bool bAllCannonsReady = false;
		if (g_bLasersIonsNeedCounting)
			CountLasersAndIons(craftInstance);

		for (int i = 0; i < MAX_CANNONS; i++)
			g_GameEvent.CannonReady[i] = false;
		for (int j = 0; j < craftInstance->NumberOfLaserSets; j++) {
			int FirstLaserIndexInGroup, LastLaserIndexInGroup;
			switch (j) {
			case 0:
				FirstLaserIndexInGroup = 0;
				LastLaserIndexInGroup = g_iNumLaserCannons - 1;
				break;
			case 1:
				FirstLaserIndexInGroup = g_iNumLaserCannons;
				LastLaserIndexInGroup = g_iNumLaserCannons + g_iNumIonCannons - 1;
				break;
			case 2:
				// The following formula is probably wrong, but I don't have a single example
				// of a 3-set laser craft to figure this out. This should suffice for now.
				FirstLaserIndexInGroup = g_iNumLaserCannons + g_iNumIonCannons;
				LastLaserIndexInGroup = craftInstance->NumberOfLasers - 1;
				break;
			}

			switch (craftInstance->LaserLinkStatus[j]) {
			case 0x1: // Single fire
				g_GameEvent.CannonReady[craftInstance->LaserNextHardpoint[j]] = true;
				break;
			case 0x2: // Dual Fire
				for (int i = craftInstance->LaserNextHardpoint[j]; i <= LastLaserIndexInGroup; i += 2)
					g_GameEvent.CannonReady[i] = true;
				break;
			case 0x3: // Fire all lasers in the current group (group j is the current group)
				for (int i = FirstLaserIndexInGroup; i <= LastLaserIndexInGroup; i++)
					g_GameEvent.CannonReady[i] = true;
				break;
			case 0x4: // Fire all lasers in all groups
				bAllCannonsReady = true;
				for (int i = 0; i < craftInstance->NumberOfLasers; i++)
					g_GameEvent.CannonReady[i] = true;
				break;
			}
		}

		// Deactivate lasers and ions if the warhead is armed
		if (PlayerDataTable[*g_playerIndex].warheadArmed) {
			for (int i = 0; i < craftInstance->NumberOfLasers; i++)
				g_GameEvent.CannonReady[i] = false;
		}
		else if (!bAllCannonsReady) {
			// Deactivate the lasers or ions depending on primarySecondaryArmed
			if (PlayerDataTable[*g_playerIndex].primarySecondaryArmed == 0) {
				// Deactivate ions
				for (int i = g_iNumLaserCannons; i < g_iNumLaserCannons + g_iNumIonCannons; i++)
					g_GameEvent.CannonReady[i] = false;
			}
			else {
				// Deactivate lasers
				for (int i = 0; i < g_iNumLaserCannons; i++)
					g_GameEvent.CannonReady[i] = false;
			}
		}
		// Deactivate lasers and ions if they ran out of energy
		for (int i = 0; i < craftInstance->NumberOfLasers; i++)
			// Not sure if I should also check that craftInstance->Hardpoints[i].WeaponType
			// is either 1 or 2 (lasers or ions). It doesn't seem to be necessary.
			if (craftInstance->Hardpoints[i].Energy == 0)
				g_GameEvent.CannonReady[i] = false;


		// Dump the CraftInstance table
		/*
		if (g_bDumpSSAOBuffers) {
			static int counter = 0;
			FILE *fileCI = NULL;
			int error = 0;
			char sFileNameCI[128];
			sprintf_s(sFileNameCI, 128, "./CraftInstance%d", counter);
			counter++;

			log_debug("[DBG] InitialCockpitInstruments: 0x%x, CockpitInstrumentStatus: 0x%x",
				craftInstance->InitialCockpitInstruments, craftInstance->CockpitInstrumentStatus);

			try {
				error = fopen_s(&fileCI, sFileNameCI, "wb");
			}
			catch (...) {
				log_debug("Could not create %s", sFileNameCI);
			}
			if (error == 0) {
				fwrite(craftInstance, sizeof(CraftInstance), 1, fileCI);
				fclose(fileCI);
				log_debug("[DBG] Dumped %s", sFileNameCI);
			}
		}
		*/
	}

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

	// BeginScene():ClearRenderTargetView: Clear all RTVs in this section (there's more blocks below)
	if (!bTransitionToHyperspace) {
		context->ClearRenderTargetView(this->_deviceResources->_renderTargetView, this->_deviceResources->clearColor);
		context->ClearRenderTargetView(resources->_shadertoyRTV, resources->clearColorRGBA);
		context->ClearRenderTargetView(resources->_transp1RTV, resources->clearColor);
		context->ClearRenderTargetView(resources->_transp2RTV, resources->clearColor);
		context->ClearRenderTargetView(resources->_ReticleRTV, resources->clearColorRGBA);

		if (g_bUseSteamVR) {
			context->ClearRenderTargetView(this->_deviceResources->_renderTargetViewR, this->_deviceResources->clearColor);
			context->ClearRenderTargetView(this->_deviceResources->_renderTargetViewHd, this->_deviceResources->clearColor);
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

	//if (g_bReshadeEnabled /*&& !bTransitionToHyperspace*/)
	{
		//float infinity[4] = { 0, 0, 32000.0f, 0 };
		float zero[4] = { 0, 0, 0, 0 };
		float blankMaterial[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float shadelessMaterial[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		context->ClearRenderTargetView(resources->_renderTargetViewNormBuf, zero /* infinity */);
		// Populate the SSAOMask and SSMask buffers with default materials
		context->ClearRenderTargetView(resources->_renderTargetViewSSAOMask, blankMaterial);
		context->ClearRenderTargetView(resources->_renderTargetViewSSMask, shadelessMaterial);
		if (g_bUseSteamVR) {
			context->ClearRenderTargetView(resources->_renderTargetViewNormBufR, zero /* infinity */);
		}
	}

	// Clear the AO RTVs
	if (g_bAOEnabled && !bTransitionToHyperspace) {
		// Filling up the ZBuffer with large values prevents artifacts in SSAO when black bars are drawn
		// on the sides of the screen
		float infinity[4] = { 0, 0, 32000.0f, 0 };
		//float zero[4] = { 0, 0, 0, 0 };

		context->ClearRenderTargetView(resources->_renderTargetViewDepthBuf, infinity);
		if (g_bUseSteamVR)
			context->ClearRenderTargetView(resources->_renderTargetViewDepthBufR, infinity);
	}

	if (!g_bInTechRoom && !g_bMapMode)
	{
		RenderSkyBox();
	}

	if (!bTransitionToHyperspace) {
		context->ClearDepthStencilView(resources->_depthStencilViewL, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
		if (g_bUseSteamVR)
			context->ClearDepthStencilView(resources->_depthStencilViewR, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
	}

	if (g_config.HDConcourseEnabled)
	{
		if (this->_deviceResources->IsInConcourseHd())
		{
			if (g_bUseSteamVR)
				this->_deviceResources->_d3dDeviceContext->CopyResource(this->_deviceResources->_offscreenBufferHd, this->_deviceResources->_offscreenBufferHdBackground);
			else
				this->_deviceResources->_d3dDeviceContext->CopyResource(this->_deviceResources->_offscreenBuffer, this->_deviceResources->_offscreenBufferHdBackground);
		}
	}
	else
	{
		if (FAILED(this->_deviceResources->RenderMain(resources->_backbufferSurface->_buffer, resources->_displayWidth, resources->_displayHeight, resources->_displayBpp)))
			return D3DERR_SCENE_BEGIN_FAILED;
	}

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

	D3dRendererSceneBegin(this->_deviceResources);

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

	//log_debug("[DBG] EndScene");

	/*
	I can't render hyperspace here because it will break the display of the main menu after exiting a mission
	Looks like even the menu uses EndScene and I'm messing it up even when using the "g_bRendering3D" flag!
	*/

	D3dRendererSceneEnd();

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

	// Check which events fired in this frame.
	UpdateEventsFired();
	// Animate all materials
	AnimateMaterials();

	_deviceResources->EndAnnotatedEvent();

	return D3D_OK;
}

HRESULT Direct3DDevice::GetDirect3D(
	LPDIRECT3D* lplpD3D
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
