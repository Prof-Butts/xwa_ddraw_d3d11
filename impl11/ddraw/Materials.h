#pragma once

#include "XWAFramework.h"
#include "Matrices.h"
#include "../shaders/material_defs.h"
#include "../shaders/shader_common.h"
#include <vector>

constexpr auto MAX_CACHED_MATERIALS = 32;
constexpr auto MAX_TEXNAME = 40;
constexpr auto MAX_OPT_NAME = 80;
constexpr auto MAX_TEX_SEQ_NAME = 80;
constexpr auto MAX_CANNONS = 8;
constexpr auto MAX_GREEBLE_NAME = 80;
constexpr auto MAX_GREEBLE_LEVELS = 2;
constexpr auto MAX_NORMALMAP_NAME = 80;

float lerp(float x, float y, float s);

typedef enum GreebleBlendModeEnum {
	GBM_MULTIPLY = 1,
	GBM_OVERLAY,
	GBM_SCREEN,
	GBM_REPLACE,
	GBM_NORMAL_MAP,
	GBM_UV_DISP,
	GBM_UV_DISP_AND_NORMAL_MAP,
	// Add more blending modes here, if you do, also update Materials.cpp:g_sGreebleBlendModes
	GBM_MAX_MODES, // Sentinel, do not remove!
} GreebleBlendMode;

extern char *g_sGreebleBlendModes[GBM_MAX_MODES - 1];

typedef enum DiegeticMeshEnum {
	DM_NONE,

	DM_JOYSTICK,

	DM_THR_ROT_X,
	DM_THR_ROT_Y,
	DM_THR_ROT_Z,
	DM_THR_TRANS,
	DM_THR_ROT_ANY,

	DM_HYPER_ROT_X,
	DM_HYPER_ROT_Y,
	DM_HYPER_ROT_Z,
	DM_HYPER_TRANS,
	DM_HYPER_ROT_ANY,
} DiegeticMeshType;

/*

How to add support for a new global event:

1. Add the new event in GameEventEnum
2. Add the corresponding string for the new event in g_sGameEventNames, follow the naming rules!
3. Add a new field to g_GameEvent (if applicable).
4. Add support in BeginScene (or wherever relevant) to detect the new event. Add it to g_GameEvent.<Relevant-Field>
5. Update Materials.h:GetCurrentATCIndex(). Mind the priority of the new event!
6. Update Materials.cpp:ResetGameEvent()
7. Update Materials.cpp:UpdateEventsFired()

How to add support for a new instance event:

1. Add the new event in InstEventType
2. Add the corresponding string for the new event in Materials.cpp:g_sInstEventNames, follow the naming rules!
3. Add a new field to InstanceEvent (if applicable).
4. Add support in EffectsRenderer::MainSceneHook() (or wherever relevant) to detect the new event.
5. Update Materials.h:GetCurrentInstATCIndex(). Mind the priority of the new event!
6. Update Materials.cpp:ResetGameEvent()
7. Update Materials.cpp:UpdateEventsFired()

*/

constexpr int TEXTURE_ATC_IDX = 0;
constexpr int LIGHTMAP_ATC_IDX = 1;
constexpr int MAX_ATC_TYPES = 2;
constexpr int MAX_ALT_EXPLOSIONS = 5;
/* Alternate explosions will be loaded in slots 0..3. The following slot is for DS2 explosions */
constexpr int DS2_ALT_EXPLOSION_IDX = 4;

// Target Event Enum
// This enum can be used to trigger animations or changes in materials
// when target-relate events occur.
// For now, the plan is to use it to change which animated textures are
// displayed.
typedef enum GameEventEnum {
	EVT_NONE = 0,				// Play when no other event is active
	TGT_EVT_SELECTED,			// Something has been targeted
	TGT_EVT_LASER_LOCK,			// Laser is "locked"
	TGT_EVT_WARHEAD_LOCKING,		// Warhead is locking (yellow)
	TGT_EVT_WARHEAD_LOCKED,		// Warhead is locked (red)
	// Cockpit Instrument Damage Events
	CPT_EVT_BROKEN_CMD,
	CPT_EVT_BROKEN_LASER_ION,
	CPT_EVT_BROKEN_BEAM_WEAPON,
	CPT_EVT_BROKEN_SHIELDS,
	CPT_EVT_BROKEN_THROTTLE,
	CPT_EVT_BROKEN_SENSORS,
	CPT_EVT_BROKEN_LASER_RECHARGE,
	CPT_EVT_BROKEN_ENGINE_POWER,
	CPT_EVT_BROKEN_SHIELD_RECHARGE,
	CPT_EVT_BROKEN_BEAM_RECHARGE,
	// Hull Damage Events
	HULL_EVT_DAMAGE_75,
	HULL_EVT_DAMAGE_50,
	HULL_EVT_DAMAGE_25,
	// Warning Light Events
	WLIGHT_EVT_LEFT,
	WLIGHT_EVT_MID_LEFT,
	WLIGHT_EVT_MID_RIGHT,
	WLIGHT_EVT_RIGHT_YELLOW,
	WLIGHT_EVT_RIGHT_RED,
	// Laser/Ion cannon ready events
	CANNON_EVT_1_READY,
	CANNON_EVT_2_READY,
	CANNON_EVT_3_READY,
	CANNON_EVT_4_READY,
	CANNON_EVT_5_READY,
	CANNON_EVT_6_READY,
	CANNON_EVT_7_READY,
	CANNON_EVT_8_READY,
	// How many more cannons can be supported?
	// ... Add more events here
	// End-of-events sentinel. Do not remove!
	MAX_GAME_EVT
} GameEvent;

extern char *g_sGameEventNames[MAX_GAME_EVT];

typedef enum {
	IEVT_NONE = 0,
	// Hull Damage Events
	IEVT_HULL_DAMAGE_75,
	IEVT_HULL_DAMAGE_50,
	IEVT_HULL_DAMAGE_25,

	IEVT_SHIELDS_DOWN,
	
	IEVT_TRACTOR_BEAM,
	IEVT_JAMMING_BEAM,

	// Add new events here...

	// End-of-events sentinel. Do not remove!
	MAX_INST_EVT
} InstEventType;

extern char *g_sInstEventNames[MAX_INST_EVT];

// *********************
// Cockpit Damage
// *********************
/*
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
*/
typedef union CockpitDamageUnion {
	struct {
		/* 0x0001 */ WORD CMD : 1;
		/* 0x000E */ WORD LaserIon : 3;
		/* 0x0010 */ WORD BeamWeapon : 1;
		/* 0x0020 */ WORD Shields : 1;
		/* 0x0040 */ WORD Throttle : 1;
		/* 0x0180 */ WORD Sensors : 2;
		/* 0x0200 */ WORD LaserRecharge : 1;
		/* 0x0400 */ WORD EngineLevel : 1;
		/* 0x0800 */ WORD ShieldRecharge : 1;
		/* 0x1000 */ WORD BeamRecharge : 1;
	};
	WORD Flags;
} CockpitDamage;

typedef struct CockpitInstrumentStateStruct {
	bool CMD;
	bool LaserIon;
	bool BeamWeapon;
	bool Shields;
	bool Throttle;
	bool Sensors;
	bool LaserRecharge;
	bool EnginePower;
	bool ShieldRecharge;
	bool BeamRecharge;

	CockpitInstrumentStateStruct() {
		CMD = true;
		LaserIon = true;
		BeamWeapon = true;
		Shields = true;
		Throttle = true;
		Sensors = true;
		LaserRecharge = true;
		EnginePower = true;
		ShieldRecharge = true;
		BeamRecharge = true;
	}

	void FromXWADamage(WORD XWADamage);
} CockpitInstrumentState;

typedef struct GlobalGameEventStruct {
	GameEvent TargetEvent;
	CockpitInstrumentState CockpitInstruments, InitialCockpitInstruments;
	bool bCockpitInitialized;
	GameEvent HullEvent;
	// Warning Lights Events
	bool WLightLLEvent;
	bool WLightMLEvent;
	bool WLightMREvent;
	uint8_t WLightRREvent;
	// Cannon Ready Events
	bool CannonReady[MAX_CANNONS];

	GlobalGameEventStruct() {
		TargetEvent = EVT_NONE;
		WLightLLEvent = false;
		WLightMLEvent = false;
		WLightMREvent = false;
		WLightRREvent = 0; // 0: No event, 1: Yellow (locking), 2: Red (locked)
		for (int i = 0; i < MAX_CANNONS; i++)
			CannonReady[i] = false;
		bCockpitInitialized = false;
	}
} GlobalGameEvent;

// Global game event. Updated throughout the frame, reset in Direct3DDevice::BeginScene()
extern GlobalGameEvent g_GameEvent;

// Per-instance events. These events are linked to specific ship instances,
// instead of being global events applied to all textures of the same name.
class InstanceEvent {
public:
	int objectId;

	InstEventType HullEvent = IEVT_NONE;
	InstEventType PrevHullEvent = IEVT_NONE;

	InstEventType ShieldBeamEvent = IEVT_NONE;
	InstEventType PrevShieldBeamEvent = IEVT_NONE;

	bool bEventsFired[MAX_INST_EVT];
	int InstTextureATCIndices[MAX_ATC_TYPES][MAX_INST_EVT];
	// For instance materials, their corresponding ATC is always initialized to an entry
	// in g_AnimatedMaterials. This entry is considered a "template". Once this template
	// has been copied into g_AnimatedInstMaterials, the following flag must be set to true
	bool bATCHasBeenInstanced;
	// Random values, used to rotate and translate the textures for the shields down effect.
	// These values are set each time an event is triggered.
	float rand0, rand1, rand2;

	InstanceEvent()
	{
		objectId = -1;
		bATCHasBeenInstanced = false;
		rand0 = rand1 = rand2 = 0.0f;
		for (int j = 0; j < MAX_ATC_TYPES; j++)
			for (int i = 0; i < MAX_INST_EVT; i++)
				InstTextureATCIndices[j][i] = -1;
	}

	void ResetEventsFired() {
		for (int i = 0; i < MAX_INST_EVT; i++)
			bEventsFired[i] = false;
	}

	bool EventFired(InstEventType Event) {
		return bEventsFired[Event];
	}

	void CopyCurrentEventsToPrev() {
		PrevHullEvent = HullEvent;
		PrevShieldBeamEvent = ShieldBeamEvent;
	}
};

// Per-instance, fixed data. This information is associated with an objectId-materialId
// and it's used to display damage textures. For instance, the normalized random location
// and scale of a damage texture is computed once and stored here for future use.
struct FixedInstanceData
{
	// Normalized (0..1) location of the damage texture
	float2 randLocNorm;
	// Normalized (0..1) scale of the damage texture
	float randScaleNorm;

	FixedInstanceData()
	{
		randLocNorm.x = (float)rand() / RAND_MAX;
		randLocNorm.y = (float)rand() / RAND_MAX;
		randScaleNorm = (float)rand() / RAND_MAX;
	}
};

// Used to store the information related to animated light maps that
// is loaded from .mat files:
typedef struct TexSeqElemStruct {
	int ExtraTextureIndex, GroupId, ImageId;
	char texname[MAX_TEX_SEQ_NAME];
	float seconds, intensity;
	bool IsDATImage;
	bool IsZIPImage;

	TexSeqElemStruct() {
		ExtraTextureIndex = -1;
		texname[0] = 0;
		IsDATImage = false;
		IsZIPImage = false;
		GroupId = ImageId = -1;
		seconds = intensity = 0.0f;
	}
} TexSeqElem;

typedef struct AnimatedTexControlStruct {
	// Animated LightMaps:
	std::vector<TexSeqElemStruct> Sequence;
	bool SequenceLoaded;
	int AnimIdx; // This is the current index in the Sequence, it can increase monotonically, or it can be random.
	float TimeLeft; // Time left for the current index in the sequence.
	bool IsRandom, BlackToAlpha, NoLoop, AlphaIsBloomMask;
	float4 Tint;
	GameEvent Event; // Activate this animation according to the value set in this field, this is like a "back-pointer" to the event
	InstEventType InstEvent; // If isInstEvent is true, then this field tells us what type of event is controlled by this ATC
	bool isInstEvent;
	// If this ATC describes an Instance Event, then the following will
	// contain the objectId associated with this ATC.
	int objectId;
	uint32_t materialId;
	float2 Offset;
	float AspectRatio;
	int Clamp;
	uint32_t OverlayCtrl;
	float2 uvSrc0;
	float2 uvSrc1;
	float2 uvOffsetMin;
	float2 uvOffsetMax;
	float2 uvScaleMin;
	float2 uvScaleMax;
	uint8_t uvRandomLoc;
	bool uvRandomScale;
	
	AnimatedTexControlStruct() {
		Sequence.clear();
		SequenceLoaded = false;
		AnimIdx = 0;
		TimeLeft = 1.0f;
		IsRandom = false;
		BlackToAlpha = false;
		AlphaIsBloomMask = false;
		Tint.x = 1.0f;
		Tint.y = 1.0f;
		Tint.z = 1.0f;
		Event = EVT_NONE;
		InstEvent = IEVT_NONE;
		Offset.x = 0.0f;
		Offset.y = 0.0f;
		AspectRatio = 1.0f;
		Clamp = 0;
		NoLoop = false; // Animations loop by default
		// Events are global by default. When this flag is set in g_AnimatedMaterials, it
		// indicates it's a template for instance events and it should not be animated.
		isInstEvent = false;
		// If this ATC is an element of g_AnimatedInstMaterials, this field will be set to
		// the objectId associated with this material.
		objectId = -1;
		OverlayCtrl = 0x0;

		// The animated textures will completely cover the destination surface by default.
		uvSrc0.x      = uvSrc0.y      = 0;
		uvSrc1.x      = uvSrc1.y      = 1;
		uvOffsetMin.x = uvOffsetMin.y = -0.2f;
		uvOffsetMax.x = uvOffsetMax.y =  0.2f;
		uvScaleMin.x  = uvScaleMin.y  = 1;
		uvScaleMax.x  = uvScaleMax.y  = 1;
		uvRandomLoc   = 2; // Defaut: random placement dicated by uvOffsetMin/Max
		uvRandomScale = false;
	}

	void ResetAnimation();

	// Updates the timer/index on the current animated material. Only call this function
	// if the current material has an animation.
	void Animate();

	// Returns true if this event can be randomized by sending a random value to the shader.
	// For instance, shields down and beam effects can be easily randomized
	bool IsRandomizableOverlay();
	// Returns true if this event is a hull damage event.
	bool IsHullDamageEvent();

} AnimatedTexControl;

extern std::vector<AnimatedTexControl> g_AnimatedMaterials;
extern std::vector<AnimatedTexControl> g_AnimatedInstMaterials;

// Used to store the information related to greebles
typedef struct GreebleDataStruct {
	// Holds the DAT filename where the greeble data is stored.
	char GreebleTexName[MAX_GREEBLE_LEVELS][MAX_GREEBLE_NAME];
	char GreebleLightMapName[MAX_GREEBLE_LEVELS][MAX_GREEBLE_NAME];
	// The following are indices into resources->_extraTextures. If greeble
	// textures are loaded, these indices will be greater than -1
	int GreebleTexIndex[MAX_GREEBLE_LEVELS];
	int GreebleLightMapIndex[MAX_GREEBLE_LEVELS];
	
	float GreebleDist[MAX_GREEBLE_LEVELS];
	float GreebleLightMapDist[MAX_GREEBLE_LEVELS];
	
	GreebleBlendMode greebleBlendMode[MAX_GREEBLE_LEVELS];
	GreebleBlendMode greebleLightMapBlendMode[MAX_GREEBLE_LEVELS];
	
	float GreebleMix[MAX_GREEBLE_LEVELS];
	float GreebleLightMapMix[MAX_GREEBLE_LEVELS];
	
	float GreebleScale[MAX_GREEBLE_LEVELS];
	float GreebleLightMapScale[MAX_GREEBLE_LEVELS];

	float2 UVDispMapResolution;

	GreebleDataStruct() {
		UVDispMapResolution.x = 1.0;
		UVDispMapResolution.y = 1.0;

		for (int i = 0; i < MAX_GREEBLE_LEVELS; i++) {
			GreebleTexName[i][0] = 0;
			GreebleTexIndex[i] = -1;
			GreebleLightMapName[i][0] = 0;
			GreebleLightMapIndex[i] = -1;
			GreebleDist[i] = 500.0f / (i + 1.0f);
			GreebleLightMapDist[i] = 500.0f / (i + 1.0f);
			greebleBlendMode[i] = GBM_MULTIPLY;
			greebleLightMapBlendMode[i] = GBM_SCREEN;
			GreebleMix[i] = 0.9f;
			GreebleLightMapMix[i] = 0.9f;
			GreebleScale[i] = 1.0f;
			GreebleLightMapScale[i] = 1.0f;
		}
	}
} GreebleData;

// Used to animate sections of an OPT
class MeshTransform {
public:
	Vector3 Center;
	float CurRotX, CurRotY, CurRotZ;
	float RotXDelta, RotYDelta, RotZDelta;
	bool bDoTransform;

	MeshTransform()
	{
		CurRotX = CurRotY = CurRotZ = 0.0f;
		RotXDelta = RotYDelta = RotZDelta = 0.0f;
		bDoTransform = false;
	}

	Matrix4 ComputeTransform();
	void UpdateTransform();
};

uint32_t GenerateUniqueMaterialId();
uint64_t InstEventIdFromObjectMaterialId(int objectId, uint32_t materialId);
void InstEventIdToObjectMatId(uint64_t instId, int *objectId, uint32_t *materialId);

// (ATCIndex, EventType)
typedef std::pair<int, int> ATCIndexEvtType;

// Materials
struct Material {
	uint32_t Id;
	int craftIdx;
	bool bIsDefaultMaterial;

	float Metallic;
	float Intensity;
	float Glossiness;
	float NMIntensity;
	float SpecValue;
	bool  IsShadeless;
	bool  NoBloom;
	Vector3 Light;
	Vector2 LightUVCoordPos;
	float LightFalloff;
	float LightAngle;
	bool IsLightEmitter;
	bool  IsLava;
	float LavaSpeed;
	float LavaSize;
	float EffectBloom;
	Vector3 LavaColor;
	bool LavaTiling;
	bool AlphaToBloom;
	bool NoColorAlpha; // When set, forces the alpha of the color output to 0
	bool AlphaIsntGlass; // When set, semi-transparent areas aren't translated to a Glass material
	float Ambient;

	int TotalFrames; // Used for animated DAT files, like the explosions

	float ExplosionScale;
	float ExplosionSpeed;
	int ExplosionBlendMode;
	// Holds indices into g_AnimatedMaterials for alternate explosion animations
	int AltExplosionIdx[MAX_ALT_EXPLOSIONS];
	int DS2ExplosionIdx;

	// Set to false by default. Should be set to true once the GroupId 
	// and ImageId have been parsed:
	bool DATGroupImageIdParsed;
	int GroupId;
	int ImageId;

	// Global ATC event pointers
	int TextureATCIndices[MAX_ATC_TYPES][MAX_GAME_EVT];
	// Instance ATC event pointers, they point to template entries in g_AnimatedMaterials.
	// It may seem redundant to have InstTextureATCIndices and in InstanceEvent too. But
	// these indices are for material-level templates (i.e. they are global) and the
	// indices in InstanceEvent are, well, copies of these indices and they are particular
	// to each instance. This allows one ATC per material per instance so that each instance
	// can have its own individual timer (that way, animations aren't globally synchronized).
	// We also need these here when loading the material files because there are no instances
	// yet anyway and to load the textures themselves in Direct3DTexture.
	// We use std::vector<int> here to allow multiple versions of the same event, but when
	// making an instance of these events, we randomly select one.
	std::vector<int> InstTextureATCIndices[MAX_ATC_TYPES][MAX_INST_EVT];

	int GreebleDataIdx;

	DiegeticMeshType DiegeticMesh;
	// If DiegeticMesh == DM_JOYSTICK then the following should be populated:
	Vector3 JoystickRoot;
	float JoystickMaxYaw;
	float JoystickMaxPitch;

	// If DiegeticMesh == DM_THR_ROT_*, then the following should be populated:
	Vector3 ThrottleRoot;
	float ThrottleMinAngle;
	float ThrottleMaxAngle;

	// If DiegeticMesh == DM_THR_TRANS, then the following should be populated
	Vector3 ThrottleStart;
	Vector3 ThrottleEnd;

	// Rotation matrix helper around an arbitrary axis. This matrix will align
	// ThrottleEnd - ThrottleStart with the Z+ axis
	Matrix4 RotAxisToZPlus;
	// If this flag is set, then RotAxisToZPlus has been computed and cached
	bool bRotAxisToZPlusReady;

	MeshTransform meshTransform;

	char NormalMapName[MAX_NORMALMAP_NAME];
	bool NormalMapLoaded;

	// Set to true if this material has at least one property that is activated on
	// a per-instance basis.
	bool bInstanceMaterial;
	bool SkipWhenDisabled;
	int DisplayIfSpeedGE;
	float DisplayIfThrottleGE;
	int DisplayIfMissionSetSpeedGE;
	int IncreaseBrightnessWithMissionSetSpeed;
	bool Raytrace;

	// DEBUG properties, remove later
	//Vector3 LavaNormalMult;
	//Vector3 LavaPosMult;
	//bool LavaTranspose;

	Material() {
		Id = 0;
		craftIdx = -1;
		bIsDefaultMaterial = false;
		Metallic = DEFAULT_METALLIC;
		Intensity = DEFAULT_SPEC_INT;
		Glossiness = DEFAULT_GLOSSINESS;
		NMIntensity = DEFAULT_NM_INT;
		SpecValue = DEFAULT_SPEC_VALUE;
		IsShadeless = false;
		Light = Vector3(0.0f, 0.0f, 0.0f);
		LightUVCoordPos = Vector2(0.1f, 0.5f);
		LightFalloff = 0.0f;
		LightAngle = 0.0f;
		IsLightEmitter = false;
		NoBloom = false;
		IsLava = false;
		LavaSpeed = 1.0f;
		LavaSize = 1.0f;
		EffectBloom = 1.0f;
		LavaTiling = true;

		LavaColor.x = 1.00f;
		LavaColor.y = 0.35f;
		LavaColor.z = 0.05f;

		AlphaToBloom = false;
		NoColorAlpha = false;
		AlphaIsntGlass = false;
		Ambient = 0.0f;

		TotalFrames	= 0;
		ExplosionScale = 2.0f; // 2.0f is the original scale
		ExplosionSpeed = 0.001f;
		// MechDonald's explosions are so good that we probably want to default to using them
		// now (instead of blending them with a procedural explosion)
		ExplosionBlendMode = 0; // 0: Original texture, 1: Blend with procedural explosion, 2: Use procedural explosions only

		DATGroupImageIdParsed = false;
		GroupId = 0;
		ImageId = 0;

		ClearATCIndices();

		GreebleDataIdx = -1;

		for (int i = 0; i < MAX_ALT_EXPLOSIONS; i++)
			AltExplosionIdx[i] = -1;
		DS2ExplosionIdx = -1;

		DiegeticMesh = DM_NONE;
		JoystickRoot = Vector3(0, 0, 0);
		JoystickMaxYaw = 10.0f;
		JoystickMaxPitch = -10.0f;
		RotAxisToZPlus.identity();
		bRotAxisToZPlusReady = false;

		NormalMapName[0] = 0;
		NormalMapLoaded = false;

		bInstanceMaterial = false;
		SkipWhenDisabled = false;
		DisplayIfSpeedGE = -INT_MAX;
		DisplayIfThrottleGE = -FLT_MAX;
		DisplayIfMissionSetSpeedGE = -INT_MAX;
		IncreaseBrightnessWithMissionSetSpeed = -1;

		Raytrace = true;

		/*
		// DEBUG properties, remove later
		LavaNormalMult.x = 1.0f;
		LavaNormalMult.y = 1.0f;
		LavaNormalMult.z = 1.0f;

		LavaPosMult.x = -1.0f;
		LavaPosMult.y = -1.0f;
		LavaPosMult.z = -1.0f;
		LavaTranspose = true;
		*/
	}

	void ClearATCIndices() {
		for (int j = 0; j < MAX_ATC_TYPES; j++)
			for (int i = 0; i < MAX_GAME_EVT; i++)
				TextureATCIndices[j][i] = -1;

		for (int j = 0; j < MAX_ATC_TYPES; j++)
			for (int i = 0; i < MAX_INST_EVT; i++)
				InstTextureATCIndices[j][i].clear();
	}

	// Returns true if any of the possible texture indices is enabled for a global event
	inline bool AnyTextureATCIndex() {
		for (int i = 0; i < MAX_GAME_EVT; i++)
			if (TextureATCIndices[TEXTURE_ATC_IDX][i] > -1)
				return true;
		return false;
	}

	// Returns true if any of the possible lightmap indices is enabled for a global event
	inline bool AnyLightMapATCIndex() {
		for (int i = 0; i < MAX_GAME_EVT; i++)
			if (TextureATCIndices[LIGHTMAP_ATC_IDX][i] > -1)
				return true;
		return false;
	}

	// Returns true if any of the possible texture indices is enabled for an instance event
	inline bool AnyInstTextureATCIndex() {
		for (int i = 0; i < MAX_INST_EVT; i++)
			if (InstTextureATCIndices[TEXTURE_ATC_IDX][i].size() > 0)
				return true;
		return false;
	}

	// Returns true if any of the possible lightmap indices is enabled for an instance event
	inline bool AnyInstLightMapATCIndex() {
		for (int i = 0; i < MAX_INST_EVT; i++)
			if (InstTextureATCIndices[LIGHTMAP_ATC_IDX][i].size() > 0)
				return true;
		return false;
	}

	int GetCurrentATCIndex(bool *bIsDamageTex, int ATCType = TEXTURE_ATC_IDX) {
		int index = TextureATCIndices[ATCType][EVT_NONE]; // Default index, this is what we'll play if EVT_NONE is set
		if (bIsDamageTex != NULL) *bIsDamageTex = false;

		// Overrides: these indices are only selected if specific events are set
		// Most-specific events first... least-specific events later.

		// Warning Light Events (this group event is independent from the others above)
		if (g_GameEvent.WLightLLEvent && TextureATCIndices[ATCType][WLIGHT_EVT_LEFT] > -1)
			return TextureATCIndices[ATCType][WLIGHT_EVT_LEFT];
		if (g_GameEvent.WLightMLEvent && TextureATCIndices[ATCType][WLIGHT_EVT_MID_LEFT] > -1)
			return TextureATCIndices[ATCType][WLIGHT_EVT_MID_LEFT];
		if (g_GameEvent.WLightMREvent && TextureATCIndices[ATCType][WLIGHT_EVT_MID_RIGHT] > -1)
			return TextureATCIndices[ATCType][WLIGHT_EVT_MID_RIGHT];
		if (g_GameEvent.WLightRREvent == 1 && TextureATCIndices[ATCType][WLIGHT_EVT_RIGHT_YELLOW] > -1)
			return TextureATCIndices[ATCType][WLIGHT_EVT_RIGHT_YELLOW];
		if (g_GameEvent.WLightRREvent == 2 && TextureATCIndices[ATCType][WLIGHT_EVT_RIGHT_RED] > -1)
			return TextureATCIndices[ATCType][WLIGHT_EVT_RIGHT_RED];

		// Cannon Ready Events
		for (int i = 0; i < MAX_CANNONS; i++)
			if (g_GameEvent.CannonReady[i] && TextureATCIndices[ATCType][CANNON_EVT_1_READY + i] > -1)
				return TextureATCIndices[ATCType][CANNON_EVT_1_READY + i];

		// Cockpit Damage Events
		if (!g_GameEvent.CockpitInstruments.CMD && TextureATCIndices[ATCType][CPT_EVT_BROKEN_CMD] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_CMD];
		}
		
		if (!g_GameEvent.CockpitInstruments.LaserIon && TextureATCIndices[ATCType][CPT_EVT_BROKEN_LASER_ION] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_LASER_ION];
		}

		// Some ships may not be equipped with a beam weapon, so we need to check the initial state too
		if (g_GameEvent.InitialCockpitInstruments.BeamWeapon && !g_GameEvent.CockpitInstruments.BeamWeapon &&
			TextureATCIndices[ATCType][CPT_EVT_BROKEN_BEAM_WEAPON] > -1 &&
			g_GameEvent.InitialCockpitInstruments.BeamWeapon) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_BEAM_WEAPON];
		}

		// Some ships may not be equipped with shields, so we need to check the intial status too
		if (g_GameEvent.CockpitInstruments.Shields && !g_GameEvent.CockpitInstruments.Shields &&
			TextureATCIndices[ATCType][CPT_EVT_BROKEN_SHIELDS] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_SHIELDS];
		}

		if (!g_GameEvent.CockpitInstruments.Throttle && TextureATCIndices[ATCType][CPT_EVT_BROKEN_THROTTLE] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_THROTTLE];
		}

		if (!g_GameEvent.CockpitInstruments.Sensors && TextureATCIndices[ATCType][CPT_EVT_BROKEN_SENSORS] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_SENSORS];
		}

		if (!g_GameEvent.CockpitInstruments.LaserRecharge && TextureATCIndices[ATCType][CPT_EVT_BROKEN_LASER_RECHARGE] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_LASER_RECHARGE];
		}

		if (!g_GameEvent.CockpitInstruments.EnginePower && TextureATCIndices[ATCType][CPT_EVT_BROKEN_ENGINE_POWER] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_ENGINE_POWER];
		}

		// Some ships may not be equipped with shields, so we need to check the intial status too
		if (g_GameEvent.InitialCockpitInstruments.ShieldRecharge && !g_GameEvent.CockpitInstruments.ShieldRecharge &&
			TextureATCIndices[ATCType][CPT_EVT_BROKEN_SHIELD_RECHARGE] > -1) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_SHIELD_RECHARGE];
		}

		// Some ships may not be equipped with a beam weapon, so we need to check the initial state too
		if (g_GameEvent.InitialCockpitInstruments.BeamRecharge && !g_GameEvent.CockpitInstruments.BeamRecharge &&
			TextureATCIndices[ATCType][CPT_EVT_BROKEN_BEAM_RECHARGE] > -1 &&
			g_GameEvent.InitialCockpitInstruments.BeamRecharge) {
			if (bIsDamageTex != NULL) *bIsDamageTex = true;
			return TextureATCIndices[ATCType][CPT_EVT_BROKEN_BEAM_RECHARGE];
		}

		// Hull Damage Events
		if (g_GameEvent.HullEvent == HULL_EVT_DAMAGE_25 && TextureATCIndices[ATCType][HULL_EVT_DAMAGE_25] > -1)
			return TextureATCIndices[ATCType][HULL_EVT_DAMAGE_25];

		if (g_GameEvent.HullEvent == HULL_EVT_DAMAGE_50 && TextureATCIndices[ATCType][HULL_EVT_DAMAGE_50] > -1)
			return TextureATCIndices[ATCType][HULL_EVT_DAMAGE_50];

		if (g_GameEvent.HullEvent == HULL_EVT_DAMAGE_75 && TextureATCIndices[ATCType][HULL_EVT_DAMAGE_75] > -1)
			return TextureATCIndices[ATCType][HULL_EVT_DAMAGE_75];

		// Target Events
		if (g_GameEvent.TargetEvent == TGT_EVT_LASER_LOCK && TextureATCIndices[ATCType][TGT_EVT_LASER_LOCK] > -1)
			return TextureATCIndices[ATCType][TGT_EVT_LASER_LOCK];
		
		if (g_GameEvent.TargetEvent == TGT_EVT_WARHEAD_LOCKING && TextureATCIndices[ATCType][TGT_EVT_WARHEAD_LOCKING] > -1)
			return TextureATCIndices[ATCType][TGT_EVT_WARHEAD_LOCKING];

		if (g_GameEvent.TargetEvent == TGT_EVT_WARHEAD_LOCKED && TextureATCIndices[ATCType][TGT_EVT_WARHEAD_LOCKED] > -1)
			return TextureATCIndices[ATCType][TGT_EVT_WARHEAD_LOCKED];

		// If we're parked in the hangar and this material belongs to the CMD, then we don't want to animate it. The reason
		// is that in the PPG the CMD will display data even when nothing is targeted and we don't want to override it with
		// an animation. Right now, we detect if the current material belong to a CMD by looking at other target-related events.
		// This is reasonable. If cockpit authors only specify EVT_NONE for the CMD, then it will never display the CMD contents.
		// They must also specify at least EVT_SELECTED so that the animation is removed once something is targeted.
		if (*g_playerInHangar && (
			TextureATCIndices[ATCType][TGT_EVT_SELECTED] > -1 ||
			TextureATCIndices[ATCType][TGT_EVT_LASER_LOCK] > -1 ||
			TextureATCIndices[ATCType][TGT_EVT_WARHEAD_LOCKING] > -1 ||
			TextureATCIndices[ATCType][TGT_EVT_WARHEAD_LOCKED] > -1))
		{
			return -1;
		}

		// Least-specific events later.
		// This event is for the TGT_EVT_SELECTED which should be enabled if there's a laser or
		// warhead lock. That's why the condition is placed at the end, after we've checked all
		// the previous events, and we compare against EVT_NONE:
		if (g_GameEvent.TargetEvent != EVT_NONE && TextureATCIndices[ATCType][TGT_EVT_SELECTED] > -1)
			return TextureATCIndices[ATCType][TGT_EVT_SELECTED];

		// Explosion variants cannot be triggered by events. Instead, we need to do a frame-by-frame replacement.
		//if (g_GameEvent.ExplosionEvent != EVT_NONE && TextureATCIndices[ATCType][EVT_DS2] > -1)
		//	TextureATCIndices[ATCType][EVT_DS2];
	
		return index;
	}

	std::vector<ATCIndexEvtType> GetCurrentInstATCIndex(const int objectId, InstanceEvent &instEvent, int ATCType=TEXTURE_ATC_IDX) {
		std::vector<ATCIndexEvtType> indices;

		if (objectId == -1) {
			log_debug("[DBG] [INST] ERROR: objectId == -1!");
			return indices;
		}

		// Lazy instancing of templates in g_AnimatedMaterials: we only make instances when
		// requesting an ATC index for an animation that is about to be displayed.
		if (!instEvent.bATCHasBeenInstanced) {
			instEvent.objectId = objectId;
			int rand_group = 0;
			// Create a copy of each template in g_AnimatedMaterials and put it into g_AnimatedInstMaterials
			for (int j = 0; j < MAX_ATC_TYPES; j++)
				for (int i = 0; i < MAX_INST_EVT; i++) {
					int size = InstTextureATCIndices[j][i].size();
					if (size <= 0)
						continue;
					
					// The randomly-selected entry from the list
					int src_idx = 0;

					// Hull damage events are grouped. If we select a random hull event, we'll
					// just display randomly increasing damage. But if hull events are grouped
					// we can add damage to previous textures and show consistent damage. To
					// achieve this we select a random group for the first hull event, and then
					// we choose textures from the same group for the other hull events.
					if (IEVT_HULL_DAMAGE_75 <= i && i <= IEVT_HULL_DAMAGE_25)
					{
						// If this is the first hull event, select a random group. All subsequent hull events will
						// now use this group.
						if (i == IEVT_HULL_DAMAGE_75) {
							rand_group = rand() % size;
							//log_debug("[DBG] [INST] objectId: %d, size: %d, rand_group selected: %d, evt: %s",
							//	objectId, size, rand_group, g_sInstEventNames[i]);
						}

						// If the random group is out of bounds for this event, select a new random slot
						if (rand_group >= size) {
							//log_debug("[DBG] [INST] objectId: %d, IGNORING previous rand_group: %d, evt: %s",
							//	objectId, rand_group, g_sInstEventNames[i]);
							src_idx = InstTextureATCIndices[j][i][rand() % size];
						}
						else {
							//log_debug("[DBG] [INST] objectId: %d, USING previous rand_group: %d, evt: %s",
							//	objectId, rand_group, g_sInstEventNames[i]);
							// Select a slot from the group that was randomly chosen previously.
							src_idx = InstTextureATCIndices[j][i][rand_group];
						}
					} else {
						int atc_idx = rand() % size;
						src_idx = InstTextureATCIndices[j][i][atc_idx];
						//log_debug("[DBG] [INST] global InstTextureATCIndices[j][%s].size(): %d, random idx: %d",
						//	g_sInstEventNames[i], size, atc_idx);

						if (src_idx == -1) {
							log_debug("[DBG] [INST] WARN: Template %d, event %s is nonempty, but has src_idx == -1",
								i, g_sInstEventNames[i]);
							continue;
						}
					}

					AnimatedTexControl atc = g_AnimatedMaterials[src_idx];
					atc.objectId = objectId;
					atc.materialId = this->Id;
					g_AnimatedInstMaterials.push_back(atc);
					const int new_slot = g_AnimatedInstMaterials.size() - 1;
					if (instEvent.InstTextureATCIndices[j][i] != -1)
						log_debug("[DBG] [INST] WARN: Overwritting object-matId %d-%d, instEvent.InstTextureATCIndices[j][%d] <-- %d. Prev value: %d",
							objectId, this->Id, g_sInstEventNames[i], new_slot, instEvent.InstTextureATCIndices[j][i]);
					instEvent.InstTextureATCIndices[j][i] = new_slot;
					//log_debug("[DBG] [INST] ATC object-matId: %d-%d instanced. instEvent.InstTextureATCIndices[j][%s] = g_AnimatedMaterials[%d] = %d",
					//	objectId, this->Id, g_sInstEventNames[i], src_idx, new_slot);
				}
			instEvent.bATCHasBeenInstanced = true;
		}

		// Overrides: these indices are only selected if specific events are set
		// Most-specific events first... least-specific events later.

		// Shield and Beam events are mutually exclusive: we can only display one of these
		// because they all use the SCREEN blending mode.
		// Beam Events
		if (instEvent.ShieldBeamEvent == IEVT_TRACTOR_BEAM && instEvent.InstTextureATCIndices[ATCType][IEVT_TRACTOR_BEAM] > -1)
			indices.push_back(std::make_pair(instEvent.InstTextureATCIndices[ATCType][IEVT_TRACTOR_BEAM], IEVT_TRACTOR_BEAM));
		else
		if (instEvent.ShieldBeamEvent == IEVT_JAMMING_BEAM && instEvent.InstTextureATCIndices[ATCType][IEVT_JAMMING_BEAM] > -1)
			indices.push_back(std::make_pair(instEvent.InstTextureATCIndices[ATCType][IEVT_JAMMING_BEAM], IEVT_JAMMING_BEAM));
		else
		// Shield Events
		if (instEvent.ShieldBeamEvent == IEVT_SHIELDS_DOWN && instEvent.InstTextureATCIndices[ATCType][IEVT_SHIELDS_DOWN] > -1)
			indices.push_back(std::make_pair(instEvent.InstTextureATCIndices[ATCType][IEVT_SHIELDS_DOWN], IEVT_SHIELDS_DOWN));

		// Hull Damage Events
		if (instEvent.HullEvent == IEVT_HULL_DAMAGE_25 && instEvent.InstTextureATCIndices[ATCType][IEVT_HULL_DAMAGE_25] > -1)
			indices.push_back(std::make_pair(instEvent.InstTextureATCIndices[ATCType][IEVT_HULL_DAMAGE_25], IEVT_HULL_DAMAGE_25));
		else
		if (instEvent.HullEvent == IEVT_HULL_DAMAGE_50 && instEvent.InstTextureATCIndices[ATCType][IEVT_HULL_DAMAGE_50] > -1)
			indices.push_back(std::make_pair(instEvent.InstTextureATCIndices[ATCType][IEVT_HULL_DAMAGE_50], IEVT_HULL_DAMAGE_50));
		else
		if (instEvent.HullEvent == IEVT_HULL_DAMAGE_75 && instEvent.InstTextureATCIndices[ATCType][IEVT_HULL_DAMAGE_75] > -1)
			indices.push_back(std::make_pair(instEvent.InstTextureATCIndices[ATCType][IEVT_HULL_DAMAGE_75], IEVT_HULL_DAMAGE_75));

		// Add more events here... remember that least-specific events come later

		return indices;
	}
};

/*
 Individual entry in the craft material definition file (*.mat). Maintains a copy
 of one entry of the form:

 [TEX000##]
 Metallic   = X
 Reflection = Y
 Glossiness = Z

*/
struct MaterialTexDef {
	Material material;
	char texname[MAX_TEXNAME];
} ;

/*
 Contains all the entries from a *.mat file for a single craft, along with the
 OPT name for this craft
*/
struct CraftMaterials {
	std::vector<MaterialTexDef> MaterialList;
	char OPTname[MAX_OPT_NAME];
};

typedef struct OPTNameStruct {
	char name[MAX_OPT_NAME];
} OPTNameType;

typedef struct TexnameStruct {
	char name[MAX_TEXNAME];
} TexnameType;

/*
 Contains all the materials for all the OPTs currently loaded
*/
extern std::vector<CraftMaterials> g_Materials;
// List of all materials with animated textures. OPTs may re-use the same
// texture several times in different areas. If this texture has an animation,
// then the animation will be rendered multiple times on the screen. In order
// to update the timer on these animations exactly *once* per frame, we need a
// way to iterate over them at the end of the frame to update their timers.
// This list is used to update the timers on animated materials once per frame.
extern std::vector<AnimatedTexControl> g_AnimatedMaterials;
// Same as g_AnimatedMaterials, but used to hold instance materials.
extern std::vector<AnimatedTexControl> g_AnimatedInstMaterials;
// List of all the OPTs seen so far
extern std::vector<OPTNameType> g_OPTnames;
// Global Greeble Data (mask, textures, blending modes)
extern std::vector<GreebleData> g_GreebleData;

extern bool g_bReloadMaterialsEnabled;
extern Material g_DefaultGlobalMaterial;

void InitOPTnames();
void ClearOPTnames();
void InitCraftMaterials();
void ClearCraftMaterials();

void OPTNameToMATParamsFile(char* OPTName, char* sFileName, int iFileNameSize);
void DATNameToMATParamsFile(char *DATName, char *sFileName, char *sFileNameShort, int iFileNameSize);
bool LoadIndividualMATParams(char *OPTname, char *sFileName, bool verbose = true);
void ReadMaterialLine(char* buf, Material* curMaterial, char *OPTname);
bool GetGroupIdImageIdFromDATName(char* DATName, int* GroupId, int* ImageId);
void InitOPTnames();
void ClearOPTnames();
void InitCraftMaterials();
void ClearCraftMaterials();
int FindCraftMaterial(char* OPTname);
Material FindMaterial(int CraftIndex, char* TexName, bool debug = false);
// Iterate over all the g_AnimatedMaterials and update their timers
void AnimateMaterials();
// Clears g_AnimatedMaterials
void ClearAnimatedMaterials();

bool ParseDatZipFileNameGroupIdImageId(char *buf, char *sDATFileNameOut, int sDATFileNameSize, short *GroupId, short *ImageId);

void ResetGameEvent();
void UpdateEventsFired();
bool EventFired(GameEvent Event);