#pragma once

#include <windows.h>
#include "XWATypes.h"

#pragma pack(push, 1)

struct MobileObjectEntry;
struct ProjectileInstance;
struct RotationMatrix3D;

struct CraftInstanceHardpoint {
	WORD ProjectileType;
	BYTE WeaponType; // 0 no weapon, 1 lasers, 2 ion, 3 warheads (any type of warheads)
	BYTE LaserIndex;
	BYTE Energy;
	BYTE Count; // Number of warheads
	WORD TurretCooldown;
	BYTE MeshID;
	BYTE HardpointID;
	WORD TurretTargetIndex;
	WORD TurretTargetSearchCooldown;
};
static_assert(sizeof(CraftInstanceHardpoint) == 0xE, "size of CraftInstance must be 0xE");

enum CraftState {
	Craftstate_Normal = 0,
	Craftstate_Inactive = 1,
	Craftstate_NoSpeed = 2,
	Craftstate_Dying = 3,
	Craftstate_DiedInstantly = 4,
	Craftstate_HyperingOut = 5,
	Craftstate_HyperingIn = 6,
	Craftstate_Despawned = 7,
	Craftstate_UNK8 = 8,
	Craftstate_NoRotation = 9,
};

/*
00000001 ; enum CockpitInstruments, mappedto_218, bitfield
00000001 CockpitInstrument_Targeting  = 1
00000002 CockpitInstrument_LeftCannon  = 2
00000004 CockpitInstrument_RightCannon  = 4
00000008 CockpitInstrument_Warhead  = 8
00000010 CockpitInstrument_BeamEnergy  = 10h
00000020 CockpitInstrument_Shields  = 20h
00000040 CockpitInstrument_Throttle  = 40h
00000080 CockpitInstrument_LeftSensor  = 80h
00000100 CockpitInstrument_RightSensor  = 100h
00000200 CockpitInstrument_CannonConfig  = 200h
00000400 CockpitInstrument_EngineConfig  = 400h
00000800 CockpitInstrument_ShieldConfig  = 800h
00001000 CockpitInstrument_BeamConfig  = 1000h
*/

struct CraftInstance {
	// Craft   struc ; (sizeof=0x3F9, mappedto_209)
	/* 0x00 */ DWORD NumberInFG;
	/* 0x04 */ WORD  CraftType;  // enum CraftTypeEnum, see CraftDefinitionEntry
	/* 0x06 */ DWORD LeaderCraftIndex;
	/* 0x0A */ BYTE  VirtualCraftPointerSet;
	/* 0x0B */ BYTE  CraftState; // enum CraftState. See above
	/* 0x0C */ BYTE  RemovedFromPlay;
	/* 0x0D */ WORD  AiSkill;
	/* 0x0F */ WORD  field_F;
	/* 0x11 */ WORD  unknown2;
	/* 0x13 */ DWORD IsUnderBeamEffect[5];
	// Index 1: Tractor Beam (value goes from 0 to 0xFFFF when activated)
	// Index 2: Jamming Beam (value goes from 0 to 0xFFFF when activated)
	BYTE SfoilsState; // Ofs 0x27
	BYTE XwaAIData_m00;
	BYTE meshStates[20]; // Ofs 0x29
	BYTE waypointLoopsCompleted[20]; // 0x3D
	BYTE XwaAIData_m29;
	BYTE currentFlightPln;
	BYTE startingFlightPlan;
	BYTE currentTargetWaypoint;
	BYTE savedAwaitingOrder;
	DWORD CycleTime;
	DWORD CycleTimer; // Ofs 0x5A
	WORD RandomSeed;
	WORD TargetObjectIndex;
	WORD TargetObjectTagID;
	WORD XwaAIData_m3C;
	BYTE XwaAIData_m3E;
	DWORD TargetWaypointPositionX; // Ofs 0x67
	DWORD TargetWaypointPositionY;
	DWORD TargetWaypointPositionZ;
	DWORD angleDistanceToWaypoint;
	DWORD XwaAIData_m4F;
	WORD playerAttackOrderTargetIndex;
	BYTE XwaAIData_m55; // Ofs 0x7D
	WORD DesiredPitch;
	WORD DesiredRoll;
	WORD DesiredYaw;
	BYTE currentManr;
	BYTE specialTimerOrState;
	DWORD TimerToAttack;
	DWORD TimerToFlyWP;
	WORD PickedUpObjectIndex;
	WORD field_90;
	WORD field_92;
	WORD field_94;
	WORD field_96;
	WORD field_98;
	WORD field_9A;
	WORD ThreatenedByIndex; // Ofs 0x9C
	WORD field_9E;
	WORD field_A0;
	WORD field_A2;
	BYTE field_A4;
	BYTE field_A5;
	BYTE field_A6;
	BYTE field_A7;
	BYTE field_A8;
	BYTE field_A9;
	BYTE field_AA;
	BYTE field_AB;
	BYTE DoneBoardedCount;
	BYTE BeingBoarded;
	BYTE DoneDockingCount;
	BYTE IsDocking;
	WORD ObjectTagID2;
	WORD field_B2; // Ofs 0xB2
	DWORD field_B4;
	DWORD field_B8;
	DWORD field_BC;
	DWORD field_C0;
	WORD TopSpeedMPH;
	WORD SpeedPercentage;
	BYTE RotationRelatedUnk1;
	BYTE RotationRelatedUnk2;
	WORD PitchRate;
	WORD PitchPercentage;
	BYTE PitchState;
	BYTE RotationRelatedUnk3;
	WORD PitchMaxPercentage;
	WORD RollRate;
	WORD RollPercentage;
	BYTE RollState;
	WORD RollMaxPercentage;
	WORD YawRate; // Ofs 0xD9
	WORD YawPercentage;
	BYTE YawState;
	WORD YawMaxPercentage;
	BYTE FormationType;
	BYTE FormationSpacing;
	BYTE SpecialCraft;
	BYTE field_E3;
	DWORD DriftXPos;
	DWORD DriftYPos;
	DWORD DriftZPos;
	WORD EngineThrottleInput; // Ofs 0xF0 <-- This is the throttle, 0xFFFF --> 100% throttle, divide by 65535.0f
	WORD PercentOfActiveEngines;
	WORD MissionSetSpeed;
	WORD EngineSLAMInput;
	DWORD HullDamageReceived;
	DWORD CriticalDamageThreshold;
	DWORD HullStrength;
	WORD SystemStrength;
	DWORD field_106;
	BYTE field_10A;
	DWORD TotalDmgReceived;
	DWORD TotalDmgToPlayer;
	DWORD TotalDmgFromCollisions;
	DWORD TotalDmgFromTurrets;
	DWORD TotalDmgFromMines;
	/* 0x11F */ DWORD field_11F[8]; // Ofs 0x11F dd 8 dup(?)
	/* 0x13F */ BYTE  field_13F[8];
	/* 0x147 */ DWORD field_147[8];
	/* 0x167 */ DWORD field_167[6];
	/* 0x17F */ WORD InitialCockpitInstruments;
	/* 0x181 */ WORD CockpitInstrumentStatus;
	/* 0x183 */ WORD InitialSubsystems;
	/* 0x185 */ WORD SubsystemStatus;
	/* 0x187 */ WORD MagTimeRemaining;
	/* 0x188 */ BYTE field_189;
	/* 0x18A */ BYTE field_18A;
	/* 0x18B */ BYTE CapturedBy;
	/* 0x18C */ BYTE AttackedByTeam[10];
	// For each team, InspectedByTeam has the following:
	//   -1: (Default): Not inspected, unidentified
	//    0: Identified
	// >= 1: Identified and inspected
	/* 0x196 */ int8_t InspectedByTeam[10];
	/* 0x1A0 */ BYTE CargoIndex;
	/* 0x1A1 */ BYTE field_1A1;
	/* 0x1A2 */ DWORD ShieldPointsFront; // // Ofs 0x1A2 dd 2 dup(? )
	/* 0x1A6 */ DWORD ShieldPointsBack;
	BYTE ElsShields;
	BYTE ShieldDirection;
	BYTE NumberOfLaserSets;
	BYTE ElsLasers;
	BYTE NumberOfLasers;
	BYTE ConvergenceMode; // db ? ; enum LaserConvergenceMode
	WORD LaserType[3];
	BYTE LaserLinkStatus[3];
	BYTE field_1B9[3];
	BYTE LaserNextHardpoint[3];
	WORD LaserCooldown[3];
	DWORD LaserLastFireAttempt[3];
	BYTE NumWarheadLauncherGroups; // Ofs 0x1D1
	WORD WarheadType[2];
	BYTE WarheadNextHardpoint[2];
	WORD WarheadCooldown[2];
	WORD LockTime;
	BYTE BeamType;
	BYTE ElsBeam;
	WORD BeamEnergy;
	BYTE BeamActive;
	WORD BeamActive2;
	BYTE field_1E5; // Ofs 0x1E5
	BYTE field_1E6;
	BYTE CountermeasureType;
	BYTE CountermeasureAmount;
	WORD ChaffLifespan;
	WORD FlareLifespan;
	WORD PrimaryLasersFired;
	WORD PrimaryLasersHit;
	WORD SecondaryLasersFired;
	WORD SecondaryLasersHit;
	BYTE WarheadsFired;
	BYTE WarheadsHit;
	BYTE SysRepairPriority[11]; // Ofs 0x1F7 db 11 dup(? )
	WORD SysRepairPercent[11];  // Ofs 0x202
	WORD SysRepairTimeRemaining[11]; // Ofs 0x218
	BYTE MeshState[50]; // Ofs 0x22E
	BYTE MeshRotationAngles[50];
	BYTE MeshHP[50];
	WORD field_2C4; // Ofs 0x2C4       dw ?
	WORD PlayerIndex;
	WORD field_2C8;
	BYTE field_2CA;
	BYTE field_2CB; // enum PloEnum
	BYTE field_2CC; // enum PloEnum
	WORD field_2CD;
	BYTE EngineThrottles[16]; // Ofs 0x2CF db 16 dup(? )
	CraftInstanceHardpoint Hardpoints[16]; // Ofs 0x2DF CraftInstanceHardpoints 16 dup(? )
	WORD ObjectTagID; // Ofs 0x3BF     dw ?
	WORD RotGunAngles[2];
	WORD RotBeamAngles[2];
	DWORD field_3C9[2];
	DWORD field_3D1[2];
	char Cargo[16]; // Ofs 0x3D9 string(C)
	DWORD field_3E9;
	DWORD field_3ED;
	DWORD field_3F1;
	DWORD pObject; // offset
};

static_assert(sizeof(CraftInstance) == 0x3F9, "size of CraftInstance must be 0x3F9");

struct RotationMatrix3D
{
	__int16 Front_X;
	__int16 Front_Y;
	__int16 Front_Z;
	__int16 Right_X;
	__int16 Right_Y;
	__int16 Right_Z;
	__int16 Up_X;
	__int16 Up_Y;
	__int16 Up_Z;
};

struct ObjectEntry
{
	/* 0x00 */ __int16 objectID;
	/* 0x02 */ __int16 objectSpecies; // Type enum ObjectTypeEnum
	/* 0x04 */ char objectGenus;
	/* 0x05 */ char FGIndex;
	/* 0x06 */ char region;
	/* 0x07 */ int x; // (x,y,z) is the actual position of this craft. We can use this to move the player or pretty much any ship.
	/* 0x0B */ int y;
	/* 0x0F */ int z;
	/* 0x13 */ __int16 yaw;
	/* 0x15 */ __int16 pitch;
	/* 0x17 */ __int16 roll;
	/* 0x19 */ __int16 rotation;
	/* 0x1B */ __int16 field_1B;
	/* 0x1D */ __int16 field_1D;
	/* 0x1F */ int humanPlayerIndex;
	/* 0x23 */ MobileObjectEntry *MobileObjectPtr;
};

static_assert(sizeof(ObjectEntry) == 39, "size of ObjectEntry must be 39");

struct MobileObjectEntry
{
	BYTE objectFamily;
	char spriteDrawScale;
	DWORD spriteScale;
	DWORD lastUpdateTime;
	DWORD PosXDelta;
	DWORD PosYDelta;
	DWORD PosZDelta;
	char collisionChecksActive;
	int collisionCheckTimers[16];
	__int16 collisionCheckObjectIndex[16];
	int collisionCheckMasterCountdown;
	__int16 currentSpeedFraction;
	__int16 field_7D;
	char field_7F;
	char field_80;
	__int16 field_81;
	__int16 _impactSpinRate_;
	__int16 currentSpeed;
	__int16 speedRelease;
	int collisionDamage;
	__int16 lifespan;
	char field_8F;
	char field_90;
	__int16 timeAlive;
	__int16 motherObjectIndex;
	__int16 motherObjectSpecies;
	char IFF;
	char Team;
	char markingColor;
	__int16 ejectionObjects;
	int field_9C;
	char inCollision;
	__int16 field_A1;
	__int16 field_A3;
	__int16 field_A5;
	__int16 field_A7;
	__int16 field_A9;
	__int16 field_AB;
	char field_AD;
	char field_AE;
	char field_AF;
	char field_B0;
	char field_B1;
	char field_B2;
	char field_B3;
	char field_B4;
	char field_B5;
	char field_B6;
	char field_B7;
	char field_B8;
	char field_B9;
	char field_BA;
	char field_BB;
	char field_BC;
	char field_BD;
	char field_BE;
	char forwardVectorNeedsRecalculation;
	__int16 forwardX;
	__int16 forwardY;
	__int16 forwardZ;
	char transformNeedsRecalculation;
	RotationMatrix3D transformMatrix;
	ProjectileInstance *projectileInstancePTR;
	CraftInstance *craftInstancePtr;
	int unknownPTR;
};

static_assert(sizeof(MobileObjectEntry) == 229, "size of MobileObjectEntry must be 229");

struct PlayerCamera {
	int PositionX;
	int PositionY;
	int PositionZ;
	int CraftIndex;
	int RelatedToMap;
	__int16 CraftPitch;
	__int16 CraftYaw;
	__int16 CraftRoll;
	__int16 Roll;
	__int16 Pitch;
	__int16 Yaw;
	int ShakeX;
	int ShakeY;
	int ShakeZ;
	__int16 collisionPitch;
	__int16 collisionYaw;
	__int16 collisionRoll;
	__int16 field_32;
	char ViewMode1;
	char ViewMode2;
	char field_36;
	char S0x08B94E0_B48_m37;
	char unk38;
	__int16 S0x08B94E0_B48_m39;
	__int16 S0x08B94E0_B48_m3B;
	__int16 MapMode;
	__int16 _RelatedToCamera_;
	__int16 ExternalCamera;
	int ExternalCameraZoomDist;
	__int16 FlyByCameraTime;    // Starts at 0 when Alt-J is pressed and goes all the way to ~1175 before being reset to 0
	__int16 S0x08B94E0_B48_m49;
	char unk4B[2];
};

static_assert(sizeof(PlayerCamera) == 0x4D, "size of PlayerCamera must be 0x4D");


// The current target object index is stored as a word value at offset 0x25 in the player table. A value of 0xffff (-1) means that there is no target.
struct PlayerDataEntry
{
	int objectIndex;
	int jumpNextCraftID;
	__int16 playerRank;
	__int16 IFF;
	__int16 team;
	__int16 playerFG;
	char currentRegion;
	char participationState;
	char isEjecting;
	char autopilotAction;
	char field_14;
	char mapState;
	char autopilot;
	char hyperspacePhase;
	int _hyperspaceRelated_;
	char field_1C;
	char field_1D;
	char field_1E;
	int timeInHyperspace;
	char allowTargetBox;
	char warheadLockState;
	// This is the index of the currently-targeted object, a value of -1 means no target
	// Use this index on this table:
	// ObjectEntry* objects = (ObjectEntry **)0x7B33C4;
	__int16 currentTargetIndex; 
	char targetTimeTargetedSeconds;
	char targetTimeTargetedMinutes;
	__int16 lastTargetIndex;
	__int16 craftMemory1;
	__int16 craftMemory2;
	__int16 craftMemory3;
	__int16 craftMemory4;
	char primarySecondaryArmed; // 0 if primary weapon armed, 1 if secondary weapon (lasers or warheads) armed
	char warheadArmed; // 0 if warheads are not armed, 1 if any warhead group is armed
	// primarySec,   warHead,	Meaning:
	//		0			0		Lasers Armed
	//		1			0		Ion Cannons Armed
	//		0			1		Primary Warheads Armed
	//		1			1		Secondary Warheads Armed
	__int16 componentTargetIndex;
	__int16 field_37;
	__int16 engineWashCraftIndex;
	__int16 engineWashAmount;
	__int16 throttlePreset1;
	__int16 throttlePreset2;
	char elsPreset1Lasers;
	char elsPreset2Lasers;
	char elsPreset1Shields;
	char elsPreset2Shields;
	char elsPreset1Beam;
	char elsPreset2Beam;
	__int16 engineThrottleInput;
	char elsLasers;
	char elsShields;
	char elsBeam;
	char shieldDirection;
	char primaryLaserLinkStatus;
	int secondaryLinkStatus;
	char criticalMessageType;
	// When a hyperspace jump is initiated, this field contains
	// the destination region
	int16_t criticalMessageObjectIndex; // 0x53-0x54
	char field_55;
	char field_56;
	char rollHeld;
	char field_58;
	__int16 yawDrift;
	__int16 pitchDrift;
	__int16 rollDrift;
	char joystickTriggerFlags;
	char field_60;
	__int16 rollDelayTimer;
	char field_63;
	char hudActive1; // IsHUDVisible;
	char hudActive2;
	char simpleHUD;  // Alt+.: 1 when only the reticle is visible.
	char bottomLeftPanel;
	char bottomRightPanel;
	char activeWeapon;
	char field_6A;
	char consoleActivated;
	char panelSelected;
	char panelSelected2;
	char field_6E;
	char bottomLeftPanelScreenState;
	char field_70;
	char field_71;
	char bottomRightPanelScreenState;
	char field_73;
	char flightCommandSelected;
	char flightCommandSelection;
	char field_76;
	char field_77;
	char flightCommandNumWingmenSelectable;
	char flightCommandWingmanSelected;
	char field_7A;
	char flightCommandWingmenSelectable[6];
	char field_81;
	char field_82;
	char field_83;
	char field_84;
	char field_85;
	char field_86;
	char field_87;
	char field_88;
	char field_89;
	char field_8A;
	char field_8B;
	char field_8C;
	char field_8D;
	char field_8E;
	char field_8F;
	char field_90;
	char field_91;
	char field_92;
	char field_93;
	char field_94;
	char field_95;
	char field_96;
	char field_97;
	char field_98;
	int field_99;
	int field_9D;
	int field_A1;
	int field_A5;
	int field_A9;
	int field_AD;
	int field_B1;
	int field_B5;
	int field_B9;
	int field_BD;
	int field_C1;
	int field_C5;
	int field_C9;
	int field_CD;
	int field_D1;
	int field_D5;
	int field_D9;
	int field_DD;
	int field_E1;
	int field_E5;
	int field_E9;
	int field_ED;
	int field_F1;
	int field_F5;
	int field_F9;
	int field_FD;
	int field_101;
	int field_105;
	int field_109;
	int field_10D;
	int field_111;
	int field_115;
	int field_119;
	int field_11D;
	int field_121;
	int field_125;
	int field_129;
	int field_12D;
	int field_131;
	int field_135;
	int field_139;
	int field_13D;
	int field_141;
	int field_145;
	int field_149;
	int field_14D;
	int field_151;
	int field_155;
	int field_159;
	int field_15D;
	int field_161;
	char field_165;
	char field_166;
	char field_167;
	char field_168;
	char field_169;
	char field_16A;
	char field_16B;
	char field_16C;
	char field_16D;
	char field_16E;
	char field_16F;
	char field_170;
	char field_171;
	char field_172;
	char field_173;
	char field_174;
	char field_175;
	char field_176;
	char field_177;
	char field_178;
	char field_179;
	char field_17A;
	char field_17B;
	char field_17C;
	char field_17D;
	char field_17E;
	char field_17F;
	char field_180;
	char field_181;
	char field_182;
	char field_183;
	char field_184;
	char field_185;
	char field_186;
	char field_187;
	char field_188;
	char field_189;
	char field_18A;
	char field_18B;
	char field_18C;
	char field_18D;
	char field_18E;
	char field_18F;
	char field_190;
	char field_191;
	char field_192;
	char field_193;
	char field_194;
	char field_195;
	char field_196;
	char field_197;
	char field_198;
	char field_199;
	char field_19A;
	char field_19B;
	char field_19C;
	char field_19D;
	char field_19E;
	char field_19F;
	char field_1A0;
	char field_1A1;
	char field_1A2;
	char field_1A3;
	char field_1A4;
	char field_1A5;
	char field_1A6;
	char field_1A7;
	char field_1A8;
	char field_1A9;
	char field_1AA;
	char field_1AB;
	char field_1AC;
	char field_1AD;
	char field_1AE;
	char field_1AF;
	char field_1B0;
	char field_1B1;
	char field_1B2;
	char field_1B3;
	char field_1B4;
	char field_1B5;
	char field_1B6;
	char field_1B7;
	char field_1B8;
	char field_1B9;
	char field_1BA;
	char field_1BB;
	char field_1BC;
	char field_1BD;
	char field_1BE;
	char field_1BF;
	char field_1C0;
	char field_1C1;
	char field_1C2;
	char field_1C3;
	char field_1C4;
	char field_1C5;
	char field_1C6;
	char field_1C7;
	char field_1C8;
	char field_1C9;
	char field_1CA;
	char field_1CB;
	char field_1CC;
	char field_1CD;
	char field_1CE;
	char field_1CF;
	char field_1D0;
	char field_1D1;
	char field_1D2;
	char field_1D3;
	char field_1D4;
	char field_1D5;
	char field_1D6;
	char field_1D7;
	char field_1D8;
	char field_1D9;
	char field_1DA;
	char field_1DB;
	char field_1DC;
	char field_1DD;
	char field_1DE;
	char field_1DF;
	char field_1E0;
	char field_1E1;
	char field_1E2;
	char field_1E3;
	char field_1E4;
	char field_1E5;
	char field_1E6;
	char field_1E7;
	char field_1E8;
	char field_1E9;
	__int16 consoleCharacterCount;
	int aiObjectIndex;
	char field_1F0; // FlightCommandMainNumOfOptions
	char field_1F1;
	char field_1F2;
	char field_1F3;
	char field_1F4;
	char field_1F5;
	char field_1F6;
	char field_1F7;
	char field_1F8;
	char cockpitDisplayed;
	char cockpitDisplayed2; // HasCockpitOpt
	char field_1FB; // HasTurretOpt
	char field_1FC;
	__int16 MousePositionX;
	__int16 MousePositionY;
	float CockpitPositionTransformedX;
	float CockpitPositionTransformedY;
	float CockpitPositionTransformedZ;
	//float _Pitch_;
	//float _Yaw_;
	//float _Roll_; // Looks like these should be float's, according to Justagai.
	float CockpitPositionX;
	float CockpitPositionY;
	float CockpitPositionZ; // Looks like these should be float's, according to Justagai.
	__int16 gunnerTurretActive;
	__int16 numberOfGunnerHardpoints;
	char currentGunnerHardpointActive;
	short gunnerTurretF[3]; // offset 0x021e - 0x223
	short gunnerTurretR[3]; // offset 0x0224 - 0x229
	short gunnerTurretU[3]; // offset 0x022a - 0x22f
	int score;
	int promoPoints;
	int worsePromoPoints;
	int field_23C;
	char field_240;
	char field_241;
	char field_242;
	char field_243;
	char field_244;
	char field_245;
	char field_246;
	char field_247;
	char field_248;
	char field_249;
	char field_24A;
	char field_24B;
	__int16 energyWeapon1Fired;
	__int16 energyWeapon1Hits;
	__int16 energyWeapon2Fired;
	__int16 energyWeapon2Hits;
	__int16 WarheadsFired;
	__int16 WarheadHits;
	__int16 numOfCraftInspected;
	__int16 numOfSpecialCraftInspected;
	__int16 killsOnFG[192];
	char field_3DC[982];
	char field_7B2;
	char field_7B3;
	char field_7B4;
	char field_7B5;
	__int16 friendliesKilled;
	__int16 totalLosses;
	__int16 totalLossesByCollision;
	__int16 totalLossesByStarship;
	__int16 totalLossesByMine;
	char field_7C0[510];
	char field_9BE;
	char field_9BF;
	char field_9C0;
	char field_9C1;
	char field_9C2;
	char field_9C3;
	char field_9C4;
	char field_9C5;
	char field_9C6;
	char field_9C7;
	char field_9C8;
	char field_9C9;
	char field_9CA;
	char field_9CB;
	char field_9CC;
	char field_9CD;
	char field_9CE;
	char field_9CF;
	char field_9D0;
	char field_9D1;
	char field_9D2;
	char field_9D3;
	char field_9D4;
	char field_9D5;
	char field_9D6;
	char field_9D7;
	char field_9D8;
	char field_9D9;
	char field_9DA;
	char field_9DB;
	char field_9DC;
	char field_9DD;
	char field_9DE;
	char field_9DF;
	char field_9E0;
	char field_9E1;
	char field_9E2;
	char field_9E3;
	char field_9E4;
	char field_9E5;
	char field_9E6;
	char field_9E7;
	char field_9E8;
	char field_9E9;
	char field_9EA;
	char field_9EB;
	char field_9EC;
	char field_9ED;
	char field_9EE;
	char field_9EF;
	char field_9F0;
	char field_9F1;
	char field_9F2;
	char field_9F3;
	char field_9F4;
	char field_9F5;
	char field_9F6;
	char field_9F7;
	char field_9F8;
	char field_9F9;
	char field_9FA;
	char field_9FB;
	char field_9FC;
	char field_9FD;
	char field_9FE;
	char field_9FF;
	char field_A00;
	char field_A01;
	char field_A02;
	char field_A03;
	char field_A04;
	char field_A05;
	char field_A06;
	char field_A07;
	char field_A08;
	char field_A09;
	char field_A0A;
	char field_A0B;
	char field_A0C;
	char field_A0D;
	char field_A0E;
	char field_A0F;
	char field_A10;
	char field_A11;
	char field_A12;
	char field_A13;
	char field_A14;
	char field_A15;
	char field_A16;
	char field_A17;
	char field_A18;
	char field_A19;
	char field_A1A;
	char field_A1B;
	char field_A1C;
	char field_A1D;
	char field_A1E;
	char field_A1F;
	char field_A20;
	char field_A21;
	char field_A22;
	char field_A23;
	char field_A24;
	char field_A25;
	char field_A26;
	char field_A27;
	char field_A28;
	char field_A29;
	char field_A2A;
	char field_A2B;
	char field_A2C;
	char field_A2D;
	char field_A2E;
	char field_A2F;
	char field_A30;
	char field_A31;
	char field_A32;
	char field_A33;
	char field_A34;
	char field_A35;
	char field_A36;
	char field_A37;
	char field_A38;
	char field_A39;
	char field_A3A;
	char field_A3B;
	char field_A3C;
	char field_A3D;
	char field_A3E;
	char field_A3F;
	char field_A40;
	char field_A41;
	char field_A42;
	char field_A43;
	char field_A44;
	char field_A45;
	char field_A46;
	char field_A47;
	char field_A48;
	char field_A49;
	char field_A4A;
	char field_A4B;
	char field_A4C;
	char field_A4D;
	char field_A4E;
	char field_A4F;
	char field_A50;
	char field_A51;
	char field_A52;
	char field_A53;
	char field_A54;
	char field_A55;
	char field_A56;
	char field_A57;
	char field_A58;
	char field_A59;
	char field_A5A;
	char field_A5B;
	char field_A5C;
	char field_A5D;
	char field_A5E;
	char field_A5F;
	char field_A60;
	char field_A61;
	char field_A62;
	char field_A63;
	char field_A64;
	char field_A65;
	char field_A66;
	char field_A67;
	char field_A68;
	char field_A69;
	char field_A6A;
	char field_A6B;
	char field_A6C;
	char field_A6D;
	char field_A6E;
	char field_A6F;
	char field_A70;
	char field_A71;
	char field_A72;
	char field_A73;
	char field_A74;
	char field_A75;
	char field_A76;
	char field_A77;
	char field_A78;
	char field_A79;
	char field_A7A;
	char field_A7B;
	char field_A7C;
	char field_A7D;
	char field_A7E;
	char field_A7F;
	char field_A80;
	char field_A81;
	char field_A82;
	char field_A83;
	char field_A84;
	char field_A85;
	char field_A86;
	char field_A87;
	char field_A88;
	char field_A89;
	char field_A8A;
	char field_A8B;
	char field_A8C;
	char field_A8D;
	char field_A8E;
	char field_A8F;
	char field_A90;
	char field_A91;
	char field_A92;
	char field_A93;
	char field_A94;
	char field_A95;
	char field_A96;
	char field_A97;
	char field_A98;
	char field_A99;
	char field_A9A;
	char field_A9B;
	char field_A9C;
	char field_A9D;
	char field_A9E;
	char field_A9F;
	char field_AA0;
	char field_AA1;
	char field_AA2;
	char field_AA3;
	char field_AA4;
	char field_AA5;
	char field_AA6;
	char field_AA7;
	char field_AA8;
	char field_AA9;
	char field_AAA;
	char field_AAB;
	char field_AAC;
	char field_AAD;
	char field_AAE;
	char field_AAF;
	char field_AB0;
	char field_AB1;
	char field_AB2;
	char field_AB3;
	char field_AB4;
	char field_AB5;
	char field_AB6;
	char field_AB7;
	char field_AB8;
	char field_AB9;
	char field_ABA;
	char field_ABB;
	char field_ABC;
	char field_ABD;
	char field_ABE;
	char field_ABF;
	char field_AC0;
	char field_AC1;
	char field_AC2;
	char field_AC3;
	char field_AC4;
	char field_AC5;
	char field_AC6;
	char field_AC7;
	char field_AC8;
	char field_AC9;
	char field_ACA;
	char field_ACB;
	char field_ACC;
	char field_ACD;
	char field_ACE;
	char field_ACF;
	char field_AD0;
	char field_AD1;
	char field_AD2;
	char field_AD3;
	char field_AD4;
	char field_AD5;
	char field_AD6;
	char field_AD7;
	char field_AD8;
	char field_AD9;
	char field_ADA;
	char field_ADB;
	char field_ADC;
	char field_ADD;
	char field_ADE;
	char field_ADF;
	char field_AE0;
	char field_AE1;
	char field_AE2;
	char field_AE3;
	char field_AE4;
	char field_AE5;
	__int16 field_AE6;
	char field_AE8;
	char field_AE9;
	char field_AEA;
	char field_AEB;
	char field_AEC;
	char field_AED;
	char field_AEE;
	char field_AEF;
	char field_AF0;
	char field_AF1;
	char field_AF2;
	char field_AF3;
	char field_AF4;
	char field_AF5;
	char field_AF6;
	char field_AF7;
	char field_AF8;
	char field_AF9;
	char field_AFA;
	char field_AFB;
	char field_AFC;
	char field_AFD;
	char field_AFE;
	char field_AFF;
	char field_B00;
	char field_B01;
	char field_B02;
	char field_B03;
	char field_B04;
	char field_B05;
	char field_B06;
	char field_B07;
	char field_B08;
	char field_B09;
	char field_B0A;
	char field_B0B;
	char field_B0C;
	char field_B0D;
	char field_B0E;
	char field_B0F;
	char field_B10;
	char field_B11;
	char field_B12;
	char field_B13;
	char chatString[49];	   // 0xb14
	char chatStringTerminator; // 0xb45
	char chatStringCharCount;  // 0xb46
	char multiChatMode;		   // 0xb47
	PlayerCamera Camera;
	__int16 screenResolutionSetting; // Ofs: 0x0B95
	int rosterID;
	int missionTime;
	int posX;
	int posY;
	int posZ;
	__int16 roll2;
	__int16 pitch2;
	__int16 yaw2;
	int lifespan;
	__int16 currentSpeed;
	__int16 speedRelease;
	__int16 currentSpeedFraction;
	__int16 objectID;
	char isEjectingDelta;
	int criticalMessageTimer;
	__int16 field_BC2;
	__int16 field_BC4;
	__int16 field_BC6;
	__int16 field_BC8;
	char timeInMissionMilliseconds2;
	char timeInMissionSeconds2;
	char timeInMissionMinutes;
	char timeInMissionHours;
	char inTraining;
};

static_assert(sizeof(PlayerDataEntry) == 3023, "size of PlayerDataEntry must be 3023");

struct CraftDefinitionEntry {
	void *pCraftShortName;
	void *pCraftName;
	WORD Score;
	WORD PromoPointWorth;
	BYTE HasHyperdrive;
	BYTE LaserConvergenceType;
	BYTE HasShields;
	DWORD ShieldHitPoints; // Ofs: 0000000F
	BYTE HitsToEvade;
	BYTE field_14;
	DWORD HullHitpoints;
	DWORD CriticalDmgThreshold;
	WORD SystemHitpoints;
	BYTE EngineThrottle; // Ofs: 0x1f
	WORD TopSpeedMPH;
	WORD Acceleration;
	WORD Deceleration;
	WORD YawRate;
	WORD RollingYawPercentage;
	WORD RollRate;
	WORD PitchRate;
	WORD DeathRotationSpeed;
	WORD DriftSpeed; // Ofs: 0x30
	char CockpitFileName[256]; // Ofs: 0x32
	WORD LaserObjectType[3]; // Ofs 0x132
	BYTE LaserStartingHardpoint[3];
	BYTE LaserEndingHardpoint[3];
	BYTE LaserNumOfCannons[3];
	BYTE LaserGroupType[3]; // Ofs: 0x141
	DWORD LaserRange[3];
	WORD LaserFireRatio[3];
	WORD WarheadObjectType[2]; // Ofs: 0x156
	BYTE WahreadStartingHardpoint[2];
	BYTE WarheadEndingHardpoint[2];
	BYTE WarheadAccumulators[2];
	BYTE WarheadCapacity[2]; // Ofs: 0x160
	BYTE /* CraftDefHardpoints */ HardPoints[128]; // Ofs: 0x162
	DWORD Engines[16];
	BYTE EngineMeshIDs[16];
	BYTE EnginesCount; // Ofs: 0x232
	BYTE CountermeasuresCount;
	DWORD field_234;
	WORD CockpitPosY; // POV Y, Ofs: 0x238
	WORD CockpitPosZ; // POV Z
	WORD CockpitPosX; // POV X
	WORD TurretPositionY[2]; // Ofs 0x23E
	WORD TurretPositionZ[2];
	WORD TurretPositionX[2];
	WORD TurretOrientationX[2];
	WORD TurretOrientationY[2];
	WORD TurretOPTModelID[2]; // Ofs: 0x252
	WORD TurretArcX[2]; // Ofs: 0x256
	WORD TurretArcY[2];
	DWORD DockPositionY;
	DWORD DockFromSmallPositionZ;
	DWORD DockFromBigPositionZ;
	DWORD DockToSmallPositionZ;
	DWORD DockToBigPositionZ; // Ofs: 0x26E
	DWORD InsideHangarX;
	DWORD InsideHangarZ;
	DWORD InsideHangarY;
	DWORD OutsideHangarX;
	DWORD OutsideHangarZ;
	DWORD OutsideHangarY;
	WORD SizeScale; // Ofs: 0x28A
	WORD SizeX;
	WORD SizeZ;
	WORD SizeY;
	WORD NumOfDockingPoints; // Ofs: 0x292
	BYTE DockingPoints[108]; // Ofs: 294 Vector3Int[9]
	DWORD AccStartPositionX;
	DWORD AccStartPositionZ;
	DWORD AccStartPositionY;
	DWORD AccEndPositionX;
	DWORD AccEndPositionZ;
	DWORD AccEndPositionY;
	BYTE NumOfFloatHardpoints;
	BYTE NumOfAccEndPoints;
	BYTE AccEndPoints[96]; // Ofs: 0x31A Vector3Int[8];
	BYTE NumOfJammingPoints;
	BYTE JammingPoints[96]; // Ofs: 0x37b, VectorInt[8];
};

static_assert(sizeof(CraftDefinitionEntry) == 0x3DB, "size of CraftDefinition must be 0x3DB");

#pragma pack(pop)
