// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes, 2019

// Shaders by Marty McFly (used with permission from the author)
// https://github.com/martymcmodding/qUINT/tree/master/Shaders

// _deviceResources->_backbufferWidth, _backbufferHeight: 3240, 2160 -- Screen/Backbuffer Resolution
// resources->_displayWidth, resources->_displayHeight -- in-game resolution
// g_WindowWidth, g_WindowHeight --> Actual Windows screen as returned by GetWindowRect

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
#include "ExecuteBufferDumper.h"
#include "PrimarySurface.h"
// TODO: Remove later
#include "TextureSurface.h"

#include <ScreenGrab.h>
#include <WICTextureLoader.h>
#include <wincodec.h>
#include <vector>
//#include <assert.h>

#include "effects.h"
#include "globals.h"
#include "commonVR.h"
#include "FreePIE.h"
#include "SteamVR.h"
#include "DirectSBS.h"
#include "VRConfig.h"

#include "Matrices.h"

#include "XWAObject.h"

#include "..\shaders\shader_common.h"


#define DBG_MAX_PRESENT_LOGS 0

#include "SharedMem.h"
SharedData *g_pSharedData = NULL;

#include "XWAFramework.h"

FILE *g_HackFile = NULL;

float RealVertFOVToRawFocalLength(float real_FOV);

void GetCraftViewMatrix(Matrix4 *result);

inline void backProjectMetric(float sx, float sy, float rhw, Vector3 *P);
inline void backProjectMetric(UINT index, Vector3 *P);
inline Vector3 projectMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR = false);
inline Vector3 projectToInGameOrPostProcCoordsMetric(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix, bool bForceNonVR = false);


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
int g_iHyperStateOverride = HS_HYPER_ENTER_ST;
//int g_iHyperStateOverride = HS_HYPER_TUNNEL_ST;
//int g_iHyperStateOverride = HS_HYPER_EXIT_ST;
//int g_iHyperStateOverride = HS_POST_HYPER_EXIT_ST;
// DEBUG

XWALightInfo g_XWALightInfo[MAX_XWA_LIGHTS];
//void InitHeadingMatrix();
//Matrix4 GetCurrentHeadingMatrix(Vector4 &Rs, Vector4 &Us, Vector4 &Fs, bool invert, bool debug);
Matrix4 GetCurrentHeadingViewMatrix();
Matrix4 GetSimpleDirectionMatrix(Vector4 Fs, bool invert);

D3D11_VIEWPORT g_nonVRViewport{};

VertexShaderMatrixCB g_VSMatrixCB;
VertexShaderCBuffer  g_VSCBuffer;
PixelShaderCBuffer   g_PSCBuffer;
DCPixelShaderCBuffer g_DCPSCBuffer;
ShadertoyCBuffer	 g_ShadertoyBuffer;
LaserPointerCBuffer	 g_LaserPointerBuffer;
ShadowMapVertexShaderMatrixCB g_ShadowMapVSCBuffer;
MetricReconstructionCB g_MetricRecCBuffer;

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

SmallestK g_LaserList;
bool g_bEnableLaserLights = false;
bool g_b3DSunPresent = false;
bool g_b3DSkydomePresent = false;

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

bool g_bDumpOBJEnabled = false;
FILE *g_DumpOBJFile = NULL, *g_DumpLaserFile = NULL;
int g_iOBJFileIdx = 0;
int g_iDumpOBJIdx = 1, g_iDumpLaserOBJIdx = 1;

void DumpVerticesToOBJ(FILE *file, LPD3DINSTRUCTION instruction, UINT curIndex, int &OBJIdx)
{
	LPD3DTRIANGLE triangle = (LPD3DTRIANGLE)(instruction + 1);
	uint32_t index;
	UINT idx = curIndex;
	Vector3 tempv0, tempv1, tempv2;
	//Vector2 v0, v1, v2;
	//float w0, w1, w2;
	std::vector<int> indices;
	int FaceIdx = 1;

	if (file == NULL) {
		log_debug("[DBG] Cannot dump vertices, NULL file ptr");
		return;
	}

	// Start a new object
	fprintf(file, "o obj_%d\n", OBJIdx);
	indices.clear();
	for (uint32_t i = 0; i < instruction->wCount; i++)
	{
		// Back-project the vertices of the triangle into metric 3D space:
		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v1;
		//v0.x = g_OrigVerts[index].sx; v0.y = g_OrigVerts[index].sy; w0 = 1.0f / g_OrigVerts[index].rhw;
		backProjectMetric(index, &tempv0);
		//fprintf(file, "# %0.3f %0.3f %0.6f\n", v0.x, v0.y, w0);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv0.x, tempv0.y, tempv0.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v2;
		//v1.x = g_OrigVerts[index].sx; v1.y = g_OrigVerts[index].sy; w1 = 1.0f / g_OrigVerts[index].rhw;
		backProjectMetric(index, &tempv1);
		//fprintf(file, "# %0.3f %0.3f %0.6f\n", v1.x, v1.y, w1);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv1.x, tempv1.y, tempv1.z);

		index = g_config.D3dHookExists ? g_OrigIndex[idx++] : triangle->v3;
		//v2.x = g_OrigVerts[index].sx; v2.y = g_OrigVerts[index].sy; w2 = 1.0f / g_OrigVerts[index].rhw;
		backProjectMetric(index, &tempv2);
		//fprintf(file, "# %0.3f %0.3f %0.6f\n", v2.x, v2.y, w2);
		fprintf(file, "v %0.6f %0.6f %0.6f\n", tempv2.x, tempv2.y, tempv2.z);

		if (!g_config.D3dHookExists) triangle++;

		indices.push_back(FaceIdx++);
		indices.push_back(FaceIdx++);
		indices.push_back(FaceIdx++);
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
	if (g_bUseSeparateEyeBuffers && g_ExecuteCount == 0) {//only wait once per frame
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
			g_iDumpOBJIdx = 1; g_iDumpLaserOBJIdx = 1;
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
							if (g_bUseSeparateEyeBuffers)
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
								if (g_bUseSeparateEyeBuffers)
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
						if (g_bUseSeparateEyeBuffers)
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
							if (g_bUseSeparateEyeBuffers) {
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

				// Dynamic Cockpit: Remove all the alpha overlays in hi-res mode
				if (g_bDCManualActivate && g_bDynCockpitEnabled &&
					bLastTextureSelectedNotNULL && lastTextureSelected->is_DynCockpitAlphaOverlay)
					goto out;

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
					//log_debug("[DBG] Smoke: %s", lastTextureSelected->_surface->_name);
					bModifiedShaders = true;
					//EnableTransparency();
					g_PSCBuffer.special_control = SPECIAL_CONTROL_SMOKE;
				}

				//if (bLastTextureSelectedNotNULL && lastTextureSelected->is_Spark) {
				//	log_debug("[DBG] Spark: %s", lastTextureSelected->_surface->_name);
				//}

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
					g_ShadertoyBuffer.bDisneyStyle = lastTextureSelected->material.ExplosionBlendMode; // AlphaBlendEnabled: true blend with original texture, false: replace original texture
					g_ShadertoyBuffer.tunnel_speed = lerp(3.0f, -1.0f, ExplosionTime); // ExplosionTime: 3..-1 The animation is performed by iTime in VolumetricExplosion()
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
					static float iTime = 0.0f;
					iTime = g_HiResTimer.global_time_s * lastTextureSelected->material.LavaSpeed;

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
						g_PSCBuffer.fBloomStrength = 0.0f;
						resources->InitPixelShader(resources->_noGlassPS);
					}
				}

				// Apply the SSAO mask/Special materials, like lasers and HUD
				//if (g_bAOEnabled && bLastTextureSelectedNotNULL) 
				if (bLastTextureSelectedNotNULL)
				{
					if (g_bIsScaleableGUIElem || bIsReticle || bIsText || g_bIsTrianglePointer || 
						lastTextureSelected->is_Debris || lastTextureSelected->is_GenericSSAOMasked ||
						lastTextureSelected->is_Electricity || bIsExplosion ||
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
						int anim_idx = lastTextureSelected->material.LightMapATCIndex;
						g_PSCBuffer.fBloomStrength = lastTextureSelected->is_CockpitTex ?
							g_BloomConfig.fCockpitStrength : g_BloomConfig.fLightMapsStrength;
						// If this is an animated light map, then use the right intensity setting
						// TODO: Make the following code more efficient
						if (anim_idx > -1) {
							AnimatedTexControl *atc = &(g_AnimatedMaterials[anim_idx]);
							g_PSCBuffer.fBloomStrength = atc->Sequence[atc->AnimIdx].intensity;
						}
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
					else if (!bIsLightTexture && lastTextureSelected->material.TextureATCIndex > -1) {
						bModifiedShaders = true;
						int anim_idx = lastTextureSelected->material.TextureATCIndex;
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

				// Animated Light Maps/Textures
				if (bHasMaterial) {
					if ((bIsLightTexture && lastTextureSelected->material.LightMapATCIndex > -1) ||
						(!bIsLightTexture && lastTextureSelected->material.TextureATCIndex > -1))
					{
						bModifiedPixelShader = true;
						//log_debug("[DBG] %s, LightMapATCIndex: %d, TextureATCIndex: %d", lastTextureSelected->_surface->_name,
						//	lastTextureSelected->material.LightMapATCIndex, lastTextureSelected->material.TextureATCIndex);

						//int ATCIndex = bIsLightTexture ?
						//	lastTextureSelected->material.LightMapATCIndex : lastTextureSelected->material.TextureATCIndex;

						// The entry condition into this block makes it impossible for ATCIndex to end up with a -1:
						// One of LightMapATCIndex or TextureATCIndex must be > -1
						int ATCIndex = -1;
						if (bIsLightTexture) {
							resources->InitPixelShader(resources->_pixelShaderAnimLightMap);
							ATCIndex = lastTextureSelected->material.LightMapATCIndex;
						}
						else {
							// If we're rendering a DC element, we don't want to replace the shader
							if (g_PSCBuffer.DynCockpitSlots == 0)
								resources->InitPixelShader(resources->_noGlassPS);
							ATCIndex = lastTextureSelected->material.TextureATCIndex;
						}

						AnimatedTexControl *atc = &(g_AnimatedMaterials[ATCIndex]);
						int idx = atc->AnimIdx;
						//log_debug("[DBG] %s, ATCIndex: %d", lastTextureSelected->_surface->_name, ATCIndex);

						//int rand_idx = rand() % lastTextureSelected->material.LightMapSequence.size();
						int extraTexIdx = atc->Sequence[idx].ExtraTextureIndex;
						if (atc->BlackToAlpha)
							g_PSCBuffer.special_control = SPECIAL_CONTROL_BLACK_TO_ALPHA;

						/*
						// DEBUG
						static std::vector<int> DumpedIndices;
						bool bInVector = false;
						for each (int index in DumpedIndices)
							if (index == extraTexIdx) {
								bInVector = true;
								break;
							}
						if (!bInVector) {
							wchar_t filename[80];
							ID3D11Resource *res = NULL;
							resources->_extraTextures[extraTexIdx]->GetResource(&res);
							swprintf_s(filename, 80, L"c:\\temp\\_extraTex-%d.png", extraTexIdx);
							DirectX::SaveWICTextureToFile(context, res, GUID_ContainerFormatPng, filename);
							DumpedIndices.push_back(extraTexIdx);
							log_debug("[DBG] Dumped extraTex %d", extraTexIdx);
						}
						// DEBUG
						*/

						if (extraTexIdx > -1) {
							// Use the following when using std::vector<ID3D11ShaderResourceView*>:
							resources->InitPSShaderResourceView(resources->_extraTextures[extraTexIdx]);
						}
					}
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
				if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled && bIsCockpit) {
					log_debug("[DBG] Dumping OBJ (Cockpit): %s", lastTextureSelected->_surface->_name);
					DumpVerticesToOBJ(g_DumpOBJFile, instruction, currentIndexLocation, g_iDumpOBJIdx);
				}
				if (g_bDumpSSAOBuffers && g_bDumpOBJEnabled && bIsLaser) {
					log_debug("[DBG] Dumping OBJ (Laser): %s", lastTextureSelected->_surface->_name);
					DumpVerticesToOBJ(g_DumpLaserFile, instruction, currentIndexLocation, g_iDumpLaserOBJIdx);
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
					if (g_bUseSeparateEyeBuffers) {
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
					if (g_bUseSeparateEyeBuffers) {
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
					if (g_bUseSeparateEyeBuffers) {
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
					if (g_bUseSeparateEyeBuffers) {
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

	// Update the hi-res timer
	g_HiResTimer.GetElapsedTime();

	g_CurrentHeadingViewMatrix = GetCurrentHeadingViewMatrix();
	UpdateDCHologramState();
	AnimateMaterials();

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
		if (g_bUseSeparateEyeBuffers) {
			context->ClearRenderTargetView(this->_deviceResources->_renderTargetViewR, this->_deviceResources->clearColor);
			context->ClearRenderTargetView(resources->_shadertoyRTV_R, resources->clearColorRGBA);
		}
	}

	// Clear the Bloom Mask RTVs -- SSDO also uses the bloom mask (and maybe SSAO should too), so we have to clear them
	// even if the Bloom effect is disabled
	if (g_bBloomEnabled || resources->_renderTargetViewBloomMask != NULL) 
		if (!bTransitionToHyperspace) {
			context->ClearRenderTargetView(resources->_renderTargetViewBloomMask, resources->clearColor);
			if (g_bUseSeparateEyeBuffers)
				context->ClearRenderTargetView(resources->_renderTargetViewBloomMaskR, resources->clearColor);
		}

	if (g_bReshadeEnabled && !bTransitionToHyperspace) {
		float infinity[4] = { 0, 0, 32000.0f, 0 };
		float zero[4] = { 0, 0, 0, 0 };
		context->ClearRenderTargetView(resources->_renderTargetViewNormBuf, infinity);
		context->ClearRenderTargetView(resources->_renderTargetViewSSAOMask, zero);
		if (g_bUseSeparateEyeBuffers) {
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
		if (g_bUseSeparateEyeBuffers)
			context->ClearRenderTargetView(resources->_renderTargetViewDepthBufR, infinity);
	}

	if (!bTransitionToHyperspace) {
		context->ClearDepthStencilView(resources->_depthStencilViewL, D3D11_CLEAR_DEPTH, resources->clearDepth, 0);
		if (g_bUseSeparateEyeBuffers)
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
