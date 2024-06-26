#pragma once


enum MissionDirectory : __int8
{
	MissionDirectory_UNK0 = 0,
	MissionDirectory_Melee = 1,
	MissionDirectory_Combat = 2,
	MissionDirectory_Skirmish = 3,
	MissionDirectory_Missions = 4,
};

enum Difficulty
{
	Difficulty_Easy = 0,
	Difficulty_Medium = 1,
	Difficulty_Hard = 2,
	Difficulty_NULL = 3,
};

enum CombatSimulatorScreenType : __int8
{
	CombatSimulatorScreenType_MissionDescription = 0,
	CombatSimulatorScreenType_Roster = 1,
	CombatSimulatorScreenType_FGCreation = 2,
	CombatSimulatorScreenType_Save = 3,
	CombatSimulatorScreenType_Goals = 4,
	CombatSimulatorScreenType_Load = 5,
	CombatSimulatorScreenType_BattleSelect = 6,
};

enum IFFCode : __int8
{
	IFF_Rebel = 0x0,
	IFF_Imperial = 0x1,
	IFF_Blue = 0x2,
	IFF_Yellow = 0x3,
	IFF_Red = 0x4,
	IFF_Purple = 0x5,
};

enum PlayerType : __int8
{
	PlayerType_None = 0,
	PlayerType_UNK1 = 1,
	PlayerType_Singleplayer = 2,
	PlayerType_Multiplayer_Client = 3,
	PlayerType_Multiplayer_Host = 4,
};

enum MissionCraftOptions : __int32
{
	MissionCraftOptions_None = 0,
	MissionCraftOptions_AllFlyable = 1,
	MissionCraftOptions_AllRebelFlyable = 2,
	MissionCraftOptions_AllImperialFlyable = 3,
	MissionCraftOptions_Custom = 4,
};

enum TacticalOfficer : __int8
{
	TacticalOfficer_Devers = 0,
	TacticalOfficer_Kupalo = 1,
	TacticalOfficer_Zaletta = 2,
	TacticalOfficer_ImpOfficer1 = 3,
	TacticalOfficer_ImpOfficer2 = 4,
	TacticalOfficer_TacOfficer3 = 5,
	TacticalOfficer_TacOfficer4 = 6,
	TacticalOfficer_TacOfficer5 = 7,
	TacticalOfficer_Emkay = 8,
};

enum FGCreationSelectCraftMenu : __int32
{
	FGCreationSelectCraftMenu_None = 0,
	FGCreationSelectCraftMenu_NonFlyableCraft = 1,
	FGCreationSelectCraftMenu_AllFlyableCraft = 2,
	FGCreationSelectCraftMenu_AutoDetect = 3,
	FGCreationSelectCraftMenu_AllRebelFlyable = 4,
	FGCreationSelectCraftMenu_AllImperialFlyable = 5,
};

enum RosterGoalType : __int8
{
	RosterGoalType_Normal = 0,
	RosterGoalType_Melee = 1,
};

enum FGCreationGenusCategory : __int32
{
	FGCreationGenusCategory_Inactive = 0,
	FGCreationGenusCategory_Starfighter = 1,
	FGCreationGenusCategory_ShuttleLightTransport = 2,
	FGCreationGenusCategory_UtilityCraft = 3,
	FGCreationGenusCategory_Container = 4,
	FGCreationGenusCategory_FreighterHeavyTransport = 5,
	FGCreationGenusCategory_Starship = 6,
	FGCreationGenusCategory_Station = 7,
	FGCreationGenusCategory_WeaponEmplacement = 8,
	FGCreationGenusCategory_Mine = 9,
	FGCreationGenusCategory_SatelliteBuoy = 10,
	FGCreationGenusCategory_Droid = 11,
};

// ; enum ObjectGenus, copyof_225, width 1 byte
enum ObjectGenus : uint8_t
{
	Genus_Starfighter = 0,
	Genus_Transport = 1,
	Genus_Utility = 2,
	Genus_Freighter = 3,
	Genus_Starship = 4,
	Genus_Platform = 5,
	Genus_PlayerProjectile = 6,
	Genus_OtherProjectile = 7,
	Genus_Mine = 8,
	Genus_Satellite = 9,
	Genus_NormalDebris = 10,
	Genus_SmallDebris = 11,
	Genus_Backdrop = 12,
	Genus_Explosion = 13,
	Genus_Obstacle = 14,
	Genus_Deathstar2 = 15,
	Genus_People = 16,
	Genus_Container = 17,
	Genus_Droid = 18,
	Genus_Armament = 19,
	Genus_LargeDebris = 20,
	Genus_SalvageYard = 21
};

enum KeyCode
{
	KeyCode_CTRLA = 0x1,
	KeyCode_CTRLB = 0x2,
	KeyCode_CTRLC = 0x3,
	KeyCode_CTRLD = 0x4,
	KeyCode_CTRLE = 0x5,
	KeyCode_CTRLF = 0x6,
	KeyCode_CTRLG = 0x7,
	KeyCode_BACKSPACE = 0x8,
	KeyCode_TAB = 0x9,
	KeyCode_CTRLJ = 0xA,
	KeyCode_CTRLK = 0xB,
	KeyCode_CTRLL = 0xC,
	KeyCode_ENTER = 0xD,
	KeyCode_CTRLN = 0xE,
	KeyCode_CTRLO = 0xF,
	KeyCode_CTRLP = 0x10,
	KeyCode_CTRLQ = 0x11,
	KeyCode_CTRLR = 0x12,
	KeyCode_CTRLS = 0x13,
	KeyCode_CTRLT = 0x14,
	KeyCode_CTRLU = 0x15,
	KeyCode_CTRLV = 0x16,
	KeyCode_CTRLW = 0x17,
	KeyCode_CTRLX = 0x18,
	KeyCode_CTRLY = 0x19,
	KeyCode_CTRLZ = 0x1A,
	KeyCode_ESCAPEORCTRLLEFTBRACKET = 0x1B,
	KeyCode_UNKNOWN1C = 0x1C,
	KeyCode_CTRLRIGHTBRACKET = 0x1D,
	KeyCode_UNKNOWN1E = 0x1E,
	KeyCode_UNKNOWN1F = 0x1F,
	KeyCode_SPACE = 0x20,
	KeyCode_EXCLAMATION = 0x21,
	KeyCode_QUOTES = 0x22,
	KeyCode_POUND = 0x23,
	KeyCode_DOLLAR = 0x24,
	KeyCode_PERCENT = 0x25,
	KeyCode_AMPERSAND = 0x26,
	KeyCode_APOSTROPHE = 0x27,
	KeyCode_LEFTPARENTHESIS = 0x28,
	KeyCode_RIGHTPARENTHESIS = 0x29,
	KeyCode_ASTERISK = 0x2A,
	KeyCode_PLUS = 0x2B,
	KeyCode_COMMA = 0x2C,
	KeyCode_SUBTRACT = 0x2D,
	KeyCode_PERIOD = 0x2E,
	KeyCode_SLASH = 0x2F,
	KeyCode_0 = 0x30,
	KeyCode_1 = 0x31,
	KeyCode_2 = 0x32,
	KeyCode_3 = 0x33,
	KeyCode_4 = 0x34,
	KeyCode_5 = 0x35,
	KeyCode_6 = 0x36,
	KeyCode_7 = 0x37,
	KeyCode_8 = 0x38,
	KeyCode_9 = 0x39,
	KeyCode_COLON = 0x3A,
	KeyCode_SEMICOLON = 0x3B,
	KeyCode_LESSTHAN = 0x3C,
	KeyCode_EQUAL = 0x3D,
	KeyCode_GREATERTHAN = 0x3E,
	KeyCode_QUESTIONMARK = 0x3F,
	KeyCode_ATSYMBOL = 0x40,
	KeyCode_SHIFTA = 0x41,
	KeyCode_SHIFTB = 0x42,
	KeyCode_SHIFTC = 0x43,
	KeyCode_SHIFTD = 0x44,
	KeyCode_SHIFTE = 0x45,
	KeyCode_SHIFTF = 0x46,
	KeyCode_SHIFTG = 0x47,
	KeyCode_SHIFTH = 0x48,
	KeyCode_SHIFTI = 0x49,
	KeyCode_SHIFTJ = 0x4A,
	KeyCode_SHIFTK = 0x4B,
	KeyCode_SHIFTL = 0x4C,
	KeyCode_SHIFTM = 0x4D,
	KeyCode_SHIFTN = 0x4E,
	KeyCode_SHIFTO = 0x4F,
	KeyCode_SHIFTP = 0x50,
	KeyCode_SHIFTQ = 0x51,
	KeyCode_SHIFTR = 0x52,
	KeyCode_SHIFTS = 0x53,
	KeyCode_SHIFTT = 0x54,
	KeyCode_SHIFTU = 0x55,
	KeyCode_SHIFTV = 0x56,
	KeyCode_SHIFTW = 0x57,
	KeyCode_SHIFTX = 0x58,
	KeyCode_SHIFTY = 0x59,
	KeyCode_SHIFTZ = 0x5A,
	KeyCode_LEFTBRACKET = 0x5B,
	KeyCode_BACKSLASH = 0x5C,
	KeyCode_RIGHTBRACKET = 0x5D,
	KeyCode_CARET = 0x5E,
	KeyCode_UNDERSCORE = 0x5F,
	KeyCode_OPENSINGLEQUOTE = 0x60,
	KeyCode_A = 0x61,
	KeyCode_B = 0x62,
	KeyCode_C = 0x63,
	KeyCode_D = 0x64,
	KeyCode_E = 0x65,
	KeyCode_F = 0x66,
	KeyCode_G = 0x67,
	KeyCode_H = 0x68,
	KeyCode_I = 0x69,
	KeyCode_J = 0x6A,
	KeyCode_K = 0x6B,
	KeyCode_L = 0x6C,
	KeyCode_M = 0x6D,
	KeyCode_N = 0x6E,
	KeyCode_O = 0x6F,
	KeyCode_P = 0x70,
	KeyCode_Q = 0x71,
	KeyCode_R = 0x72,
	KeyCode_S = 0x73,
	KeyCode_T = 0x74,
	KeyCode_U = 0x75,
	KeyCode_V = 0x76,
	KeyCode_W = 0x77,
	KeyCode_X = 0x78,
	KeyCode_Y = 0x79,
	KeyCode_Z = 0x7A,
	KeyCode_ALTLEFTBRACKET = 0x7B,
	KeyCode_ALTBACKSLASH = 0x7C,
	KeyCode_ALTRIGHTBRACKET = 0x7D,
	KeyCode_UNKNOWN7E = 0x7E,
	KeyCode_UNKNOWN7F = 0x7F,
	KeyCode_ALTA = 0x80,
	KeyCode_ALTB = 0x81,
	KeyCode_ALTC = 0x82,
	KeyCode_ALTD = 0x83,
	KeyCode_ALTE = 0x84,
	KeyCode_ALTF = 0x85,
	KeyCode_ALTG = 0x86,
	KeyCode_ALTH = 0x87,
	KeyCode_ALTI = 0x88,
	KeyCode_ALTJ = 0x89,
	KeyCode_ALTK = 0x8A,
	KeyCode_ALTL = 0x8B,
	KeyCode_ALTM = 0x8C,
	KeyCode_ALTN = 0x8D,
	KeyCode_ALTO = 0x8E,
	KeyCode_ALTP = 0x8F,
	KeyCode_ALTQ = 0x90,
	KeyCode_ALTR = 0x91,
	KeyCode_ALTS = 0x92,
	KeyCode_ALTT = 0x93,
	KeyCode_ALTU = 0x94,
	KeyCode_ALTV = 0x95,
	KeyCode_ALTW = 0x96,
	KeyCode_ALTX = 0x97,
	KeyCode_ALTY = 0x98,
	KeyCode_ALTZ = 0x99,
	KeyCode_ALT0 = 0x9A,
	KeyCode_ALT1 = 0x9B,
	KeyCode_ALT2 = 0x9C,
	KeyCode_ALT3 = 0x9D,
	KeyCode_ALT4 = 0x9E,
	KeyCode_ALT5 = 0x9F,
	KeyCode_ALT6 = 0xA0,
	KeyCode_ALT7 = 0xA1,
	KeyCode_ALT8 = 0xA2,
	KeyCode_ALT9 = 0xA3,
	KeyCode_LEFTBRACE = 0x7B,
	KeyCode_PIPE = 0x7C,
	KeyCode_RIGHTBRACE = 0x7D,
	KeyCode_TILDE = 0x7E,
	KeyCode_ARROWLEFT = 0xA4,
	KeyCode_ARROWRIGHT = 0xA5,
	KeyCode_ARROWUP = 0xA6,
	KeyCode_ARROWDOWN = 0xA7,
	KeyCode_INSERT = 0xA8,
	KeyCode_DELETE = 0xA9,
	KeyCode_HOME = 0xAA,
	KeyCode_END = 0xAB,
	KeyCode_PGUP = 0xAC,
	KeyCode_PGDN = 0xAD,
	KeyCode_PRINTSCREEN = 0xAE,
	KeyCode_SCROLLLOCK = 0xAF,
	KeyCode_UNKNOWNB0 = 0xB0,
	KeyCode_CAPSLOCK = 0xB1,
	KeyCode_NUMPAD0 = 0xB2,
	KeyCode_NUMPAD1 = 0xB3,
	KeyCode_NUMPAD2 = 0xB4,
	KeyCode_NUMPAD3 = 0xB5,
	KeyCode_NUMPAD4 = 0xB6,
	KeyCode_NUMPAD5 = 0xB7,
	KeyCode_NUMPAD6 = 0xB8,
	KeyCode_NUMPAD7 = 0xB9,
	KeyCode_NUMPAD8 = 0xBA,
	KeyCode_NUMPAD9 = 0xBB,
	KeyCode_NUMLOCK = 0xBC,
	KeyCode_NUMPADDIV = 0xBD,
	KeyCode_NUMPADMULT = 0xBE,
	KeyCode_NUMPADSUB = 0xBF,
	KeyCode_NUMPADADD = 0xC0,
	KeyCode_NUMPADENTER = 0xC1,
	KeyCode_NUMPADDOT = 0xC2,
	KeyCode_F1 = 0xC3,
	KeyCode_F2 = 0xC4,
	KeyCode_F3 = 0xC5,
	KeyCode_F4 = 0xC6,
	KeyCode_F5 = 0xC7,
	KeyCode_F6 = 0xC8,
	KeyCode_F7 = 0xC9,
	KeyCode_F8 = 0xCA,
	KeyCode_F9 = 0xCB,
	KeyCode_F10 = 0xCC,
	KeyCode_F11 = 0xCD,
	KeyCode_F12 = 0xCE,
	KeyCode_SHIFTF1 = 0xCF,
	KeyCode_SHIFTF2 = 0xD0,
	KeyCode_SHIFTF3 = 0xD1,
	KeyCode_SHIFTF4 = 0xD2,
	KeyCode_SHIFTF5 = 0xD3,
	KeyCode_SHIFTF6 = 0xD4,
	KeyCode_SHIFTF7 = 0xD5,
	KeyCode_SHIFTF8 = 0xD6,
	KeyCode_SHIFTF9 = 0xD7,
	KeyCode_SHIFTF10 = 0xD8,
	KeyCode_SHIFTF11 = 0xD9,
	KeyCode_SHIFTF12 = 0xDA,
	KeyCode_THROTTLE6PERCENT = 0xDB,
	KeyCode_THROTTLE12PERCENT = 0xDC,
	KeyCode_THROTTLE18PERCENT = 0xDD,
	KeyCode_THROTTLE25PERCENT = 0xDE,
	KeyCode_THROTTLE37PERCENT = 0xDF,
	KeyCode_THROTTLE43PERCENT = 0xE0,
	KeyCode_THROTTLE50PERCENT = 0xE1,
	KeyCode_THROTTLE56PERCENT = 0xE2,
	KeyCode_THROTTLE62PERCENT = 0xE3,
	KeyCode_THROTTLE75PERCENT = 0xE4,
	KeyCode_THROTTLE81PERCENT = 0xE5,
	KeyCode_THROTTLE87PERCENT = 0xE6,
	KeyCode_THROTTLE93PERCENT = 0xE7,
	KeyCode_CTRLPRINTSCREEN = 0xE8,
	KeyCode_CTRLSCROLLLOCK = 0xE9,
	KeyCode_SHIFT = 0xFD,
	KeyCode_CTRL = 0xFE,
	KeyCode_ALT = 0xFF,
};
