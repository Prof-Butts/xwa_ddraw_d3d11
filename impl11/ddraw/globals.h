#pragma once

#include "commonVR.h"
#include "Effects.h"
#include "ActiveCockpit.h"
#include "DynamicCockpit.h"
#include "ShadowMapping.h"
#include "SharedMem.h"
#include <map>
#include <rtcore.h> //Embree

// METRIC RECONSTRUCTION:
extern bool g_bYCenterHasBeenFixed;

extern bool g_bDisableBarrelEffect, g_bEnableVR, g_bResetHeadCenter, g_bBloomEnabled, g_bAOEnabled, g_bCustomFOVApplied;
extern bool g_b3DVisionEnabled, g_b3DVisionForceFullScreen;
extern bool g_bEnableSSAOInShader, g_bEnableIndirectSSDO, g_bResetDC, g_bProceduralSuns, g_bEnableHeadLights;
extern bool g_bShowSSAODebug, g_bShowNormBufDebug, g_bFNEnable, g_bGlobalSpecToggle, g_bToggleSkipDC, g_bFadeLights, g_bDisplayGlowMarks;
extern Vector4 g_LightVector[2];
extern float g_fSpecIntensity, g_fSpecBloomIntensity, g_fFocalDist, g_fFakeRoll, g_fMinLightIntensity, g_fGlowMarkZOfs;
extern int g_iDelayedDumpDebugBuffers;

extern bool g_bRendering3D; // Used to distinguish between 2D (Concourse/Menus) and 3D rendering (main in-flight game)
// g_fZOverride is activated when it's greater than -0.9f, and it's used for bracket rendering so that 
// objects cover the brackets. In this way, we avoid visual contention from the brackets.
extern float g_fCurInGame2DWidth, g_fCurInGame2DHeight;

extern bool g_bZoomOut, g_bZoomOutInitialState;

extern int g_KeySet; // Default setting: let users adjust the FOV, I can always override this with the "key_set" SSAO.cfg param

// Counter for calls to DrawIndexed() (This helps us know where were are in the rendering process)
// Gets reset everytime the backbuffer is presented and is increased only after BOTH the left and
// right images have been rendered.
extern int g_iDrawCounter, g_iNoDrawBeforeIndex, g_iNoDrawAfterIndex, g_iDrawCounterAfterHUD;
// Similar to the above, but only get incremented after each Execute() is finished.
extern int g_iNoExecBeforeIndex, g_iNoExecAfterIndex, g_iNoDrawAfterHUD;
// The Skybox cannot be detected using g_iExecBufCounter anymore when using the hook_d3d because the whole frame is
// rendered with a single Execute call
//extern int g_iSkyBoxExecIndex = DEFAULT_SKYBOX_INDEX; // This gives us the threshold for the Skybox
// g_iSkyBoxExecIndex is compared against g_iExecBufCounter to determine when the SkyBox is rendered
// This is important because in XWAU the SkyBox is *not* rendered at infinity and causes a lot of
// visual contention if not handled properly.
extern bool g_bIsSkyBox, g_bPrevIsSkyBox, g_bSkyBoxJustFinished;

extern bool g_bFixSkyBox; // Fix the skybox (send it to infinity: use original vertices without parallax)
extern bool g_bSkipSkyBox;
extern bool g_bStartedGUI; // Set to false at the beginning of each frame. Set to true when the GUI has begun rendering.
extern bool g_bPrevStartedGUI; // Keeps the last value of g_bStartedGUI -- helps detect when the GUI is about to be rendered.
extern bool g_bIsScaleableGUIElem; // Set to false at the beginning of each frame. Set to true when the scaleable GUI has begun rendering.
extern bool g_bPrevIsScaleableGUIElem; // Keeps the last value of g_bIsScaleableGUIElem -- helps detect when scaleable GUI is about to start rendering.
extern bool g_bScaleableHUDStarted; // Becomes true as soon as the scaleable HUD is about to be rendered. Reset to false at the end of each frame
extern bool g_bSkipGUI; // Skip non-skybox draw calls with disabled Z-Buffer
extern bool g_bSkipText; // Skips text draw calls
extern bool g_bSkipAfterTargetComp; // Skip all draw calls after the targetting computer has been drawn
extern bool g_bTargetCompDrawn; // Becomes true after the targetting computer has been drawn
extern bool g_bPrevIsFloatingGUI3DObject; // Stores the last value of g_bIsFloatingGUI3DObject -- useful to detect when the targeted craft is about to be drawn
extern bool g_bIsFloating3DObject; // true when rendering the targeted 3D object.
extern bool g_bIsTrianglePointer, g_bLastTrianglePointer;
extern bool g_bIsPlayerObject, g_bPrevIsPlayerObject, g_bHyperspaceEffectRenderedOnCurrentFrame;
extern bool g_bIsTargetHighlighted; // True if the target can  be fired upon, gets reset every frame
extern bool g_bPrevIsTargetHighlighted; // The value of g_bIsTargetHighlighted for the previous frame

//bool g_bLaserBoxLimitsUpdated = false; // Set to true whenever the laser/ion charge limit boxes are updated
extern unsigned int g_iFloatingGUIDrawnCounter;
extern int g_iPresentCounter, g_iNonZBufferCounter, g_iSkipNonZBufferDrawIdx;
constexpr int PLAYERDATATABLE_MIN_SAFE_FRAME = 5;
extern float g_fZBracketOverride; // 65535 is probably the maximum Z value in XWA
extern bool g_bResetDC;

// Performance Counters and Timing
#include "HiResTimer.h"

// DS2 Effects
extern int g_iReactorExplosionCount;
extern bool g_bDS2ForceProceduralExplosions;





// HYPERSPACE
extern HyperspacePhaseEnum g_HyperspacePhaseFSM;
extern short g_fLastCockpitCameraYaw, g_fLastCockpitCameraPitch;
extern int g_lastCockpitXReference, g_lastCockpitYReference, g_lastCockpitZReference;
extern float g_fHyperspaceTunnelSpeed, g_fHyperShakeRotationSpeed, g_fHyperLightRotationSpeed, g_fHyperspaceRand;
extern float g_HyperTwirl;
extern float g_fCockpitCameraYawOnFirstHyperFrame, g_fCockpitCameraPitchOnFirstHyperFrame, g_fCockpitCameraRollOnFirstHyperFrame;
extern float g_fHyperTimeOverride; // DEBUG, remove later
extern int g_iHyperStateOverride; // DEBUG, remove later
extern bool g_bHyperDebugMode; // DEBUG -- needed to fine-tune the effect, won't be able to remove until I figure out an automatic way to setup the effect
extern bool g_bHyperspaceFirstFrame; // Set to true on the first frame of hyperspace, reset to false at the end of each frame
extern bool g_bClearedAuxBuffer, g_bExecuteBufferLock;
extern bool g_bHyperHeadSnapped, g_bHyperspaceEffectRenderedOnCurrentFrame;
extern int g_iHyperExitPostFrames;
extern bool g_bKeybExitHyperspace;
extern Vector4 g_TempLightColor[2], g_TempLightVector[2];
extern int g_iHyperStyle;
extern bool g_bInterdictionActive;
extern uint8_t g_iInterdictionBitfield;
extern float g_fInterdictionShake;
extern float g_fInterdictionShakeInVR;
extern float g_fInterdictionAngleScale;

extern bool g_bFXAAEnabled;



// SSAO
extern SSAOTypeEnum g_SSAO_Type;
extern float g_fSSAOZoomFactor, g_fSSAOZoomFactor2, g_fSSAOWhitePoint, g_fNormWeight, g_fNormalBlurRadius;
extern int g_iSSDODebug, g_iSSAOBlurPasses;
extern bool g_bBlurSSAO, g_bDepthBufferResolved, g_bOverrideLightPos;
extern bool g_bShowSSAODebug, g_bEnableIndirectSSDO, g_bFNEnable;
extern bool g_bDumpSSAOBuffers, g_bEnableSSAOInShader, g_bEnableBentNormalsInShader;
extern Vector4 g_LightVector[2];
extern Vector4 g_LightColor[2];
extern float g_fViewYawSign, g_fViewPitchSign;
extern float g_fMoireOffsetDir, g_fMoireOffsetInd;

extern Vector2 g_TriangleCentroid;
extern float g_fTrianglePointerDist;

// DEBUG vars
extern bool g_enable_ac_debug;
extern Vector3 g_debug_v0, g_debug_v1, g_debug_v2;
extern bool g_bDumpLaserPointerDebugInfo;
extern Vector3 g_LPdebugPoint;
extern float g_fLPdebugPointOffset, g_fDebugYCenter;
extern bool g_bApplyCockpitDamage, g_bResetCockpitDamage, g_bResetCockpitDamageInHangar;
extern bool g_bAutoGreeblesEnabled;
extern bool g_bShowBlastMarks;
extern float g_fBlastMarkOfsX, g_fBlastMarkOfsY;

/*
 * Dumps the vertices in the current instruction to the given file after back-projecting them
 * into 3D space.
 * After dumping solid polygons, I realized the back-project code skews the Y axis... considerably.
 */
extern bool g_bDumpOBJEnabled;
//extern FILE* g_DumpOBJFile;
//extern int g_iDumpOBJFaceIdx, g_iDumpOBJIdx;
// DEBUG vars

extern int g_iNaturalConcourseAnimations, g_iHUDOffscreenCommandsRendered;
extern bool g_bIsTrianglePointer, g_bLastTrianglePointer, g_bFixedGUI, g_bFloatingAimingHUD;
extern bool g_bYawPitchFromMouseOverride, g_bIsSkyBox, g_bPrevIsSkyBox, g_bSkyBoxJustFinished;
extern bool g_bIsPlayerObject, g_bPrevIsPlayerObject, g_bSwitchedToGUI;
extern bool g_bIsTargetHighlighted, g_bPrevIsTargetHighlighted;

// SPEED SHADER EFFECT
extern bool g_bHyperspaceTunnelLastFrame, g_bHyperspaceLastFrame;
extern bool g_bEnableSpeedShader, g_bEnableAdditionalGeometry;
extern float g_fSpeedShaderScaleFactor, g_fSpeedShaderParticleSize, g_fSpeedShaderMaxIntensity, g_fSpeedShaderTrailSize, g_fSpeedShaderParticleRange;
extern float g_fCockpitTranslationScale;
extern int g_iSpeedShaderMaxParticles;
//Vector4 g_prevFs(0, 0, 0, 0), g_prevUs(0, 0, 0, 0); //Not used
extern D3DTLVERTEX g_SpeedParticles2D[MAX_SPEED_PARTICLES * 12];



/*
 * Used to store a list of textures for fast lookup. For instance, all suns must
 * have their associated lights reset after jumping through hyperspace; and all
 * textures with materials can be placed here so that material properties can be
 * applied while flying.
 */
extern std::vector<Direct3DTexture*> g_AuxTextureVector;

// DS2 Effects
extern int g_iReactorExplosionCount;

extern Matrix4 g_CurrentHeadingViewMatrix;

extern bool g_bExternalHUDEnabled, g_bEdgeDetectorEnabled, g_bStarDebugEnabled;

extern float g_f2DYawMul, g_f2DPitchMul, g_f2DRollMul;
extern TrackerType g_TrackerType;

extern bool g_bAOEnabled;

extern float g_fCurInGameHeight;

extern float g_fDefaultFOVDist;
extern float g_fDebugFOVscale, g_fDebugYCenter;
extern float g_fCurrentShipFocalLength, g_fCurrentShipLargeFocalLength, g_fReticleScale;
extern bool g_bYCenterHasBeenFixed;

extern uint32_t* g_rawFOVDist; /* = (uint32_t *)0x91AB6C*/ // raw FOV dist(dword int), copy of one of the six values hard-coded with the resolution slots, which are what xwahacker edits
extern float* g_fRawFOVDist; /*= (float *)0x8B94CC;*/ // FOV dist(float), same value as above
extern float* g_cachedFOVDist; /*= (float *)0x8B94BC;*/ // cached FOV dist / 512.0 (float), seems to be used for some sprite processing

extern float g_fCurrentShipFocalLength, g_fCurrentShipLargeFocalLength, g_fVR_FOV;
extern float g_fDebugFOVscale, g_fDebugYCenter;
extern bool g_bCustomFOVApplied, g_bLastFrameWasExterior;
extern float g_fRealHorzFOV, g_fRealVertFOV;

extern SharedMem<SharedMemDataCockpitLook> g_SharedMemCockpitLook;
extern SharedMem<SharedMemDataTgSmush> g_SharedMemTgSmush;
extern SharedMemDataCockpitLook* g_pSharedDataCockpitLook;
extern SharedMemDataTgSmush* g_pSharedDataTgSmush;

// Custom HUD colors
extern uint32_t g_iHUDInnerColor, g_iHUDBorderColor;
extern uint32_t g_iOriginalHUDInnerColor, g_iOriginaHUDBorderColor;
// Laser/Ion Cannon counting vars
extern bool g_bLasersIonsNeedCounting;
extern int g_iNumLaserCannons, g_iNumIonCannons;

// Alternate Explosions
constexpr int MAX_XWA_EXPLOSIONS = 7; // XWA only has 7 DAT Groups for explosions
// We can load alternate versions of each one of the 7 explosions that XWA uses. The version
// that is displayed is selected with g_AltExplosionSelector. Every few frames we'll randomly
// choose another version provided that no explosions are being displayed at the moment of the
// switch.
extern int g_AltExplosionSelector[MAX_XWA_EXPLOSIONS];
// Reset to false during Present(). Set to true if at least one explosion was displayed in the
// current frame.
extern bool g_bExplosionsDisplayedOnCurrentFrame;
extern int g_iForceAltExplosion;

// Sun Colors, to be used to apply colors to the flares later
extern float4 g_SunColors[MAX_SUN_FLARES];
extern int g_iSunFlareCount;

extern bool g_bMapMode;
extern int  g_ExecuteCount;

// D3DRendererHook draw call counter;
extern int g_iD3DExecuteCounter;
constexpr float OPT_TO_METERS = 1.0f / 40.96f;
constexpr float METERS_TO_OPT = 40.96f;

constexpr float GLOVE_NEAR_THRESHOLD_METERS = 0.05f;
constexpr float GLOVE_NEAR_THRESHOLD_OPTSCALE = GLOVE_NEAR_THRESHOLD_METERS * METERS_TO_OPT;

// Raytracing
enum class BLASBuilderType
{
	BVH2,
	QBVH,
	FastQBVH,
	//Embree,
	//DirectBVH2CPU,
	DirectBVH4GPU,
	Online,
	PQ,
	PLOC,
	MAX,
};

enum class TLASBuilderType
{
	FastQBVH,
	DirectBVH4GPU,
	MAX
};

constexpr BLASBuilderType DEFAULT_BLAS_BUILDER = BLASBuilderType::FastQBVH;
constexpr TLASBuilderType DEFAULT_TLAS_BUILDER = TLASBuilderType::FastQBVH;
extern BLASBuilderType g_BLASBuilderType;
extern TLASBuilderType g_TLASBuilderType;
extern char* g_sBLASBuilderTypeNames[(int)BLASBuilderType::MAX];
extern char* g_sTLASBuilderTypeNames[(int)TLASBuilderType::MAX];

extern bool g_bRTEnabledInTechRoom;
extern bool g_bRTEnabled;
extern bool g_bRTEnabledInCockpit;
extern bool g_bRTEnableSoftShadows;
extern float g_fRTSoftShadowThresholdMult;
extern float g_fRTGaussFactor;
extern int g_iRTTotalBLASNodesInFrame, g_iRTMaxBLASNodesSoFar, g_iRTMaxTLASNodesSoFar;
extern uint32_t g_iRTMaxMeshesSoFar;
extern int g_iRTMatricesNextSlot;
extern bool g_bRTReAllocateBvhBuffer;
extern bool g_bRTEnableEmbree;
extern std::map<std::string, bool> g_RTExcludeOPTNames;
extern std::map<uint8_t, bool> g_RTExcludeShipCategories;
extern std::map<int, bool> g_RTExcludeMeshes;

extern RTCDevice g_rtcDevice;
extern RTCScene g_rtcScene;

// Levels.fx
extern bool g_bEnableLevelsShader;
extern float g_fLevelsWhitePoint;
extern float g_fLevelsBlackPoint;

// Gimbal Lock Fix
// Configurable settings
extern bool g_bEnableGimbalLockFix, g_bGimbalLockFixActive;
extern bool g_bEnableRudder, g_bYTSeriesShip;
extern float g_fRollFromYawScale;

extern float g_fYawAccelRate_s;
extern float g_fPitchAccelRate_s;
extern float g_fRollAccelRate_s;

extern float g_fMaxYawRate_s;
extern float g_fMaxPitchRate_s;
extern float g_fMaxRollRate_s;

extern float g_fTurnRateScaleThr_0;
extern float g_fTurnRateScaleThr_100;
extern float g_fMaxTurnAccelRate_s;
extern bool g_bThrottleModulationEnabled;

// Gimbal lock debug settings
extern float g_fMouseRangeX, g_fMouseRangeY;
extern float g_fMouseDecelRate_s;

// OPT Debugging
extern bool g_bDumpOptNodes;

// ZIP variables
extern bool g_bCleanupZIPDirs;

struct BracketVR
{
	Vector3 posOPT;
	float halfWidthOPT;
	float strokeWidth;
	Vector3 color;
	float rollCompensation;
};
extern std::vector<BracketVR> g_bracketsVR;

// Enhanced HUD
extern bool g_bEnableEnhancedHUD;

// *****************************************************
// Global functions
// *****************************************************
void RenderDeferredDrawCalls();