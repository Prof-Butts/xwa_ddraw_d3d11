#pragma once

#include "common.h"
#include <vector>
#include <map>
#include "XWAFramework.h"
#include "EffectsCommon.h"
#include "DynamicCockpit.h"
#include "ActiveCockpit.h"
#include "ShadowMapping.h"
#include "Reticle.h"
#include "Materials.h"

// SSAO Type
typedef enum {
	SSO_AMBIENT,
	SSO_DIRECTIONAL,
	SSO_BENT_NORMALS,
	SSO_DEFERRED,
	SSO_PBR,
} SSAOTypeEnum;

// In order to blend the background with the hyperspace effect when exiting, we need to extend the
// effect for a few more frames. To do that, we need to track the current state of the effect and
// that's why we need a small state machine:
enum HyperspacePhaseEnum {
	HS_INIT_ST = 0,				// Initial state, we're not even in Hyperspace
	HS_HYPER_ENTER_ST = 1,		// We're entering hyperspace
	HS_HYPER_TUNNEL_ST = 2,		// Traveling through the blue Hyperspace tunnel
	HS_HYPER_EXIT_ST = 3,		// HyperExit streaks are being rendered
	HS_POST_HYPER_EXIT_ST = 4   // HyperExit streaks have finished rendering; but now we're blending with the backround
};
const int MAX_POST_HYPER_EXIT_FRAMES = 10; // I had 20 here up to version 1.1.1. Making this smaller makes the zoom faster
const int HYPER_INTERDICTION_STYLE = 2;

// xwahacker computes the FOV like this: FOV = 2.0 * atan(height/focal_length). This formula is questionable, the actual
// FOV seems to be: 2.0 * atan((height/2)/focal_length), same for the horizontal FOV. I confirmed this by geometry
// and by computing the angle between the lights and the current forward point.
// Data provided by keiranhalcyon7:
inline uint32_t* g_rawFOVDist = (uint32_t*)0x91AB6C; // raw FOV dist(dword int), copy of one of the six values hard-coded with the resolution slots, which are what xwahacker edits
inline float* g_fRawFOVDist = (float*)0x8B94CC; // FOV dist(float), same value as above
inline float* g_cachedFOVDist = (float*)0x8B94BC; // cached FOV dist / 512.0 (float), seems to be used for some sprite processing
inline float g_fDefaultFOVDist = 1280.0f; // Original FOV dist

extern std::vector<char*> Text_ResNames;
extern std::vector<char*> Floating_GUI_ResNames;
extern std::vector<char*> LaserIonEnergy_ResNames;
// List of regular GUI elements (this is not an exhaustive list). It's mostly used to detect when
// the game has started rendering the GUI
extern std::vector<char*> GUI_ResNames;
// List of common roots for the electricity names
extern std::vector<char*> Electricity_ResNames;
// List of common roots for the Explosion names
extern std::vector<char*>Explosion_ResNames;
// Smoke from explosions:
extern std::vector<char*> Smoke_ResNames;
extern std::vector<char*> Sparks_ResNames;
extern std::vector<char*> HitEffects_ResNames;
// List of Lens Flare effects
extern std::vector<char*> LensFlare_ResNames;
// List of Suns in the Backdrop.dat file
extern std::vector<char*> Sun_ResNames;
extern std::vector<char*> SpaceDebris_ResNames;
extern std::vector<char*> Trails_ResNames;

#define MAX_SPEED_PARTICLES 256
extern Vector4 g_SpeedParticles[MAX_SPEED_PARTICLES];

// Constant Buffers
extern VertexShaderMatrixCB     g_VSMatrixCB;
extern BloomPixelShaderCBuffer  g_BloomPSCBuffer;
extern SSAOPixelShaderCBuffer   g_SSAO_PSCBuffer;
extern PSShadingSystemCB        g_ShadingSys_PSBuffer;
extern ShadertoyCBuffer         g_ShadertoyBuffer;
extern LaserPointerCBuffer      g_LaserPointerBuffer;
extern OPTMeshTransformCBuffer  g_OPTMeshTransformCB;
extern RTConstantsBuffer        g_RTConstantsBuffer;
extern VRGeometryCBuffer        g_VRGeometryCBuffer;

extern D3DTLVERTEX* g_OrigVerts;
extern uint32_t* g_OrigIndex;

// SteamVR
extern DWORD g_FullScreenWidth, g_FullScreenHeight;
extern Matrix4 g_viewMatrix;
extern Matrix4 g_ReflRotX;

extern bool g_bStart3DCapture, g_bDo3DCapture;
extern FILE* g_HackFile;
#ifdef DBR_VR
extern bool g_bCapture2DOffscreenBuffer;
#endif

extern bool g_b3DSunPresent, g_b3DSkydomePresent;

// BLOOM
extern bool /* g_bDumpBloomBuffers, */ g_bDCManualActivate;
extern BloomConfig g_BloomConfig;

// LASER LIGHTS AND DYNAMIC LIGHTS
extern SmallestK g_LaserList;
extern bool g_bEnableLaserLights, g_bEnableExplosionLights, g_bEnableHeadLights;
extern Vector3 g_LaserPointDebug;
extern Vector3 g_HeadLightsPosition, g_HeadLightsColor;
extern float g_fHeadLightsAmbient, g_fHeadLightsDistance, g_fHeadLightsAngleCos;
extern bool g_bHeadLightsAutoTurnOn;
extern bool g_bCurrentMissionUsesHeadLights;
extern const float DEFAULT_DYNAMIC_LIGHT_FALLOFF;
extern const Vector3 DEFAULT_EXPLOSION_COLOR;

extern int   g_iDraw2DCounter;
extern bool g_bPrevPlayerInHangar;
extern bool g_bInTechRoom;
extern bool g_bMetricParamsNeedReapply;

extern Vector3 g_CockpitPOVOffset;
extern Vector3 g_GunnerTurretPOVOffset;

bool isInVector(uint32_t crc, std::vector<uint32_t>& vector);
bool isInVector(char* name, std::vector<char*>& vector);
bool isInVector(char* OPTname, std::vector<OPTNameType>& vector);
void DisplayCoords(LPD3DINSTRUCTION instruction, UINT curIndex);
void InGameToScreenCoords(UINT left, UINT top, UINT width, UINT height, float x, float y, float* x_out, float* y_out);
void ScreenCoordsToInGame(float left, float top, float width, float height, float x, float y, float* x_out, float* y_out);
void GetScreenLimitsInUVCoords(float* x0, float* y0, float* x1, float* y1, bool UseNonVR = false);

bool UpdateXWAHackerFOV();
void CycleFOVSetting();
float ComputeRealVertFOV();
float ComputeRealHorzFOV();
float RealVertFOVToRawFocalLength(float real_FOV);

// ********************************
// Raytracing
// Maps face group index --> numTris in face group
using FaceGroups = std::map<int32_t, int32_t>;

// Each OPT is made of multiple meshes.
// Each mesh is made of multiple LODs.
// Each LOD is made of multiple Face Groups.
// From our perspective, inside ddraw, we see a draw call per face group
// but each face group references the whole set of vertices from the mesh.
// The only way to reconstruct the original faces is by accumulating all
// face groups that reference the same mesh -- and even then, we may only
// see the faces that make a specific LOD, leaving several "unused" vertices.
// That's why we need to group all the face groups that belong to the same
// mesh. We do that with MeshData.
// g_LBVHMap maps a mesh (vertex pointer) to a MeshData. That's how we can reconstruct
// a mesh as much as it's possible from this perspective (we might be missing LODs)
// If an LBVH pointer is NULL and we're not in the Tech Room, then that means this
// mesh needs to have its BVH rebuilt.
//
// (FaceGroup map, NumMeshVertices, LBVH, BaseNodeOffset)
//
// 0: FaceGroup std::map -- A map with all the Face Groups in this mesh
// 1: NumMeshVertices    -- The number of vertices in this mesh
// 2: LBVH               -- The BVH for this mesh (only used during regular flight)
// 3: BaseNodeOffset     -- The index into _RTBvh where this BLAS begins (only used during regular flight)
using MeshData = std::tuple<FaceGroups, int32_t, void*, int>;

inline FaceGroups& GetFaceGroups(MeshData& X) { return std::get<0>(X); }
inline int32_t& GetNumMeshVertices(MeshData& X) { return std::get<1>(X); }
inline void*& GetLBVH(MeshData& X) { return std::get<2>(X); } // <-- This is probably not used anymore
inline int& GetBaseNodeOffset(MeshData& X) { return std::get<3>(X); } // <-- This is probably not used anymore

// (FaceGroup map, NumMeshVertices, LBVH, BaseNodeOffset, MeshVerticesPtr)
// BLASData is now a superset of MeshData. At some point, I may be able to replace the latter.
// 0: FaceGroupMap     -- An std::map with all the Face Groups in this mesh
// 1: NumMeshVertices  -- The number of vertices in this mesh
// 2: LBVH             -- The BVH for this mesh (only used during regular flight)
// 3: BaseNodeOffset   -- The index into _RTBvh where this BLAS begins (only used during regular flight)
// 4: MeshVerticesPtr  -- Pointer to the MeshVertices
using BLASData = std::tuple<FaceGroups, int32_t, void*, int32_t, int32_t>;

inline FaceGroups& BLASGetFaceGroups(BLASData& X) { return std::get<0>(X); }
inline int32_t&    BLASGetNumVertices(BLASData& X) { return std::get<1>(X); }
inline void *&     BLASGetBVH(BLASData& X) { return std::get<2>(X); }
inline int32_t&    BLASGetBaseNodeOffset(BLASData& X) { return std::get<3>(X); }
inline int32_t&    BLASGetMeshVertices(BLASData& X) { return std::get<4>(X); }

// BLAS Key: <MeshKey, LOD>. This tuple can be used to uniquely identify a BLAS entry.
// There's one BLAS per mesh per LOD.
// - MeshKey is scene->MeshVertices
// - LOD is the LOD's index. We can get the LOD from its FaceGroup.
using BLASKey_t = std::tuple<int, int>;
// Map of unique IDs for BLASes. (MeshKey, LOD) --> BlasId. Gets cleared on OnSizeChanged()
extern std::map<BLASKey_t, int> g_BLASIdMap;

// TLAS leaf uniqueness is determined by the BlasID and its centroid.
// BlasId, x, y, z:
using IDCentroid_t = std::tuple<int32_t, float, float, float>;

// The {Single|Coalesced} BLAS map: meshKey --> MeshData (only used in the Tech Room)
extern std::map<int32_t, MeshData> g_LBVHMap;
// The Multiple BLAS map: BlasId --> BLASData. Gets cleared on OnSizeChanged().
extern std::map<int, BLASData> g_BLASMap;
// The TLAS map: (BlasId, centroid) --> matrixSlot. Gets cleared at the beginning of each frame.
extern std::map<IDCentroid_t, int32_t> g_TLASMap;
// The TLAS matrix buffer (1:1 correspondence with g_TLASMap). The matrix slot index is reset each frame.
extern std::vector<Matrix4> g_TLASMatrices;
#undef DEBUG_RT
#ifdef DEBUG_RT
// DEBUG only
extern std::map<int32_t, std::tuple<std::string, int, int>> g_DebugMeshToNameMap;
#endif

/// <summary>
/// Maps GroupId-ImageId keys to Direct3DTexture pointers.
/// Used to associate Backdrop Ids with GroupId-ImageId pairs.
/// </summary>
extern std::map<int, void*> g_GroupIdImageIdToTextureMap;
namespace STARFIELD_TYPE
{
	enum STARFIELD_TYPE
	{
		TOP = 0,
		BOTTOM,
		FRONT,
		BACK,
		LEFT,
		RIGHT,
		MAX
	};
};
extern Direct3DTexture* g_StarfieldSRVs[STARFIELD_TYPE::MAX];

// ********************************
// DATReader function pointers
constexpr uint32_t DAT_READER_VERSION_101 = 65537; // 1.0.1 -- default version
extern uint32_t g_DATReaderVersion;
typedef uint32_t(_cdecl * GetDATReaderVersionFun)();
typedef void(_cdecl * SetDATVerbosityFun)(bool Verbose);
typedef bool(_cdecl * LoadDATFileFun)(const char *sDatFileName);
typedef bool(_cdecl * GetDATImageMetadataFun)(int GroupId, int ImageId, short *Width_out, short *Height_out, uint8_t *Format_out);
typedef bool(_cdecl * ReadDATImageDataFun)(uint8_t *RawData_out, int RawData_size);
typedef bool(_cdecl* ReadFlippedDATImageDataFun)(uint8_t* RawData_out, int RawData_size);
typedef int(_cdecl * GetDATGroupImageCountFun)(int GroupId);
typedef bool(_cdecl * GetDATGroupImageListFun)(int GroupId, short *ImageIds_out, int ImageIds_size);
typedef int(_cdecl* GetDATGroupCountFun)();
typedef bool(_cdecl* GetDATGroupListFun)(short* GroupIds_out);

extern LoadDATFileFun LoadDATFile;
extern GetDATImageMetadataFun GetDATImageMetadata;
extern ReadFlippedDATImageDataFun ReadFlippedDATImageData;
extern ReadDATImageDataFun ReadDATImageData;
extern SetDATVerbosityFun SetDATVerbosity;
extern GetDATGroupImageCountFun GetDATGroupImageCount;
extern GetDATGroupImageListFun GetDATGroupImageList;
extern GetDATGroupCountFun GetDATGroupCount;
extern GetDATGroupListFun GetDATGroupList;

bool InitDATReader();
void CloseDATReader();
std::vector<short> ReadDATImageListFromGroup(const char *sDATFileName, int GroupId);
// ********************************

// ********************************
// ZIPReader function pointers
typedef void(_cdecl * SetZIPVerbosityFun)(bool Verbose);
typedef bool(_cdecl * LoadZIPFileFun)(const char *sZIPFileName);
typedef int(_cdecl * GetZIPGroupImageCountFun)(int GroupId);
typedef bool(_cdecl * GetZIPGroupImageListFun)(int GroupId, short *ImageIds_out, int ImageIds_size);
typedef void(_cdecl * DeleteAllTempZIPDirectoriesFun)();
typedef bool(_cdecl * GetZIPImageMetadataFun)(
	int GroupId, int ImageId, short *Width_out, short *Height_out, char **ImagePath_out, int *ImagePathSize);

extern LoadZIPFileFun LoadZIPFile;
extern SetZIPVerbosityFun SetZIPVerbosity;
extern GetZIPGroupImageCountFun GetZIPGroupImageCount;
extern GetZIPGroupImageListFun GetZIPGroupImageList;
extern DeleteAllTempZIPDirectoriesFun DeleteAllTempZIPDirectories;
extern GetZIPImageMetadataFun GetZIPImageMetadata;

bool IsZIPReaderLoaded();
bool InitZIPReader();
void CloseZIPReader();
std::vector<short> ReadZIPImageListFromGroup(const char *sZIPFileName, int GroupId);
// ********************************

CraftInstance *GetPlayerCraftInstanceSafe();
CraftInstance *GetPlayerCraftInstanceSafe(ObjectEntry** object);
CraftInstance* GetPlayerCraftInstanceSafe(ObjectEntry** object, MobileObjectEntry** mobileObject);
// Also updates g_bInTechGlobe when called.
bool InTechGlobe();
bool InBriefingRoom();
