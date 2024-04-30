#include "common.h"
#include "EffectsRenderer.h"
#include <algorithm>

// DEBUG vars
int g_iD3DExecuteCounter = 0, g_iD3DExecuteCounterSkipHi = -1, g_iD3DExecuteCounterSkipLo = -1;

// Control vars
bool g_bEnableAnimations = true;
extern bool g_bKeepMouseInsideWindow;
extern char g_curOPTLoaded[MAX_OPT_NAME];

// SkyCylinder:
constexpr float BACKGROUND_CUBE_SIZE_METERS = 1000.0f;
constexpr float BACKGROUND_CUBE_HALFSIZE_METERS = BACKGROUND_CUBE_SIZE_METERS / 2.0f;
constexpr float BACKGROUND_CYL_RATIO = 1.09f; // 1.053f ? 1.375f ?
static constexpr int s_numCylTriangles = 12;

// Raytracing
extern bool g_bEnableQBVHwSAH;
//BVHBuilderType g_BVHBuilderType = BVHBuilderType_BVH2;
//BVHBuilderType g_BVHBuilderType = BVHBuilderType_QBVH;
//BVHBuilderType g_BVHBuilderType = BVHBuilderType_FastQBVH;
//BVHBuilderType g_BVHBuilderType = BVHBuilderType_Embree;
BLASBuilderType g_BLASBuilderType = DEFAULT_BLAS_BUILDER;
TLASBuilderType g_TLASBuilderType = DEFAULT_TLAS_BUILDER;
// DEBUG
extern LONG g_directBuilderNextInnerNode;

bool g_bUseCentroids = true;

static bool s_captureProjectionDeltas = true;
float g_f0x08C1600 = NAN, g_f0x0686ACC = NAN;
float g_f0x080ACF8 = NAN, g_f0x07B33C0 = NAN, g_f0x064D1AC = NAN;

RTCDevice g_rtcDevice = nullptr;
RTCScene g_rtcScene = nullptr;

char* g_sBLASBuilderTypeNames[(int)BLASBuilderType::MAX] = {
	"      BVH2",
	"      QBVH",
	"  FastQBVH",
	//"    Embree",
	//"DirectBVH2CPU",
	"DirectBVH4",
	"    Online",
	"  OnlinePQ",
	"      PLOC",
};

extern char* g_sTLASBuilderTypeNames[(int)TLASBuilderType::MAX] = {
	"  FastQBVH",
	"DirectBVH4"
};

bool g_bRTEnabledInTechRoom = true;
bool g_bRTEnabled = false; // In-flight RT switch.
bool g_bRTEnabledInCockpit = false;
bool g_bRTEnableSoftShadows = false;
bool g_bRTEnableEmbree = false;
// Used for in-flight RT, to create the BVH buffer that will store all the
// individual BLASes needed for the current frame.
int g_iRTTotalBLASNodesInFrame = 0;
int g_iRTMaxBLASNodesSoFar = 0;
int g_iRTMaxTLASNodesSoFar = 0;
// Values between 0.05 and 0.35 seem to work fine. These values were determined
// empirically.
float g_fRTSoftShadowThresholdMult = 0.05f;
// GaussFactor: 0.1 -- Soft shadow
// GaussFactor: 1.5 -- Sharp shadow
float g_fRTGaussFactor = 0.5f;

uint32_t g_iRTMaxMeshesSoFar = 0;
int g_iRTMeshesInThisFrame = 0;

int g_iRTMatricesNextSlot = 0;
int g_iRTNextBLASId = 0;
bool g_bRTReAllocateBvhBuffer = false;
AABB g_CameraAABB; // AABB built from the camera's frustrum
AABB g_GlobalAABB; // AABB built after all the meshes have been seen in the current frame
AABB g_GlobalCentroidAABB; // The DirectBVH4 builder needs this AABB to compute the splits.
XwaVector3 g_CameraRange;
XwaVector3 g_GlobalRange;
LBVH* g_TLASTree = nullptr;
LBVH* g_ACTLASTree = nullptr;
std::vector<TLASLeafItem> tlasLeaves;
std::vector<TLASLeafItem> g_ACtlasLeaves; // Active Cockpit TLAS leaves

std::map<std::string, bool> g_RTExcludeOPTNames;
std::map<uint8_t, bool> g_RTExcludeShipCategories;
std::map<int, bool> g_RTExcludeMeshes;

// Maps an ObjectId to its index in the ObjectEntry table.
// Textures have an associated objectId, this map tells us the slot
// in the objects array where we'll find the corresponding objectId.
std::map<int, int> g_objectIdToIndex;
// The new per-craft events will only store hull events (and maybe sys/disabled events
// later), but we need a previous and current event, and a timer (for animations).

// Maps an ObjectId to its InstanceEvent
std::map<uint64_t, InstanceEvent> g_objectIdToInstanceEvent;
// Maps an ObjectId-MaterialId to its fixed data struct. The fixed data is
// used to randomize the location and scale of the damage texture (and probably
// other stuff in the future)
std::map<uint64_t, FixedInstanceData> g_fixedInstanceDataMap;

VRGlovesMesh g_vrGlovesMeshes[2];

EffectsRenderer *g_effects_renderer = nullptr;

std::vector<BracketVR> g_bracketsVR;

// Current turn rate. This will make the ship turn faster at 1/3 throttle.
// This variable makes a smooth transition when the throttle changes.
float g_fTurnRateScale = 1.0f;
// The player's current yaw, pitch, roll rate expressed in degrees
// These values do *not* represent the current heading and they are used to
// do a smooth interpolation between the desired ypr rate and the current ypr rate.
float CurPlayerYawRateDeg = 0, CurPlayerPitchRateDeg = 0, CurPlayerRollRateDeg = 0;

//float g_HMDYaw = 0, g_HMDPitch = 0, g_HMDRoll = 0;

float lerp(float x, float y, float s);

inline float clamp(float val, float min, float max)
{
	if (val < min) val = min;
	if (val > max) val = max;
	return val;
}

inline float sign(float val)
{
	return (val >= 0.0f) ? 1.0f : -1.0f;
}

void InitializePlayerYawPitchRoll();
void ApplyYawPitchRoll(float yaw_deg, float pitch_deg, float roll_deg);
Matrix4 GetSimpleDirectionMatrix(Vector4 Fs, bool invert);
void ClearGlobalLBVHMap();

void VRControllerToOPTCoords(Vector4 contOrigin[2], Vector4 contDir[2]);
Intersection getIntersection(Ray ray, float3 A, float3 B, float3 C);
bool RayTriangleTest(const Intersection& inters);
bool rayTriangleIntersect(
	const Vector3& orig, const Vector3& dir,
	const Vector3& v0, const Vector3& v1, const Vector3& v2,
	float& t, Vector3& P, float& u, float& v, float margin);
float ClosestPointOnTriangle(
	const Vector3& orig, const Vector3& v0, const Vector3& v1, const Vector3& v2,
	Vector3& P, float& u, float& v, float margin);
Intersection _TraceRaySimpleHit(BVHNode* g_BVH, Ray ray, int Offset);
Intersection ClosestHit(BVHNode* g_BVH, float3 origin, int Offset, float3& P_out,
	ac_uv_coords* coords, int contIdx);

Vector4 SteamVRToOPTCoords(Vector4 P);
void SetLights(DeviceResources* resources, float fSSDOEnabled);

inline int MakeKeyFromGroupIdImageId(int groupId, int imageId)
{
	return (groupId << 16) | (imageId);
}

//#define DUMP_TLAS 1
#undef DUMP_TLAS
#ifdef DUMP_TLAS
static FILE* g_TLASFile = NULL;
#endif

/// <summary>
/// Returns scene->MeshVertices
/// </summary>
int32_t MakeMeshKey(const SceneCompData* scene)
{
	return (int32_t)scene->MeshVertices;
}

/// <summary>
/// Returns scene->FaceIndices
/// </summary>
int32_t MakeFaceGroupKey(const SceneCompData* scene)
{
	return (int32_t)scene->FaceIndices;
}

void RTResetMatrixSlotCounter()
{
	g_iRTMatricesNextSlot = 0;
}

int RTGetNextAvailableMatrixSlot()
{
	g_iRTMatricesNextSlot++;
	return g_iRTMatricesNextSlot - 1;
}

void RTResetBlasIDs()
{
	g_iRTNextBLASId = 0;
}

int RTGetNextBlasID()
{
	g_iRTNextBLASId++;
	return g_iRTNextBLASId - 1;
}

Matrix4 GetBLASMatrix(TLASLeafItem& tlasLeaf, int *matrixSlot)
{
	*matrixSlot = TLASGetMatrixSlot(tlasLeaf);
	return g_TLASMatrices[*matrixSlot];
}

void ClearGroupIdImageIdToTextureMap()
{
	g_GroupIdImageIdToTextureMap.clear();
	for (int i = 0; i < STARFIELD_TYPE::MAX; i++)
		g_StarfieldSRVs[i] = nullptr;
}

float4 TransformProjection(float3 input)
{
	float vpScaleX = g_VSCBuffer.viewportScale[0];
	float vpScaleY = g_VSCBuffer.viewportScale[1];
	float vpScaleZ = g_VSCBuffer.viewportScale[2];
	float Znear = *(float*)0x08B94CC; // Znear
	float Zfar  = *(float*)0x05B46B4; // Zfar
	float projectionDeltaX = *(float*)0x08C1600 + *(float*)0x0686ACC;
	float projectionDeltaY = *(float*)0x080ACF8 + *(float*)0x07B33C0 + *(float*)0x064D1AC;

	float4 pos;
	// st0 = Znear / input.z == pos.w
	float st0 = Znear / input.z; // st0 = Znear / MetricZ
	pos.x = input.x * st0 + projectionDeltaX;
	pos.y = input.y * st0 + projectionDeltaY;
	// DEPTH-BUFFER-CHANGE DONE
	// pos.z = (st0 * Zfar/32) / (abs(st0) * Zfar/32 + Znear/3) * 0.5
	//pos.z = (st0 * Zfar / 32) / (abs(st0) * Zfar / 32 + Znear / 3) * 0.5f;
	pos.z = (st0 * Zfar / g_config.ProjectionParameterA) / (abs(st0) * Zfar / g_config.ProjectionParameterB + Znear * g_config.ProjectionParameterC);
	pos.w = 1.0f;
	pos.x = (pos.x * vpScaleX - 1.0f) * vpScaleZ;
	pos.y = (pos.y * vpScaleY + 1.0f) * vpScaleZ;
	// We previously did pos.w = 1. After the next line, pos.w = 1 / st0, that implies
	// that pos.w == rhw and st0 == w
	pos.x *= 1.0f / st0;
	pos.y *= 1.0f / st0;
	pos.z *= 1.0f / st0;
	pos.w *= 1.0f / st0;

	return pos;
}

/// <summary>
/// Same as TransformProjection(), but returns in-game screen coordinates + depth buffer
/// Coordinate system is OPT-scale:
/// X+ Right
/// Y+ Up
/// Z+ Forwards
/// </summary>
float4 TransformProjectionScreen(float3 input)
{
	float4 pos = TransformProjection(input);
	float w = pos.w;
	// Apply the division by w that DirectX does implicitly
	pos.x = pos.x / w;
	pos.y = pos.y / w;
	//pos.z = pos.z / w;
	//pos.w = 1.0f;
	// pos.xy is now in the range -1..1, convert to screen coords
	// TODO: What should I do with vpScaleZ?
	pos.x = (pos.x + 1.0f) / g_VSCBuffer.viewportScale[0];
	pos.y = (pos.y - 1.0f) / g_VSCBuffer.viewportScale[1];
	return pos;
}

/*
 * Back-projects a 2D point in normalized DirectX coordinates (-1..1) into an OPT-scale 3D point
 * This function is probably not up-to-date, use InverseTransformProjectionScreen() instead.
 */
float3 InverseTransformProjection(float4 input)
{
	float3 pos;
	float vpScaleX = g_VSCBuffer.viewportScale[0];
	float vpScaleY = g_VSCBuffer.viewportScale[1];
	float vpScaleZ = g_VSCBuffer.viewportScale[2];
	float projectionValue1 = *(float*)0x08B94CC; // Znear
	//float projectionValue2 = *(float*)0x05B46B4; // Zfar
	float projectionDeltaX = *(float*)0x08C1600 + *(float*)0x0686ACC;
	float projectionDeltaY = *(float*)0x080ACF8 + *(float*)0x07B33C0 + *(float*)0x064D1AC;

	float st0 = 1.0f / input.w;
	// input.w = 1 / st0 = pos.z / projectionValue1
	// input.w = pos.z / projectionValue1
	// input.w * projectionValue1 = pos.z
	pos.z = projectionValue1 * input.w;
	// pos.z is now OPT-scale

	// Reverse the premultiplication by w:
	pos.x = input.x * st0;
	pos.y = input.y * st0;

	// Convert from screen coords to normalized coords
	pos.x = (pos.x / vpScaleZ + 1.0f) / vpScaleX;
	pos.y = (pos.y / vpScaleZ - 1.0f) / vpScaleY;

	pos.x = (pos.x - projectionDeltaX) / st0;
	pos.y = (pos.y - projectionDeltaY) / st0;
	// input.xyz is now OPT-scale

	return pos;
}

/*
 * Converts 2D into metric 3D at OPT-scale (you get 100% metric 3D by multiplying with OPT_TO_METERS after this)
 * The formulas used here work with the engine glow and with the explosions because w == z in that case.
 * It may not work in other cases.
 *
 * Confirmed cases: Engine glow, Explosions.
 *
 * NOTE 1: This is not the direct inverse of TransformProjection anymore (there's g_config settings that have yet
 * to be ported over here).
 * NOTE 2: The OPT universe is centered around the HUD. If the input coords are the center of the HUD, then the
 * resulting OPT value will be (0, 0, depth). The center of the HUD is usually not the same as the screen center!
 * NOTE 3: In external view, the HUD may not be centered on the screen if inertia is enabled, but the center of the
 * screen corresponds to (0, 0, depth)
 * NOTE 4: The output is OPT scale but the axes are... unconventional:
 *
 * X+: Right
 * Y+: DOWN
 * Z+: FORWARDS
 */
float3 InverseTransformProjectionScreen(float4 input)
{
	// DEPTH-BUFFER-CHANGE DONE
	float Znear = *(float *)0x08B94CC;
	float Zfar  = *(float *)0x05B46B4;
	float projectionDeltaX = *(float*)0x08C1600 + *(float*)0x0686ACC;
	float projectionDeltaY = *(float*)0x080ACF8 + *(float*)0x07B33C0 + *(float*)0x064D1AC;
	//float st0 = input.w;
	// st0 = Znear / MetricZ == pos.w

	// depthZ = (st0 * Zfar / 32.0f) / (abs(st0) * Zfar / 32.0f + Znear / 3.0f) * 0.5f;
	// depthZ * 2.0f = (st0 * Zfar / 32.0f) / (abs(st0) * Zfar / 32.0f + Znear / 3.0f);
	// depthZ * 2.0f * (abs(st0) * Zfar / 32.0f + Znear / 3.0f) = st0 * Zfar / 32.0f;
	// depthZ * 2.0f * (abs(st0) * Zfar / 32.0f + Znear / 3.0f) * 32.0f = st0 * Zfar;
	// depthZ * 2.0f * (abs(st0) * Zfar / 32.0f + Znear / 3.0f) * 32.0f / Zfar = st0;
	// st0 = depthZ * 2.0f * (abs(st0) * Zfar / 32.0f + Znear / 3.0f) * 32.0f / Zfar;

	// The engine glow has z = w!!! So we must also do:
	// st0 = Znear / (Zfar / input.pos.w - Zfar);
	// Znear / st0 = Zfar / input.pos.w - Zfar
	// Znear / st0 + Zfar = Zfar / input.pos.w
	// Zfar / (Znear / st0 + Zfar) = input.pos.w
	// input.pos.w = Zfar / (Znear / st0 + Zfar)

	// Znear / MetricZ = depthZ * 2.0f * (abs(st0) * Zfar / 32.0f + Znear / 3.0f) * 32.0f / Zfar;
	// MetricZ = Znear / (depthZ * 2.0f * (abs(st0) * Zfar / 32.0f + Znear / 3.0f) * 32.0f / Zfar);

	float vpScaleX = g_VSCBuffer.viewportScale[0];
	float vpScaleY = g_VSCBuffer.viewportScale[1];
	float vpScaleZ = g_VSCBuffer.viewportScale[2];
	// input.xy is in screen coords (0,0)-(W,H), convert to normalized DirectX: -1..1
	input.x = (input.x * vpScaleX) - 1.0f;
	input.y = (input.y * vpScaleY) + 1.0f;
	// input.xy is now in the range -1..1, invert the formulas in TransformProjection:
	input.x = (input.x / vpScaleZ + 1.0f) / vpScaleX;
	input.y = (input.y / vpScaleZ - 1.0f) / vpScaleY;

	/*
	The next step, in TransformProjection() is to recover metric Z from sz (depthZ).
	However, in this case we must make a detour. From the VertexShader, we see this:

	if (input.pos.z == input.pos.w)
	{
		float z = s_V0x05B46B4 / input.pos.w - s_V0x05B46B4;
		st0 = s_V0x08B94CC / z;
	}

	The engine glow has w == z, so we must use those formulas. Also, that temporary z is the
	Metric Z we're looking for, so the formula is straightforward now:
	*/
	float3 P;
	if (input.z == input.w)
	{
		P.z = Zfar / input.w - Zfar; // Tech Room version, also provides better Z for engine glows
		//P.z = Zfar / input.w; // Old version
	}
	else
	{
		// For the regular case, when w != z, we can probably just invert st0 from TransformProjection():
		P.z = Znear / input.w;
		//P.z = Znear * input.w;
		//P.z = input.w;
	}

	// We can now continue inverting the formulas in TransformProjection
	float st0 = Znear / P.z;

	// Continue inverting the formulas in TransformProjection:
	// input.x = P.x * st0 + projectionDeltaX;
	// input.y = P.y * st0 + projectionDeltaY;
	P.x = (input.x - projectionDeltaX) / st0;
	P.y = (input.y - projectionDeltaY) / st0;
	// P is now metric * OPT-scale
	return P;
}


void IncreaseD3DExecuteCounterSkipHi(int Delta) {
	g_iD3DExecuteCounterSkipHi += Delta;
	if (g_iD3DExecuteCounterSkipHi < -1)
		g_iD3DExecuteCounterSkipHi = -1;
	log_debug("[DBG] g_iD3DExecuteCounterSkip, Lo: %d, Hi: %d", g_iD3DExecuteCounterSkipLo, g_iD3DExecuteCounterSkipHi);
}

void IncreaseD3DExecuteCounterSkipLo(int Delta) {
	g_iD3DExecuteCounterSkipLo += Delta;
	if (g_iD3DExecuteCounterSkipLo < -1)
		g_iD3DExecuteCounterSkipLo = -1;
	log_debug("[DBG] g_iD3DExecuteCounterSkip, Lo: %d, Hi: %d", g_iD3DExecuteCounterSkipLo, g_iD3DExecuteCounterSkipHi);
}

void ResetObjectIndexMap() {
	g_objectIdToIndex.clear();
	g_objectIdToInstanceEvent.clear();
	g_fixedInstanceDataMap.clear();
}

// ****************************************************
// Dump to OBJ
// ****************************************************
// Set the following flag to true to enable dumping the current scene to an OBJ file
bool bD3DDumpOBJEnabled = true;
bool bHangarDumpOBJEnabled = false;
FILE *D3DDumpOBJFile = NULL, *D3DDumpLaserOBJFile = NULL;
int D3DOBJFileIdx = 0, D3DTotalVertices = 0, D3DTotalNormals = 0, D3DOBJGroup = 0;
int D3DOBJLaserFileIdx = 0, D3DTotalLaserVertices = 0, D3DTotalLaserTextureVertices = 0, D3DOBJLaserGroup = 0;

void OBJDump(XwaVector3 *vertices, int count)
{
	static int obj_idx = 1;
	log_debug("[DBG] OBJDump, count: %d, obj_idx: %d", count, obj_idx);

	fprintf(D3DDumpOBJFile, "o obj-%d\n", obj_idx++);
	for (int index = 0; index < count; index++) {
		XwaVector3 v = vertices[index];
		fprintf(D3DDumpOBJFile, "v %0.6f %0.6f %0.6f\n", v.x, v.y, v.z);
	}
	fprintf(D3DDumpOBJFile, "\n");
}

inline Matrix4 XwaTransformToMatrix4(const XwaTransform &M)
{
	return Matrix4(
		M.Rotation._11, M.Rotation._12, M.Rotation._13, 0.0f,
		M.Rotation._21, M.Rotation._22, M.Rotation._23, 0.0f,
		M.Rotation._31, M.Rotation._32, M.Rotation._33, 0.0f,
		M.Position.x, M.Position.y, M.Position.z, 1.0f
	);
}

inline Vector4 XwaVector3ToVector4(const XwaVector3 &V)
{
	return Vector4(V.x, V.y, V.z, 1.0f);
}

inline Vector3 XwaVector3ToVector3(const XwaVector3& V)
{
	return Vector3(V.x, V.y, V.z);
}

inline XwaVector3 Vector4ToXwaVector3(const Vector4& V)
{
	return XwaVector3(V.x, V.y, V.z);
}

inline Vector3 Vector4ToVector3(const Vector4& V)
{
	return Vector3(V.x, V.y, V.z);
}

inline Vector4 Vector3ToVector4(const Vector3& V, float w)
{
	return Vector4(V.x, V.y, V.z, w);
}

inline Vector2 XwaTextureVertexToVector2(const XwaTextureVertex &V)
{
	return Vector2(V.u, V.v);
}

/*
 * Dump the current Limits to an OBJ file.
 * Before calling this method, make sure you call UpdateLimits() to convert the internal aabb into
 * a list of vertices and then call TransformLimits() with the appropriate transform matrix.
 */
int AABB::DumpLimitsToOBJ(FILE* D3DDumpOBJFile, const std::string &name, int VerticesCountOffset)
{
	fprintf(D3DDumpOBJFile, "o %s\n", name.c_str());
	for (uint32_t i = 0; i < Limits.size(); i++) {
		Vector4 V = Limits[i];
		fprintf(D3DDumpOBJFile, "v %0.6f %0.6f %0.6f\n", V.x, V.y, V.z);
	}

	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 0, VerticesCountOffset + 1);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 1, VerticesCountOffset + 2);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 2, VerticesCountOffset + 3);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 3, VerticesCountOffset + 0);

	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 4, VerticesCountOffset + 5);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 5, VerticesCountOffset + 6);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 6, VerticesCountOffset + 7);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 7, VerticesCountOffset + 4);

	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 0, VerticesCountOffset + 4);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 1, VerticesCountOffset + 5);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 2, VerticesCountOffset + 6);
	fprintf(D3DDumpOBJFile, "f %d %d\n", VerticesCountOffset + 3, VerticesCountOffset + 7);
	fprintf(D3DDumpOBJFile, "\n");
	return VerticesCountOffset + 8;
}

/*
 * Dump the current Limits to an OBJ file.
 * Before calling this method, make sure you call UpdateLimits() to convert the internal aabb into
 * a list of vertices and then call TransformLimits() with the appropriate transform matrix.
 */
int AABB::DumpLimitsToOBJ(FILE *D3DDumpOBJFile, int OBJGroupId, int VerticesCountOffset)
{
	return DumpLimitsToOBJ(D3DDumpOBJFile, std::string("aabb-") + std::to_string(OBJGroupId), VerticesCountOffset);
}

void DumpTLASLeaves(std::vector<TLASLeafItem> &tlasLeaves, char *fileName)
{
	Matrix4 S1;

	int VerticesCount = 1;
	FILE* D3DDumpOBJFile = NULL;
	fopen_s(&D3DDumpOBJFile, fileName, "wt");
	fprintf(D3DDumpOBJFile, "o globalAABB\n");

	S1.scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS);
	// Dump the global AABB
	g_GlobalAABB.UpdateLimits();
	g_GlobalAABB.TransformLimits(S1);
	VerticesCount = g_GlobalAABB.DumpLimitsToOBJ(D3DDumpOBJFile, "GlobalAABB", VerticesCount);

	// Dump all the other AABBs in tlasLeaves
	int counter = 0;
	for (auto& leaf : tlasLeaves)
	{
		// Morton Code, Bounding Box, TriID, Matrix, Centroid
		AABB obb = TLASGetOBB(leaf);
		int matrixSlot = -1;
		Matrix4 m = GetBLASMatrix(leaf, &matrixSlot);
		int meshKey = TLASGetID(leaf);
		// Fetch the meshData associated with this TLAS leaf
		MeshData& meshData = g_LBVHMap[meshKey];
		LBVH* bvh = (LBVH*)GetLBVH(meshData);
		if (bvh != nullptr && matrixSlot != -1)
		{
			log_debug("[DBG] [BVH] TLAS leaf %d, matrixSlot: %d", counter, matrixSlot);
			// The matrices are stored inverted in g_TLASMatrices because that's what
			// the RT shader needs
			//m = m.transpose(); // Matrices are stored transposed because HLSL needs that
			m = m.invert();
			obb.UpdateLimits();
			obb.TransformLimits(S1 * m);
			VerticesCount = obb.DumpLimitsToOBJ(D3DDumpOBJFile,
				std::to_string(counter) + "-" + std::to_string(matrixSlot),
				VerticesCount);
			counter++;
		}
	}
	fclose(D3DDumpOBJFile);
	log_debug("[DBG] [BVH] Dumped: %d tlas leaves", counter);
}

void DumpTLASTree(LBVH *g_TLASTree, char* sFileName, bool useMetricScale)
{
	BVHNode* nodes = (BVHNode*)(g_TLASTree->nodes);
	FILE* file = NULL;
	int index = 1;
	float scale[3] = { 1.0f, 1.0f, 1.0f };
	if (useMetricScale)
	{
		scale[0] =  OPT_TO_METERS;
		scale[1] = -OPT_TO_METERS;
		scale[2] =  OPT_TO_METERS;
	}
	const int numNodes = g_TLASTree->numNodes;

	fopen_s(&file, sFileName, "wt");
	if (file == NULL) {
		log_debug("[DBG] [BVH] Could not open file: %s", sFileName);
		return;
	}

	int root = nodes[0].rootIdx;
	log_debug("[DBG] [BVH] Dumping %d nodes to OBJ", numNodes - root);
	for (int i = root; i < numNodes; i++)
	{
		if (nodes[i].ref != -1)
		{
			// TLAS leaf
			BVHTLASLeafNode* node = (BVHTLASLeafNode*)&(nodes[i]);
			// Dump the AABB
			fprintf(file, "o tleaf-aabb-%d\n", i);

			fprintf(file, "v %f %f %f\n",
				node->min[0] * scale[0], node->min[1] * scale[1], node->min[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node->max[0] * scale[0], node->min[1] * scale[1], node->min[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node->max[0] * scale[0], node->max[1] * scale[1], node->min[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node->min[0] * scale[0], node->max[1] * scale[1], node->min[2] * scale[2]);

			fprintf(file, "v %f %f %f\n",
				node->min[0] * scale[0], node->min[1] * scale[1], node->max[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node->max[0] * scale[0], node->min[1] * scale[1], node->max[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node->max[0] * scale[0], node->max[1] * scale[1], node->max[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node->min[0] * scale[0], node->max[1] * scale[1], node->max[2] * scale[2]);

			fprintf(file, "f %d %d\n", index + 0, index + 1);
			fprintf(file, "f %d %d\n", index + 1, index + 2);
			fprintf(file, "f %d %d\n", index + 2, index + 3);
			fprintf(file, "f %d %d\n", index + 3, index + 0);

			fprintf(file, "f %d %d\n", index + 4, index + 5);
			fprintf(file, "f %d %d\n", index + 5, index + 6);
			fprintf(file, "f %d %d\n", index + 6, index + 7);
			fprintf(file, "f %d %d\n", index + 7, index + 4);

			fprintf(file, "f %d %d\n", index + 0, index + 4);
			fprintf(file, "f %d %d\n", index + 1, index + 5);
			fprintf(file, "f %d %d\n", index + 2, index + 6);
			fprintf(file, "f %d %d\n", index + 3, index + 7);
			index += 8;

			// Dump the OBB
			Matrix4 S1;
			S1.scale(scale[0], scale[1], scale[2]);

			Matrix4 W = g_TLASMatrices[node->matrixSlot]; // WorldView to OPT-coords
			//W = W.transpose(); // Matrices are stored transposed for HLSL
			W = W.invert(); // OPT-coords to WorldView
			AABB aabb;
			// Recover the OBB from the node:
			aabb.min.x = node->min[3];
			aabb.min.y = node->max[3];
			aabb.min.z = node->obb_max[3];

			aabb.max.x = node->obb_max[0];
			aabb.max.y = node->obb_max[1];
			aabb.max.z = node->obb_max[2];

			// Dump the OBB
			aabb.UpdateLimits();
			aabb.TransformLimits(S1 * W);
			index = aabb.DumpLimitsToOBJ(file, std::string("tleaf-obb-") + std::to_string(i), index);
		}
		else
		{
			// Inner node, dump the AABB
			BVHNode node = nodes[i];
			fprintf(file, "o aabb-%d\n", i);

			fprintf(file, "v %f %f %f\n",
				node.min[0] * scale[0], node.min[1] * scale[1], node.min[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node.max[0] * scale[0], node.min[1] * scale[1], node.min[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node.max[0] * scale[0], node.max[1] * scale[1], node.min[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node.min[0] * scale[0], node.max[1] * scale[1], node.min[2] * scale[2]);

			fprintf(file, "v %f %f %f\n",
				node.min[0] * scale[0], node.min[1] * scale[1], node.max[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node.max[0] * scale[0], node.min[1] * scale[1], node.max[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node.max[0] * scale[0], node.max[1] * scale[1], node.max[2] * scale[2]);
			fprintf(file, "v %f %f %f\n",
				node.min[0] * scale[0], node.max[1] * scale[1], node.max[2] * scale[2]);

			fprintf(file, "f %d %d\n", index + 0, index + 1);
			fprintf(file, "f %d %d\n", index + 1, index + 2);
			fprintf(file, "f %d %d\n", index + 2, index + 3);
			fprintf(file, "f %d %d\n", index + 3, index + 0);

			fprintf(file, "f %d %d\n", index + 4, index + 5);
			fprintf(file, "f %d %d\n", index + 5, index + 6);
			fprintf(file, "f %d %d\n", index + 6, index + 7);
			fprintf(file, "f %d %d\n", index + 7, index + 4);

			fprintf(file, "f %d %d\n", index + 0, index + 4);
			fprintf(file, "f %d %d\n", index + 1, index + 5);
			fprintf(file, "f %d %d\n", index + 2, index + 6);
			fprintf(file, "f %d %d\n", index + 3, index + 7);
			index += 8;
		}
	}
	fclose(file);
	log_debug("[DBG] [BVH] BVH Dumped to OBJ");
}

void BuildTLAS(std::vector<TLASLeafItem> &tlasLeaves, bool isACTLAS=false)
{
	const uint32_t numLeaves = tlasLeaves.size();
	if (!isACTLAS)
	{
		g_iRTMeshesInThisFrame = numLeaves;
	}

	if (numLeaves == 0)
	{
		//log_debug("[DBG] [BVH] BuildTLAS: numLeaves 0. Early exit.");
		return;
	}

	// Get the morton codes for the tlas leaves
	for (uint32_t i = 0; i < numLeaves; i++)
	{
		auto& leaf = tlasLeaves[i];
		Vector3 centroid = TLASGetCentroid(leaf);
		Normalize(centroid, g_GlobalAABB, g_GlobalRange);
		TLASGetMortonCode(leaf) = GetMortonCode(centroid);
	}

	// Sort the tlas leaves
	std::sort(tlasLeaves.begin(), tlasLeaves.end(), tlasLeafSorter);

	// Encode the sorted leaves
	// TODO: Encode the leaves before sorting, and use TriID as the sort index.
	const int numQBVHInnerNodes = CalcNumInnerQBVHNodes(numLeaves);
	const int numQBVHNodes = numQBVHInnerNodes + numLeaves;

	// We can reserve the buffer for the QBVH now.
	BVHNode* QBVHBuffer = new BVHNode[numQBVHNodes];

	// Encode the TLAS leaves (the matrixSlot and BLASBaseNodeOffset are encoded here)
	int LeafEncodeIdx = numQBVHInnerNodes;
	for (uint32_t i = 0; i < numLeaves; i++)
	{
		TLASEncodeLeafNode(QBVHBuffer, tlasLeaves, i, LeafEncodeIdx++);
	}

	//log_debug("[DBG] [BVH] BuildTLAS: numLeaves: %d, numQBVHInnerNodes: %d, numQBVHNodes: %d", numLeaves, numQBVHInnerNodes, numQBVHNodes);
	// Build, convert and encode the QBVH
	int root = -1;
	TLASSingleStepFastLQBVH(QBVHBuffer, numQBVHInnerNodes, tlasLeaves, root);
	//log_debug("[DBG] [BVH] FastLQBVH** finished. QTree built. root: %d, numQBVHNodes: %d", root, numQBVHNodes);
	// Initialize the root
	QBVHBuffer[0].rootIdx = root;
	if (g_bDumpSSAOBuffers)
		log_debug("[DBG] [BVH] TLAS root: %d, isACTLAS: %d", root, isACTLAS);

	// delete[] QBVHBuffer;

	// The previous TLAS tree should be deleted at the beginning of each frame.
	if (!isACTLAS)
	{
		g_TLASTree = new LBVH();
		g_TLASTree->nodes = QBVHBuffer;
		g_TLASTree->numNodes = numQBVHNodes;
		g_TLASTree->scale = 1.0f;
		g_TLASTree->scaleComputed = true;

		if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled)
		{
			// The single-node tree's AABB matches the global AABB and also contains the OBB
			//g_TLASTree->DumpToOBJ(".\\TLASTree.obj", true /* isTLAS */, true /* Metric Scale? */);
			DumpTLASTree(g_TLASTree, ".\\TLASTree.obj", true /* Metric Scale? */);
		}
	}
	else
	{
		g_ACTLASTree = new LBVH();
		g_ACTLASTree->nodes = QBVHBuffer;
		g_ACTLASTree->numNodes = numQBVHNodes;
		g_ACTLASTree->scale = 1.0f;
		g_ACTLASTree->scaleComputed = true;

		if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled)
		{
			// The single-node tree's AABB matches the global AABB and also contains the OBB
			//g_TLASTree->DumpToOBJ(".\\TLASTree.obj", true /* isTLAS */, true /* Metric Scale? */);
			DumpTLASTree(g_ACTLASTree, ".\\ACTLASTree.obj", true /* Metric Scale? */);
		}
	}
}

void BuildTLASDBVH4()
{
	g_iRTMeshesInThisFrame = tlasLeaves.size();
	const uint32_t numLeaves = g_iRTMeshesInThisFrame;
	if (numLeaves == 0)
	{
		//log_debug("[DBG] [BVH] BuildTLAS: numLeaves 0. Early exit.");
		return;
	}

	const int numInnerNodes = CalcNumInnerQBVHNodes(numLeaves);
	const int numNodes = numInnerNodes + numLeaves;
	BVHNode* QBVHBuffer = new BVHNode[numNodes];

	int finalInnerNodes;
	TLASDirectBVH4BuilderGPU(g_GlobalCentroidAABB, tlasLeaves, QBVHBuffer, finalInnerNodes);
	if (g_bDumpSSAOBuffers)
		log_debug("[DBG] [BVH] TLAS root: 0");
	// delete[] QBVHBuffer;

	// DEBUG: Print the maxRatio so far and dump the TLAS input if the ratio is too big
	{
		static float maxRatio = 0.0f;
		float ratio = (float)g_directBuilderNextInnerNode / (float)numLeaves;
		if (ratio > maxRatio)
		{
			maxRatio = ratio;
			log_debug("[DBG] [BVH] numInnerNodes: %d, numLeaves: %d, maxRatio: %0.4f, finalRatio: %0.4f",
				g_directBuilderNextInnerNode, numLeaves, maxRatio, (float)finalInnerNodes / (float)numLeaves);

			// Dump the data needed to build the TLAS tree for debugging purposes
			/*
			if (maxRatio > 0.667f)
			{
				static int fileCounter = 0;
				char fileName[80];
				sprintf_s(fileName, 80, "tlasLeaves-%d.txt", fileCounter++);

				FILE* file = nullptr;
				fopen_s(&file, fileName, "wt");
				if (file != nullptr)
				{
					fprintf(file, "%s\n", g_GlobalCentroidAABB.ToString().c_str());
					fprintf(file, "%d\n", (int)tlasLeaves.size());
					for (int i = 0; i < (int)tlasLeaves.size(); i++)
						fprintf(file, "%s\n", tlasLeaves[i].ToString().c_str());
					fclose(file);
					log_debug("[DBG] [BVH] Dumped %s for debugging", fileName);
				}
			}
			*/
		}
	}

	// The previous TLAS tree should be deleted at the beginning of each frame.
	g_TLASTree = new LBVH();
	g_TLASTree->nodes = QBVHBuffer;
	g_TLASTree->numNodes = numNodes;
	g_TLASTree->scale = 1.0f;
	g_TLASTree->scaleComputed = true;

	if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled)
	{
		// The single-node tree's AABB matches the global AABB and also contains the OBB
		//g_TLASTree->DumpToOBJ(".\\TLASTree.obj", true /* isTLAS */, true /* Metric Scale? */);
		DumpTLASTree(g_TLASTree, ".\\TLASTree.obj", true /* Metric Scale? */);
	}
}

// https://github.com/embree/embree/blob/master/tutorials/bvh_builder/bvh_builder_device.cpp
struct BuildDataTLAS
{
	LONG* pTotalNodes = nullptr;
	std::vector<RTCBuildPrimitive> prims;
	BVHNode* QBVHBuffer = nullptr;
	RTCBVH bvh = nullptr;
	std::map<NodeChildKey, AABB> nodeToABBMap;

	BuildDataTLAS(int numPrims)
	{
		prims.reserve(numPrims);
		prims.resize(numPrims);

		pTotalNodes = (LONG*)_aligned_malloc(sizeof(LONG), 32);
		*pTotalNodes = 0;

		bvh = g_rtcNewBVH(g_rtcDevice);
	}

	~BuildDataTLAS()
	{
		_aligned_free(pTotalNodes);
		g_rtcReleaseBVH(bvh);
	}
};

#undef RTC_DEBUG

static void* RTCCreateNodeTLAS(RTCThreadLocalAllocator alloc, unsigned int numChildren, void* userPtr)
{
	BuildDataTLAS* buildData = (BuildDataTLAS*)userPtr;
	InterlockedAdd(buildData->pTotalNodes, 1);
	//void* ptr = rtcThreadLocalAlloc(alloc, sizeof(InnerNode), 16);

	//QTreeNode *node = (QTreeNode *)rtcThreadLocalAlloc(alloc, sizeof(QTreeNode), 16);
	//node->Init();
	QTreeNode* node = new QTreeNode();

#ifdef RTC_DEBUG
	log_debug("[DBG] [BVH] [EMB] Created new inner node 0x%x, total nodes: %d",
		node, *buildData->pTotalNodes);
#endif
	return node;
	//return (void*) new (ptr) InnerNode;
	//return (void*) new (node)QTreeNode;
}

static void* RTCCreateLeafTLAS(RTCThreadLocalAllocator alloc, const RTCBuildPrimitive* prims, size_t numPrims, void* userPtr)
{
	BuildDataTLAS* buildData = (BuildDataTLAS*)userPtr;
	//assert(numPrims == 1);
	//void* ptr = rtcThreadLocalAlloc(alloc, sizeof(LeafNode), 16);
	InterlockedAdd(buildData->pTotalNodes, 1);
	//QTreeNode* node = (QTreeNode*)rtcThreadLocalAlloc(alloc, sizeof(QTreeNode), 16);
	//node->Init();
	QTreeNode* node = new QTreeNode();
#ifdef RTC_DEBUG
	log_debug("[DBG] [BVH] [EMB] Created new leaf node 0x%x, total nodes: %d",
		node, *buildData->pTotalNodes);
#endif

	node->TriID = prims->primID;
	node->box.min.x = prims->lower_x;
	node->box.min.y = prims->lower_y;
	node->box.min.z = prims->lower_z;

	node->box.max.x = prims->upper_x;
	node->box.max.y = prims->upper_y;
	node->box.max.z = prims->upper_z;

	return node;
	//return (void*) new (ptr) LeafNode(prims->primID, *(BBox3fa*)prims);
	//return (void*) new (node)QTreeNode(prims->primID);
}

static void RTCSetChildrenTLAS(void* nodePtr, void** childPtr, unsigned int numChildren, void* userPtr)
{
	BuildDataTLAS* buildData = (BuildDataTLAS*)userPtr;
	QTreeNode* node = (QTreeNode*)nodePtr;
	AABB aabb;

	for (size_t i = 0; i < numChildren; i++)
	{
		//((InnerNode*)nodePtr)->children[i] = (Node*)childPtr[i];
		NodeChildKey key = NodeChildKey((uint32_t)node, i);
		node->children[i] = (QTreeNode*)childPtr[i];
		QTreeNode* child = node->children[i];

#ifdef RTC_DEBUG
		log_debug("[DBG] [BVH] [EMB] node 0x%x connected to 0x%x",
			(uint32_t)node, (uint32_t)childPtr[i]);
#endif

		// TODO: This is a crutch, we shouldn't have to query the map, but sometimes the
		// AABBs get set on NULL children (i.e. RTCSetChildren() is called _after_ RTCSetBounds()),
		// so here we are...
		const auto& it = buildData->nodeToABBMap.find(key);
		if (it != buildData->nodeToABBMap.end())
		{
#ifdef RTC_DEBUG
			log_debug("[DBG] [BVH] [EMB] Found box for 0x%x-%d", (uint32_t)node, i);
#endif
			child->box = it->second;
		}
		aabb.Expand(child->box);
	}
#ifdef RTC_DEBUG
	log_debug("[DBG] [BVH] [EMB] node 0x%x, box: %s", (uint32_t)node, aabb.ToString().c_str());
#endif
	node->box = aabb;
}

static void RTCSetBoundsTLAS(void* nodePtr, const RTCBounds** bounds, unsigned int numChildren, void* userPtr)
{
	BuildDataTLAS* buildData = (BuildDataTLAS*)userPtr;
	//assert(numChildren == 2);
	QTreeNode* node = (QTreeNode*)nodePtr;
	AABB aabb;

	for (size_t i = 0; i < numChildren; i++)
	{
		QTreeNode* child = node->children[i];
		if (child != nullptr)
		{
			child->box.min.x = bounds[i]->lower_x;
			child->box.min.y = bounds[i]->lower_y;
			child->box.min.z = bounds[i]->lower_z;

			child->box.max.x = bounds[i]->upper_x;
			child->box.max.y = bounds[i]->upper_y;
			child->box.max.z = bounds[i]->upper_z;
#ifdef RTC_DEBUG
			log_debug("[DBG] [BVH] [EMB] Set bounds on node 0x%x", (uint32_t)child);
#endif
		}
		else
		{
#ifdef RTC_DEBUG
			log_debug("[DBG] [BVH] [EMB] ERROR: Attempted to set bounds on NULL ptr, parent: 0x%x, child: %d",
				(uint32_t)node, i);
#endif
			AABB childAABB;
			childAABB.Expand(bounds[i]);
			// TODO: This silly map is here because sometimes this callback gets triggered when
			// the children haven't been set yet, so we need to save the AABBs for later.
			buildData->nodeToABBMap[NodeChildKey((uint32_t)node, i)] = childAABB;
#ifdef RTC_DEBUG
			log_debug("[DBG] [BVH] [EMB] Added AABB for 0x%x-%d, box: %s",
				(uint32_t)node, i, childAABB.ToString().c_str());
#endif
		}

		// Update the AABB for the inner node
		aabb.Expand(bounds[i]);
	}
	node->box = aabb;
}

static bool RTCBuildProgressTLAS(void* userPtr, double f) {
	//log_debug("[DBG] [BVH] [EMB] Build Progress: %0.3f", f);
	// Return false to cancel this build
	return true;
}

void BuildTLASEmbree()
{
	g_iRTMeshesInThisFrame = tlasLeaves.size();
	const uint32_t numLeaves = g_iRTMeshesInThisFrame;
	if (numLeaves == 0)
	{
		//log_debug("[DBG] [BVH] BuildTLAS: numLeaves 0. Early exit.");
		return;
	}

	BuildDataTLAS buildData(numLeaves);
	// Populate the primitives
	for (uint32_t ID = 0; ID < numLeaves; ID++) {
		AABB aabb = TLASGetAABBFromOBB(tlasLeaves[ID]);
		RTCBuildPrimitive prim;

		prim.geomID = 0;
		prim.primID = ID;

		prim.lower_x = aabb.min.x;
		prim.lower_y = aabb.min.y;
		prim.lower_z = aabb.min.z;

		prim.upper_x = aabb.max.x;
		prim.upper_y = aabb.max.y;
		prim.upper_z = aabb.max.z;

		buildData.prims[ID] = prim;
	}

	// Configure the BVH build
	RTCBuildArguments arguments = rtcDefaultBuildArguments();
	arguments.byteSize = sizeof(arguments);
	arguments.buildFlags = RTCBuildFlags::RTC_BUILD_FLAG_DYNAMIC;
	arguments.buildQuality = RTCBuildQuality::RTC_BUILD_QUALITY_LOW;
	arguments.maxBranchingFactor = 4;
	arguments.maxDepth = 1024;
	arguments.sahBlockSize = 1;
	arguments.minLeafSize = 1;
	arguments.maxLeafSize = 1;
	arguments.traversalCost = 1.0f;
	arguments.intersectionCost = 2.0f;
	arguments.bvh = buildData.bvh;
	arguments.primitives = buildData.prims.data();
	arguments.primitiveCount = buildData.prims.size();
	arguments.primitiveArrayCapacity = buildData.prims.capacity();
	arguments.createNode = RTCCreateNodeTLAS;
	arguments.setNodeChildren = RTCSetChildrenTLAS;
	arguments.setNodeBounds = RTCSetBoundsTLAS;
	arguments.createLeaf = RTCCreateLeafTLAS;
	arguments.splitPrimitive = nullptr;
	arguments.buildProgress = RTCBuildProgressTLAS;
	arguments.userPtr = &buildData;

	QTreeNode* root = (QTreeNode*)g_rtcBuildBVH(&arguments);
	int totalNodes = *(buildData.pTotalNodes);
	root->SetNumNodes(totalNodes);

	buildData.QBVHBuffer = (BVHNode*)TLASEncodeNodes(root, tlasLeaves);
	// Initialize the root
	buildData.QBVHBuffer[0].rootIdx = 0;
	DeleteTree(root);

	// The previous TLAS tree should be deleted at the beginning of each frame.
	g_TLASTree = new LBVH();
	g_TLASTree->nodes = buildData.QBVHBuffer;
	g_TLASTree->numNodes = totalNodes;
	g_TLASTree->scale = 1.0f;
	g_TLASTree->scaleComputed = true;

	if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled)
	{
		// The single-node tree's AABB matches the global AABB and also contains the OBB
		//g_TLASTree->DumpToOBJ(".\\TLASTree.obj", true /* isTLAS */, true /* Metric Scale? */);
		DumpTLASTree(g_TLASTree, ".\\TLASTree.obj", true /* Metric Scale? */);
	}
}

/// <summary>
/// Appends the current MeshVertices/FaceIndices to the current OBJ file.
/// </summary>
void EffectsRenderer::OBJDumpD3dVertices(const SceneCompData *scene, const Matrix4 &A)
{
	std::ostringstream str;
	XwaVector3 *MeshVertices = scene->MeshVertices;
	int MeshVerticesCount = *(int*)((int)scene->MeshVertices - 8);
	XwaVector3* MeshNormals = scene->MeshVertexNormals;
	int MeshNormalsCount = *(int*)((int)MeshNormals - 8);
	static XwaVector3 *LastMeshVertices = nullptr;
	static int LastMeshVerticesCount = 0, LastMeshNormalsCount = 0;
	bool bShadowDump = g_rendererType == RendererType_Shadow;

	if (D3DOBJGroup == 1) {
		LastMeshVerticesCount = 0;
		LastMeshNormalsCount = 0;
	}

	// DEPTH-BUFFER-CHANGE DONE
	float *Znear = (float *)0x08B94CC;
	float *Zfar = (float *)0x05B46B4;
	float projectionDeltaX = *(float*)0x08C1600 + *(float*)0x0686ACC;
	float projectionDeltaY = *(float*)0x080ACF8 + *(float*)0x07B33C0 + *(float*)0x064D1AC;
	fprintf(D3DDumpOBJFile, "# (Znear) 0x08B94CC: %0.6f, (Zfar) 0x05B46B4: %0.6f\n", *Znear, *Zfar);
	fprintf(D3DDumpOBJFile, "# projDeltaX,Y: %0.6f, %0.6f\n", projectionDeltaX, projectionDeltaY);
	fprintf(D3DDumpOBJFile, "# viewportScale: %0.6f, %0.6f %0.6f\n",
		g_VSCBuffer.viewportScale[0], g_VSCBuffer.viewportScale[1], g_VSCBuffer.viewportScale[2]);
	if (bShadowDump) {
		fprintf(D3DDumpOBJFile, "# Hangar Shadow Render, Floor: %0.3f, hangarShadowAccStartEnd: %0.3f, "
			"sx1,sy1: (%0.3f, %0.3f), sx2,sy2:(%0.3f, %0.3f)\n",
			_constants.floorLevel, _constants.hangarShadowAccStartEnd,
			_constants.sx1, _constants.sy1,
			_constants.sx2, _constants.sy2);
		fprintf(D3DDumpOBJFile, "# Camera: %0.3f, %0.3f\n",
			_constants.cameraPositionX, _constants.cameraPositionY);
	}

	if (LastMeshVertices != MeshVertices) {
		// This is a new mesh, dump all the vertices.
		Matrix4 W = XwaTransformToMatrix4(scene->WorldViewTransform);
		Matrix4 L, S1, S2;
		L.identity();
		S1.scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS);
		S2.scale(METERS_TO_OPT, -METERS_TO_OPT, METERS_TO_OPT);
		if (g_rendererType == RendererType_Shadow)
			// See HangarShadowSceneHook for an explanation of why L looks like this:
			L = A * S1 * Matrix4(_constants.hangarShadowView) * S2;

#define DUMP_AABBS 0
#if DUMP_AABBS == 1
		auto it = _AABBs.find((int)(scene->MeshVertices));
		if (it != _AABBs.end())
		{
			AABB aabb = it->second;
			aabb.UpdateLimits();
			aabb.TransformLimits(L * S1 * W);
			aabb.DumpLimitsToOBJ(D3DDumpOBJFile, D3DOBJGroup, D3DTotalVertices + LastMeshVerticesCount);
			D3DTotalVertices += aabb.Limits.size();
		}
		else
			fprintf(D3DDumpOBJFile, "# No AABB found for this mesh\n");
#endif

		//log_debug("[DBG] Writting obj_idx: %d, MeshVerticesCount: %d, NormalsCount: %d, FacesCount: %d",
		//	D3DOBJGroup, MeshVerticesCount, MeshNormalsCount, scene->FacesCount);
		fprintf(D3DDumpOBJFile, "o %s-%d\n", bShadowDump ? "shw" : "obj", D3DOBJGroup);
		for (int i = 0; i < MeshVerticesCount; i++) {
			XwaVector3 v = MeshVertices[i];
			Vector4 V(v.x, v.y, v.z, 1.0f);
			V = W * V;

			// Enable the following block to debug InverseTransformProjection
#define EXTRA_DEBUG 0
#if EXTRA_DEBUG == 1
			{
				float3 P;
				P.x = V.x;
				P.y = V.y;
				P.z = V.z;
				float4 Q = TransformProjectionScreen(P);
				float3 R = InverseTransformProjectionScreen(Q);
				R.x *= OPT_TO_METERS;
				R.y *= OPT_TO_METERS;
				R.z *= OPT_TO_METERS;
				fprintf(D3DDumpOBJFile, "# Q %0.3f %0.3f %0.6f %0.6f\n", Q.x, Q.y, Q.z, Q.w);
				fprintf(D3DDumpOBJFile, "# R %0.3f %0.3f %0.6f\n", R.x, R.y, R.z);

				//float4 Q = TransformProjection(P);
				//float3 R = InverseTransformProjection(Q);
				//fprintf(D3DDumpOBJFile, "# Q %0.3f %0.3f %0.6f %0.6f\n", Q.x, Q.y, Q.z, Q.w);
				//fprintf(D3DDumpOBJFile, "# R %0.3f %0.3f %0.3f\n", R.x, R.y, R.z);
				//fprintf(D3DDumpOBJFile, "# V %0.3f %0.3f %0.3f\n", V.x, V.y, V.z);
			}
#endif

#define DUMP_2D 0 // If enabled, it will project 3D content to 2D screen coords and then dump those coords
#if DUMP_2D == 0
			// OPT to meters conversion:
			//V *= OPT_TO_METERS;
			V = L * S1 * V;
			fprintf(D3DDumpOBJFile, "v %0.6f %0.6f %0.6f\n", V.x, V.y, V.z);
#else
			float3 P = { V.x, V.y, V.z };
			float4 Q = TransformProjectionScreen(P);
			fprintf(D3DDumpOBJFile, "v %0.6f %0.6f %0.6f # %0.6f\n", Q.x, Q.z, Q.y, Q.w);
			//float4 Q = TransformProjection(P);
			//fprintf(D3DDumpOBJFile, "v %0.6f %0.6f %0.6f # %0.6f\n", g_fCurInGameWidth * Q.x, -g_fCurInGameHeight * Q.y, Q.z, Q.w);
#endif
		}
		fprintf(D3DDumpOBJFile, "\n");

		// Dump the normals
#if DUMP_2D == 0
		for (int i = 0; i < MeshNormalsCount; i++) {
			XwaVector3 N = MeshNormals[i];
			fprintf(D3DDumpOBJFile, "vn %0.6f %0.6f %0.6f\n", N.x, N.y, N.z);
		}
		fprintf(D3DDumpOBJFile, "\n");
#endif

		D3DTotalVertices += LastMeshVerticesCount;
		D3DTotalNormals += LastMeshNormalsCount;
		D3DOBJGroup++;

		LastMeshVertices = MeshVertices;
		LastMeshVerticesCount = MeshVerticesCount;
		LastMeshNormalsCount = MeshNormalsCount;
	}

	// The following works alright, but it's not how things are rendered.
	for (int faceIndex = 0; faceIndex < scene->FacesCount; faceIndex++) {
		OptFaceDataNode_01_Data_Indices& faceData = scene->FaceIndices[faceIndex];
		int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;
		std::string line = "f ";

		for (int vertexIndex = 0; vertexIndex < edgesCount; vertexIndex++)
		{
			// faceData.Vertex[vertexIndex] matches the vertex index data from the OPT
			line += std::to_string(faceData.Vertex[vertexIndex] + D3DTotalVertices) + "//" +
					std::to_string(faceData.VertexNormal[vertexIndex] + D3DTotalNormals) + " ";
		}
		fprintf(D3DDumpOBJFile, "%s\n", line.c_str());
	}
	fprintf(D3DDumpOBJFile, "\n");
}

/// <summary>
/// Dumps a single file for the current MeshVertices/FaceIndices.
/// </summary>
void EffectsRenderer::SingleFileOBJDumpD3dVertices(const SceneCompData* scene, int trianglesCount, const std::string& name)
{
	std::ostringstream str;
	XwaVector3* MeshVertices = scene->MeshVertices;
	int MeshVerticesCount = *(int*)((int)scene->MeshVertices - 8);
	XwaTextureVertex* MeshTextureVertices = scene->MeshTextureVertices;
	int MeshTextureVerticesCount = *(int*)((int)scene->MeshTextureVertices - 8);
	XwaVector3* MeshNormals = scene->MeshVertexNormals;
	int MeshNormalsCount = *(int*)((int)MeshNormals - 8);

	int D3DOBJGroup = 0;
	FILE* D3DDumpOBJFile = nullptr;
	fopen_s(&D3DDumpOBJFile, name.c_str(), "wt");
	if (D3DDumpOBJFile == nullptr)
	{
		log_debug("[DBG] Could not dump file: %s", name.c_str());
		return;
	}

	// This is a new mesh, dump all the vertices.
	//Matrix4 W = XwaTransformToMatrix4(scene->WorldViewTransform);
	Matrix4 W;
	W.identity();

	Matrix4 S1;
	S1.scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS);

	fprintf(D3DDumpOBJFile, "# %s\n", _lastTextureSelected->_name.c_str());
	fprintf(D3DDumpOBJFile, "o obj-%d\n", D3DOBJGroup);

	// Dump the vertices
	for (int i = 0; i < MeshVerticesCount; i++) {
		XwaVector3 v = MeshVertices[i];
		Vector4 V(v.x, v.y, v.z, 1.0f);
		// Apply the world view transform + OPT->meters conversion
		V = S1 * W * V;
		fprintf(D3DDumpOBJFile, "v %0.6f %0.6f %0.6f\n", V.x, V.y, V.z);
	}
	fprintf(D3DDumpOBJFile, "\n");

	// Dump the UVs
	for (int i = 0; i < MeshTextureVerticesCount; i++) {
		XwaTextureVertex vt = MeshTextureVertices[i];
		fprintf(D3DDumpOBJFile, "vt %0.3f %0.3f\n", vt.u, vt.v);
	}
	fprintf(D3DDumpOBJFile, "\n");

	// Dump the normals
	for (int i = 0; i < MeshNormalsCount; i++) {
		XwaVector3 N = MeshNormals[i];
		fprintf(D3DDumpOBJFile, "vn %0.6f %0.6f %0.6f\n", N.x, N.y, N.z);
	}
	fprintf(D3DDumpOBJFile, "\n");
	D3DOBJGroup++;

	// The following works alright, but it's not how things are rendered.
	for (int faceIndex = 0; faceIndex < scene->FacesCount; faceIndex++)
	{
		OptFaceDataNode_01_Data_Indices& faceData = scene->FaceIndices[faceIndex];
		int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;

		// Undefine the following to dump full quads
#define DUMP_TRIS 1
#ifdef DUMP_TRIS
		// This converts quads into 2 tris if necessary
		for (int edge = 2; edge < edgesCount; edge++)
		{
			D3dTriangle t;
			t.v1 = 0;
			t.v2 = edge - 1;
			t.v3 = edge;

			std::string line = "f ";
			line += std::to_string(faceData.Vertex[t.v1] + 1) + "/" +
				    std::to_string(faceData.TextureVertex[t.v1] + 1) + "/" +
				    std::to_string(faceData.VertexNormal[t.v1] + 1) + " ";

			line += std::to_string(faceData.Vertex[t.v2] + 1) + "/" +
				    std::to_string(faceData.TextureVertex[t.v2] + 1) + "/" +
				    std::to_string(faceData.VertexNormal[t.v2] + 1) + " ";

			line += std::to_string(faceData.Vertex[t.v3] + 1) + "/" +
				    std::to_string(faceData.TextureVertex[t.v3] + 1) + "/" +
				    std::to_string(faceData.VertexNormal[t.v3] + 1) + " ";
			fprintf(D3DDumpOBJFile, "%s\n", line.c_str());
		}
#else
		std::string line = "f ";
		for (int vertexIndex = 0; vertexIndex < edgesCount; vertexIndex++)
		{
			// faceData.Vertex[vertexIndex] matches the vertex index data from the OPT
			line += std::to_string(faceData.Vertex[vertexIndex] + 1) + "/" +
					std::to_string(faceData.TextureVertex[vertexIndex] + 1) + "/" +
					std::to_string(faceData.VertexNormal[vertexIndex] + 1) + " ";
		}
		fprintf(D3DDumpOBJFile, "%s\n", line.c_str());
#endif
	}
	fclose(D3DDumpOBJFile);
}

//************************************************************************
// Effects Renderer
//************************************************************************

EffectsRenderer::EffectsRenderer() : D3dRenderer() {
	_hangarShadowMapRotation.identity();
	_hangarShadowMapRotation.rotateX(180.0f);
}

// Based on Direct3DTexture::CreateSRVFromBuffer()
// Yes, I know I shouldn't duplicate code, but this is a much simpler
// way to load textures. The code in Direct3DTexture is loaded in a
// deferred fashion and that's always been a bit cumbersome.
HRESULT EffectsRenderer::CreateSRVFromBuffer(uint8_t* Buffer, int BufferLength, int Width, int Height, ID3D11ShaderResourceView** srv)
{
	auto& resources = this->_deviceResources;
	auto& context = resources->_d3dDeviceContext;
	auto& device = resources->_d3dDevice;

	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	D3D11_SUBRESOURCE_DATA textureData = { 0 };
	ComPtr<ID3D11Texture2D> texture2D;
	*srv = NULL;

	bool isBc7 = (BufferLength == Width * Height);
	DXGI_FORMAT ColorFormat = (g_DATReaderVersion <= DAT_READER_VERSION_101 || g_config.FlipDATImages) ?
		DXGI_FORMAT_R8G8B8A8_UNORM : // Original, to be used with DATReader 1.0.1. Needs channel swizzling.
		DXGI_FORMAT_B8G8R8A8_UNORM;  // To be used with DATReader 1.0.2+. Enables Marshal.Copy(), no channel swizzling.
	desc.Width = (UINT)Width;
	desc.Height = (UINT)Height;
	desc.Format = isBc7 ? DXGI_FORMAT_BC7_UNORM : ColorFormat;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	textureData.pSysMem = (void*)Buffer;
	textureData.SysMemPitch = sizeof(uint8_t) * Width * 4;
	textureData.SysMemSlicePitch = 0;

	if (FAILED(hr = device->CreateTexture2D(&desc, &textureData, &texture2D))) {
		log_debug("[DBG] Failed when calling CreateTexture2D from Buffer, reason: 0x%x",
			device->GetDeviceRemovedReason());
		goto out;
	}

	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	if (FAILED(hr = device->CreateShaderResourceView(texture2D, &shaderResourceViewDesc, srv))) {
		log_debug("[DBG] Failed when calling CreateShaderResourceView on texture2D, reason: 0x%x",
			device->GetDeviceRemovedReason());
		goto out;
	}

out:
	return hr;
}

// Based on Direct3DTexture::LoadDATImage()
// Yes, I know I shouldn't duplicate code... see CreateSRVFromBuffer() above
int EffectsRenderer::LoadDATImage(char* sDATFileName, int GroupId, int ImageId, ID3D11ShaderResourceView** srv,
	short* Width_out, short* Height_out)
{
	short Width = 0, Height = 0;
	uint8_t Format = 0;
	uint8_t* buf = nullptr;
	int buf_len = 0;
	// Initialize the output to null/failure by default:
	HRESULT res = E_FAIL;
	auto& resources = this->_deviceResources;
	int index = -1;
	*srv = nullptr;

	if (!InitDATReader()) // This call is idempotent and does nothing when DATReader is already loaded
	{
		log_debug("[DBG] InitDATReader() failed");
		return -1;
	}

	if (!LoadDATFile(sDATFileName)) {
		log_debug("[DBG] Could not load DAT file: %s", sDATFileName);
		return -1;
	}

	if (!GetDATImageMetadata(GroupId, ImageId, &Width, &Height, &Format)) {
		log_debug("[DBG] [C++] DAT Image %d-%d not found", GroupId, ImageId);
		return -1;
	}

	if (Width_out != nullptr) *Width_out = Width;
	if (Height_out != nullptr) *Height_out = Height;

	const bool isBc7 = (Format == 27);

	if (isBc7 && (Width % 4 == 0) && (Height % 4 == 0))
	{
		buf_len = Width * Height;
	}
	else
	{
		buf_len = Width * Height * 4;
	}

	buf = new uint8_t[buf_len];
	if (!isBc7 && g_config.FlipDATImages && ReadFlippedDATImageData != nullptr)
	{
		if (ReadFlippedDATImageData(buf, buf_len))
			res = CreateSRVFromBuffer(buf, buf_len, Width, Height, srv);
		else
			log_debug("[DBG] [C++] Failed to read flipped image data");
	}
	else
	{
		if (ReadDATImageData(buf, buf_len))
			res = CreateSRVFromBuffer(buf, buf_len, Width, Height, srv);
		else
			log_debug("[DBG] [C++] Failed to read image data");
	}

	if (buf != nullptr) delete[] buf;

	if (FAILED(res))
	{
		log_debug("[DBG] [C++] Could not create SRV from image data");
		return -1;
	}

	return S_OK;
}

constexpr int g_vrKeybNumTriangles = 2;
constexpr int g_vrKeybMeshVerticesCount = 4;
constexpr int g_vrKeybTextureCoordsCount = 4;
D3dTriangle g_vrKeybTriangles[g_vrKeybNumTriangles];
XwaVector3 g_vrKeybMeshVertices[g_vrKeybMeshVerticesCount];
XwaTextureVertex g_vrKeybTextureCoords[g_vrKeybTextureCoordsCount];

constexpr int g_vrDotNumTriangles = 2;
constexpr int g_vrDotMeshVerticesCount = 4;
constexpr int g_vrDotTextureCoordsCount = 4;
D3dTriangle g_vrDotTriangles[g_vrDotNumTriangles];
XwaVector3 g_vrDotMeshVertices[g_vrDotMeshVerticesCount];
XwaTextureVertex g_vrDotTextureCoords[g_vrDotTextureCoordsCount];

char* g_GlovesProfileNames[2][VRGlovesProfile::MAX] = {
	{ "Effects\\ActiveCockpit\\LGloveNeutral.obj",
	  "Effects\\ActiveCockpit\\LGlovePoint.obj",
	  "Effects\\ActiveCockpit\\LGloveGrasp.obj" },
	{ "Effects\\ActiveCockpit\\RGloveNeutral.obj",
	  "Effects\\ActiveCockpit\\RGlovePoint.obj",
	  "Effects\\ActiveCockpit\\RGloveGrasp.obj" },
};

/// <summary>
/// Loads and OBJ and returns the number of triangles read
/// </summary>
int EffectsRenderer::LoadOBJ(int gloveIdx, Matrix4 R, char* sFileName, int profile, bool buildBVH)
{
	FILE* file;
	int error = 0;

	try {
		error = fopen_s(&file, sFileName, "rt");
	}
	catch (...) {
		log_debug("[DBG] [AC] Could not load file %s", sFileName);
	}

	if (error != 0) {
		log_debug("[DBG] [AC] Error %d when loading %s", error, sFileName);
		return -1;
	}

	float yMin = FLT_MAX;
	// In XWA's coord sys, Y- is forwards, so we initialize to FLT_MAX to get the lowest point
	// in the Y axis:
	g_vrGlovesMeshes[gloveIdx].forwardPmeters[profile] = FLT_MAX;

	std::vector<XwaVector3> vertices;
	std::vector<XwaVector3> normals;
	std::vector<D3dVertex> indices;
	std::vector<D3dTriangle> triangles;
	std::vector<int> bvhIndices;
	int indexCounter = 0;

	char line[256];
	while (!feof(file)) {
		fgets(line, 256, file);
		// fgets may fail because EOF has been reached, so we need to check
		// again here.
		if (feof(file))
			break;

		if (line[0] == '#')
			continue;

		if (line[0] == 'v' && line[1] == ' ')
		{
			XwaVector3 v;
			// With the D3dRendererHook, we're going to use transform matrices that work on
			// OPT coordinates, so we need to convert OBJ coords into OPT coords and we need
			// to swap Y and Z coordinates:
			sscanf_s(line, "v %f %f %f", &v.x, &v.z, &v.y);
			if (v.y < g_vrGlovesMeshes[gloveIdx].forwardPmeters[profile])
				g_vrGlovesMeshes[gloveIdx].forwardPmeters[profile] = v.y;
			// And now we add the 40.96 scale factor.
			v.x *= METERS_TO_OPT;
			v.y *= METERS_TO_OPT;
			v.z *= METERS_TO_OPT;
			Vector4 V = R * XwaVector3ToVector4(v);
			vertices.push_back(Vector4ToXwaVector3(V));
		}
		if (line[0] == 'v' && line[1] == 'n')
		{
			XwaVector3 n;
			sscanf_s(line, "vn %f %f %f", &n.x, &n.z, &n.y);
			n.normalize();
			Vector4 N = XwaVector3ToVector4(n);
			N.w = 0;
			N = R * N;
			normals.push_back(Vector4ToXwaVector3(N));
		}
		else if (line[0] == 'v' && line[1] == 't')
		{
			XwaTextureVertex t;
			sscanf_s(line, "vt %f %f", &t.u, &t.v);
			// UVs are flipped vertically.
			t.v = 1.0f - t.v;
			g_vrGlovesMeshes[gloveIdx].texCoords.push_back(t);
		}
		else if (line[0] == 'f')
		{
			D3dTriangle tri;
			int v[4], t[4], n[4];
			// vertex/tex/normal
			int items = sscanf_s(line, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
				&v[0], &t[0], &n[0],
				&v[1], &t[1], &n[1],
				&v[2], &t[2], &n[2],
				&v[3], &t[3], &n[3]);
			int edgesCount = items / 3;

			for (int edge = 2; edge < edgesCount; edge++)
			{
				// edge = 2 --> 0, 1, 2
				// edge = 3 --> 0, 2, 3
				D3dVertex index;
				index.iV = v[0] - 1;
				index.iT = t[0] - 1;
				index.iN = n[0] - 1;
				index.c = 0;
				tri.v1 = indices.size();
				indices.push_back(index);

				index.iV = v[edge - 1] - 1;
				index.iT = t[edge - 1] - 1;
				index.iN = n[edge - 1] - 1;
				index.c = 0;
				tri.v2 = indices.size();
				indices.push_back(index);

				index.iV = v[edge] - 1;
				index.iT = t[edge] - 1;
				index.iN = n[edge] - 1;
				index.c = 0;
				tri.v3 = indices.size();
				indices.push_back(index);

				triangles.push_back(tri);

				if (buildBVH)
				{
					bvhIndices.push_back(v[0] - 1);
					bvhIndices.push_back(v[edge - 1] - 1);
					bvhIndices.push_back(v[edge] - 1);

					g_vrGlovesMeshes[gloveIdx].texIndices.push_back(t[0] - 1);
					g_vrGlovesMeshes[gloveIdx].texIndices.push_back(t[edge - 1] - 1);
					g_vrGlovesMeshes[gloveIdx].texIndices.push_back(t[edge] - 1);
				}
			}
		}
	}
	fclose(file);

	log_debug("[DBG] [AC] Loaded %d vertices, %d normals, %d indices, %d triangles",
		vertices.size(), normals.size(), indices.size(), triangles.size());
	g_vrGlovesMeshes[gloveIdx].forwardPmeters[profile] = fabs(g_vrGlovesMeshes[gloveIdx].forwardPmeters[profile]);
	log_debug("[DBG] [AC] forwardPmeters: %0.3f", g_vrGlovesMeshes[gloveIdx].forwardPmeters[profile]);

	ID3D11Device* device = _deviceResources->_d3dDevice;

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;

	if (profile == VRGlovesProfile::NEUTRAL)
	{
		// Create the index and triangle buffers
		initialData.pSysMem = indices.data();
		device->CreateBuffer(&CD3D11_BUFFER_DESC(indices.size() * sizeof(D3dVertex), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData,
			                 &(g_vrGlovesMeshes[gloveIdx].vertexBuffer));
		initialData.pSysMem = triangles.data();
		device->CreateBuffer(&CD3D11_BUFFER_DESC(triangles.size() * sizeof(D3dTriangle), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData,
			                 &(g_vrGlovesMeshes[gloveIdx].indexBuffer));

		if (buildBVH)
			g_vrGlovesMeshes[gloveIdx].bvh = BuildBVH(vertices, bvhIndices);
	}

	// Create the mesh buffers and SRVs
	initialData.pSysMem = vertices.data();
	device->CreateBuffer(&CD3D11_BUFFER_DESC(vertices.size() * sizeof(XwaVector3), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData,
		&(g_vrGlovesMeshes[gloveIdx].meshVerticesBuffers[profile]));

	device->CreateShaderResourceView(g_vrGlovesMeshes[gloveIdx].meshVerticesBuffers[profile],
		&CD3D11_SHADER_RESOURCE_VIEW_DESC(g_vrGlovesMeshes[gloveIdx].meshVerticesBuffers[profile], DXGI_FORMAT_R32G32B32_FLOAT, 0, vertices.size()),
		&(g_vrGlovesMeshes[gloveIdx].meshVerticesSRVs[profile]));

	if (profile == VRGlovesProfile::NEUTRAL)
	{
		initialData.pSysMem = normals.data();
		device->CreateBuffer(&CD3D11_BUFFER_DESC(normals.size() * sizeof(XwaVector3), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData,
			&(g_vrGlovesMeshes[gloveIdx].meshNormalsBuffer));

		device->CreateShaderResourceView(g_vrGlovesMeshes[gloveIdx].meshNormalsBuffer,
			&CD3D11_SHADER_RESOURCE_VIEW_DESC(g_vrGlovesMeshes[gloveIdx].meshNormalsBuffer, DXGI_FORMAT_R32G32B32_FLOAT, 0, normals.size()),
			&(g_vrGlovesMeshes[gloveIdx].meshNormalsSRV));

		initialData.pSysMem = g_vrGlovesMeshes[gloveIdx].texCoords.data();
		device->CreateBuffer(&CD3D11_BUFFER_DESC(g_vrGlovesMeshes[gloveIdx].texCoords.size() * sizeof(XwaTextureVertex),
			D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &(g_vrGlovesMeshes[gloveIdx].meshTexCoordsBuffer));
		device->CreateShaderResourceView(g_vrGlovesMeshes[gloveIdx].meshTexCoordsBuffer,
			&CD3D11_SHADER_RESOURCE_VIEW_DESC(g_vrGlovesMeshes[gloveIdx].meshTexCoordsBuffer, DXGI_FORMAT_R32G32_FLOAT, 0,
			g_vrGlovesMeshes[gloveIdx].texCoords.size()), &(g_vrGlovesMeshes[gloveIdx].meshTexCoordsSRV));
	}

	return triangles.size();
}

/// <summary>
/// Creates a rectangle in OPT coords on the X-Z plane (Y = 0, meaning
/// that the rectangle is at a fixed depth and facing the camera).
/// An optional displacement vector can be specified to translate the mesh.
/// tris must have at least 2 elements.
/// meshVertices must have at least 4 elements.
/// texCoords must have at least 4 elements.
/// </summary>
void EffectsRenderer::CreateRectangleMesh(
	float widthMeters,
	float heightMeters,
	XwaVector3 dispMeters,
	/* out */ D3dTriangle* tris,
	/* out */ XwaVector3* meshVertices,
	/* out */ XwaTextureVertex* texCoords,
	/* out */ ComPtr<ID3D11Buffer>& vertexBuffer,
	/* out */ ComPtr<ID3D11Buffer>& indexBuffer,
	/* out */ ComPtr<ID3D11Buffer>& meshVerticesBuffer,
	/* out */ ComPtr<ID3D11ShaderResourceView>& meshVerticesSRV,
	/* out */ ComPtr<ID3D11Buffer>& texCoordsBuffer,
	/* out */ ComPtr<ID3D11ShaderResourceView>& texCoordsSRV)
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	constexpr int numVertices = 4;
	D3dVertex _vertices[numVertices];

	// The OPT/D3dHook system uses an indexing scheme, but I don't need it
	// here because the keyboard is just two triangles, so let's just use
	// an "identity function":
	_vertices[0] = { 0, 0, 0, 0 };
	_vertices[1] = { 1, 0, 1, 0 };
	_vertices[2] = { 2, 0, 2, 0 };
	_vertices[3] = { 3, 0, 3, 0 };

	tris[0] = { 0, 1, 2 };
	tris[1] = { 0, 2, 3 };

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;

	initialData.pSysMem = _vertices;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(numVertices * sizeof(D3dVertex), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &vertexBuffer);

	initialData.pSysMem = tris;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(g_vrKeybNumTriangles * sizeof(D3dTriangle), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &indexBuffer);

	// Create the mesh
	const float halfW = widthMeters  / 2.0f * METERS_TO_OPT;
	const float halfH = heightMeters / 2.0f * METERS_TO_OPT;
	XwaVector3  disp  = dispMeters * METERS_TO_OPT;
	// Reference mesh: this displays a rectangle on the center of the A-Wing dashboard
	/*g_vrKeybMeshVertices[0] = { -10.0f, -25.0f, 30.0f };
	g_vrKeybMeshVertices[1] = {  10.0f, -25.0f, 30.0f };
	g_vrKeybMeshVertices[2] = {  10.0f, -25.0f, 18.0f };
	g_vrKeybMeshVertices[3] = { -10.0f, -25.0f, 18.0f };*/
	meshVertices[0] = { -halfW + disp.x, disp.y,  halfH + disp.z }; // Up-Left
	meshVertices[1] = {  halfW + disp.x, disp.y,  halfH + disp.z }; // Up-Right
	meshVertices[2] = {  halfW + disp.x, disp.y, -halfH + disp.z }; // Dn-Right
	meshVertices[3] = { -halfW + disp.x, disp.y, -halfH + disp.z }; // Dn-Left

	initialData.pSysMem = meshVertices;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(numVertices * sizeof(XwaVector3),
		D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &meshVerticesBuffer);
	device->CreateShaderResourceView(meshVerticesBuffer,
		&CD3D11_SHADER_RESOURCE_VIEW_DESC(meshVerticesBuffer, DXGI_FORMAT_R32G32B32_FLOAT, 0, numVertices), &meshVerticesSRV);

	// Create the UVs
	constexpr int texCoordsCount = 4;
	texCoords[0] = { 0, 0 };
	texCoords[1] = { 1, 0 };
	texCoords[2] = { 1, 1 };
	texCoords[3] = { 0, 1 };

	initialData.pSysMem = texCoords;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(texCoordsCount * sizeof(XwaTextureVertex),
		D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &texCoordsBuffer);
	device->CreateShaderResourceView(texCoordsBuffer,
		&CD3D11_SHADER_RESOURCE_VIEW_DESC(texCoordsBuffer, DXGI_FORMAT_R32G32_FLOAT, 0, texCoordsCount), &texCoordsSRV);
}

/// <summary>
/// Creates a "flat" rectangle (i.e. lying on the X-Y plane in OPT coords with Z = 0,
/// meaning that it's aligned with the global starfield horizon).
/// An optional displacement vector can be specified to translate the mesh.
/// tris must have at least 2 elements.
/// meshVertices must have at least 4 elements.
/// texCoords must have at least 4 elements.
/// </summary>
void EffectsRenderer::CreateFlatRectangleMesh(
	float widthMeters,
	float depthMeters,
	XwaVector3 dispMeters,
	/* out */ D3dTriangle* tris,
	/* out */ XwaVector3* meshVertices,
	/* out */ XwaTextureVertex* texCoords,
	/* out */ ComPtr<ID3D11Buffer>& vertexBuffer,
	/* out */ ComPtr<ID3D11Buffer>& indexBuffer,
	/* out */ ComPtr<ID3D11Buffer>& meshVerticesBuffer,
	/* out */ ComPtr<ID3D11ShaderResourceView>& meshVerticesSRV,
	/* out */ ComPtr<ID3D11Buffer>& texCoordsBuffer,
	/* out */ ComPtr<ID3D11ShaderResourceView>& texCoordsSRV)
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	constexpr int numVertices = 4;
	D3dVertex _vertices[numVertices];

	// The OPT/D3dHook system uses an indexing scheme, but I don't need it here,
	// so let's just use an "identity function":
	_vertices[0] = { 0, 0, 0, 0 };
	_vertices[1] = { 1, 0, 1, 0 };
	_vertices[2] = { 2, 0, 2, 0 };
	_vertices[3] = { 3, 0, 3, 0 };

	tris[0] = { 0, 1, 2 };
	tris[1] = { 0, 2, 3 };

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;

	initialData.pSysMem = _vertices;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(numVertices * sizeof(D3dVertex), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &vertexBuffer);

	initialData.pSysMem = tris;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(g_vrKeybNumTriangles * sizeof(D3dTriangle), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &indexBuffer);

	// Create the mesh
	const float halfW = widthMeters / 2.0f * METERS_TO_OPT;
	const float halfD = depthMeters / 2.0f * METERS_TO_OPT;
	XwaVector3  disp  = dispMeters * METERS_TO_OPT;
	meshVertices[0] = { -halfW + disp.x,  halfD + disp.y, disp.z }; // Left-Back
	meshVertices[1] = {  halfW + disp.x,  halfD + disp.y, disp.z }; // Right-Back
	meshVertices[2] = {  halfW + disp.x, -halfD + disp.y, disp.z }; // Right-Front
	meshVertices[3] = { -halfW + disp.x, -halfD + disp.y, disp.z }; // Left-Front

	initialData.pSysMem = meshVertices;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(numVertices * sizeof(XwaVector3),
		D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &meshVerticesBuffer);
	device->CreateShaderResourceView(meshVerticesBuffer,
		&CD3D11_SHADER_RESOURCE_VIEW_DESC(meshVerticesBuffer, DXGI_FORMAT_R32G32B32_FLOAT, 0, numVertices), &meshVerticesSRV);

	// Create the UVs
	constexpr int texCoordsCount = 4;
	texCoords[0] = { 1, 0 };
	texCoords[1] = { 1, 1 };
	texCoords[2] = { 0, 1 };
	texCoords[3] = { 0, 0 };

	initialData.pSysMem = texCoords;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(texCoordsCount * sizeof(XwaTextureVertex),
		D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &texCoordsBuffer);
	device->CreateShaderResourceView(texCoordsBuffer,
		&CD3D11_SHADER_RESOURCE_VIEW_DESC(texCoordsBuffer, DXGI_FORMAT_R32G32_FLOAT, 0, texCoordsCount), &texCoordsSRV);
}

/// <summary>
/// Creates a quarter of a vertical cylinder (a "side") consisting of 6 segments.
/// Each segment has constant depth (Y) and goes vertically along the X-Z plane
/// measuring heightMeters. The radius of the cylinder is widthMeters / 2.
/// An optional displacement vector can be specified to translate the mesh.
/// tris must have at least 12 elements.
/// meshVertices must have at least 14 elements.
/// texCoords must have at least 14 elements.
/// </summary>
void EffectsRenderer::CreateCylinderSideMesh(
	float widthMeters,
	float heightMeters,
	XwaVector3 dispMeters,
	/* out */ D3dTriangle* tris,
	/* out */ XwaVector3* meshVertices,
	/* out */ XwaTextureVertex* texCoords,
	/* out */ ComPtr<ID3D11Buffer>& vertexBuffer,
	/* out */ ComPtr<ID3D11Buffer>& indexBuffer,
	/* out */ ComPtr<ID3D11Buffer>& meshVerticesBuffer,
	/* out */ ComPtr<ID3D11ShaderResourceView>& meshVerticesSRV,
	/* out */ ComPtr<ID3D11Buffer>& texCoordsBuffer,
	/* out */ ComPtr<ID3D11ShaderResourceView>& texCoordsSRV)
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	constexpr int numVertices = 14;
	D3dVertex _vertices[numVertices];

	// The OPT/D3dHook system uses an indexing scheme, but I don't need it here,
	// so let's just use an "identity function":
	for (int i = 0; i < numVertices; i++)
		_vertices[i] = { i, 0, i, 0 };

	for (int i = 0, j = 0; i < s_numCylTriangles; i += 2, j += 2)
	{
		//int j = i * 4;
		tris[i + 0] = { j + 0, j + 1, j + 2 };
		tris[i + 1] = { j + 2, j + 1, j + 3 };
		/*log_debug("[DBG] [CUBE] tri[%d] = {%d, %d, %d}",
			i, tris[i].v1, tris[i].v2, tris[i].v3);
		log_debug("[DBG] [CUBE] tri[%d] = {%d, %d, %d}",
			i+1, tris[i+1].v1, tris[i+1].v2, tris[i+1].v3);*/
	}

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;

	initialData.pSysMem = _vertices;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(numVertices * sizeof(D3dVertex),
		D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &vertexBuffer);

	initialData.pSysMem = tris;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(s_numCylTriangles * sizeof(D3dTriangle),
		D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &indexBuffer);

	// Create the mesh
	const float halfW = widthMeters  / 2.0f * METERS_TO_OPT;
	const float halfH = heightMeters / 2.0f * METERS_TO_OPT;
	XwaVector3  disp  = dispMeters * METERS_TO_OPT;
	for (int n = 0, i = 0; n <= 6; n++, i += 2)
	{
		float ang = 135.0f - 15.0f * n;
		float xn =  halfW * cos(ang * DEG_TO_RAD);
		float yn = -halfW * sin(ang * DEG_TO_RAD);
		meshVertices[i + 0] = { xn + disp.x, yn + disp.y,  halfH + disp.z };
		meshVertices[i + 1] = { xn + disp.x, yn + disp.y, -halfH + disp.z };

		// Create the UVs
		float un = 1.0f - n / 6.0f;
		texCoords[i + 0] = { un, 0 };
		texCoords[i + 1] = { un, 1 };

		//log_debug("[DBG] [CUBE] verts[%d, %d]: (%0.3f, %0.3f):%0.3f", i, i+1, xn, yn, un);
	}

	initialData.pSysMem = meshVertices;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(numVertices * sizeof(XwaVector3),
		D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &meshVerticesBuffer);
	device->CreateShaderResourceView(meshVerticesBuffer,
		&CD3D11_SHADER_RESOURCE_VIEW_DESC(meshVerticesBuffer, DXGI_FORMAT_R32G32B32_FLOAT, 0, numVertices), &meshVerticesSRV);

	initialData.pSysMem = texCoords;
	device->CreateBuffer(&CD3D11_BUFFER_DESC(numVertices * sizeof(XwaTextureVertex),
		D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &texCoordsBuffer);
	device->CreateShaderResourceView(texCoordsBuffer,
		&CD3D11_SHADER_RESOURCE_VIEW_DESC(texCoordsBuffer, DXGI_FORMAT_R32G32_FLOAT, 0, numVertices), &texCoordsSRV);
}

void EffectsRenderer::CreateVRMeshes()
{
	int res;

	// *************************************************
	// Rects used to display VR dots and other things
	// *************************************************
	log_debug("[DBG] [AC] Creating VR Dot buffers");
	CreateRectangleMesh(0.017f, 0.017f, { 0, 0, 0 },
		g_vrDotTriangles, g_vrDotMeshVertices, g_vrDotTextureCoords,
		_vrDotVertexBuffer, _vrDotIndexBuffer,
		_vrDotMeshVerticesBuffer, _vrDotMeshVerticesSRV,
		_vrDotMeshTexCoordsBuffer, _vrDotMeshTexCoordsSRV);

	// Load the green circles used to display the intersection point
	res = LoadDATImage(".\\Effects\\ActiveCockpit\\Textures.dat", 2, 0, _vrGreenCirclesSRV.GetAddressOf());
	if (SUCCEEDED(res))
	{
		log_debug("[DBG] [AC] VR green circles loaded!");
	}
	else
	{
		log_debug("[DBG] [AC] Could not load texture for green circles");
	}

	// The VR keyboard and gloves are only displayed when the Active Cockpit is on.
	if (!g_bActiveCockpitEnabled)
		return;

	// *************************************************
	// Gloves
	// *************************************************
	Matrix4 R;
	R.identity();
	R.rotateX(45.0f); // The VR controllers tilt the objects a bit
	for (int i = 0; i < 2; i++)
	{
		for (int profile = 0; profile < VRGlovesProfile::MAX; profile++)
		{
			g_vrGlovesMeshes[i].numTriangles = LoadOBJ(i, R, g_GlovesProfileNames[i][profile], profile, true);
			if (g_vrGlovesMeshes[i].numTriangles > 0)
			{
				log_debug("[DBG] [AC] Loaded OBJ: %s", g_GlovesProfileNames[i][profile]);
			}
		}

		res = LoadDATImage(g_vrGlovesMeshes[i].texName,
			g_vrGlovesMeshes[i].texGroupId,
			g_vrGlovesMeshes[i].texImageId,
			g_vrGlovesMeshes[i].textureSRV.GetAddressOf());
		if (SUCCEEDED(res))
		{
			log_debug("[DBG] [AC] Glove texture successfully loaded!");
		}
		else
		{
			log_debug("[DBG] [AC] Could not load texture for Glove %d: [%s]-%d-%d",
				i, g_vrGlovesMeshes[i].texName, g_vrGlovesMeshes[i].texGroupId, g_vrGlovesMeshes[i].texImageId);
		}
		log_debug("[DBG] [AC] VR glove hand buffers CREATED");
	}

	// *************************************************
	// VR Keyboard
	// *************************************************
	log_debug("[DBG] [AC] Creating virtual keyboard buffers");
	const float ratio = g_vrKeybState.fPixelWidth / g_vrKeybState.fPixelHeight;
	const float height = g_vrKeybState.fMetersWidth / ratio;
	const float forward = g_vrGlovesMeshes[0].forwardPmeters[VRGlovesProfile::POINT];
	CreateRectangleMesh(g_vrKeybState.fMetersWidth, height, { 0, -forward, height / 2.0f },
		g_vrKeybTriangles, g_vrKeybMeshVertices, g_vrKeybTextureCoords,
		_vrKeybVertexBuffer, _vrKeybIndexBuffer,
		_vrKeybMeshVerticesBuffer, _vrKeybMeshVerticesSRV,
		_vrKeybMeshTexCoordsBuffer, _vrKeybMeshTexCoordsSRV);

	res = LoadDATImage(g_vrKeybState.sImageName, g_vrKeybState.iGroupId, g_vrKeybState.iImageId, _vrKeybTextureSRV.GetAddressOf());
	if (SUCCEEDED(res))
	{
		log_debug("[DBG] [AC] VR Keyboard texture successfully loaded!");
	}
	else
	{
		log_debug("[DBG] [AC] Could not load texture for VR Keyboard [%s]-[%d]-[%d]",
			g_vrKeybState.sImageName, g_vrKeybState.iGroupId, g_vrKeybState.iImageId);
	}
	log_debug("[DBG] [AC] Virtual keyboard buffers CREATED");

	//_vrKeybVertexBuffer->AddRef();
	//_vrKeybIndexBuffer->AddRef();
	//_vrKeybMeshVerticesBuffer->AddRef();
	//_vrKeybMeshVerticesView->AddRef();
	//_vrKeybMeshTextureCoordsBuffer->AddRef();
	//_vrKeybMeshTextureCoordsView->AddRef();

	// TODO: Check for memory leaks. Should I Release() these resources?
}

void EffectsRenderer::CreateBackgroundMeshes()
{
	if (!g_bReplaceBackdrops)
		return;

	log_debug("[DBG] [CUBE] Creating Background Meshes");

	D3dTriangle tris[12];
	XwaVector3 meshVertices[14];
	XwaTextureVertex texCoords[14];

	// CreateFlatRectangleMesh() scales the dimensions by METERS_TO_OPT, so we can just specify meters:
	// Create the caps of the sky cylinder:
	CreateFlatRectangleMesh(BACKGROUND_CUBE_SIZE_METERS, BACKGROUND_CUBE_SIZE_METERS,
		{ 0, 0, 0 }, tris, meshVertices, texCoords,
		_bgCapVertexBuffer, _bgCapIndexBuffer,
		_bgCapMeshVerticesBuffer, _bgCapMeshVerticesSRV,
		_bgCapTexCoordsBuffer, _bgCapMeshTexCoordsSRV);

	// Create the sides of the cube (we're going to use this for planets and other flat background surfaces.
	CreateRectangleMesh(BACKGROUND_CUBE_SIZE_METERS, BACKGROUND_CUBE_SIZE_METERS,
		{ 0, 0, 0 }, tris, meshVertices, texCoords,
		_bgSideVertexBuffer, _bgSideIndexBuffer,
		_bgSideMeshVerticesBuffer, _bgSideMeshVerticesSRV,
		_bgSideTexCoordsBuffer, _bgSideMeshTexCoordsSRV);

	// Create the sides of the cylinder
	CreateCylinderSideMesh(BACKGROUND_CUBE_SIZE_METERS, BACKGROUND_CYL_RATIO * BACKGROUND_CUBE_SIZE_METERS,
		{ 0, 0, 0 }, tris, meshVertices, texCoords,
		_bgCylVertexBuffer, _bgCylIndexBuffer,
		_bgCylMeshVerticesBuffer, _bgCylMeshVerticesSRV,
		_bgCylTexCoordsBuffer, _bgCylMeshTexCoordsSRV);
}

void EffectsRenderer::CreateBackdropIdMapping()
{
	if (!g_bReplaceBackdrops)
		return;

	log_debug("[DBG] [CUBE] Creating mission -> backdrop mapping");
	if (!InitDATReader()) // This call is idempotent and does nothing when DATReader is already loaded
		log_debug("[DBG] [CUBE] Could not load DATReader!");

	g_BackdropIdToGroupId.clear();
	LoadDATFile(".\\ResData\\Planet2.dat");
	int numGroups = GetDATGroupCount();
	log_debug("[DBG] [CUBE] numGroups: %d", numGroups);
	short* groups = new short[numGroups];
	GetDATGroupList(groups);
	for (int i = 0; i < numGroups; i++)
	{
		int backdropId = i + 1;
		// Backdrop #25 does not exist:
		if (backdropId >= 25) backdropId++;
		//log_debug("[DBG] [CUBE]   backdropId[%d] = %d", backdropId, groups[i]);
		g_BackdropIdToGroupId[backdropId] = groups[i];
	}
	delete[] groups;

	// Populate the starfield map
	g_StarfieldGroupIdImageIdMap.clear();
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6104, 0)] = true;

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6079, 2)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6079, 3)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6079, 4)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6079, 5)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6079, 6)] = true;

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6034, 3)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6034, 4)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6034, 5)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6034, 6)] = true;

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6042, 1)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6042, 2)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6042, 3)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6042, 4)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6042, 5)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6042, 6)] = true;

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6094, 1)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6094, 2)] = true; // Cap
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6094, 3)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6094, 4)] = true; // Cap
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6094, 5)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6094, 6)] = true; // Cap

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6083, 2)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6083, 3)] = true; // Cap
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6083, 5)] = true;

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6084, 1)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6084, 2)] = true; // Cap
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6084, 4)] = true;
	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6084, 6)] = true; // Cap

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6094, 2)] = true; // Cap

	g_StarfieldGroupIdImageIdMap[MakeKeyFromGroupIdImageId(6104, 5)] = true; // Cap
}

void EffectsRenderer::CreateShaders() {
	ID3D11Device* device = _deviceResources->_d3dDevice;

	D3dRenderer::CreateShaders();

	//StartCascadedShadowMap();

	CreateVRMeshes();

	CreateBackgroundMeshes();
	CreateBackdropIdMapping();
}

void ResetGimbalLockFix()
{
	CurPlayerYawRateDeg   = 0.0f;
	CurPlayerPitchRateDeg = 0.0f;
	CurPlayerRollRateDeg  = 0.0f;
}

void ApplyGimbalLockFix(float elapsedTime, CraftInstance *craftInstance)
{
	if (g_pSharedDataJoystick == NULL || !g_SharedMemJoystick.IsDataReady())
		return;
	bool RMouseDown = GetAsyncKeyState(VK_RBUTTON);
	bool CtrlKey = (GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000;
	float DesiredYawRate_s = 0, DesiredPitchRate_s = 0, DesiredRollRate_s = 0;

	const float RollFromYawScale = g_fRollFromYawScale;
	// How can I tell if the current ship doesn't roll when applying yaw?

	/*log_debug("[DBG] IsDocking: %d, DoneDockingCount: %d, BeingBoarded: %d, DoneBoardedCount: %d",
		craftInstance->IsDocking, craftInstance->DoneDockingCount,
		craftInstance->BeingBoarded, craftInstance->DoneBoardedCount);*/
	/*log_debug("[DBG] specialTimerOrState: %d, PickedUpObjectIndex: %d, currentManr: %d, CraftState: %d",
		craftInstance->specialTimerOrState, craftInstance->PickedUpObjectIndex,
		craftInstance->currentManr, craftInstance->CraftState);*/
	// log_debug("[DBG] CraftType: %u", craftInstance->CraftType); // This is the 0-based slot# of this craft
	//log_debug("[DBG] RollingYawPercentage: %d",
	//	CraftDefinitionTable[craftInstance->CraftType].RollingYawPercentage); // This don't work: T/F is reported as 0

	// This calibration is wrt to the Xwing and some educated guessing
	float TurnRate = craftInstance->YawRate / 10240.0f;
	// Modulate the turn rate according to the current throttle
	float throttle = craftInstance->EngineThrottleInput / 65535.0f;
	float DesiredTurnRateScale = 1.0f;
	if (g_bThrottleModulationEnabled)
	{
		if (throttle < 0.333f)
			DesiredTurnRateScale = lerp(g_fTurnRateScaleThr_0, 1.0f, throttle / 0.333f);
		else
			DesiredTurnRateScale = lerp(1.0f, g_fTurnRateScaleThr_100, (throttle - 0.333f) / 0.667f);
	}
	float TurnRateScaleDelta = DesiredTurnRateScale - g_fTurnRateScale;
	// Provide a smooth transition between the current turn rate and the desired turn rate
	g_fTurnRateScale += elapsedTime * g_fMaxTurnAccelRate_s * TurnRateScaleDelta;

	TurnRate *= g_fTurnRateScale;
	const float MaxYawRate_s   = TurnRate * g_fMaxYawRate_s;
	const float MaxPitchRate_s = TurnRate * g_fMaxPitchRate_s;
	const float MaxRollRate_s  = TurnRate * g_fMaxRollRate_s;

	DesiredPitchRate_s = g_pSharedDataJoystick->JoystickPitch * MaxPitchRate_s;
	if (g_config.JoystickEmul)
	{
		if (!RMouseDown && !CtrlKey)
		{
			DesiredYawRate_s = g_pSharedDataJoystick->JoystickYaw * MaxYawRate_s;
			// Apply a little roll when yaw is applied
			DesiredRollRate_s = RollFromYawScale * g_pSharedDataJoystick->JoystickYaw * MaxRollRate_s;
		}
		else
		{
			DesiredRollRate_s = g_bEnableRudder ? -g_pSharedDataJoystick->JoystickYaw * MaxRollRate_s : 0.0f;
		}
	}
	else
	{
		DesiredYawRate_s  =  g_pSharedDataJoystick->JoystickYaw * MaxYawRate_s;
		DesiredRollRate_s = g_bEnableRudder ? -g_pSharedDataJoystick->JoystickRoll * MaxRollRate_s : 0.0f;
		// Add a little roll when yaw is applied
		DesiredRollRate_s += RollFromYawScale * g_pSharedDataJoystick->JoystickYaw * MaxRollRate_s;
	}

	const float DeltaYaw   = DesiredYawRate_s   - CurPlayerYawRateDeg;
	const float DeltaPitch = DesiredPitchRate_s - CurPlayerPitchRateDeg;
	const float DeltaRoll  = DesiredRollRate_s  - CurPlayerRollRateDeg;

	// Accumulate the joystick input
	CurPlayerYawRateDeg   += elapsedTime * g_fYawAccelRate_s   * DeltaYaw;
	CurPlayerPitchRateDeg += elapsedTime * g_fPitchAccelRate_s * DeltaPitch;
	CurPlayerRollRateDeg  += elapsedTime * g_fRollAccelRate_s  * DeltaRoll;

	//g_PlayerYawDeg   = clamp(g_PlayerYawDeg,   -MaxYawRate_s,   MaxYawRate_s);
	//g_PlayerPitchDeg = clamp(g_PlayerPitchDeg, -MaxPitchRate_s, MaxPitchRate_s);
	//g_PlayerRollDeg  = clamp(g_PlayerRollDeg,  -MaxRollRate_s,  MaxRollRate_s);

	//log_debug("[DBG] CurYPRRateInc: %0.3f, %0.3f, %0.3f", CurYawRateInc_s, CurRollRateInc_s, CurPitchRateInc_s);
	//log_debug("[DBG] elapsed: %0.3f, g_PlayerYPR: %0.3f, %0.3f, %0.3f",
	//	elapsedTime, g_PlayerYawDeg, g_PlayerPitchDeg, g_PlayerRollDeg);

	//Vector4 Rs, Us, Fs;
	//Matrix4 H = TestShipOrientation(Rs, Us, Fs, false, false);
	//log_debug("[DBG] joystick ypr: %0.3f, %0.3f, %0.3f", g_PlayerYawDeg, g_PlayerPitchDeg, g_PlayerRollDeg);
	InitializePlayerYawPitchRoll();
	ApplyYawPitchRoll(elapsedTime * CurPlayerYawRateDeg, elapsedTime * CurPlayerPitchRateDeg, elapsedTime * CurPlayerRollRateDeg);
}

void EffectsRenderer::SceneBegin(DeviceResources* deviceResources)
{
	D3dRenderer::SceneBegin(deviceResources);

#ifdef DISABLED
	{
		if (g_bDumpOptNodes)
		{
			int numLights = *s_XwaGlobalLightsCount;
			log_debug("[DBG] ------------------------------");
			for (int i = 0; i < numLights; i++)
			{
				log_debug("[DBG] light: %d: [%0.3f, %0.3f, %0.3f]",
					i,
					s_XwaGlobalLights[i].PositionX / 32768.0f,
					s_XwaGlobalLights[i].PositionY / 32768.0f,
					s_XwaGlobalLights[i].PositionZ / 32768.0f);
			}
			log_debug("[DBG] ------------------------------");

			constexpr int CraftId_183_9001_1100_ResData_Backdrop = 183;
			const XwaMission* mission = *(XwaMission**)0x09EB8E0;

			for (int i = 0; i < 192; i++)
			{
				int CraftId = mission->FlightGroups[i].CraftId;
				int PlanetId = mission->FlightGroups[i].PlanetId;

				if (CraftId == CraftId_183_9001_1100_ResData_Backdrop && PlanetId < 104)
				{
					short SX = mission->FlightGroups[i].StartPoints->X;
					short SY = mission->FlightGroups[i].StartPoints->Y;
					short SZ = mission->FlightGroups[i].StartPoints->Z;
					int region = mission->FlightGroups[i].StartPointRegions[0];
					int currentRegion = PlayerDataTable[*g_playerIndex].currentRegion;

					int ModelIndex = g_XwaPlanets[PlanetId].ModelIndex;
					//log_debug("[DBG]     ModelIndex: %d", ModelIndex);
					if (ModelIndex < 557)
					{
						int data1 = g_ExeObjectsTable[ModelIndex].DataIndex1;
						int data2 = g_ExeObjectsTable[ModelIndex].DataIndex2;
						Vector3 S = Vector3((float)SX, (float)-SY, (float)SZ);
						S = S.normalize();
						if (data1 >= 9001)
						{
							log_debug("[DBG] [%s], CraftId: %d, PlanetId: %d, S:[%0.3f, %0.3f, %0.3f]",
								mission->FlightGroups[i].Name, CraftId, PlanetId, S.x, S.y, S.z);
							//log_debug("[DBG]     ShipCategory: %d, ObjectCategory: %d",
							//	g_ExeObjectsTable[ModelIndex].ShipCategory,
							//	g_ExeObjectsTable[ModelIndex].ObjectCategory);
							log_debug("[DBG]     region: %d, curRegion: %d, Group-Id: %d-%d",
								region, currentRegion,
								g_ExeObjectsTable[ModelIndex].DataIndex1,
								g_ExeObjectsTable[ModelIndex].DataIndex2);
						}
					}
				}
			}

			g_bDumpOptNodes = false;
		}
	}
#endif

	static float lastTime = g_HiResTimer.global_time_s;
	float now = g_HiResTimer.global_time_s;
	if (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME)
	{
		bool bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
		bool bGunnerTurret = PlayerDataTable[*g_playerIndex].gunnerTurretActive;
		int hyperspacePhase = PlayerDataTable[*g_playerIndex].hyperspacePhase;
		CraftInstance* craftInstance = GetPlayerCraftInstanceSafe();

		// Set either the Cockpit or Gunner Turret POV Offsets into the proper shared memory slots
		// THIS SHOULD BE THE ONLY SPOT WHERE WE WRITE TO g_pSharedDataCockpitLook->POVOffsetX/Y/Z
		if (g_pSharedDataCockpitLook != nullptr)
		{
			if (!bGunnerTurret)
			{
				g_pSharedDataCockpitLook->POVOffsetX = g_CockpitPOVOffset.x;
				g_pSharedDataCockpitLook->POVOffsetY = g_CockpitPOVOffset.y;
				g_pSharedDataCockpitLook->POVOffsetZ = g_CockpitPOVOffset.z;
			}
			else
			{
				g_pSharedDataCockpitLook->POVOffsetX = g_GunnerTurretPOVOffset.x;
				g_pSharedDataCockpitLook->POVOffsetY = g_GunnerTurretPOVOffset.y;
				g_pSharedDataCockpitLook->POVOffsetZ = g_GunnerTurretPOVOffset.z;
			}
		}

		//log_debug("[DBG] viewingFilmState: %d, inMissionFilmState: %d",
		//	*viewingFilmState, *inMissionFilmState);
		// *viewingFilmState is 0 during regular flight and becomes 2 when playing back a movie.
		// Fun fact: the gimbal lock fix is still active when playing back a film, so we can steer
		// while playing a film!
		// *inMissionFilmState is 0 during regular flight, and becomes 1 when recording (I think)

		// 18 when docking, 6 when flying, 0 when flying, 35 when releasing cargo -- unfortunately it stays
		// at 35 after the cargo is released, so we can't really use that...
		//log_debug("[DBG] currentManr: %d", craftInstance->currentManr);

		g_bGimbalLockFixActive = g_bEnableGimbalLockFix && !bExternalCamera && !bGunnerTurret &&
			!(*g_playerInHangar) && hyperspacePhase == 0 &&
#undef NO_STEERING_IN_FILMS // #undef this guy to allow steering in films
#ifdef NO_STEERING_IN_FILMS
			*viewingFilmState == 0 &&
#endif
			// Don't allow this fix when flying a YT-series ship. These ships can pick up cargo and
			// that operation just fails when this fix is on. Also they move funny anyway.
			!g_bYTSeriesShip &&
			// currentManr == 18 when the ship is docking
			//craftInstance != nullptr && craftInstance->currentManr != 18; // &&

			// The speed boost section in the DS2 map becomes broken with the gimbal lock fix.
			// So let's disable the fix in this level for now.
			(*missionIndexLoaded != DEATH_STAR_MISSION_INDEX);

		// DEBUG, print mobileObject->transformMatrix (RUF)
		/*
		ObjectEntry* object = NULL;
		MobileObjectEntry* mobileObject = NULL;
		if (GetPlayerCraftInstanceSafe(&object, &mobileObject) != NULL)
		{
			Vector4 Rs, Us, Fs;
			Rs.y = -mobileObject->transformMatrix.Right_X / 32768.0f;
			Rs.x = -mobileObject->transformMatrix.Right_Y / 32768.0f;
			Rs.z = -mobileObject->transformMatrix.Right_Z / 32768.0f;

			Us.y = mobileObject->transformMatrix.Up_X / 32768.0f;
			Us.x = mobileObject->transformMatrix.Up_Y / 32768.0f;
			Us.z = mobileObject->transformMatrix.Up_Z / 32768.0f;

			Fs.y = mobileObject->transformMatrix.Front_X / 32768.0f;
			Fs.x = mobileObject->transformMatrix.Front_Y / 32768.0f;
			Fs.z = mobileObject->transformMatrix.Front_Z / 32768.0f;

			log_debug("[DBG] RUF: [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f], [%0.3f, %0.3f, %0.3f]",
				Rs.x, Rs.y, Rs.z,
				Us.x, Us.y, Us.z,
				Fs.x, Fs.y, Fs.z);
		}
		*/

		if (g_bGimbalLockFixActive)
		{
			ApplyGimbalLockFix(now - lastTime, craftInstance);
		}
	}
	lastTime = now;

	// Reset any deferred-rendering variables here
	_LaserDrawCommands.clear();
	_TransparentDrawCommands.clear();
	_ShadowMapDrawCommands.clear();
	_bCockpitConstantsCaptured = false;
	_bExteriorConstantsCaptured = false;
	_bShadowsRenderedInCurrentFrame = false;
	_bHangarShadowsRenderedInCurrentFrame = false;
	// Initialize the joystick mesh transform on this frame
	_bJoystickTransformReady = false;
	//_bThrottleTransformReady = false;
	//_bThrottleRotAxisToZPlusReady = false;
	_joystickMeshTransform.identity();
	// Initialize the mesh transform for this frame
	g_OPTMeshTransformCB.MeshTransform.identity();
	deviceResources->InitVSConstantOPTMeshTransform(
		deviceResources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);
	// Initialize the hangar AABB
	_hangarShadowAABB.SetInfinity();

	if (PlayerDataTable->missionTime == 0)
		ApplyCustomHUDColor();

	if (g_bKeepMouseInsideWindow && !g_bInTechRoom)
	{
		// I'm a bit tired of clicking the mouse outside the window when debugging.
		// I. shall. end. this. now.
		SetCursorPos(0, 0);
	}

	_BLASNeedsUpdate = false;
	if (g_bRTEnabled || g_bActiveCockpitEnabled)
	{
		// Restart the TLAS for the frame that is about to begin
		g_iRTMeshesInThisFrame = 0;
		g_GlobalAABB.SetInfinity();
		g_GlobalCentroidAABB.SetInfinity();
		tlasLeaves.clear();
		g_ACtlasLeaves.clear();
		g_TLASMap.clear();
		RTResetMatrixSlotCounter();
		//ShowMatrix4(g_VSMatrixCB.fullViewMat, "SceneBegin");

		if (g_TLASTree != nullptr)
		{
			delete g_TLASTree;
			g_TLASTree = nullptr;
		}
		if (g_bActiveCockpitEnabled && g_ACTLASTree != nullptr)
		{
			delete g_ACTLASTree;
			g_ACTLASTree = nullptr;
		}
	}

	// VR Keyboard and gloves
	g_vrKeybState.bRendered = false;
	g_vrGlovesMeshes[0].rendered = false;
	g_vrGlovesMeshes[1].rendered = false;
	_bDotsbRendered = false;
	_bHUDRendered = false;
	_bBracketsRendered = false;

	// Initialize the OBJ dump file for the current frame
	if ((bD3DDumpOBJEnabled || bHangarDumpOBJEnabled) && g_bDumpSSAOBuffers) {
		// Create the file if it doesn't exist
		if (D3DDumpOBJFile == NULL) {
			char sFileName[128];
			sprintf_s(sFileName, 128, "d3dcapture-%d.obj", D3DOBJFileIdx);
			fopen_s(&D3DDumpOBJFile, sFileName, "wt");
		}
		// Reset the vertex counter and group
		D3DTotalVertices = 1;
		D3DTotalNormals = 1;
		D3DOBJGroup = 1;

		if (D3DDumpLaserOBJFile == NULL) {
			char sFileName[128];
			sprintf_s(sFileName, 128, "d3dlasers-%d.obj", D3DOBJLaserFileIdx);
			fopen_s(&D3DDumpLaserOBJFile, sFileName, "wt");
		}
		D3DTotalLaserVertices = 1;
		D3DTotalLaserTextureVertices = 1;
		D3DOBJLaserGroup = 1;
	}
}

LBVH* EffectsRenderer::BuildBVH(const std::vector<XwaVector3>& vertices, const std::vector<int>& indices)
{
	// All the data for this FaceGroup is ready, let's build the BLAS BVH
	switch (g_BLASBuilderType)
	{
	case BLASBuilderType::BVH2:
		// 3-step LBVH build: BVH2, QBVH conversion, Encoding.
		return LBVH::Build(vertices.data(), vertices.size(), indices.data(), indices.size());

	case BLASBuilderType::QBVH:
		// 2-step LBVH build: QBVH, Encoding.
		return LBVH::BuildQBVH(vertices.data(), vertices.size(), indices.data(), indices.size());

	case BLASBuilderType::FastQBVH:
		// 1-step LBVH build: QBVH is built and encoded in one step.
		return LBVH::BuildFastQBVH(vertices.data(), vertices.size(), indices.data(), indices.size());

	//case BVHBuilderType_Embree:
	//	return LBVH::BuildEmbree(vertices.data(), vertices.size(), indices.data(), indices.size());

	/*case BVHBuilderType_DirectBVH2CPU:
		return LBVH::BuildDirectBVH2CPU(vertices.data(), vertices.size(), indices.data(), indices.size());*/

	case BLASBuilderType::DirectBVH4GPU:
		return LBVH::BuildDirectBVH4GPU(vertices.data(), vertices.size(), indices.data(), indices.size());

	case BLASBuilderType::Online:
		return LBVH::BuildOnline(vertices.data(), vertices.size(), indices.data(), indices.size());

	case BLASBuilderType::PQ:
		return LBVH::BuildPQ(vertices.data(), vertices.size(), indices.data(), indices.size());

	case BLASBuilderType::PLOC:
		return LBVH::BuildPLOC(vertices.data(), vertices.size(), indices.data(), indices.size());
	}
	return nullptr;
}

// Build a single BVH from the contents of the g_LBVHMap and put it into _lbvh.
void EffectsRenderer::BuildSingleBLASFromCurrentBVHMap()
{
	// DEBUG, dump the vertices we saw in the previous frame to a file
#ifdef DISABLED
	if (false) {
		int OBJIndex = 1;
		char sFileName[80];
		sprintf_s(sFileName, 80, ".\\mesh-all.obj");
		FILE* file = NULL;
		fopen_s(&file, sFileName, "wt");

		for (const auto& it : g_LBVHMap)
		{
			int meshIndex = it.first;
			XwaVector3* vertices = (XwaVector3*)it.first; // The mesh key is actually the Vertex array
			MeshData meshData = it.second;

			// This block will cause each mesh to be dumped to a separate file
			/*
			int OBJIndex = 1;
			char sFileName[80];
			sprintf_s(sFileName, 80, ".\\mesh-%x.obj", meshIndex);
			FILE* file = NULL;
			fopen_s(&file, sFileName, "wt");
			*/
			if (file != NULL)
			{
				std::vector<int> indices;
				const FaceGroups& FGs = std::get<0>(meshData);
				for (const auto& FG : FGs)
				{
					OptFaceDataNode_01_Data_Indices* FaceIndices = (OptFaceDataNode_01_Data_Indices*)(FG.first);
					int FacesCount = FG.second;

					for (int faceIndex = 0; faceIndex < FacesCount; faceIndex++) {
						OptFaceDataNode_01_Data_Indices& faceData = FaceIndices[faceIndex];
						int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;
						indices.push_back(faceData.Vertex[0]);
						indices.push_back(faceData.Vertex[1]);
						indices.push_back(faceData.Vertex[2]);
						if (edgesCount == 4) {
							indices.push_back(faceData.Vertex[0]);
							indices.push_back(faceData.Vertex[2]);
							indices.push_back(faceData.Vertex[3]);
						}
					}

					int numTris = indices.size() / 3;
					for (int TriID = 0; TriID < numTris; TriID++)
					{
						int i = TriID * 3;

						XwaVector3 v0 = vertices[indices[i + 0]];
						XwaVector3 v1 = vertices[indices[i + 1]];
						XwaVector3 v2 = vertices[indices[i + 2]];

						//name = "t-" + std::to_string(TriID);
						OBJIndex = DumpTriangle(std::string(""), file, OBJIndex, v0, v1, v2);
					}
				}
				// fclose(file); // Uncomment this line when dumping each mesh to a separate file
			} // if (file != NULL)

		}

		if (file) fclose(file);
	}
#endif

	// Rebuild the whole tree using the current contents of g_LBVHMap
	std::vector<XwaVector3> vertices;
	std::vector<int> indices;
	int TotalVertices = 0;

	for (const auto& it : g_LBVHMap)
	{
		int meshKey = it.first;
		XwaVector3* XwaVertices = (XwaVector3*)meshKey; // The mesh key is actually the Vertex array
		MeshData meshData = it.second;
		const FaceGroups& FGs = std::get<0>(meshData);
		int NumVertices = std::get<1>(meshData);

		// Populate the vertices
		for (int i = 0; i < NumVertices; i++)
		{
			vertices.push_back(XwaVertices[i]);
		}

		// Populate the indices
		for (const auto& FG : FGs)
		{
			OptFaceDataNode_01_Data_Indices* FaceIndices = (OptFaceDataNode_01_Data_Indices*)(FG.first);
			int FacesCount = FG.second;

			for (int faceIndex = 0; faceIndex < FacesCount; faceIndex++) {
				OptFaceDataNode_01_Data_Indices& faceData = FaceIndices[faceIndex];
				int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;
				indices.push_back(faceData.Vertex[0] + TotalVertices);
				indices.push_back(faceData.Vertex[1] + TotalVertices);
				indices.push_back(faceData.Vertex[2] + TotalVertices);
				if (edgesCount == 4) {
					indices.push_back(faceData.Vertex[0] + TotalVertices);
					indices.push_back(faceData.Vertex[2] + TotalVertices);
					indices.push_back(faceData.Vertex[3] + TotalVertices);
				}
			}
		}

		// All the FaceGroups have been added, update the starting offset
		// for the indices
		TotalVertices += NumVertices;
	}

	// All the vertices and indices have been accumulated, the tree can be built now
	if (_lbvh != nullptr)
		delete _lbvh;

	// g_HiResTimer is called here to measure the time it takes to build the BVH. This should
	// not be used during regular flight as it will mess up the animations
//#define BVH_BENCHMARK_MODE
#undef BVH_BENCHMARK_MODE
#ifdef BVH_BENCHMARK_MODE
	g_HiResTimer.GetElapsedTime();
#endif
	_lbvh = BuildBVH(vertices, indices);
#ifdef BVH_BENCHMARK_MODE
	g_HiResTimer.GetElapsedTime();
	float elapsed_s = g_HiResTimer.elapsed_s;
	log_debug("[DBG] [BVH] Elapsed Build Time: %0.6fms", elapsed_s * 1000.0f);
#endif

	int root = _lbvh->nodes[0].rootIdx;
	float totalArea = (float)CalcTotalTreeSAH(_lbvh->nodes);
	log_debug("[DBG] [BVH] Builder: %s:%s, %s, Total SA: %0.6f, total nodes: %d, actual nodes: %d",
		g_sBLASBuilderTypeNames[(int)g_BLASBuilderType], g_bEnableQBVHwSAH ? "SAH" : "Non-SAH",
		g_curOPTLoaded, totalArea,
		_lbvh->numNodes, _lbvh->numNodes - root);

	// These lines are for testing only. They get some stats for the BVH that has been just built
	{
		//BufferTreeNode* tree = new BufferTreeNode(_lbvh->nodes, root);
		//ComputeTreeStats(tree);
		//delete tree;
	}
}

// Builds one BLAS per mesh and populates its corresponding tuple in g_BLASMap
void EffectsRenderer::BuildMultipleBLASFromCurrentBLASMap()
{
	// At least one BLAS needs to be rebuilt in this frame, let's count
	// the total nodes again.
	g_iRTTotalBLASNodesInFrame = 0;

	for (auto& it : g_BLASMap)
	{
		std::vector<XwaVector3>   vertices;
		std::vector<int>          indices;
		int blasID              = it.first;
		BLASData& blasData      = it.second;
		int meshKey             = BLASGetMeshVertices(blasData);
		XwaVector3* XwaVertices = (XwaVector3*)meshKey; // The mesh key is actually the Vertex array
		const int NumVertices   = BLASGetNumVertices(blasData);
		LBVH* bvh               = (LBVH*)BLASGetBVH(blasData);
		const FaceGroups FGs    = BLASGetFaceGroups(blasData); // To make this code uniform, the FG is populated even when the blasData is not coalesced

		// First, let's check if this (mesh, LOD) already has a BVH. If it does, skip it.
		if (bvh != nullptr) {
			// Update the total node count
			g_iRTTotalBLASNodesInFrame += bvh->numNodes;
			continue;
		}

		// DEBUG: Skip meshes we don't care about
#undef DEBUG_RT
#ifdef DEBUG_RT
		if (false)
		{
			auto& debugItem = g_DebugMeshToNameMap[meshKey];
			if (stristr(std::get<0>(debugItem).c_str(), "ImperialStarDestroyer") == NULL)
			{
				// Remove this BVH
				BLASGetBVH(blasData) = nullptr;
				continue;
			}

			// We only care about the ISD after this point, the bridge has 83 vertices
			/*
			if (std::get<1>(debugItem) != 83 && std::get<1>(debugItem) != 205)
			{
				// Remove this BVH
				GetLBVH(meshData) = nullptr;
				continue;
			}
			*/
		}
#endif

		// Populate the vertices
		for (int i = 0; i < NumVertices; i++)
		{
			vertices.push_back(XwaVertices[i]);
		}

		// Populate the indices from all FaceGroups in this entry
		for (const auto& FG : FGs)
		{
			const int facesGroupID = FG.first;
			const int facesCount = FG.second;
			OptFaceDataNode_01_Data_Indices* FaceIndices = (OptFaceDataNode_01_Data_Indices*)facesGroupID;
			for (int faceIndex = 0; faceIndex < facesCount; faceIndex++) {
				OptFaceDataNode_01_Data_Indices& faceData = FaceIndices[faceIndex];
				int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;
				indices.push_back(faceData.Vertex[0]);
				indices.push_back(faceData.Vertex[1]);
				indices.push_back(faceData.Vertex[2]);
				if (edgesCount == 4) {
					indices.push_back(faceData.Vertex[0]);
					indices.push_back(faceData.Vertex[2]);
					indices.push_back(faceData.Vertex[3]);
				}
			}
		}

		bvh = BuildBVH(vertices, indices);
		// Update the total node count
		g_iRTTotalBLASNodesInFrame += bvh->numNodes;
		// Put this bvh back into the g_LBVHMap
		BLASGetBVH(blasData) = bvh;

		// DEBUG
#ifdef DEBUG_RT
		{
			int root = bvh->nodes[0].rootIdx;
			auto& debugItem = g_DebugMeshToNameMap[meshKey];
			log_debug("[DBG] [BVH] %sMultiBuilder: %s:%s, %s, vertCount: %d, FGs: %d, OPTmeshIndex: %d, "
				"ID: 0x%x, total nodes: %d",
				(FGs.size() > 1) ? "[CLS] " : "",
				g_sBVHBuilderTypeNames[g_BVHBuilderType], g_bEnableQBVHwSAH ? "SAH" : "Non-SAH",
				std::get<0>(debugItem).c_str(), // Name of the OPT
				std::get<1>(debugItem),         // vertCount
				FGs.size(),
				std::get<2>(debugItem),			// OPTmeshIndex
				ID,
				bvh->numNodes);
		}
#endif
	}

	g_bRTReAllocateBvhBuffer = (g_iRTTotalBLASNodesInFrame > g_iRTMaxBLASNodesSoFar);
	log_debug("[DBG] [BVH] MultiBuilder: %s:%s g_iRTTotalNumNodesInFrame: %d, g_iRTMaxNumNodesSoFar: %d, Reallocate? %d",
		g_sTLASBuilderTypeNames[(int)g_TLASBuilderType],
		g_sBLASBuilderTypeNames[(int)g_BLASBuilderType],
		g_iRTTotalBLASNodesInFrame, g_iRTMaxBLASNodesSoFar, g_bRTReAllocateBvhBuffer);

	g_iRTMaxBLASNodesSoFar = max(g_iRTTotalBLASNodesInFrame, g_iRTMaxBLASNodesSoFar);
}

void EffectsRenderer::ReAllocateAndPopulateBvhBuffers(const int numNodes)
{
	// Create the buffers for the BVH -- this code path applies for in-flight RT
	auto& resources = _deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	HRESULT hr;

	// (Re-)Create the BVH buffers
	if (g_bRTReAllocateBvhBuffer)
	{
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
#ifdef DEBUG_RT
		log_debug("[DBG] [BVH] [REALLOC] START");
#endif

		if (resources->_RTBvh != nullptr)
		{
			resources->_RTBvh.Release();
			resources->_RTBvh = nullptr;
		}

		if (resources->_RTBvhSRV != nullptr)
		{
			resources->_RTBvhSRV.Release();
			resources->_RTBvhSRV = nullptr;
		}

		desc.ByteWidth = sizeof(BVHNode) * g_iRTMaxBLASNodesSoFar;
		desc.Usage = D3D11_USAGE_DYNAMIC; // CPU: Write, GPU: Read
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(BVHNode);

		hr = device->CreateBuffer(&desc, nullptr, &(resources->_RTBvh));
		if (FAILED(hr)) {
			log_debug("[DBG] [BVH] [REALLOC] Failed when creating BVH buffer: 0x%x", hr);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = g_iRTMaxBLASNodesSoFar;

		hr = device->CreateShaderResourceView(resources->_RTBvh, &srvDesc, &(resources->_RTBvhSRV));
		if (FAILED(hr)) {
			log_debug("[DBG] [BVH] [REALLOC] Failed when creating BVH SRV: 0x%x", hr);
		}
	}

	// Populate the BVH buffer
	if (g_bRTReAllocateBvhBuffer || _BLASNeedsUpdate)
	{
		if (!InTechGlobe())
		{
			D3D11_MAPPED_SUBRESOURCE map;
			ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
			hr = context->Map(resources->_RTBvh.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

			if (SUCCEEDED(hr))
			{
				uint8_t* base_ptr = (uint8_t*)map.pData;
				int BaseNodeOffset = 0;
				for (auto& it : g_BLASMap)
				{
					BLASData& blasData = it.second;
					LBVH* bvh = (LBVH*)BLASGetBVH(blasData);
					if (bvh != nullptr)
					{
#ifdef DEBUG_RT
						if (BaseNodeOffset >= g_iRTMaxBLASNodesSoFar ||
							BaseNodeOffset + bvh->numNodes > g_iRTMaxBLASNodesSoFar)
						{
							log_debug("[DBG] [BVH] [REALLOC] ERROR: BaseNodeOffset: %d, numNodes: %d, addition: %d, "
								"g_iRTMaxBLASNodesSoFar: %d",
								BaseNodeOffset, bvh->numNodes, BaseNodeOffset + bvh->numNodes, g_iRTMaxBLASNodesSoFar);
						}
#endif
						// Save the location where this BLAS begins
						BLASGetBaseNodeOffset(blasData) = BaseNodeOffset;
						// Populate the buffer itself
						memcpy(base_ptr, bvh->nodes, sizeof(BVHNode) * bvh->numNodes);
						base_ptr += sizeof(BVHNode) * bvh->numNodes;
						BaseNodeOffset += bvh->numNodes;
					}
					else
					{
						BLASGetBaseNodeOffset(blasData) = -1;
					}
				}
				context->Unmap(resources->_RTBvh.Get(), 0);
			}
			else
				log_debug("[DBG] [BVH] [REALLOC] Failed when mapping BVH nodes: 0x%x", hr);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE map;
			ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
			hr = context->Map(resources->_RTBvh.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

			if (SUCCEEDED(hr))
			{
				uint8_t* base_ptr = (uint8_t*)map.pData;
				int BaseNodeOffset = 0;
				for (auto& it : g_LBVHMap)
				{
					MeshData& meshData = it.second;
					LBVH* bvh = (LBVH*)GetLBVH(meshData);
					if (bvh != nullptr)
					{
#ifdef DEBUG_RT
						if (BaseNodeOffset >= g_iRTMaxBLASNodesSoFar ||
							BaseNodeOffset + bvh->numNodes > g_iRTMaxBLASNodesSoFar)
						{
							log_debug("[DBG] [BVH] [REALLOC] ERROR: BaseNodeOffset: %d, numNodes: %d, addition: %d, "
								"g_iRTMaxBLASNodesSoFar: %d",
								BaseNodeOffset, bvh->numNodes, BaseNodeOffset + bvh->numNodes, g_iRTMaxBLASNodesSoFar);
						}
#endif
						// Save the location where this BLAS begins
						GetBaseNodeOffset(meshData) = BaseNodeOffset;
						// Populate the buffer itself
						memcpy(base_ptr, bvh->nodes, sizeof(BVHNode) * bvh->numNodes);
						base_ptr += sizeof(BVHNode) * bvh->numNodes;
						BaseNodeOffset += bvh->numNodes;
					}
					else
					{
						GetBaseNodeOffset(meshData) = -1;
					}
				}
				context->Unmap(resources->_RTBvh.Get(), 0);
			}
			else
				log_debug("[DBG] [BVH] [REALLOC] Failed when mapping BVH nodes: 0x%x", hr);
		}
	}

#ifdef DEBUG_RT
	log_debug("[DBG] [BVH] [REALLOC] EXIT");
#endif
}

void EffectsRenderer::ReAllocateAndPopulateTLASBvhBuffers()
{
	// Create the buffers for the BVH -- this code path applies for in-flight RT
	auto& resources = _deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	HRESULT hr;

	if (g_TLASTree == nullptr)
		return;

	const int numNodes = g_TLASTree->numNodes;
	if (numNodes == 0)
		return;

	const bool bReallocate = (numNodes > g_iRTMaxTLASNodesSoFar);
	g_iRTMaxTLASNodesSoFar = max(g_iRTMaxTLASNodesSoFar, numNodes);

	// (Re-)Create the BVH buffers
	if (bReallocate)
	{
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		if (resources->_RTTLASBvh != nullptr)
		{
			resources->_RTTLASBvh.Release();
			resources->_RTTLASBvh = nullptr;
		}

		if (resources->_RTTLASBvhSRV != nullptr)
		{
			resources->_RTTLASBvhSRV.Release();
			resources->_RTTLASBvhSRV = nullptr;
		}

		desc.ByteWidth = sizeof(BVHNode) * g_iRTMaxTLASNodesSoFar;
		desc.Usage = D3D11_USAGE_DYNAMIC; // CPU: Write, GPU: Read
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(BVHNode);

		hr = device->CreateBuffer(&desc, nullptr, &(resources->_RTTLASBvh));
		if (FAILED(hr)) {
			log_debug("[DBG] [BVH] [REALLOC] Failed when creating TLAS BVH buffer: 0x%x", hr);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = g_iRTMaxTLASNodesSoFar;

		hr = device->CreateShaderResourceView(resources->_RTTLASBvh, &srvDesc, &(resources->_RTTLASBvhSRV));
		if (FAILED(hr)) {
			log_debug("[DBG] [BVH] [REALLOC] Failed when creating TLAS BVH SRV: 0x%x", hr);
		}
	}

	// Populate the BVH buffer
	{
		D3D11_MAPPED_SUBRESOURCE map;
		ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
		hr = context->Map(resources->_RTTLASBvh.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		if (SUCCEEDED(hr))
		{
			memcpy(map.pData, g_TLASTree->nodes, sizeof(BVHNode) * numNodes);
			context->Unmap(resources->_RTTLASBvh.Get(), 0);
		}
		else
			log_debug("[DBG] [BVH] [REALLOC] Failed when mapping TLAS BVH nodes: 0x%x", hr);
	}
}

void EffectsRenderer::ReAllocateAndPopulateMatrixBuffer()
{
	// Create the buffers for the Matrices -- this code path applies for in-flight RT
	auto& resources = _deviceResources;
	auto& device = resources->_d3dDevice;
	auto& context = resources->_d3dDeviceContext;
	HRESULT hr;

	// (Re-)Create the matrices buffer
	const uint32_t numMatrices = g_iRTMeshesInThisFrame; // tlasLeaves.size();
	if (numMatrices == 0)
		return;

	bool bReallocateMatrixBuffers = numMatrices > g_iRTMaxMeshesSoFar;
#ifdef DEBUG_RT
	if (bReallocateMatrixBuffers)
	{
		log_debug("[DBG] [BVH] [REALLOC] RESIZING MATRIX BUFFER. numMatrices: %d, g_iRTMaxMeshesSoFar: %d",
			numMatrices, g_iRTMaxMeshesSoFar);
	}
#endif
	g_iRTMaxMeshesSoFar = max(numMatrices, g_iRTMaxMeshesSoFar);

	if (bReallocateMatrixBuffers)
	{
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		if (resources->_RTMatrices != nullptr)
		{
			resources->_RTMatrices.Release();
			resources->_RTMatrices = nullptr;
		}

		if (resources->_RTMatricesSRV != nullptr)
		{
			resources->_RTMatricesSRV.Release();
			resources->_RTMatricesSRV = nullptr;
		}

		desc.ByteWidth = sizeof(Matrix4) * g_iRTMaxMeshesSoFar;
		desc.Usage = D3D11_USAGE_DYNAMIC; // CPU: Write, GPU: Read
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(Matrix4);

		hr = device->CreateBuffer(&desc, nullptr, &(resources->_RTMatrices));
		if (FAILED(hr)) {
			log_debug("[DBG] [BVH] [REALLOC] Failed when creating _RTMatrices: 0x%x", hr);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = g_iRTMaxMeshesSoFar;

		hr = device->CreateShaderResourceView(resources->_RTMatrices, &srvDesc, &(resources->_RTMatricesSRV));
		if (FAILED(hr)) {
			log_debug("[DBG] [BVH] [REALLOC] Failed when creating _RTMatricesSRV: 0x%x", hr);
		}
#ifdef DEBUG_RT
		else
		{
			log_debug("[DBG] [BVH] [REALLOC] _RTMatricesSRV created");
		}
#endif
	}

	{
		// Populate the matrices for all the faceGroupIDs
		D3D11_MAPPED_SUBRESOURCE map;
		ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
		HRESULT hr = context->Map(resources->_RTMatrices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		if (SUCCEEDED(hr))
		{
			memcpy(map.pData, g_TLASMatrices.data(), sizeof(Matrix4) * numMatrices);
			/*
			BYTE* base_ptr = (BYTE*)map.pData;
			for (auto& tlasLeaf : tlasLeaves)
			{
				int matrixSlot = -1;
				Matrix4 W = GetBLASMatrix(tlasLeaf, &matrixSlot);
				if (matrixSlot != -1)
				{
					memcpy(base_ptr + matrixSlot * sizeof(Matrix4), W.get(), sizeof(Matrix4));
				}
			}
			*/
			context->Unmap(resources->_RTMatrices.Get(), 0);
		}
		else
			log_debug("[DBG] [BVH] Failed when mapping _RTMatrices: 0x%x", hr);
	}
}

void EffectsRenderer::SceneEnd()
{
	//EndCascadedShadowMap();
	D3dRenderer::SceneEnd();
	s_captureProjectionDeltas = true;

	if (_BLASNeedsUpdate)
	{
		if (InTechGlobe())
		{
			if (g_bRTEnabledInTechRoom)
			{
				// Build a single BVH from the contents of g_LBVHMap and put it in _lbvh
				BuildSingleBLASFromCurrentBVHMap();
			}
		}
		else if ((g_bRTEnabled || g_bActiveCockpitEnabled) && !(*g_playerInHangar))
		{
			// Build multiple BLASes and put them in g_LBVHMap
			BuildMultipleBLASFromCurrentBLASMap();
			// Encode the BLASes in g_LBVHMap into the SRVs and resize them if necessary
			ReAllocateAndPopulateBvhBuffers(g_iRTTotalBLASNodesInFrame);
			g_bRTReAllocateBvhBuffer = false;
		}
		_BLASNeedsUpdate = false;
	}

	if ((g_bRTEnabled || g_bActiveCockpitEnabled) && !g_bInTechRoom && !(*g_playerInHangar))
	{
		// We may need to reallocate the matrices buffer depending on how many
		// unique meshes we saw in this frame
		int numTLASleaves = tlasLeaves.size();
		if (numTLASleaves > 0)
		{
			// tlasLeaves should have the same number of entries as the g_TLASMap
			if (tlasLeaves.size() != g_TLASMap.size())
			{
				log_debug("[DBG] [BVH] ERROR, size mismatch: tlasLeaves.size(): %d, g_TLASMap.size(): %d",
					tlasLeaves.size(), g_TLASMap.size());
			}
			g_GlobalRange.x = g_GlobalAABB.max.x - g_GlobalAABB.min.x;
			g_GlobalRange.y = g_GlobalAABB.max.y - g_GlobalAABB.min.y;
			g_GlobalRange.z = g_GlobalAABB.max.z - g_GlobalAABB.min.z;
			if (g_bRTEnableEmbree)
				BuildTLASEmbree();
			else
			{
				switch (g_TLASBuilderType)
				{
				case TLASBuilderType::FastQBVH:
					BuildTLAS(tlasLeaves);
					break;
				case TLASBuilderType::DirectBVH4GPU:
					BuildTLASDBVH4();
					break;
				}

				if (g_bActiveCockpitEnabled)
				{
					BuildTLAS(g_ACtlasLeaves, true /* is AC TLAS */);
				}
			}
			//log_debug("[DBG] [BVH] TLAS Built");
			ReAllocateAndPopulateMatrixBuffer();
			//log_debug("[DBG] [BVH] Matrices Realloc'ed");
			ReAllocateAndPopulateTLASBvhBuffers();
			//log_debug("[DBG] [BVH] TLAS Buffers Realloc'ed");
			//ShowMatrix4(g_VSMatrixCB.fullViewMat, "SceneEnd");
		}
	}

	if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled && g_bRTEnabled)
	{
#ifdef DUMP_TLAS
		if (g_TLASFile != NULL)
		{
			fclose(g_TLASFile);
			g_TLASFile = NULL;
		}
#endif
		//DumpFrustrumToOBJ();
		DumpTLASLeaves(tlasLeaves, "./TLASLeaves.obj");
		if (g_bActiveCockpitEnabled)
		{
			DumpTLASLeaves(g_ACtlasLeaves, "./ACTLASLeaves.obj");
		}
	}

	// Close the OBJ dump file for the current frame
	if ((bD3DDumpOBJEnabled || bHangarDumpOBJEnabled) && g_bDumpSSAOBuffers) {
		fclose(D3DDumpOBJFile); D3DDumpOBJFile = NULL;
		log_debug("[DBG] OBJ file [d3dcapture-%d.obj] written", D3DOBJFileIdx);
		D3DOBJFileIdx++;

		fclose(D3DDumpLaserOBJFile); D3DDumpLaserOBJFile = NULL;
		log_debug("[DBG] OBJ file [d3dlasers-%d.obj] written", D3DOBJLaserFileIdx);
		D3DOBJLaserFileIdx++;

		if (!_hangarShadowAABB.IsInvalid() && bHangarDumpOBJEnabled) {
			FILE *ShadowMapFile = NULL;
			fopen_s(&ShadowMapFile, ".\\HangarShadowMapLimits.OBJ", "wt");
			_hangarShadowAABB.UpdateLimits();
			// The hangar AABB should already be in metric space and in the light view frame. There's no need
			// to apply additional transforms here.
			_hangarShadowAABB.DumpLimitsToOBJ(ShadowMapFile, 1, 1);
			fclose(ShadowMapFile);
		}
	}

	// This value gets reset to 0 when a mission is restarted by pressing H:
	// When a mission starts for the first time and we're in the hangar, this counter will
	// remain at 0. Same happens if the mission restarts: it remains at 0 for as long as
	// we're in the hangar. But if we park in the hangar mid-mission, it will continue to
	// tick.
	//if (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME)
	//	log_debug("[DBG] missionTime: %d", PlayerDataTable[*g_playerIndex].missionTime);
}

/* Function to quickly enable/disable ZWrite. */
HRESULT EffectsRenderer::QuickSetZWriteEnabled(BOOL Enabled)
{
	HRESULT hr;
	auto &resources = _deviceResources;
	auto &device = resources->_d3dDevice;

	D3D11_DEPTH_STENCIL_DESC desc;
	_solidDepthState->GetDesc(&desc);
	ComPtr<ID3D11DepthStencilState> depthState;

	desc.DepthEnable = Enabled;
	desc.DepthWriteMask = Enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = Enabled ? D3D11_COMPARISON_GREATER : D3D11_COMPARISON_ALWAYS;
	desc.StencilEnable = FALSE;
	hr = resources->_d3dDevice->CreateDepthStencilState(&desc, &depthState);
	if (SUCCEEDED(hr))
		resources->_d3dDeviceContext->OMSetDepthStencilState(depthState, 0);
	return hr;
}

inline void EffectsRenderer::EnableTransparency() {
	auto& resources = _deviceResources;
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

inline void EffectsRenderer::EnableHoloTransparency() {
	auto& resources = _deviceResources;
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

void EffectsRenderer::SaveContext()
{
	auto &context = _deviceResources->_d3dDeviceContext;

	_oldVSCBuffer = g_VSCBuffer;
	_oldPSCBuffer = g_PSCBuffer;
	_oldDCPSCBuffer = g_DCPSCBuffer;

	context->VSGetConstantBuffers(0, 1, _oldVSConstantBuffer.GetAddressOf());
	context->PSGetConstantBuffers(0, 1, _oldPSConstantBuffer.GetAddressOf());

	context->VSGetShaderResources(0, 3, _oldVSSRV[0].GetAddressOf());
	context->PSGetShaderResources(0, 13, _oldPSSRV[0].GetAddressOf());

	context->VSGetShader(_oldVertexShader.GetAddressOf(), nullptr, nullptr);
	// TODO: Use GetCurrentPixelShader here instead of PSGetShader and do *not* Release()
	// _oldPixelShader in RestoreContext
	context->PSGetShader(_oldPixelShader.GetAddressOf(), nullptr, nullptr);

	context->PSGetSamplers(0, 2, _oldPSSamplers[0].GetAddressOf());

	context->OMGetRenderTargets(8, _oldRTVs[0].GetAddressOf(), _oldDSV.GetAddressOf());
	context->OMGetDepthStencilState(_oldDepthStencilState.GetAddressOf(), &_oldStencilRef);
	context->OMGetBlendState(_oldBlendState.GetAddressOf(), _oldBlendFactor, &_oldSampleMask);

	context->IAGetInputLayout(_oldInputLayout.GetAddressOf());
	context->IAGetPrimitiveTopology(&_oldTopology);
	context->IAGetVertexBuffers(0, 1, _oldVertexBuffer.GetAddressOf(), &_oldStride, &_oldOffset);
	context->IAGetIndexBuffer(_oldIndexBuffer.GetAddressOf(), &_oldFormat, &_oldIOffset);

	_oldNumViewports = 2;
	context->RSGetViewports(&_oldNumViewports, _oldViewports);

	UINT NumRects = 1;
	context->RSGetScissorRects(&NumRects, &_oldScissorRect);
	_oldPose = g_OPTMeshTransformCB.MeshTransform;
	for (int i = 0; i < 16; i++)
		_oldTransformWorldView[i] = _CockpitConstants.transformWorldView[i];
}

void EffectsRenderer::RestoreContext()
{
	auto &resources = _deviceResources;
	auto &context = _deviceResources->_d3dDeviceContext;

	// Restore a previously-saved context
	g_VSCBuffer = _oldVSCBuffer;
	g_PSCBuffer = _oldPSCBuffer;
	g_DCPSCBuffer = _oldDCPSCBuffer;
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitPSConstantBufferDC(resources->_PSConstantBufferDC.GetAddressOf(), &g_DCPSCBuffer);

	// The hyperspace effect needs the current VS constants to work properly
	if (g_HyperspacePhaseFSM == HS_INIT_ST)
		context->VSSetConstantBuffers(0, 1, _oldVSConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _oldPSConstantBuffer.GetAddressOf());

	context->VSSetShaderResources(0, 3, _oldVSSRV[0].GetAddressOf());
	context->PSSetShaderResources(0, 13, _oldPSSRV[0].GetAddressOf());

	// It's important to use the Init*Shader methods here, or the shaders won't be
	// applied sometimes.
	resources->InitVertexShader(_oldVertexShader);
	resources->InitPixelShader(_oldPixelShader);

	context->PSSetSamplers(0, 2, _oldPSSamplers[0].GetAddressOf());
	context->OMSetRenderTargets(8, _oldRTVs[0].GetAddressOf(), _oldDSV.Get());
	context->OMSetDepthStencilState(_oldDepthStencilState.Get(), _oldStencilRef);
	context->OMSetBlendState(_oldBlendState.Get(), _oldBlendFactor, _oldSampleMask);

	resources->InitInputLayout(_oldInputLayout);
	resources->InitTopology(_oldTopology);
	context->IASetVertexBuffers(0, 1, _oldVertexBuffer.GetAddressOf(), &_oldStride, &_oldOffset);
	context->IASetIndexBuffer(_oldIndexBuffer.Get(), _oldFormat, _oldIOffset);

	context->RSSetViewports(_oldNumViewports, _oldViewports);
	context->RSSetScissorRects(1, &_oldScissorRect);

	// Release everything. Previous calls to *Get* increase the refcount
	_oldVSConstantBuffer.Release();
	_oldPSConstantBuffer.Release();

	for (int i = 0; i < 3; i++)
		_oldVSSRV[i].Release();
	for (int i = 0; i < 13; i++)
		_oldPSSRV[i].Release();

	_oldVertexShader.Release();
	_oldPixelShader.Release();

	for (int i = 0; i < 2; i++)
		_oldPSSamplers[i].Release();

	for (int i = 0; i < 8; i++)
		_oldRTVs[i].Release();
	_oldDSV.Release();
	_oldDepthStencilState.Release();
	_oldBlendState.Release();
	_oldInputLayout.Release();
	_oldVertexBuffer.Release();
	_oldIndexBuffer.Release();
	g_OPTMeshTransformCB.MeshTransform = _oldPose;
	for (int i = 0; i < 16; i++)
		_CockpitConstants.transformWorldView[i] = _oldTransformWorldView[i];

	_overrideRTV = TRANSP_LYR_NONE;
}

void EffectsRenderer::UpdateTextures(const SceneCompData* scene)
{
	const unsigned char ShipCategory_PlayerProjectile = 6;
	const unsigned char ShipCategory_OtherProjectile = 7;
	unsigned char category = scene->pObject->ShipCategory;
	bool isProjectile = category == ShipCategory_PlayerProjectile || category == ShipCategory_OtherProjectile;

	const auto XwaD3dTextureCacheUpdateOrAdd = (void(*)(XwaTextureSurface*))0x00597784;
	const auto L00432750 = (short(*)(unsigned short, short, short))0x00432750;
	const ExeEnableEntry* s_ExeEnableTable = (ExeEnableEntry*)0x005FB240;

	XwaTextureSurface* surface = nullptr;
	XwaTextureSurface* surface2 = nullptr;

	_constants.renderType = 0;
	_constants.renderTypeIllum = 0;
	_bRenderingLightingEffect = false;

	if (g_isInRenderLasers || isProjectile)
	{
		_constants.renderType = 2;
	}

	if (scene->D3DInfo != nullptr)
	{
		surface = scene->D3DInfo->ColorMap[0];

		if (scene->D3DInfo->LightMap[0] != nullptr)
		{
			surface2 = scene->D3DInfo->LightMap[0]; // surface2 is a lightmap
			_constants.renderTypeIllum = 1;
		}
	}
	else
	{
		// This is a lighting effect... I wonder which ones? Smoke perhaps?
		_bRenderingLightingEffect = true;
		//log_debug("[DBG] Rendering Lighting Effect");
		const unsigned short ModelIndex_237_1000_0_ResData_LightingEffects = 237;
		L00432750(ModelIndex_237_1000_0_ResData_LightingEffects, 0x02, 0x100);
		XwaSpeciesTMInfo* esi = (XwaSpeciesTMInfo*)s_ExeEnableTable[ModelIndex_237_1000_0_ResData_LightingEffects].pData1;
		surface = (XwaTextureSurface*)esi->pData;
	}

	XwaD3dTextureCacheUpdateOrAdd(surface);

	if (surface2 != nullptr)
	{
		XwaD3dTextureCacheUpdateOrAdd(surface2);
	}

	Direct3DTexture* texture = (Direct3DTexture*)surface->D3dTexture.D3DTextureHandle;
	Direct3DTexture* texture2 = surface2 == nullptr ? nullptr : (Direct3DTexture*)surface2->D3dTexture.D3DTextureHandle;
	_deviceResources->InitPSShaderResourceView(texture->_textureView, texture2 == nullptr ? nullptr : texture2->_textureView.Get());
	_lastTextureSelected = texture;
	_lastLightmapSelected = texture2;
}

void EffectsRenderer::DoStateManagement(const SceneCompData* scene)
{
	// ***************************************************************
	// State management begins here
	// ***************************************************************
	// The following state variables will probably need to be pruned. I suspect we don't
	// need to care about GUI/2D/HUD stuff here anymore.
	// At this point, texture and texture2 have been selected, we can check their names to see if
	// we need to apply effects. If there's a lightmap, it's going to be in texture2.
	// Most of the local flags below should now be class members, but I'll be hand
	_bLastTextureSelectedNotNULL = (_lastTextureSelected != NULL);
	_bLastLightmapSelectedNotNULL = (_lastLightmapSelected != NULL);
	_bIsBlastMark = false;
	_bIsCockpit = false;
	_bIsGunner = false;
	_bIsExplosion = false;
	_bDCIsTransparent = false;
	_bDCElemAlwaysVisible = false;
	_bIsHologram = false;
	_bIsNoisyHolo = false;
	_bIsActiveCockpit = false;
	_bWarheadLocked = PlayerDataTable[*g_playerIndex].warheadArmed && PlayerDataTable[*g_playerIndex].warheadLockState == 3;
	_bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	_bCockpitDisplayed = PlayerDataTable[*g_playerIndex].cockpitDisplayed;
	// If we reach this path, we're no longer rendering the skybox.
	g_bSkyBoxJustFinished = true;

	_bIsTargetHighlighted = false;
	//bool bIsExterior = false, bIsDAT = false;
	//bool bIsDS2CoreExplosion = false;
	bool bHasMaterial = false;

	if (_bLastTextureSelectedNotNULL) {
		if (g_bDynCockpitEnabled && _lastTextureSelected->is_DynCockpitDst)
		{
			int idx = _lastTextureSelected->DCElementIndex;
			if (idx >= 0 && idx < g_iNumDCElements) {
				_bIsHologram |= g_DCElements[idx].bHologram;
				_bIsNoisyHolo |= g_DCElements[idx].bNoisyHolo;
				_bDCIsTransparent |= g_DCElements[idx].bTransparent;
				_bDCElemAlwaysVisible |= g_DCElements[idx].bAlwaysVisible;
			}
		}

		_bIsLaser = _lastTextureSelected->is_Laser || _lastTextureSelected->is_TurboLaser;
		//_bIsLaser = _constants.renderType == 2;
		g_bIsTargetHighlighted |= _lastTextureSelected->is_HighlightedReticle;
		_bIsTargetHighlighted = g_bIsTargetHighlighted || g_bPrevIsTargetHighlighted;
		if (_bIsTargetHighlighted) g_GameEvent.TargetEvent = TGT_EVT_LASER_LOCK;
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
		//bIsLensFlare = lastTextureSelected->is_LensFlare;
		//bIsHyperspaceTunnel = lastTextureSelected->is_HyperspaceAnim;
		_bIsCockpit = _lastTextureSelected->is_CockpitTex;
		_bIsGunner = _lastTextureSelected->is_GunnerTex;
		_bIsExterior = _lastTextureSelected->is_Exterior;
		//bIsDAT = lastTextureSelected->is_DAT;
		_bIsActiveCockpit = _lastTextureSelected->ActiveCockpitIdx > -1;
		_bIsBlastMark = _lastTextureSelected->is_BlastMark;
		//bIsDS2CoreExplosion = lastTextureSelected->is_DS2_Reactor_Explosion;
		//bIsElectricity = lastTextureSelected->is_Electricity;
		_bHasMaterial = _lastTextureSelected->bHasMaterial;
		_bIsExplosion = _lastTextureSelected->is_Explosion;
		//if (_bHasMaterial) _bForceShaded = _lastTextureSelected->material.ForceShaded;
		if (_bIsExplosion) g_bExplosionsDisplayedOnCurrentFrame = true;
	}

	// The new d3dhook by Jeremy changes slightly how draw commands are processed.
	// Sometimes, after the CMD GUI element is rendered, control comes here from
	// Direct3DDevice::Execute(), so we need to update g_bTargetCompDrawn in this
	// path as well.
	if (!g_bTargetCompDrawn && g_iFloatingGUIDrawnCounter > 0)
		g_bTargetCompDrawn = true;
	// Hysteresis detection (state is about to switch to render something different, like the HUD)
	g_bPrevIsFloatingGUI3DObject = g_bIsFloating3DObject;
	// Do *not* use g_isInRenderMiniature here, it's not reliable.
	g_bIsFloating3DObject = g_bTargetCompDrawn && _bLastTextureSelectedNotNULL &&
		!_lastTextureSelected->is_Text && !_lastTextureSelected->is_TrianglePointer &&
		!_lastTextureSelected->is_Reticle && !_lastTextureSelected->is_Floating_GUI &&
		!_lastTextureSelected->is_TargetingComp;

	// The GUI starts rendering whenever we detect a GUI element, or Text, or a bracket.
	// ... or not at all if we're in external view mode with nothing targeted.
	//g_bPrevStartedGUI = g_bStartedGUI;
	// Apr 10, 2020: g_bDisableDiffuse will make the reticle look white when the HUD is
	// hidden. To prevent this, I added bIsAimingHUD to g_bStartedGUI; but I don't know
	// if this breaks VR. If it does, then I need to add !bIsAimingHUD around line 6425,
	// where I'm setting fDisableDiffuse = 1.0f
	//g_bStartedGUI |= bIsGUI || bIsText || bIsBracket || bIsFloatingGUI || bIsReticle;
	// bIsScaleableGUIElem is true when we're about to render a HUD element that can be scaled down with Ctrl+Z
	g_bPrevIsScaleableGUIElem = g_bIsScaleableGUIElem;
	g_bIsScaleableGUIElem = g_bStartedGUI && !g_bIsTrianglePointer;
}

// Apply specific material properties for the current texture
void EffectsRenderer::ApplyMaterialProperties()
{
	if (!_bLastTextureSelectedNotNULL)
		return;

	// Some properties can be applied even if there's no associated material:
	g_PSCBuffer.bIsTransparent = _lastTextureSelected->is_Transparent;

	if (!_bHasMaterial)
		return;

	auto &resources = _deviceResources;

	_bModifiedShaders = true;

	g_PSCBuffer.fSSAOMaskVal   = _lastTextureSelected->material.Metallic * 0.5f; // Metallicity is encoded in the range 0..0.5 of the SSAOMask
	g_PSCBuffer.bIsShadeless   = _lastTextureSelected->material.IsShadeless;
	g_PSCBuffer.fGlossiness    = _lastTextureSelected->material.Glossiness;
	g_PSCBuffer.fSpecInt       = _lastTextureSelected->material.Intensity;
	g_PSCBuffer.fNMIntensity   = _lastTextureSelected->material.NMIntensity;
	g_PSCBuffer.fSpecVal       = _lastTextureSelected->material.SpecValue;
	g_PSCBuffer.fAmbient       = _lastTextureSelected->material.Ambient;

	if (_lastTextureSelected->material.AlphaToBloom) {
		_bModifiedPixelShader = true;
		_bModifiedShaders = true;
		resources->InitPixelShader(resources->_alphaToBloomPS);
		if (_lastTextureSelected->material.NoColorAlpha)
			g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_NO_COLOR_ALPHA;
		g_PSCBuffer.fBloomStrength = _lastTextureSelected->material.EffectBloom;
	}

	// lastTextureSelected can't be a lightmap anymore, so we don't need (?) to test bIsLightTexture
	if (_lastTextureSelected->material.AlphaIsntGlass)
		// I feel like the AlphaIsntGlass property only makes sense when there's transparency, but
		// that's now how I coded it, so there may be artifacts if I enable the following line:
		// && _lastTextureSelected->is_Transparent)
	{
		_bModifiedPixelShader = true;
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = 0.0f;
		resources->InitPixelShader(resources->_noGlassPS);
	}
}

// Apply the SSAO mask/Special materials, like lasers and HUD
void EffectsRenderer::ApplySpecialMaterials()
{
	if (!_bLastTextureSelectedNotNULL)
		return;

	if (g_bIsScaleableGUIElem || g_bIsTrianglePointer || _bIsExplosion ||
		_lastTextureSelected->is_Debris || _lastTextureSelected->is_GenericSSAOMasked ||
		_lastTextureSelected->is_Electricity || _lastTextureSelected->is_Smoke)
	{
		_bModifiedShaders = true;
		g_PSCBuffer.fSSAOMaskVal = 0;
		g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
		g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
		g_PSCBuffer.fNMIntensity = 0.0f;
		g_PSCBuffer.fSpecVal     = 0.0f;
		g_PSCBuffer.bIsShadeless = 1;
		g_PSCBuffer.fPosNormalAlpha = 0.0f;
	}
	else if (_lastTextureSelected->is_Debris ||
		_lastTextureSelected->is_CockpitSpark || _lastTextureSelected->is_Spark ||
		_lastTextureSelected->is_Chaff)
	{
		_bModifiedShaders = true;
		g_PSCBuffer.fSSAOMaskVal = 0;
		g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
		g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
		g_PSCBuffer.fNMIntensity = 0.0f;
		g_PSCBuffer.fSpecVal     = 0.0f;
		g_PSCBuffer.fPosNormalAlpha = 0.0f;
	}
	else if (_lastTextureSelected->is_Missile) {
		g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_MISSILE;
	}
	else if (_lastTextureSelected->is_Laser) {
		_bModifiedShaders = true;
		g_PSCBuffer.fSSAOMaskVal = 0;
		g_PSCBuffer.fGlossiness  = DEFAULT_GLOSSINESS;
		g_PSCBuffer.fSpecInt     = DEFAULT_SPEC_INT;
		g_PSCBuffer.fNMIntensity = 0.0f;
		g_PSCBuffer.fSpecVal     = 0.0f;
		g_PSCBuffer.bIsShadeless = 1;
		g_PSCBuffer.fPosNormalAlpha = 0.0f;
	}
}

/// <summary>
/// Returns 0 if there's no throttle, 1 for full throttle.
/// </summary>
float GetCurrentPlayerThrottle()
{
	CraftInstance* craftInstance = GetPlayerCraftInstanceSafe();
	if (craftInstance == NULL) return 0.0f;
	return craftInstance->EngineThrottleInput / 65535.0f;
};

void EffectsRenderer::ApplyDiegeticCockpit()
{
	// g_OPTMeshTransformCB.MeshTransform should be reset to identity for each mesh at
	// the beginning of MainSceneHook(). So if we just return from this function, no
	// transform will be applied to the current mesh
	if (!g_bDiegeticCockpitEnabled || !_bHasMaterial || !_bLastTextureSelectedNotNULL)
		return;

	auto &resources = _deviceResources;

	auto GetHyperThrottle = []() {
		float timeInHyperspace = (float)PlayerDataTable[*g_playerIndex].timeInHyperspace;
		switch (g_HyperspacePhaseFSM) {
		case HS_INIT_ST:
			return 0.0f;
		case HS_HYPER_ENTER_ST:
			return min(1.0f, timeInHyperspace / 550.0f);
		case HS_HYPER_TUNNEL_ST:
			return 1.0f;
		case HS_HYPER_EXIT_ST:
			return max(0.0f, 1.0f - timeInHyperspace / 190.0f);
		}
		return 0.0f;
	};

	DiegeticMeshType DiegeticMesh = _lastTextureSelected->material.DiegeticMesh;
	switch (DiegeticMesh) {
	case DM_JOYSTICK: {
		// _bJoystickTransformReady is set to false at the beginning of each frame. We set it to
		// true as soon as one mesh computes it, so that we don't compute it several times per
		// frame.
		if (!_bJoystickTransformReady && g_pSharedDataJoystick != NULL && g_SharedMemJoystick.IsDataReady()) {
			Vector3 JoystickRoot = _lastTextureSelected->material.JoystickRoot;
			float MaxYaw = _lastTextureSelected->material.JoystickMaxYaw;
			float MaxPitch = _lastTextureSelected->material.JoystickMaxPitch;
			Matrix4 T, R;

			// Translate the JoystickRoot to the origin
			T.identity(); T.translate(-JoystickRoot);
			_joystickMeshTransform = T;

			R.identity(); R.rotateY(MaxYaw * g_pSharedDataJoystick->JoystickYaw);
			_joystickMeshTransform = R * _joystickMeshTransform;

			R.identity(); R.rotateX(MaxPitch * g_pSharedDataJoystick->JoystickPitch);
			_joystickMeshTransform = R * _joystickMeshTransform;

			// Return the system to its original position
			T.identity(); T.translate(JoystickRoot);
			_joystickMeshTransform = T * _joystickMeshTransform;

			// We need to transpose the matrix because the Vertex Shader is expecting the
			// matrix in this format
			_joystickMeshTransform.transpose();
			// Avoid computing the transform above more than once per frame:
			_bJoystickTransformReady = true;
		}
		g_OPTMeshTransformCB.MeshTransform = _joystickMeshTransform;
		break;
	}
	case DM_THR_ROT_X:
	case DM_THR_ROT_Y:
	case DM_THR_ROT_Z:
	case DM_HYPER_ROT_X:
	case DM_HYPER_ROT_Y:
	case DM_HYPER_ROT_Z:
	{
		// Caching the transform may not work well when both the throttle and the hyper
		// throttle are present.
		//if (!_bThrottleTransformReady)
		{
			float throttle = (DiegeticMesh == DM_THR_ROT_X || DiegeticMesh == DM_THR_ROT_Y || DiegeticMesh == DM_THR_ROT_Z) ?
				GetCurrentPlayerThrottle() : GetHyperThrottle();

			// Build the transform matrix
			Vector3 ThrottleRoot = _lastTextureSelected->material.ThrottleRoot;
			float ThrottleMaxAngle = _lastTextureSelected->material.ThrottleMaxAngle;
			Matrix4 T, R;

			// Translate the root to the origin
			T.identity(); T.translate(-ThrottleRoot);
			_throttleMeshTransform = T;

			// Apply the rotation
			R.identity();
			if (DiegeticMesh == DM_THR_ROT_X || DiegeticMesh == DM_HYPER_ROT_X) R.rotateX(throttle * ThrottleMaxAngle);
			if (DiegeticMesh == DM_THR_ROT_Y || DiegeticMesh == DM_HYPER_ROT_Y) R.rotateY(throttle * ThrottleMaxAngle);
			if (DiegeticMesh == DM_THR_ROT_Z || DiegeticMesh == DM_HYPER_ROT_Z) R.rotateZ(throttle * ThrottleMaxAngle);
			_throttleMeshTransform = R * _throttleMeshTransform;

			// Return the system to its original position
			T.identity(); T.translate(ThrottleRoot);
			_throttleMeshTransform = T * _throttleMeshTransform;

			// We need to transpose the matrix because the Vertex Shader is expecting the
			// matrix in this format
			_throttleMeshTransform.transpose();
			// Avoid computing the transform above more than once per frame:
			//_bThrottleTransformReady = true;
		}
		g_OPTMeshTransformCB.MeshTransform = _throttleMeshTransform;
		break;
	}
	case DM_THR_TRANS:
	case DM_HYPER_TRANS:
		// Caching the transform may not work well when both the throttle and the hyper
		// throttle are present.
		//if (!_bThrottleTransformReady)
		{
			float throttle = DiegeticMesh == DM_THR_TRANS ? GetCurrentPlayerThrottle() : GetHyperThrottle();

			// Build the transform matrix
			Matrix4 T, R;
			Vector3 ThrottleStart = _lastTextureSelected->material.ThrottleStart;
			Vector3 ThrottleEnd = _lastTextureSelected->material.ThrottleEnd;
			Vector3 Dir = ThrottleEnd - ThrottleStart;
			Dir.normalize();

			// Apply the rotation
			Dir *= throttle;
			T.translate(Dir);
			_throttleMeshTransform = T;

			// We need to transpose the matrix because the Vertex Shader is expecting the
			// matrix in this format
			_throttleMeshTransform.transpose();
			// Avoid computing the transform above more than once per frame:
			//_bThrottleTransformReady = true;
		}
		g_OPTMeshTransformCB.MeshTransform = _throttleMeshTransform;
		break;
	case DM_THR_ROT_ANY:
	case DM_HYPER_ROT_ANY:
		// Caching the transform may not work well when both the throttle and the hyper
		// throttle are present.
		//if (!_bThrottleRotAxisToZPlusReady)
		{
			float throttle = DiegeticMesh == DM_THR_ROT_ANY ? GetCurrentPlayerThrottle() : GetHyperThrottle();
			Material *material = &(_lastTextureSelected->material);

			float ThrottleMaxAngle = material->ThrottleMaxAngle;
			Vector3 ThrottleStart = material->ThrottleStart;
			Vector3 ThrottleEnd = material->ThrottleEnd;
			if (!material->bRotAxisToZPlusReady) {
				// Cache RotAxisToZPlus so that we don't have to compute it on every frame
				Vector4 Dir;
				Dir.x = ThrottleEnd.x - ThrottleStart.x;
				Dir.y = ThrottleEnd.y - ThrottleStart.y;
				Dir.z = ThrottleEnd.z - ThrottleStart.z;
				Dir.w = 0;
				material->RotAxisToZPlus = GetSimpleDirectionMatrix(Dir, false);
				material->bRotAxisToZPlusReady = true;
			}
			Matrix4 RotAxisToZPlus = material->RotAxisToZPlus;

			Matrix4 T, R;
			// Translate the root to the origin
			T.identity(); T.translate(-ThrottleStart);
			_throttleMeshTransform = T;

			// Align with Z+
			_throttleMeshTransform = RotAxisToZPlus * _throttleMeshTransform;

			// Apply the rotation
			R.identity();
			R.rotateZ(throttle * ThrottleMaxAngle);
			_throttleMeshTransform = R * _throttleMeshTransform;

			// Invert Z+ alignment
			RotAxisToZPlus.transpose();
			_throttleMeshTransform = RotAxisToZPlus * _throttleMeshTransform;

			// Return the system to its original position
			T.identity(); T.translate(ThrottleStart);
			_throttleMeshTransform = T * _throttleMeshTransform;

			// We need to transpose the matrix because the Vertex Shader is expecting the
			// matrix in this format
			_throttleMeshTransform.transpose();
			//_bThrottleRotAxisToZPlusReady = true;
		}
		g_OPTMeshTransformCB.MeshTransform = _throttleMeshTransform;
		break;
	}
}

void EffectsRenderer::ApplyMeshTransform()
{
	// g_OPTMeshTransformCB.MeshTransform should be reset to identity for each mesh at
	// the beginning of MainSceneHook(). So if we just return from this function, no
	// transform will be applied to the current mesh
	if (!_bHasMaterial || !_bLastTextureSelectedNotNULL || !_lastTextureSelected->material.meshTransform.bDoTransform)
		return;

	auto &resources = _deviceResources;
	Material *material = &(_lastTextureSelected->material);
	
	material->meshTransform.UpdateTransform();
	g_OPTMeshTransformCB.MeshTransform = material->meshTransform.ComputeTransform();
}

/// <summary>
/// Test for intersections with additional VR geometry (VR keyboard, gloves...)
/// </summary>
void EffectsRenderer::IntersectVRGeometry()
{
	if (!g_bRendering3D)
		return;

	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const bool bInHangar = *g_playerInHangar;

	Matrix4 KeybTransform = (bGunnerTurret || bInHangar) ? g_vrKeybState.OPTTransform : g_vrKeybState.Transform;
	// We premultiply in the code below, so we need to transpose the matrix because
	// the Vertex shader does a postmultiplication
	KeybTransform.transpose();

	Vector4 contOrigin[2], contDir[2];
	if (!bGunnerTurret && !bInHangar)
		VRControllerToOPTCoords(contOrigin, contDir);
	else
	{
		Matrix4 swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });
		Matrix4 Sinv = Matrix4().scale(METERS_TO_OPT);
		Matrix4 toOPT = Sinv * swap;
		for (int i = 0; i < 2; i++)
		{
			g_contOriginWorldSpace[i].w = 1.0f;
			contOrigin[i] = toOPT * g_contOriginWorldSpace[i];
		}
	}

	const float margin = 0.01f;
	for (int contIdx = 0; contIdx < 2; contIdx++)
	{
		const int auxContIdx = (contIdx + 1) % 2;

		// Test the VR gloves -- but only the opposing hand!
		if (g_vrGlovesMeshes[auxContIdx].visible && g_contStates[auxContIdx].bIsValid && g_iVRGloveSlot[auxContIdx] != -1)
		{
			Intersection inters;
			const int acGloveSlot = g_iVRGloveSlot[auxContIdx];
			ac_uv_coords* coords = &(g_ACElements[acGloveSlot].coords);

			Matrix4 pose = g_vrGlovesMeshes[auxContIdx].pose;
			Matrix4 pose0;
			pose.transpose(); // Enable pre-multiplication again
			pose0 = pose;
			pose.invert();    // We're going from Cockpit coords to Glove OPT coords

			// Transform the controller's origin into the OPT coord sys
			Vector4 O;
			if (bGunnerTurret || bInHangar)
			{
				// The gloves are in the following coord sys after the "pose" transform is
				// applied:
				//
				// Scale: OPT
				// Viewspace (moving the headset modifies the coords)
				// X+: Right
				// Y+: Down
				// Z+: Forwards
				//
				// SteamVR is in the following system:
				// Scale: metric
				// Worldspace (moving the headset does not alter the coords)
				// X+: Right
				// Y-: Down
				// Z-: Forwards
				//
				// So, to transform SteamVR to OPT-viewspace we need to:
				// Apply headset's inverse transform
				// invert Y and Z
				// Scale to OPT sizes
				Matrix4 invYZ({ 1,0,0,0,  0,-1,0,0,  0,0,-1,0,  0,0,0,1 });
				Matrix4 Sinv = Matrix4().scale(METERS_TO_OPT);
				Matrix4 Vinv = g_VSMatrixCB.fullViewMat;
				Vinv.invert();
				O = Sinv * invYZ * Vinv * g_contOriginWorldSpace[contIdx];
				// Convert OPT-Viewspace back into OPT-Objectspace
				O = pose * O;
			}
			else
			{
				// Convert OPT-Viewspace back into OPT-Objectspace
				O = pose * contOrigin[contIdx];
			}

			// Find the closest intersection with the Glove OPT
			float3 P;
			LBVH* bvh = (LBVH*)g_vrGlovesMeshes[auxContIdx].bvh;
			Intersection tempInters = ClosestHit(bvh->nodes, { O.x, O.y, O.z }, 0, P, coords, auxContIdx);
			if (tempInters.TriID != -1 && // There was an intersection
				tempInters.T > 0.0f)
			{
				inters = tempInters;
			}

			if (inters.TriID != -1 && inters.T < g_fBestIntersectionDistance[contIdx])
			{
				g_fBestIntersectionDistance[contIdx] = inters.T;

				const float baryU = inters.U;
				const float baryV = inters.V;
				const float baryW = 1.0f - baryU - baryV;

				const int index0 = inters.TriID * 3;

				const int t0Idx = g_vrGlovesMeshes[auxContIdx].texIndices[index0 + 0];
				const int t1Idx = g_vrGlovesMeshes[auxContIdx].texIndices[index0 + 1];
				const int t2Idx = g_vrGlovesMeshes[auxContIdx].texIndices[index0 + 2];

				XwaTextureVertex bestUV0 = g_vrGlovesMeshes[auxContIdx].texCoords[t0Idx];
				XwaTextureVertex bestUV1 = g_vrGlovesMeshes[auxContIdx].texCoords[t1Idx];
				XwaTextureVertex bestUV2 = g_vrGlovesMeshes[auxContIdx].texCoords[t2Idx];

				g_LaserPointerBuffer.uv[contIdx][0] = baryU * bestUV0.u + baryV * bestUV1.u + baryW * bestUV2.u;
				g_LaserPointerBuffer.uv[contIdx][1] = baryU * bestUV0.v + baryV * bestUV1.v + baryW * bestUV2.v;
				g_iBestIntersTexIdx[contIdx] = g_iVRGloveSlot[auxContIdx];

				// P is in the OPT-Object coord sys...
				Vector4 Q = { P.x, P.y, P.z, 1.0f };
				// ... so we need to transform it into OPT-Viewspace coords:
				Q = pose0 * Q;

				if (bGunnerTurret || bInHangar)
				{
					g_LaserPointer3DIntersection[contIdx] = { Q.x, -Q.y, Q.z };
					Matrix4 invYZ({ 1,0,0,0,  0,-1,0,0,  0,0,-1,0,  0,0,0,1 });
					Matrix4 S = Matrix4().scale(OPT_TO_METERS);
					Matrix4 toSteamVR = g_VSMatrixCB.fullViewMat * invYZ * S;
					Q = toSteamVR * Q;

					// This is a bit of a crutch, but I'm temporarily storing the intersection in
					// SteamVR coords because it's easier to test the distance with the controller
					// in this framework. This shouldn't be necessary, though.
					g_LaserPointerIntersSteamVR[contIdx].x = Q.x;
					g_LaserPointerIntersSteamVR[contIdx].y = Q.y;
					g_LaserPointerIntersSteamVR[contIdx].z = Q.z;
				}
				else
					g_LaserPointer3DIntersection[contIdx] = { Q.x, Q.y, Q.z };
				// Skip to the next contIdx/glove?
				//continue;
			}
		}

		// Test the VR keyboard
		if (g_vrKeybState.state == KBState::HOVER || g_vrKeybState.state == KBState::STATIC)
		{
			for (int i = 0; i < g_vrKeybNumTriangles; i++)
			{
				D3dTriangle t = g_vrKeybTriangles[i];

				Vector4 p0 = KeybTransform * XwaVector3ToVector4(g_vrKeybMeshVertices[t.v1]);
				Vector4 p1 = KeybTransform * XwaVector3ToVector4(g_vrKeybMeshVertices[t.v2]);
				Vector4 p2 = KeybTransform * XwaVector3ToVector4(g_vrKeybMeshVertices[t.v3]);

				Vector3 v0 = Vector4ToVector3(p0);
				Vector3 v1 = Vector4ToVector3(p1);
				Vector3 v2 = Vector4ToVector3(p2);

				Vector3 P;
				float dist = FLT_MAX, baryU, baryV;

				// Find the intersection along the triangle's normal (the closest point on the triangle)
				Vector3 e10 = v1 - v0;
				Vector3 e20 = v2 - v0;
				Vector3 N = -1.0f * e10.cross(e20);
				N.normalize();
				N *= METERS_TO_OPT; // Everything is OPT scale here
				Vector3 O = Vector4ToVector3(contOrigin[contIdx]);
				const bool directedInters = rayTriangleIntersect(O, N, v0, v1, v2, dist, P, baryU, baryV, margin);
				const float baryW = 1.0f - baryU - baryV;

				// Allowing negative distances prevents phantom clicking when we "push" behind the
				// floating keyboard. dir is already in OPT scale, so we can just use 0.01 below and
				// that means "1cm".
				// GLOVE_NEAR_THRESHOLD_METERS rejects intersections that are too far away from the target
				// (more than 5cms)
				if (directedInters && dist > -0.02f && dist < GLOVE_NEAR_THRESHOLD_METERS &&
					dist < g_fBestIntersectionDistance[contIdx])
				{
					g_fBestIntersectionDistance[contIdx] = dist;
					g_iBestIntersTexIdx[contIdx] = g_iVRKeyboardSlot;

					XwaTextureVertex bestUV0 = g_vrKeybTextureCoords[t.v1];
					XwaTextureVertex bestUV1 = g_vrKeybTextureCoords[t.v2];
					XwaTextureVertex bestUV2 = g_vrKeybTextureCoords[t.v3];

					float u = baryU * bestUV0.u + baryV * bestUV1.u + baryW * bestUV2.u;
					float v = baryU * bestUV0.v + baryV * bestUV1.v + baryW * bestUV2.v;

					// Search the list of active elements in this keyboard and find the closest one
					ac_uv_coords* coords = &(g_ACElements[g_iVRKeyboardSlot].coords);
					int bestAreaIdx = -1;
					float bestAreaDist = FLT_MAX;
					for (int i = 0; i < coords->numCoords; i++)
					{
						if (coords->area[i].x0 <= u && u <= coords->area[i].x1 &&
							coords->area[i].y0 <= v && v <= coords->area[i].y1)
						{
							bestAreaIdx = i;
							break;
						}
						else
						{
							const float uc = (coords->area[i].x0 + coords->area[i].x1) / 2.0f;
							const float vc = (coords->area[i].y0 + coords->area[i].y1) / 2.0f;
							const float dx = uc - u, dy = vc - v;
							const float dist = dx * dx + dy * dy;
							if (dist < bestAreaDist)
							{
								bestAreaDist = dist;
								bestAreaIdx = i;
							}
						}
					}

					if (bestAreaIdx != -1)
					{
						// Recompute P and center it on the current AC element

						// First, let's re-compute the UVs
						u = (coords->area[bestAreaIdx].x0 + coords->area[bestAreaIdx].x1) / 2.0f;
						v = (coords->area[bestAreaIdx].y0 + coords->area[bestAreaIdx].y1) / 2.0f;

						// Now let's use the new UV to compute a new P... and we do it as follows:
						// The VR Keyboard has the X-axis going from index 0 to index 1
						// and the Y axis goes from index 0 to index 3. Looks like this:
						//
						// 0 -> 1
						// |
						// 3
						const Vector3 O = XwaVector3ToVector3(g_vrKeybMeshVertices[0]);
						const Vector3 X = XwaVector3ToVector3(g_vrKeybMeshVertices[1] - g_vrKeybMeshVertices[0]);
						const Vector3 Y = XwaVector3ToVector3(g_vrKeybMeshVertices[3] - g_vrKeybMeshVertices[0]);
						P = Vector4ToVector3(KeybTransform * XwaVector3ToVector4(O + u * X + v * Y));
					}

					g_LaserPointerBuffer.uv[contIdx][0] = u;
					g_LaserPointerBuffer.uv[contIdx][1] = v;

					if (contIdx == 0)
					{
						//g_enable_ac_debug = true;
						g_enable_ac_debug = false;
						g_debug_v0 = v0;
						g_debug_v1 = v1;
						g_debug_v2 = v2;
					}

					if (bGunnerTurret || bInHangar)
					{
						Matrix4 swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });
						Matrix4 Sinv = Matrix4().scale(METERS_TO_OPT);
						Matrix4 S = Matrix4().scale(OPT_TO_METERS);
						Matrix4 toSteamVR = swap * S;
						Matrix4 Vinv = g_VSMatrixCB.fullViewMat;
						Vinv.invert();

						Vector4 Q = { P.x, P.y, P.z, 1.0f };
						// Convert Q (which is in OPT coords), to SteamVR coords, then invert the
						// headset rotation. Finally convert to OPT-scale -- but don't swap the axes!
						// Just invert Z to make it consistent with RenderLaserPointer()
						Q = toSteamVR * Q;
						// This is a bit of a crutch, but I'm temporarily storing the intersection in
						// SteamVR coords because it's easier to test the distance with the controller
						// in this framework. This shouldn't be necessary, though.
						g_LaserPointerIntersSteamVR[contIdx].x = Q.x;
						g_LaserPointerIntersSteamVR[contIdx].y = Q.y;
						g_LaserPointerIntersSteamVR[contIdx].z = Q.z;
						Q = Sinv * Vinv * Q;
						Q.z = -Q.z;
						P = Vector4ToVector3(Q);
					}
					g_LaserPointer3DIntersection[contIdx] = P;
					// There's no need to test the other triangle in the VR keyb mesh. The closest
					// intersection is along the normal anyway and we just found it.
					continue;
				}
			}
		}
	} // for contIdx
}

/// <summary>
/// If the current texture is ActiveCockpit-enabled, then this code will check for intersections
/// </summary>
void EffectsRenderer::ApplyActiveCockpit(const SceneCompData* scene)
{
	if (!g_bActiveCockpitEnabled || !_bLastTextureSelectedNotNULL || !_bIsActiveCockpit || _bIsHologram)
		return;

	// DEBUG: Dump the mesh associated with the current texture
	/*if (g_bDumpSSAOBuffers)
	{
		static int counter = 0;
		log_debug("[DBG] [AC] Dumping vertices with AC-enabled textures");
		SingleFileOBJDumpD3dVertices(scene, _trianglesCount, std::string(".\\AC-") + std::to_string(counter++) + ".obj");
	}*/

	XwaVector3* MeshVertices = scene->MeshVertices;
	int MeshVerticesCount = *(int*)((int)scene->MeshVertices - 8);
	XwaTextureVertex* MeshTextureVertices = scene->MeshTextureVertices;
	int MeshTextureVerticesCount = *(int*)((int)scene->MeshTextureVertices - 8);
	XwaVector3* MeshNormals = scene->MeshVertexNormals;
	int MeshNormalsCount = *(int*)((int)MeshNormals - 8);
	Matrix4 MeshTransform = g_OPTMeshTransformCB.MeshTransform;
	// We premultiply in the code below, so we need to transpose the matrix because
	// the Vertex shader does a postmultiplication
	MeshTransform.transpose();

	// Intersect the current texture with the controller
	Vector4 contOrigin[2], contDir[2];
	VRControllerToOPTCoords(contOrigin, contDir);

	const float margin = 0.0001f;
	for (int contIdx = 0; contIdx < 2; contIdx++)
	{
		const bool gloveVisible = g_vrGlovesMeshes[contIdx].visible;
		//const float margin = gloveVisible ? 0.1f : 0.0001f;

		Vector3 orig = Vector4ToVector3(contOrigin[contIdx]);
		Vector3 dir  = Vector4ToVector3(contDir[contIdx]);

		// TODO: Create a TLAS just for the cockpit so that we can quickly find the intersection of the
		// ray coming from the cursor.

		Intersection bestInters;
		int bestId = -1;
		float baryU = -FLT_MAX, baryV = -FLT_MAX;
		XwaTextureVertex bestUV0, bestUV1, bestUV2;

		for (int faceIndex = 0; faceIndex < scene->FacesCount; faceIndex++)
		{
			OptFaceDataNode_01_Data_Indices& faceData = scene->FaceIndices[faceIndex];
			int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;

			// See BuildMultipleBLASFromCurrentBLASMap() too
			for (int edge = 2; edge < edgesCount; edge++)
			{
				for (int vertexIndex = 0; vertexIndex < edgesCount; vertexIndex++)
				{
					D3dTriangle t;
					t.v1 = 0;
					t.v2 = edge - 1;
					t.v3 = edge;

					Vector4 p0 = MeshTransform * XwaVector3ToVector4(MeshVertices[faceData.Vertex[t.v1]]);
					Vector4 p1 = MeshTransform * XwaVector3ToVector4(MeshVertices[faceData.Vertex[t.v2]]);
					Vector4 p2 = MeshTransform * XwaVector3ToVector4(MeshVertices[faceData.Vertex[t.v3]]);

					Vector3 v0 = Vector4ToVector3(p0);
					Vector3 v1 = Vector4ToVector3(p1);
					Vector3 v2 = Vector4ToVector3(p2);
					Vector3 P;
					float dist = FLT_MAX, u, v;
					//if (rayTriangleIntersect(orig, dir, v0, v1, v2, dist, P, u, v, margin))
					dist = ClosestPointOnTriangle(orig, v0, v1, v2, P, u, v, margin);
					{
						if (dist > -0.01f && dist < g_fBestIntersectionDistance[contIdx] && dist < GLOVE_NEAR_THRESHOLD_METERS * METERS_TO_OPT)
						{
							g_fBestIntersectionDistance[contIdx] = dist;

							baryU   = u;
							baryV   = v;
							bestUV0 = scene->MeshTextureVertices[faceData.TextureVertex[t.v1]];
							bestUV1 = scene->MeshTextureVertices[faceData.TextureVertex[t.v2]];
							bestUV2 = scene->MeshTextureVertices[faceData.TextureVertex[t.v3]];
							bestId  = faceIndex;
							if (contIdx == 0)
							{
								g_debug_v0 = v0;
								g_debug_v1 = v1;
								g_debug_v2 = v2;
								g_enable_ac_debug = true;
							}
							g_LaserPointer3DIntersection[contIdx] = P;

							//if (bGunnerTurret || bInHangar)
							if (*g_playerInHangar)
							{
								Matrix4 swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });
								const float cockpitOriginX = *g_POV_X;
								const float cockpitOriginY = *g_POV_Y;
								const float cockpitOriginZ = *g_POV_Z;

								Matrix4 T;
								T.translate(-cockpitOriginX - (g_pSharedDataCockpitLook->POVOffsetX * g_pSharedDataCockpitLook->povFactor),
								            -cockpitOriginY + (g_pSharedDataCockpitLook->POVOffsetZ * g_pSharedDataCockpitLook->povFactor),
								            -cockpitOriginZ - (g_pSharedDataCockpitLook->POVOffsetY * g_pSharedDataCockpitLook->povFactor));

								Matrix4 S = Matrix4().scale(OPT_TO_METERS);
								Matrix4 toSteamVR = swap * S;
								Vector4 Q = toSteamVR * T * Vector3ToVector4(P, 1.0f);

								// This is a bit of a crutch, but I'm temporarily storing the intersection in
								// SteamVR coords because it's easier to test the distance with the controller
								// in this framework. This shouldn't be necessary, though.
								g_LaserPointerIntersSteamVR[contIdx].x = Q.x;
								g_LaserPointerIntersSteamVR[contIdx].y = Q.y;
								g_LaserPointerIntersSteamVR[contIdx].z = Q.z;
							}
						}
					}
				}
			}
		}

		//if (bestInters.TriID != -1)
		if (bestId != -1)
		{
			// Interpolate the texture UV using the barycentric coords:
			const float baryW = 1.0f - baryU - baryV;
			const float u = baryU * bestUV0.u + baryV * bestUV1.u + baryW * bestUV2.u;
			const float v = baryU * bestUV0.v + baryV * bestUV1.v + baryW * bestUV2.v;
			g_LaserPointerBuffer.uv[contIdx][0] = u;
			g_LaserPointerBuffer.uv[contIdx][1] = v;
			g_iBestIntersTexIdx[contIdx] = _lastTextureSelected->ActiveCockpitIdx;
			/*log_debug("[DBG] [AC] bestUV0: (%0.3f, %0.3f)", bestUV0.u, bestUV0.v);
			log_debug("[DBG] [AC] bestUV1: (%0.3f, %0.3f)", bestUV1.u, bestUV1.v);
			log_debug("[DBG] [AC] bestUV2: (%0.3f, %0.3f)", bestUV2.u, bestUV2.v);
			log_debug("[DBG] [AC] baryPoint: (%0.3f, %0.3f, %0.3f), uv: %0.3f, %0.3f",
				baryU, baryV, baryW, u, v);*/
		}
	}
}

// Apply BLOOM flags and 32-bit mode enhancements
void EffectsRenderer::ApplyBloomSettings(float bloomOverride)
{
	if (!_bLastTextureSelectedNotNULL)
		return;

	if (bloomOverride > 0.0f)
	{
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = bloomOverride;
		g_PSCBuffer.bIsEngineGlow = 1;
		log_debug("[DBG] bloomOverride: %0.3f", bloomOverride);
		return;
	}

	if (_bIsLaser) {
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = _lastTextureSelected->is_Laser ?
			g_BloomConfig.fLasersStrength : g_BloomConfig.fTurboLasersStrength;
		g_PSCBuffer.bIsLaser = g_config.EnhanceLasers ? 2 : 1;
	}
	// Send the flag for light textures (enhance them in 32-bit mode, apply bloom)
	// TODO: Check if the animation for light textures still works
	else if (_bLastLightmapSelectedNotNULL) {
		_bModifiedShaders = true;
		int anim_idx = _lastTextureSelected->material.GetCurrentATCIndex(NULL, LIGHTMAP_ATC_IDX);
		g_PSCBuffer.fBloomStrength = _lastTextureSelected->is_CockpitTex ?
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
	else if (_lastTextureSelected->is_EngineGlow) {
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fEngineGlowStrength;
		g_PSCBuffer.bIsEngineGlow = g_config.EnhanceEngineGlow ? 2 : 1;
	}
	else if (_lastTextureSelected->is_Electricity || _bIsExplosion)
	{
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fExplosionsStrength;
		g_PSCBuffer.bIsEngineGlow = g_config.EnhanceExplosions ? 2 : 1;
	}
	else if (_lastTextureSelected->is_LensFlare) {
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fLensFlareStrength;
		g_PSCBuffer.bIsEngineGlow = 1;
	}
	/*
	// I believe Suns are not rendered here
	else if (bIsSun) {
		bModifiedShaders = true;
		// If there's a 3D sun in the scene, then we shouldn't apply bloom to Sun textures  they should be invisible
		g_PSCBuffer.fBloomStrength = g_b3DSunPresent ? 0.0f : g_BloomConfig.fSunsStrength;
		g_PSCBuffer.bIsEngineGlow = 1;
	} */
	else if (_lastTextureSelected->is_Spark || _lastTextureSelected->is_HitEffect) {
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fSparksStrength;
		g_PSCBuffer.bIsEngineGlow = 1;
	}
	else if (_lastTextureSelected->is_CockpitSpark) {
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fCockpitSparksStrength;
		g_PSCBuffer.bIsEngineGlow = 1;
	}
	/* else if (_lastTextureSelected->is_Chaff) // Chaff is rendered in Direct3DDevice.cpp
	{
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fSparksStrength;
		g_PSCBuffer.bIsEngineGlow = 1;
	} */
	else if (_lastTextureSelected->is_Missile)
	{
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fMissileStrength;
	}
	else if (_lastTextureSelected->is_SkydomeLight) {
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = g_BloomConfig.fSkydomeLightStrength;
		g_PSCBuffer.bIsEngineGlow = 1;
	}
	else if (!_bLastLightmapSelectedNotNULL && _lastTextureSelected->material.GetCurrentATCIndex(NULL) > -1) {
		_bModifiedShaders = true;
		// TODO: Check if this animation still works
		int anim_idx = _lastTextureSelected->material.GetCurrentATCIndex(NULL);
		// If this is an animated light map, then use the right intensity setting
		// TODO: Make the following code more efficient
		if (anim_idx > -1) {
			AnimatedTexControl *atc = &(g_AnimatedMaterials[anim_idx]);
			g_PSCBuffer.fBloomStrength = atc->Sequence[atc->AnimIdx].intensity;
		}
	}

	// Remove Bloom for all textures with materials tagged as "NoBloom"
	if (_bHasMaterial && _lastTextureSelected->material.NoBloom)
	{
		_bModifiedShaders = true;
		g_PSCBuffer.fBloomStrength = 0.0f;
		g_PSCBuffer.bIsEngineGlow = 0;
	}
}

void EffectsRenderer::ExtraPreprocessing()
{
	// Extra processing before the draw call. VR-specific stuff, for instance
}

void EffectsRenderer::AddLaserLights(const SceneCompData* scene)
{
	XwaVector3 *MeshVertices = scene->MeshVertices;
	int MeshVerticesCount = *(int*)((int)scene->MeshVertices - 8);
	XwaTextureVertex *MeshTextureVertices = scene->MeshTextureVertices;
	int MeshTextureVerticesCount = *(int*)((int)scene->MeshTextureVertices - 8);
	Vector4 tempv0, tempv1, tempv2, P;
	Vector2 UV0, UV1, UV2, UV = _lastTextureSelected->material.LightUVCoordPos;
	Matrix4 T = XwaTransformToMatrix4(scene->WorldViewTransform);

	if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled) {
		// This is a new mesh, dump all the vertices.
		//log_debug("[DBG] Writting obj_idx: %d, MeshVerticesCount: %d, TexCount: %d, FacesCount: %d",
		//	D3DOBJLaserGroup, MeshVerticesCount, MeshTextureVerticesCount, scene->FacesCount);
		fprintf(D3DDumpLaserOBJFile, "o obj-%d\n", D3DOBJLaserGroup);

		Matrix4 T = XwaTransformToMatrix4(scene->WorldViewTransform);
		for (int i = 0; i < MeshVerticesCount; i++) {
			XwaVector3 v = MeshVertices[i];
			Vector4 V(v.x, v.y, v.z, 1.0f);
			V = T * V;
			// OPT to meters conversion:
			V *= OPT_TO_METERS;
			fprintf(D3DDumpLaserOBJFile, "v %0.6f %0.6f %0.6f\n", V.x, V.y, V.z);
		}
		fprintf(D3DDumpLaserOBJFile, "\n");

		for (int i = 0; i < MeshTextureVerticesCount; i++) {
			XwaTextureVertex vt = MeshTextureVertices[i];
			fprintf(D3DDumpLaserOBJFile, "vt %0.3f %0.3f\n", vt.u, vt.v);
		}
		fprintf(D3DDumpLaserOBJFile, "\n");

		D3DOBJLaserGroup++;
	}

	// Here we look at the uv's of each face and see if the current triangle contains
	// the uv coord we're looking for (the default is (0.1, 0.5)). If the uv is contained,
	// then we compute the 3D point using its barycentric coords and add it to the list
	for (int faceIndex = 0; faceIndex < scene->FacesCount; faceIndex++)
	{
		OptFaceDataNode_01_Data_Indices& faceData = scene->FaceIndices[faceIndex];
		int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;
		// This converts quads into 2 tris if necessary
		for (int edge = 2; edge < edgesCount; edge++)
		{
			D3dTriangle t;
			t.v1 = 0;
			t.v2 = edge - 1;
			t.v3 = edge;

			UV0 = XwaTextureVertexToVector2(MeshTextureVertices[faceData.TextureVertex[t.v1]]);
			UV1 = XwaTextureVertexToVector2(MeshTextureVertices[faceData.TextureVertex[t.v2]]);
			UV2 = XwaTextureVertexToVector2(MeshTextureVertices[faceData.TextureVertex[t.v3]]);
			tempv0 = OPT_TO_METERS * T * XwaVector3ToVector4(MeshVertices[faceData.Vertex[t.v1]]);
			tempv1 = OPT_TO_METERS * T * XwaVector3ToVector4(MeshVertices[faceData.Vertex[t.v2]]);
			tempv2 = OPT_TO_METERS * T * XwaVector3ToVector4(MeshVertices[faceData.Vertex[t.v3]]);

			if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled) {
				fprintf(D3DDumpLaserOBJFile, "f %d/%d %d/%d %d/%d\n",
					faceData.Vertex[t.v1] + D3DTotalLaserVertices, faceData.TextureVertex[t.v1] + D3DTotalLaserTextureVertices,
					faceData.Vertex[t.v2] + D3DTotalLaserVertices, faceData.TextureVertex[t.v2] + D3DTotalLaserTextureVertices,
					faceData.Vertex[t.v3] + D3DTotalLaserVertices, faceData.TextureVertex[t.v3] + D3DTotalLaserTextureVertices);
			}

			// Our coordinate system has the Y-axis inverted. See also XwaD3dVertexShader
			tempv0.y = -tempv0.y;
			tempv1.y = -tempv1.y;
			tempv2.y = -tempv2.y;

			float u, v;
			if (IsInsideTriangle(UV, UV0, UV1, UV2, &u, &v)) {
				P = tempv0 + u * (tempv2 - tempv0) + v * (tempv1 - tempv0);
				// Compute the normal for this face, we'll need that for directional lights
				Vector3 e1 = Vector3(tempv1.x - tempv0.x,
									 tempv1.y - tempv0.y,
									 tempv1.z - tempv0.z);

				Vector3 e2 = Vector3(tempv2.x - tempv1.x,
									 tempv2.y - tempv1.y,
									 tempv2.z - tempv1.z);
				Vector3 N = e1.cross(e2);
				// We need to invert the Z component or the lights won't work properly.
				N.z = -N.z;
				N.normalize();

				// Prevent lasers behind the camera: they will cause a very bright flash
				if (P.z > 0.01)
					g_LaserList.insert(Vector3(P.x, P.y, P.z),
						_lastTextureSelected->material.Light,
						N,
						_lastTextureSelected->material.LightFalloff,
						_lastTextureSelected->material.LightAngle);
				//log_debug("[DBG] LaserLight: %0.1f, %0.1f, %0.1f", P.x, P.y, P.z);
			}
		}
	}

	if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled) {
		D3DTotalLaserVertices += MeshVerticesCount;
		D3DTotalLaserTextureVertices += MeshTextureVerticesCount;
		fprintf(D3DDumpLaserOBJFile, "\n");
	}
}

void EffectsRenderer::ApplyProceduralLava()
{
	static float iTime = 0.0f;
	auto &context = _deviceResources->_d3dDeviceContext;
	iTime = g_HiResTimer.global_time_s * _lastTextureSelected->material.LavaSpeed;

	_bModifiedShaders = true;
	_bModifiedPixelShader = true;
	_bModifiedSamplerState = true;

	g_ShadertoyBuffer.iTime = iTime;
	g_ShadertoyBuffer.Style = _lastTextureSelected->material.LavaTiling;
	g_ShadertoyBuffer.iResolution[0] = _lastTextureSelected->material.LavaSize;
	g_ShadertoyBuffer.iResolution[1] = _lastTextureSelected->material.EffectBloom;
	// SunColor[0] --> Color
	g_ShadertoyBuffer.SunColor[0].x = _lastTextureSelected->material.LavaColor.x;
	g_ShadertoyBuffer.SunColor[0].y = _lastTextureSelected->material.LavaColor.y;
	g_ShadertoyBuffer.SunColor[0].z = _lastTextureSelected->material.LavaColor.z;
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

	_deviceResources->InitPixelShader(_deviceResources->_lavaPS);
	// Set the noise texture and sampler state with wrap/repeat enabled.
	context->PSSetShaderResources(1, 1, _deviceResources->_grayNoiseSRV.GetAddressOf());
	// bModifiedSamplerState restores this sampler state at the end of this instruction.
	context->PSSetSamplers(1, 1, _deviceResources->_repeatSamplerState.GetAddressOf());

	// Set the constant buffer
	_deviceResources->InitPSConstantBufferHyperspace(
		_deviceResources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
}

void EffectsRenderer::ApplyGreebles()
{
	if (!g_bAutoGreeblesEnabled || !_bHasMaterial || _lastTextureSelected->material.GreebleDataIdx == -1)
		return;

	auto &resources = _deviceResources;
	auto &context = _deviceResources->_d3dDeviceContext;
	Material *material = &(_lastTextureSelected->material);
	GreebleData *greeble_data = &(g_GreebleData[material->GreebleDataIdx]);

	bool bIsRegularGreeble = (!_bLastLightmapSelectedNotNULL && greeble_data->GreebleTexIndex[0] != -1);
	bool bIsLightmapGreeble = (_bLastLightmapSelectedNotNULL && greeble_data->GreebleLightMapIndex[0] != -1);
	if (bIsRegularGreeble || bIsLightmapGreeble) {
		// 0x1: This greeble will use normal mapping
		// 0x2: This is a lightmap greeble
		// See PixelShaderGreeble for a full list of bits used in this effect
		uint32_t GreebleControlBits = bIsLightmapGreeble ? 0x2 : 0x0;
		_bModifiedShaders = true;
		_bModifiedPixelShader = true;

		resources->InitPixelShader(resources->_pixelShaderGreeble);
		if (bIsRegularGreeble) {
			g_PSCBuffer.GreebleMix1 = greeble_data->GreebleMix[0];
			g_PSCBuffer.GreebleMix2 = greeble_data->GreebleMix[1];

			g_PSCBuffer.GreebleDist1 = greeble_data->GreebleDist[0];
			g_PSCBuffer.GreebleDist2 = greeble_data->GreebleDist[1];

			g_PSCBuffer.GreebleScale1 = greeble_data->GreebleScale[0];
			g_PSCBuffer.GreebleScale2 = greeble_data->GreebleScale[1];

			uint32_t blendMask1 = greeble_data->GreebleTexIndex[0] != -1 ? greeble_data->greebleBlendMode[0] : 0x0;
			uint32_t blendMask2 = greeble_data->GreebleTexIndex[1] != -1 ? greeble_data->greebleBlendMode[1] : 0x0;
			if (blendMask1 == GBM_NORMAL_MAP || blendMask1 == GBM_UV_DISP_AND_NORMAL_MAP ||
				blendMask2 == GBM_NORMAL_MAP || blendMask2 == GBM_UV_DISP_AND_NORMAL_MAP)
				GreebleControlBits |= 0x1;
			if (blendMask1 == GBM_UV_DISP || blendMask1 == GBM_UV_DISP_AND_NORMAL_MAP ||
				blendMask2 == GBM_UV_DISP || blendMask2 == GBM_UV_DISP_AND_NORMAL_MAP)
				g_PSCBuffer.UVDispMapResolution = greeble_data->UVDispMapResolution;
			g_PSCBuffer.GreebleControl = (GreebleControlBits << 16) | (blendMask2 << 4) | blendMask1;

			// Load regular greebles...
			if (greeble_data->GreebleTexIndex[0] != -1)
				context->PSSetShaderResources(10, 1, &(resources->_extraTextures[greeble_data->GreebleTexIndex[0]]));
			if (greeble_data->GreebleTexIndex[1] != -1)
				context->PSSetShaderResources(11, 1, &(resources->_extraTextures[greeble_data->GreebleTexIndex[1]]));
		}
		else if (bIsLightmapGreeble) {
			g_PSCBuffer.GreebleMix1 = greeble_data->GreebleLightMapMix[0];
			g_PSCBuffer.GreebleMix2 = greeble_data->GreebleLightMapMix[1];

			g_PSCBuffer.GreebleDist1 = greeble_data->GreebleLightMapDist[0];
			g_PSCBuffer.GreebleDist2 = greeble_data->GreebleLightMapDist[1];

			g_PSCBuffer.GreebleScale1 = greeble_data->GreebleLightMapScale[0];
			g_PSCBuffer.GreebleScale2 = greeble_data->GreebleLightMapScale[1];

			uint32_t blendMask1 = greeble_data->GreebleLightMapIndex[0] != -1 ? greeble_data->greebleLightMapBlendMode[0] : 0x0;
			uint32_t blendMask2 = greeble_data->GreebleLightMapIndex[1] != -1 ? greeble_data->greebleLightMapBlendMode[1] : 0x0;
			if (blendMask1 == GBM_NORMAL_MAP || blendMask1 == GBM_UV_DISP_AND_NORMAL_MAP ||
				blendMask2 == GBM_NORMAL_MAP || blendMask2 == GBM_UV_DISP_AND_NORMAL_MAP)
				GreebleControlBits |= 0x1;
			if (blendMask1 == GBM_UV_DISP || blendMask1 == GBM_UV_DISP_AND_NORMAL_MAP ||
				blendMask2 == GBM_UV_DISP || blendMask2 == GBM_UV_DISP_AND_NORMAL_MAP)
				g_PSCBuffer.UVDispMapResolution = greeble_data->UVDispMapResolution;
			g_PSCBuffer.GreebleControl = (GreebleControlBits << 16) | (blendMask2 << 4) | blendMask1;

			// ... or load lightmap greebles
			if (greeble_data->GreebleLightMapIndex[0] != -1)
				context->PSSetShaderResources(10, 1, &(resources->_extraTextures[greeble_data->GreebleLightMapIndex[0]]));
			if (greeble_data->GreebleLightMapIndex[1] != -1)
				context->PSSetShaderResources(11, 1, &(resources->_extraTextures[greeble_data->GreebleLightMapIndex[1]]));
		}
	}
}

bool ATCListContainsEventType(const std::vector<ATCIndexEvtType>& ATCList, int EvtType)
{
	for (const auto& item : ATCList) {
		if (item.second == EvtType)
			return true;
	}
	return false;
}

void EffectsRenderer::ApplyAnimatedTextures(int objectId, bool bInstanceEvent, FixedInstanceData *fixedInstanceData)
{
	// Do not apply animations if there's no material or there's a greeble in the current
	// texture. All textures with a .mat file have at least the default material.
	if (!_bHasMaterial || _lastTextureSelected->material.GreebleDataIdx != -1 || g_bInTechRoom)
		return;

	bool bIsDCDamageTex = false;
	std::vector<ATCIndexEvtType> TexATCIndices, LightATCIndices;
	std::vector<bool> TexATCIndexTypes, LightATCIndexTypes; // false: Global Event, true: Instance Event
	InstanceEvent* instEvent = nullptr;
	// Random value used to alter the shields down effect (and others). These
	// values are set into InstanceEvent every time the event is triggered,
	float rand0 = 0.0f, rand1 = 0.0f, rand2 = 0.0f;

	if (bInstanceEvent) {
		// This is an instance ATC. We can have regular materials or
		// default materials in this path.
		instEvent = ObjectIDToInstanceEvent(objectId, _lastTextureSelected->material.Id);
		if (instEvent != nullptr) {
			TexATCIndices = _lastTextureSelected->material.GetCurrentInstATCIndex(objectId, *instEvent, TEXTURE_ATC_IDX);
			LightATCIndices = _lastTextureSelected->material.GetCurrentInstATCIndex(objectId, *instEvent, LIGHTMAP_ATC_IDX);
			rand0 = instEvent->rand0;
			rand1 = instEvent->rand1;
			rand2 = instEvent->rand2;

			// Populate the index types as instance events (true means this is an instance event)
			for (size_t i = 0; i < TexATCIndices.size(); i++)
				TexATCIndexTypes.push_back(true);
			for (size_t i = 0; i < LightATCIndices.size(); i++)
				LightATCIndexTypes.push_back(true);
		}
	}
	else {
		// This is a global ATC
		int TexATCIndex = _lastTextureSelected->material.GetCurrentATCIndex(&bIsDCDamageTex, TEXTURE_ATC_IDX);
		int LightATCIndex = _lastTextureSelected->material.GetCurrentATCIndex(NULL, LIGHTMAP_ATC_IDX);
		if (TexATCIndex != -1) {
			TexATCIndices.push_back(std::make_pair(TexATCIndex, -1));
			TexATCIndexTypes.push_back(false);
		}
		if (LightATCIndex != -1) {
			LightATCIndices.push_back(std::make_pair(LightATCIndex, -1));
			LightATCIndexTypes.push_back(false);
		}
	}
	
	// If the current material is not a default material, then we need to look up
	// instance events from the default material and inherit them
	if (!(_lastTextureSelected->material.bIsDefaultMaterial))
	{
		int craftIdx = _lastTextureSelected->material.craftIdx;
		if (craftIdx != -1) {
			CraftMaterials* craftMaterials = &(g_Materials[craftIdx]);
			Material* defaultMaterial = &(craftMaterials->MaterialList[0].material);
			if (defaultMaterial->bInstanceMaterial) {
				std::vector<ATCIndexEvtType> CraftTexATCIndices, CraftLightATCIndices;
				InstanceEvent* craftInstEvent = ObjectIDToInstanceEvent(objectId, defaultMaterial->Id);
				if (craftInstEvent != nullptr) {
					CraftTexATCIndices = defaultMaterial->GetCurrentInstATCIndex(objectId, *craftInstEvent, TEXTURE_ATC_IDX);
					CraftLightATCIndices = defaultMaterial->GetCurrentInstATCIndex(objectId, *craftInstEvent, LIGHTMAP_ATC_IDX);
					rand0 = craftInstEvent->rand0;
					rand1 = craftInstEvent->rand1;
					rand2 = craftInstEvent->rand2;
				}

				// Inherit animations from the Default entry
				// TODO: Avoid overwriting entries for events already in these lists
				for (const auto &atcitem : CraftTexATCIndices) {
					int ATCIndex = atcitem.first;
					int EvtType = atcitem.second;
					// Instance events in a material override events coming from Default materials.
					// In other words: if we already have an instance event of the current type, then
					// don't add the event coming from the default material.
					if (!ATCListContainsEventType(TexATCIndices, EvtType)) {
						TexATCIndices.push_back(std::make_pair(ATCIndex, EvtType));
						TexATCIndexTypes.push_back(true);
					}
				}
				for (const auto &atcitem : CraftLightATCIndices) {
					int ATCIndex = atcitem.first;
					int EvtType = atcitem.second;
					// Instance events in a material override events coming from Default materials.
					// In other words: if we already have an instance event of the current type, then
					// don't add the event coming from the default material.
					if (!ATCListContainsEventType(LightATCIndices, EvtType)) {
						LightATCIndices.push_back(std::make_pair(ATCIndex, EvtType));
						LightATCIndexTypes.push_back(true);
					}
				}
			}
		}
	}

	// If there's no texture animation and no lightmap animation, then there's nothing to do here
	if (TexATCIndices.size() == 0 && LightATCIndices.size() == 0)
		return;

	auto &resources = _deviceResources;
	auto &context = _deviceResources->_d3dDeviceContext;
	const bool bRenderingDC = g_PSCBuffer.DynCockpitSlots > 0;

	_bModifiedShaders = true;
	_bModifiedPixelShader = true;

	// If we reach this point then one of LightMapATCIndex or TextureATCIndex must be > -1 or both!
	// If we're rendering a DC element, we don't want to replace the shader
	if (!bRenderingDC)
		resources->InitPixelShader(resources->_pixelShaderAnim);

	// We're not updating the Hyperspace FSM in the D3DRendererHook, we still do it in
	// Direct3DDevice::Execute. That means that we may reach this point without knowing
	// we've entered hyperspace. Let's provide a quick update here:
	g_PSCBuffer.bInHyperspace = PlayerDataTable[*g_playerIndex].hyperspacePhase != 0 || g_HyperspacePhaseFSM != HS_INIT_ST;
	g_PSCBuffer.AuxColor.x = 1.0f;
	g_PSCBuffer.AuxColor.y = 1.0f;
	g_PSCBuffer.AuxColor.z = 1.0f;
	g_PSCBuffer.AuxColor.w = 1.0f;

	g_PSCBuffer.AuxColorLight.x = 1.0f;
	g_PSCBuffer.AuxColorLight.y = 1.0f;
	g_PSCBuffer.AuxColorLight.z = 1.0f;
	g_PSCBuffer.AuxColorLight.w = 1.0f;

	g_PSCBuffer.uvSrc0.x = g_PSCBuffer.uvSrc0.y = 0.0f;
	g_PSCBuffer.uvSrc1.x = g_PSCBuffer.uvSrc1.y = 1.0f;
	g_PSCBuffer.uvOffset.x = g_PSCBuffer.uvOffset.y = 0.0f;
	g_PSCBuffer.uvScale.x  = g_PSCBuffer.uvScale.y  = 1.0f;

	uint32_t OverlayCtrl = 0;
	int extraTexIdx = -1, extraLightIdx = -1;
	for (size_t i = 0; i < TexATCIndices.size(); i++)
	{
		const auto& texatcitem = TexATCIndices[i];
		const int TexATCIndex = texatcitem.first;
		bool bATCType = TexATCIndexTypes[i];
		AnimatedTexControl* atc = bATCType ?
			&(g_AnimatedInstMaterials[TexATCIndex]) : &(g_AnimatedMaterials[TexATCIndex]);
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

		// Apply the randomization of damage textures
		if (atc->IsHullDamageEvent())
		{
			if (fixedInstanceData != nullptr)
			{
				// Apply RAND_SCALE using fixedInstanceData
				if (atc->uvRandomScale)
				{
					float rand_val_x = lerp(atc->uvScaleMin.x, atc->uvScaleMax.x, fixedInstanceData->randScaleNorm);
					float rand_val_y = lerp(atc->uvScaleMin.y, atc->uvScaleMax.y, fixedInstanceData->randScaleNorm);
					float half_x = (1.0f - rand_val_x) / 2.0f;
					float half_y = (1.0f - rand_val_y) / 2.0f;
					atc->uvSrc0.x = half_x;
					atc->uvSrc1.x = 1.0f - half_x;
					atc->uvSrc0.y = half_y;
					atc->uvSrc1.y = 1.0f - half_y;
					/*log_debug("[DBG] [UV] uvRandomScale uvScaleMinMax: %0.3f, %0.3f, uvSrc: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
						atc->uvScaleMin.x, atc->uvScaleMax.x,
						atc->uvSrc0.x, atc->uvSrc0.y, atc->uvSrc1.x, atc->uvSrc1.y);*/
				}
				else
				{
					// Initialize uvSrc with the regular coords: we'll need this to apply RAND_LOC when
					// RAND_SCALE isn't present
					atc->uvSrc0.x = atc->uvSrc0.y = 0.0f;
					atc->uvSrc1.x = atc->uvSrc1.y = 1.0f;
				}
			}
			// Apply RAND_LOC using fixedInstanceData
			if (fixedInstanceData != nullptr && atc->uvRandomLoc)
			{
				//static int count = 0;
				float range_x, range_y, ofs_x, ofs_y;
				/*log_debug("[DBG] [UV] Initial uvSrc: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
					atc->uvSrc0.x, atc->uvSrc0.y, atc->uvSrc1.x, atc->uvSrc1.y);*/
				if (atc->uvRandomLoc == 1)
				{
					// Random location, place the texture within the [0..1] bounds so that it doesn't
					// wrap around
					range_x = atc->uvSrc0.x + (1.0f - atc->uvSrc1.x);
					range_y = atc->uvSrc0.y + (1.0f - atc->uvSrc1.y);
					// This offset moves uvSrc so that it doesn't cross the 0..1 range
					// (i.e. it doesn't "wrap around")
					ofs_x = (range_x * fixedInstanceData->randLocNorm.x) - atc->uvSrc0.x;
					ofs_y = (range_y * fixedInstanceData->randLocNorm.y) - atc->uvSrc0.y;
				}
				else
				{
					// uvRandomLoc == 2
					// Random location in the range provided by the mat file.
					range_x = atc->uvOffsetMax.x - atc->uvOffsetMin.x;
					range_y = atc->uvOffsetMax.y - atc->uvOffsetMin.y;
					ofs_x = (range_x * fixedInstanceData->randLocNorm.x) - atc->uvOffsetMin.x;
					ofs_y = (range_y * fixedInstanceData->randLocNorm.y) - atc->uvOffsetMin.y;
				}
				//log_debug("[DBG] [UV]    sel: %0.3f, %0.3f", sel_x, sel_y);
				//log_debug("[DBG] [UV]    range: %0.3f, %0.3f", range_x, range_y);
				//log_debug("[DBG] [UV]    ofs: %0.3f, %0.3f", ofs_x, ofs_y);
				atc->uvSrc0.x += ofs_x;
				atc->uvSrc0.y += ofs_y;
				atc->uvSrc1.x += ofs_x;
				atc->uvSrc1.y += ofs_y;
				/*log_debug("[DBG] [UV]    Final uvSrc: (%0.3f, %0.3f)-(%0.3f, %0.3f)",
					atc->uvSrc0.x, atc->uvSrc0.y, atc->uvSrc1.x, atc->uvSrc1.y);*/
			}
			// Apply UV_AREA (and RAND_SCALE/RAND_LOC indirectly):
			g_PSCBuffer.uvSrc0 = atc->uvSrc0;
			g_PSCBuffer.uvSrc1 = atc->uvSrc1;
		}

		g_PSCBuffer.AspectRatio = atc->AspectRatio;
		g_PSCBuffer.Clamp = atc->Clamp;
		if ((atc->OverlayCtrl & OVERLAY_CTRL_SCREEN) != 0x0) {
			g_PSCBuffer.fOverlayBloomPower = atc->Sequence[idx].intensity;
			// Only enable randomness for specific events
			if (atc->IsRandomizableOverlay()) {
				g_PSCBuffer.rand0 = rand0;
				g_PSCBuffer.rand1 = rand1;
				g_PSCBuffer.rand2 = rand2;
			}
		}
		else
			g_PSCBuffer.fBloomStrength = atc->Sequence[idx].intensity;

		if (extraTexIdx > -1) {
			if (atc->OverlayCtrl == 0) {
				// Use the following when using std::vector<ID3D11ShaderResourceView*>:
				// We cannot use InitPSShaderResourceView here because that will set slots 0 and 1, thus changing
				// the DC foreground SRV
				context->PSSetShaderResources(0, 1, &(resources->_extraTextures[extraTexIdx]));
			}
			if ((atc->OverlayCtrl & OVERLAY_CTRL_MULT) != 0x0) {
				OverlayCtrl |= OVERLAY_CTRL_MULT;
				g_PSCBuffer.OverlayCtrl = OverlayCtrl;
				// Set the animated texture in the multiplier layer
				context->PSSetShaderResources(14, 1, &(resources->_extraTextures[extraTexIdx]));
			}
			if ((atc->OverlayCtrl & OVERLAY_CTRL_SCREEN) != 0x0) {
				OverlayCtrl |= OVERLAY_CTRL_SCREEN;
				g_PSCBuffer.OverlayCtrl = OverlayCtrl;
				// Set the animated texture in the screen layer
				context->PSSetShaderResources(15, 1, &(resources->_extraTextures[extraTexIdx]));
			}

			// Force the use of damage textures if DC is on. This makes damage textures visible
			// even when no cover texture is available:
			if (bRenderingDC)
				g_DCPSCBuffer.use_damage_texture = bIsDCDamageTex;
		}
	}

	for (size_t i = 0; i < LightATCIndices.size(); i++)
	{
		const auto &lightatcitem = LightATCIndices[i];
		const int LightATCIndex = lightatcitem.first;
		bool bATCType = LightATCIndexTypes[i];
		AnimatedTexControl *atc = bATCType ?
			&(g_AnimatedInstMaterials[LightATCIndex]) : &(g_AnimatedMaterials[LightATCIndex]);
		int idx = atc->AnimIdx;
		extraLightIdx = atc->Sequence[idx].ExtraTextureIndex;

		if (atc->BlackToAlpha)
			g_PSCBuffer.special_control_light.ExclusiveMask = SPECIAL_CONTROL_BLACK_TO_ALPHA;
		else if (atc->AlphaIsBloomMask)
			g_PSCBuffer.special_control_light.ExclusiveMask = SPECIAL_CONTROL_ALPHA_IS_BLOOM_MASK;
		else
			g_PSCBuffer.special_control_light.ExclusiveMask = 0;
		g_PSCBuffer.AuxColorLight = atc->Tint;
		g_PSCBuffer.Offset = atc->Offset;
		// TODO: We need separate uvSrc settings: one for regular tex and one for lightmaps, otherwise
		//       these settings overwrite the uvSrc values set previously and the scale is altered while
		//       the shields down effect is displayed
		//g_PSCBuffer.uvSrc0 = atc->uvSrc0;
		//g_PSCBuffer.uvSrc1 = atc->uvSrc1;
		g_PSCBuffer.AspectRatio = atc->AspectRatio;
		g_PSCBuffer.Clamp = atc->Clamp;
		if ((atc->OverlayCtrl & OVERLAY_CTRL_SCREEN) != 0x0) {
			g_PSCBuffer.fOverlayBloomPower = atc->Sequence[idx].intensity;
			// Only enable randomness for specific events
			if (atc->IsRandomizableOverlay()) {
				g_PSCBuffer.rand0 = rand0;
				g_PSCBuffer.rand1 = rand1;
				g_PSCBuffer.rand2 = rand2;
			}
		}
		else
			g_PSCBuffer.fBloomStrength = atc->Sequence[idx].intensity;

		// Set the animated lightmap in slot 1, but only if we're not rendering DC -- DC uses
		// that slot for something else
		if (extraLightIdx > -1 && !bRenderingDC) {
			if (atc->OverlayCtrl == 0) {
				// Use the following when using std::vector<ID3D11ShaderResourceView*>:
				context->PSSetShaderResources(1, 1, &(resources->_extraTextures[extraLightIdx]));
			}
			if ((atc->OverlayCtrl & OVERLAY_ILLUM_CTRL_MULT) != 0x0) {
				OverlayCtrl |= OVERLAY_ILLUM_CTRL_MULT;
				g_PSCBuffer.OverlayCtrl = OverlayCtrl;
				// Set the animated texture in the multiplier layer
				context->PSSetShaderResources(14, 1, &(resources->_extraTextures[extraLightIdx]));
			}
			if ((atc->OverlayCtrl & OVERLAY_ILLUM_CTRL_SCREEN) != 0x0) {
					OverlayCtrl |= OVERLAY_ILLUM_CTRL_SCREEN;
					g_PSCBuffer.OverlayCtrl = OverlayCtrl;
				// Set the animated texture in the screen layer
				context->PSSetShaderResources(15, 1, &(resources->_extraTextures[extraLightIdx]));
			}
		}
	}
}

void EffectsRenderer::ApplyNormalMapping()
{
	if (!g_bFNEnable || !_bHasMaterial || !_lastTextureSelected->material.NormalMapLoaded ||
		_lastTextureSelected->NormalMapIdx == -1)
		return;

	auto &resources = _deviceResources;
	auto &context = _deviceResources->_d3dDeviceContext;
	Material *material = &(_lastTextureSelected->material);
	_bModifiedShaders = true;

	// Enable normal mapping and make sure the proper intensity is set
	g_PSCBuffer.bDoNormalMapping = 1;
	g_PSCBuffer.fNMIntensity = _lastTextureSelected->material.NMIntensity;
	// Set the normal map
	context->PSSetShaderResources(13, 1, &(resources->_extraTextures[_lastTextureSelected->NormalMapIdx]));
}

void EffectsRenderer::ApplyRTShadowsTechRoom(const SceneCompData* scene)
{
	_bModifiedShaders = true;
	// Enable/Disable Raytracing in the Tech Room
	g_PSCBuffer.bDoRaytracing = g_bRTEnabledInTechRoom && (_lbvh != nullptr);

	if (!g_bRTEnabledInTechRoom || !g_bInTechRoom || _lbvh == nullptr)
		return;

	auto &context = _deviceResources->_d3dDeviceContext;
	auto &resources = _deviceResources;

	Matrix4 transformWorldViewInv = _constants.transformWorldView;
	transformWorldViewInv = transformWorldViewInv.invert();

	// Update the matrices buffer
	D3D11_MAPPED_SUBRESOURCE map;
	ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
	HRESULT hr = context->Map(resources->_RTMatrices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (SUCCEEDED(hr)) {
		memcpy(map.pData, transformWorldViewInv.get(), sizeof(Matrix4));
		context->Unmap(resources->_RTMatrices.Get(), 0);
	}
	else
		log_debug("[DBG] [BVH] Failed when mapping _RTMatrices: 0x%x", hr);

	// Embedded Geometry:
	ID3D11ShaderResourceView *srvs[] = {
		resources->_RTBvhSRV.Get(),
		resources->_RTMatricesSRV.Get(),
	};
	// Slots 14-15 are used for Raytracing buffers (BVH and Matrices)
	context->PSSetShaderResources(14, 2, srvs);
}

/*
 * Returns the CraftInstance associated to an objectId. Uses g_objectIdToIndex
 * to check if objectId has been cached. If it isn't, then *objects is searched
 * for objectId and then an entry is added to the g_objectIdToIndex cache.
 */
CraftInstance *EffectsRenderer::ObjectIDToCraftInstance(int objectId, MobileObjectEntry **mobileObject_out)
{
	int objIndex = -1;
	if (objects == NULL) return nullptr;

	// Let's be extra-safe in GetPlayerCraftInstanceSafe(), we have a similar check
	// because sometimes the game will crash if we try to access the craft table in
	// the first few frames of a new mission.
	if (g_iPresentCounter <= PLAYERDATATABLE_MIN_SAFE_FRAME) return nullptr;
	if (*objects == NULL) return nullptr;

	// Have we cached the objectId?
	auto it = g_objectIdToIndex.find(objectId);
	if (it == g_objectIdToIndex.end()) {
		// There's no entry for this objId, find it and add it
		for (int i = 0; i < *g_XwaObjectsCount; i++) {
			ObjectEntry *object = &((*objects)[i]);
			if (object == NULL) return nullptr;
			if (object->objectID == objectId) {
				objIndex = i;
				g_objectIdToIndex.insert(std::make_pair(objectId, objIndex));
				break;
			}
		}
	}
	else {
		// Get the cached index
		objIndex = it->second;
	}

	if (objIndex != -1) {
		ObjectEntry *object = &((*objects)[objIndex]);
		MobileObjectEntry *mobileObject = object->MobileObjectPtr;
		if (mobileObject == NULL) return nullptr;
		if (mobileObject_out != NULL)
			*mobileObject_out = mobileObject;
		CraftInstance *craftInstance = mobileObject->craftInstancePtr;
		return craftInstance;
	}

	return nullptr;
}

/*
 * Fetches the InstanceEvent associated with the given objectId-materialId or adds a new one
 * if it doesn't exist. The materialId represents a per-texture material.
 */
InstanceEvent *EffectsRenderer::ObjectIDToInstanceEvent(int objectId, uint32_t materialId)
{
	uint64_t Id = InstEventIdFromObjectMaterialId(objectId, materialId);

	auto it = g_objectIdToInstanceEvent.find(Id);
	if (it == g_objectIdToInstanceEvent.end()) {
		// Add a new entry to g_objectIdToInstanceEvent
		log_debug("[DBG] [INST] New InstanceEvent added to objectId-matId: %d-%d",
			objectId, materialId);
		g_objectIdToInstanceEvent.insert(std::make_pair(Id, InstanceEvent()));
		auto &new_it = g_objectIdToInstanceEvent.find(Id);
		if (new_it != g_objectIdToInstanceEvent.end())
			return &new_it->second;
		else
			return nullptr;
	}
	else
		return &it->second;
}

/*
 * Fetches the FixedInstanceData associated with the given objectId-materialId or adds a new one
 * if it doesn't exist. The materialId represents a per-texture material, so the output struct
 * is unique for each objectId-materialId combination.
 */
FixedInstanceData* EffectsRenderer::ObjectIDToFixedInstanceData(int objectId, uint32_t materialId)
{
	uint64_t Id = InstEventIdFromObjectMaterialId(objectId, materialId);
	auto it = g_fixedInstanceDataMap.find(Id);
	if (it == g_fixedInstanceDataMap.end())
	{
		// Add a new entry
		log_debug("[DBG] [UV] New FixedInstance added to objectId-matId: %d-%d",
			objectId, materialId);
		g_fixedInstanceDataMap.insert(std::make_pair(Id, FixedInstanceData()));
		auto& new_it = g_fixedInstanceDataMap.find(Id);
		if (new_it != g_fixedInstanceDataMap.end())
			return &new_it->second;
		else
			return nullptr;
	}
	else
		return &it->second;
}

bool EffectsRenderer::GetOPTNameFromLastTextureSelected(char *OPTname)
{
	char sToken[] = "Flightmodels\\";
	char* subString = stristr(_lastTextureSelected->_name.c_str(), sToken);
	// Sometimes we'll get a DAT texture here, so subString can be null:
	if (subString == nullptr) {
		OPTname[0] = 0;
		return false;
	}
	subString += strlen(sToken);
	int i = 0;
	while (subString[i] != 0 && subString[i] != '.') {
		OPTname[i] = subString[i];
		i++;
	}
	OPTname[i] = 0;
	return true;
}

// Update g_TLASMap: checks if we've seen the current mesh in this frame. If we
// haven't seen this mesh, a new matrix slot is requested and a new (meshKey, matrixSlot)
// entry is added to g_TLASMap. Otherwise we fetch the matrixSlot for the meshKey.
//
// Regular Flight:
// Update g_BLASMap: checks if the current mesh/face group combination is new.
// If it is, then a new blasData entry will be added to g_BLASMap and this will
// request a BLAS tree to be built at the end of the frame.
//
// Tech Room:
// Update g_LBVHMap: checks if the current mesh/face group combination is new.
// If it is, then a new face group will be added to the meshData tuple in g_LBVHMap.
// This will request a single coalesced BLAS rebuild at the end of the frame.
//
// The same matrixSlot is used for both maps and makes a direct link between the TLAS
// and the BLASes
void EffectsRenderer::UpdateBVHMaps(const SceneCompData* scene, int LOD)
{
	XwaVector3* MeshVertices = scene->MeshVertices;
	int MeshVerticesCount    = *(int*)((int)scene->MeshVertices - 8);
	int32_t meshKey          = MakeMeshKey(scene);
	int32_t faceGroupID      = MakeFaceGroupKey(scene);
	int matrixSlot           = -1;
	int blasID               = -1;
	FaceGroups FGs;
	FGs.clear();

	// Fetch the ID for this (MeshKey, LOD)
	const BLASKey_t blasKey = BLASKey_t(meshKey, LOD);
	const auto& bit = g_BLASIdMap.find(blasKey);
	if (bit == g_BLASIdMap.end())
	{
		// We've never seen this Mesh/LOD before, create a new ID and add it
		blasID = RTGetNextBlasID();
		g_BLASIdMap[blasKey] = blasID;
	}
	else
	{
		blasID = bit->second;
	}

	// Update g_TLASMap and get a new BlasID and Matrix Slot if necessary -- or find the
	// existing blasID and matrixSlot for the current mesh/LOD/centroid
	if (!g_bInTechRoom)
	{
		// This is the correct transform chain to apply the Diegetic Joystick in RT; but
		// I'm getting double shadows in the X-Wing (the A-Wing is fine). I think there's
		// multiple profiles/meshes and maybe the transform is not applied to all the relevant
		// meshes.
		//Matrix4 MeshTransform = g_OPTMeshTransformCB.MeshTransform;
		//MeshTransform = MeshTransform.invert();
		//const Matrix4 W = XwaTransformToMatrix4(scene->WorldViewTransform) * MeshTransform;
		const Matrix4 W = XwaTransformToMatrix4(scene->WorldViewTransform);

		// Fetch the AABB for this mesh
		auto aabb_it = _AABBs.find(meshKey);
		auto center_it = _centers.find(meshKey);
		if (aabb_it != _AABBs.end()) {
			AABB obb = aabb_it->second;                   // The AABB in object space
			obb.UpdateLimits();                           // Generate all the vertices (8) so that we can transform them.
			obb.TransformLimits(W);                       // Now it's an OBB in WorldView space...
			AABB aabb = obb.GetAABBFromCurrentLimits();   // so we get the AABB from this OBB...
			Vector3 centroid = aabb.GetCentroidVector3(); // and its centroid.
			// Repeat the process for the mesh's center of mass
			if (g_bUseCentroids)
			{
				XwaVector3 xwacenter = center_it->second;
				Vector4 center(xwacenter.x, xwacenter.y, xwacenter.z, 1.0f);
				center = W * center;   // Now the center is in WorldView space
				centroid.x = center.x; // And we override the centroid with the center of mass now
				centroid.y = center.y;
				centroid.z = center.z;
			}

			IDCentroid_t IDCentroidKey = IDCentroid_t(blasID, centroid.x, centroid.y, centroid.z);
			auto it = g_TLASMap.find(IDCentroidKey);
			if (it == g_TLASMap.end())
			{
				// We haven't seen this (blasID, centroid) combination before, add a new entry
				matrixSlot = RTGetNextAvailableMatrixSlot();
				g_TLASMap[IDCentroidKey] = matrixSlot;
				// Store the matrix proper, but inverted. That's what the RT code needs so that
				// we can transform from WorldView to OPT-coords
				Matrix4 WInv = W;
				WInv = WInv.invert();
				if (matrixSlot >= (int)g_TLASMatrices.size())
					g_TLASMatrices.resize(g_TLASMatrices.size() + 128);
				g_TLASMatrices[matrixSlot] = WInv;

				// Add a new entry to tlasLeaves and update the global centroid
				//AddAABBToTLAS(W, meshKey, obb, centroid, matrixSlot);
				g_GlobalAABB.Expand(aabb);
				g_GlobalCentroidAABB.Expand(centroid);
				tlasLeaves.push_back({ 0, centroid, aabb, blasID, matrixSlot, obb });

				if (g_bActiveCockpitEnabled && (_bIsCockpit || _bIsGunner))
				{
					//ACtlasLeaves.push_back({ 0, centroid, aabb, blasID, matrixSlot, obb });
					// Instead of the matrixSlot, let's store te blasID. That gets copied to the TLAS tree and
					// we can use it to jump to the proper BLAS.
					g_ACtlasLeaves.push_back({ 0, centroid, aabb, blasID, blasID, obb });
				}
			}
			else
			{
				// We have seen this mesh/centroid before, get its matrix slot
				matrixSlot = it->second;
			}
		}
	}

	if (!g_bInTechRoom)
	{
		// Now update the g_BLASMap so that we can build multiple BLASes if needed.
		BLASData blasData;
		BLASGetBVH(blasData) = nullptr; // Initialize the new blasData BVH to NULL
		auto it = g_BLASMap.find(blasID);
		if (it != g_BLASMap.end())
		{
			// We've seen this (Mesh, LOD) before, we need to check
			// if we've seen this FG before too
			blasData = it->second;
			FGs = BLASGetFaceGroups(blasData);
			// The FG key is FaceIndices:
			auto it = FGs.find(faceGroupID);
			if (it != FGs.end())
			{
				// We've seen this mesh/FG combination before, ignore
				return;
			}
		}

		// Signal that there's at least one BLAS that needs to be rebuilt
		_BLASNeedsUpdate = true;

		// Update the g_BLASMap entry
		// Delete any previous BVH for this blasData entry
		LBVH* bvh = (LBVH*)BLASGetBVH(blasData);
		if (bvh != nullptr)
			delete bvh;
		BLASGetBVH(blasData)          = nullptr; // Force this BVH to be rebuilt
		// Add the FG to the map so that it's not processed again.
		FGs[faceGroupID]              = scene->FacesCount;
		BLASGetFaceGroups(blasData)   = FGs;
		BLASGetMeshVertices(blasData) = (int)scene->MeshVertices;
		BLASGetNumVertices(blasData)  = scene->VerticesCount;
		g_BLASMap[blasID]             = blasData;
	}
	else
	{
		// Now update the g_LBVHMap so that we can rebuild BLASes if needed.
		MeshData meshData;
		FaceGroups FGs;
		GetLBVH(meshData) = nullptr; // Initialize the new meshData BVH to NULL
		auto it = g_LBVHMap.find(meshKey);
		// We have seen this mesh before, but we need to check if we've seen
		// the FG as well
		if (it != g_LBVHMap.end())
		{
			// Check if we've seen this FG group before
			meshData = it->second;
			FGs = GetFaceGroups(meshData);
			// The FG key is FaceIndices:
			auto it = FGs.find((int32_t)scene->FaceIndices);
			if (it != FGs.end())
			{
				// We've seen this mesh/FG combination before, ignore
				return;
			}
		}

		// Signal that there's at least one BLAS that needs to be rebuilt
		_BLASNeedsUpdate = true;
		// Delete any previous BVH for this mesh
		LBVH* bvh = (LBVH*)GetLBVH(meshData);
		if (bvh != nullptr)
			delete bvh;

		// Update the g_LBVHMap
		// Add the FG to the map so that it's not processed again
		FGs[(int32_t)scene->FaceIndices] = scene->FacesCount;
		// Update the FaceGroup in the meshData
		GetFaceGroups(meshData) = FGs;
		GetNumMeshVertices(meshData) = scene->VerticesCount;
		GetLBVH(meshData) = nullptr; // Force an update on this BLAS (only used outside the Tech Room)
		g_LBVHMap[meshKey] = meshData;
	}

	// DEBUG: Add the OPT's name to g_MeshToNameMap
#ifdef DEBUG_RT
	{
		char OPTname[128];
		GetOPTNameFromLastTextureSelected(OPTname);
		g_DebugMeshToNameMap[meshKey] = std::tuple(
			std::string(OPTname),
			MeshVerticesCount,
			_currentOptMeshIndex);
	}
#endif
}

/// <summary>
/// Exclude projectiles, debris, salvage yard and DS2 out of RT. Also excludes specific
/// meshes by OPT name or ship category (i.e. applies the raytracing_exclude_opt and
/// raytracing_exclude_class settings).
/// </summary>
bool EffectsRenderer::RTCheckExcludeMesh(const SceneCompData* scene)
{
	const int Genus = scene->pObject->ShipCategory;
	const bool bNonRTObject =
		(Genus == Genus_PlayerProjectile) ||
		(Genus == Genus_SmallDebris) ||
		(Genus == Genus_OtherProjectile) ||
		(Genus == Genus_SalvageYard) ||
		(Genus == Genus_Deathstar2);

	if (bNonRTObject)
		return true;

	// Let's check if this mesh has been tagged (for skipping)
	const int meshKey = MakeMeshKey(scene);
	const auto& it = g_RTExcludeMeshes.find(meshKey);
	// If this mesh has been tagged, just return the tag
	if (it != g_RTExcludeMeshes.end())
		return it->second;

	// This mesh has not been tagged, let's check and tag:
	// Check if this ship's genus must be excluded
	const auto& git = g_RTExcludeShipCategories.find(Genus);
	if (git != g_RTExcludeShipCategories.end())
	{
		// Yes: this genus must be excluded, tag and return
		g_RTExcludeMeshes[meshKey] = true;
		return true;
	}

	// This genus must not be excluded, let's check for name exclusions
	char OPTName[128];
	GetOPTNameFromLastTextureSelected(OPTName);
	toupper(OPTName);
	//log_debug("[DBG] OPT: %s, Genus: %d", OPTName, scene->pObject->ShipCategory);

	// Skydome, Planet3D: No shadows
	if ((stristr(OPTName, "Skydome") != NULL) ||
		(stristr(OPTName, "Planet3D") != NULL))
	{
		g_RTExcludeMeshes[meshKey] = true;
		return true;
	}

	// OPTs containing "Land" must receive shadows even from far away...
	// ... TODO

	const auto& nit = g_RTExcludeOPTNames.find(std::string(OPTName));
	if (nit != g_RTExcludeOPTNames.end())
	{
		//log_debug("[DBG] [BVH] Skipping mesh: 0x%x, for OPT: %s", meshKey, OPTName);
		// Name is in the list, exclude it
		g_RTExcludeMeshes[meshKey] = true;
		return true;
	}

	// Neither the genus nor the name are excluded. Tag and return (we can render it):
	g_RTExcludeMeshes[meshKey] = false;
	return false;
}

void EffectsRenderer::MainSceneHook(const SceneCompData* scene)
{
	auto &context = _deviceResources->_d3dDeviceContext;
	auto &resources = _deviceResources;

	_overrideRTV = TRANSP_LYR_NONE;

	ComPtr<ID3D11Buffer> oldVSConstantBuffer;
	ComPtr<ID3D11Buffer> oldPSConstantBuffer;
	ComPtr<ID3D11ShaderResourceView> oldVSSRV[3];

	const bool bExternalCamera = g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME &&
		PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;

#ifdef DISABLED
	if (s_captureProjectionDeltas)
	{
		// DEBUG: Display changes in projection constants.
		// There's two sets of constants: cockpit camera and external camera. The change is small, though.
#ifdef DISABLED
		if (g_f0x08C1600 != *(float*)0x08C1600 ||
			g_f0x0686ACC != *(float*)0x0686ACC ||
			g_f0x080ACF8 != *(float*)0x080ACF8 ||
			g_f0x07B33C0 != *(float*)0x07B33C0 ||
			g_f0x064D1AC != *(float*)0x064D1AC)
		{
			log_debug("[DBG] [PRJ] Projection constants change detected.");
			log_debug("[DBG] [PRJ] Prev: %0.3f, %0.3f :: %0.3f, %0.3f, %0.3f",
				g_f0x08C1600,
				g_f0x0686ACC,
				g_f0x080ACF8,
				g_f0x07B33C0,
				g_f0x064D1AC);
			log_debug("[DBG] [PRJ] New: %0.3f, %0.3f :: %0.3f, %0.3f, %0.3f",
				*(float*)0x08C1600,
				*(float*)0x0686ACC,
				*(float*)0x080ACF8,
				*(float*)0x07B33C0,
				*(float*)0x064D1AC);
		}
#endif
		g_f0x08C1600 = *(float*)0x08C1600;
		g_f0x0686ACC = *(float*)0x0686ACC;
		g_f0x080ACF8 = *(float*)0x080ACF8;
		g_f0x07B33C0 = *(float*)0x07B33C0;
		g_f0x064D1AC = *(float*)0x064D1AC;

		_frameConstants = _constants;
		s_captureProjectionDeltas = false;
	}
#endif

	context->VSGetConstantBuffers(0, 1, oldVSConstantBuffer.GetAddressOf());
	context->PSGetConstantBuffers(0, 1, oldPSConstantBuffer.GetAddressOf());
	context->VSGetShaderResources(0, 3, oldVSSRV[0].GetAddressOf());

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	_deviceResources->InitRasterizerState(g_isInRenderLasers ? _rasterizerState : _rasterizerStateCull);
	_deviceResources->InitRasterizerState(_rasterizerState);
	_deviceResources->InitSamplerState(_samplerState.GetAddressOf(), nullptr);

	if (scene->TextureAlphaMask == 0)
	{
		_deviceResources->InitBlendState(_solidBlendState, nullptr);
		_deviceResources->InitDepthStencilState(_solidDepthState, nullptr);
		_bIsTransparentCall = false;
	}
	else
	{
		_deviceResources->InitBlendState(_transparentBlendState, nullptr);
		_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);
		_bIsTransparentCall = true;
	}

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);
	ID3D11PixelShader* lastPixelShader = InTechGlobe() ? _techRoomPixelShader : _pixelShader;
	_deviceResources->InitPixelShader(lastPixelShader);

	UpdateTextures(scene);
	UpdateMeshBuffers(scene);
	UpdateVertexAndIndexBuffers(scene);
	UpdateConstantBuffer(scene);

	// Effects Management starts here.
	// Do the state management
	DoStateManagement(scene);

	// DEBUG
	// The scene pointer seems to be the same for every draw call, but the contents change.
	// scene->TextureName seems to be NULL all the time.
	// We can now have texture names associated with a specific ship instance. Meshes and faceData
	// can be rendered multiple times per frame, but we object ID is unique. We also have access to
	// the MobileObjectEntry and the CraftInstance. In other words: we can now apply effects on a
	// per-ship, per-texture basis. See the example below...
	//log_debug("[DBG] Rendering scene: 0x%x, faceData: 0x%x %s",
	//	scene, scene->FaceIndices, g_isInRenderLasers ? "LASER" : "");
	//MobileObjectEntry *pMobileObject = (MobileObjectEntry *)scene->pObject->pMobileObject;
	//log_debug("[DBG] FaceData: 0x%x, Id: %d, Species: %d, Category: %d",
	//	scene->FaceIndices, scene->pObject->ObjectId, scene->pObject->ObjectSpecies, scene->pObject->ShipCategory);
	// Show extra debug information on the current mesh:
	/*
	if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled)
		log_debug("[DBG] [%s]: Mesh: 0x%x, faceData: 0x%x, Id: %d, Type: %d, Genus: %d, Player: %d",
			_bLastTextureSelectedNotNULL ? _lastTextureSelected->_name.c_str() : "(null)", scene->MeshVertices, scene->FaceIndices,
			scene->pObject->ObjectId, scene->pObject->ObjectSpecies, scene->pObject->ShipCategory, *g_playerIndex);
	*/

	/*
	// The preybird cockpit re-uses the same mesh, but different faceData:
	[500] [DBG] [opt,FlightModels\PreybirdFighterCockpit.opt,TEX00051,color,0]: Mesh: 0x183197a3, faceData: 0x18319bac, Id: 3, Type: 29, Genus: 0
	[500] [DBG] [opt,FlightModels\PreybirdFighterCockpit.opt,TEX00052,color,0]: Mesh: 0x183197a3, faceData: 0x18319ddd, Id: 3, Type: 29, Genus: 0
	[500] [DBG] [opt,FlightModels\PreybirdFighterCockpit.opt,TEX00054,color,0]: Mesh: 0x183197a3, faceData: 0x1831a00e, Id: 3, Type: 29, Genus: 0
	[500] [DBG] [opt,FlightModels\PreybirdFighterCockpit.opt,TEX00053,color,0]: Mesh: 0x183197a3, faceData: 0x1831a23f, Id: 3, Type: 29, Genus: 0

	// These are several different TIE Fighters: the faceData and Texname are repeated, but we see different IDs:
	[500][DBG][opt, FlightModels\TieFighter.opt, TEX00029, color, 0] : Mesh : 0x1dde3181, faceData : 0x1ddecd62, Id : 8,  Type : 5, Genus : 0
	[500][DBG][opt, FlightModels\TieFighter.opt, TEX00029, color, 0] : Mesh : 0x1dd3f0f2, faceData : 0x1dd698c0, Id : 11, Type : 5, Genus : 0
	[500][DBG][opt, FlightModels\TieFighter.opt, TEX00029, color, 0] : Mesh : 0x1dde3181, faceData : 0x1ddecd62, Id : 11, Type : 5, Genus : 0
	[500][DBG][opt, FlightModels\TieFighter.opt, TEX00029, color, 0] : Mesh : 0x1dd3f0f2, faceData : 0x1dd698c0, Id : 10, Type : 5, Genus : 0
	[500][DBG][opt, FlightModels\TieFighter.opt, TEX00029, color, 0] : Mesh : 0x1dde3181, faceData : 0x1ddecd62, Id : 10, Type : 5, Genus : 0
	[500][DBG][opt, FlightModels\TieFighter.opt, TEX00029, color, 0] : Mesh : 0x1dd3f0f2, faceData : 0x1dd698c0, Id : 12, Type : 5, Genus : 0
	[500][DBG][opt, FlightModels\TieFighter.opt, TEX00029, color, 0] : Mesh : 0x1dde3181, faceData : 0x1ddecd62, Id : 12, Type : 5, Genus : 0
	*/

	/*
	if (stristr(_lastTextureSelected->_name.c_str(), "TIE") != 0 &&
		((stristr(_lastTextureSelected->_name.c_str(), "TEX00023") != 0 ||
		  stristr(_lastTextureSelected->_name.c_str(), "TEX00032") != 0))
	   )
	*/
	
	// Cache the current object's ID
	int objectId = -1;
	if (scene != nullptr && scene->pObject != nullptr)
		objectId = scene->pObject->ObjectId;
	const bool bInstanceEvent = _lastTextureSelected->material.bInstanceMaterial && objectId != -1;
	const int materialId = _lastTextureSelected->material.Id;
	float bloomOverride = -1.0f;
	FixedInstanceData* fixedInstanceData = nullptr;

	// UPDATE THE STATE OF INSTANCE EVENTS.
	// A material is associated with either a global ATC or an instance ATC for now.
	// Not sure if it would be legal to have both, but I'm going to simplify things.
	if (bInstanceEvent)
	{
		MobileObjectEntry* mobileObject = nullptr;
		CraftInstance *craftInstance = ObjectIDToCraftInstance(objectId, &mobileObject);
		InstanceEvent *instanceEvent = ObjectIDToInstanceEvent(objectId, materialId);
		fixedInstanceData = ObjectIDToFixedInstanceData(objectId, materialId);
		if (craftInstance != nullptr && mobileObject != nullptr) {
			int hull = max(0, (int)(100.0f * (1.0f - (float)craftInstance->HullDamageReceived / (float)craftInstance->HullStrength)));
			int shields = craftInstance->ShieldPointsBack + craftInstance->ShieldPointsFront;
			// This value seems to be somewhat arbitrary. ISDs seem to be 741 when healthy,
			// and TIEs seem to be 628. But either way, this value is 0 when disabled. I think.
			int subsystems = craftInstance->SubsystemStatus;
			float curThrottle = craftInstance->EngineThrottleInput / 65535.0f; // Percentage in the range: 0..1
			//float topSpeed = craftInstance->TopSpeedMPH / 2.25f;
			int curSpeed = (int)(mobileObject->currentSpeed / 2.25f); // MGLT
			int curMissionSetSpeed = (int)(craftInstance->MissionSetSpeed / 2.25f); // MGLT

			bool DisplayIfMissionSetSpeedGE = (curThrottle >= 0.99f && curMissionSetSpeed >= _lastTextureSelected->material.DisplayIfMissionSetSpeedGE);
			if (_lastTextureSelected->material.DisplayIfMissionSetSpeedGE > 0 && !DisplayIfMissionSetSpeedGE)
			{
				goto out;
			}

			if (curSpeed < _lastTextureSelected->material.DisplayIfSpeedGE)
			{
				goto out;
			}

			if (curThrottle < _lastTextureSelected->material.DisplayIfThrottleGE)
			{
				goto out;
			}

			if (_lastTextureSelected->material.SkipWhenDisabled && subsystems == 0)
			{
				//log_debug("[DBG] [%s], systems: %d", _lastTextureSelected->_name.c_str(), systems);
				goto out;
			}

			if (_lastTextureSelected->material.IncreaseBrightnessWithMissionSetSpeed > 0)
			{
				bloomOverride = g_BloomConfig.fEngineGlowStrength *
					(float)curMissionSetSpeed / (float)_lastTextureSelected->material.IncreaseBrightnessWithMissionSetSpeed;
				//bloomOverride = g_BloomConfig.fEngineGlowStrength * curThrottle;
			}
			
			if (instanceEvent != nullptr) {
				// Update the event for this instance
				instanceEvent->ShieldBeamEvent = IEVT_NONE;
				instanceEvent->HullEvent = IEVT_NONE;

				if (shields == 0)
					instanceEvent->ShieldBeamEvent = IEVT_SHIELDS_DOWN;
				if (craftInstance->IsUnderBeamEffect[1] != 0)
					instanceEvent->ShieldBeamEvent = IEVT_TRACTOR_BEAM;
				if (craftInstance->IsUnderBeamEffect[2] != 0)
					instanceEvent->ShieldBeamEvent = IEVT_JAMMING_BEAM;

				if (50.0f < hull && hull <= 75.0f)
					instanceEvent->HullEvent = IEVT_HULL_DAMAGE_75;
				else if (25.0f < hull && hull <= 50.0f)
					instanceEvent->HullEvent = IEVT_HULL_DAMAGE_50;
				else if (hull <= 25.0f)
					instanceEvent->HullEvent = IEVT_HULL_DAMAGE_25;
			}
		}
	}

	// The main 3D content is rendered here, that includes the cockpit and 3D models. But
	// there's content that is still rendered in Direct3DDevice::Execute():
	// - The backdrop, including the Suns
	// - Engine Glow
	// - The HUD, including the reticle
	// - Explosions, including the DS2 core explosion
	/*
		We have an interesting mixture of Execute() calls and Hook calls. The sequence for
		each frame, looks like this:
		[11528][DBG] * ****************** PRESENT 3D
		[11528][DBG] BeginScene <-- Old method
		[11528][DBG] SceneBegin <-- New Hook
		[11528][DBG] Execute(1) <-- Old method (the backdrop is probably rendered here)
		[17076][DBG] EffectsRenderer::MainSceneHook
		[17076][DBG] EffectsRenderer::MainSceneHook
		... a bunch of calls to MainSceneHook. Most of the 3D content is rendered here ...
		[17076][DBG] EffectsRenderer::MainSceneHook
		[11528][DBG] Execute(1) <-- The engine glow might be rendered here (?)
		[11528][DBG] Execute(1) <-- Maybe the HUD and reticle is rendered here (?)
		[11528][DBG] EndScene   <-- Old method
		[11528][DBG] SceneEnd   <-- New Hook
		[11528][DBG] * ****************** PRESENT 3D
	*/
	g_PSCBuffer = { 0 };
	g_PSCBuffer.brightness = MAX_BRIGHTNESS;
	g_PSCBuffer.fBloomStrength  = 1.0f;
	g_PSCBuffer.fPosNormalAlpha = 1.0f;
	g_PSCBuffer.fSSAOAlphaMult  = g_fSSAOAlphaOfs;
	g_PSCBuffer.fSSAOMaskVal    = g_DefaultGlobalMaterial.Metallic * 0.5f;
	g_PSCBuffer.fGlossiness     = g_DefaultGlobalMaterial.Glossiness;
	g_PSCBuffer.fSpecInt        = g_DefaultGlobalMaterial.Intensity;  // DEFAULT_SPEC_INT;
	g_PSCBuffer.fNMIntensity    = g_DefaultGlobalMaterial.NMIntensity;
	g_PSCBuffer.AuxColor.x  = 1.0f;
	g_PSCBuffer.AuxColor.y  = 1.0f;
	g_PSCBuffer.AuxColor.z  = 1.0f;
	g_PSCBuffer.AuxColor.w  = 1.0f;
	g_PSCBuffer.AspectRatio = 1.0f;
	g_PSCBuffer.Offset      = float2(0, 0);
	g_PSCBuffer.uvSrc1.x = g_PSCBuffer.uvSrc1.y = 1.0f;
	if (g_config.OnlyGrayscaleInTechRoom)
		g_PSCBuffer.special_control.ExclusiveMask = SPECIAL_CONTROL_GRAYSCALE;

	// Initialize the mesh transform for each mesh. During MainSceneHook,
	// this transform may be modified to apply an animation. See
	// ApplyDiegeticCockpit() and ApplyMeshTransform()
	g_OPTMeshTransformCB.MeshTransform.identity();

	// We will be modifying the regular render state from this point on. The state and the Pixel/Vertex
	// shaders are already set by this point; but if we modify them, we'll set bModifiedShaders to true
	// so that we can restore the state at the end of the draw call.
	_bModifiedShaders = false;
	_bModifiedPixelShader = false;
	_bModifiedBlendState = false;
	_bModifiedSamplerState = false;

	// Apply specific material properties for the current texture
	ApplyMaterialProperties();

	// Apply the SSAO mask/Special materials, like lasers and HUD
	ApplySpecialMaterials();

	ApplyNormalMapping();

	// Animate the Diegetic Cockpit (joystick, throttle, hyper-throttle, etc)
	ApplyDiegeticCockpit();

	// Animate the current mesh (if applicable)
	ApplyMeshTransform();

	ApplyActiveCockpit(scene);

	if (g_bInTechRoom)
	{
		ApplyRTShadowsTechRoom(scene);
	}
	else
	{
		g_RTConstantsBuffer.bRTEnable = g_bRTEnabled &&	(!*g_playerInHangar) &&
			(g_HyperspacePhaseFSM == HS_INIT_ST);
		// g_RTConstantsBuffer.bRTEnabledInCockpit = g_bRTEnabledInCockpit;
		g_RTConstantsBuffer.bRTAllowShadowMapping =
			// Allow shadow mapping if we're in the hangar, or if RT is disabled...
			*g_playerInHangar || !g_bRTEnabled ||
			// Or if we're not in the hangar, not in external view and RTCockpit is off
			(!*g_playerInHangar && !_bExternalCamera && !g_bRTEnabledInCockpit);
		//g_RTConstantsBuffer.bEnablePBRShading = 0;
		//g_RTConstantsBuffer.bEnablePBRShading = g_bEnablePBRShading;
		//g_RTConstantsBuffer.bEnablePBRShading      = g_bRTEnabled; // Let's force PBR shading when RT is on, at least for now
		g_RTConstantsBuffer.RTEnableSoftShadows    = g_bRTEnableSoftShadows;
		g_RTConstantsBuffer.RTShadowMaskPixelSizeX = g_fCurScreenWidthRcp;
		g_RTConstantsBuffer.RTShadowMaskPixelSizeY = g_fCurScreenHeightRcp;
		g_RTConstantsBuffer.RTSoftShadowThreshold  = g_fRTSoftShadowThresholdMult;
		g_RTConstantsBuffer.RTGaussFactor          = g_fRTGaussFactor;
		resources->InitPSRTConstantsBuffer(resources->_RTConstantsBuffer.GetAddressOf(), &g_RTConstantsBuffer);
	}

	// EARLY EXIT 1: Render the targetted craft to the Dynamic Cockpit RTVs and continue
	if (g_bDynCockpitEnabled && (g_bIsFloating3DObject || g_isInRenderMiniature)) {
		DCCaptureMiniature();
		goto out;
	}

	// Modify the state for both VR and regular game modes...

	// Maintain the k-closest lasers to the camera (but ignore the miniature lasers)
	if ((g_bEnableLaserLights && _bIsLaser && _bHasMaterial && !g_bStartedGUI) ||
		_lastTextureSelected->material.IsLightEmitter)
		AddLaserLights(scene);

	// Apply BLOOM flags and 32-bit mode enhancements
	// TODO: This code expects either a lightmap or a regular texture, but now we can have both at the same time
	// this will mess up all the animation logic when both the regular and lightmap layers have animations
	ApplyBloomSettings(bloomOverride);

	// Transparent textures are currently used with DC to render floating text. However, if the erase region
	// commands are being ignored, then this will cause the text messages to be rendered twice. To avoid
	// having duplicated messages, we're removing these textures here when the erase_region commmands are
	// not being applied.
	// The above behavior is overridden if the DC element is set as "always_visible". In that case, the
	// transparent layer will remain visible even when the HUD is displayed.
	if (g_bDCManualActivate && g_bDynCockpitEnabled && _bDCIsTransparent && !g_bDCApplyEraseRegionCommands && !_bDCElemAlwaysVisible)
		goto out;

	// Dynamic Cockpit: Replace textures at run-time. Returns true if we need to skip the current draw call
	if (DCReplaceTextures())
		goto out;

	// TODO: Update the Hyperspace FSM -- but only update it exactly once per frame.
	// Looks like the code to do this update in Execute() still works. So moving on for now

	// Capture the cockpit OPT -> View transform. It's used for ShadowMapping and VR gloves
	if (!_bCockpitConstantsCaptured && (_bIsCockpit || _bIsGunner))
	{
		_bCockpitConstantsCaptured = true;
		_CockpitConstants = _constants;
		_CockpitWorldView = scene->WorldViewTransform;
	}

	if (!_bExteriorConstantsCaptured && _bIsExterior)
	{
		_bExteriorConstantsCaptured = true;
		_ExteriorConstants = _constants;
	}

	// Procedural Lava
	if (g_bProceduralLava && _bLastTextureSelectedNotNULL && _bHasMaterial && _lastTextureSelected->material.IsLava)
		ApplyProceduralLava();

	ApplyGreebles();

	if (g_bEnableAnimations)
		ApplyAnimatedTextures(objectId, bInstanceEvent, fixedInstanceData);

	if (g_bInTechRoom)
	{
		g_PSCBuffer.AuxColor.x = g_config.TechRoomMetallicity;
		g_PSCBuffer.AuxColor.y = g_config.TechRoomAmbient;
	}

	// BLAS/TLAS construction
	// Only add these vertices to the BLAS if the texture is not transparent
	// (engine glows are transparent and may both cast and catch shadows
	// otherwise)... and other conditions
	if (((g_bRTEnabledInTechRoom && InTechGlobe()) || g_bRTEnabled || g_bActiveCockpitEnabled) &&
		g_rendererType != RendererType_Shadow && // This is a hangar shadow, ignore
		!(*g_playerInHangar) && // Disable raytracing when parked in the hangar
		_bLastTextureSelectedNotNULL &&
		!_lastTextureSelected->is_Transparent &&
		!_lastTextureSelected->is_LightTexture)
	{
		bool bSkipCockpit = (_bIsCockpit && !g_bRTEnabledInCockpit);
		if (g_bActiveCockpitEnabled)
		{
			bSkipCockpit = false;
		}
		bool bExclude = RTCheckExcludeMesh(scene);
		//bool bRaytrace = _lastTextureSelected->material.Raytrace;

		if (!bExclude && !bSkipCockpit && !_bIsLaser && !_bIsExplosion && !_bIsGunner &&
			!(g_bIsFloating3DObject || g_isInRenderMiniature))
		{
			// DEBUG
			/*
			char OPTname[120];
			GetOPTNameFromLastTextureSelected(OPTname);
			if (stristr(OPTname, "MediumTransport") != NULL && _currentOptMeshIndex == 0)
			{
				{
					int faceGroupID = MakeFaceGroupKey(scene);
					auto& it = g_FGToLODMap.find(faceGroupID);
					if (it != g_FGToLODMap.end())
					{
						log_debug("[DBG] [OPT] %s FG 0x%x --> %d",
							OPTname, faceGroupID, it->second);
					}
				}
			}
			*/

			// Find the LOD for this FaceGroup. We need this so that we can coalesce
			// all the FGs belonging to the same LOD in one BVH. That improves performance.
			int faceGroupId = MakeFaceGroupKey(scene);
			auto& it = g_FGToLODMap.find(faceGroupId);
			int LOD = -1;
			if (it != g_FGToLODMap.end())
			{
				LOD = it->second;
			}
			else
				log_debug("[DBG] [BVH] ERROR. No LOD for FG: 0x%x", faceGroupId);
			// Populate the TLAS and BLAS maps so that we can build BVHs at the end of the frame
			UpdateBVHMaps(scene, LOD);
		}
	}

	// Additional processing for VR or similar. Not implemented in this class, but will be in
	// other subclasses.
	ExtraPreprocessing();

	// Capture the projection constants and other data needed to render the sky cylinder.
	if (s_captureProjectionDeltas)
	{
		s_captureProjectionDeltas = false;

		g_f0x08C1600 = *(float*)0x08C1600;
		g_f0x0686ACC = *(float*)0x0686ACC;
		g_f0x080ACF8 = *(float*)0x080ACF8;
		g_f0x07B33C0 = *(float*)0x07B33C0;
		g_f0x064D1AC = *(float*)0x064D1AC;

		_frameConstants = _constants;
		_frameVSCBuffer = g_VSCBuffer;
	}

	// Apply the changes to the vertex and pixel shaders
	//if (bModifiedShaders) 
	{
		// Tech Room Hologram control
		if (g_bInTechRoom && g_config.TechRoomHolograms)
		{
			g_PSCBuffer.rand0 = 8.0f * g_fDCHologramTime;
			g_PSCBuffer.rand1 = 1.0f;
		}
		resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
		resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		if (g_PSCBuffer.DynCockpitSlots > 0)
			resources->InitPSConstantBufferDC(resources->_PSConstantBufferDC.GetAddressOf(), &g_DCPSCBuffer);
		// Set the current mesh transform
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);
	}

	// Dump the current scene to an OBJ file
	if (g_bDumpSSAOBuffers && bD3DDumpOBJEnabled) {
		// The coordinates are in Object (OPT) space and scale, centered on the origin.
		//OBJDump(scene->MeshVertices, *(int*)((int)scene->MeshVertices - 8));

		// This function is called once per face group. Each face group is associated with a single texture.
		// A single mesh can contain multiple face groups.
		// An OPT contains multiple meshes
		// _verticesCount has the number of vertices in the current face group
		//log_debug("[DBG] _vertices.size(): %lu, _verticesCount: %d", _vertices.size(), _verticesCount);
		OBJDumpD3dVertices(scene, Matrix4().identity());
	}

	// There's a bug with the lasers: they are sometimes rendered at the start of the frame, causing them to be
	// displayed *behind* the geometry. To fix this, we're going to save all the lasers in a list and then render
	// them at the end of the frame.
	// TODO: Instead of storing draw calls, use a deferred context to record the calls and then execute it later
	if (_bIsLaser) {
		DrawCommand command;
		// There's apparently a bug in the latest D3DRendererHook ddraw: the miniature does not set the proper
		// viewport, so lasers and other projectiles that are rendered in the CMD also show in the bottom of the
		// screen. This is not a fix, but a workaround: we're going to skip rendering any such objects if we've
		// started rendering the CMD.
		// Also, it looks like we can't use g_isInRenderMiniature for this check, since that doesn't seem to work
		// in some cases; we need to use g_bIsFloating3DObject instead.
		if (g_bStartedGUI || g_bIsFloating3DObject || g_isInRenderMiniature)
			goto out;

		// Save the current draw command and skip. We'll render the lasers later
		// Save the textures
		command.colortex = _lastTextureSelected;
		command.lighttex = _lastLightmapSelected;
		// Save the SRVs
		command.vertexSRV = _lastMeshVerticesView;
		command.normalsSRV = _lastMeshVertexNormalsView;
		command.tangentsSRV = _lastMeshVertexTangentsView;
		command.texturesSRV = _lastMeshTextureVerticesView;
		// Save the vertex and index buffers
		command.vertexBuffer = _lastVertexBuffer;
		command.indexBuffer = _lastIndexBuffer;
		command.trianglesCount = _trianglesCount;
		// Save the constants
		command.constants = _constants;
		command.meshTransformMatrix.identity();
		// Add the command to the list of deferred commands
		_LaserDrawCommands.push_back(command);

		// Do not render the laser at this moment
		goto out;
	}

	// Transparent polygons are sometimes rendered in the middle of a frame, causing them to appear
	// behind other geometry. We need to store those draw calls and render them later, near the end
	// of the frame.
	// TODO: Instead of storing draw calls, use a deferred context to record the calls and then execute it later
	if (_bIsTransparentCall) {
		DrawCommand command;
		// Save the current draw command and skip. We'll render the transparency later

		// Save the textures. The following will increase the refcount on the SRVs, we need
		// to decrease it later to avoid memory leaks
		for (int i = 0; i < 2; i++)
			command.SRVs[i] = nullptr;
		context->PSGetShaderResources(0, 2, command.SRVs);
		// Save the Vertex, Normal and UVs SRVs
		command.vertexSRV = _lastMeshVerticesView;
		command.normalsSRV = _lastMeshVertexNormalsView;
		command.tangentsSRV = _lastMeshVertexTangentsView;
		command.texturesSRV = _lastMeshTextureVerticesView;
		// Save the vertex and index buffers
		command.vertexBuffer = _lastVertexBuffer;
		command.indexBuffer = _lastIndexBuffer;
		command.trianglesCount = _trianglesCount;
		// Save the constants
		command.constants = _constants;
		// Save extra data
		command.PSCBuffer = g_PSCBuffer;
		command.DCPSCBuffer = g_DCPSCBuffer;
		command.bIsCockpit = _bIsCockpit;
		command.bIsGunner = _bIsGunner;
		command.bIsBlastMark = _bIsBlastMark;
		command.pixelShader = resources->GetCurrentPixelShader();
		command.meshTransformMatrix = g_OPTMeshTransformCB.MeshTransform;
		// Add the command to the list of deferred commands
		_TransparentDrawCommands.push_back(command);
		goto out;
	}

	RenderScene(false);

out:
	// The hyperspace effect needs the current VS constants to work properly
	if (g_HyperspacePhaseFSM == HS_INIT_ST)
		context->VSSetConstantBuffers(0, 1, oldVSConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, oldPSConstantBuffer.GetAddressOf());
	context->VSSetShaderResources(0, 3, oldVSSRV[0].GetAddressOf());

	if (_bModifiedPixelShader)
		resources->InitPixelShader(lastPixelShader);
	_overrideRTV = TRANSP_LYR_NONE;

	// Decrease the refcount of all the objects we queried at the prologue. (Is this
	// really necessary? They live on the stack, so maybe they are auto-released?)
	/*
	oldVSConstantBuffer.Release();
	oldPSConstantBuffer.Release();
	for (int i = 0; i < 3; i++)
		oldVSSRV[i].Release();
	*/

#if LOGGER_DUMP
	DumpConstants(_constants);
	DumpVector3(scene->MeshVertices, *(int*)((int)scene->MeshVertices - 8));
	DumpTextureVertices(scene->MeshTextureVertices, *(int*)((int)scene->MeshTextureVertices - 8));
	DumpD3dVertices(_vertices.data(), _verticesCount);
#endif
}

/*
 If the game is rendering the hyperspace effect, this function will select shaderToyBuf
 when rendering the cockpit. Otherwise it will select the regular offscreenBuffer
 */
inline ID3D11RenderTargetView *EffectsRenderer::SelectOffscreenBuffer() {
	auto& resources = this->_deviceResources;

	ID3D11RenderTargetView *regularRTV = resources->_renderTargetView.Get();
	// Since we're now splitting the background and the 3D content, we don't need the shadertoyRTV
	// anymore. When hyperspace is activated, no external OPTs are rendered, so we still just get
	// the cockpit on the regularRTV
	if (_overrideRTV == TRANSP_LYR_1) return resources->_transp1RTV;
	if (_overrideRTV == TRANSP_LYR_2) return resources->_transp2RTV;
	if (_overrideRTV == BACKGROUND_LYR) return resources->_backgroundRTV;
	// Normal output buffer (_offscreenBuffer)
	return regularRTV;
}

// This function should only be called when the miniature (targetted craft) is being rendered.
void EffectsRenderer::DCCaptureMiniature()
{
	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;

	// The viewport for the miniature is not properly set at the moment. So lasers and
	// projectiles (?) and maybe other objects show outside the CMD. We need to avoid
	// capturing them.
	if (_bIsLaser || _lastTextureSelected->is_Missile) return;

	// Remember the current scissor rect before modifying it
	UINT NumRects = 1;
	D3D11_RECT rect;
	context->RSGetScissorRects(&NumRects, &rect);

	unsigned short scissorLeft = *(unsigned short*)0x07D5244;
	unsigned short scissorTop = *(unsigned short*)0x07CA354;
	unsigned short scissorWidth = *(unsigned short*)0x08052B8;
	unsigned short scissorHeight = *(unsigned short*)0x07B33BC;
	float scaleX = _viewport.Width / _deviceResources->_displayWidth;
	float scaleY = _viewport.Height / _deviceResources->_displayHeight;
	D3D11_RECT scissor{};
	// The scissor is in screen coordinates.
	scissor.left = (LONG)(_viewport.TopLeftX + scissorLeft * scaleX + 0.5f);
	scissor.top = (LONG)(_viewport.TopLeftY + scissorTop * scaleY + 0.5f);
	scissor.right = scissor.left + (LONG)(scissorWidth * scaleX + 0.5f);
	scissor.bottom = scissor.top + (LONG)(scissorHeight * scaleY + 0.5f);
	_deviceResources->InitScissorRect(&scissor);

	// Apply the brightness settings to the pixel shader
	g_PSCBuffer.brightness = g_fBrightness;
	// Restore the non-VR dimensions:
	_deviceResources->InitViewport(&_viewport);
	//resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	_deviceResources->InitVertexShader(_vertexShader);
	_deviceResources->InitPixelShader(_pixelShader);
	//ID3D11DepthStencilView* ds = g_bUseSteamVR ? NULL : resources->_depthStencilViewL.Get();
	ID3D11DepthStencilView* ds = g_bUseSteamVR ? resources->_depthStencilViewR.Get() : resources->_depthStencilViewL.Get();
	// Select the proper RTV
	context->OMSetRenderTargets(1, resources->_renderTargetViewDynCockpit.GetAddressOf(), ds);

	// Enable Z-Buffer since we're drawing the targeted craft
	QuickSetZWriteEnabled(TRUE);

	// Disable any custom MeshTransform's we may have loaded from the VR Gloves or HMD.
	// I could not remove the HMD's transform cleanly, but at least the targeted craft no
	// longer appears outside the CMD in ships like the YT-2000.
	if (g_bUseSteamVR)
	{
		/*{
			std::string msg;
			Vector3 P;
			int y = 50;

			msg = "ypr: " + std::to_string(g_HMDYaw) + ", " + std::to_string(g_HMDPitch) + ", " + std::to_string(g_HMDRoll);
			DisplayCenteredText((char*)msg.c_str(), FONT_LARGE_IDX, y, FONT_WHITE_COLOR);
			y += 25;
		}*/

		Matrix4 V;
		// This transform chain was copied from CustomHMD where roll is similarly applied
		// along the current view direction. It works here, but only when viewing the ships
		// "head-on". Looks like the targeted ship's own orientation is also in play here
		/*Matrix4 R = Matrix4().rotateZ(-g_HMDYaw) * Matrix4().rotateX(g_HMDPitch);
		Matrix4 Rinv = R;
		Rinv.invert();
		Matrix4 Rx = Rinv * Matrix4().rotateY(-g_HMDRoll) * R;
		V = R * Rx;*/

		/*
		const float* m = g_VSMatrixCB.fullViewMat.get();
		const float x = m[12];
		const float y = m[13];
		const float z = m[14];
		V.translate(-x * METERS_TO_OPT, -z * METERS_TO_OPT, -y * METERS_TO_OPT);
		*/
		V.identity();

		g_OPTMeshTransformCB.MeshTransform = V;
		g_OPTMeshTransformCB.MeshTransform.transpose();
		_deviceResources->InitVSConstantOPTMeshTransform(_deviceResources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);
		g_OPTMeshTransformCB.MeshTransform.identity();
	}

	// Render
	if (g_bUseSteamVR)
		context->DrawIndexedInstanced(_trianglesCount * 3, 1, 0, 0, 0); // if (g_bUseSteamVR)
	else
		context->DrawIndexed(_trianglesCount * 3, 0, 0);
	g_iHUDOffscreenCommandsRendered++;

	// Restore the regular texture, RTV, shaders, etc:
	context->PSSetShaderResources(0, 1, _lastTextureSelected->_textureView.GetAddressOf());
	context->OMSetRenderTargets(1, resources->_renderTargetView.GetAddressOf(), resources->_depthStencilViewL.Get());
	/*
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
	*/
	// Restore the Pixel Shader constant buffers:
	g_PSCBuffer.brightness = MAX_BRIGHTNESS;
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);

	// Restore the scissor rect to its previous value
	_deviceResources->InitScissorRect(&rect);

	if (g_bDumpSSAOBuffers) {
		DirectX::SaveWICTextureToFile(context, resources->_offscreenBufferDynCockpit, GUID_ContainerFormatJpeg, L"c:\\temp\\_DC-FG-Input-Raw.jpg");
	}
}

bool EffectsRenderer::DCReplaceTextures()
{
	bool bSkip = false;
	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;

	// Dynamic Cockpit: Replace textures at run-time:
	if (!g_bDCManualActivate || !g_bDynCockpitEnabled || !_bLastTextureSelectedNotNULL || !_lastTextureSelected->is_DynCockpitDst ||
		// We should never render lightmap textures with the DC pixel shader:
		_lastTextureSelected->is_DynCockpitAlphaOverlay) {
		bSkip = false;
		goto out;
	}

	int idx = _lastTextureSelected->DCElementIndex;

	if (g_HyperspacePhaseFSM != HS_INIT_ST) {
		// If we're in hyperspace, let's set the corresponding flag before rendering DC controls
		_bModifiedShaders = true;
		g_PSCBuffer.bInHyperspace = 1;
	}

	// Check if this idx is valid before rendering
	if (idx >= 0 && idx < g_iNumDCElements) {
		dc_element *dc_element = &g_DCElements[idx];
		if (dc_element->bActive) {
			_bModifiedShaders = true;
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
				g_DCPSCBuffer.noisy_holo = _bIsNoisyHolo;
				g_DCPSCBuffer.transparent = _bDCIsTransparent;
				g_DCPSCBuffer.use_damage_texture = false;
				if (_bWarheadLocked)
					g_DCPSCBuffer.bgColor[numCoords] = dc_element->coords.uWHColor[i];
				else
					g_DCPSCBuffer.bgColor[numCoords] = _bIsTargetHighlighted ?
					dc_element->coords.uHGColor[i] :
					dc_element->coords.uBGColor[i];
				// The hologram property will make *all* uvcoords in this DC element
				// holographic as well:
				//bIsHologram |= (dc_element->bHologram);
				numCoords++;
			} // for
			// g_bDCHologramsVisible is a hard switch, let's use g_fDCHologramFadeIn instead to
			// provide a softer ON/OFF animation
			if (_bIsHologram && g_fDCHologramFadeIn <= 0.01f) {
				bSkip = true;
				goto out;
			}
			g_PSCBuffer.DynCockpitSlots = numCoords;
			//g_PSCBuffer.bUseCoverTexture = (dc_element->coverTexture != nullptr) ? 1 : 0;
			g_PSCBuffer.bUseCoverTexture = (resources->dc_coverTexture[idx] != nullptr) ? 1 : 0;
			// If there are no DC elements to display and no cover texture, then there's nothing to do here.
			// If we continue anyway and replace the textures below, artifacts will appear on empty elements.
			// Artifacts appear on ships with no beam weapon in the beam weapon DC cockpit area, for instance.
			if (numCoords == 0 && !g_PSCBuffer.bUseCoverTexture)
				goto out;

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
			}
			else
				context->PSSetShaderResources(0, 1, _lastTextureSelected->_textureView.GetAddressOf());
			//resources->InitPSShaderResourceView(lastTextureSelected->_textureView.Get());
		// No need for an else statement, slot 0 is already set to:
		// context->PSSetShaderResources(0, 1, texture->_textureView.GetAddressOf());
		// See D3DRENDERSTATE_TEXTUREHANDLE, where lastTextureSelected is set.
			if (g_PSCBuffer.DynCockpitSlots > 0) {
				_bModifiedPixelShader = true;
				if (_bIsHologram) {
					EnableHoloTransparency();
					_bModifiedBlendState = true;
					uint32_t hud_color = (*g_XwaFlightHudColor) & 0x00FFFFFF;
					//log_debug("[DBG] hud_color, border, inside: 0x%x, 0x%x", *g_XwaFlightHudBorderColor, *g_XwaFlightHudColor);
					g_ShadertoyBuffer.iTime = g_fDCHologramTime;
					g_ShadertoyBuffer.twirl = g_fDCHologramFadeIn;
					// Override the background color if the current DC element is a hologram:
					g_DCPSCBuffer.bgColor[0] = hud_color;
					resources->InitPSConstantBufferHyperspace(resources->_hyperspaceConstantBuffer.GetAddressOf(), &g_ShadertoyBuffer);
				}
				resources->InitPixelShader(_bIsHologram ? resources->_pixelShaderDCHolo : resources->_pixelShaderDC);
			}
			else if (g_PSCBuffer.bUseCoverTexture) {
				_bModifiedPixelShader = true;
				resources->InitPixelShader(resources->_pixelShaderEmptyDC);
			}
		} // if dc_element->bActive
	}

out:
	return bSkip;
}

void EffectsRenderer::RenderScene(bool bBindTranspLyr1)
{
	if (_deviceResources->_displayWidth == 0 || _deviceResources->_displayHeight == 0)
	{
		return;
	}

	// Skip hangar shadows if configured:
	if (g_rendererType == RendererType_Shadow && !g_config.HangarShadowsEnabled)
		return;

	auto& resources = _deviceResources;
	auto& context = _deviceResources->_d3dDeviceContext;

	if (g_iD3DExecuteCounter == 0 && !g_bInTechRoom)
	{
		// Temporarily replace the background with a solid color to debug MSAA halos:
		//float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		//float bgColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		//context->ClearRenderTargetView(resources->_renderTargetView, bgColor);

		// Populate the SSAOMask and SSMask buffers with default materials
		// This is also done in Direct3DDevice::BeginScene(), but if we don't clean these RTVs here
		// we see some artifacts when doing a hyperjump.
		float blankMaterial[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float shadelessMaterial[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

		if (!g_bMapMode)
		{
			context->ClearRenderTargetView(resources->_renderTargetViewSSAOMask, blankMaterial);
			context->ClearRenderTargetView(resources->_renderTargetViewSSMask, shadelessMaterial);

			context->CopyResource(resources->_backgroundBuffer, resources->_offscreenBuffer);
			// Wipe out the background:
			context->ClearRenderTargetView(resources->_renderTargetView, resources->clearColor);
			//if (g_bUseSteamVR)
				//context->ClearRenderTargetView(resources->_renderTargetViewHd, resources->clearColor); // Probably not necessary
		}

		if (g_bDumpSSAOBuffers)
			DirectX::SaveDDSTextureToFile(context, resources->_backgroundBuffer, L"c:\\temp\\_backgroundBuffer.dds");
		g_bBackgroundCaptured = true;
	}

	unsigned short scissorLeft = *(unsigned short*)0x07D5244;
	unsigned short scissorTop = *(unsigned short*)0x07CA354;
	unsigned short scissorWidth = *(unsigned short*)0x08052B8;
	unsigned short scissorHeight = *(unsigned short*)0x07B33BC;
	float scaleX = _viewport.Width / _deviceResources->_displayWidth;
	float scaleY = _viewport.Height / _deviceResources->_displayHeight;
	D3D11_RECT scissor{};
	// The scissor is in screen coordinates.
	scissor.left = (LONG)(_viewport.TopLeftX + scissorLeft * scaleX + 0.5f);
	scissor.top = (LONG)(_viewport.TopLeftY + scissorTop * scaleY + 0.5f);
	scissor.right = scissor.left + (LONG)(scissorWidth * scaleX + 0.5f);
	scissor.bottom = scissor.top + (LONG)(scissorHeight * scaleY + 0.5f);
	_deviceResources->InitScissorRect(&scissor);

	// This method isn't called to draw the hyperstreaks or the hypertunnel. A different
	// (unknown, maybe RenderMain?) path is taken instead.

	ID3D11RenderTargetView *rtvs[7] = {
		SelectOffscreenBuffer(), // Select the main RTV

		_deviceResources->_renderTargetViewBloomMask.Get(),
		g_bAOEnabled ? _deviceResources->_renderTargetViewDepthBuf.Get() : NULL,
		// The normals hook should not be allowed to write normals for light textures. This is now implemented
		// in XwaD3dPixelShader
		_deviceResources->_renderTargetViewNormBuf.Get(),
		// Blast Marks are confused with glass because they are not shadeless; but they have transparency
		_bIsBlastMark ? NULL : _deviceResources->_renderTargetViewSSAOMask.Get(),
		_bIsBlastMark ? NULL : _deviceResources->_renderTargetViewSSMask.Get(),
		bBindTranspLyr1 ? resources->_transp1RTV.Get() : NULL,
	};
	context->OMSetRenderTargets(7, rtvs, _deviceResources->_depthStencilViewL.Get());

	// DEBUG: Skip draw calls if we're debugging the process
	/*
	if (g_iD3DExecuteCounterSkipHi > -1 && g_iD3DExecuteCounter > g_iD3DExecuteCounterSkipHi)
		goto out;
	if (g_iD3DExecuteCounterSkipLo > -1 && g_iD3DExecuteCounter < g_iD3DExecuteCounterSkipLo)
		goto out;
	*/

	context->DrawIndexed(_trianglesCount * 3, 0, 0);

	//RenderCascadedShadowMap();

//out:
	g_iD3DExecuteCounter++;
	g_iDrawCounter++; // EffectsRenderer. We need this counter to enable proper Tech Room detection
}

void EffectsRenderer::RenderLasers()
{
	if (_LaserDrawCommands.size() == 0)
		return;
	_deviceResources->BeginAnnotatedEvent(L"RenderLasers");
	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;
	//log_debug("[DBG] Rendering %d deferred draw calls", _LaserDrawCommands.size());

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rastersize and depth stencil states for transparency
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);
	_deviceResources->InitPixelShader(_pixelShader);

	g_PSCBuffer = { 0 };
	g_PSCBuffer.brightness = MAX_BRIGHTNESS;
	g_PSCBuffer.fBloomStrength  = 1.0f;
	g_PSCBuffer.fPosNormalAlpha = 1.0f;
	g_PSCBuffer.fSSAOAlphaMult  = g_fSSAOAlphaOfs;
	g_PSCBuffer.AuxColor.x  = 1.0f;
	g_PSCBuffer.AuxColor.y  = 1.0f;
	g_PSCBuffer.AuxColor.z  = 1.0f;
	g_PSCBuffer.AuxColor.w  = 1.0f;
	g_PSCBuffer.AspectRatio = 1.0f;

	// Laser-specific stuff from ApplySpecialMaterials():
	g_PSCBuffer.fSSAOMaskVal    = 0;
	g_PSCBuffer.fGlossiness     = DEFAULT_GLOSSINESS;
	g_PSCBuffer.fSpecInt        = DEFAULT_SPEC_INT;
	g_PSCBuffer.fNMIntensity    = 0.0f;
	g_PSCBuffer.fSpecVal        = 0.0f;
	g_PSCBuffer.bIsShadeless    = 1;
	g_PSCBuffer.fPosNormalAlpha = 0.0f;
	// Laser-specific stuff from ApplyBloomSettings():
	g_PSCBuffer.fBloomStrength = g_BloomConfig.fLasersStrength;

	g_OPTMeshTransformCB.MeshTransform.identity();

	// Flags used in RenderScene():
	_bIsCockpit = false;
	_bIsGunner = false;
	_bIsBlastMark = false;

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	//resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
	resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;
	// Run the deferred commands
	for (DrawCommand command : _LaserDrawCommands) {
		// Set the textures
		_deviceResources->InitPSShaderResourceView(command.colortex->_textureView.Get(),
			command.lighttex == nullptr ? nullptr : command.lighttex->_textureView.Get());

		// Set the mesh buffers
		ID3D11ShaderResourceView* vsSSRV[4] = { command.vertexSRV, command.normalsSRV, command.texturesSRV, command.tangentsSRV };
		context->VSSetShaderResources(0, 4, vsSSRV);

		// Set the index and vertex buffers
		_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
		_deviceResources->InitVertexBuffer(&(command.vertexBuffer), &vertexBufferStride, &vertexBufferOffset);
		_deviceResources->InitIndexBuffer(nullptr, true);
		_deviceResources->InitIndexBuffer(command.indexBuffer, true);

		// Set the constants buffer
		context->UpdateSubresource(_constantBuffer, 0, nullptr, &(command.constants), 0, 0);

		// Set the number of triangles
		_trianglesCount = command.trianglesCount;

		// Never set TRANSP_LYR_1 and RenderScene(true) at the same time!
		// That'll cause transp1RTV to be bound to two slots!
		_overrideRTV = TRANSP_LYR_1;
		// Render the deferred commands
		RenderScene(false);
	}

	// Clear the command list and restore the previous state
	_LaserDrawCommands.clear();
	RestoreContext();

	if (g_bDumpSSAOBuffers)
	{
		DirectX::SaveDDSTextureToFile(context, resources->_transpBuffer1, L"C:\\Temp\\_transpBuffer1Lasers.dds");
	}
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderTransparency()
{
	if (_TransparentDrawCommands.size() == 0)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderTransparencyAndDC");

	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rastersize and depth stencil states for transparency
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	// Run the deferred commands
	for (DrawCommand command : _TransparentDrawCommands) {
		g_PSCBuffer = command.PSCBuffer;
		g_DCPSCBuffer = command.DCPSCBuffer;

		// Flags used in RenderScene():
		_bIsCockpit = command.bIsCockpit;
		_bIsGunner = command.bIsGunner;
		_bIsBlastMark = command.bIsBlastMark;

		// Apply the VS and PS constants
		resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
		//resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);
		g_OPTMeshTransformCB.MeshTransform = command.meshTransformMatrix;
		resources->InitVSConstantOPTMeshTransform(
			resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);
		if (g_PSCBuffer.DynCockpitSlots > 0)
			resources->InitPSConstantBufferDC(resources->_PSConstantBufferDC.GetAddressOf(), &g_DCPSCBuffer);

		// Set the textures
		_deviceResources->InitPSShaderResourceView(command.SRVs[0], command.SRVs[1]);

		// Set the mesh buffers
		ID3D11ShaderResourceView* vsSSRV[4] = { command.vertexSRV, command.normalsSRV, command.texturesSRV, command.tangentsSRV };
		context->VSSetShaderResources(0, 4, vsSSRV);

		// Set the index and vertex buffers
		_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
		_deviceResources->InitVertexBuffer(&(command.vertexBuffer), &vertexBufferStride, &vertexBufferOffset);
		_deviceResources->InitIndexBuffer(nullptr, true);
		_deviceResources->InitIndexBuffer(command.indexBuffer, true);

		// Set the constants buffer
		context->UpdateSubresource(_constantBuffer, 0, nullptr, &(command.constants), 0, 0);

		// Set the number of triangles
		_trianglesCount = command.trianglesCount;

		// Set the right pixel shader
		_deviceResources->InitPixelShader(command.pixelShader);

		if (!g_bInTechRoom)
		{
			// We can't select TRANSP_LYR_1 here, because some OPTs have solid areas and transparency,
			// like the windows on the CRS. If we set _overrideRTV to TRANSP_LRY_1, then the whole window
			// is rendered on a transparent layer which is later blended without shading the solid areas.
			// Instead, we bind transp1RTV during RenderScene() below so that we can selectively render
			// to that layer or offscreenBuffer depending on the alpha channel.
			_overrideRTV = _bIsCockpit ? TRANSP_LYR_2 : TRANSP_LYR_NONE;
		}
		// Render the deferred commands
		RenderScene(true);

		// Decrease the refcount of the textures
		for (int i = 0; i < 2; i++)
			if (command.SRVs[i] != nullptr) command.SRVs[i]->Release();
	}

	// Clear the command list and restore the previous state
	_TransparentDrawCommands.clear();
	ID3D11RenderTargetView* rtvs[7] = { NULL, NULL, NULL, NULL,  NULL, NULL, NULL };
	context->OMSetRenderTargets(7, rtvs, resources->_depthStencilViewL.Get());
	RestoreContext();

	if (g_bDumpSSAOBuffers)
	{
		DirectX::SaveDDSTextureToFile(context, resources->_transpBuffer1, L"C:\\Temp\\_transpBuffer1.dds");
		DirectX::SaveDDSTextureToFile(context, resources->_transpBuffer2, L"C:\\Temp\\_transpBuffer2.dds");
	}

	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderVRDots()
{
	if (!g_bUseSteamVR || !g_bRendering3D || !g_bActiveCockpitEnabled || _bDotsbRendered || !_bCockpitConstantsCaptured)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderVRDots");

	// Test intersections on additional geometry (VR Keyboard, gloves)
	if (g_bActiveCockpitEnabled)
	{
		IntersectVRGeometry();
	}

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;
	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const bool bInHangar = *g_playerInHangar;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rasterizer and depth stencil states for transparency
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	// _mainDepthState is D3D11_COMPARISON_ALWAYS, so the VR dots are always displayed
	_deviceResources->InitDepthStencilState(_deviceResources->_mainDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	ZeroMemory(&g_PSCBuffer, sizeof(g_PSCBuffer));
	g_PSCBuffer.bIsShadeless = 1;
	g_PSCBuffer.fPosNormalAlpha = 0.0f;
	// fSSAOAlphaMult ?
	// fSSAOMaskVal ?
	// fPosNormalAlpha ?
	// fBloomStrength ?
	// bInHyperspace ?

	g_VRGeometryCBuffer.numStickyRegions = 0;
	// Disable region highlighting
	g_VRGeometryCBuffer.clicked[0] = false;
	g_VRGeometryCBuffer.clicked[1] = false;
	g_VRGeometryCBuffer.bRenderBracket = false;

	// Flags used in RenderScene():
	_bIsCockpit = !bGunnerTurret;
	_bIsGunner = bGunnerTurret;
	_bIsBlastMark = false;

	// Set the textures
	_deviceResources->InitPSShaderResourceView(_vrGreenCirclesSRV.Get(), nullptr);

	// Set the mesh buffers
	ID3D11ShaderResourceView* vsSSRV[4] = { _vrDotMeshVerticesSRV.Get(), nullptr, _vrDotMeshTexCoordsSRV.Get(), nullptr };
	context->VSSetShaderResources(0, 4, vsSSRV);

	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
	_deviceResources->InitVertexBuffer(_vrDotVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(nullptr, true);
	_deviceResources->InitIndexBuffer(_vrDotIndexBuffer.Get(), true);

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitVRGeometryCBuffer(resources->_VRGeometryCBuffer.GetAddressOf(), &g_VRGeometryCBuffer);
	_deviceResources->InitPixelShader(resources->_pixelShaderVRGeom);

	// Set the constants buffer
	Matrix4 Vinv;
	if (bGunnerTurret || bInHangar)
	{
		// For the Gunner Turret, we're going to remove the world-view transform and replace it
		// with an identity matrix. That way, the gloves, which are already in viewspace coords,
		// will follow the headset no matter how the turret is oriented.
		Matrix4 Id;
		const float* m = Id.get();
		for (int i = 0; i < 16; i++) _CockpitConstants.transformWorldView[i] = m[i];

		Vinv = g_VSMatrixCB.fullViewMat;
		Vinv.invert();
	}
	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_CockpitConstants, 0, 0);
	_trianglesCount = g_vrDotNumTriangles;

	Matrix4 V, swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });

	for (int contIdx = 0; contIdx < 2; contIdx++)
	{
		if (g_iBestIntersTexIdx[contIdx] == -1)
			continue;

		// Compute a new matrix for the dot by using the origin -> intersection point view vector.
		// First we'll align this vector with Z+ and then we'll use the inverse of this matrix to
		// rotate the dot so that it always faces the origin.
		{
			Vector4 P;
			if (!bGunnerTurret && !bInHangar)
			{
				// g_LaserPointerIntersSteamVR is not populated in this case, so we need to transform
				// g_LaserPointer3DIntersection, which is in OPT scale, into the SteamVR coord sys.
				const float cockpitOriginX = *g_POV_X;
				const float cockpitOriginY = *g_POV_Y;
				const float cockpitOriginZ = *g_POV_Z;
				Matrix4 T;

				T.translate(-cockpitOriginX - (g_pSharedDataCockpitLook->POVOffsetX * g_pSharedDataCockpitLook->povFactor),
				            -cockpitOriginY + (g_pSharedDataCockpitLook->POVOffsetZ * g_pSharedDataCockpitLook->povFactor),
				            -cockpitOriginZ - (g_pSharedDataCockpitLook->POVOffsetY * g_pSharedDataCockpitLook->povFactor));

				Matrix4 S = Matrix4().scale(OPT_TO_METERS);
				Matrix4 toSteamVR = swap * S;
				P = toSteamVR * T * Vector3ToVector4(g_LaserPointer3DIntersection[contIdx], 1.0f);
			}
			else
				P = Vector3ToVector4(g_LaserPointerIntersSteamVR[contIdx], 1);

			// O is the headset's center, in SteamVR coords:
			Vector4 O = g_VSMatrixCB.fullViewMat * Vector4(0, 0, 0, 1);
			// N goes from the intersection point to the headset's origin: it's the view vector now
			Vector4 N = P - O;
			//log_debug_vr(50 + contIdx * 25, FONT_WHITE_COLOR, "P[%d]: %0.3f, %0.3f, %0.3f", contIdx, P.x, P.y, P.z);

			N.normalize();
			// Rotate N into the Y-Z plane --> make x == 0
			const float Yang = atan2(N.x, N.z) * RAD_TO_DEG;
			Matrix4 Ry = Matrix4().rotateY(-Yang);
			N = Ry * N;

			// Rotate N into the X-Z plane --> make y == 0. N should now be equal to Z+
			const float Xang = atan2(N.y, N.z) * RAD_TO_DEG;
			Matrix4 Rx = Matrix4().rotateX(Xang);
			//N = Rx * N;
			//log_debug_vr(50 + contIdx * 25, FONT_WHITE_COLOR, "[%d]: %0.3f, %0.3f, %0.3f", contIdx, N.x, N.y, N.z);
			// The transform chain is now Rx * Ry: this will align the view vector going from the
			// origin to the intersection with Z+
			// Adding Rz to the chain makes the dot keep the up direction aligned with the camera.
			// This is what the brackets do right now.
			// Removing Rz keeps the up direction aligned with the reticle: this is probably
			// what we want to do if we want to replace the brackets/reticle/pips
			//Matrix4 Rz = Matrix4().rotateZ(g_pSharedDataCockpitLook->Roll);
			//V = Rz * Rx * Ry; // <-- Up direction is always view-aligned
			V = Rx * Ry; // <-- Up direction is reticle-aligned
			// The transpose is the inverse, so it will align Z+ with the view vector:
			V.transpose();
			V = swap * V * swap;
		}

		Matrix4 DotTransform;
		// This transform puts the dot near the center of the A-Wing dashboard:
		//DotTransform.translate(0, -30.0f, 20.0f);
		if (!bGunnerTurret && !bInHangar)
		{
			DotTransform = V;
			// This is the same as doing DotTransform = T * V:
			DotTransform.translate(g_LaserPointer3DIntersection[contIdx]);
		}
		else
		{
			Matrix4 swapScale({ 1,0,0,0,  0,0,-1,0,  0,-1,0,0,  0,0,0,1 });
			Matrix4 S    = Matrix4().scale(OPT_TO_METERS);
			Matrix4 Sinv = Matrix4().scale(METERS_TO_OPT);
			Matrix4 T    = Matrix4().translate(g_LaserPointerIntersSteamVR[contIdx]);
			Matrix4 toOPT     = Sinv * swap;
			Matrix4 toSteamVR = swap * S;
			// This transform chain is the same as the one used in RenderVRGloves minus gloveDisp and
			// pose is replaced with T. Also, V is used to convert the mesh into a billboard first
			DotTransform      = swapScale * toOPT * Vinv * T * toSteamVR * V;

			// This transform will put the dot in the center of the screen, no matter where we look.
			// but bInHangar must be forced to true:
			//DotTransform.identity();
			//T = Matrix4().translate(0, -15.0f, 0.0f);
			//DotTransform = swapScale * T;
		}
		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	_bDotsbRendered = true;
	RestoreContext();
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderVRBrackets()
{
	if (!g_bUseSteamVR || !g_bRendering3D || _bBracketsRendered)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderVRBrackets");

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;
	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rasterizer and depth stencil states for transparency
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	//_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);
	// _mainDepthState is D3D11_COMPARISON_ALWAYS, so the VR dots are always displayed
	_deviceResources->InitDepthStencilState(_deviceResources->_mainDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	ZeroMemory(&g_PSCBuffer, sizeof(g_PSCBuffer));
	g_PSCBuffer.bIsShadeless = 1;
	g_PSCBuffer.fPosNormalAlpha = 0.0f;

	g_VRGeometryCBuffer.numStickyRegions = 0;
	g_VRGeometryCBuffer.bRenderBracket = 1;
	// Disable region highlighting
	g_VRGeometryCBuffer.clicked[0] = false;
	g_VRGeometryCBuffer.clicked[1] = false;

	// Flags used in RenderScene():
	_bIsCockpit = !bGunnerTurret;
	_bIsGunner = bGunnerTurret;
	_bIsBlastMark = false;

	// Set the textures
	// Use this to render each bracket individually:
	_deviceResources->InitPSShaderResourceView(_vrGreenCirclesSRV.Get(), nullptr);
	// Use this to render 2D brackets on a big canvas:
	//_deviceResources->InitPSShaderResourceView(resources->_BracketsSRV.Get(), nullptr);

	// Set the mesh buffers
	ID3D11ShaderResourceView* vsSSRV[4] = { _vrDotMeshVerticesSRV.Get(), nullptr, _vrDotMeshTexCoordsSRV.Get(), nullptr };
	context->VSSetShaderResources(0, 4, vsSSRV);

	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
	_deviceResources->InitVertexBuffer(_vrDotVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(nullptr, true);
	_deviceResources->InitIndexBuffer(_vrDotIndexBuffer.Get(), true);

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	_deviceResources->InitPixelShader(resources->_pixelShaderVRGeom);

	const bool bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	Vector4 cUp, cBk, cDn, cFd;
	Matrix4 V, swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });
	Matrix4 swapScale({ 1,0,0,0,  0,0,-1,0,  0,-1,0,0,  0,0,0,1 });

	// Turns out we don't need the Cockpit Transform at all. We only need the headset's
	// transform for both the cockpit and external views.
	{
		Matrix4 ViewMatrix = g_VSMatrixCB.fullViewMat;
		ViewMatrix.invert();

		// Up vector, SteamVR coords. Compensated for HMD rotation
		cUp = ViewMatrix * Vector4(0, 1, 0, 0);
		cDn = -1.0f * cUp;
		// This produces a backwards vector in SteamVR coords:
		cBk = ViewMatrix * Vector4(0, 0, 1, 0);
		cFd = -1.0f * cBk;
	}
	//log_debug("[DBG] cUp: %0.3f, %0.3f, %0.3f", cUp.x, cUp.y, cUp.z);
	//log_debug("[DBG] cBk: %0.3f, %0.3f, %0.3f", cBk.x, cBk.y, cBk.z);

	// Let's replace transformWorldView with the identity matrix:
	Matrix4 Id;
	const float* m = Id.get();
	if (!bExternalCamera)
	{
		for (int i = 0; i < 16; i++) _CockpitConstants.transformWorldView[i] = m[i];
		context->UpdateSubresource(_constantBuffer, 0, nullptr, &_CockpitConstants, 0, 0);
	}
	else
	{
		for (int i = 0; i < 16; i++) _ExteriorConstants.transformWorldView[i] = m[i];
		context->UpdateSubresource(_constantBuffer, 0, nullptr, &_ExteriorConstants, 0, 0);
	}

	_trianglesCount = g_vrDotNumTriangles;
	// Get the width in OPT-scale of the mesh that will be rendered:
	// 0 -> 1
	// |    |
	// 3 -> 2
	const float meshWidth = g_vrDotMeshVertices[1].x - g_vrDotMeshVertices[0].x;
	for (uint32_t i = 0; i < g_bracketsVR.size(); i++)
	{
		const auto& bracketVR = g_bracketsVR[i];
		Vector4 dotPosSteamVR;
		dotPosSteamVR.x = bracketVR.posOPT.x * OPT_TO_METERS;
		dotPosSteamVR.y = bracketVR.posOPT.z * OPT_TO_METERS;
		dotPosSteamVR.z = bracketVR.posOPT.y * OPT_TO_METERS;
		dotPosSteamVR.w = 1.0f;

		const float meshScale = (bracketVR.halfWidthOPT * 2.0f) / meshWidth;
		g_VRGeometryCBuffer.strokeWidth = bracketVR.strokeWidth;
		g_VRGeometryCBuffer.bracketColor = bracketVR.color;

		{
			Vector4 P = dotPosSteamVR;

			// U points to the local Up direction, as defined by the current Cockpit transform
			// F points towards the bracket
			Vector3 F = { P.x, P.y, P.z }; F.normalize();
			// F . cFd can be 0 when the bracket is on the sides too. In that case, we want
			// to use kUp (i.e. have it become 1)
			float kUp; // = max(0.0f, F.dot(Vector4ToVector3(cFd)));
			float kBk = max(0.0f, F.dot(Vector4ToVector3(cUp)));
			float kFd = max(0.0f, F.dot(Vector4ToVector3(cDn)));

			// This fixes kUp when looking at brackets on the sides of the ship.
			kUp = 1.0f - (kBk + kFd);
			//log_debug_vr_set_row(5);
			//log_debug_vr("kUp: %0.3f, kBk: %0.3f, kFd: %0.3f", kUp, kBk, kFd);

			Vector3 U = kUp * Vector4ToVector3(cUp) + kBk * Vector4ToVector3(cBk) + kFd * Vector4ToVector3(cFd);
			U.normalize();

			Vector3 R = F.cross(U); R.normalize();
			// Re-compute the Up vector
			U = F.cross(R); U.normalize();
			float m[16] = { R.x, U.x, F.x, 0,
							R.y, U.y, F.y, 0,
							R.z, U.z, F.z, 0,
							0, 0, 0, 1 };
			V.set(m);
			// This transpose inverts the rotation:
			V.transpose();
			V = swap * V * swap;
		}

		Matrix4 DotTransform;
		DotTransform.identity();
		// This transform will put the dot in the center of the screen, no matter where we look.
		// but bInHangar must be forced to true:
		//T = Matrix4().translate(0, -15.0f, 0.0f);
		Vector3 posOPT = { dotPosSteamVR.x * METERS_TO_OPT,
						   dotPosSteamVR.z * METERS_TO_OPT,
						   dotPosSteamVR.y * METERS_TO_OPT };
		Matrix4 T = Matrix4().translate(posOPT);
		// Use this to render individual brackets and apply billboard correction:
		DotTransform = swapScale * T * Matrix4().scale(meshScale) * V;

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVRGeometryCBuffer(resources->_VRGeometryCBuffer.GetAddressOf(), &g_VRGeometryCBuffer);
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	_bBracketsRendered = true;
	RestoreContext();
	g_bracketsVR.clear();

	context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, 0,
		resources->_offscreenBufferBloomMask, 0, BLOOM_BUFFER_FORMAT);
	context->ResolveSubresource(resources->_offscreenBufferAsInputBloomMask, D3D11CalcSubresource(0, 1, 1),
		resources->_offscreenBufferBloomMask, D3D11CalcSubresource(0, 1, 1), BLOOM_BUFFER_FORMAT);

	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderVRHUD()
{
	if (!g_bUseSteamVR || !g_bRendering3D || _bHUDRendered || !_bCockpitConstantsCaptured)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderVRHUD");

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;
	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const bool bInHangar = *g_playerInHangar;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rasterizer and depth stencil states for transparency
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	//_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);
	// _mainDepthState is D3D11_COMPARISON_ALWAYS, so the VR dots are always displayed
	_deviceResources->InitDepthStencilState(_deviceResources->_mainDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	ZeroMemory(&g_PSCBuffer, sizeof(g_PSCBuffer));
	g_PSCBuffer.bIsShadeless = 1;
	g_PSCBuffer.fPosNormalAlpha = 0.0f;

	g_VRGeometryCBuffer.numStickyRegions = 0;
	// Disable region highlighting
	g_VRGeometryCBuffer.clicked[0] = false;
	g_VRGeometryCBuffer.clicked[1] = false;
	g_VRGeometryCBuffer.bRenderBracket = false;

	// Flags used in RenderScene():
	_bIsCockpit = !bGunnerTurret;
	_bIsGunner = bGunnerTurret;
	_bIsBlastMark = false;

	// Set the textures
	_deviceResources->InitPSShaderResourceView(_vrGreenCirclesSRV.Get(), nullptr);

	// Set the mesh buffers
	ID3D11ShaderResourceView* vsSSRV[4] = { _vrDotMeshVerticesSRV.Get(), nullptr, _vrDotMeshTexCoordsSRV.Get(), nullptr };
	context->VSSetShaderResources(0, 4, vsSSRV);

	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
	_deviceResources->InitVertexBuffer(_vrDotVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(nullptr, true);
	_deviceResources->InitIndexBuffer(_vrDotIndexBuffer.Get(), true);

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitVRGeometryCBuffer(resources->_VRGeometryCBuffer.GetAddressOf(), &g_VRGeometryCBuffer);
	_deviceResources->InitPixelShader(resources->_pixelShaderVRGeom);

	// Set the constants buffer
	Matrix4 Vinv;
	if (bGunnerTurret || bInHangar)
	{
		// For the Gunner Turret, we're going to remove the world-view transform and replace it
		// with an identity matrix. That way, the gloves, which are already in viewspace coords,
		// will follow the headset no matter how the turret is oriented.
		Matrix4 Id;
		const float* m = Id.get();
		for (int i = 0; i < 16; i++) _CockpitConstants.transformWorldView[i] = m[i];

		Vinv = g_VSMatrixCB.fullViewMat;
		Vinv.invert();
	}
	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_CockpitConstants, 0, 0);
	_trianglesCount = g_vrDotNumTriangles;

	Matrix4 V, swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });

	// X is 200m in front of us. This is the location of the reticle and the place where lasers
	// converge. Turns out it wasn't that hard to figure out after all...
	Vector3 X(0, -8192, 0);

	// Compute a new matrix for the dot by using the origin -> intersection point view vector.
	// First we'll align this vector with Z+ and then we'll use the inverse of this matrix to
	// rotate the dot so that it always faces the origin.
	{
		Vector4 P;
		if (!bGunnerTurret && !bInHangar)
		{
			// g_LaserPointerIntersSteamVR is not populated in this case, so we need to transform
			// g_LaserPointer3DIntersection, which is in OPT scale, into the SteamVR coord sys.
			const float cockpitOriginX = *g_POV_X;
			const float cockpitOriginY = *g_POV_Y;
			const float cockpitOriginZ = *g_POV_Z;
			Matrix4 T;

			T.translate(-cockpitOriginX - (g_pSharedDataCockpitLook->POVOffsetX * g_pSharedDataCockpitLook->povFactor),
				-cockpitOriginY + (g_pSharedDataCockpitLook->POVOffsetZ * g_pSharedDataCockpitLook->povFactor),
				-cockpitOriginZ - (g_pSharedDataCockpitLook->POVOffsetY * g_pSharedDataCockpitLook->povFactor));

			Matrix4 S = Matrix4().scale(OPT_TO_METERS);
			Matrix4 toSteamVR = swap * S;

			P = toSteamVR * T * Vector3ToVector4(X, 1.0f);
		}
		//else
		//	// Gunner turret... TODO
		//	P = Vector3ToVector4(g_LaserPointerIntersSteamVR[contIdx], 1);

		// O is the headset's center, in SteamVR coords:
		Vector4 O = g_VSMatrixCB.fullViewMat * Vector4(0, 0, 0, 1);
		// N goes from the intersection point to the headset's origin: it's the view vector now
		Vector4 N = P - O;
		//log_debug_vr(50 + contIdx * 25, FONT_WHITE_COLOR, "P[%d]: %0.3f, %0.3f, %0.3f", contIdx, P.x, P.y, P.z);

		N.normalize();
		// Rotate N into the Y-Z plane --> make x == 0
		const float Yang = atan2(N.x, N.z) * RAD_TO_DEG;
		Matrix4 Ry = Matrix4().rotateY(-Yang);
		N = Ry * N;

		// Rotate N into the X-Z plane --> make y == 0. N should now be equal to Z+
		const float Xang = atan2(N.y, N.z) * RAD_TO_DEG;
		Matrix4 Rx = Matrix4().rotateX(Xang);
		//N = Rx * N;
		//log_debug_vr(50 + contIdx * 25, FONT_WHITE_COLOR, "[%d]: %0.3f, %0.3f, %0.3f", contIdx, N.x, N.y, N.z);
		// The transform chain is now Rx * Ry: this will align the view vector going from the
		// origin to the intersection with Z+
		// Adding Rz to the chain makes the dot keep the up direction aligned with the camera.
		// This is what the brackets do right now.
		// Removing Rz keeps the up direction aligned with the reticle: this is probably
		// what we want to do if we want to replace the brackets/reticle/pips
		//Matrix4 Rz = Matrix4().rotateZ(g_pSharedDataCockpitLook->Roll);
		//V = Rz * Rx * Ry; // <-- Up direction is always view-aligned
		V = Rx * Ry; // <-- Up direction is reticle-aligned
		// The transpose is the inverse, so it will align Z+ with the view vector:
		V.transpose();
		V = Matrix4().scale(812.0f) * swap * V * swap;
	}

	Matrix4 DotTransform;
	if (!bGunnerTurret && !bInHangar)
	{
		DotTransform = V;
		// This is the same as doing DotTransform = T * V:
		DotTransform.translate(X);
	}
	//else
	//{
	//	Matrix4 swapScale({ 1,0,0,0,  0,0,-1,0,  0,-1,0,0,  0,0,0,1 });
	//	Matrix4 S = Matrix4().scale(OPT_TO_METERS);
	//	Matrix4 Sinv = Matrix4().scale(METERS_TO_OPT);
	//	Matrix4 T = Matrix4().translate(g_LaserPointerIntersSteamVR[contIdx]);
	//	Matrix4 toOPT = Sinv * swap;
	//	Matrix4 toSteamVR = swap * S;
	//	// This transform chain is the same as the one used in RenderVRGloves minus gloveDisp and
	//	// pose is replaced with T. Also, V is used to convert the mesh into a billboard first
	//	DotTransform = swapScale * toOPT * Vinv * T * toSteamVR * V;
	//}
	// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
	DotTransform.transpose();
	g_OPTMeshTransformCB.MeshTransform = DotTransform;

	// Apply the VS and PS constants
	resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);
	RenderScene(false);

	_bHUDRendered = true;
	RestoreContext();
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderVRSkyBox(bool debug)
{
	if (!g_bUseSteamVR || !g_bRendering3D)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderVRSkyBox");

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;
	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rasterizer and depth stencil states for transparency
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	//_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);
	// _mainDepthState is COMPARE_ALWAYS, so the VR dots are always displayed
	_deviceResources->InitDepthStencilState(_deviceResources->_mainDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	ZeroMemory(&g_PSCBuffer, sizeof(g_PSCBuffer));
	g_PSCBuffer.bIsShadeless = 2;
	g_PSCBuffer.fPosNormalAlpha = 0.0f;

	g_VRGeometryCBuffer.numStickyRegions = 0;
	// Disable region highlighting
	g_VRGeometryCBuffer.clicked[0] = false;
	g_VRGeometryCBuffer.clicked[1] = false;

	// Flags used in RenderScene():
	_bIsCockpit = !bGunnerTurret;
	_bIsGunner = bGunnerTurret;
	_bIsBlastMark = false;

	// Set the mesh buffers
	ID3D11ShaderResourceView* vsSSRV[4] = { _vrDotMeshVerticesSRV.Get(), nullptr, _vrDotMeshTexCoordsSRV.Get(), nullptr };
	context->VSSetShaderResources(0, 4, vsSSRV);

	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
	_deviceResources->InitVertexBuffer(_vrDotVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(nullptr, true);
	_deviceResources->InitIndexBuffer(_vrDotIndexBuffer.Get(), true);

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitVRGeometryCBuffer(resources->_VRGeometryCBuffer.GetAddressOf(), &g_VRGeometryCBuffer);
	_deviceResources->InitPixelShader(resources->_pixelShaderVRGeom);

	const bool bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	// Let's replace transformWorldView with the identity matrix:
	Matrix4 Id;
	const float* m = Id.get();
	for (int i = 0; i < 16; i++) _frameConstants.transformWorldView[i] = m[i];
	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_frameConstants, 0, 0);
	_trianglesCount = g_vrDotNumTriangles;

	// Get the width in OPT-scale of the mesh that will be rendered:
	// 0 -> 1
	// |
	// 3
	const float meshWidth = g_vrDotMeshVertices[1].x - g_vrDotMeshVertices[0].x;
	//log_debug_vr("meshWidth: %0.3f", meshWidth);

	ID3D11ShaderResourceView* srvs[] = {
		resources->_textureCubeSRV /* .Get() */, // 21
	};
	context->PSSetShaderResources(21, 1, srvs);

	if (!debug)
	{
		_overrideRTV = BACKGROUND_LYR;
		float black[4] = { 0, 0, 0, 1 };
		context->ClearRenderTargetView(resources->_backgroundRTV, black);
	}

	for (const auto& bracketVR : g_bracketsVR)
	{
		Vector4 dotPosSteamVR;
		dotPosSteamVR.x = bracketVR.posOPT.x * OPT_TO_METERS;
		dotPosSteamVR.y = bracketVR.posOPT.z * OPT_TO_METERS;
		dotPosSteamVR.z = bracketVR.posOPT.y * OPT_TO_METERS;
		dotPosSteamVR.w = 1.0f;
		const float meshScale = (bracketVR.halfWidthOPT * 2.0f) / meshWidth;

		Matrix4 DotTransform;
		{
			Matrix4 swapScale({ 1,0,0,0,  0,0,-1,0,  0,-1,0,0,  0,0,0,1 });

			DotTransform.identity();
			Vector3 posOPT = { dotPosSteamVR.x * METERS_TO_OPT,
							   dotPosSteamVR.z * METERS_TO_OPT,
							   dotPosSteamVR.y * METERS_TO_OPT };
			Matrix4 T = Matrix4().translate(posOPT);
			DotTransform = swapScale * T * Matrix4().scale(meshScale);
		}
		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	RestoreContext();
	_overrideRTV = TRANSP_LYR_NONE;
	g_bracketsVR.clear();
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderSkyCylinder()
{
	if (!g_bRendering3D || g_HyperspacePhaseFSM != HS_INIT_ST)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderSkyCylinder");

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;
	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rasterizer and depth stencil states for transparency
	//_deviceResources->InitBlendState(_solidBlendState, nullptr);
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	//_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);
	//_deviceResources->InitDepthStencilState(_solidDepthState, nullptr);
	// _mainDepthState is D3D11_COMPARISON_ALWAYS, so the VR dots are always displayed
	_deviceResources->InitDepthStencilState(_deviceResources->_mainDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	ZeroMemory(&g_PSCBuffer, sizeof(g_PSCBuffer));
	g_PSCBuffer.bIsShadeless = 2;
	g_PSCBuffer.fPosNormalAlpha = 0.0f;

	g_VRGeometryCBuffer.numStickyRegions = 0;
	// Disable region highlighting
	g_VRGeometryCBuffer.clicked[0] = false;
	g_VRGeometryCBuffer.clicked[1] = false;

	// Flags used in RenderScene():
	_bIsCockpit = !bGunnerTurret;
	_bIsGunner = bGunnerTurret;
	_bIsBlastMark = false;

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &_frameVSCBuffer);
	resources->InitVRGeometryCBuffer(resources->_VRGeometryCBuffer.GetAddressOf(), &g_VRGeometryCBuffer);
	_deviceResources->InitPixelShader(resources->_pixelShaderVRGeom);

	_overrideRTV = BACKGROUND_LYR;
	float black[4] = { 0, 0, 0, 1 };
	context->ClearRenderTargetView(resources->_backgroundRTV, black);
	Matrix4 swapScale({ 1,0,0,0,  0,0,-1,0,  0,-1,0,0,  0,0,0,1 });

	const bool bExternalCamera = PlayerDataTable[*g_playerIndex].Camera.ExternalCamera;
	// Let's replace transformWorldView with the identity matrix:
	Matrix4 Id;
	const float* m = Id.get();
	for (int i = 0; i < 16; i++) _frameConstants.transformWorldView[i] = m[i];
	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_frameConstants, 0, 0);

	ID3D11ShaderResourceView* vsSSRV[4] = { nullptr, nullptr, nullptr, nullptr };

	// ***************************************************
	// Caps
	// ***************************************************
	// Set the mesh buffers
	vsSSRV[0] = _bgCapMeshVerticesSRV.Get();
	vsSSRV[2] = _bgCapMeshTexCoordsSRV.Get();
	context->VSSetShaderResources(0, 4, vsSSRV);
	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(_bgCapVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(_bgCapIndexBuffer.Get(), true);
	_trianglesCount = 2;

	// Top cap (Z+):
	{
		ID3D11ShaderResourceView* srvs[] = {
			g_StarfieldSRVs[STARFIELD_TYPE::TOP] != nullptr ?
				g_StarfieldSRVs[STARFIELD_TYPE::TOP]->_textureView.Get() : nullptr
		};
		context->PSSetShaderResources(0, 1, srvs);

		// The caps are created at the origin, so we need to translate them to the poles:
		Matrix4 T = Matrix4().translate(0, 0, BACKGROUND_CYL_RATIO * BACKGROUND_CUBE_HALFSIZE_METERS * METERS_TO_OPT);
		Matrix4 DotTransform = swapScale * g_VRGeometryCBuffer.viewMat * T;

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	// Bottom cap (Z-):
	{
		ID3D11ShaderResourceView* srvs[] = {
			g_StarfieldSRVs[STARFIELD_TYPE::BOTTOM] != nullptr ?
			g_StarfieldSRVs[STARFIELD_TYPE::BOTTOM]->_textureView.Get() : nullptr
		};
		context->PSSetShaderResources(0, 1, srvs);

		// The caps are created at the origin, so we need to translate them to the poles:
		Matrix4 T = Matrix4().translate(0, 0, -BACKGROUND_CYL_RATIO * BACKGROUND_CUBE_HALFSIZE_METERS * METERS_TO_OPT);
		Matrix4 DotTransform = swapScale * g_VRGeometryCBuffer.viewMat * T;

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	// ***************************************************
	// Sides
	// ***************************************************
#ifdef DISABLED
	_trianglesCount = 2;
	// Set the mesh buffers
	vsSSRV[0] = _bgSideMeshVerticesSRV.Get();
	vsSSRV[2] = _bgSideMeshTexCoordsSRV.Get();
	context->VSSetShaderResources(0, 4, vsSSRV);
	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(_bgSideVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(_bgSideIndexBuffer.Get(), true);

	// Front (Y-):
	{
		Matrix4 T = Matrix4().translate(0, -BACKGROUND_CUBE_HALFSIZE_METERS * METERS_TO_OPT, 0);
		Matrix4 DotTransform = swapScale * g_VRGeometryCBuffer.viewMat * T;

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}
#endif

	_trianglesCount = s_numCylTriangles;
	// Set the mesh buffers
	vsSSRV[0] = _bgCylMeshVerticesSRV.Get();
	vsSSRV[2] = _bgCylMeshTexCoordsSRV.Get();
	context->VSSetShaderResources(0, 4, vsSSRV);
	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(_bgCylVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(_bgCylIndexBuffer.Get(), true);
	_trianglesCount = s_numCylTriangles;

	// Front (Y-):
	{
		ID3D11ShaderResourceView* srvs[] = {
			g_StarfieldSRVs[STARFIELD_TYPE::FRONT] != nullptr ?
			g_StarfieldSRVs[STARFIELD_TYPE::FRONT]->_textureView.Get() : nullptr
		};
		context->PSSetShaderResources(0, 1, srvs);
		Matrix4 DotTransform = swapScale * g_VRGeometryCBuffer.viewMat;

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	// Left: (X-):
	{
		ID3D11ShaderResourceView* srvs[] = {
			g_StarfieldSRVs[STARFIELD_TYPE::LEFT] != nullptr ?
			g_StarfieldSRVs[STARFIELD_TYPE::LEFT]->_textureView.Get() : nullptr
		};
		context->PSSetShaderResources(0, 1, srvs);
		Matrix4 DotTransform = swapScale * g_VRGeometryCBuffer.viewMat * Matrix4().rotateZ(90.0f);

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	// Back: (Y+):
	{
		ID3D11ShaderResourceView* srvs[] = {
			g_StarfieldSRVs[STARFIELD_TYPE::BACK] != nullptr ?
			g_StarfieldSRVs[STARFIELD_TYPE::BACK]->_textureView.Get() : nullptr
		};
		context->PSSetShaderResources(0, 1, srvs);
		Matrix4 DotTransform = swapScale * g_VRGeometryCBuffer.viewMat * Matrix4().rotateZ(180.0f);

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	// Right: (X+):
	{
		ID3D11ShaderResourceView* srvs[] = {
			g_StarfieldSRVs[STARFIELD_TYPE::RIGHT] != nullptr ?
			g_StarfieldSRVs[STARFIELD_TYPE::RIGHT]->_textureView.Get() : nullptr
		};
		context->PSSetShaderResources(0, 1, srvs);
		Matrix4 DotTransform = swapScale * g_VRGeometryCBuffer.viewMat * Matrix4().rotateZ(270.0f);

		// The Vertex Shader does post-multiplication, so we need to transpose the matrix:
		DotTransform.transpose();
		g_OPTMeshTransformCB.MeshTransform = DotTransform;

		// Apply the VS and PS constants
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		RenderScene(false);
	}

	RestoreContext();
	_overrideRTV = TRANSP_LYR_NONE;
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderVRKeyboard()
{
	if (!g_bUseSteamVR || !g_bActiveCockpitEnabled || !g_bRendering3D)
		return;

	// g_vrKeybState.bRendered is set to false on SceneBegin() -- at the beginning of each frame
	if (g_vrKeybState.bRendered || g_vrKeybState.state == KBState::OFF || !_bCockpitConstantsCaptured)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderVRKeyboard");

	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const bool bInHangar = *g_playerInHangar;

	// Update the position of the keyboard
	{
		const float cockpitOriginX = *g_POV_X;
		const float cockpitOriginY = *g_POV_Y;
		const float cockpitOriginZ = *g_POV_Z;

		static Matrix4 swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });

		Matrix4 R, S;
		S.scale(OPT_TO_METERS);
		// The InitialTransform below ensures that the keyboard is always vertical when displayed, however,
		// we'd still like to tilt it a little bit to make it easier to type things
		R.rotateX(25.0f);

		Matrix4 Tinv, Sinv;
		if (!bGunnerTurret && !bInHangar)
			Tinv.translate(cockpitOriginX + (g_pSharedDataCockpitLook->POVOffsetX * g_pSharedDataCockpitLook->povFactor),
			               cockpitOriginY - (g_pSharedDataCockpitLook->POVOffsetZ * g_pSharedDataCockpitLook->povFactor),
			               cockpitOriginZ + (g_pSharedDataCockpitLook->POVOffsetY * g_pSharedDataCockpitLook->povFactor));
		else
			Tinv.identity();
		Sinv.scale(METERS_TO_OPT);

		Matrix4 swapScale({ 1,0,0,0,  0,0,-1,0,  0,-1,0,0,  0,0,0,1 });
		Matrix4 Vinv;
		if (bGunnerTurret || bInHangar)
		{
			// fullViewMat is available here, near the end of the frame; but it's not available in SceneBegin()
			Vinv = g_VSMatrixCB.fullViewMat;
			Vinv.invert();
		}

		Matrix4 toSteamVR = swap * S * R;
		Matrix4 toOPT     = Tinv * Sinv * swap;

		static Matrix4 H; // Hover pose matrix
		// Only update the position while the keyboard is hovering
		if (g_vrKeybState.state == KBState::HOVER && g_contStates[g_vrKeybState.iHoverContIdx].bIsValid)
		{
			// The hover matrix is only updated when the VR keyb is in the HOVER state
			H = g_contStates[g_vrKeybState.iHoverContIdx].pose;

			// This is the origin of the controller, in SteamVR coords:
			//const float* m0 = g_contStates[thrIdx].pose.get();
			//Vector4 P = Vector4(m0[12], m0[13], m0[14], 1.0f);

			// g_vrKeybState.state is HOVER because of the if above. So this is the transition
			// where the keyboard is initially displayed.
			if (g_vrKeybState.prevState != KBState::HOVER)
			{
				g_vrKeybState.InitialTransform = g_contStates[g_vrKeybState.iHoverContIdx].pose;
				const float* m0 = g_vrKeybState.InitialTransform.get();
				float m[16];
				for (int i = 0; i < 16; i++) m[i] = m0[i];
				// Erase the translation
				m[12] = m[13] = m[14] = 0;
				g_vrKeybState.InitialTransform.set(m);
				// Invert the rotation
				g_vrKeybState.InitialTransform.invert();
			}
		}

		if (!bGunnerTurret && !bInHangar)
		{
			// Convert OPT to SteamVR coords, then invert the initial pose so that the keyboard is
			// always shown upright, then apply the current VR controller pose, and finally move
			// everything back to cockpit (viewpsace) coords.
			g_vrKeybState.Transform = toOPT * H * g_vrKeybState.InitialTransform * toSteamVR;
		}
		else
		{
			// Convert OPT to SteamVR coords, then invert the initial pose so that the keyboard is
			// always shown upright, then apply the current VR controller pose, invert the headset view
			// matrix, convert everything back to viewspace coords and swap the axes since we're not
			// using the regular world-view transform matrix (it's the identity matrix, see below).
			// We do all this so that the VR keyboard is displayed in viewspace coords in the gunner turret.
			g_vrKeybState.Transform = swapScale * toOPT * Vinv * H * g_vrKeybState.InitialTransform * toSteamVR;
			g_vrKeybState.OPTTransform = toOPT * H * g_vrKeybState.InitialTransform * toSteamVR;
		}
		// XwaD3dVertexShader does a post-multiplication, so we need to transpose this:
		g_vrKeybState.Transform.transpose();
		g_vrKeybState.OPTTransform.transpose();
		g_vrKeybState.prevState = g_vrKeybState.state;
	}

	// I think prevState is redundant, we should be able to use g_vrKeybState.prevState instead
	static KBState prevState = KBState::OFF;
	static float fadeIn, fadeInIncr;
	if (prevState != KBState::HOVER && g_vrKeybState.state == KBState::HOVER)
	{
		fadeIn = 1.0f;
		fadeInIncr = 2.0f;
	}
	else if (prevState == KBState::STATIC && g_vrKeybState.state != KBState::STATIC)
	{
		fadeIn = 2.0f;
		fadeInIncr = -2.0f;
	}

	fadeIn += fadeInIncr * g_HiResTimer.elapsed_s;
	if (fadeIn > 2.0f) {
		fadeIn = 0.0f;
		fadeInIncr = 0.0f;
	}
	else if (g_vrKeybState.state == KBState::CLOSING && fadeIn < 1.0f) {
		fadeIn = 0.0f;
		fadeInIncr = 0.0f;
		g_vrKeybState.state = KBState::OFF;
	}
	prevState = g_vrKeybState.state;

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rasterizer and depth stencil states for transparency
	_deviceResources->InitBlendState(_transparentBlendState, nullptr);
	//_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);
	//_deviceResources->InitBlendState(_solidBlendState, nullptr);
	_deviceResources->InitDepthStencilState(_solidDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	ZeroMemory(&g_PSCBuffer, sizeof(g_PSCBuffer));
	g_PSCBuffer.bIsShadeless = 1;
	g_PSCBuffer.fPosNormalAlpha = 0.0f;
	g_PSCBuffer.rand0 = fadeIn;
	// fSSAOAlphaMult ?
	// fSSAOMaskVal ?
	// fPosNormalAlpha ?
	// fBloomStrength ?
	// bInHyperspace ?

	// Highlight the sticky keys
	for (int i = 0; i < g_vrKeybState.iNumStickyRegions; i++)
	{
		g_VRGeometryCBuffer.stickyRegions[i] = g_vrKeybState.stickyRegions[i];
	}
	g_VRGeometryCBuffer.numStickyRegions = g_vrKeybState.iNumStickyRegions;

	// Highlight regular clicks
	for (int i = 0; i < 2; i++)
	{
		g_VRGeometryCBuffer.clicked[i] = g_LaserPointerBuffer.TriggerState[i];
		g_VRGeometryCBuffer.clickRegions[i] = g_vrKeybState.clickRegions[i];
	}
	g_VRGeometryCBuffer.bRenderBracket = false;

	// Flags used in RenderScene():
	_bIsCockpit = !bGunnerTurret;
	_bIsGunner = bGunnerTurret;
	_bIsBlastMark = false;

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitVRGeometryCBuffer(resources->_VRGeometryCBuffer.GetAddressOf(), &g_VRGeometryCBuffer);
	g_OPTMeshTransformCB.MeshTransform = g_vrKeybState.Transform;
	resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

	// Set the textures
	_deviceResources->InitPSShaderResourceView(_vrKeybTextureSRV.Get(), nullptr);

	// Set the mesh buffers
	ID3D11ShaderResourceView* vsSSRV[4] = { _vrKeybMeshVerticesSRV.Get(), nullptr, _vrKeybMeshTexCoordsSRV.Get(), nullptr};
	context->VSSetShaderResources(0, 4, vsSSRV);

	// Set the index and vertex buffers
	_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
	_deviceResources->InitVertexBuffer(_vrKeybVertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(nullptr, true);
	_deviceResources->InitIndexBuffer(_vrKeybIndexBuffer.Get(), true);

	// Set the constants buffer
	if (bGunnerTurret || bInHangar)
	{
		// For the Gunner Turret, we're going to remove the world-view transform and replace it
		// with an identity matrix. That way, the gloves, which are already in viewspace coords,
		// will follow the headset no matter how the turret is oriented.
		Matrix4 Id;
		const float* m = Id.get();
		for (int i = 0; i < 16; i++) _CockpitConstants.transformWorldView[i] = m[i];
	}
	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_CockpitConstants, 0, 0);
	_trianglesCount = g_vrKeybNumTriangles;
	_deviceResources->InitPixelShader(resources->_pixelShaderVRGeom);

	// Render the VR keyboard
	RenderScene(false);

	// Decrease the refcount of the textures
	/*for (int i = 0; i < 2; i++)
		if (_vrKeybCommand.SRVs[i] != nullptr) _vrKeybCommand.SRVs[i]->Release();*/

	// Restore the previous state
	g_vrKeybState.bRendered = true;
	RestoreContext();
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderVRGloves()
{
	if (!g_bUseSteamVR || !g_bRendering3D || !g_bActiveCockpitEnabled || !_bCockpitConstantsCaptured)
		return;

	_deviceResources->BeginAnnotatedEvent(L"RenderVRGloves");

	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	// Set the proper rasterizer and depth stencil states for transparency
	_deviceResources->InitBlendState(_solidBlendState, nullptr);
	_deviceResources->InitDepthStencilState(_solidDepthState, nullptr);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	ZeroMemory(&g_PSCBuffer, sizeof(g_PSCBuffer));
	// fSSAOAlphaMult ?
	// fSSAOMaskVal ?
	// fPosNormalAlpha ?
	// fBloomStrength ?
	// bInHyperspace ?

	g_VRGeometryCBuffer.numStickyRegions = 0;
	// Disable region highlighting on the gloves for now...
	g_VRGeometryCBuffer.clicked[0] = false;
	g_VRGeometryCBuffer.clicked[1] = false;
	g_VRGeometryCBuffer.bRenderBracket = false;

	const bool bGunnerTurret = (g_iPresentCounter > PLAYERDATATABLE_MIN_SAFE_FRAME) ?
		PlayerDataTable[*g_playerIndex].gunnerTurretActive : false;
	const bool bInHangar = *g_playerInHangar;

	// Flags used in RenderScene():
	_bIsCockpit = !bGunnerTurret;
	_bIsGunner = bGunnerTurret;
	_bIsBlastMark = false;

	const float cockpitOriginX = *g_POV_X;
	const float cockpitOriginY = *g_POV_Y;
	const float cockpitOriginZ = *g_POV_Z;

	Matrix4 swap({ 1,0,0,0,  0,0,1,0,  0,1,0,0,  0,0,0,1 });
	Matrix4 S, T, Sinv, Tinv;
	S.scale(OPT_TO_METERS);
	if (!bGunnerTurret && !bInHangar)
		Tinv.translate(cockpitOriginX + (g_pSharedDataCockpitLook->POVOffsetX * g_pSharedDataCockpitLook->povFactor),
		               cockpitOriginY - (g_pSharedDataCockpitLook->POVOffsetZ * g_pSharedDataCockpitLook->povFactor),
		               cockpitOriginZ + (g_pSharedDataCockpitLook->POVOffsetY * g_pSharedDataCockpitLook->povFactor));
	else
		Tinv.identity();
	Sinv.scale(METERS_TO_OPT);

	Matrix4 swapScale({ 1,0,0,0,  0,0,-1,0,  0,-1,0,0,  0,0,0,1 });
	Matrix4 Vinv;
	if (bGunnerTurret || bInHangar)
	{
		Vinv = g_VSMatrixCB.fullViewMat;
		Vinv.invert();
	}

	Matrix4 toSteamVR = swap * S;
	Matrix4 toOPT     = Tinv * Sinv * swap;

	// Apply the VS and PS constants
	resources->InitPSConstantBuffer3D(resources->_PSConstantBuffer.GetAddressOf(), &g_PSCBuffer);
	resources->InitVRGeometryCBuffer(resources->_VRGeometryCBuffer.GetAddressOf(), &g_VRGeometryCBuffer);
	_deviceResources->InitPixelShader(resources->_pixelShaderVRGeom);

	Vector4 contOrigin[2];
	Vector4 contDir[2];
	VRControllerToOPTCoords(contOrigin, contDir);

	// Render both gloves (if they are enabled)
	for (int i = 0; i < 2; i++)
	{
		// g_vrGlovesMeshes.rendered is set to false on SceneBegin() -- at the beginning of each frame
		if (g_vrGlovesMeshes[i].numTriangles <= 0 || !g_vrGlovesMeshes[i].visible || g_vrGlovesMeshes[i].rendered || !g_contStates[i].bIsValid)
			continue;

		// DEBUG: This translation puts an OBJ centered at the origin on top of the AwingCockpit dashboard
		/*
		Matrix4 T;
		T.identity();
		T.translate(0.0f, -20.0f, 20.0f);
		T.transpose();
		g_vrGlovesMeshes[i].pose = T;
		*/

		int profile = VRGlovesProfile::NEUTRAL;
		if (g_contStates[i].buttons[VRButtons::GRIP])
			profile = VRGlovesProfile::GRASP;
		if (g_fLaserIntersectionDistance[i] < GLOVE_NEAR_THRESHOLD_METERS * METERS_TO_OPT &&
			g_bPrevHoveringOnActiveElem[i])
			profile = VRGlovesProfile::POINT;

		// Translate the glove so that it clicks on objects when the trigger button is pressed.
		Matrix4 gloveDisp;
		if (profile == VRGlovesProfile::POINT && g_contStates[i].buttons[VRButtons::TRIGGER])
		{
			Vector4 I;

			if (bGunnerTurret || bInHangar)
			{
				// The following transform chain is inspired by the VR glove transform chain, which is:
				// g_vrGlovesMeshes[i].pose = swapScale * gloveDisp * toOPT * Vinv * g_contStates[i].pose * toSteamVR;
				// But in this case, swapScale is not needed, gloveDisp does not exist, and we're starting
				// from SteamVR coords, so the chain is simpler:
				Matrix4 toGunnerOPT = toOPT * Vinv;

				// Recompute contOrigin and I:
				g_contOriginWorldSpace[i].w = 1.0f;
				contOrigin[i] = toGunnerOPT * g_contOriginWorldSpace[i];
				I = toGunnerOPT * Vector3ToVector4(g_LaserPointerIntersSteamVR[i], 1.0f);
			}
			else
			{
				I = Vector4(g_LaserPointer3DIntersection[i]);
			}

			// Just move the finger (i.e. the origin) to the intersection point. That should work
			// regardless of the direction the controller is facing.
			// Coordinates here are OPT-Viewspace.
			Vector4 dir = I - contOrigin[i];
			// Only displace the glove if it's close enough to the target:
			if (dir.length() < GLOVE_NEAR_THRESHOLD_METERS * METERS_TO_OPT)
				gloveDisp.translate(dir.x, dir.y, dir.z);
		}

		// The gloves are in the OPT-viewspace coord sys. In order to apply the SteamVR
		// pose, we need to transform them to the SteamVR coord sys, then we apply the
		// SteamVR pose, and then we move it back to the OPT-viewspace system.
		if (!_bIsGunner && !bInHangar)
		{
			g_vrGlovesMeshes[i].pose = gloveDisp * toOPT * g_contStates[i].pose * toSteamVR;
		}
		else
		{
			// Vinv inverts the headset's rotation
			// swapScale takes the place of the world-view transform matrix (we'll replace it with
			// and identity matrix, see below)
			g_vrGlovesMeshes[i].pose = swapScale * gloveDisp * toOPT * Vinv * g_contStates[i].pose * toSteamVR;
		}
		g_vrGlovesMeshes[i].pose.transpose();

		g_OPTMeshTransformCB.MeshTransform = g_vrGlovesMeshes[i].pose;
		resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

		// Set the textures
		_deviceResources->InitPSShaderResourceView(g_vrGlovesMeshes[i].textureSRV.Get(), nullptr);

		// Set the mesh buffers
		ID3D11ShaderResourceView* vsSSRV[4] = {
			g_vrGlovesMeshes[i].meshVerticesSRVs[profile].Get(),
			g_vrGlovesMeshes[i].meshNormalsSRV.Get(),
			g_vrGlovesMeshes[i].meshTexCoordsSRV.Get(),
			nullptr };
		context->VSSetShaderResources(0, 4, vsSSRV);

		// Set the index and vertex buffers
		_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
		_deviceResources->InitVertexBuffer(g_vrGlovesMeshes[i].vertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
		_deviceResources->InitIndexBuffer(nullptr, true);
		_deviceResources->InitIndexBuffer(g_vrGlovesMeshes[i].indexBuffer.Get(), true);

		// Set the constants buffer
		if (bGunnerTurret || bInHangar)
		{
			// For the Gunner Turret, we're going to remove the world-view transform and replace it
			// with an identity matrix. That way, the gloves, which are already in viewspace coords,
			// will follow the headset no matter how the turret is oriented.
			Matrix4 Id;
			const float* m = Id.get();
			for (int i = 0; i < 16; i++) _CockpitConstants.transformWorldView[i] = m[i];
		}
		context->UpdateSubresource(_constantBuffer, 0, nullptr, &_CockpitConstants, 0, 0);
		_trianglesCount = g_vrGlovesMeshes[i].numTriangles;

		// Render the gloves
		RenderScene(false);

		// Decrease the refcount of the textures
		/*for (int i = 0; i < 2; i++)
			if (_vrKeybCommand.SRVs[i] != nullptr) _vrKeybCommand.SRVs[i]->Release();*/
		g_vrGlovesMeshes[i].rendered = true;
	}

	// Restore the previous state
	RestoreContext();
	_deviceResources->EndAnnotatedEvent();
}

/*
 * Using the current 3D box limits loaded in g_OBJLimits, compute the 2D/Z-Depth limits
 * needed to center the Shadow Map depth buffer. This version uses the transforms in
 * g_CockpitConstants
 */
Matrix4 EffectsRenderer::GetShadowMapLimits(Matrix4 L, float *OBJrange, float *OBJminZ) {
	float minx = 100000.0f, maxx = -100000.0f;
	float miny = 100000.0f, maxy = -100000.0f;
	float minz = 100000.0f, maxz = -100000.0f;
	float cx, cy, sx, sy;
	Matrix4 S, T;
	Vector4 P, Q;
	Matrix4 WorldView = XwaTransformToMatrix4(_CockpitWorldView);
	FILE *file = NULL;

	if (g_bDumpSSAOBuffers) {
		fopen_s(&file, "./ShadowMapLimits.OBJ", "wt");

		XwaTransform M = _CockpitWorldView;
		log_debug("[DBG] -----------------------------------------------");
		log_debug("[DBG] GetShadowMapLimits WorldViewTransform:");
		log_debug("[DBG] %0.3f, %0.3f, %0.3f, %0.3f",
			M.Rotation._11, M.Rotation._12, M.Rotation._13, 0.0f);
		log_debug("[DBG] %0.3f, %0.3f, %0.3f, %0.3f",
			M.Rotation._21, M.Rotation._22, M.Rotation._23, 0.0f);
		log_debug("[DBG] %0.3f, %0.3f, %0.3f, %0.3f",
			M.Rotation._31, M.Rotation._32, M.Rotation._33, 0.0f);
		log_debug("[DBG] %0.3f, %0.3f, %0.3f, %0.3f",
			M.Position.x, M.Position.y, M.Position.z, 1.0f);
		log_debug("[DBG] -----------------------------------------------");
	}

	for (Vector4 X : g_OBJLimits) {
		// This transform chain should be the same we apply in ShadowMapVS.hlsl

		// OPT to camera view transform. First transform object space into view space:
		Q = WorldView * X;
		// Now, transform OPT coords to meters:
		Q *= OPT_TO_METERS;
		// Invert the Y-axis since our coordinate system has Y+ pointing up
		Q.y = -Q.y;
		// Just make sure w = 1
		Q.w = 1.0f;

		// The point is now in metric 3D, with the POV at the origin.
		// Apply the light transform, keep the points in metric 3D.
		P = L * Q;

		// Update the limits
		if (P.x < minx) minx = P.x;
		if (P.y < miny) miny = P.y;
		if (P.z < minz) minz = P.z;

		if (P.x > maxx) maxx = P.x;
		if (P.y > maxy) maxy = P.y;
		if (P.z > maxz) maxz = P.z;

		if (g_bDumpSSAOBuffers)
			fprintf(file, "v %0.6f %0.6f %0.6f\n", P.x, P.y, P.z);
	}

	if (g_bDumpSSAOBuffers) {
		fprintf(file, "\n");
		fprintf(file, "f 1 2 3\n");
		fprintf(file, "f 1 3 4\n");

		fprintf(file, "f 5 6 7\n");
		fprintf(file, "f 5 7 8\n");

		fprintf(file, "f 1 5 6\n");
		fprintf(file, "f 1 6 2\n");

		fprintf(file, "f 4 8 7\n");
		fprintf(file, "f 4 7 3\n");
		fflush(file);
		fclose(file);
	}

	// Compute the centroid
	cx = (minx + maxx) / 2.0f;
	cy = (miny + maxy) / 2.0f;

	// Compute the scale
	sx = 1.95f / (maxx - minx); // Map to -0.975..0.975
	sy = 1.95f / (maxy - miny); // Map to -0.975..0.975
	// Having an anisotropic scale provides a better usage of the shadow map. However
	// it also distorts the shadow map, making it harder to debug.
	// release
	float s = min(sx, sy);
	//sz = 1.8f / (maxz - minz); // Map to -0.9..0.9
	//sz = 1.0f / (maxz - minz);

	// We want to map xy to the origin; but we want to map Z to 0..0.98, so that Z = 1.0 is at infinity
	// Translate the points so that the centroid is at the origin
	T.translate(-cx, -cy, 0.0f);
	// Scale around the origin so that the xyz limits are [-0.9..0.9]
	if (g_ShadowMapping.bAnisotropicMapScale)
		S.scale(sx, sy, 1.0f); // Anisotropic scale: better use of the shadow map
	else
		S.scale(s, s, 1.0f); // Isotropic scale: better for debugging.

	*OBJminZ = minz;
	*OBJrange = maxz - minz;

	if (g_bDumpSSAOBuffers) {
		log_debug("[DBG] [SHW] min-x,y,z: %0.3f, %0.3f, %0.3f, max-x,y,z: %0.3f, %0.3f, %0.3f",
			minx, miny, minz, maxx, maxy, maxz);
		log_debug("[DBG] [SHW] cx,cy: %0.3f, %0.3f, sx,sy,s: %0.3f, %0.3f, %0.3f",
			cx, cy, sx, sy, s);
		log_debug("[DBG] [SHW] maxz: %0.3f, OBJminZ: %0.3f, OBJrange: %0.3f",
			maxz, *OBJminZ, *OBJrange);
		log_debug("[DBG] [SHW] sm_z_factor: %0.6f, FOVDistScale: %0.3f",
			g_ShadowMapVSCBuffer.sm_z_factor, g_ShadowMapping.FOVDistScale);
	}
	return S * T;
}

/*
 * Using the given 3D box Limits, compute the 2D/Z-Depth limits needed to center the Shadow Map depth
 * buffer. This version expects the AABB to be in the lightview coordinate system
 */
Matrix4 EffectsRenderer::GetShadowMapLimits(const AABB &aabb, float *range, float *minZ) {
	float cx, cy, sx, sy;
	Matrix4 S, T;
	Vector4 P, Q;

	// Compute the centroid
	cx = (aabb.min.x + aabb.max.x) / 2.0f;
	cy = (aabb.min.y + aabb.max.y) / 2.0f;

	// Compute the scale
	sx = 1.95f / (aabb.max.x - aabb.min.x); // Map to -0.975..0.975
	sy = 1.95f / (aabb.max.y - aabb.min.y); // Map to -0.975..0.975
	// Having an anisotropic scale provides a better usage of the shadow map. However
	// it also distorts the shadow map, making it harder to debug.
	// release
	float s = min(sx, sy);
	//sz = 1.8f / (maxz - minz); // Map to -0.9..0.9
	//sz = 1.0f / (maxz - minz);

	// We want to map xy to the origin; but we want to map Z to 0..0.98, so that Z = 1.0 is at infinity
	// Translate the points so that the centroid is at the origin
	T.translate(-cx, -cy, 0.0f);
	// Scale around the origin so that the xyz limits are [-0.9..0.9]
	if (g_ShadowMapping.bAnisotropicMapScale)
		S.scale(sx, sy, 1.0f); // Anisotropic scale: better use of the shadow map
	else
		S.scale(s, s, 1.0f); // Isotropic scale: better for debugging.

	*minZ = aabb.min.z;
	*range = aabb.max.z - aabb.min.z;

	if (g_bDumpSSAOBuffers) {
		log_debug("[DBG] [HNG] min-x,y,z: %0.3f, %0.3f, %0.3f, max-x,y,z: %0.3f, %0.3f, %0.3f",
			aabb.min.x, aabb.min.y, aabb.min.z,
			aabb.max.x, aabb.max.y, aabb.max.z);
		log_debug("[DBG] [HNG] cx,cy: %0.3f, %0.3f, sx,sy,s: %0.3f, %0.3f, %0.3f",
			cx, cy, sx, sy, s);
		log_debug("[DBG] [HNG] maxZ: %0.3f, minZ: %0.3f, range: %0.3f",
			aabb.max.z, *minZ, *range);
	}
	return S * T;
}

void EffectsRenderer::RenderCockpitShadowMap()
{
	_deviceResources->BeginAnnotatedEvent(L"RenderCockpitShadowMap");
	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;

	// We're still tagging the lights in PrimarySurface::TagXWALights(). Here we just render
	// the ShadowMap.

	// TODO: The g_bShadowMapEnable was added later to be able to toggle the shadows with a hotkey
	//	     Either remove the multiplicity of "enable" variables or get rid of the hotkey.
	g_ShadowMapping.bEnabled = g_bShadowMapEnable;
	g_ShadowMapVSCBuffer.sm_enabled = g_bShadowMapEnable && g_ShadowMapping.bUseShadowOBJ;
	// The post-proc shaders (SSDOAddPixel, SSAOAddPixel) use sm_enabled to compute shadows,
	// we must set the PS constants here even if we're not rendering shadows at all
	resources->InitPSConstantBufferShadowMap(resources->_shadowMappingPSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);

	// Don't render the shadow map if Raytraced cockpit shadows is enabled
	// ... unless we're in the hangar. Always do shadow mapping in the hangar
	if ((g_bRTEnabled && g_bRTEnabledInCockpit) && !(*g_playerInHangar))
	{
		goto out;
	}

	// Shadow Mapping is disabled when the we're in external view or traveling through hyperspace.
	// Maybe also disable it if the cockpit is hidden
	if (!g_ShadowMapping.bEnabled || !g_bShadowMapEnable || !g_ShadowMapping.bUseShadowOBJ || _bExternalCamera ||
		!_bCockpitDisplayed || g_HyperspacePhaseFSM != HS_INIT_ST || !_bCockpitConstantsCaptured ||
		_bShadowsRenderedInCurrentFrame)
	{
		goto out;
	}

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());

	// Enable ZWrite: we'll need it for the ShadowMap
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);
	// Solid blend state, no transparency
	resources->InitBlendState(_solidBlendState, nullptr);

	// Init the Viewport. This viewport has the dimensions of the shadowmap texture
	_deviceResources->InitViewport(&g_ShadowMapping.ViewPort);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);

	_deviceResources->InitVertexShader(resources->_shadowMapVS);
	_deviceResources->InitPixelShader(resources->_shadowMapPS);

	// Set the vertex and index buffers
	UINT stride = sizeof(D3DTLVERTEX), ofs = 0;
	resources->InitVertexBuffer(resources->_shadowVertexBuffer.GetAddressOf(), &stride, &ofs);
	resources->InitIndexBuffer(resources->_shadowIndexBuffer.Get(), false);

	g_ShadowMapVSCBuffer.sm_PCSS_enabled = g_bShadowMapEnablePCSS;
	g_ShadowMapVSCBuffer.sm_resolution = (float)g_ShadowMapping.ShadowMapSize;
	g_ShadowMapVSCBuffer.sm_hardware_pcf = g_bShadowMapHardwarePCF;
	// Select either the SW or HW bias depending on which setting is enabled
	g_ShadowMapVSCBuffer.sm_bias = g_bShadowMapHardwarePCF ? g_ShadowMapping.hw_pcf_bias : g_ShadowMapping.sw_pcf_bias;
	g_ShadowMapVSCBuffer.sm_enabled = g_bShadowMapEnable;
	g_ShadowMapVSCBuffer.sm_debug = g_bShadowMapDebug;
	g_ShadowMapVSCBuffer.sm_VR_mode = g_bEnableVR;

	// Set the cockpit transform matrix and other shading-related constants
	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_CockpitConstants, 0, 0);

	// Compute all the lightWorldMatrices and their OBJrange/minZ's first:
	for (int idx = 0; idx < *s_XwaGlobalLightsCount; idx++)
	{
		float range, minZ;
		// Don't bother computing shadow maps for lights with a high black level
		if (g_ShadowMapVSCBuffer.sm_black_levels[idx] > 0.95f)
			continue;

		// Reset the range for each active light
		g_ShadowMapVSCBuffer.sm_minZ[idx] = 0.0f;
		g_ShadowMapVSCBuffer.sm_maxZ[idx] = DEFAULT_COCKPIT_SHADOWMAP_MAX_Z; // Regular range for the cockpit

		// g_OPTMeshTransformCB.MeshTransform is only relevant if we're going to apply
		// mesh transforms to individual shadow map meshes. Like pieces of the diegetic cockpit.

		// Compute the LightView (Parallel Projection) Matrix
		// g_CurrentHeadingViewMatrix needs to have the correct data from SteamVR, including roll
		Matrix4 L = ComputeLightViewMatrix(idx, g_CurrentHeadingViewMatrix, false);
		Matrix4 ST = GetShadowMapLimits(L, &range, &minZ);

		g_ShadowMapVSCBuffer.lightWorldMatrix[idx] = ST * L;
		g_ShadowMapVSCBuffer.OBJrange[idx] = range;
		g_ShadowMapVSCBuffer.OBJminZ[idx] = minZ;

		// Render each light to its own shadow map
		g_ShadowMapVSCBuffer.light_index = idx;

		// Set the constant buffer for the VS and PS.
		resources->InitVSConstantBufferShadowMap(resources->_shadowMappingVSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);
		// The pixel shader is empty for the shadow map, but the SSAO/SSDO/Deferred PS do use these constants later
		resources->InitPSConstantBufferShadowMap(resources->_shadowMappingPSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);

		// Clear the Shadow Map DSV (I may have to update this later for the hyperspace state)
		context->ClearDepthStencilView(resources->_shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
		// Set the Shadow Map DSV
		context->OMSetRenderTargets(0, 0, resources->_shadowMapDSV.Get());
		// Render the Shadow Map
		context->DrawIndexed(g_ShadowMapping.NumIndices, 0, 0);

		// Copy the shadow map to the right slot in the array
		context->CopySubresourceRegion(resources->_shadowMapArray, D3D11CalcSubresource(0, idx, 1), 0, 0, 0,
			resources->_shadowMap, D3D11CalcSubresource(0, 0, 1), NULL);
	}

	if (g_bDumpSSAOBuffers) {
		wchar_t wFileName[80];
		
		for (int i = 0; i < *s_XwaGlobalLightsCount; i++) {
			context->CopySubresourceRegion(resources->_shadowMapDebug, D3D11CalcSubresource(0, 0, 1), 0, 0, 0,
				resources->_shadowMapArray, D3D11CalcSubresource(0, i, 1), NULL);
			swprintf_s(wFileName, 80, L"c:\\Temp\\_shadowMap%d.dds", i);
			DirectX::SaveDDSTextureToFile(context, resources->_shadowMapDebug, wFileName);
		}
	}

	RestoreContext();
	_bShadowsRenderedInCurrentFrame = true;

out:
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::RenderHangarShadowMap()
{
	_deviceResources->BeginAnnotatedEvent(L"RenderHangarShadowMap");
	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	Matrix4 S1, S2, ST;

	if (!*g_playerInHangar || !g_bShadowMapEnable || _ShadowMapDrawCommands.size() == 0 ||
		_bHangarShadowsRenderedInCurrentFrame || g_HyperspacePhaseFSM != HS_INIT_ST)
	{
		_ShadowMapDrawCommands.clear();
		goto out;
	}

	// If there's no cockpit shadow map, we must disable the first shadow map slot, but continue rendering hangar shadows
	if (!g_ShadowMapping.bUseShadowOBJ || _bExternalCamera)
	{
		// This is getting tricky, but the PBR shader modulates lights and shadows and we need to
		// keep the first light as a shadow caster or the PBR shader will darken everything when
		// enabled. For the Deferred shader, the first caster must still be disabled.
		// Currently, the global RT switch controls whether we use the Deferred or PBR shaders.
		g_ShadowMapVSCBuffer.sm_black_levels[0] = g_bRTEnabled ? 0.05f : 1.0f;
	}
	else
		g_ShadowMapVSCBuffer.sm_black_levels[0] = 0.05f;
	// Make hangar shadows darker, as in the original version
	g_ShadowMapVSCBuffer.sm_black_levels[1] = 0.05f;

	// Adjust the range for the shadow maps. The first light is only for the cockpit:
	g_ShadowMapVSCBuffer.sm_minZ[0] = 0.0f;
	g_ShadowMapVSCBuffer.sm_maxZ[0] = DEFAULT_COCKPIT_SHADOWMAP_MAX_Z;
	// The second light is for the hangar:
	g_ShadowMapVSCBuffer.sm_minZ[1] = _bExternalCamera ? 0.0f : DEFAULT_COCKPIT_SHADOWMAP_MAX_Z - DEFAULT_COCKPIT_SHADOWMAP_MAX_Z / 2.0f;
	g_ShadowMapVSCBuffer.sm_maxZ[1] = DEFAULT_HANGAR_SHADOWMAP_MAX_Z;

	// TODO: The g_bShadowMapEnable was added later to be able to toggle the shadows with a hotkey
	//	     Either remove the multiplicity of "enable" variables or get rid of the hotkey.
	g_ShadowMapping.bEnabled = g_bShadowMapEnable;
	g_ShadowMapVSCBuffer.sm_enabled = g_bShadowMapEnable;
	// The post-proc shaders (SSDOAddPixel, SSAOAddPixel) use sm_enabled to compute shadows,
	// we must set the PS constants here even if we're not rendering shadows at all
	resources->InitPSConstantBufferShadowMap(resources->_shadowMappingPSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);

	SaveContext();

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());

	// Enable ZWrite: we'll need it for the ShadowMap
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);
	// Solid blend state, no transparency
	resources->InitBlendState(_solidBlendState, nullptr);

	// Init the Viewport. This viewport has the dimensions of the shadowmap texture
	_deviceResources->InitViewport(&g_ShadowMapping.ViewPort);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);

	_deviceResources->InitVertexShader(resources->_hangarShadowMapVS);
	_deviceResources->InitPixelShader(resources->_shadowMapPS);

	g_ShadowMapVSCBuffer.sm_PCSS_enabled = g_bShadowMapEnablePCSS;
	g_ShadowMapVSCBuffer.sm_resolution = (float)g_ShadowMapping.ShadowMapSize;
	g_ShadowMapVSCBuffer.sm_hardware_pcf = g_bShadowMapHardwarePCF;
	// Select either the SW or HW bias depending on which setting is enabled
	g_ShadowMapVSCBuffer.sm_bias = g_bShadowMapHardwarePCF ? g_ShadowMapping.hw_pcf_bias : g_ShadowMapping.sw_pcf_bias;
	g_ShadowMapVSCBuffer.sm_enabled = g_bShadowMapEnable;
	g_ShadowMapVSCBuffer.sm_debug = g_bShadowMapDebug;
	g_ShadowMapVSCBuffer.sm_VR_mode = g_bEnableVR;

	// Other stuff that is common in the loop below
	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;

	// Shadow map limits
	float range, minZ;
	ST = GetShadowMapLimits(_hangarShadowAABB, &range, &minZ);

	// Compute all the lightWorldMatrices and their OBJrange/minZ's first:
	int ShadowMapIdx = 1; // Shadow maps for the hangar are always located at index 1
	g_ShadowMapVSCBuffer.OBJrange[ShadowMapIdx] = range;
	g_ShadowMapVSCBuffer.OBJminZ[ShadowMapIdx] = minZ;

	// Clear the Shadow Map DSV
	context->ClearDepthStencilView(resources->_shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
	// Set the Shadow Map DSV
	context->OMSetRenderTargets(0, 0, resources->_shadowMapDSV.Get());
	S1.scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS);
	S2.scale(METERS_TO_OPT, -METERS_TO_OPT, METERS_TO_OPT);

	for (DrawCommand command : _ShadowMapDrawCommands) {
		// Set the mesh buffers
		ID3D11ShaderResourceView* vsSSRV[1] = { command.vertexSRV };
		context->VSSetShaderResources(0, 1, vsSSRV);

		// Set the index and vertex buffers
		_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
		_deviceResources->InitVertexBuffer(&(command.vertexBuffer), &vertexBufferStride, &vertexBufferOffset);
		_deviceResources->InitIndexBuffer(nullptr, true);
		_deviceResources->InitIndexBuffer(command.indexBuffer, true);

		// Compute the LightView (Parallel Projection) Matrix
		// See HangarShadowSceneHook for an explanation of why L looks like this:
		Matrix4 L = _hangarShadowMapRotation * S1 * Matrix4(command.constants.hangarShadowView) * S2;
		g_ShadowMapVSCBuffer.lightWorldMatrix[ShadowMapIdx] = ST * L;
		g_ShadowMapVSCBuffer.light_index = ShadowMapIdx;

		// Set the constants buffer
		context->UpdateSubresource(_constantBuffer, 0, nullptr, &(command.constants), 0, 0);

		// Set the number of triangles
		_trianglesCount = command.trianglesCount;

		// Set the constant buffer for the VS and PS.
		resources->InitVSConstantBufferShadowMap(resources->_shadowMappingVSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);
		// The pixel shader is empty for the shadow map, but the SSAO/SSDO/Deferred PS do use these constants later
		resources->InitPSConstantBufferShadowMap(resources->_shadowMappingPSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);

		// Render the Shadow Map
		context->DrawIndexed(_trianglesCount * 3, 0, 0);
	}

	// Copy the shadow map to the right slot in the array
	context->CopySubresourceRegion(resources->_shadowMapArray, D3D11CalcSubresource(0, ShadowMapIdx, 1), 0, 0, 0,
		resources->_shadowMap, D3D11CalcSubresource(0, 0, 1), NULL);

	if (g_bDumpSSAOBuffers) {
		context->CopySubresourceRegion(resources->_shadowMapDebug, D3D11CalcSubresource(0, 0, 1), 0, 0, 0,
			resources->_shadowMapArray, D3D11CalcSubresource(0, ShadowMapIdx, 1), NULL);
		DirectX::SaveDDSTextureToFile(context, resources->_shadowMapDebug, L"c:\\Temp\\_shadowMapHangar.dds");
	}

	RestoreContext();
	_bHangarShadowsRenderedInCurrentFrame = true;
	_ShadowMapDrawCommands.clear();

out:
	_deviceResources->EndAnnotatedEvent();
}

void EffectsRenderer::StartCascadedShadowMap()
{
	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;

	if (!g_ShadowMapping.bCSMEnabled || g_HyperspacePhaseFSM != HS_INIT_ST) {
		return;
	}

	// This is the shadow factor, 1.0 disables shadows, 0.0 makes shadows pitch black.
	g_ShadowMapVSCBuffer.sm_black_levels[0] = 0.05f;
	g_ShadowMapVSCBuffer.sm_black_levels[1] = 0.05f;

	// Adjust the range for the shadow maps. The first light is only for the cockpit:
	g_ShadowMapVSCBuffer.sm_minZ[0] = 10.0f;
	g_ShadowMapVSCBuffer.sm_maxZ[0] = 100.0f;
	// The second light is for the hangar:
	g_ShadowMapVSCBuffer.sm_minZ[1] = 100.0f;
	g_ShadowMapVSCBuffer.sm_maxZ[1] = 300.0f;

	//g_ShadowMapVSCBuffer.sm_PCSS_enabled = g_bShadowMapEnablePCSS;
	g_ShadowMapVSCBuffer.sm_resolution = (float)g_ShadowMapping.ShadowMapSize;
	g_ShadowMapVSCBuffer.sm_hardware_pcf = g_bShadowMapHardwarePCF;
	// Select either the SW or HW bias depending on which setting is enabled
	g_ShadowMapVSCBuffer.sm_bias = g_bShadowMapHardwarePCF ? g_ShadowMapping.hw_pcf_bias : g_ShadowMapping.sw_pcf_bias;
	g_ShadowMapVSCBuffer.sm_enabled = g_bShadowMapEnable;
	g_ShadowMapVSCBuffer.sm_debug = g_bShadowMapDebug;
	g_ShadowMapVSCBuffer.sm_VR_mode = g_bEnableVR;

	// TODO: The g_bShadowMapEnable was added later to be able to toggle the shadows with a hotkey
	//	     Either remove the multiplicity of "enable" variables or get rid of the hotkey.
	//g_ShadowMapping.bEnabled = g_bShadowMapEnable;
	//g_ShadowMapVSCBuffer.sm_enabled = g_bShadowMapEnable;
	

	// Shadow map limits
	//float range, minZ;
	//Matrix4 ST = GetShadowMapLimits(_hangarShadowAABB, &range, &minZ);
	Matrix4 ST;
	ST.identity();

	// Compute all the lightWorldMatrices and their OBJrange/minZ's first:
	int ShadowMapIdx = 1; // Shadow maps for the hangar are always located at index 1
	g_ShadowMapVSCBuffer.OBJrange[ShadowMapIdx] = 300;
	g_ShadowMapVSCBuffer.OBJminZ[ShadowMapIdx] = 50;

	Matrix4 S1, S2;
	//S1.scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS);
	//S2.scale(METERS_TO_OPT, -METERS_TO_OPT, METERS_TO_OPT);
	S1.identity();
	S2.identity();

	// Compute the LightView (Parallel Projection) Matrix
	// See HangarShadowSceneHook for an explanation of why L looks like this:
	//Matrix4 L = _hangarShadowMapRotation * S1 * Matrix4(command.constants.hangarShadowView) * S2;
	Matrix4 L = S1;
	g_ShadowMapVSCBuffer.lightWorldMatrix[ShadowMapIdx] = ST * L;
	g_ShadowMapVSCBuffer.light_index = ShadowMapIdx;

	// The post-proc shaders (SSDOAddPixel, SSAOAddPixel) use sm_enabled to compute shadows,
	// we must set the PS constants here even if we're not rendering shadows at all
	resources->InitPSConstantBufferShadowMap(resources->_shadowMappingPSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);

	// Set the constant buffer for the VS and PS.
	resources->InitVSConstantBufferShadowMap(resources->_shadowMappingVSConstantBuffer.GetAddressOf(), &g_ShadowMapVSCBuffer);

	// Clear the Shadow Map DSV
	context->ClearDepthStencilView(resources->_shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void EffectsRenderer::RenderCascadedShadowMap()
{
	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;

	if (!g_ShadowMapping.bCSMEnabled || g_HyperspacePhaseFSM != HS_INIT_ST) {
		return;
	}

	//SaveContext();

	//context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	//context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());

	// Enable ZWrite: we'll need it for the ShadowMap
	/*
	D3D11_DEPTH_STENCIL_DESC desc;
	ComPtr<ID3D11DepthStencilState> depthState;
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	desc.StencilEnable = FALSE;
	resources->InitDepthStencilState(depthState, &desc);
	// Solid blend state, no transparency
	resources->InitBlendState(_solidBlendState, nullptr);
	*/

	// Init the Viewport. This viewport has the dimensions of the shadowmap texture
	_deviceResources->InitViewport(&g_ShadowMapping.ViewPort);
	//_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//_deviceResources->InitInputLayout(_inputLayout);

	_deviceResources->InitVertexShader(resources->_csmVS);
	_deviceResources->InitPixelShader(resources->_shadowMapPS);

	// Set the Shadow Map DSV
	context->OMSetRenderTargets(0, 0, resources->_csmMapDSV.Get());

	// Render the Shadow Map
	context->DrawIndexed(_trianglesCount * 3, 0, 0);
}

void EffectsRenderer::EndCascadedShadowMap()
{
	auto& resources = _deviceResources;
	auto& context = resources->_d3dDeviceContext;

	if (!g_ShadowMapping.bCSMEnabled || g_HyperspacePhaseFSM != HS_INIT_ST) {
		return;
	}

	int ShadowMapIdx = 1; // Shadow maps for the hangar are always located at index 1

	// Copy the shadow map to the right slot in the array
	context->CopySubresourceRegion(resources->_csmArray, D3D11CalcSubresource(0, ShadowMapIdx, 1), 0, 0, 0,
		resources->_csmMap, D3D11CalcSubresource(0, 0, 1), NULL);

	if (g_bDumpSSAOBuffers) {
		context->CopySubresourceRegion(resources->_shadowMapDebug, D3D11CalcSubresource(0, 0, 1), 0, 0, 0,
			resources->_csmArray, D3D11CalcSubresource(0, ShadowMapIdx, 1), NULL);
		DirectX::SaveDDSTextureToFile(context, resources->_shadowMapDebug, L"c:\\Temp\\_csmMap.dds");
	}

	//RestoreContext();
}

void EffectsRenderer::HangarShadowSceneHook(const SceneCompData* scene)
{
	if (!g_config.EnableSoftHangarShadows) {
		// Jump to the original version of this hook and return: this disables the new effect.
		// There are some artifacts that need to be fixed before going live with this version.
		D3dRenderer::HangarShadowSceneHook(scene);
		return;
	}

	ID3D11DeviceContext* context = _deviceResources->_d3dDeviceContext;
	auto &resources = _deviceResources;

	ComPtr<ID3D11Buffer> oldVSConstantBuffer;
	ComPtr<ID3D11Buffer> oldPSConstantBuffer;
	ComPtr<ID3D11ShaderResourceView> oldVSSRV[3];

	context->VSGetConstantBuffers(0, 1, oldVSConstantBuffer.GetAddressOf());
	context->PSGetConstantBuffers(0, 1, oldPSConstantBuffer.GetAddressOf());
	context->VSGetShaderResources(0, 3, oldVSSRV[0].GetAddressOf());

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	_deviceResources->InitRasterizerState(_rasterizerState);
	_deviceResources->InitSamplerState(_samplerState.GetAddressOf(), nullptr);

	UpdateTextures(scene);
	UpdateMeshBuffers(scene);
	UpdateVertexAndIndexBuffers(scene);
	UpdateConstantBuffer(scene);

	int ShadowMapIdx = 1;
	Matrix4 W = XwaTransformToMatrix4(scene->WorldViewTransform);
	Matrix4 S1, S2;
	S1.scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS);
	S2.scale(METERS_TO_OPT, -METERS_TO_OPT, METERS_TO_OPT);
	// The transform chain here looks a bit odd, but it makes sense. Here's the explanation.
	// XWA uses:
	//
	//     V = _constants.hangarShadowView * WorldViewTransform * V
	//
	// To transform from the OPT system to the world view and then to the light's point of view
	// in the hangar. But this transform chain only works when transforming from OPT coordinates.
	// Meanwhile, the transform rule in XwaD3DPixelShader looks like this:
	//
	//     output.pos3D = scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS) * transformWorldView * V
	//
	// or:
	//
	//     output.pos3D = S * W * V
	//
	// We need pos3D, because that's what we use for shadow mapping. But we need to apply
	// hangarShadowView before we apply the OPT_TO_METERS scale matrix or the transform won't work.
	// So this is how I solved this problem:
	// Transform V into metric 3D, so that we get a valid pos3D, but then invert the scale matrix
	// so that we get OPT scale back again and then we can apply the hangarShadowView in a valid
	// coordinate system. Finally, apply the scale matrix once more to get metric coordinates and
	// project. Unfortunately, after using hangarShadowView, things will be "upside down" in the
	// shadow map, so we have to rotate things by 180 degrees. In other words, we do this:
	//
	//    V = (RotX180 * S * hangarShadowView * S') * S * W * V
	//
	// where L = (RotX180 * S * hangarShadowView * S') is our light view transform:
	// (BTW, we need to use this same rule in HangarShadowMapVS)
	Matrix4 L = _hangarShadowMapRotation * S1 * Matrix4(_constants.hangarShadowView) * S2;

	// Compute the AABB for the hangar to use later when rendering the shadow map
	auto it = _AABBs.find((int)(scene->MeshVertices));
	if (it != _AABBs.end())
	{
		AABB aabb = it->second;
		aabb.UpdateLimits();
		aabb.TransformLimits(L * S1 * W);
		_hangarShadowAABB.Expand(aabb.Limits);
	}

	// Dump the current scene to an OBJ file
	if (g_bDumpSSAOBuffers && (bHangarDumpOBJEnabled || bD3DDumpOBJEnabled))
		OBJDumpD3dVertices(scene, _hangarShadowMapRotation);

	// The hangar shadow map must be rendered as a post-processing effect. The main reason is that
	// we don't know the size of the AABB for the hangar until we're done rendering it. So, we're
	// going to store the commands to use later.
	DrawCommand command;
	// Save the Vertex SRV
	command.vertexSRV = _lastMeshVerticesView;
	// Save the vertex and index buffers
	command.vertexBuffer = _lastVertexBuffer;
	command.indexBuffer = _lastIndexBuffer;
	command.trianglesCount = _trianglesCount;
	// Save the constants
	command.constants = _constants;
	// Add the command to the list of deferred commands
	_ShadowMapDrawCommands.push_back(command);

	context->VSSetConstantBuffers(0, 1, oldVSConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, oldPSConstantBuffer.GetAddressOf());
	context->VSSetShaderResources(0, 3, oldVSSRV[0].GetAddressOf());
}

/*
 * This method is called from two places: in Direct3D::Execute() at the beginning of the HUD and
 * in PrimarySurface::Flip() before we start rendering post-proc effects. Any calls placed in this
 * method should be idempotent or they will render the same content twice.
 */
void EffectsRenderer::RenderDeferredDrawCalls()
{
	_deviceResources->BeginAnnotatedEvent(L"RenderDeferredDrawCalls");
	// All the calls below should be rendered with RendererType_Main
	g_rendererType = RendererType_Main;
	RenderCockpitShadowMap();
	RenderHangarShadowMap();

	RenderVRKeyboard();
	// We can't render the VR dots here because they will get obscured by the transparency
	// layers. They have to be rendered after the DeferredPass() has been executed.
	//RenderVRDots();
	RenderVRGloves();

	// RenderLasers() has to be executed after RenderTransparency(). The reason is that in
	// RenderTransparency() we're rendering glass on external objects and the cockpit, and
	// the lasers are rendered to the first transparent layer afterwards. So, the order is:
	// External Glass --> Lasers --> Cockpit Glass
	// RenderTransparency() renders both the External Glass and Cockpit Glass to different layers.
	RenderTransparency();
	RenderLasers();

	//RenderVRHUD();
	_deviceResources->EndAnnotatedEvent();
}
