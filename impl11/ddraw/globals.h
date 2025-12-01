#pragma once

#include "commonVR.h"
#include "Effects.h"
#include "ActiveCockpit.h"
#include "DynamicCockpit.h"
#include "ShadowMapping.h"
#include "SharedMem.h"
#include <map>
#include <rtcore.h> //Embree

#define MOVE_LIGHTS_KEY_SET      1
#define CHANGE_FOV_KEY_SET       2 // Default
#define MOVE_POINT_LIGHT_KEY_SET 3
#define ROTATE_CUBEMAPS_KEY_SET  16

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
extern bool g_bShowSSAODebug, g_bEnableIndirectSSDO, g_bFNEnable, g_bSpecularMappingEnabled;
extern bool g_bDumpSSAOBuffers, g_bEnableSSAOInShader, g_bEnableBentNormalsInShader;
extern Vector4 g_LightVector[2];
extern Vector4 g_LightColor[2];
extern float g_fViewYawSign, g_fViewPitchSign;
extern float g_fMoireOffsetDir, g_fMoireOffsetInd;

extern Vector2 g_TriangleCentroid;
extern float g_fTrianglePointerDist;
extern float g_fTrianglePointerSize;
extern bool g_bTrianglePointerEnabled;

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
extern bool g_bEnableDeveloperMode;
extern bool g_bBackdropTransformCaptured;
extern Matrix4 g_BackdropTransform;

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
class XwaTextureData;
extern std::vector<XwaTextureData*> g_AuxTextureVector;

// Texture loading.
extern std::vector<char>* g_textureSurfaceBuffer;
extern std::vector<char>* g_d3dTextureBuffer;
extern std::vector<uint8_t>* g_loadDatImageBuffer;
extern std::vector<unsigned char>* g_colorMapBuffer;
extern std::vector<unsigned char>* g_illumMapBuffer;
void ClearTextureBuffers();

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

// XwaDDrawPlayer
extern bool g_bXwaDDrawPlayerPresent;

extern bool g_bForce60FPS;

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
	int  widthPix;
	bool isSubComponent;
};
extern std::vector<BracketVR> g_bracketsVR;
extern BracketVR g_curTargetBracketVR, g_curSubCmpBracketVR;
extern bool g_curTargetBracketVRCaptured, g_curSubCmpBracketVRCaptured;
constexpr float DOT_MESH_SIZE_M = 0.017f;

// Enhanced HUD
constexpr int VR_ENHANCED_HUD_BUFFER_SIZE = 512;
constexpr int MAX_DC_SHIELDS_CHARS = 5;
constexpr int MAX_DC_SHIP_NAME_CHARS = 40;
struct DCChar
{
	int x, y;
	char c;
	DCChar()
	{
		x = y = -1;
		c = 0;
	}
};

struct EnhancedHUDData
{
	bool Enabled;
	int  MinBracketSize;
	int  MaxBracketSize;
	int  fontIdx;
	std::string sName, sTime, sTgtShds, sTgtHull, sTgtSys;
	std::string sTgtDist, sCargo, sSubCmp, sTmp;
	uint32_t nameColor, statsColor, subCmpColor;
	int   tgtShds, tgtHull, tgtSys;
	float tgtDist;
	int   primMsls[2];
	uint32_t shieldsCol = 0xFF2080FF;
	uint32_t overShdCol = 0xFF7DF9FF; // Electric blue
	uint32_t sysCol     = 0xFFCCC0FF; // Periwinkle!
	float3 hullCol1 = { 0.0f, 1.0f, 0.0f };
	float3 hullCol2 = { 1.0f, 1.0f, 0.0f };
	float3 hullCol3 = { 1.0f, 0.0f, 0.0f };
	bool   enhanceNameColor = true;
	bool   displayBars = true;
	bool   verticalBarLayout = false;
	float  minBarW = 16.0f, maxBarW = 140.0f, barStrokeSize = 3.0f;
	float  barH = 9.0f, gapH = 7.0f;
	float  vrTextScale = 1.0f;
	bool   bgTextBoxComputed, bgTextBoxEnabled = false;
	bool   subCmpBoxComputed, barsBoxComputed;
	bool   applyDepthOcclusion = true;
	int    bgTextBoxNumLines;
	Box    bgTextBox;
	Box    subCmpBox;
	Box    barsBox;
	Box    shdBarBox, hullBarBox, sysBarBox;

	std::string sShieldsFwd, sShieldsBck, sShipName;
	std::string sMissiles, sSpeed, sChaff, sMissilesMis;
	//std::string sThrottle, sSpeed;
	DCChar shdFwdChars[MAX_DC_SHIELDS_CHARS];
	DCChar shdBckChars[MAX_DC_SHIELDS_CHARS];
	DCChar shipNameChars[MAX_DC_SHIP_NAME_CHARS];
	int shdFwdNumChars;
	int shdBckNumChars;
	int shipNameNumChars;
};
extern EnhancedHUDData g_EnhancedHUDData;

constexpr int MAX_MISSION_REGIONS = 4;

enum class CubeMapEditMode
{
	DISABLED,
	AZIMUTH_ELEVATION,
	LOCAL_COORDS
};

struct CubeMapData
{
	bool bRenderAllRegions = false;
	bool bAllRegionsIllum = false;
	bool bAllRegionsOvr = false;
	bool bRenderInThisRegion[MAX_MISSION_REGIONS] = { false, false, false, false };
	bool bRenderIllumInThisRegion[MAX_MISSION_REGIONS] = { false, false, false, false };
	bool bRenderOvrInThisRegion[MAX_MISSION_REGIONS] = { false, false, false, false };
	// When set, this flag uses Jeremy's code to set the CubeMap orientation (fixes hangar CubeMaps)
	bool bUseNewOrientation = true;
	float allRegionsSpecular   = 0.7f;
	float allRegionsAmbientInt = 0.15f;
	float allRegionsAmbientMin = 0.0f;
	float allRegionsDiffuseMipLevel = 5;
	float allRegionsIllumDiffuseMipLevel = 5;
	float allRegionsAngX = 0.0f, allRegionsAngY = 0.0f, allRegionsAngZ = 0.0f;
	float allRegionsOvrAngX = 0, allRegionsOvrAngY = 0, allRegionsOvrAngZ = 0;
	Vector4 allRegionsR, allRegionsU, allRegionsF;
	float allRegionsTexRes = -1;
	float allRegionsIllumTexRes = -1;
	float allRegionsMipRes = 16.0f;
	float allRegionsIllumMipRes = 16.0f;
	float hangarAngX = 0.0f, hangarAngY = 0.0f, hangarAngZ = 0.0f;
	float hangarOvrAngX = 0.0f, hangarOvrAngY = 0.0f, hangarOvrAngZ = 0.0f;

	float regionSpecular[MAX_MISSION_REGIONS];
	float regionAmbientInt[MAX_MISSION_REGIONS];
	float regionAmbientMin[MAX_MISSION_REGIONS];
	float regionDiffuseMipLevel[MAX_MISSION_REGIONS];
	float regionIllumDiffuseMipLevel[MAX_MISSION_REGIONS];
	float regionAngX[MAX_MISSION_REGIONS] = { 0, 0, 0, 0 };
	float regionAngY[MAX_MISSION_REGIONS] = { 0, 0, 0, 0 };
	float regionAngZ[MAX_MISSION_REGIONS] = { 0, 0, 0, 0 };
	float regionOvrAngX[MAX_MISSION_REGIONS] = { 0, 0, 0, 0 };
	float regionOvrAngY[MAX_MISSION_REGIONS] = { 0, 0, 0, 0 };
	float regionOvrAngZ[MAX_MISSION_REGIONS] = { 0, 0, 0, 0 };
	float regionTexRes[MAX_MISSION_REGIONS] = { -1, -1, -1, -1 };
	float regionIllumTexRes[MAX_MISSION_REGIONS] = { -1, -1, -1, -1 };
	float regionMipRes[MAX_MISSION_REGIONS];
	float regionIllumMipRes[MAX_MISSION_REGIONS];

	ID3D11ShaderResourceView* allRegionsSRV = nullptr;
	ID3D11ShaderResourceView* allRegionsIllumSRV = nullptr;
	ID3D11ShaderResourceView* allRegionsOvrSRV = nullptr;
	ID3D11ShaderResourceView* regionSRV[MAX_MISSION_REGIONS] = { nullptr, nullptr, nullptr, nullptr };
	ID3D11ShaderResourceView* regionIllumSRV[MAX_MISSION_REGIONS] = { nullptr, nullptr, nullptr, nullptr };
	ID3D11ShaderResourceView* regionOvrSRV[MAX_MISSION_REGIONS] = { nullptr, nullptr, nullptr, nullptr };

	// Live edit mode variables
	CubeMapEditMode editMode = CubeMapEditMode::DISABLED;
	bool editParamsModified = false;
	bool editOverlays = false;
	float editAngX = 0, editAngY = 0, editAngZ = 0;
	float editOvrAngX = 0, editOvrAngY = 0, editOvrAngZ = 0;
	float editAngIncr = 2.0f;
	int editCurRegion  = -1;
	int editPrevRegion = 0xFFFF;
	Vector4 editAllRegionsR, editAllRegionsU, editAllRegionsF;
	Vector4 editAllRegionsOvrR, editAllRegionsOvrU, editAllRegionsOvrF;
	Vector4 editRegionR[MAX_MISSION_REGIONS] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	Vector4 editRegionU[MAX_MISSION_REGIONS] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	Vector4 editRegionF[MAX_MISSION_REGIONS] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	Vector4 editRegionOvrR[MAX_MISSION_REGIONS] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	Vector4 editRegionOvrU[MAX_MISSION_REGIONS] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	Vector4 editRegionOvrF[MAX_MISSION_REGIONS] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };

	Vector4& RVector(int region) { if (region == -1) return editAllRegionsR; else return editRegionR[region]; }
	Vector4& UVector(int region) { if (region == -1) return editAllRegionsU; else return editRegionU[region]; }
	Vector4& FVector(int region) { if (region == -1) return editAllRegionsF; else return editRegionF[region]; }

	Vector4& OvrRVector(int region) { if (region == -1) return editAllRegionsOvrR; else return editRegionOvrR[region]; }
	Vector4& OvrUVector(int region) { if (region == -1) return editAllRegionsOvrU; else return editRegionOvrU[region]; }
	Vector4& OvrFVector(int region) { if (region == -1) return editAllRegionsOvrF; else return editRegionOvrF[region]; }
};
extern CubeMapData g_CubeMaps;
void CubeMapEditResetAngles();
void CubeMapEditResetRUF();
void CubeMapEditIncrAngX(float mult, bool overlay);
void CubeMapEditIncrAngY(float mult, bool overlay);
void CubeMapEditIncrAngZ(float mult, bool overlay);

// *****************************************************
// Global functions
// *****************************************************
void RenderDeferredDrawCalls();
