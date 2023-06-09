#pragma once
#include "common.h"
#include <vector>

#include "EffectsCommon.h"
#include "xwa_structures.h"

constexpr int ENCODED_TREE_NODE4_SIZE = 64; // BVH4 node size

struct Vector3;
struct Vector4;

#pragma pack(push, 1)

// QBVH inner node
struct BVHNode {
	int ref;    // TriID: -1 for internal nodes, Triangle index for leaves
	int parent; // Not used at this point
	int rootIdx;
	int numChildren;
	// 16 bytes
	float min[4];
	// 32 bytes
	float max[4];
	// 48 bytes
	int children[4];
	// 64 bytes
};

// QBVH BLAS leaf node
struct BVHPrimNode {
	int ref;
	int parent;
	int padding[2];
	// 16 bytes
	float v0[4];
	// 32 bytes
	float v1[4];
	// 48 bytes
	float v2[4];
	// 64 bytes
};

// QBVH TLAS leaf node
struct BVHTLASLeafNode {
	int ref;         // Mesh ID
	int parent;
	int matrixSlot;  // WorldView matrix slot
	int BLASBaseNodeOfs; // Base offset into _RTBvh where the corresponding BLAS begins
	// 16 bytes
	float min[4];    // Object-space AABB min (needs to be multiplied by the matrix in matrixSlot)
	// 32 bytes
	float max[4];    // Object-space AABB max  (needs to be multiplied by the matrix in matrixSlot)
	// 48 bytes
	float obb_max[4];
	// 64 bytes
};

static_assert(sizeof(BVHNode) == ENCODED_TREE_NODE4_SIZE, "BVHNode (4) must be ENCODED_TREE_NODE4_SIZE bytes");
static_assert(sizeof(BVHPrimNode) == ENCODED_TREE_NODE4_SIZE, "BVHPrimNode (4) must be ENCODED_TREE_NODE4_SIZE bytes");
static_assert(sizeof(BVHTLASLeafNode) == ENCODED_TREE_NODE4_SIZE, "BVHTLASLeafNode (4) must be ENCODED_TREE_NODE4_SIZE bytes");

struct MinMax {
	Vector3 min;
	Vector3 max;
};

#pragma pack(pop)

// NodeIndex, IsLeaf, QBVHEncodeOfs
using EncodeItem = std::tuple<int, bool, int>;

class AABB
{
public:
	Vector3 min;
	Vector3 max;
	// Limits are computed from min, max. Do not update Limits directly. Set min,max and
	// call UpdateLimits() instead.
	std::vector<Vector4> Limits;

	AABB() {
		SetInfinity();
	}

	inline void SetInfinity() {
		min.x = min.y = min.z = FLT_MAX;
		max.x = max.y = max.z = -FLT_MAX;
	}

	bool IsInvalid() {
		return (min.x == FLT_MAX || max.x == -FLT_MAX);
	}

	inline void Expand(const XwaVector3 &v) {
		if (v.x < min.x) min.x = v.x;
		if (v.y < min.y) min.y = v.y;
		if (v.z < min.z) min.z = v.z;

		if (v.x > max.x) max.x = v.x;
		if (v.y > max.y) max.y = v.y;
		if (v.z > max.z) max.z = v.z;
	}

	inline void Expand(const Vector4 &v) {
		if (v.x < min.x) min.x = v.x;
		if (v.y < min.y) min.y = v.y;
		if (v.z < min.z) min.z = v.z;

		if (v.x > max.x) max.x = v.x;
		if (v.y > max.y) max.y = v.y;
		if (v.z > max.z) max.z = v.z;
	}

	inline void Expand(const Vector3& v) {
		if (v.x < min.x) min.x = v.x;
		if (v.y < min.y) min.y = v.y;
		if (v.z < min.z) min.z = v.z;

		if (v.x > max.x) max.x = v.x;
		if (v.y > max.y) max.y = v.y;
		if (v.z > max.z) max.z = v.z;
	}

	inline void Expand(const float3& v) {
		if (v.x < min.x) min.x = v.x;
		if (v.y < min.y) min.y = v.y;
		if (v.z < min.z) min.z = v.z;

		if (v.x > max.x) max.x = v.x;
		if (v.y > max.y) max.y = v.y;
		if (v.z > max.z) max.z = v.z;
	}

	inline void Expand(const AABB &aabb) {
		if (aabb.min.x < min.x) min.x = aabb.min.x;
		if (aabb.min.y < min.y) min.y = aabb.min.y;
		if (aabb.min.z < min.z) min.z = aabb.min.z;

		if (aabb.max.x > max.x) max.x = aabb.max.x;
		if (aabb.max.y > max.y) max.y = aabb.max.y;
		if (aabb.max.z > max.z) max.z = aabb.max.z;
	}

	inline void Expand(const RTCBounds* bounds) {
		if (bounds->lower_x < min.x) min.x = bounds->lower_x;
		if (bounds->lower_y < min.y) min.y = bounds->lower_y;
		if (bounds->lower_z < min.z) min.z = bounds->lower_z;

		if (bounds->upper_x > max.x) max.x = bounds->upper_x;
		if (bounds->upper_y > max.y) max.y = bounds->upper_y;
		if (bounds->upper_z > max.z) max.z = bounds->upper_z;
	}

	inline XwaVector3 GetCentroid()
	{
		return XwaVector3(
			(min.x + max.x) / 2.0f,
			(min.y + max.y) / 2.0f,
			(min.z + max.z) / 2.0f);
	}

	inline Vector3 GetCentroidVector3()
	{
		return Vector3(
			(min.x + max.x) / 2.0f,
			(min.y + max.y) / 2.0f,
			(min.z + max.z) / 2.0f);
	}

	inline void Expand(const std::vector<Vector4> &Limits)
	{
		for each (Vector4 v in Limits)
			Expand(v);
	}

	inline Vector3 GetRange() {
		return max - min;
	}

	inline int GetLargestDimension()
	{
		Vector3 range = GetRange();
		float max = range.x;
		int idx = 0;

		if (range.y > max)
		{
			max = range.y;
			idx = 1;
		}

		if (range.z > max)
		{
			max = range.z;
			idx = 2;
		}

		return idx;
	}

	void UpdateLimits() {
		Limits.clear();
		Limits.push_back(Vector4(min.x, min.y, min.z, 1.0f));
		Limits.push_back(Vector4(max.x, min.y, min.z, 1.0f));
		Limits.push_back(Vector4(max.x, max.y, min.z, 1.0f));
		Limits.push_back(Vector4(min.x, max.y, min.z, 1.0f));

		Limits.push_back(Vector4(min.x, min.y, max.z, 1.0f));
		Limits.push_back(Vector4(max.x, min.y, max.z, 1.0f));
		Limits.push_back(Vector4(max.x, max.y, max.z, 1.0f));
		Limits.push_back(Vector4(min.x, max.y, max.z, 1.0f));
	}

	void TransformLimits(const Matrix4 &T) {
		for (uint32_t i = 0; i < Limits.size(); i++)
			Limits[i] = T * Limits[i];
	}

	inline AABB GetAABBFromCurrentLimits()
	{
		AABB result;
		for (const Vector4 &v : Limits)
			result.Expand(v);
		return result;
	}

	float GetArea()
	{
		Vector3 range = GetRange();
		// Area of a parallelepiped: 2ab + 2bc + 2ac
		return 2.0f * (range.x * range.y + range.y * range.z * range.x * range.z);
	}

	std::string ToString()
	{
		return "(" +
			std::to_string(min.x) + ", " +
			std::to_string(min.y) + ", " +
			std::to_string(min.z) + ")-(" +
			std::to_string(max.x) + ", " +
			std::to_string(max.y) + ", " +
			std::to_string(max.z) + ")";
	}

	int DumpLimitsToOBJ(FILE *D3DDumpOBJFile, const std::string &name, int VerticesCountOffset);
	int DumpLimitsToOBJ(FILE *D3DDumpOBJFile, int OBJGroupId, int VerticesCountOffset);
};

// Classes and data used for the Fast LBVH build.
#undef MORTON_CODE_30
#ifdef MORTON_CODE_30
using MortonCode_t = uint32_t;
#else
using MortonCode_t = uint64_t;
#endif
// 0: Morton Code, 1: Bounding Box, 2: TriID
//using LeafItem = std::tuple<MortonCode_t, AABB, int>;
// 0: Morton Code, 1: aabbFromOBB, 2: BlasID, 3: Centroid, 4: MatrixSlot, 5: Oriented Bounding Box
//using TLASLeafItem = std::tuple<MortonCode_t, AABB, int, XwaVector3, int, AABB>;
// Used in the DirectBVH algorithms
struct LeafItem
{
	MortonCode_t code;
	Vector3 centroid;
	AABB aabb;
	int PrimID;
};
// Used in the DirectBVH algorithms
struct TLASLeafItem : LeafItem
{
	int matrixSlot;
	AABB obb;
};

inline MortonCode_t &GetMortonCode(LeafItem& X) { return X.code; }
inline AABB &GetAABB(LeafItem& X) { return X.aabb; }
inline int& GetID(LeafItem& X) { return X.PrimID; }

inline MortonCode_t& TLASGetMortonCode(TLASLeafItem& X) { return X.code; }
inline AABB& TLASGetAABBFromOBB(TLASLeafItem& X) { return X.aabb; }
inline int& TLASGetID(TLASLeafItem& X) { return X.PrimID; }
inline Vector3& TLASGetCentroid(TLASLeafItem& X) { return X.centroid; }
inline int& TLASGetMatrixSlot(TLASLeafItem& X) { return X.matrixSlot; }
inline AABB& TLASGetOBB(TLASLeafItem& X) { return X.obb; }

struct InnerNode
{
	int parent;
	// Children
	int left;
	int right;
	bool leftIsLeaf;
	bool rightIsLeaf;
	// Range
	int first;
	int last;
	AABB aabb;
	// Processing
	uint8_t readyCount;
	bool processed;
};

struct InnerNode4
{
	int parent;
	// Children
	int children[4];
	bool isLeaf[4];
	int QBVHOfs[4]; // Encoded QBVH offset for the children
	int selfQBVHOfs; // Encoded QBVH offset for this node
	int numChildren;
	int totalNodes;
	// Range
	int first;
	int last;
	AABB aabb;
	// Processing
	uint8_t readyCount;
	bool processed;
	bool isEncoded;
};

bool leafSorter(const LeafItem& i, const LeafItem& j);
bool tlasLeafSorter(const TLASLeafItem& i, const TLASLeafItem& j);

class IGenericTreeNode
{
public:
	virtual int GetArity() = 0;
	virtual bool IsLeaf() = 0;
	virtual AABB GetBox() = 0;
	virtual int GetTriID() = 0;
	virtual std::vector<IGenericTreeNode *> GetChildren() = 0;
	virtual IGenericTreeNode *GetParent() = 0;
	virtual void SetNumNodes(int numNodes) = 0;
	virtual int GetNumNodes() = 0;
};

class TreeNode : public IGenericTreeNode
{
public:
	int TriID, numNodes;
	TreeNode *left, *right, *parent;
	AABB box;
	XwaVector3 centroid;
	MortonCode_t code;
	Matrix4 m; // Used for TLAS leaves to represent OOBBs.
	bool red;  // Used in the Red-Black tree implementation.
	int bal;   // Used in the AVL tree BVH implementation.

	void InitNode()
	{
		TriID = -1;
		left = right = parent = nullptr;
		box.SetInfinity();
		numNodes = 1;
		code = 0xFFFFFFFF;
		// All new nodes are red by default
		red = true;
		bal = 0;
		// Invalid centroid:
		centroid.x = NAN;
		centroid.y = NAN;
		centroid.z = NAN;
		m.identity();
	}

	TreeNode()
	{
		InitNode();
	}

	TreeNode(int TriID)
	{
		InitNode();
		this->TriID = TriID;
	}

	TreeNode(int TriID, MortonCode_t code)
	{
		InitNode();
		this->TriID = TriID;
		this->code = code;
	}

	TreeNode(int TriID, MortonCode_t code, const AABB &box)
	{
		InitNode();
		this->TriID = TriID;
		this->code = code;
		this->box.Expand(box);
	}

	TreeNode(int TriID, MortonCode_t code, const AABB& box, const Matrix4 &m)
	{
		InitNode();
		this->TriID = TriID;
		this->code = code;
		this->box.Expand(box);
		this->m.set(m.get());
	}

	TreeNode(int TriID, TreeNode *left, TreeNode *right)
	{
		InitNode();
		this->TriID = TriID;
		this->left = left;
		this->right = right;
	}

	TreeNode(int TriID, const AABB& box)
	{
		InitNode();
		this->TriID = TriID;
		this->box.Expand(box);
	}

	TreeNode(int TriID, const XwaVector3 &centroid, const AABB& box)
	{
		InitNode();
		this->TriID = TriID;
		this->box.Expand(box);
		this->centroid = centroid;
	}

	/*
	TreeNode(int TriID, const AABB &box, const XwaVector3 &centroid)
	{
		InitNode();
		this->TriID = TriID;
		this->box.Expand(box);
		this->centroid = centroid;
	}
	*/

	TreeNode(int TriID, const AABB &box, TreeNode *left, TreeNode *right)
	{
		InitNode();
		this->TriID = TriID;
		this->left = left;
		this->right = right;
		this->box.Expand(box);
	}

	TreeNode(int TriID, TreeNode *left, TreeNode *right, TreeNode *parent)
	{
		InitNode();
		this->TriID = TriID;
		this->left = left;
		this->right = right;
		this->parent = parent;
	}

	virtual int GetArity()
	{
		return 2;
	}

	virtual AABB GetBox()
	{
		return this->box;
	}

	/// <summary>
	/// Multiplies this node's transform matrix with its box, thus generating a set of
	/// limits representing an OOBB. Then it gets the AABB that contains that OOBB
	/// </summary>
	/// <returns></returns>
	AABB GetAABBFromOOBB()
	{
		this->box.UpdateLimits();
		this->box.TransformLimits(this->m);
		AABB res;
		for (auto& v : this->box.Limits)
			res.Expand(v);
		return res;
	}

	virtual int GetTriID()
	{
		return this->TriID;
	}

	virtual bool IsLeaf()
	{
		return (this->left == nullptr) && (this->right == nullptr);
	}

	virtual std::vector<IGenericTreeNode*> GetChildren()
	{
		std::vector<IGenericTreeNode*> List;
		if (this->left != nullptr)
			List.push_back(this->left);
		if (this->right != nullptr)
			List.push_back(this->right);
		return List;
	}

	virtual IGenericTreeNode *GetParent()
	{
		return this->parent;
	}

	virtual void SetNumNodes(int numNodes)
	{
		this->numNodes = numNodes;
	}

	virtual int GetNumNodes()
	{
		return this->numNodes;
	}
};

class QTreeNode : public IGenericTreeNode
{
public:
	int TriID, numNodes;
	QTreeNode* parent;
	QTreeNode* children[4];
	AABB box;

	void SetNullChildren()
	{
		for (int i = 0; i < 4; i++)
			this->children[i] = nullptr;
	}

	void SetChildren(QTreeNode* children[])
	{
		for (int i = 0; i < 4; i++) {
			this->children[i] = children[i];
			if (children[i] != nullptr)
				this->numNodes += children[i]->numNodes;
		}
	}

	void Init()
	{
		this->TriID = -1;
		this->box.SetInfinity();
		this->numNodes = 0;
		this->parent = nullptr;
		SetNullChildren();
	}

	QTreeNode()
	{
		Init();
	}

	QTreeNode(int TriID)
	{
		this->TriID = TriID;
		this->box.SetInfinity();
		this->numNodes = 0;
		this->parent = nullptr;
		SetNullChildren();
	}

	QTreeNode(int TriID, AABB box)
	{
		this->TriID = TriID;
		this->box = box;
		this->numNodes = 1;
		this->parent = nullptr;
		SetNullChildren();
	}

	QTreeNode(int TriID, QTreeNode *children[])
	{
		this->TriID = TriID;
		this->box.SetInfinity();
		this->numNodes = 1;
		this->parent = nullptr;
		SetChildren(children);
	}

	QTreeNode(int TriID, AABB box, QTreeNode *children[], QTreeNode* parent)
	{
		this->TriID = TriID;
		this->box = box;
		this->parent = parent;
		this->numNodes = 1;
		SetChildren(children);
	}

	virtual int GetArity()
	{
		return 4;
	}

	virtual AABB GetBox()
	{
		return this->box;
	}

	virtual int GetTriID()
	{
		return this->TriID;
	}

	virtual bool IsLeaf()
	{
		for (int i = 0; i < 4; i++)
			if (children[i] != nullptr)
				return false;
		return true;
	}

	virtual std::vector<IGenericTreeNode*> GetChildren()
	{
		std::vector<IGenericTreeNode*> result;
		for (int i = 0; i < 4; i++)
			if (children[i] != nullptr)
				result.push_back(children[i]);
		return result;
	}

	virtual IGenericTreeNode* GetParent()
	{
		return parent;
	}

	virtual void SetNumNodes(int numNodes)
	{
		this->numNodes = numNodes;
	}

	virtual int GetNumNodes()
	{
		return this->numNodes;
	}
};

class BufferTreeNode : public IGenericTreeNode
{
public:
	BVHNode* nodes;
	int curNode;
	int numNodes;

	BufferTreeNode(BVHNode* nodes, int curNode)
	{
		this->nodes = nodes;
		this->curNode = curNode;
		numNodes = 1;
	}

	virtual int GetArity()
	{
		return 4;
	}

	virtual AABB GetBox()
	{
		AABB box;
		box.min.x = nodes[curNode].min[0];
		box.min.y = nodes[curNode].min[1];
		box.min.z = nodes[curNode].min[2];

		box.max.x = nodes[curNode].max[0];
		box.max.y = nodes[curNode].max[1];
		box.max.z = nodes[curNode].max[2];

		return box;
	}

	virtual int GetTriID() {
		return nodes[curNode].ref;
	}

	virtual bool IsLeaf() {
		return nodes[curNode].ref != -1;
	}

	virtual std::vector<IGenericTreeNode*> GetChildren()
	{
		std::vector<IGenericTreeNode*> children;
		if (IsLeaf())
			return children;

		for (int i = 0; i < 4; i++)
		{
			int childIdx = nodes[curNode].children[i];
			if (childIdx == -1)
				break;
			children.push_back(new BufferTreeNode(nodes, childIdx));
		}
		return children;
	}

	virtual IGenericTreeNode* GetParent()
	{
		return new BufferTreeNode(nodes, nodes[curNode].parent);
	}

	virtual void SetNumNodes(int numNodes)
	{
		this->numNodes = numNodes;
	}

	virtual int GetNumNodes()
	{
		return this->numNodes;
	}
};

class LBVH {
public:
	float3 *vertices;
	int32_t *indices;
	BVHNode *rawBuffer;
	BVHNode *nodes;
	uint32_t *vertexCounts;
	int /* rootIdx,*/ numVertices, numIndices, numNodes;
	float scale;
	bool scaleComputed;

	LBVH() {
		this->vertices = nullptr;
		this->indices = nullptr;
		this->nodes = nullptr;
		this->rawBuffer = nullptr;
		this->scale = 1.0f;
		this->scaleComputed = false;

		this->numVertices = 0;
		this->numIndices = 0;
		this->numNodes = 0;
		this->vertexCounts = nullptr;
	}
	
	~LBVH() {
		if (vertices != nullptr)
			delete[] vertices;

		if (indices != nullptr)
			delete[] indices;

		// "nodes" always points to the root, but sometimes
		// "rawBuffer" may be a little larger than "nodes" and
		// point to the same memory area. This happens with the
		// bottom-up single-step QBVH builder. If set, "rawBuffer"
		// should be released.
		if (rawBuffer != nullptr)
			delete[] rawBuffer;
		else if (nodes != nullptr)
			delete[] nodes;
	}

	static LBVH *LoadLBVH(char *sFileName, bool EmbeddedVerts=false, bool verbose=false);
	// Build the BVH2. Convert to QBVH. Encode the QBVH.
	static LBVH *Build(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// PLOC. Build BVH2, convert to QBVH, encode.
	static LBVH* BuildPLOC(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// Build the QBVH at the same time as the BVH2 is built. Encoding is a separate step.
	static LBVH *BuildQBVH(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// Build & Encode the QBVH at the same time as the BVH2 is built.
	static LBVH *BuildFastQBVH(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// Build & Encode using Embree
	static LBVH* BuildEmbree(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// Build & Encode using the DirectBVH2 approach (no Morton codes).
	static LBVH* BuildDirectBVH2CPU(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// Build & Encode using the DirectBVH2 approach (no Morton codes). GPU-Friendly version.
	static LBVH* BuildDirectBVH2GPU(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// Build & Encode using the DirectBVH4 approach (no Morton codes). GPU-Friendly version.
	static LBVH* BuildDirectBVH4GPU(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);
	// Online build, powered by the AVL algorithm (experimental)
	static LBVH* BuildAVL(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices);

	void PrintTree(std::string level, int curnode);
	void DumpToOBJ(char *sFileName, bool isTLAS=false, bool useMetricScale=true);
};

using NodeChildKey = std::tuple<uint32_t, int>;

int CalcNumInnerQBVHNodes(int numPrimitives);

void Normalize(Vector3& A, const AABB& sceneBox, const XwaVector3& range);

MortonCode_t GetMortonCode(const Vector3& V);

// Red-Black balanced insertion
TreeNode* InsertRB(TreeNode* T, int TriID, MortonCode_t code, const AABB& box, const Matrix4& m);
void DeleteRB(TreeNode* T);
int DumpRBToOBJ(FILE* file, TreeNode* T, const std::string& name, int VerticesCountOffset);

QTreeNode* BinTreeToQTree(int curNode, bool curNodeIsLeaf, const InnerNode* innerNodes, const std::vector<LeafItem>& leafItems);
void DeleteTree(TreeNode* T);
void DeleteTree(QTreeNode* Q);

uint8_t* EncodeNodes(IGenericTreeNode* root, const XwaVector3* Vertices, const int* Indices);
uint8_t* TLASEncodeNodes(IGenericTreeNode* root, std::vector<TLASLeafItem>& leafItems);
int TLASEncodeLeafNode(BVHNode* buffer, std::vector<TLASLeafItem>& leafItems, int leafIdx, int EncodeNodeIdx);

void DeleteBufferTree(BufferTreeNode *node);

void TLASSingleStepFastLQBVH(BVHNode* buffer, int numQBVHInnerNodes, std::vector<TLASLeafItem>& leafItems, int& root_out);

// Embree definitions for dynamic DLL loading.
typedef RTCDevice (*rtcNewDeviceFun)(const char* config);
typedef void (*rtcReleaseDeviceFun)(RTCDevice device);
typedef RTCScene (*rtcNewSceneFun)(RTCDevice device);
typedef void (*rtcReleaseSceneFun)(RTCScene scene);


typedef RTCBVH (*rtcNewBVHFun)(RTCDevice device);
typedef void* (*rtcBuildBVHFun)(const struct RTCBuildArguments* args);
typedef void* (*rtcThreadLocalAllocFun)(RTCThreadLocalAllocator allocator, size_t bytes, size_t align);
typedef void (*rtcReleaseBVHFun)(RTCBVH bvh);

bool LoadEmbree();
void UnloadEmbree();

extern rtcNewDeviceFun g_rtcNewDevice;
extern rtcReleaseDeviceFun g_rtcReleaseDevice;
extern rtcNewSceneFun g_rtcNewScene;
extern rtcReleaseSceneFun g_rtcReleaseScene;

extern rtcNewBVHFun g_rtcNewBVH;
extern rtcBuildBVHFun g_rtcBuildBVH;
extern rtcThreadLocalAllocFun g_rtcThreadLocalAlloc;
extern rtcReleaseBVHFun g_rtcReleaseBVH;