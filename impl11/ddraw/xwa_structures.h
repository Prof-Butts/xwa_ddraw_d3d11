#pragma once

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef STRICT
#define STRICT
#endif

#include <d3d.h>
#include "Vectors.h"

#pragma pack(push, 1)

enum OptNodeEnum : unsigned int
{
	OptNode_NodeGroup = 0,
	OptNode_FaceData = 1,
	OptNode_TransformPositionRotation = 2,
	OptNode_MeshVertices = 3,
	OptNode_TransformPosition = 4,
	OptNode_TransformRotation = 5,
	OptNode_TransformScale = 6,
	OptNode_NodeReference = 7,
	OptNode_Unknown9 = 9,
	OptNode_Unknown10 = 10,
	OptNode_VertexNormals = 11,
	OptNode_Unknown12 = 12,
	OptNode_TextureVertices = 13,
	OptNode_Unknown14 = 14,
	OptNode_FaceData_0F = 15,
	OptNode_FaceData_10 = 16,
	OptNode_FaceData_11 = 17,
	OptNode_Unknown19 = 19,
	OptNode_Texture = 20,
	OptNode_FaceGrouping = 21,
	OptNode_Hardpoint = 22,
	OptNode_RotationScale = 23,
	OptNode_NodeSwitch = 24,
	OptNode_MeshDescriptor = 25,
	OptNode_TextureAlpha = 26,
	OptNode_D3DTexture = 27,
	OptNode_EngineGlow = 28,
};

struct TieFlightGroupWaypoint
{
	short X;
	short Y;
	short Z;
	short m06;
};

static_assert(sizeof(TieFlightGroupWaypoint) == 8, "size of TieFlightGroupWaypoint must be 8");

struct TieFlightGroup
{
	char Name[20];
	char unk014[5];
	unsigned char GlobalCargoIndex;
	char unk01A[14];
	char Cargo[20];
	char SpecialCargo[20];
	char unk050[27];
	unsigned char CraftId;
	unsigned char CraftsCount;
	char unk06D[3];
	unsigned char Iff;
	unsigned char Team;
	char unk072[6];
	unsigned char GlobalGroupId;
	char unk079[1];
	unsigned char WavesCount;
	char unk07B[47];
	unsigned char ArrivalDelayMinutes;
	unsigned char ArrivalDelaySeconds;
	char unk0AC[3294];
	TieFlightGroupWaypoint StartPoints[4];
	char StartPointRegions[4];
	char unkDAE[100];
	int PlanetId;
	char unkE16[40];
};

static_assert(sizeof(TieFlightGroup) == 3646, "size of TieFlightGroup must be 3646");

// V0x0080DC80
//Array<TieFlightGroupEx, 192> s_XwaTieFlightGroups;
extern TieFlightGroup* g_XwaTieFlightGroups;

struct XwaPlanet
{
	/* 0x0000 */ unsigned short ModelIndex;
	/* 0x0002 */ unsigned char BackdropFlags;
};

// V0x005B1140
//Array<XwaPlanet, 104> s_XwaPlanets
extern XwaPlanet* g_XwaPlanets;

struct ExeEnableEntry
{
	/* 0x0000 */ char EnableOptions;    // flags ExeEnable00Enum
	/* 0x0001 */ char RessourceOptions; // flags ExeEnable01Enum
	/* 0x0002 */ char ObjectCategory;   // ObjectCategoryEnum
	/* 0x0003 */ char ShipCategory;     // ShipCategoryEnum
	/* 0x0004 */ unsigned int ObjectSize;
	/* 0x0008 */ void* pData1;
	/* 0x000C */ void* pData2;
	/* 0x0010 */ unsigned short GameOptions; // flags ExeEnable10Enum
	/* 0x0012 */ short CraftIndex; // CraftIndexEnum
	/* 0x0014 */ short DataIndex1;
	/* 0x0016 */ short DataIndex2;
};

static_assert(sizeof(ExeEnableEntry) == 24, "size of ExeEnableEntry must be 24");

// ExeObjectsTable:
// V0x005FB240
//Array<ExeObjectEntry, 557> s_ExeObjectsTable;
const extern ExeEnableEntry* g_ExeObjectsTable;

struct TieHeader
{
	char unk0000[9128];
	unsigned char TimeLimit;
	char unk23A9[65];
};

static_assert(sizeof(TieHeader) == 9194, "size of TieHeader must be 9194");

struct XwaMission
{
	TieFlightGroup FlightGroups[192];
	char unkAAE80[18780];
	TieHeader Header;
	char unkB1BC6[562426];
};

static_assert(sizeof(XwaMission) == 1290432, "size of XwaMission must be 1290432");

struct XwaPlayer
{
	char unk0000[16];
	unsigned char Region;
	char unk0011[3006];
};

static_assert(sizeof(XwaPlayer) == 3023, "size of XwaPlayer must be 3023");

// This is the same as ObjectEntry
struct XwaObject
{
	//short XwaObject_m00;
	short ObjectId;

	//unsigned short ModelIndex;
	unsigned short ObjectSpecies; // Type enum ObjectTypeEnum

	unsigned char ShipCategory; // See XWAEnums.h:ObjectGenus
	unsigned char TieFlightGroupIndex;
	unsigned char Region;
	int PositionX; // The actual position of this craft. We can use this to move the player or pretty much any ship, maybe.
	int PositionY;
	int PositionZ;
	short HeadingXY;
	short HeadingZ;
	short XwaObject_m17;
	short XwaObject_m19;
	short XwaObject_m1B;
	unsigned char XwaObject_m1D[2];
	int PlayerIndex;
	int pMobileObject; // This is the same as MobileObjectEntry *MobileObjectPtr;
};

static_assert(sizeof(XwaObject) == 39, "size of XwaObject must be 39");

struct XwaSpeciesTMInfo
{
	void* pData;
	unsigned short Width;
	unsigned short Height;
	unsigned int Color;
	unsigned char MipmapLevel;
};

static_assert(sizeof(XwaSpeciesTMInfo) == 13, "size of XwaSpeciesTMInfo must be 13");

struct XwaD3dTexture
{
	LPDIRECT3DTEXTURE lpD3DTexture;
	LPDIRECTDRAWSURFACE lpDDSurface;
	DDSURFACEDESC DDSurfaceDesc;
	D3DTEXTUREHANDLE D3DTextureHandle;
	bool IsCached;
	char unk79[3];
	int Width;
	int Height;
	unsigned int DataSize;
	int XwaD3dTexture_m88;
	XwaD3dTexture* pPrevious;
	XwaD3dTexture* pNext;
};

static_assert(sizeof(XwaD3dTexture) == 148, "size of XwaD3dTexture must be 148");

struct XwaD3dVertex
{
	float x;
	float y;
	float z;
	float rhw;
	unsigned int color;
	unsigned int specular;
	float tu;
	float tv;
};

static_assert(sizeof(XwaD3dVertex) == 32, "size of XwaD3dVertex must be 32");

struct XwaD3dTriangle
{
	int v1;
	int v2;
	int v3;
	unsigned int RenderStatesFlags;
	XwaD3dTexture* pTexture;
};

static_assert(sizeof(XwaD3dTriangle) == 20, "size of XwaD3dTriangle must be 20");

/*
struct XwaDataMesh
{
	XwaD3dVertex Vertices[384];
	int VerticesCount;
	XwaD3dTriangle Triangles[384];
	int TrianglesCount;
	XwaDataMesh* Next;
};

static_assert(sizeof(XwaDataMesh) == 19980, "size of XwaDataMesh must be 19980");
*/

struct XwaVector3
{
	float x;
	float y;
	float z;

	XwaVector3() {
		x = y = z = 0.0f;
	}

	XwaVector3(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	XwaVector3(const Vector3& that)
	{
		this->x = that.x;
		this->y = that.y;
		this->z = that.z;
	}

	friend XwaVector3 operator+(XwaVector3 lhs, const XwaVector3& rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
		return lhs;
	}

	friend XwaVector3 operator-(XwaVector3 lhs, const XwaVector3& rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
		return lhs;
	}

	friend XwaVector3 operator*(XwaVector3 lhs, const float rhs)
	{
		lhs.x *= rhs;
		lhs.y *= rhs;
		lhs.z *= rhs;
		return lhs;
	}

	inline float operator[](int index) const {
		return (&x)[index];
	}

	inline float& operator[](int index) {
		return (&x)[index];
	}

	bool equals(const XwaVector3 &rhs)
	{
		float difx = fabs(x - rhs.x);
		float dify = fabs(y - rhs.y);
		float difz = fabs(z - rhs.z);
		if (difx <= 0.0001f && dify <= 0.0001f && difz <= 0.0001f)
			return true;
		else
			return false;
	}

	XwaVector3 normalize() {
		float L = sqrt(x * x + y * y + z * z);
		x /= L;
		y /= L;
		z /= L;
		return *this;
	}

};

static_assert(sizeof(XwaVector3) == 12, "size of XwaVector3 must be 12");

struct XwaTextureVertex
{
	float u;
	float v;
};

static_assert(sizeof(XwaTextureVertex) == 8, "size of XwaTextureVertex must be 8");

struct XwaMatrix3x3
{
	float _11;
	float _12;
	float _13;
	float _21;
	float _22;
	float _23;
	float _31;
	float _32;
	float _33;
};

static_assert(sizeof(XwaMatrix3x3) == 36, "size of XwaMatrix3x3 must be 36");

struct XwaTransform
{
	XwaVector3 Position;
	XwaMatrix3x3 Rotation;
};

static_assert(sizeof(XwaTransform) == 48, "size of XwaTransform must be 48");

struct OptFaceDataNode_01_Data_Indices
{
	int Vertex[4];
	int Edge[4];
	int TextureVertex[4];
	int VertexNormal[4];
};

static_assert(sizeof(OptFaceDataNode_01_Data_Indices) == 64, "size of OptFaceDataNode_01_Data_Indices must be 64");

struct XwaTextureDescription
{
	unsigned short* Palettes;
	int XwaTextureDescription_m04;
	int TextureSize;
	int DataSize;
	int Width;
	int Height;
};

static_assert(sizeof(XwaTextureDescription) == 24, "size of XwaTextureDescription must be 24");

struct XwaTextureSurface
{
	unsigned char XwaTextureSurface_m00;
	char unk01[3];
	LPDIRECTDRAWSURFACE lpDDSurface;
	LPDIRECT3DTEXTURE lpD3DTexture;
	LPDIRECTDRAWPALETTE lpDDPalette;
	XwaD3dTexture D3dTexture;
	unsigned char XwaTextureSurface_mA4;
	char unkA5[3];
	int MipmapLevel;
	int MipmapCount;
};

static_assert(sizeof(XwaTextureSurface) == 176, "size of XwaTextureSurface must be 176");

struct XwaD3DInfo
{
	int Id;
	short RefCounter;
	XwaTextureDescription TextureDescription;
	unsigned char AlphaMask;
	int MipMapsCount;
	XwaTextureSurface* ColorMap[6];
	XwaTextureSurface* LightMap[6];
	XwaD3DInfo* pNext;
	XwaD3DInfo* pPrevious;
};

static_assert(sizeof(XwaD3DInfo) == 91, "size of XwaD3DInfo must be 91");

struct XwaGlowMarkItem
{
	float Size;
	unsigned int Color;
	XwaTextureSurface* pTextureSurface;
};

static_assert(sizeof(XwaGlowMarkItem) == 12, "size of XwaGlowMarkItem must be 12");

struct XwaGlowMark
{
	int Count;
	XwaGlowMarkItem Items[2];
	XwaTextureVertex* UVArray;
	unsigned short* FaceOn;
};

static_assert(sizeof(XwaGlowMark) == 36, "size of XwaGlowMark must be 36");

struct XwaKnockoutData
{
	unsigned short GlowMarksCount;
	XwaVector3* GlowMarksPositions[16];
	XwaGlowMark GlowMarks[16];
	float XwaKnockoutData_m282;
	float XwaKnockoutData_m286;
	float XwaKnockoutData_m28A;
	float XwaKnockoutData_m28E;
	float XwaKnockoutData_m292;
	float XwaKnockoutData_m296;
	float XwaKnockoutData_m29A;
	float XwaKnockoutData_m29E;
	float XwaKnockoutData_m2A2;
	float XwaKnockoutData_m2A6;
	float XwaKnockoutData_m2AA;
	float XwaKnockoutData_m2AE;
	float XwaKnockoutData_m2B2;
	char unk2B6[4];
	short XwaKnockoutData_m2BA;
	short XwaKnockoutData_m2BC;
	int XwaKnockoutData_m2BE;
	short XwaKnockoutData_m2C2;
	unsigned short ModelIndex;
	short XwaKnockoutData_m2C6;
	unsigned int XwaKnockoutData_m2C8[2];
	float XwaKnockoutData_m2D0[2];
	XwaKnockoutData* XwaKnockoutData_m2D8;
	XwaKnockoutData* XwaKnockoutData_m2DC;
	XwaKnockoutData* XwaKnockoutData_m2E0;
};

static_assert(sizeof(XwaKnockoutData) == 740, "size of XwaKnockoutData must be 740");

struct XwaObject3D
{
	int XwaObject3D_m00;
	void* XwaObject3D_m04;
	void* pDataTrail;
	void* XwaObject3D_m0C;
	XwaKnockoutData* pKnockoutData;
	void* pDataKnockout;
};

static_assert(sizeof(XwaObject3D) == 24, "size of XwaObject3D must be 24");

// See JeremyAnsel.Xwa.Opt for more details
struct MeshDescriptor {
	DWORD MeshType;		 // MainHull, Engine, Wing, etc
	DWORD ExplosionType; // ExplosionTypes
	XwaVector3 Span;
	XwaVector3 Center;
	XwaVector3 Min;
	XwaVector3 Max;
	DWORD TargetId;
	XwaVector3 Target;
};

struct SceneCompData
{
	XwaObject* pObject;
	float RotationAngle;
	XwaTransform WorldViewTransform;
	XwaTransform WorldViewTransformTransposed;
	XwaVector3 SceneCompData_m068;
	int SceneCompData_m074; // Always 0
	int VerticesCount; // Matches data in OPT file
	XwaVector3* MeshVertices;
	XwaTextureVertex* MeshTextureVertices;
	XwaVector3* MeshVertexNormals;
	int SceneCompData_m088; // Always 0
	int FacesCount; // Matches data in OPT file
	int EdgesCount; // Matches data in OPT file
	XwaVector3* FaceNormals;
	XwaVector3* FaceTexturingInfos;
	int SceneCompData_m09C; // Always 0
	OptFaceDataNode_01_Data_Indices* FaceIndices;
	const char* TextureName; // Always 0
	XwaTextureDescription* TextureDescription;
	unsigned char* pTextureData;
	unsigned short* TexturePalettes;
	unsigned int TextureAlphaMask;
	XwaD3DInfo* D3DInfo;
	int FaceListIndex; // Always 0
	int VertListIndex; // Always 0
	int EdgeListIndex; // Always 0
	int FaceListCount; // Always 0
	int VertListCount; // Always 0
	int EdgeListCount; // Always 0
	MeshDescriptor *pMeshDescriptor;
	int GlowMarksCount;
	XwaGlowMark* GlowMarks[16];
};

static_assert(sizeof(SceneCompData) == 284, "size of SceneCompData must be 284");

/*
struct SceneFaceList
{
	int FaceIndex;
	int Id;
	SceneCompData* pSceneCompData;
	int SceneFaceList_m0C;
	int SceneFaceList_m10;
	float TexturingDirectionX;
	float TexturingDirectionY;
	float TexturingDirectionZ;
	float TexturingMagnitureX;
	float TexturingMagnitureY;
	float TexturingMagnitureZ;
	float SceneFaceList_m2C;
	float SceneFaceList_m30;
	float SceneFaceList_m34;
	float SceneFaceList_m38;
	int SceneFaceList_m3C;
	int pPhongData;
	int SceneFaceList_m44;
	int SceneFaceList_m48;
	float SceneFaceList_m4C;
	float SceneFaceList_m50;
	int pEdgeLists[4];
	char unk64[4];
	int SceneFaceList_m68;
	int SceneFaceList_m6C;
	int SceneFaceList_m70;
};

static_assert(sizeof(SceneFaceList) == 116, "size of SceneFaceList must be 116");
*/

struct SceneVertList
{
	XwaVector3 Position;
	float ColorIntensity;
	float ColorA;
	float ColorR;
	float ColorG;
	float ColorB;
	float TextureU;
	float TextureV;
	XwaTextureVertex TextureVertices[16];
	int TextureVerticesCount;
};

static_assert(sizeof(SceneVertList) == 172, "size of SceneVertList must be 172");

struct XwaGlobalLight
{
	int PositionX;
	int PositionY;
	int PositionZ;
	float DirectionX;
	float DirectionY;
	float DirectionZ;
	float Intensity;
	float m1C;
	float ColorR;
	float ColorB;
	float ColorG;
	float BlendStartIntensity;
	float BlendStartColor1C;
	float BlendStartColorR;
	float BlendStartColorB;
	float BlendStartColorG;
	float BlendEndIntensity;
	float BlendEndColor1C;
	float BlendEndColorR;
	float BlendEndColorB;
	float BlendEndColorG;
};

static_assert(sizeof(XwaGlobalLight) == 84, "size of XwaGlobalLight must be 84");

struct XwaLocalLight
{
	int iPositionX;
	int iPositionY;
	int iPositionZ;
	float fPositionX;
	float fPositionY;
	float fPositionZ;
	float m18;
	int m1C;
	float m20;
	float ColorR;
	float ColorB;
	float ColorG;
};

static_assert(sizeof(XwaLocalLight) == 48, "size of XwaLocalLight must be 48");

struct OptNode
{
	const char* Name;
	OptNodeEnum NodeType;
	int NumOfNodes;
	OptNode** Nodes;
	int Parameter1;
	int Parameter2;
};

static_assert(sizeof(OptNode) == 24, "size of OptNode must be 24");

struct OptHeader
{
	int GlobalOffset;
	char unk04[2];
	int NumOfNodes;
	OptNode** Nodes;
};

static_assert(sizeof(OptHeader) == 14, "size of OptHeader must be 14");

#pragma pack(pop)
