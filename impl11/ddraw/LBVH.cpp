#include "common.h"
#include "globals.h"
#include "LBVH.h"
#include <stdio.h>
#include <queue>
#include <algorithm>

#include <rtcore.h> //Embree

// Remaining issues:
// - Don't add meshes to the TLAS that come from targeted objects.

// This is the global index where the next inner QBVH node will be written.
// It's used by BuildFastQBVH/FastLQBVH (the single-step version) to create
// the QBVH buffer as the BVH2 is built. It's expected that the algorithm
// will be parallelized and multiple threads will have to update this variable
// atomically, so that's why it's a global variable for now.
static int g_QBVHEncodeNodeIdx = 0;

bool g_bEnableQBVHwSAH = false; // The FastLQBVH builder still has some problems when SAH is enabled

// Used by the DirectBVH builder
//#define DEBUG_BU 1
#undef DEBUG_BU

int g_directBuilderFirstActiveInnerNode = 0;
int g_directBuilderNextNode = 0;
int g_maxDirectBVHIteration = -1;

static HMODULE hEmbree = NULL;
static bool g_bEmbreeLoaded = false;
rtcNewDeviceFun g_rtcNewDevice = nullptr;
rtcReleaseDeviceFun g_rtcReleaseDevice = nullptr;
rtcNewSceneFun g_rtcNewScene = nullptr;
rtcReleaseSceneFun g_rtcReleaseScene = nullptr;

rtcNewBVHFun g_rtcNewBVH = nullptr;
rtcBuildBVHFun g_rtcBuildBVH = nullptr;
rtcThreadLocalAllocFun g_rtcThreadLocalAlloc = nullptr;
rtcReleaseBVHFun g_rtcReleaseBVH = nullptr;

void PrintTreeBuffer(std::string level, BVHNode* buffer, int curNode);
InnerNode* DirectBVH2BuilderGPU(AABB sceneAABB, std::vector<LeafItem>& leafItems, int& root_out);
InnerNode* DirectBVH2BuilderCPU(AABB sceneAABB, AABB centroidBox, std::vector<LeafItem>& leafItems, int& root_out);
BVHNode* DirectBVH4BuilderGPU(AABB centroidBox, std::vector<LeafItem>& leafItems, int& numNodes, const XwaVector3* vertices, const int* indices);
BVHNode* AVLBuilder(std::vector<LeafItem>& leafItems, int& numNodes, const XwaVector3* vertices, const int* indices);

// Load a BVH2, deprecated since the BVH4 are more better
#ifdef DISABLED
LBVH *LBVH::LoadLBVH(char *sFileName, bool verbose) {
	FILE *file;
	errno_t error = 0;
	LBVH *lbvh = nullptr;

	try {
		error = fopen_s(&file, sFileName, "rb");
	}
	catch (...) {
		if (verbose)
			log_debug("[DBG] [BVH] Could not load [%s]", sFileName);
		return lbvh;
	}

	if (error != 0) {
		if (verbose)
			log_debug("[DBG] [BVH] Error %d when loading [%s]", error, sFileName);
		return lbvh;
	}

	try {
		// Read the Magic Word and the version
		{
			char magic[9];
			fread(magic, 1, 8, file);
			magic[8] = 0;
			if (strcmp(magic, "BVH2-1.0") != 0)
			{
				log_debug("[DBG] [BVH] Unknown BVH version. Got: [%s]", magic);
				return lbvh;
			}
		}

		lbvh = new LBVH();

		// Read the vertices. The vertices are in OPT coords. They should match what we see
		// in XwaOptEditor
		{
			int32_t NumVertices = 0;
			fread(&NumVertices, sizeof(int32_t), 1, file);
			lbvh->numVertices = NumVertices;
			lbvh->vertices = new float3[NumVertices];
			int NumItems = fread(lbvh->vertices, sizeof(float3), NumVertices, file);
			if (verbose)
				log_debug("[DBG] [BVH] Read %d vertices from BVH file", NumItems);
			// DEBUG
			/*
			log_debug("[DBG] [BVH] Vertices BEGIN");
			for (int i = 0; i < lbvh->numVertices; i++) {
				float3 V = lbvh->vertices[i];
				log_debug("[DBG] [BVH] %0.6f, %0.6f, %0.6f",
					V.x, V.y, V.z);
			}
			log_debug("[DBG] [BVH] Vertices END");
			*/
			// DEBUG
		}

		// Read the indices
		{
			int32_t NumIndices = 0;
			fread(&NumIndices, sizeof(int32_t), 1, file);
			lbvh->numIndices = NumIndices;
			lbvh->indices = new int32_t[NumIndices];
			int NumItems = fread(lbvh->indices, sizeof(int32_t), NumIndices, file);
			if (verbose)
				log_debug("[DBG] [BVH] Read %d indices from BVH file", NumItems);
		}

		// Read the BVH nodes
		{
			int32_t NumNodes = 0;
			fread(&NumNodes, sizeof(int32_t), 1, file);
			lbvh->numNodes = NumNodes;
			lbvh->nodes = new BVHNode[NumNodes];
			int NumItems = fread(lbvh->nodes, sizeof(BVHNode), NumNodes, file);
			if (verbose)
				log_debug("[DBG] [BVH] Read %d BVH nodes from BVH file", NumItems);
		}

		// Thanks to Jeremy I no longer need to read the mesh AABBs nor their vertex counts,
		// I can read the OPT scale directly off of XWA's heap. So this block is now unnecessary.
		// See D3dRenderer::UpdateMeshBuffers() for details on how to get the OPT scale.
#ifdef DISABLED
		// Read the mesh AABBs
		{
			int32_t NumMeshMinMaxs = 0;
			fread(&NumMeshMinMaxs, sizeof(int32_t), 1, file);
			lbvh->numMeshMinMaxs = NumMeshMinMaxs;
			lbvh->meshMinMaxs = new MinMax[NumMeshMinMaxs];
			int NumItems = fread(lbvh->meshMinMaxs, sizeof(MinMax), NumMeshMinMaxs, file);
			if (verbose)
				log_debug("[DBG] [BVH] Read %d AABBs from BVH file", NumItems);
		}

		// Read the Vertex Counts
		{
			int32_t NumVertexCounts = 0;
			fread(&NumVertexCounts, sizeof(int32_t), 1, file);
			lbvh->numVertexCounts = NumVertexCounts;
			lbvh->vertexCounts = new uint32_t[NumVertexCounts];
			int NumItems = fread(lbvh->vertexCounts, sizeof(uint32_t), NumVertexCounts, file);
			if (verbose)
				log_debug("[DBG] [BVH] Read %d Vertex Counts from BVH file", NumItems);
		}
#endif

		// DEBUG
		// Check some basic properties of the BVH
		if (false) {
			int minTriID = 2000000, maxTriID = -1;
			bool innerNodeComplete = true;

			for (int i = 0; i < lbvh->numNodes; i++) {
				if (lbvh->nodes[i].ref != -1) {
					minTriID = min(minTriID, lbvh->nodes[i].ref);
					maxTriID = max(maxTriID, lbvh->nodes[i].ref);
				}
				else {
					if (lbvh->nodes[i].left == -1 || lbvh->nodes[i].right == -1)
						innerNodeComplete = false;
				}
			}
			log_debug("[DBG] [BVH] minTriID: %d, maxTriID: %d", minTriID, maxTriID);
			log_debug("[DBG] [BVH] innerNodeComplete: %d", innerNodeComplete);

			// Check that all the indices reference existing vertices
			bool indicesRangeOK = true;
			for (int i = 0; i < lbvh->numIndices; i++) {
				if (lbvh->indices[i] < 0 || lbvh->indices[i] >= lbvh->numVertices) {
					log_debug("[DBG] [BVH] Invalid LBVH index: ", lbvh->indices[i]);
					indicesRangeOK = false;
					break;
				}
			}
			log_debug("[DBG] [BVH] indicesRangeOK: %d", indicesRangeOK);
		}
	}
	catch (...) {
		log_debug("[DBG] [BVH] There were errors while reading [%s]", sFileName);
		if (lbvh != nullptr) {
			delete lbvh;
			lbvh = nullptr;
		}
	}

	fclose(file);

	// DEBUG: PrintTree
	{
		//lbvh->PrintTree("", 0);
	}
	// DEBUG

	return lbvh;
}

void LBVH::PrintTree(std::string level, int curnode)
{
	if (curnode == -1)
		return;
	PrintTree(level + "    ", nodes[curnode].right);
	log_debug("[DBG] [BVH] %s%d", level.c_str(), nodes[curnode].ref);
	PrintTree(level + "    ", nodes[curnode].left);
}
#endif

int CalcNumInnerQBVHNodes(int numPrimitives)
{
	return max(1, (int)(0.667f * numPrimitives));
}

bool leafSorter(const LeafItem& i, const LeafItem& j)
{
	return i.code < j.code;
}

bool tlasLeafSorter(const TLASLeafItem& i, const TLASLeafItem& j)
{
	return i.code < j.code;
}

// Load a BVH4
LBVH *LBVH::LoadLBVH(char *sFileName, bool EmbeddedVerts, bool verbose) {
	FILE *file;
	errno_t error = 0;
	LBVH *lbvh = nullptr;

	try {
		error = fopen_s(&file, sFileName, "rb");
	}
	catch (...) {
		if (verbose)
			log_debug("[DBG] [BVH] Could not load [%s]", sFileName);
		return lbvh;
	}

	if (error != 0) {
		if (verbose)
			log_debug("[DBG] [BVH] Error %d when loading [%s]", error, sFileName);
		return lbvh;
	}

	try {
		// Read the Magic Word and the version
		{
			char magic[9];
			fread(magic, 1, 8, file);
			magic[8] = 0;
			if (strcmp(magic, "BVH4-1.0") != 0)
			{
				log_debug("[DBG] [BVH] Unknown BVH version. Got: [%s]", magic);
				return lbvh;
			}
		}

		lbvh = new LBVH();

		// Read Vertices and Indices when the geometry is not embedded
		if (!EmbeddedVerts) {
			// Read the vertices. The vertices are in OPT coords. They should match what we see
			// in XwaOptEditor
			{
				int32_t NumVertices = 0;
				fread(&NumVertices, sizeof(int32_t), 1, file);
				lbvh->numVertices = NumVertices;
				lbvh->vertices = new float3[NumVertices];
				int NumItems = fread(lbvh->vertices, sizeof(float3), NumVertices, file);
				if (verbose)
					log_debug("[DBG] [BVH] Read %d vertices from BVH file", NumItems);
			}

			// Read the indices
			{
				int32_t NumIndices = 0;
				fread(&NumIndices, sizeof(int32_t), 1, file);
				lbvh->numIndices = NumIndices;
				lbvh->indices = new int32_t[NumIndices];
				int NumItems = fread(lbvh->indices, sizeof(int32_t), NumIndices, file);
				if (verbose)
					log_debug("[DBG] [BVH] Read %d indices from BVH file", NumItems);
			}
		}

		// Read the BVH nodes
		{
			int32_t NumNodes = 0;
			fread(&NumNodes, sizeof(int32_t), 1, file);
			lbvh->numNodes = NumNodes;
			lbvh->nodes = new BVHNode[NumNodes];
			int NumItems = fread(lbvh->nodes, sizeof(BVHNode), NumNodes, file);
			if (verbose)
				log_debug("[DBG] [BVH] Read %d BVH nodes from BVH file", NumItems);
		}

		// DEBUG
		// Check some basic properties of the BVH
		if (false) {
			int minTriID = 2000000, maxTriID = -1;
			int minChildren = 10, maxChildren = -1;

			for (int i = 0; i < lbvh->numNodes; i++) {
				if (lbvh->nodes[i].ref != -1) {
					// Leaf node
					minTriID = min(minTriID, lbvh->nodes[i].ref);
					maxTriID = max(maxTriID, lbvh->nodes[i].ref);
				}
				else {
					// Inner node
					int numChildren = 0;
					for (int j = 0; j < 4; j++)
						numChildren += (lbvh->nodes[i].children[j] != -1) ? 1 : 0;
					if (numChildren < minChildren) minChildren = numChildren;
					if (numChildren > maxChildren) maxChildren = numChildren;
				}
			}
			log_debug("[DBG] [BVH] minTriID: %d, maxTriID: %d", minTriID, maxTriID);
			log_debug("[DBG] [BVH] min,maxChildren: %d, %d", minChildren, maxChildren);

			// Check that all the indices reference existing vertices
			if (!EmbeddedVerts) {
				bool indicesRangeOK = true;
				for (int i = 0; i < lbvh->numIndices; i++) {
					if (lbvh->indices[i] < 0 || lbvh->indices[i] >= lbvh->numVertices) {
						log_debug("[DBG] [BVH] Invalid LBVH index: ", lbvh->indices[i]);
						indicesRangeOK = false;
						break;
					}
				}
				log_debug("[DBG] [BVH] indicesRangeOK: %d", indicesRangeOK);
			}
			else {
				// Embedded Verts, dump an OBJ for debugging purposes
				//lbvh->DumpToOBJ("C:\\Temp\\BVHEmbedded.obj");
			}
		}
	}
	catch (...) {
		log_debug("[DBG] [BVH] There were errors while reading [%s]", sFileName);
		if (lbvh != nullptr) {
			delete lbvh;
			lbvh = nullptr;
		}
	}

	fclose(file);
	return lbvh;
}

void LBVH::PrintTree(std::string level, int curnode)
{
	if (curnode == -1)
		return;
	log_debug("[DBG] [BVH] %s", (level + "    --").c_str());
	for (int i = 3; i >= 2; i--)
		if (nodes[curnode].children[i] != -1)
			PrintTree(level + "    ", nodes[curnode].children[i]);

	log_debug("[DBG] [BVH] %s%d", level.c_str(), nodes[curnode].ref);

	for (int i = 1; i >= 0; i--)
		if (nodes[curnode].children[i] != -1)
			PrintTree(level + "    ", nodes[curnode].children[i]);
	log_debug("[DBG] [BVH] %s", (level + "    --").c_str());
}

void LBVH::DumpToOBJ(char *sFileName, bool isTLAS, bool useMetricScale)
{
	BVHPrimNode *primNodes = (BVHPrimNode *)nodes;
	FILE *file = NULL;
	int index = 1;
	float scale[3] = { 1.0f, 1.0f, 1.0f };
	if (useMetricScale)
	{
		scale[0] =  OPT_TO_METERS;
		scale[1] = -OPT_TO_METERS;
		scale[2] =  OPT_TO_METERS;
	}

	fopen_s(&file, sFileName, "wt");
	if (file == NULL) {
		log_debug("[DBG] [BVH] Could not open file: %s", sFileName);
		return;
	}

	int root = nodes[0].rootIdx;
	log_debug("[DBG] [BVH] Dumping %d nodes to OBJ", numNodes - root);
	for (int i = root; i < numNodes; i++) {
		if (nodes[i].ref != -1)
		{
			if (isTLAS)
			{
				BVHTLASLeafNode *node = (BVHTLASLeafNode *)&(nodes[i]);
				fprintf(file, "o tleaf-%d\n", i);

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
			}
			else
			{
				//BVHPrimNode node = primNodes[i];
				BVHNode node = nodes[i];
				// Leaf node, let's dump the embedded vertices
				fprintf(file, "o leaf-%d\n", i);
				Vector3 v0, v1, v2;
				v0.x = node.min[0];
				v0.y = node.min[1];
				v0.z = node.min[2];

				v1.x = node.max[0];
				v1.y = node.max[1];
				v1.z = node.max[2];

				v2.x = *(float*)&(node.children[0]);
				v2.y = *(float*)&(node.children[1]);
				v2.z = *(float*)&(node.children[2]);

				fprintf(file, "v %f %f %f\n", v0.x * scale[0], v0.y * scale[1], v0.z * scale[2]);
				fprintf(file, "v %f %f %f\n", v1.x * scale[0], v1.y * scale[1], v1.z * scale[2]);
				fprintf(file, "v %f %f %f\n", v2.x * scale[0], v2.y * scale[1], v2.z * scale[2]);

				fprintf(file, "f %d %d %d\n", index, index + 1, index + 2);
				index += 3;
			}
		}
		else {
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

static int firstbithigh(uint32_t X)
{
	int pos = 31;
	uint32_t mask = 0x1;
	while (pos >= 0)
	{
		if ((X & (mask << pos)) != 0x0)
			return pos;
		pos--;
	}
	return pos;
}

static int delta32(uint32_t X, uint32_t Y)
{
	if (X == Y)
		return -1;
	return firstbithigh(X ^ Y);
}


#ifdef MORTON_CODE_30
static uint32_t SpreadBits(uint32_t x, int offset)
{
	if ((x < 0) || (x > 1023))
	{
		return -1;
	}

	if ((offset < 0) || (offset > 2))
	{
		return -1;
	}

	x = (x | (x << 10)) & 0x000F801F;
	x = (x | (x <<  4)) & 0x00E181C3;
	x = (x | (x <<  2)) & 0x03248649;
	x = (x | (x <<  2)) & 0x09249249;

	return x << offset;
}
#else
static uint64_t SpreadBits(uint64_t x, int offset)
{
	x = (x | (x << 20)) & 0x000001FFC00003FF;
	x = (x | (x << 10)) & 0x0007E007C00F801F;
	x = (x | (x <<  4)) & 0x00786070C0E181C3;
	x = (x | (x <<  2)) & 0x0199219243248649;
	x = (x | (x <<  2)) & 0x0649249249249249;
	x = (x | (x <<  2)) & 0x1249249249249249;
	return x << offset;
}
#endif

// From https://stackoverflow.com/questions/1024754/how-to-compute-a-3d-morton-number-interleave-the-bits-of-3-ints
MortonCode_t GetMortonCode(uint32_t x, uint32_t y, uint32_t z)
{
	return SpreadBits(x, 2) | SpreadBits(y, 1) | SpreadBits(z, 0);
}

#ifdef MORTON_CODE_30
MortonCode_t GetMortonCode(const Vector3 &V)
{
	constexpr float k = 1023.0f; // 0x3FF
	uint32_t x = (uint32_t)(V.x * k);
	uint32_t y = (uint32_t)(V.y * k);
	uint32_t z = (uint32_t)(V.z * k);
	return GetMortonCode(x, y, z);
}
#else
MortonCode_t GetMortonCode(const Vector3& V)
{
	constexpr float k = 2097151.0f; // 0x1FFFFF
	uint32_t x = (uint32_t)(V.x * k);
	uint32_t y = (uint32_t)(V.y * k);
	uint32_t z = (uint32_t)(V.z * k);
	return GetMortonCode(x, y, z);
}
#endif

// This is the delta used in Apetrei 2014
template<class T>
static MortonCode_t delta(const std::vector<T> &leafItems, int i)
{
	MortonCode_t mi = leafItems[i].code;
	MortonCode_t mj = leafItems[i + 1].code;
	return (mi == mj) ? i ^ (i + 1) : mi ^ mj;
}

int ChooseParent(int curNode, bool isLeaf, int numLeaves, const std::vector<LeafItem> &leafItems, InnerNode *innerNodes)
{
	int parent = -1;
	int left, right;
	AABB curAABB;

	if (isLeaf)
	{
		left = curNode;
		right = curNode;
		curAABB = leafItems[curNode].aabb;
	}
	else
	{
		left = innerNodes[curNode].first;
		right = innerNodes[curNode].last;
		curAABB = innerNodes[curNode].aabb;
	}

	/*
	log_debug("[DBG] [BVH] %s, curNode: %d, left,right: [%d,%d], d(right): %d, d(left - 1): %d",
		isLeaf ? "LEAF" : "INNER", curNode, left, right,
		right < numLeaves - 1 ? delta(leafItems, right) : -1,
		left - 1 >= 0 ? delta(leafItems, left - 1) : -1);
	*/
	if (left == 0 || (right != (numLeaves - 1) && delta(leafItems, right) < delta(leafItems, left - 1)))
	{
		parent = right;
		if (parent == numLeaves - 1)
		{
			// We have found the root
			return curNode;
		}
		innerNodes[parent].left = curNode;
		innerNodes[parent].leftIsLeaf = isLeaf;
		innerNodes[parent].first = left;
		innerNodes[parent].readyCount++;
		innerNodes[parent].aabb.Expand(curAABB);
		//log_debug("[DBG] [BVH]    case 1, parent: %d, parent.left: %d, parent.first: %d, readyCount: %d",
		//	parent, innerNodes[parent].left, innerNodes[parent].first, innerNodes[parent].readyCount);
	}
	else
	{
		parent = left - 1;
		innerNodes[parent].right = curNode;
		innerNodes[parent].rightIsLeaf = isLeaf;
		innerNodes[parent].last = right;
		innerNodes[parent].readyCount++;
		innerNodes[parent].aabb.Expand(curAABB);
		//log_debug("[DBG] [BVH]    case 2, parent: %d, parent.right: %d, parent.last: %d, readyCount: %d",
		//	parent, innerNodes[parent].right, innerNodes[parent].last, innerNodes[parent].readyCount);
	}
	return -1;
}

/// <summary>
/// Standard LBVH2 builder.
/// </summary>
/// <param name="leafItems"></param>
/// <param name="root"></param>
/// <returns></returns>
InnerNode *FastLBVH(const std::vector<LeafItem> &leafItems, int *root)
{
	int numLeaves = leafItems.size();
	*root = -1;
	//log_debug("[DBG] [BVH] numLeaves: %d", numLeaves);
	if (numLeaves <= 1)
		// Nothing to do, the single leaf is the root
		return nullptr;

	// Initialize the inner nodes
	int numInnerNodes = numLeaves - 1;
	int innerNodesProcessed = 0;
	InnerNode* innerNodes = new InnerNode[numInnerNodes];
	for (int i = 0; i < numInnerNodes; i++) {
		innerNodes[i].readyCount = 0;
		innerNodes[i].processed = false;
		innerNodes[i].aabb.SetInfinity();
	}

	// Start the tree by iterating over the leaves
	//log_debug("[DBG] [BVH] Adding leaves to BVH");
	for (int i = 0; i < numLeaves; i++)
		ChooseParent(i, true, numLeaves, leafItems, innerNodes);

	// Build the tree
	while (*root == -1 && innerNodesProcessed < numInnerNodes)
	{
		//log_debug("[DBG] [BVH] ********** Inner node iteration");
		for (int i = 0; i < numInnerNodes; i++) {
			if (!innerNodes[i].processed && innerNodes[i].readyCount == 2)
			{
				*root = ChooseParent(i, false, numLeaves, leafItems, innerNodes);
				innerNodes[i].processed = true;
				innerNodesProcessed++;
				if (*root != -1)
					break;
			}
		}
	}
	//log_debug("[DBG] [BVH] root at index: %d", *root);
	return innerNodes;
}

/// <summary>
/// Implements the Fast LBVH choose parent algorithm for QBVH nodes.
/// </summary>
/// <param name="curNode"></param>
/// <param name="isLeaf"></param>
/// <param name="numLeaves"></param>
/// <param name="leafItems"></param>
/// <param name="innerNodes"></param>
/// <returns>The root index if it was found, or -1 otherwise.</returns>
template<class T>
int ChooseParent4(int curNode, bool isLeaf, int numLeaves, const std::vector<T>& leafItems, InnerNode4* innerNodes)
{
	int parent = -1;
	int totalNodes = 0;
	int left, right;
	AABB curAABB;

	if (isLeaf)
	{
		left = curNode;
		right = curNode;
		curAABB = leafItems[curNode].aabb;
		totalNodes = 1;
	}
	else
	{
		left = innerNodes[curNode].first;
		right = innerNodes[curNode].last;
		curAABB = innerNodes[curNode].aabb;
		totalNodes = innerNodes[curNode].totalNodes;
	}

	if (left == 0 || (right != (numLeaves - 1) && delta(leafItems, right) < delta(leafItems, left - 1)))
	{
		parent = right;
		if (parent == numLeaves - 1)
		{
			// We have found the root
			return curNode;
		}
		innerNodes[parent].children[0] = curNode;
		innerNodes[parent].isLeaf[0] = isLeaf;
		innerNodes[parent].first = left;
	}
	else
	{
		parent = left - 1;
		innerNodes[parent].children[1] = curNode;
		innerNodes[parent].isLeaf[1] = isLeaf;
		innerNodes[parent].last = right;
		//log_debug("[DBG] [BVH]    case 2, parent: %d, parent.right: %d, parent.last: %d, readyCount: %d",
		//	parent, innerNodes[parent].right, innerNodes[parent].last, innerNodes[parent].readyCount);
	}
	innerNodes[parent].readyCount++;
	innerNodes[parent].aabb.Expand(curAABB);
	innerNodes[parent].numChildren++;
	innerNodes[parent].totalNodes += totalNodes;
	return -1;
}

/// <summary>
/// Converts a BVH2 node into a BVH4 node.
/// </summary>
/// <param name="innerNodes">The inner node list</param>
/// <param name="i">The current node to convert</param>
void ConvertToBVH4Node(InnerNode4 *innerNodes, int i)
{
	InnerNode4 node = innerNodes[i];

	// To be 100% correct, we need to iterate over all the node.numChildren and count
	// the grandchildren, not just the first two (see below). Here we're assuming that
	// this function will only be called at most once per node, and that only inner nodes
	// will go through here, so there will be exactly two nodes. Always. However, the
	// single-step builder may call this function twice for the root in some cases (to make
	// sure the root is converted to a QBVH node). So let's make sure this node has at most
	// two nodes.
	if (node.numChildren == 1)
	{
		//log_debug("[DBG] [BVH] node %d only has one child. Skipping", i);
		return;
	}

	if (node.numChildren > 2)
	{
		//log_debug("[DBG] [BVH] node %d has already been converted. Skipping", i);
		return;
	}

	// Count all the grandchildren.
	int numGrandChildren = 0;
	numGrandChildren += (node.isLeaf[0]) ? 1 : innerNodes[node.children[0]].numChildren;
	numGrandChildren += (node.isLeaf[1]) ? 1 : innerNodes[node.children[1]].numChildren;

	//log_debug("[DBG] [BVH] Attempting BVH4 conversion for node: %d, numChildren: %d, numGrandChildren: %d",
	//	i, node.numChildren, numGrandChildren);

	if (3 <= numGrandChildren && numGrandChildren <= 4)
	{
		// This node can be collapsed
		int numChild = 0;
		int totalNodes = node.totalNodes;
		InnerNode4 tmp = node;
		int arity = node.numChildren;
		//log_debug("[DBG] [BVH] Collapsing node of arity: %d", arity);
		for (int k = 0; k < arity; k++)
		{
			//log_debug("[DBG] [BVH]   child: %d, isLeaf: %d", k, node.isLeaf[k]);
			if (node.isLeaf[k])
			{
				// No grandchildren, copy immediate child
				tmp.children[numChild] = node.children[k];
				tmp.isLeaf[numChild]   = true;
				numChild++;
			}
			else
			{
				// Pull up grandchildren
				const int child = node.children[k];
				const int numChildren = innerNodes[child].numChildren;
				for (int j = 0; j < numChildren; j++)
				{
					//log_debug("[DBG] [BVH]      gchild: %d, node: %d, isLeaf: %d",
					//	j, innerNodes[child].children[j], innerNodes[child].isLeaf[j]);
					tmp.children[numChild] = innerNodes[child].children[j];
					tmp.isLeaf[numChild]   = innerNodes[child].isLeaf[j];
					tmp.QBVHOfs[numChild]  = innerNodes[child].QBVHOfs[j];

					numChild++;
				}
				// Disable the node that is now empty
				innerNodes[child].numChildren = 0;
				totalNodes--;
			}
		}
		tmp.numChildren = numChild;
		tmp.totalNodes = totalNodes;
		// Replace the original node
		innerNodes[i] = tmp;
		//log_debug("[DBG] [BVH] converted node %d to BVH4 node, numChildren: %d",
		//	i, tmp.numChildren);
	}
}

/// <summary>
/// Convert a BVH2 node into a BVH4 node, using SAH. Uses BVH4 nodes as input.
/// </summary>
/// <param name="innerNodes"></param>
/// <param name="curNodeIdx"></param>
/// <param name="leafItems"></param>
/// <param name="numQBVHInnerNodes"></param>
template<class T>
void ConvertToBVH4NodeSAH(InnerNode4* innerNodes, int curNodeIdx, const int numQBVHInnerNodes, const std::vector<T>& leafItems)
{
	InnerNode4 node = innerNodes[curNodeIdx];

	int numGrandChildren = 0;
	// Count all the grandchildren
	numGrandChildren += (node.isLeaf[0]) ? 1 : innerNodes[node.children[0]].numChildren;
	numGrandChildren += (node.isLeaf[1]) ? 1 : innerNodes[node.children[1]].numChildren;

	//log_debug("[DBG] [BVH] Attempting BVH4 conversion for node: %d, numChildren: %d, numGrandChildren: %d",
	//	curNodeIdx, node.numChildren, numGrandChildren);

	// 2 grandchildren, nothing to do
	if (numGrandChildren <= 2 || numGrandChildren == 8)
		return;

	// 3..8 grandchildren: let's pull up the nodes with the largest SAH and keep inner nodes
	// for the rest. Here we're making the assumption that the current node's children are
	// already collapsed, so we do not need to look further than the grandchildren.
	if (3 <= numGrandChildren && numGrandChildren <= 4)
	{
		// If we have 3 or 4 grandchildren, we can just pull them all up
		int numChild = 0;
		int totalNodes = node.totalNodes;
		InnerNode4 tmp = node;
		int arity = node.numChildren;
		//log_debug("[DBG] [BVH] Collapsing node of arity: %d", arity);
		for (int k = 0; k < arity; k++)
		{
			//log_debug("[DBG] [BVH]   child: %d, isLeaf: %d", k, node.isLeaf[k]);
			if (node.isLeaf[k])
			{
				// No grandchildren, copy immediate child
				tmp.children[numChild] = node.children[k];
				tmp.isLeaf[numChild] = true;
				numChild++;
			}
			else
			{
				// Pull up grandchildren
				const int child = node.children[k];
				const int numChildren = innerNodes[child].numChildren;
				for (int j = 0; j < numChildren; j++)
				{
					//log_debug("[DBG] [BVH]      gchild: %d, node: %d, isLeaf: %d",
					//	j, innerNodes[child].children[j], innerNodes[child].isLeaf[j]);
					tmp.children[numChild] = innerNodes[child].children[j];
					tmp.isLeaf[numChild] = innerNodes[child].isLeaf[j];
					tmp.QBVHOfs[numChild] = innerNodes[child].QBVHOfs[j];

					numChild++;
				}
				// Disable the node that is now empty
				innerNodes[child].numChildren = 0;
				totalNodes--;
			}
		}
		tmp.numChildren = numChild;
		tmp.totalNodes = totalNodes;
		// Replace the original node
		innerNodes[curNodeIdx] = tmp;
		//log_debug("[DBG] [BVH] converted node %d to BVH4 node, numChildren: %d",
		//	i, tmp.numChildren);
	}
	else if (5 <= numGrandChildren && numGrandChildren <= 7) // Applying SAH to 8 grandchildren actually appeared to drop FPS
	{
		// Having at least 5 grandchildren implies we have two inner nodes
		// If we have 5..8 grandchildren, then we can do the following:
		// 5 grandchildren: pull 3 up, keep one inner node and put 2 grandchildren there
		// 6 grandchildren: pull 3 up, keep one inner node and put 3 grandchildren there
		// 7 grandchildren: pull 3 up, keep one inner node and put 4 grandchildren there
		// 8 grandchildren: pull 2 up, keep two inner nodes and put 3,3 grandchildren on each inner node
		//		This case needs to be implemented judiciously. We need to pull up the largest node from the left and the right.
		const bool use2InnerNodes     = (numGrandChildren == 8);
		const int numChildrenToPullUp = use2InnerNodes ? 2 : 3;
		std::vector<int> availableInnerNodes;
		int c0 = node.children[0];
		int c1 = node.children[1];
		bool c0IsLeaf = node.isLeaf[0];
		bool c1IsLeaf = node.isLeaf[1];

		if (!c0IsLeaf) availableInnerNodes.push_back(c0);
		if (!c1IsLeaf) availableInnerNodes.push_back(c1);

		//if (c0IsLeaf) log_debug("[DBG] [BVH] c0IsLeaf, curNode: %d, numGrandChildren: %d",
		//	curNodeIdx, numGrandChildren);
		//if (c1IsLeaf) log_debug("[DBG] [BVH] c1IsLeaf, curNode: %d, numGrandChildren: %d",
		//	curNodeIdx, numGrandChildren);

		/*
		log_debug("[DBG] [BVH] Case 2, c0: %d, c1: %d, use2InnerNodes: %d, c0.numChildren: %d, c1.numChildren: %d",
			c0, c1, use2InnerNodes, innerNodes[c0].numChildren, innerNodes[c1].numChildren);
		log_debug("[DBG] [BVH] innerNodeStartIdx: %d, innerNodeEndIdx: %d", innerNodeStartIdx, innerNodeEndIdx);
		*/

		// Collect all the grandchildren
		std::vector<EncodeItem> gchildren;
		if (c0IsLeaf)
		{
			gchildren.push_back(EncodeItem(c0, true, c0 + numQBVHInnerNodes));
		}
		else
			for (int i = 0; i < innerNodes[c0].numChildren; i++) {
				gchildren.push_back(EncodeItem(
					innerNodes[c0].children[i],
					innerNodes[c0].isLeaf[i],
					innerNodes[c0].QBVHOfs[i]));
			}

		if (c1IsLeaf) {
			gchildren.push_back(EncodeItem(c1, true, c1 + numQBVHInnerNodes));
		}
		else
			for (int i = 0; i < innerNodes[c1].numChildren; i++) {
				gchildren.push_back(EncodeItem(
					innerNodes[c1].children[i],
					innerNodes[c1].isLeaf[i],
					innerNodes[c1].QBVHOfs[i]));
			}
		/*
		log_debug("[DBG] [BVH] gchildren.size: %d", gchildren.size());
		for (uint32_t i = 0; i < gchildren.size(); i++)
		{
			log_debug("[DBG] [BVH] gchildren[%d]: %d, isLeaf: %d",
				i, std::get<0>(gchildren[i]), std::get<1>(gchildren[i]));
		}
		*/

		// Find the N grandchildren with the largest SAH (these will be pulled up)
		bool picked[8] = { false };
		std::vector<EncodeItem> pullup;
		for (int j = 0; j < numChildrenToPullUp; j++)
		{
			float maxArea  = 0.0f;
			int maxAreaIdx = -1;
			// Find the node with the max area
			for (uint32_t i = 0; i < gchildren.size(); i++)
			{
				if (!picked[i])
				{
					int  gchild = std::get<0>(gchildren[i]);
					bool isLeaf = std::get<1>(gchildren[i]);
					AABB box;
					if (isLeaf)
						box = std::get<1>(leafItems[gchild]);
					else
						box = innerNodes[gchild].aabb;
					float area = box.GetArea();
					if (area >= maxArea)
					{
						maxArea    = area;
						maxAreaIdx = i;
					}
				}
			}

			// Add the node with the max area to the pullup vector
			if (maxAreaIdx != -1) {
				const EncodeItem &item = gchildren[maxAreaIdx];
				picked[maxAreaIdx] = true;
				pullup.push_back(EncodeItem(
					std::get<0>(item),
					std::get<1>(item),
					std::get<2>(item)));
			}
		}
		/*
		log_debug("[DBG] [BVH] pullup.size: %d", pullup.size());
		for (uint32_t i = 0; i < pullup.size(); i++)
		{
			log_debug("[DBG] [BVH] pullup[%d]: %d, isLeaf: %d",
				i, std::get<0>(pullup[i]), std::get<1>(pullup[i]));
		}
		*/

		// pullup now has the N grandchildren with the largest SAH, link them to the
		// new node and add inner nodes for the rest
		InnerNode4 tmp = node;
		tmp.numChildren = 4;
		uint32_t nextChild = 0;
		while (nextChild < pullup.size()) {
			const EncodeItem &item = pullup[nextChild];
			tmp.children[nextChild] = std::get<0>(item);
			tmp.isLeaf[nextChild]   = std::get<1>(item);
			tmp.QBVHOfs[nextChild]  = std::get<2>(item);
			nextChild++;
		}

		// Clear and reset the old inner nodes
		for (const int& nodeIdx : availableInnerNodes)
		{
			innerNodes[nodeIdx].numChildren = 0;
			innerNodes[nodeIdx].aabb.SetInfinity();
			for (int i = 0; i < 4; i++)
				innerNodes[nodeIdx].children[i] = -1;
		}

		// Link the remaining children through inner nodes
		tmp.children[nextChild] = availableInnerNodes[0];
		tmp.isLeaf[nextChild]   = false;
		if (nextChild < 3 && availableInnerNodes.size() < 1)
			log_debug("[DBG] [BVH] ERROR: nextChild: %d, expected more available inner nodes (got %d)",
				nextChild, availableInnerNodes.size());
		if (nextChild < 3 && availableInnerNodes.size() > 1)
		{
			tmp.children[nextChild + 1] = availableInnerNodes[1];
			tmp.isLeaf[nextChild + 1]   = false;
		}
		// Add the unpicked nodes to the inner nodes
		// TODO: For the 8 grandchildren case, balance the remaining 6 nodes
		//       between the two inner nodes (3,3)
		int child = tmp.children[nextChild];
		int numGrandChild = 0;
		AABB childBox;
		for (uint32_t i = 0; i < gchildren.size(); i++)
			if (!picked[i]) {
				const EncodeItem &item = gchildren[i];
				int  childIdx     = std::get<0>(item);
				bool childIsLeaf  = std::get<1>(item);
				int  childQBVHOfs = std::get<2>(item);
				innerNodes[child].children[numGrandChild] = childIdx;
				innerNodes[child].isLeaf[numGrandChild]   = childIsLeaf;
				innerNodes[child].QBVHOfs[numGrandChild]  = childQBVHOfs;
				if (childIsLeaf)
					childBox = std::get<1>(leafItems[childIdx]);
				else
					childBox = innerNodes[childIdx].aabb;
				innerNodes[child].aabb.Expand(childBox);
				innerNodes[child].numChildren++;
				//log_debug("[DBG] [BVH] child: %d, gchild: %d, isLeaf: %d, numChildren: %d",
				//	child, innerNodes[child].children[numGrandChild], innerNodes[child].isLeaf[numGrandChild],
				//	innerNodes[child].numChildren);
				numGrandChild++;
				if (numGrandChild >= 4)
				{
					nextChild++;
					child = tmp.children[nextChild];
					numGrandChild = 0;
				}
			}
		//log_debug("[DBG] [BVH] c0.numChildren: %d, c1.numChildren: %d",
		//	innerNodes[c0].numChildren, innerNodes[c1].numChildren);

		/*
		if (innerNodes[c0].numChildren < 2)
		{
			log_debug("[DBG] [BVH] ERROR: innerNodes[%d].numChildren: %d",
				c0, innerNodes[c0].numChildren);
			log_debug("[DBG] [BVH] curNodeIdx: %d", curNodeIdx);
		}
		*/

		/*
		if (use2InnerNodes && innerNodes[c1].numChildren < 2)
		{
			log_debug("[DBG] [BVH] ERROR: innerNodes[%d].numChildren: %d",
				c1, innerNodes[c1].numChildren);
			log_debug("[DBG] [BVH] curNodeIdx: %d", curNodeIdx);
		}
		*/

		// Replace the original node
		innerNodes[curNodeIdx] = tmp;
	}
}

int FindChildWithMaxArea(int curNode, InnerNode4* innerNodes, const std::vector<LeafItem>& leafItems, bool checkLeaves)
{
	uint32_t numChildren = innerNodes[curNode].numChildren;
	float    maxArea     = -1.0f;
	int      maxAreaIdx  = -1;

	for (uint32_t i = 0; i < numChildren; i++)
	{
		int  child       = innerNodes[curNode].children[i];
		bool childIsLeaf = innerNodes[curNode].isLeaf[i];
		AABB box;

		if (!checkLeaves && childIsLeaf)
			continue;

		if (childIsLeaf)
			box = leafItems[child].aabb;
		else
			box = innerNodes[child].aabb;

		float area = box.GetArea();
		if (area > maxArea) {
			maxArea    = area;
			maxAreaIdx = i;
		}
	}

	return maxAreaIdx;
}

void RemoveChildIdx(int curNode, int childIdx, InnerNode4* innerNodes)
{
	int child = innerNodes[curNode].children[childIdx];
	//if (innerNodes[child].isEncoded)
	//	log_debug("[DBG] [BVH] WARNING: Removing an encoded node: %d", child);
	//if (innerNodes[curNode].isEncoded)
	//	log_debug("[DBG] [BVH] WARNING: Removing child of an encoded node: %d", curNode);

	for (int i = childIdx; i < 3 /* arity - 1 */; i++)
	{
		innerNodes[curNode].children[i] = innerNodes[curNode].children[i + 1];
		innerNodes[curNode].isLeaf[i]   = innerNodes[curNode].isLeaf[i + 1];
		innerNodes[curNode].QBVHOfs[i]  = innerNodes[curNode].QBVHOfs[i + 1];
	}
	// We just removed one child
	innerNodes[curNode].numChildren--;
	for (int i = innerNodes[curNode].numChildren; i < 4; i++)
		innerNodes[curNode].children[i] = -1;
}

AABB CalcInnerNodeAABB(int curNode, InnerNode4* innerNodes, const std::vector<LeafItem>& leafItems)
{
	uint32_t numChildren = innerNodes[curNode].numChildren;
	innerNodes[curNode].aabb.SetInfinity();
	for (uint32_t i = 0; i < numChildren; i++)
	{
		int  child       = innerNodes[curNode].children[i];
		bool childIsLeaf = innerNodes[curNode].isLeaf[i];
		AABB box;

		if (childIsLeaf)
			box = leafItems[child].aabb;
		else
			box = innerNodes[child].aabb;
		innerNodes[curNode].aabb.Expand(box);
	}

	return innerNodes[curNode].aabb;
}

/// <summary>
/// Converts a BVH2 node into a BVH4 node using SAH. Use this with the
/// single-step FastLBVH builder (notice how it uses InnerNode4 nodes).
/// Only call this for inner nodes.
/// </summary>
void ConvertToBVH4NodeSAH2(int curNode, InnerNode4* innerNodes, const std::vector<LeafItem>& leafItems)
{
	std::vector<EncodeItem> items;
	std::vector<EncodeItem> result;
	uint32_t numChildren = innerNodes[curNode].numChildren;
	uint32_t fill = 4 - numChildren;

	while (fill)
	{
		// Open the node with the largest area, ignore leaves
		int maxAreaIdx = FindChildWithMaxArea(curNode, innerNodes, leafItems, false);
		// Nothing can be pulled up (all children are leaves):
		if (maxAreaIdx < 0)
			break;

		// child can't be a leaf
		int child = innerNodes[curNode].children[maxAreaIdx];

		// Expand/pullup the node with the maximum area. We're only
		// expanding inner nodes, so we can assume there's at least
		// two children on the node to be expanded.
		int numGrandChildren = innerNodes[child].numChildren;
		if (numGrandChildren == 2)
		{
			// Make space for the nodes we'll pullup
			RemoveChildIdx(curNode, maxAreaIdx, innerNodes);
			uint32_t numChildren = innerNodes[curNode].numChildren;

			int nextChild = numChildren;
			// Pull up the two grandchildren
			for (int i = 0; i < numGrandChildren; i++, nextChild++) {
				innerNodes[curNode].children[nextChild] = innerNodes[child].children[i];
				innerNodes[curNode].isLeaf[nextChild]   = innerNodes[child].isLeaf[i];
				innerNodes[curNode].QBVHOfs[nextChild]  = innerNodes[child].QBVHOfs[i];
			}
			// We just added two children
			innerNodes[curNode].numChildren += 2;
			// Cancel the node we just pulled up
			innerNodes[child].numChildren = 0;
			for (int i = 0; i < 4; i++)
				innerNodes[child].children[i] = -1;
			// Recompute the AABB for the current node
			CalcInnerNodeAABB(curNode, innerNodes, leafItems);
		}
		else if (numGrandChildren >= 3)
		{
			// Pull up one grandchild (the one with the max area)
			int nextChild = innerNodes[curNode].numChildren;
			int maxAreaGChildIdx = FindChildWithMaxArea(child, innerNodes, leafItems, true);
			if (maxAreaGChildIdx >= 0 && maxAreaGChildIdx < numGrandChildren)
			{
				int  GChild        = innerNodes[child].children[maxAreaGChildIdx];
				bool GChildIsLeaf  = innerNodes[child].isLeaf[maxAreaGChildIdx];
				int  GChildQBVHOfs = innerNodes[child].QBVHOfs[maxAreaGChildIdx];
				innerNodes[curNode].children[nextChild] = GChild;
				innerNodes[curNode].isLeaf[nextChild]   = GChildIsLeaf;
				innerNodes[curNode].QBVHOfs[nextChild]  = GChildQBVHOfs;
				innerNodes[curNode].numChildren++;
				// Remove the grandchild from the child
				RemoveChildIdx(child, maxAreaGChildIdx, innerNodes);
				// Recompute the AABB for the child node
				CalcInnerNodeAABB(child, innerNodes, leafItems);
			}
		}

		int newfill = 4 - innerNodes[curNode].numChildren;
		// Break if nothing could be pulled up.
		if (fill == newfill)
			break;
		fill = newfill;
	}
}

/// <summary>
/// Two-step QBVH builder. Build the BVH2 and convert it into BVH4, but encode later.
/// </summary>
/// <param name="level"></param>
/// <param name="T"></param>
InnerNode4* FastLQBVH(const std::vector<LeafItem>& leafItems, int &root_out)
{
	int numLeaves = leafItems.size();
	root_out = -1;
	if (numLeaves <= 1)
		// Nothing to do, the single leaf is the root
		return nullptr;

	// Initialize the inner nodes
	int numInnerNodes = numLeaves - 1;
	int innerNodesProcessed = 0;
	InnerNode4* innerNodes = new InnerNode4[numInnerNodes];
	for (int i = 0; i < numInnerNodes; i++) {
		innerNodes[i].readyCount = 0;
		innerNodes[i].processed = false;
		innerNodes[i].aabb.SetInfinity();
		innerNodes[i].numChildren = 0;
		innerNodes[i].totalNodes = 1; // Count the current node
	}

	// Start the tree by iterating over the leaves
	//log_debug("[DBG] [BVH] Adding leaves to BVH");
	for (int i = 0; i < numLeaves; i++)
		ChooseParent4(i, true, numLeaves, leafItems, innerNodes);

	// Build the tree
	while (root_out == -1 && innerNodesProcessed < numInnerNodes)
	{
		//log_debug("[DBG] [BVH] ********** Inner node iteration");
		for (int i = 0; i < numInnerNodes; i++)
		{
			if (!innerNodes[i].processed && innerNodes[i].readyCount == 2)
			{
				// This node has its two children and doesn't have a parent yet.
				// Pull-up grandchildren and convert to BVH4 node
				//if (g_bEnableQBVHwSAH)
				//	ConvertToBVH4NodeSAH2(i, innerNodes, leafItems);
					//ConvertToBVH4NodeSAH(innerNodes, i, 0, leafItems);
				//else
					ConvertToBVH4Node(innerNodes, i);
				root_out = ChooseParent4(i, false, numLeaves, leafItems, innerNodes);
				innerNodes[i].processed = true;
				innerNodesProcessed++;
				if (root_out != -1) {
					//if (g_bEnableQBVHwSAH)
					//	ConvertToBVH4NodeSAH2(root_out, innerNodes, leafItems);
						//ConvertToBVH4NodeSAH(innerNodes, root_out, 0, leafItems);
					//else
						ConvertToBVH4Node(innerNodes, root_out);
					break;
				}
			}
		}
	}
	//log_debug("[DBG] [BVH] root at index: %d", *root);
	return innerNodes;
}

int EncodeInnerNode(BVHNode* buffer, BVHNode *node, int EncodeNodeIndex)
{
	uint32_t* ubuffer = (uint32_t*)buffer;
	float* fbuffer = (float*)buffer;
	// Convert the node index into an int offset:
	int ofs = EncodeNodeIndex * sizeof(BVHNode) / 4;

	// Encode the next inner node.
	ubuffer[ofs++] = -1; // TriID
	ubuffer[ofs++] = -1; // parent;
	ubuffer[ofs++] = 0; // rootIdx;
	ubuffer[ofs++] = 0; // numChildren;
	// 16 bytes
	fbuffer[ofs++] = node->min[0];
	fbuffer[ofs++] = node->min[1];
	fbuffer[ofs++] = node->min[2];
	fbuffer[ofs++] = 1.0f;
	// 32 bytes
	fbuffer[ofs++] = node->max[0];
	fbuffer[ofs++] = node->max[1];
	fbuffer[ofs++] = node->max[2];
	fbuffer[ofs++] = 1.0f;
	// 48 bytes
	for (int j = 0; j < 4; j++)
		ubuffer[ofs++] = node->children[j];
	// 64 bytes
	return ofs;
}

int EncodeInnerNode(BVHNode* buffer, InnerNode4* innerNodes, int curNode, int EncodeNodeIndex)
{
	uint32_t* ubuffer = (uint32_t*)buffer;
	float* fbuffer = (float*)buffer;
	AABB box = innerNodes[curNode].aabb;
	const int numChildren = innerNodes[curNode].numChildren;
	int ofs = EncodeNodeIndex * sizeof(BVHNode) / 4;

	// Encode the next inner node.
	ubuffer[ofs++] = -1; // TriID
	ubuffer[ofs++] = -1; // parent;
	ubuffer[ofs++] =  0; // rootIdx;
	ubuffer[ofs++] =  numChildren;
	// 16 bytes
	fbuffer[ofs++] = box.min.x;
	fbuffer[ofs++] = box.min.y;
	fbuffer[ofs++] = box.min.z;
	fbuffer[ofs++] = 1.0f;
	// 32 bytes
	fbuffer[ofs++] = box.max.x;
	fbuffer[ofs++] = box.max.y;
	fbuffer[ofs++] = box.max.z;
	fbuffer[ofs++] = 1.0f;
	// 48 bytes
	for (int j = 0; j < 4; j++)
		ubuffer[ofs++] = (j < numChildren) ? innerNodes[curNode].QBVHOfs[j] : -1;
	// 64 bytes
	return ofs;
}

int EncodeLeafNode(BVHNode* buffer, const std::vector<LeafItem>& leafItems, int leafIdx, int EncodeNodeIdx,
	const XwaVector3 *vertices, const int *indices)
{
	uint32_t* ubuffer = (uint32_t*)buffer;
	float* fbuffer = (float*)buffer;
	int TriID = leafItems[leafIdx].PrimID;
	int idx = TriID * 3;
	XwaVector3 v0, v1, v2;
	int EncodeOfs = EncodeNodeIdx * sizeof(BVHNode) / 4;
	//log_debug("[DBG] [BVH] Encoding leaf %d, TriID: %d, at QBVHOfs: %d",
	//	leafIdx, TriID, (EncodeOfs * 4) / 64);
	// The following "if (...)" is only needed when debugging the builders, since they
	// run without any actual geometry. During regular gameplay, it can be removed
	// The following check is only needed when testing these algorithms (like TestImplicitMortonCodes4())
#ifdef DEBUG_BU
	if (vertices != nullptr && indices != nullptr)
#endif
	{
		v0 = vertices[indices[idx + 0]];
		v1 = vertices[indices[idx + 1]];
		v2 = vertices[indices[idx + 2]];
	}
	// Encode the current primitive into the QBVH buffer, in the leaf section
	//ubuffer[EncodeOfs++] = TriID;
	ubuffer[EncodeOfs++] = leafIdx;
	ubuffer[EncodeOfs++] = -1; // parent
	ubuffer[EncodeOfs++] =  0; // rootIdx
	ubuffer[EncodeOfs++] =  0; // numChildren
	// 16 bytes
	fbuffer[EncodeOfs++] = v0.x;
	fbuffer[EncodeOfs++] = v0.y;
	fbuffer[EncodeOfs++] = v0.z;
	fbuffer[EncodeOfs++] = 1.0f;
	// 32 bytes
	fbuffer[EncodeOfs++] = v1.x;
	fbuffer[EncodeOfs++] = v1.y;
	fbuffer[EncodeOfs++] = v1.z;
	fbuffer[EncodeOfs++] = 1.0f;
	// 48 bytes
	fbuffer[EncodeOfs++] = v2.x;
	fbuffer[EncodeOfs++] = v2.y;
	fbuffer[EncodeOfs++] = v2.z;
	fbuffer[EncodeOfs++] = 1.0f;
	// 64 bytes
	return EncodeOfs;
}

int TLASEncodeLeafNode(BVHNode* buffer, std::vector<TLASLeafItem>& leafItems, int leafIdx, int EncodeNodeIdx)
{
	uint32_t* ubuffer = (uint32_t*)buffer;
	float* fbuffer    = (float*)buffer;
	auto& leafItem    = leafItems[leafIdx];
	int blasID        = TLASGetID(leafItem);
	int matrixSlot    = TLASGetMatrixSlot(leafItem);
	AABB obb          = TLASGetOBB(leafItem);
	AABB aabb         = TLASGetAABBFromOBB(leafItem); // This is OBB * WorldViewTransform
	int BLASBaseNodeOffset = -1;

	const auto& it = g_BLASMap.find(blasID);
	if (it != g_BLASMap.end())
	{
		BLASData& blasData = it->second;
		BLASBaseNodeOffset = BLASGetBaseNodeOffset(blasData);
		// Disable this leaf if the BVH does not exist
		if (BLASGetBVH(blasData) == nullptr)
		{
			BLASBaseNodeOffset = -1;
		}
	}

	int EncodeOfs		 = EncodeNodeIdx * sizeof(BVHNode) / 4;
	//log_debug("[DBG] [BVH] Encoding leaf %d, TriID: %d, at QBVHOfs: %d",
	//	leafIdx, TriID, (EncodeOfs * 4) / 64);

	// Encode the current TLAS leaf into the QBVH buffer, in the leaf section
	ubuffer[EncodeOfs++] = blasID;
	ubuffer[EncodeOfs++] = -1; // parent
	ubuffer[EncodeOfs++] = matrixSlot;
	ubuffer[EncodeOfs++] = BLASBaseNodeOffset;
	// 16 bytes
	fbuffer[EncodeOfs++] = aabb.min[0];
	fbuffer[EncodeOfs++] = aabb.min[1];
	fbuffer[EncodeOfs++] = aabb.min[2];
	fbuffer[EncodeOfs++] = obb.min[0];
	// 32 bytes
	fbuffer[EncodeOfs++] = aabb.max[0];
	fbuffer[EncodeOfs++] = aabb.max[1];
	fbuffer[EncodeOfs++] = aabb.max[2];
	fbuffer[EncodeOfs++] = obb.min[1];
	// 48 bytes
	fbuffer[EncodeOfs++] = obb.max[0];
	fbuffer[EncodeOfs++] = obb.max[1];
	fbuffer[EncodeOfs++] = obb.max[2];
	fbuffer[EncodeOfs++] = obb.min[2];
	// 64 bytes
	return EncodeOfs;
}

// Encodes the immediate children of inner node curNode
// Use with the single-step FastLQBVH
template<class T>
void EncodeChildren(BVHNode *buffer, int numQBVHInnerNodes, InnerNode4* innerNodes, int curNode, std::vector<T>& leafItems, bool isTLAS=false)
{
	uint32_t* ubuffer = (uint32_t*)buffer;
	float* fbuffer = (float*)buffer;

	const int numChildren = innerNodes[curNode].numChildren;
	for (int i = 0; i < numChildren; i++) {
		int childNode = innerNodes[curNode].children[i];

		// Leaves are already encoded
		if (innerNodes[curNode].isLeaf[i]) {
			// TODO: Write the parent of the leaf
			int TriID = leafItems[childNode].PrimID;
			//innerNodes[curNode].QBVHOfs[i] = TriID + numQBVHInnerNodes;
			innerNodes[curNode].QBVHOfs[i] = childNode + numQBVHInnerNodes; // ?
			//log_debug("[DBG] [BVH] Linking Leaf. curNode: %d, i: %d, childNode: %d, QBVHOfs: %d",
			//	curNode, i, childNode, innerNodes[curNode].QBVHOfs[i]);
		}
		else
		{
			if (!(innerNodes[childNode].isEncoded)) {
				if (g_QBVHEncodeNodeIdx < 0)
				{
					log_debug("[DBG] [BVH] %s EncodeChildren: ERROR: g_QBVHEncodeNodeIdx: %d, numQBVHInnerNodes: %d, numLeaves: %d",
						isTLAS ? "[TLAS]" : "[BLAS]",
						g_QBVHEncodeNodeIdx, numQBVHInnerNodes, leafItems.size());
					DumpTLASLeaves(leafItems);
				}
				// Sometimes there's a crash in this line:
				EncodeInnerNode(buffer, innerNodes, childNode, g_QBVHEncodeNodeIdx);
				// Update the parent's QBVH offset for this child:
				innerNodes[curNode].QBVHOfs[i] = g_QBVHEncodeNodeIdx;
				//int child = innerNodes[curNode].children[i];
				//bool childIsLeaf = innerNodes[curNode].isLeaf[i];
				//if (!childIsLeaf) innerNodes[child].selfQBVHOfs = QBVHEncodeNodeIdx;
				innerNodes[childNode].isEncoded = true;
				//log_debug("[DBG] [BVH] Linking Inner (E). curNode: %d, i: %d, childNode: %d, QBVHOfs: %d",
				//	curNode, i, childNode, innerNodes[curNode].QBVHOfs[i]);
				// Jump to the next available slot (atomic decrement)
				g_QBVHEncodeNodeIdx--;
			}
			/*else {
				log_debug("[DBG] [BVH] Linking Inner (NE). curNode: %d, i: %d, childNode: %d, QBVHOfs: %d",
					curNode, i, childNode, innerNodes[curNode].QBVHOfs[i]);
			}*/
		}
	}
}

/// <summary>
/// Single-step QBVH and encoding.
/// Builds the BVH2, converts it to a QBVH and encodes it into buffer, all at the same time.
/// </summary>
/// <param name="buffer">The BVHNode encoding buffer</param>
/// <param name="leafItems"></param>
/// <param name="root_out">The index of the root node</param>
void SingleStepFastLQBVH(BVHNode* buffer, int numQBVHInnerNodes, std::vector<LeafItem>& leafItems, int &root_out
	/*int& inner_root, bool debug = false*/)
{
	int numLeaves = leafItems.size();
	// g_QBVHEncodeNodeIdx points to the next available inner node index.
	g_QBVHEncodeNodeIdx = numQBVHInnerNodes - 1;
	root_out = -1;
	//inner_root = -1;
	if (numLeaves <= 1) {
		// Nothing to do, the single leaf is the root. Return the index of the root
		root_out = numQBVHInnerNodes;
		return;
	}
	//log_debug("[DBG] [BVH] Initial QBVHEncodeNodeIdx: %d", QBVHEncodeNodeIdx);

	// Initialize the inner nodes
	int numInnerNodes = numLeaves - 1;
	int innerNodesProcessed = 0;
	InnerNode4* innerNodes = new InnerNode4[numInnerNodes];
	for (int i = 0; i < numInnerNodes; i++) {
		innerNodes[i].readyCount = 0;
		innerNodes[i].processed = false;
		innerNodes[i].aabb.SetInfinity();
		innerNodes[i].numChildren = 0;
		innerNodes[i].totalNodes = 1; // Count the current node
		innerNodes[i].isEncoded = false;
	}

	// Start the tree by iterating over the leaves
	//log_debug("[DBG] [BVH] Adding leaves to BVH");
	for (int i = 0; i < numLeaves; i++)
		ChooseParent4(i, true, numLeaves, leafItems, innerNodes);

	// Build the tree
	while (root_out == -1 && innerNodesProcessed < numInnerNodes)
	{
		//log_debug("[DBG] [BVH] ********** Inner node iteration");
		for (int i = 0; i < numInnerNodes; i++)
		{
			if (!innerNodes[i].processed && innerNodes[i].readyCount == 2)
			{
				// This node has its two children and doesn't have a parent yet.
				// Pull-up grandchildren and convert to BVH4 node
				//if (g_bEnableQBVHwSAH)
				//	ConvertToBVH4NodeSAH(innerNodes, i, numQBVHInnerNodes, leafItems);
				//else
					ConvertToBVH4Node(innerNodes, i);
				// The children of this node can now be encoded
				EncodeChildren(buffer, numQBVHInnerNodes, innerNodes, i, leafItems);
				root_out = ChooseParent4(i, false, numLeaves, leafItems, innerNodes);
				innerNodes[i].processed = true;
				innerNodesProcessed++;
				if (root_out != -1)
				{
					//inner_root_out = root_out;
					// Convert the root to BVH4
					//if (g_bEnableQBVHwSAH)
					//	ConvertToBVH4NodeSAH(innerNodes, i, numQBVHInnerNodes, leafItems);
					//else
						ConvertToBVH4Node(innerNodes, root_out);
					// The children of this node can now be encoded
					EncodeChildren(buffer, numQBVHInnerNodes, innerNodes, root_out, leafItems);
					//log_debug("[DBG] [BVH] Encoding the root (%d), after processing node: %d, at QBVHOfs: %d",
					//	root_out, i, QBVHEncodeNodeIdx);
					// Encode the root
					EncodeInnerNode(buffer, innerNodes, root_out, g_QBVHEncodeNodeIdx);
					// Replace root with the encoded QBVH root offset
					//inner_root = root_out;
					root_out = g_QBVHEncodeNodeIdx;
					//log_debug("[DBG] [BVH] inner_root: %d, root_out: %d");
					break;
				}
			}
		}
	}
	delete[] innerNodes;
	return;
}

template<class T>
void DumpTLASLeafItem(FILE *file, T& X)
{
	//using TLASLeafItem = std::tuple<MortonCode_t, AABB, int, XwaVector3, int, AABB>;
	AABB aabb = X.aabb;
	fprintf(file, "%lld, (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f), %d\n",
		X.code,
		aabb.min.x, aabb.min.y, aabb.min.z,
		aabb.max.x, aabb.max.y, aabb.max.z,
		X.PrimID);
}

void ReadTLASLeafItem(FILE* file, TLASLeafItem& X)
{
	int code;
	AABB aabb;
	float minx, miny, minz;
	float maxx, maxy, maxz;
	int ID;
	fscanf_s(file, "%d, (%f, %f, %f)-(%f, %f, %f), %d\n",
		&code,
		&minx, &miny, &minz,
		&maxx, &maxy, &maxz,
		&ID);

	aabb.min.x = (float)minx;
	aabb.min.y = (float)miny;
	aabb.min.z = (float)minz;

	aabb.max.x = (float)maxx;
	aabb.max.y = (float)maxy;
	aabb.max.z = (float)maxz;

	X.code = code;
	X.aabb = aabb;
	X.PrimID = ID;
}

template<class T>
void DumpTLASLeaves(std::vector<T>& leafItems)
{
	static int counter = 0;
	char sFileName[100];
	sprintf_s(sFileName, 100, ".\\tlasLeavesRaw-%d.txt", counter++);
	FILE* file = NULL;
	fopen_s(&file, sFileName, "wt");
	fprintf(file, "%d\n", leafItems.size());
	for (uint32_t i = 0; i < leafItems.size(); i++)
	{
		DumpTLASLeafItem(file, leafItems[i]);
	}
	fclose(file);
	log_debug("[DBG] [BVH] [TLAS] Dumped %s", sFileName);
}

void ReadTLASLeaves(char *sFileName, std::vector<TLASLeafItem> &leafItems)
{
	FILE* file = NULL;
	int numLeaves = 0;

	fopen_s(&file, sFileName, "rt");
	fscanf_s(file, "%d\n", & numLeaves);
	for (int i = 0; i < numLeaves; i++)
	{
		TLASLeafItem X;
		ReadTLASLeafItem(file, X);
		leafItems.push_back(X);
	}
	fclose(file);
	log_debug("[DBG] [BVH] Read %d TLASLeafItems", leafItems.size());
}

// TODO: This code is a duplicate of the above, I need to refactor these functions
void TLASSingleStepFastLQBVH(BVHNode* buffer, int numQBVHInnerNodes, std::vector<TLASLeafItem>& leafItems, int& root_out)
{
	int numLeaves = leafItems.size();
	// g_QBVHEncodeNodeIdx points to the next available inner node index.
	g_QBVHEncodeNodeIdx = numQBVHInnerNodes - 1;
	root_out = -1;
	//inner_root = -1;
	if (numLeaves <= 1) {
		// Nothing to do, the single leaf is the root. Return the index of the root
		root_out = numQBVHInnerNodes;
		return;
	}

	if (g_QBVHEncodeNodeIdx < 0)
	{
		log_debug("[DBG] [BVH] [TLAS] ERROR: QBVHEncodeNodeIdx: %d", g_QBVHEncodeNodeIdx);
		DumpTLASLeaves(leafItems);
	}
	//log_debug("[DBG] [BVH] Initial g_QBVHEncodeNodeIdx: %d", g_QBVHEncodeNodeIdx);

	// Initialize the inner nodes
	int numInnerNodes = numLeaves - 1;
	// numLeaves must be at least 2, which means that numInnerNodes should be at least 1
	if (numInnerNodes < 1)
	{
		log_debug("[DBG] [BVH] [TLAS] ERROR: numInnerNodes: %d", numInnerNodes);
		DumpTLASLeaves(leafItems);
	}
	//log_debug("[DBG] [BVH] TLAS single-step build: numLeaves: %d, numInnerNodes: %d",
	//	numLeaves, numInnerNodes);

	int innerNodesProcessed = 0;
	InnerNode4* innerNodes = new InnerNode4[numInnerNodes];
	for (int i = 0; i < numInnerNodes; i++) {
		innerNodes[i].readyCount = 0;
		innerNodes[i].processed = false;
		innerNodes[i].aabb.SetInfinity();
		innerNodes[i].numChildren = 0;
		innerNodes[i].totalNodes = 1; // Count the current node
		innerNodes[i].isEncoded = false;
	}

	// Start the tree by iterating over the leaves
	//log_debug("[DBG] [BVH] Adding leaves to BVH");
	for (int i = 0; i < numLeaves; i++)
		ChooseParent4(i, true, numLeaves, leafItems, innerNodes);

	//log_debug("[DBG] [BVH] Building the tree proper");
	// Build the tree
	while (root_out == -1 && innerNodesProcessed < numInnerNodes)
	{
		//log_debug("[DBG] [BVH] ********** Inner node iteration");
		for (int i = 0; i < numInnerNodes; i++)
		{
			if (!innerNodes[i].processed && innerNodes[i].readyCount == 2)
			{
				// This node has its two children and doesn't have a parent yet.
				// Pull-up grandchildren and convert to BVH4 node
				ConvertToBVH4Node(innerNodes, i);
				// The children of this node can now be encoded
				EncodeChildren(buffer, numQBVHInnerNodes, innerNodes, i, leafItems);
				root_out = ChooseParent4(i, false, numLeaves, leafItems, innerNodes);
				innerNodes[i].processed = true;
				innerNodesProcessed++;
				if (root_out != -1)
				{
					//log_debug("[DBG] [BVH] BVH2 root: %d", root_out);
					//inner_root_out = root_out;
					// Convert the root to BVH4
					ConvertToBVH4Node(innerNodes, root_out);
					// The children of this node can now be encoded
					try {
						// Sometimes there's a crash in this line:
						EncodeChildren(buffer, numQBVHInnerNodes, innerNodes, root_out, leafItems);
					}
					catch (...) {
						log_debug("[DBG] [BVH] [TLAS] FATAL ERROR CAUGHT.");
						DumpTLASLeaves(leafItems);
						log_debug("[DBG] [BVH] [TLAS] TERMINATING.");
						exit(0);
					}
					//log_debug("[DBG] [BVH] Encoding the root (%d), after processing node: %d, at QBVHOfs: %d",
					//	root_out, i, QBVHEncodeNodeIdx);
					// Encode the root
					if (g_QBVHEncodeNodeIdx < 0)
					{
						log_debug("[DBG] [BVH] [TLAS] EncodeInnerNode: ERROR: g_QBVHEncodeNodeIdx: %d, numLeaves: %d",
							g_QBVHEncodeNodeIdx, numLeaves);
						DumpTLASLeaves(leafItems);
					}
					EncodeInnerNode(buffer, innerNodes, root_out, g_QBVHEncodeNodeIdx);
					// Replace root with the encoded QBVH root offset
					//inner_root = root_out;
					root_out = g_QBVHEncodeNodeIdx;
					//log_debug("[DBG] [BVH] inner_root: %d, root_out: %d");
					break;
				}
			}
		}
	}
	delete[] innerNodes;
	return;
}

void TestTLASBuilder(char* sFileName)
{
	try {
		std::vector<TLASLeafItem> tlasLeaves;
		ReadTLASLeaves(sFileName, tlasLeaves);

		const uint32_t numLeaves = tlasLeaves.size();
		const int numQBVHInnerNodes = CalcNumInnerQBVHNodes(numLeaves);
		const int numQBVHNodes = numQBVHInnerNodes + numLeaves;

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
		delete[] QBVHBuffer;
		log_debug("[DBG] [BVH] Test TLAS built successfully");
	}
	catch (...) {
		log_debug("[DBG] [BVH] EXCEPTION CAUGHT WHEN TESTING THE TLAS BUILDER");
	}
}

//#define DEBUG_PLOC 1
#undef DEBUG_PLOC

constexpr int PLOC_RADIUS = 10;

int g_plocNextAvailableNode;

struct PLOCCluster
{
	int id;
	//Vector3 centroid;
	AABB aabb;
	bool active;
	int bestNeighbor;
	bool isLeaf;
};

InnerNode* PLOC(const std::vector<LeafItem>& leafItems, int& root)
{
	int numLeaves = leafItems.size();
	root = -1;
	//log_debug("[DBG] [BVH] numLeaves: %d", numLeaves);
	if (numLeaves <= 1)
		// Nothing to do, the single leaf is the root
		return nullptr;

	// Initialize the inner nodes
	int numInnerNodes = numLeaves - 1;
	g_plocNextAvailableNode = numInnerNodes - 1;

	InnerNode* innerNodes = new InnerNode[numInnerNodes];
	for (int i = 0; i < numInnerNodes; i++)
	{
		innerNodes[i].aabb.SetInfinity();
	}

	// Initialize the clusters
	PLOCCluster* clusters[2];
	clusters[0] = new PLOCCluster[numLeaves];
	clusters[1] = new PLOCCluster[numLeaves];
	int clusterNums[2] = { numLeaves, 0 };
	int curClusterSet = 0, nextClusterSet;
	for (int i = 0; i < numLeaves; i++)
	{
		const LeafItem& leaf = leafItems[i];
		clusters[curClusterSet][i] = { i, leaf.aabb, true, -1, true };
	}

	for (int iteration = 0; iteration < numLeaves; iteration++)
	{
#ifdef DEBUG_PLOC
		log_debug("[DBG] [BVH] iteration: %d, num clusters: %d", iteration, clusters.size());
#endif

		// Find the nearest neighbor for each cluster
		for (int i = 0; i < clusterNums[curClusterSet]; i++)
		{
			PLOCCluster& c = clusters[curClusterSet][i];
			if (!c.active)
				continue;

			// Find the best neighbor for cluster i
			float bestDist = FLT_MAX;
			int left = max(0, i - PLOC_RADIUS);
			int right = min(i + PLOC_RADIUS, clusterNums[curClusterSet] - 1);
			c.bestNeighbor = -1;
			for (int j = left; j <= right; j++)
			{
				if (i == j || !clusters[curClusterSet][j].active)
					continue;

				//const float dist = c.centroid.distance(clusters[curClusterSet][j].centroid);
				AABB tmpBox = c.aabb;
				tmpBox.Expand(clusters[curClusterSet][j].aabb);
				const float dist = tmpBox.GetArea();
				if (dist < bestDist)
				{
					c.bestNeighbor = j;
					bestDist = dist;
				}
			}

#ifdef DEBUG_PLOC
			log_debug("[DBG] [BVH] cluster[%d]: centroid: (%0.3f, %0.3f, %0.3f), bestNeighbor: %d",
				i,
				c.centroid[0],
				c.centroid[1],
				c.centroid[2],
				c.bestNeighbor);
#endif
		}

		// All clusters have selected their best neighbor, let's join them
		for (int i = 0; i < clusterNums[curClusterSet]; i++)
		{
			if (!clusters[curClusterSet][i].active)
				continue;

			const int j = clusters[curClusterSet][i].bestNeighbor;
			if (j < 0)
				continue;

			const int j_neighbor = clusters[curClusterSet][j].bestNeighbor;

			if (i == j_neighbor)
			{
				// Join clusters i and j
#ifdef DEBUG_PLOC
				log_debug("[DBG] [BVH] cluster %d and %d should join", i, j);
#endif
				// Disable the second cluster
				clusters[curClusterSet][j].active = false;
				//Vector3 newCentroid = 0.5f * (clusters[curClusterSet][i].centroid + clusters[curClusterSet][j].centroid);
				AABB newAABB = clusters[curClusterSet][i].aabb;
				newAABB.Expand(clusters[curClusterSet][j].aabb);

				// Emit an inner node to join the two clusters
				const int nextNodeIdx = g_plocNextAvailableNode;
				g_plocNextAvailableNode--;
				InnerNode& newNode = innerNodes[nextNodeIdx];

				newNode.left = clusters[curClusterSet][i].id;
				newNode.leftIsLeaf = clusters[curClusterSet][i].isLeaf;
				newNode.right = clusters[curClusterSet][j].id;
				newNode.rightIsLeaf = clusters[curClusterSet][j].isLeaf;

				// Refit
				newNode.aabb.SetInfinity();
				AABB leftBox  = newNode.leftIsLeaf  ? leafItems[newNode.left].aabb  : innerNodes[newNode.left].aabb;
				AABB rightBox = newNode.rightIsLeaf ? leafItems[newNode.right].aabb : innerNodes[newNode.right].aabb;
				newNode.aabb.Expand(leftBox);
				newNode.aabb.Expand(rightBox);
				// Update the inner nodes
				innerNodes[nextNodeIdx] = newNode;

				// Update the old cluster: it now points to the new inner node just created
				clusters[curClusterSet][i].isLeaf = false;
				clusters[curClusterSet][i].id = nextNodeIdx;
				//clusters[i].centroid = 0.5f * (leftBox.GetCentroidVector3() + rightBox.GetCentroidVector3());
				clusters[curClusterSet][i].aabb = newAABB;

#ifdef DEBUG_PLOC
				log_debug("[DBG] [BVH] nextNodeIdx: %d, centroid: (%0.3f, %0.3f, %0.3f), left: %d(%s), right: %d(%s), box: %s",
					nextNodeIdx,
					clusters[i].centroid[0],
					clusters[i].centroid[1],
					clusters[i].centroid[2],
					newNode.left, newNode.leftIsLeaf ? "]" : "I",
					newNode.right, newNode.rightIsLeaf ? "]" : "I",
					newNode.aabb.ToString().c_str());
#endif
			}
		}

		// Test the exit condition.
		if (g_plocNextAvailableNode < 0)
		{
			root = 0;
#ifdef DEBUG_PLOC
			log_debug("[DBG] [BVH] PLOC has finished");
#endif
			break;
		}

		// Remove disabled clusters
		nextClusterSet = (curClusterSet + 1) % 2;
		int k = 0;
		for (int i = 0; i < clusterNums[curClusterSet]; i++)
		{
			if (clusters[curClusterSet][i].active)
				clusters[nextClusterSet][k++] = clusters[curClusterSet][i];
		}
		clusterNums[nextClusterSet] = k;
		curClusterSet = nextClusterSet;

#ifdef DEBUG_PLOC
		log_debug("[DBG] [BVH] num clusters: %d", clusters.size());
		log_debug("[DBG] [BVH] ***************************************************");
#endif
	}

	delete[] clusters[0];
	delete[] clusters[1];
	return innerNodes;
}

std::string tab(int N)
{
	std::string res = "";
	for (int i = 0; i < N; i++)
		res += " ";
	return res;
}

static void printTree(int N, int curNode, bool isLeaf, InnerNode* innerNodes)
{
	if (curNode == -1 || innerNodes == nullptr)
		return;

	if (isLeaf)
	{
		log_debug("[DBG] [BVH] %s%d]", tab(N).c_str(), curNode);
		return;
	}

	printTree(N + 4, innerNodes[curNode].right, innerNodes[curNode].rightIsLeaf, innerNodes);
	log_debug("[DBG] [BVH] %s%d", tab(N).c_str(), curNode);
	printTree(N + 4, innerNodes[curNode].left, innerNodes[curNode].leftIsLeaf, innerNodes);
}

static void printTree(int N, int curNode, bool isLeaf, InnerNode4* innerNodes)
{
	if (curNode == -1 || innerNodes == nullptr)
		return;

	if (isLeaf)
	{
		log_debug("[DBG] [BVH] %s%d]", tab(N).c_str(), curNode);
		return;
	}

	int arity = innerNodes[curNode].numChildren;
	log_debug("[DBG] [BVH] %s", (tab(N) + "   /--\\").c_str());
	for (int i = arity - 1; i >= arity / 2; i--)
		printTree(N + 4, innerNodes[curNode].children[i], innerNodes[curNode].isLeaf[i], innerNodes);
	log_debug("[DBG] [BVH] %s%d,%d", tab(N).c_str(), curNode, innerNodes[curNode].totalNodes);
	for (int i = arity / 2 - 1; i >= 0; i--)
		printTree(N + 4, innerNodes[curNode].children[i], innerNodes[curNode].isLeaf[i], innerNodes);
	log_debug("[DBG] [BVH] %s", (tab(N) + "   \\--/").c_str());
}

static void PrintTree(std::string level, IGenericTreeNode *T)
{
	if (T == nullptr)
		return;

	int arity = T->GetArity();
	std::vector<IGenericTreeNode*> children = T->GetChildren();
	bool isLeaf = T->IsLeaf();

	if (arity > 2 && !isLeaf) log_debug("[DBG] [BVH] %s", (level + "   /--\\").c_str());
	for (int i = arity - 1; i >= arity / 2; i--)
		if (i < (int)children.size())
			PrintTree(level + "    ", children[i]);

	log_debug("[DBG] [BVH] %s%s",
		(level + std::to_string(T->GetTriID())).c_str(), isLeaf ? "]" : "");

	for (int i = arity / 2 - 1; i >= 0; i--)
		if (i < (int)children.size())
			PrintTree(level + "    ", children[i]);
	if (arity > 2 && !isLeaf) log_debug("[DBG] [BVH] %s", (level + "   \\--/").c_str());
}

static void PrintTreeNode(std::string level, TreeNode* T)
{
	if (T == nullptr)
		return;

	bool isLeaf = T->IsLeaf();

	PrintTreeNode(level + "      ", T->right);

	log_debug("[DBG] [BVH] %s(%d:%0.3f),%d,%d%s",
		level.c_str(),
		T->TriID,
		isLeaf ? T->centroid.x : 0.0f,
		T->bal,
		T->numNodes,
		isLeaf ? "]" : "");

	PrintTreeNode(level + "      ", T->left);
}

void PrintTreeBuffer(std::string level, BVHNode *buffer, int curNode)
{
	if (buffer == nullptr)
		return;

	BVHNode node = buffer[curNode];
	int TriID = node.ref;
	if (TriID != -1) {
		// Leaf
		log_debug("[DBG] [BVH] %s%d,%d]", level.c_str(), curNode, TriID);
		/*
		BVHPrimNode* n = (BVHPrimNode *)&node;
		log_debug("[DBG] [BVH] %sv0: (%0.3f, %0.3f, %0.3f)", (level + "   ").c_str(),
			n->v0[0], n->v0[1], n->v0[2]);
		log_debug("[DBG] [BVH] %sv1: (%0.3f, %0.3f, %0.3f)", (level + "   ").c_str(),
			n->v1[0], n->v1[1], n->v1[2]);
		log_debug("[DBG] [BVH] %sv2: (%0.3f, %0.3f, %0.3f)", (level + "   ").c_str(),
			n->v2[0], n->v2[1], n->v2[2]);
		*/
		return;
	}

	int arity = 4;
	if (arity > 2) log_debug("[DBG] [BVH] %s", (level + "   /----\\").c_str());
	for (int i = arity - 1; i >= arity / 2; i--)
		if (node.children[i] != -1)
			PrintTreeBuffer(level + "    ", buffer, node.children[i]);

	log_debug("[DBG] [BVH] %s%d", level.c_str(), curNode);

	for (int i = arity / 2 - 1; i >= 0; i--)
		if (node.children[i] != -1)
			PrintTreeBuffer(level + "    ", buffer, node.children[i]);
	if (arity > 2) log_debug("[DBG] [BVH] %s", (level + "   \\----/").c_str());
}

void Normalize(Vector3 &A, const AABB &sceneBox, const XwaVector3 &range)
{
	A.x -= sceneBox.min.x;
	A.y -= sceneBox.min.y;
	A.z -= sceneBox.min.z;

	A.x /= range.x;
	A.y /= range.y;
	A.z /= range.z;
}

int DumpTriangle(const std::string &name, FILE *file, int OBJindex, const XwaVector3 &v0, const XwaVector3& v1, const XwaVector3& v2)
{
	if (name.size() != 0)
		fprintf(file, "o %s\n", name.c_str());

	fprintf(file, "v %f %f %f\n", v0.x * OPT_TO_METERS, v0.y * OPT_TO_METERS, v0.z * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n", v1.x * OPT_TO_METERS, v1.y * OPT_TO_METERS, v1.z * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n", v2.x * OPT_TO_METERS, v2.y * OPT_TO_METERS, v2.z * OPT_TO_METERS);

	fprintf(file, "f %d %d %d\n", OBJindex, OBJindex + 1, OBJindex + 2);
	return OBJindex + 3;
}

int DumpAABB(const std::string &name, FILE* file, int OBJindex, const Vector3 &min, const Vector3 &max)
{
	fprintf(file, "o %s\n", name.c_str());

	fprintf(file, "v %f %f %f\n",
		min[0] * OPT_TO_METERS, min[1] * OPT_TO_METERS, min[2] * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n",
		max[0] * OPT_TO_METERS, min[1] * OPT_TO_METERS, min[2] * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n",
		max[0] * OPT_TO_METERS, max[1] * OPT_TO_METERS, min[2] * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n",
		min[0] * OPT_TO_METERS, max[1] * OPT_TO_METERS, min[2] * OPT_TO_METERS);

	fprintf(file, "v %f %f %f\n",
		min[0] * OPT_TO_METERS, min[1] * OPT_TO_METERS, max[2] * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n",
		max[0] * OPT_TO_METERS, min[1] * OPT_TO_METERS, max[2] * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n",
		max[0] * OPT_TO_METERS, max[1] * OPT_TO_METERS, max[2] * OPT_TO_METERS);
	fprintf(file, "v %f %f %f\n",
		min[0] * OPT_TO_METERS, max[1] * OPT_TO_METERS, max[2] * OPT_TO_METERS);

	fprintf(file, "f %d %d\n", OBJindex + 0, OBJindex + 1);
	fprintf(file, "f %d %d\n", OBJindex + 1, OBJindex + 2);
	fprintf(file, "f %d %d\n", OBJindex + 2, OBJindex + 3);
	fprintf(file, "f %d %d\n", OBJindex + 3, OBJindex + 0);

	fprintf(file, "f %d %d\n", OBJindex + 4, OBJindex + 5);
	fprintf(file, "f %d %d\n", OBJindex + 5, OBJindex + 6);
	fprintf(file, "f %d %d\n", OBJindex + 6, OBJindex + 7);
	fprintf(file, "f %d %d\n", OBJindex + 7, OBJindex + 4);

	fprintf(file, "f %d %d\n", OBJindex + 0, OBJindex + 4);
	fprintf(file, "f %d %d\n", OBJindex + 1, OBJindex + 5);
	fprintf(file, "f %d %d\n", OBJindex + 2, OBJindex + 6);
	fprintf(file, "f %d %d\n", OBJindex + 3, OBJindex + 7);
	return OBJindex + 8;
}

void DumpInnerNodesToOBJ(char *sFileName, int rootIdx,
	const InnerNode* innerNodes, const std::vector<LeafItem>& leafItems,
	const XwaVector3* vertices, const int* indices)
{
	int OBJindex = 1;
	int numLeaves = leafItems.size();
	int numInnerNodes = numLeaves - 1;
	std::string name;

	log_debug("[DBG] [BVH] ***** Dumping Fast LBVH to file: %s", sFileName);

	FILE* file = NULL;
	fopen_s(&file, sFileName, "wt");
	if (file == NULL) {
		log_debug("[DBG] [BVH] Could not open file: %s", sFileName);
		return;
	}

	for (int curNode = 0; curNode < numLeaves; curNode++)
	{
		int TriID = leafItems[curNode].PrimID;
		int i = TriID * 3;

		XwaVector3 v0 = vertices[indices[i + 0]];
		XwaVector3 v1 = vertices[indices[i + 1]];
		XwaVector3 v2 = vertices[indices[i + 2]];

		name = "leaf-" + std::to_string(curNode);
		OBJindex = DumpTriangle(name, file, OBJindex, v0, v1, v2);
	}

	for (int curNode = 0; curNode < numInnerNodes; curNode++)
	{
		InnerNode node = innerNodes[curNode];

		name = (curNode == rootIdx) ?
			name = "ROOT-" + std::to_string(curNode) :
			name = "aabb-" + std::to_string(curNode);
		OBJindex = DumpAABB(name, file, OBJindex, node.aabb.min, node.aabb.max);
	}

	fclose(file);
}

// Encode a BVH4 node using Embedded Geometry.
// Returns the new offset (in multiples of 4 bytes) that can be written to.
static int EncodeTreeNode4(void* buffer, int startOfs, IGenericTreeNode* T, int32_t parent, const std::vector<int>& children,
	const XwaVector3* Vertices, const int* Indices)
{
	AABB box = T->GetBox();
	int TriID = T->GetTriID();
	int padding = 0;
	int ofs = startOfs;
	uint32_t* ubuffer = (uint32_t*)buffer;
	float* fbuffer = (float*)buffer;

	// This leaf node must have its vertices embedded in the node
	if (TriID != -1)
	{
		int vertofs = TriID * 3;
		XwaVector3 v0, v1, v2;

		//if (Vertices != nullptr && Indices != nullptr)
		{
			v0 = Vertices[Indices[vertofs]];
			v1 = Vertices[Indices[vertofs + 1]];
			v2 = Vertices[Indices[vertofs + 2]];
		}

		ubuffer[ofs++] = TriID;
		ubuffer[ofs++] = parent;
		ubuffer[ofs++] = padding;
		ubuffer[ofs++] = padding;
		// 16 bytes

		fbuffer[ofs++] = v0.x;
		fbuffer[ofs++] = v0.y;
		fbuffer[ofs++] = v0.z;
		fbuffer[ofs++] = 1.0f;
		// 32 bytes
		fbuffer[ofs++] = v1.x;
		fbuffer[ofs++] = v1.y;
		fbuffer[ofs++] = v1.z;
		fbuffer[ofs++] = 1.0f;
		// 48 bytes
		fbuffer[ofs++] = v2.x;
		fbuffer[ofs++] = v2.y;
		fbuffer[ofs++] = v2.z;
		fbuffer[ofs++] = 1.0f;
		// 64 bytes
	}
	else
	{
		ubuffer[ofs++] = TriID;
		ubuffer[ofs++] = parent;
		ubuffer[ofs++] = padding;
		ubuffer[ofs++] = padding;
		// 16 bytes
		fbuffer[ofs++] = box.min.x;
		fbuffer[ofs++] = box.min.y;
		fbuffer[ofs++] = box.min.z;
		fbuffer[ofs++] = 1.0f;
		// 32 bytes
		fbuffer[ofs++] = box.max.x;
		fbuffer[ofs++] = box.max.y;
		fbuffer[ofs++] = box.max.z;
		fbuffer[ofs++] = 1.0f;
		// 48 bytes
		for (int i = 0; i < 4; i++)
			ubuffer[ofs++] = children[i];
		// 64 bytes
	}

	/*
	if (ofs - startOfs == ENCODED_TREE_NODE4_SIZE)
	{
		log_debug("[DBG] [BVH] TreeNode should be encoded in %d bytes, but got %d instead",
			ENCODED_TREE_NODE4_SIZE, ofs - startOfs);
	}
	*/
	return ofs;
}

// Encode a BVH4 node using Embedded Geometry.
// Returns the new offset (in multiples of 4 bytes) that can be written to.
static int EncodeTreeNode4(void* buffer, int startOfs,
	int curNode, InnerNode4* innerNodes,
	bool isLeaf, const std::vector<LeafItem>& leafItems,
	int32_t parent, const std::vector<int>& children,
	const XwaVector3* Vertices, const int* Indices)
{
	AABB  box   = isLeaf ? leafItems[curNode].aabb   : innerNodes[curNode].aabb;
	int   TriID = isLeaf ? leafItems[curNode].PrimID : -1;
	int padding = 0;
	int ofs = startOfs;
	uint32_t* ubuffer = (uint32_t*)buffer;
	float* fbuffer = (float*)buffer;

	// This leaf node must have its vertices embedded in the node
	if (TriID != -1)
	{
		int vertofs = TriID * 3;
		XwaVector3 v0, v1, v2;
		if (Vertices != nullptr && Indices != nullptr)
		{
			v0 = Vertices[Indices[vertofs]];
			v1 = Vertices[Indices[vertofs + 1]];
			v2 = Vertices[Indices[vertofs + 2]];
		}

		ubuffer[ofs++] = TriID;
		ubuffer[ofs++] = parent;
		ubuffer[ofs++] = padding;
		ubuffer[ofs++] = padding;
		// 16 bytes

		fbuffer[ofs++] = v0.x;
		fbuffer[ofs++] = v0.y;
		fbuffer[ofs++] = v0.z;
		fbuffer[ofs++] = 1.0f;
		// 32 bytes
		fbuffer[ofs++] = v1.x;
		fbuffer[ofs++] = v1.y;
		fbuffer[ofs++] = v1.z;
		fbuffer[ofs++] = 1.0f;
		// 48 bytes
		fbuffer[ofs++] = v2.x;
		fbuffer[ofs++] = v2.y;
		fbuffer[ofs++] = v2.z;
		fbuffer[ofs++] = 1.0f;
		// 64 bytes
	}
	else
	{
		ubuffer[ofs++] = TriID;
		ubuffer[ofs++] = parent;
		ubuffer[ofs++] = padding;
		ubuffer[ofs++] = padding;
		// 16 bytes
		fbuffer[ofs++] = box.min.x;
		fbuffer[ofs++] = box.min.y;
		fbuffer[ofs++] = box.min.z;
		fbuffer[ofs++] = 1.0f;
		// 32 bytes
		fbuffer[ofs++] = box.max.x;
		fbuffer[ofs++] = box.max.y;
		fbuffer[ofs++] = box.max.z;
		fbuffer[ofs++] = 1.0f;
		// 48 bytes
		for (int i = 0; i < 4; i++)
			ubuffer[ofs++] = children[i];
		// 64 bytes
	}

	/*
	if (ofs - startOfs == ENCODED_TREE_NODE4_SIZE)
	{
		log_debug("[DBG] [BVH] TreeNode should be encoded in %d bytes, but got %d instead",
			ENCODED_TREE_NODE4_SIZE, ofs - startOfs);
	}
	*/
	return ofs;
}

// Returns a compact buffer containing BVHNode entries that represent the given tree
// The number of nodes is read from root->GetNumNodes();
uint8_t* EncodeNodes(IGenericTreeNode* root, const XwaVector3* Vertices, const int* Indices)
{
	uint8_t* result = nullptr;
	if (root == nullptr)
		return result;

	int NumNodes = root->GetNumNodes();
	result = new uint8_t[NumNodes * ENCODED_TREE_NODE4_SIZE];
	int startOfs = 0;
	uint32_t arity = root->GetArity();
	//log_debug("[DBG] [BVH] Encoding %d BVH nodes", NumNodes);

	// A breadth-first traversal will ensure that each level of the tree is encoded to the
	// buffer before advancing to the next level. We can thus keep track of the offset in
	// the buffer where the next node will appear.
	std::queue<IGenericTreeNode*> Q;

	// Initialize the queue and the offsets.
	Q.push(root);
	// Since we're going to put this data in an array, it's easier to specify the children
	// offsets as indices into this array.
	int nextNode = 1;
	std::vector<int> childOfs;

	while (Q.size() != 0)
	{
		IGenericTreeNode* T = Q.front();
		Q.pop();
		std::vector<IGenericTreeNode*> children = T->GetChildren();

		// In a breadth-first search, the left child will always be at offset nextNode
		// Add the children offsets
		childOfs.clear();
		for (uint32_t i = 0; i < arity; i++)
			childOfs.push_back(i < children.size() ? nextNode + i : -1);

		startOfs = EncodeTreeNode4(result, startOfs, T, -1 /* parent (TODO) */,
			childOfs, Vertices, Indices);

		// Enqueue the children
		for (const auto& child : children)
		{
			Q.push(child);
			nextNode++;
		}
	}

	return result;
}

// Returns a compact buffer containing BVHNode entries that represent the given TLAS tree
// The number of nodes is read from root->GetNumNodes();
uint8_t* TLASEncodeNodes(IGenericTreeNode* root, std::vector<TLASLeafItem>& leafItems)
{
	uint8_t* result = nullptr;
	if (root == nullptr)
		return result;

	int NumNodes = root->GetNumNodes();
	result = new uint8_t[NumNodes * ENCODED_TREE_NODE4_SIZE];
	int startOfs = 0;
	uint32_t arity = root->GetArity();
	//log_debug("[DBG] [BVH] Encoding %d BVH nodes", NumNodes);

	// A breadth-first traversal will ensure that each level of the tree is encoded to the
	// buffer before advancing to the next level. We can thus keep track of the offset in
	// the buffer where the next node will appear.
	std::queue<IGenericTreeNode*> Q;

	// Initialize the queue and the offsets.
	Q.push(root);
	// Since we're going to put this data in an array, it's easier to specify the children
	// offsets as indices into this array.
	int nextNode = 1;
	std::vector<int> childOfs;
	constexpr int ofsFactor = sizeof(BVHNode) / 4;

	while (Q.size() != 0)
	{
		IGenericTreeNode* T = Q.front();
		Q.pop();
		std::vector<IGenericTreeNode*> children = T->GetChildren();

		// In a breadth-first search, the left child will always be at offset nextNode
		// Add the children offsets
		childOfs.clear();
		for (uint32_t i = 0; i < arity; i++)
			childOfs.push_back(i < children.size() ? nextNode + i : -1);

		if (T->IsLeaf())
		{
			TLASEncodeLeafNode((BVHNode*)result, leafItems, T->GetTriID(), startOfs / ofsFactor);
			startOfs += ofsFactor;
		}
		else
		{
			startOfs = EncodeTreeNode4(result, startOfs, T, -1 /* parent (TODO) */,
				childOfs, nullptr, nullptr);
		}

		// Enqueue the children
		for (const auto& child : children)
		{
			Q.push(child);
			nextNode++;
		}
	}

	return result;
}

// Returns a compact buffer containing BVHNode entries that represent the given tree
// The tree is expected to be of arity 4.
// The current ray-tracer uses embedded geometry.
uint8_t* EncodeNodes(int root, InnerNode4* innerNodes, const std::vector<LeafItem>& leafItems,
	const XwaVector3* Vertices, const int* Indices)
{
	using Item = std::pair<int, bool>;
	uint8_t* result = nullptr;
	if (root == -1)
		return result;

	int NumNodes = innerNodes[root].totalNodes;
	result = new uint8_t[NumNodes * ENCODED_TREE_NODE4_SIZE];
	int startOfs = 0;
	uint32_t arity = 4; // Yeah, hard-coded, this will _definitely_ come back and bite me in the ass later, but whatever.
	//log_debug("[DBG] [BVH] Encoding %d QBVH nodes", NumNodes);

	// A breadth-first traversal will ensure that each level of the tree is encoded to the
	// buffer before advancing to the next level. We can thus keep track of the offset in
	// the buffer where the next node will appear.
	std::queue<Item> Q;

	// Initialize the queue and the offsets.
	Q.push(Item(root, false));
	// Since we're going to put this data in an array, it's easier to specify the children
	// offsets as indices into this array.
	int nextNode = 1;
	std::vector<int> childOfs;

	while (Q.size() != 0)
	{
		Item curItem = Q.front();
		int curNode = curItem.first;
		bool isLeaf = curItem.second;
		Q.pop();
		int* children = isLeaf ? nullptr : innerNodes[curNode].children;
		bool* isLeafArray = isLeaf ? nullptr : innerNodes[curNode].isLeaf;
		uint32_t numChildren = isLeaf ? 0 : innerNodes[curNode].numChildren;

		// In a breadth-first search, the left child will always be at offset nextNode
		// Add the children offsets
		childOfs.clear();
		for (uint32_t i = 0; i < arity; i++)
			childOfs.push_back(i < numChildren ? nextNode + i : -1);

		startOfs = EncodeTreeNode4(result, startOfs,
			curNode, innerNodes,
			isLeaf, leafItems,
			-1 /* parent (TODO) */, childOfs,
			Vertices, Indices);

		// Enqueue the children
		for (uint32_t i = 0; i < numChildren; i++)
		{
			Q.push(Item(children[i], isLeafArray[i]));
			nextNode++;
		}
	}

	return result;
}

// Converts a BVH2 into an encoded QBVH and returns a compact buffer
// The current ray-tracer uses embedded geometry.
BVHNode* EncodeNodesAsQBVH(int root, InnerNode* innerNodes, const std::vector<LeafItem>& leafItems,
	const XwaVector3* Vertices, const int* Indices, int &numQBVHNodes_out)
{
	using Item = std::pair<int, bool>;
	BVHNode* result = nullptr;
	if (root == -1)
		return result;

	int numPrimitives = leafItems.size();
	numQBVHNodes_out = numPrimitives + CalcNumInnerQBVHNodes(numPrimitives);
	result = new BVHNode[numQBVHNodes_out];
	// Initialize the root
	result[0].rootIdx = 0;

	// A breadth-first traversal will ensure that each level of the tree is encoded to the
	// buffer before advancing to the next level. We can thus keep track of the offset in
	// the buffer where the next node will appear.
	std::queue<Item> Q;

	// Initialize the queue and the offsets.
	Q.push(Item(root, false));
	// Since we're going to put this data in an array, it's easier to specify the children
	// offsets as indices into this array.
	int nextNode = 1;
	int EncodeIdx = 0;

	while (Q.size() != 0)
	{
		Item curItem = Q.front();
		int curNode = curItem.first;
		bool isLeaf = curItem.second;
		Q.pop();
		BVHNode node;
		int nextchild = 0;

		if (!isLeaf)
		{
			// Initialize the node
			node.ref    = -1;
			node.parent  = -1;

			node.min[0] = innerNodes[curNode].aabb.min.x;
			node.min[1] = innerNodes[curNode].aabb.min.y;
			node.min[2] = innerNodes[curNode].aabb.min.z;

			node.max[0] = innerNodes[curNode].aabb.max.x;
			node.max[1] = innerNodes[curNode].aabb.max.y;
			node.max[2] = innerNodes[curNode].aabb.max.z;

			for (int i = 0; i < 4; i++)
				node.children[i] = -1;

			// Pull-up the grandchildren, if possible
			if (innerNodes[curNode].leftIsLeaf)
			{
				node.children[nextchild++] = nextNode++;
				Q.push(Item(innerNodes[curNode].left, true));
			}
			else
			{
				int c0 = innerNodes[curNode].left;

				node.children[nextchild++] = nextNode++;
				Q.push(Item(innerNodes[c0].left, innerNodes[c0].leftIsLeaf));
				node.children[nextchild++] = nextNode++;
				Q.push(Item(innerNodes[c0].right, innerNodes[c0].rightIsLeaf));
			}

			if (innerNodes[curNode].rightIsLeaf)
			{
				node.children[nextchild++] = nextNode++;
				Q.push(Item(innerNodes[curNode].right, true));
			}
			else
			{
				int c1 = innerNodes[curNode].right;

				node.children[nextchild++] = nextNode++;
				Q.push(Item(innerNodes[c1].left, innerNodes[c1].leftIsLeaf));
				node.children[nextchild++] = nextNode++;
				Q.push(Item(innerNodes[c1].right, innerNodes[c1].rightIsLeaf));
			}

			// Encode the inner node
			EncodeInnerNode(result, &node, EncodeIdx++);
		}
		else
		{
			// Encode a leaf node
			EncodeLeafNode(result, leafItems, curNode, EncodeIdx++, Vertices, Indices);
		}
	}

	return result;
}

// Only call this function for inner nodes
std::vector<EncodeItem> PullUpChildren(int curNode, InnerNode* innerNodes, const std::vector<LeafItem>& leafItems)
{
	std::vector<EncodeItem> items;

	// Pull-up the grandchildren, if possible
	if (innerNodes[curNode].leftIsLeaf)
	{
		items.push_back(EncodeItem(innerNodes[curNode].left, true, 0));
	}
	else
	{
		int c0 = innerNodes[curNode].left;
		items.push_back(EncodeItem(innerNodes[c0].left, innerNodes[c0].leftIsLeaf, 0));
		items.push_back(EncodeItem(innerNodes[c0].right, innerNodes[c0].rightIsLeaf, 0));
	}

	if (innerNodes[curNode].rightIsLeaf)
	{
		items.push_back(EncodeItem(innerNodes[curNode].right, true, 0));
	}
	else
	{
		int c1 = innerNodes[curNode].right;

		items.push_back(EncodeItem(innerNodes[c1].left, innerNodes[c1].leftIsLeaf, 0));
		items.push_back(EncodeItem(innerNodes[c1].right, innerNodes[c1].rightIsLeaf, 0));
	}

	return items;
}

/// <summary>
/// Finds which children to pull up for a BVH4 conversion
/// using SAH.
/// Use this with the standard FastLBVH2 builder.
/// Only call this function for inner nodes.
/// </summary>
std::vector<EncodeItem> PullUpChildrenSAH(int curNode, InnerNode* innerNodes, const std::vector<LeafItem>& leafItems)
{
	std::vector<EncodeItem> items;
	std::vector<EncodeItem> result;

	// Assumption: curNode is always an inner node, so let's add its two
	// children to the item vector
	items.push_back(EncodeItem(innerNodes[curNode].left, innerNodes[curNode].leftIsLeaf, 0));
	items.push_back(EncodeItem(innerNodes[curNode].right, innerNodes[curNode].rightIsLeaf, 0));

	for (int i = 0; i < 2; i++)
	{
		result.clear();
		// Open the node with the largest area
		float maxArea = 0.0f;
		int maxAreaIdx = -1;
		for (uint32_t j = 0; j < items.size(); j++)
		{
			EncodeItem item  = items[j];
			int  childNode   = std::get<0>(item);
			bool childIsLeaf = std::get<1>(item);
			// Only inner nodes can be opened
			if (!childIsLeaf) {
				float area = innerNodes[childNode].aabb.GetArea();
				if (area > maxArea) {
					maxArea = area;
					maxAreaIdx = j;
				}
			}
		}

		// Expand the node with the maximum area
		for (uint32_t j = 0; j < items.size(); j++)
		{
			if (j == maxAreaIdx)
			{
				EncodeItem item  = items[j];
				int  childNode   = std::get<0>(item);
				bool childIsLeaf = std::get<1>(item);
				if (childIsLeaf)
				{
					result.push_back(item);
				}
				else
				{
					items.push_back(EncodeItem(innerNodes[childNode].left, innerNodes[childNode].leftIsLeaf, 0));
					items.push_back(EncodeItem(innerNodes[childNode].right, innerNodes[childNode].rightIsLeaf, 0));
				}
			}
			else
			{
				result.push_back(items[j]);
			}
		}

		// Copy the expanded nodes back into items for the next iteration
		items.clear();
		for (const auto& item : result)
			items.push_back(item);
	}

	return result;
}

// Converts a BVH2 into an encoded QBVH and returns a compact buffer
// The current ray-tracer uses embedded geometry.
BVHNode* EncodeNodesAsQBVHwSAH(int root, InnerNode* innerNodes, const std::vector<LeafItem>& leafItems,
	const XwaVector3* Vertices, const int* Indices, int& numQBVHNodes_out)
{
	BVHNode* result = nullptr;
	if (root == -1)
		return result;

	int numPrimitives = leafItems.size();
	numQBVHNodes_out = numPrimitives + CalcNumInnerQBVHNodes(numPrimitives);
	result = new BVHNode[numQBVHNodes_out];
	// Initialize the root
	result[0].rootIdx = 0;

	// A breadth-first traversal will ensure that each level of the tree is encoded to the
	// buffer before advancing to the next level. We can thus keep track of the offset in
	// the buffer where the next node will appear.
	std::queue<EncodeItem> Q;

	// Initialize the queue and the offsets.
	Q.push(EncodeItem(root, false, 0));
	// Since we're going to put this data in an array, it's easier to specify the children
	// offsets as indices into this array.
	int nextNode = 1;
	int EncodeIdx = 0;

	while (Q.size() != 0)
	{
		EncodeItem curItem = Q.front();
		int curNode = std::get<0>(curItem);
		bool isLeaf = std::get<1>(curItem);
		Q.pop();
		BVHNode node;
		int nextchild = 0;

		if (isLeaf)
		{
			// Encode a leaf node
			EncodeLeafNode(result, leafItems, curNode, EncodeIdx++, Vertices, Indices);
		}
		else
		{
			// Initialize the node
			node.ref    = -1;
			node.parent = -1;

			node.min[0] = innerNodes[curNode].aabb.min.x;
			node.min[1] = innerNodes[curNode].aabb.min.y;
			node.min[2] = innerNodes[curNode].aabb.min.z;

			node.max[0] = innerNodes[curNode].aabb.max.x;
			node.max[1] = innerNodes[curNode].aabb.max.y;
			node.max[2] = innerNodes[curNode].aabb.max.z;

			// Initialize the children pointers
			for (int i = 0; i < 4; i++)
				node.children[i] = -1;

			// Pull up the grandchildren
			std::vector<EncodeItem> items;
			if (g_bEnableQBVHwSAH)
				items = PullUpChildrenSAH(curNode, innerNodes, leafItems);
			else
				items = PullUpChildren(curNode, innerNodes, leafItems);

			for (const auto& item : items)
			{
				node.children[nextchild++] = nextNode++;
				Q.push(item);
			}

			// Encode the inner node
			EncodeInnerNode(result, &node, EncodeIdx++);
		}
	}

	return result;
}

void CheckTree(InnerNode4* innerNodes, int curNode)
{
	if (innerNodes[curNode].numChildren < 0 || innerNodes[curNode].numChildren > 4) {
		log_debug("[DBG] [BVH] ERROR. node: %d, has numChildren: %d", curNode, innerNodes[curNode].numChildren);
		return;
	}
	for (int i = 0; i < 4; i++) {
		if (!(innerNodes[curNode].isLeaf[i]))
			CheckTree(innerNodes, innerNodes[curNode].children[i]);
	}
}

int CalcMaxDepth(IGenericTreeNode* node)
{
	if (node->IsLeaf())
	{
		return 1;
	}

	std::vector<IGenericTreeNode*>children = node->GetChildren();
	int max_depth = 0;
	for (const auto &child : children) {
		int depth = CalcMaxDepth(child);
		max_depth = max(1 + depth, max_depth);
	}
	return max_depth;
}

void CalcOccupancy(IGenericTreeNode* node, int &OccupiedNodes_out, int &TotalNodes_out)
{
	if (node->IsLeaf())
	{
		OccupiedNodes_out = 0; TotalNodes_out = 0;
		return;
	}

	std::vector<IGenericTreeNode*>children = node->GetChildren();
	TotalNodes_out = 4;
	OccupiedNodes_out = children.size();
	for (const auto& child : children) {
		int Occupancy, TotalNodes;
		CalcOccupancy(child, Occupancy, TotalNodes);
		OccupiedNodes_out += Occupancy;
		TotalNodes_out += TotalNodes;
	}
}

int CountNodes(IGenericTreeNode* node)
{
	if (node->IsLeaf())
	{
		return 1;
	}

	std::vector<IGenericTreeNode*>children = node->GetChildren();
	int temp = 1; // Count this node
	for (const auto& child : children) {
		temp += CountNodes(child);
	}
	return temp;
}

void ComputeTreeStats(IGenericTreeNode* root)
{
	// Get the maximum depth of the tree
	int max_depth = CalcMaxDepth(root);
	int TotalNodes = 0, Occupancy = 0;
	CalcOccupancy(root, Occupancy, TotalNodes);
	float OccupancyPerc = (float)Occupancy / (float)TotalNodes * 100.0f;
	log_debug("[DBG] [BVH] max_depth: %d, Occupancy: %d, TotalNodes: %d, OccupancyPerc: %0.3f",
		max_depth, Occupancy, TotalNodes, OccupancyPerc);
}

TreeNode* RotLeft(TreeNode* T)
{
	if (T == nullptr) return nullptr;

	const int rootNumNodes = T->numNodes;
	TreeNode* L = T->left;
	TreeNode* R = T->right;
	TreeNode* RL = T->right->left;
	TreeNode* RR = T->right->right;
	R->right = RR;
	R->left = T;
	T->left = L;
	T->right = RL;

	T->numNodes = T->left->numNodes + T->right->numNodes + 1;
	R->numNodes = R->left->numNodes + R->right->numNodes + 1;
	return R;
}

TreeNode* RotRight(TreeNode* T)
{
	if (T == nullptr) return nullptr;

	const int rootNumNodes = T->numNodes;
	TreeNode* L = T->left;
	TreeNode* R = T->right;
	TreeNode* LL = T->left->left;
	TreeNode* LR = T->left->right;
	L->left = LL;
	L->right = T;
	T->left = LR;
	T->right = R;

	T->numNodes = T->left->numNodes + T->right->numNodes + 1;
	L->numNodes = L->left->numNodes + L->right->numNodes + 1;
	return L;
}

// Red-Black balanced insertion
TreeNode* InsertRB(TreeNode* T, int TriID, MortonCode_t code, const AABB &box, const Matrix4 &m)
{
	if (T == nullptr)
	{
		return new TreeNode(TriID, code, box, m);
	}

	// Avoid duplicate meshes? The same mesh, but with a different transform matrix may
	// appear multiple times. However, it's also true that the same mesh, but with different
	// face groups may also appear multiple times, creating duplicate entries in the tree.
	// So, to be sure, we need to check the mesh ID and the transform matrix
	if (T->TriID == TriID && T->m == m)
		return T;

	if (code <= T->code)
	{
		T->left = InsertRB(T->left, TriID, code, box, m);
		// Rebalance
		if (T->left->left != nullptr && T->left->red && T->left->left->red)
		{
			T = RotRight(T);
			T->left->red = false;
			T->right->red = false;
		}
		else if (T->left->right != nullptr && T->left->red && T->left->right->red)
		{
			T->left = RotLeft(T->left);
			T = RotRight(T);
			T->left->red = false;
			T->right->red = false;
		}
	}
	else
	{
		T->right = InsertRB(T->right, TriID, code, box, m);
		// Rebalance
		if (T->right->right != nullptr && T->right->red && T->right->right->red)
		{
			T = RotLeft(T);
			T->left->red = false;
			T->right->red = false;
		}
		else if (T->right->left != nullptr && T->right->red && T->right->left->red)
		{
			T->right = RotRight(T->right);
			T = RotLeft(T);
			T->left->red = false;
			T->right->red = false;
		}
	}

	// In an RB tree, all nodes (including the inner nodes) contain primitives.
	// We would need an additional AABB to represent the box that spans all
	// primitives under the current node
	/*
	T->m.identity();
	T->box.SetInfinity();
	if (T->left != nullptr) {
		if (T->left->IsLeaf())
			T->box.Expand(T->left->GetAABBFromOOBB());
		else
			T->box.Expand(T->left->box);
	}
	if (T->right != nullptr) {
		if (T->right->IsLeaf())
			T->box.Expand(T->right->GetAABBFromOOBB());
		else
			T->box.Expand(T->right->box);
	}
	*/
	return T;
}

void DeleteRB(TreeNode* T)
{
	if (T == nullptr)
		return;
	DeleteRB(T->left);
	DeleteRB(T->right);
}

int DumpRBToOBJ(FILE* file, TreeNode* T, const std::string &name, int VerticesCountOffset)
{
	if (T == nullptr)
		return VerticesCountOffset;

	Matrix4 S1;
	S1.scale(OPT_TO_METERS, -OPT_TO_METERS, OPT_TO_METERS);
	T->box.UpdateLimits();
	T->box.TransformLimits(S1 * T->m);

	VerticesCountOffset = T->box.DumpLimitsToOBJ(file, name, VerticesCountOffset);
	VerticesCountOffset = DumpRBToOBJ(file, T->left, name + "L", VerticesCountOffset);
	VerticesCountOffset = DumpRBToOBJ(file, T->right, name + "R", VerticesCountOffset);
	return VerticesCountOffset;
}

#undef DEBUG_AVL
//#define DEBUG_AVL 1

void Refit(TreeNode* T)
{
	if (T == nullptr)
		return;

	if (T->IsLeaf())
		return;

	Refit(T->left);
	Refit(T->right);

	T->box.SetInfinity();
	T->box.Expand(T->left->box);
	T->box.Expand(T->right->box);
}

// AVL-based BVH.
TreeNode* InsertAVL(TreeNode* T, int TriID, const XwaVector3& centroid, const AABB &box)
{
	if (T == nullptr)
	{
		return new TreeNode(TriID, centroid, box);
	}

	if (T->TriID != -1)
	{
		// We have reached a leaf, create a new inner node
		AABB newBox = box;
		newBox.Expand(T->box);
		const int dim = newBox.GetLargestDimension();
		const float split = 0.5f * (newBox.min[dim] + newBox.max[dim]);
		const bool goLeft = centroid[dim] <= split;

		TreeNode* newNode = new TreeNode(-1);
		TreeNode* newLeaf = new TreeNode(TriID, centroid, box);

		newNode->box = newBox;
		newNode->numNodes = 3;
		if (goLeft)
		{
			newNode->left = newLeaf;
			newNode->right = T;
		}
		else
		{
			newNode->left = T;
			newNode->right = newLeaf;
		}
		return newNode;
	}

	AABB &innerBox = T->box;
	const int dim = innerBox.GetLargestDimension();
	const float split = 0.5f * (innerBox.min[dim] + innerBox.max[dim]);
#ifdef DEBUG_AVL
	log_debug("[DBG] [BVH] centroid[%d]:%0.3f, split: %0.3f, %s",
		dim,
		centroid[dim],
		split,
		innerBox.ToString().c_str());
#endif
	if (centroid[dim] <= split)
	{
		T->left = InsertAVL(T->left, TriID, centroid, box);
		T->numNodes += 2;
		T->bal--;

		if (T->bal == -2)
		{
			if (T->left->bal > 0)
				T->left = RotLeft(T->left);
			T = RotRight(T);
			T->bal = 0;
			T->left->bal = 0;
			T->right->bal = 0;
		}

		// Refit
		//T->box = T->left->box;
		//T->box.Expand(T->right->box);
	}
	else
	{
		T->right = InsertAVL(T->right, TriID, centroid, box);
		T->numNodes += 2;
		T->bal++;

		if (T->bal == 2)
		{
			if (T->right->bal < 0)
				T->right = RotRight(T->right);
			T = RotLeft(T);
			T->bal = 0;
			T->left->bal = 0;
			T->right->bal = 0;
		}

		// Refit
		//T->box = T->left->box;
		//T->box.Expand(T->right->box);
	}
	Refit(T);

	return T;
}

// Regular Binary Tree insertion. Let's pre-sort the primitives, aye?
TreeNode* InsertTree(TreeNode* T, int TriID, const XwaVector3& centroid, const AABB& box)
{
	if (T == nullptr)
	{
		return new TreeNode(TriID, centroid, box);
	}

	if (T->TriID != -1)
	{
		// We have reached a leaf, create a new inner node
		AABB newBox = box;
		newBox.Expand(T->box);
		const int dim = newBox.GetLargestDimension();
		const float split = 0.5f * (newBox.min[dim] + newBox.max[dim]);
		const bool goLeft = centroid[dim] <= split;

		TreeNode* newNode = new TreeNode(-1);
		TreeNode* newLeaf = new TreeNode(TriID, centroid, box);

		newNode->box = newBox;
		newNode->numNodes = 3;
		if (goLeft)
		{
			newNode->left = newLeaf;
			newNode->right = T;
		}
		else
		{
			newNode->left = T;
			newNode->right = newLeaf;
		}
		return newNode;
	}

	AABB& innerBox = T->box;
	const int dim = innerBox.GetLargestDimension();
	const float split = 0.5f * (innerBox.min[dim] + innerBox.max[dim]);
#ifdef DEBUG_AVL
	log_debug("[DBG] [BVH] centroid[%d]:%0.3f, split: %0.3f, %s",
		dim,
		centroid[dim],
		split,
		innerBox.ToString().c_str());
#endif
	if (centroid[dim] <= split)
	{
		T->left = InsertAVL(T->left, TriID, centroid, box);
	}
	else
	{
		T->right = InsertAVL(T->right, TriID, centroid, box);
	}
	// Refit
	T->box.Expand(box);

	return T;
}

void InOrder(TreeNode* T, std::vector<LeafItem> &result)
{
	if (T == nullptr)
		return;
	InOrder(T->left, result);
	if (T->TriID != -1)
		result.push_back({0, T->box.GetCentroidVector3(), T->box, T->TriID});
	InOrder(T->right, result);
}

// Function to swap two elements
void swap(LeafItem* a, LeafItem* b)
{
	LeafItem t = *a;
	*a = *b;
	*b = t;
}

// Partition the array using the last element as the pivot
int partition(LeafItem arr[], int low, int high)
{
	// Choosing the pivot
	LeafItem pivot = arr[high];

	// Index of smaller element and indicates
	// the right position of pivot found so far
	int i = (low - 1);

	for (int j = low; j <= high - 1; j++) {

		// If current element is smaller than the pivot
		if (arr[j].code < pivot.code) {

			// Increment index of smaller element
			i++;
			swap(&arr[i], &arr[j]);
		}
	}
	swap(&arr[i + 1], &arr[high]);
	return (i + 1);
}

// The main function that implements QuickSort
// From https://www.geeksforgeeks.org/quick-sort/#
// arr[] --> Array to be sorted,
// low --> Starting index,
// high --> Ending index
void QuickSort(LeafItem arr[], int low, int high)
{
	if (low < high)
	{
		// pi is partitioning index, arr[p]
		// is now at right place
		int pi = partition(arr, low, high);

		// Separately sort elements before
		// partition and after partition
		QuickSort(arr, low, pi - 1);
		QuickSort(arr, pi + 1, high);
	}
}

LBVH* LBVH::Build(const XwaVector3* vertices, const int numVertices, const int *indices, const int numIndices)
{
	// Get the scene limits
	AABB sceneBox;
	XwaVector3 range;
	for (int i = 0; i < numVertices; i++)
		sceneBox.Expand(vertices[i]);
	range.x = sceneBox.max.x - sceneBox.min.x;
	range.y = sceneBox.max.y - sceneBox.min.y;
	range.z = sceneBox.max.z - sceneBox.min.z;

	int numTris = numIndices / 3;
	/*
	log_debug("[DBG] [BVH] numVertices: %d, numIndices: %d, numTris: %d, scene: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
		numVertices, numIndices, numTris,
		sceneBox.min.x, sceneBox.min.y, sceneBox.min.z,
		sceneBox.max.x, sceneBox.max.y, sceneBox.max.z);
	*/

	// Get the Morton Code and AABB for each triangle.
	std::vector<LeafItem> leafItems;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		aabb.Expand(vertices[indices[i + 0]]);
		aabb.Expand(vertices[indices[i + 1]]);
		aabb.Expand(vertices[indices[i + 2]]);

		Vector3 centroid = aabb.GetCentroidVector3();
		Normalize(centroid, sceneBox, range);
		MortonCode_t m = GetMortonCode(centroid);
		leafItems.push_back({ m, Vector3(), aabb, TriID });
	}

	// Sort the morton codes
	//std::sort(leafItems.begin(), leafItems.end(), leafSorter);
	QuickSort(leafItems.data(), 0, leafItems.size() - 1);

	// Build the tree
	int root = -1;
	InnerNode* innerNodes = FastLBVH(leafItems, &root);
	//log_debug("[DBG] [BVH] FastLBVH finished. Tree built. root: %d", root);

	//char sFileName[80];
	//sprintf_s(sFileName, 80, ".\\BLAS-%d.obj", meshIndex);
	//DumpInnerNodesToOBJ(sFileName, root, innerNodes, leafItems, vertices, indices);

	// Convert to QBVH
	//QTreeNode *Q = BinTreeToQTree(root, leafItems.size() == 1, innerNodes, leafItems);
	//delete[] innerNodes;
	// Encode the QBVH in a buffer
	//void *buffer = EncodeNodes(Q, vertices, indices);

	int numNodes = 0;
	BVHNode* buffer = EncodeNodesAsQBVHwSAH(root, innerNodes, leafItems, vertices, indices, numNodes);
	delete[] innerNodes;

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Buffer");
	//PrintTreeBuffer("", buffer, 0);

	LBVH *lbvh = new LBVH();
	lbvh->nodes = (BVHNode *)buffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	//lbvh->numNodes = Q->numNodes;
	lbvh->numNodes = numNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;
	// DEBUG
	//log_debug("[DBG} [BVH] Dumping file: %s", sFileName);
	//lbvh->DumpToOBJ(sFileName);
	//log_debug("[DBG] [BVH] BLAS dumped");
	// DEBUG

	// Tidy up
	//DeleteTree(Q);
	return lbvh;
}

// Parallel Locally-Ordered Clustering BVH builder
LBVH* LBVH::BuildPLOC(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	// Get the scene limits
	AABB sceneBox;
	XwaVector3 range;
	for (int i = 0; i < numVertices; i++)
		sceneBox.Expand(vertices[i]);
	range.x = sceneBox.max.x - sceneBox.min.x;
	range.y = sceneBox.max.y - sceneBox.min.y;
	range.z = sceneBox.max.z - sceneBox.min.z;

	int numTris = numIndices / 3;

	// Get the Morton Code and AABB for each triangle.
	//TreeNode* T = nullptr;
	std::vector<LeafItem> leafItems;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		aabb.Expand(vertices[indices[i + 0]]);
		aabb.Expand(vertices[indices[i + 1]]);
		aabb.Expand(vertices[indices[i + 2]]);

		Vector3 centroid = aabb.GetCentroidVector3();
		Normalize(centroid, sceneBox, range);
		MortonCode_t m = GetMortonCode(centroid);
		leafItems.push_back({ m, centroid, aabb, TriID });
		//T = InsertTree(T, TriID, centroid, aabb);
		//T = InsertAVL(T, TriID, centroid, aabb);
	}

	// Sort the morton codes
	std::sort(leafItems.begin(), leafItems.end(), leafSorter);
	//InOrder(T, leafItems);
	//DeleteTree(T);

	// Build the tree
	int root = -1;
	InnerNode* innerNodes = PLOC(leafItems, root);

	//char sFileName[80];
	//sprintf_s(sFileName, 80, ".\\BLAS-%d.obj", meshIndex);
	//DumpInnerNodesToOBJ(sFileName, root, innerNodes, leafItems, vertices, indices);

	// Convert to QBVH
	//QTreeNode *Q = BinTreeToQTree(root, leafItems.size() == 1, innerNodes, leafItems);
	//delete[] innerNodes;
	// Encode the QBVH in a buffer
	//void *buffer = EncodeNodes(Q, vertices, indices);

	int numNodes = 0;
	BVHNode* buffer = EncodeNodesAsQBVHwSAH(root, innerNodes, leafItems, vertices, indices, numNodes);
	delete[] innerNodes;

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Buffer");
	//PrintTreeBuffer("", buffer, 0);

	LBVH* lbvh = new LBVH();
	lbvh->nodes = (BVHNode*)buffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	//lbvh->numNodes = Q->numNodes;
	lbvh->numNodes = numNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;
	// DEBUG
	//log_debug("[DBG} [BVH] Dumping file: %s", sFileName);
	//lbvh->DumpToOBJ(sFileName);
	//log_debug("[DBG] [BVH] BLAS dumped");
	// DEBUG

	// Tidy up
	//DeleteTree(Q);
	return lbvh;
}

LBVH* LBVH::BuildQBVH(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	// Get the scene limits
	AABB sceneBox;
	XwaVector3 range;
	for (int i = 0; i < numVertices; i++)
		sceneBox.Expand(vertices[i]);
	range.x = sceneBox.max.x - sceneBox.min.x;
	range.y = sceneBox.max.y - sceneBox.min.y;
	range.z = sceneBox.max.z - sceneBox.min.z;

	int numTris = numIndices / 3;
	//log_debug("[DBG] [BVH] numVertices: %d, numIndices: %d, numTris: %d, scene: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
	//	numVertices, numIndices, numTris,
	//	sceneBox.min.x, sceneBox.min.y, sceneBox.min.z,
	//	sceneBox.max.x, sceneBox.max.y, sceneBox.max.z);

	// Get the Morton Code and AABB for each triangle.
	std::vector<LeafItem> leafItems;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		aabb.Expand(vertices[indices[i + 0]]);
		aabb.Expand(vertices[indices[i + 1]]);
		aabb.Expand(vertices[indices[i + 2]]);

		Vector3 centroid = aabb.GetCentroidVector3();
		Normalize(centroid, sceneBox, range);
		MortonCode_t m = GetMortonCode(centroid);
		leafItems.push_back({ m, Vector3(), aabb, TriID });
	}

	// Sort the morton codes
	std::sort(leafItems.begin(), leafItems.end(), leafSorter);

	// Build the tree
	int root = -1;
	InnerNode4* innerNodes = FastLQBVH(leafItems, root);
	int totalNodes = innerNodes[root].totalNodes;
	//log_debug("[DBG] [BVH] FastLQBVH* finished. QTree built. root: %d, totalNodes: %d", root, totalNodes);
	AABB scene = innerNodes[root].aabb;
	//log_debug("[DBG] [BVH] scene size from the QTree: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
	//	scene.min.x, scene.min.y, scene.min.z,
	//	scene.max.x, scene.max.y, scene.max.z);

	// Encode the QBVH in a buffer
	BVHNode *buffer = (BVHNode * )EncodeNodes(root, innerNodes, leafItems, vertices, indices);
	delete[] innerNodes;
	// Initialize the root
	buffer[0].rootIdx = 0;

	LBVH* lbvh = new LBVH();
	lbvh->nodes = (BVHNode*)buffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	lbvh->numNodes = totalNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;
	// DEBUG
	//log_debug("[DBG} [BVH] Dumping file: %s", sFileName);
	//lbvh->DumpToOBJ(sFileName);
	//log_debug("[DBG] [BVH] BLAS dumped");
	// DEBUG

	// Tidy up
	//delete[] buffer; // We can't delete the buffer here, lbvh->nodes now owns it
	return lbvh;
}

LBVH* LBVH::BuildFastQBVH(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	// Get the scene limits
	AABB sceneBox;
	XwaVector3 range;
	for (int i = 0; i < numVertices; i++)
		sceneBox.Expand(vertices[i]);
	range.x = sceneBox.max.x - sceneBox.min.x;
	range.y = sceneBox.max.y - sceneBox.min.y;
	range.z = sceneBox.max.z - sceneBox.min.z;

	int numTris = numIndices / 3;
	const int numQBVHInnerNodes = CalcNumInnerQBVHNodes(numTris);
	const int numQBVHNodes = numTris + numQBVHInnerNodes;
	/*
	log_debug("[DBG] [BVH] numVertices: %d, numIndices: %d, numTris: %d, numQBVHInnerNodes: %d, numQBVHNodes: %d, "
		"scene: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
		numVertices, numIndices, numTris, numQBVHInnerNodes, numQBVHNodes,
		sceneBox.min.x, sceneBox.min.y, sceneBox.min.z,
		sceneBox.max.x, sceneBox.max.y, sceneBox.max.z);
	*/

	// We can reserve the buffer for the QBVH now.
	BVHNode* QBVHBuffer = new BVHNode[numQBVHNodes];
	//log_debug("[DBG] [BVH] sizeof(BVHNode): %d", sizeof(BVHNode));

	// Get the Morton Code and AABB for each triangle.
	std::vector<LeafItem> leafItems;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		XwaVector3 v0 = vertices[indices[i + 0]];
		XwaVector3 v1 = vertices[indices[i + 1]];
		XwaVector3 v2 = vertices[indices[i + 2]];
		aabb.Expand(v0);
		aabb.Expand(v1);
		aabb.Expand(v2);

		Vector3 centroid = aabb.GetCentroidVector3();
		Normalize(centroid, sceneBox, range);
		MortonCode_t m = GetMortonCode(centroid);
		leafItems.push_back({ m, Vector3(), aabb, TriID });
	}

	// Sort the morton codes
	std::sort(leafItems.begin(), leafItems.end(), leafSorter);

	// Encode the sorted leaves
	// TODO: Encode the leaves before sorting, and use TriID as the sort index.
	//int LeafOfs = numQBVHInnerNodes * sizeof(BVHNode) / 4;
	int LeafEncodeIdx = numQBVHInnerNodes;
	for (unsigned int i = 0; i < leafItems.size(); i++)
	{
		EncodeLeafNode(QBVHBuffer, leafItems, i, LeafEncodeIdx++, vertices, indices);
	}

	// Build, convert and encode the QBVH
	int root = -1;
	SingleStepFastLQBVH(QBVHBuffer, numQBVHInnerNodes, leafItems, root);
	//log_debug("[DBG] [BVH] FastLQBVH** finished. QTree built. root: %d, numQBVHNodes: %d", root, numQBVHNodes);
	int totalNodes = numQBVHNodes;
	//log_debug("[DBG] [BVH] Checking tree...");
	//CheckTree(innerNodes, inner_root);
	//log_debug("[DBG] [BVH] Tree checked.");

	/*
	const bool bDoPostEncode = true;
	if (bDoPostEncode)
	{
		innerNodes[inner_root].totalNodes = totalNodes;
		delete[] QBVHBuffer;
		QBVHBuffer = (BVHNode*)EncodeNodes(inner_root, innerNodes, leafItems, vertices, indices);
		QBVHBuffer[0].rootIdx = 0;
		delete[] innerNodes;
		log_debug("[DBG] [BVH] Buffer post-encoded");
	}
	else
	*/
	{
		// Initialize the root
		QBVHBuffer[0].rootIdx = root;
	}

	//log_debug("[DBG] [BVH] FastLQBVH** finished. QTree built. root: %d, numQBVHNodes: %d, totalNodes: %d",
	//	root, numQBVHNodes, totalNodes);
	/*
	AABB scene;
	scene.min.x = QBVHBuffer[root].min[0];
	scene.min.y = QBVHBuffer[root].min[1];
	scene.min.z = QBVHBuffer[root].min[2];
	scene.max.x = QBVHBuffer[root].max[0];
	scene.max.y = QBVHBuffer[root].max[1];
	scene.max.z = QBVHBuffer[root].max[2];
	log_debug("[DBG] [BVH] scene size from the QTree: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
		scene.min.x, scene.min.y, scene.min.z,
		scene.max.x, scene.max.y, scene.max.z);
	*/
	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Tree");
	//printTree(0, inner_root, false, innerNodes);
	//delete[] innerNodes;

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Buffer");
	//PrintTreeBuffer("", QBVHBuffer, root);

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Buffer");
	//PrintTreeBuffer("", QBVHBuffer + root, 0);
	//PrintTreeBuffer("", QBVHBuffer, root);

	LBVH* lbvh = new LBVH();
	//lbvh->rawBuffer = QBVHBuffer;
	//lbvh->nodes = QBVHBuffer + root;
	lbvh->nodes = QBVHBuffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	lbvh->numNodes = totalNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;
	// DEBUG
	//log_debug("[DBG} [BVH] Dumping file: %s", sFileName);
	//lbvh->DumpToOBJ(sFileName);
	//log_debug("[DBG] [BVH] BLAS dumped");
	// DEBUG

	// Tidy up
	//delete[] buffer; // We can't delete the buffer here, lbvh->nodes now owns it
	return lbvh;
}

/// <summary>
/// Builds a BVH2 using the DirectBVH method. No Morton Codes, no explicit sort algorithm.
/// The BVH2 is then converted to a QBVH.
/// </summary>
LBVH* LBVH::BuildDirectBVH2CPU(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	// Get the scene limits
	AABB sceneBox;
	for (int i = 0; i < numVertices; i++)
		sceneBox.Expand(vertices[i]);

	int numPrimitives = numIndices / 3;
	/*
	log_debug("[DBG] [BVH] numVertices: %d, numIndices: %d, numTris: %d, scene: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
		numVertices, numIndices, numTris,
		sceneBox.min.x, sceneBox.min.y, sceneBox.min.z,
		sceneBox.max.x, sceneBox.max.y, sceneBox.max.z);
	*/

	// Get the centroid and AABB for each triangle.
	std::vector<LeafItem> leafItems;
	AABB centroidBox;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		aabb.Expand(vertices[indices[i + 0]]);
		aabb.Expand(vertices[indices[i + 1]]);
		aabb.Expand(vertices[indices[i + 2]]);

		Vector3 centroid = aabb.GetCentroidVector3();
		centroidBox.Expand(centroid);
		leafItems.push_back({ 0, centroid, aabb, TriID });
	}

	// Build the tree
	int root = -1;
	InnerNode* innerNodes = DirectBVH2BuilderCPU(sceneBox, centroidBox, leafItems, root);
	if (innerNodes == nullptr)
	{
		log_debug("[DBG] [BVH] DirectBVH2BuilderCPU failed");
		return nullptr;
	}

	//char sFileName[80];
	//sprintf_s(sFileName, 80, ".\\BLAS-%d.obj", meshIndex);
	//DumpInnerNodesToOBJ(sFileName, root, innerNodes, leafItems, vertices, indices);

	// Convert to QBVH
	//QTreeNode *Q = BinTreeToQTree(root, leafItems.size() == 1, innerNodes, leafItems);
	//delete[] innerNodes;
	// Encode the QBVH in a buffer
	//void *buffer = EncodeNodes(Q, vertices, indices);

	int numNodes = 0;
	BVHNode* buffer = EncodeNodesAsQBVHwSAH(root, innerNodes, leafItems, vertices, indices, numNodes);
	delete[] innerNodes;

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Buffer");
	//PrintTreeBuffer("", buffer, 0);

	LBVH* lbvh = new LBVH();
	lbvh->nodes = (BVHNode*)buffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	//lbvh->numNodes = Q->numNodes;
	lbvh->numNodes = numNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;
	// DEBUG
	//log_debug("[DBG} [BVH] Dumping file: %s", sFileName);
	//lbvh->DumpToOBJ(sFileName);
	//log_debug("[DBG] [BVH] BLAS dumped");
	// DEBUG

	// Tidy up
	//DeleteTree(Q);
	return lbvh;
}

/// <summary>
/// Same as BuildDirectBVH2(), but uses the GPU-Friendly version.
/// Builds a BVH2 using the DirectBVH method. No Morton Codes, no explicit sort algorithm.
/// The BVH2 is then converted to a QBVH.
/// </summary>
LBVH* LBVH::BuildDirectBVH2GPU(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	// Get the scene limits
	AABB sceneBox;
	for (int i = 0; i < numVertices; i++)
		sceneBox.Expand(vertices[i]);

	int numPrimitives = numIndices / 3;
	/*
	log_debug("[DBG] [BVH] numVertices: %d, numIndices: %d, numTris: %d, scene: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
		numVertices, numIndices, numTris,
		sceneBox.min.x, sceneBox.min.y, sceneBox.min.z,
		sceneBox.max.x, sceneBox.max.y, sceneBox.max.z);
	*/

	// Get the centroid and AABB for each triangle.
	std::vector<LeafItem> leafItems;
	AABB centroidBox;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		aabb.Expand(vertices[indices[i + 0]]);
		aabb.Expand(vertices[indices[i + 1]]);
		aabb.Expand(vertices[indices[i + 2]]);

		Vector3 centroid = aabb.GetCentroidVector3();
		centroidBox.Expand(centroid);
		leafItems.push_back({ 0, centroid, aabb, TriID });
	}

	// Build the tree
	int root = -1;
	InnerNode* innerNodes = DirectBVH2BuilderGPU(centroidBox, leafItems, root);
	if (innerNodes == nullptr)
	{
		log_debug("[DBG] [BVH] DirectBVH2BuilderGPU failed");
		return nullptr;
	}

	//char sFileName[80];
	//sprintf_s(sFileName, 80, ".\\BLAS-%d.obj", meshIndex);
	//DumpInnerNodesToOBJ(sFileName, root, innerNodes, leafItems, vertices, indices);

	// Convert to QBVH
	//QTreeNode *Q = BinTreeToQTree(root, leafItems.size() == 1, innerNodes, leafItems);
	//delete[] innerNodes;
	// Encode the QBVH in a buffer
	//void *buffer = EncodeNodes(Q, vertices, indices);

	int numNodes = 0;
	BVHNode* buffer = EncodeNodesAsQBVHwSAH(root, innerNodes, leafItems, vertices, indices, numNodes);
	delete[] innerNodes;

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Buffer");
	//PrintTreeBuffer("", buffer, 0);

	LBVH* lbvh = new LBVH();
	lbvh->nodes = (BVHNode*)buffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	//lbvh->numNodes = Q->numNodes;
	lbvh->numNodes = numNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;
	// DEBUG
	//log_debug("[DBG} [BVH] Dumping file: %s", sFileName);
	//lbvh->DumpToOBJ(sFileName);
	//log_debug("[DBG] [BVH] BLAS dumped");
	// DEBUG

	// Tidy up
	//DeleteTree(Q);
	return lbvh;
}

/// <summary>
/// Same as BuildDirectBVH2(), but uses the GPU-Friendly version and builds a QBVH directly.
/// Builds a BVH4 using the DirectBVH method. No Morton Codes, no explicit sort algorithm.
/// </summary>
LBVH* LBVH::BuildDirectBVH4GPU(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	// Get the scene limits
	AABB sceneBox;
	//for (int i = 0; i < numVertices; i++)
	//	sceneBox.Expand(vertices[i]);

	const int numPrimitives = numIndices / 3;

	/*
	log_debug("[DBG] [BVH] numVertices: %d, numIndices: %d, numTris: %d, scene: (%0.3f, %0.3f, %0.3f)-(%0.3f, %0.3f, %0.3f)",
		numVertices, numIndices, numTris,
		sceneBox.min.x, sceneBox.min.y, sceneBox.min.z,
		sceneBox.max.x, sceneBox.max.y, sceneBox.max.z);
	*/

	// Get the centroid and AABB for each triangle.
	std::vector<LeafItem> leafItems;
	AABB centroidBox;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		aabb.Expand(vertices[indices[i + 0]]);
		aabb.Expand(vertices[indices[i + 1]]);
		aabb.Expand(vertices[indices[i + 2]]);

		Vector3 centroid = aabb.GetCentroidVector3();
		centroidBox.Expand(centroid);
		leafItems.push_back({ 0, centroid, aabb, TriID });
		sceneBox.Expand(aabb);
	}

	// Build the tree
	int root = -1;
	int numNodes = 0;
	BVHNode* buffer = DirectBVH4BuilderGPU(centroidBox, leafItems, numNodes, vertices, indices);
	if (buffer == nullptr)
	{
		log_debug("[DBG] [BVH] DirectBVH4BuilderGPU failed");
		return nullptr;
	}

	// DEBUG
	/*log_debug("[DBG] [BVH] sceneBox: %s", sceneBox.ToString().c_str());
	log_debug("[DBG] [BVH]  rootbox: (%0.6f, %0.6f, %0.6f)-(%0.6f, %0.6f, %0.6f)",
		buffer[0].min[0], buffer[0].min[1], buffer[0].min[2],
		buffer[0].max[0], buffer[0].max[1], buffer[0].max[2]);*/

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Buffer");
	//PrintTreeBuffer("", buffer, 0);

	LBVH* lbvh = new LBVH();
	lbvh->nodes = (BVHNode*)buffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	lbvh->numNodes = numNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;
	// DEBUG
	/*
	char* sFileName = ".\\Test.obj";
	log_debug("[DBG} [BVH] Dumping file: %s", sFileName);
	lbvh->DumpToOBJ(sFileName);
	log_debug("[DBG] [BVH] BLAS dumped");
	*/
	// DEBUG

	// Tidy up
	//DeleteTree(Q);
	return lbvh;
}

LBVH* LBVH::BuildAVL(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	const int numPrimitives = numIndices / 3;

	// Get the centroid and AABB for each triangle.
	std::vector<LeafItem> leafItems;
	AABB centroidBox;
	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		aabb.Expand(vertices[indices[i + 0]]);
		aabb.Expand(vertices[indices[i + 1]]);
		aabb.Expand(vertices[indices[i + 2]]);

		Vector3 centroid = aabb.GetCentroidVector3();
		leafItems.push_back({ 0, centroid, aabb, TriID });
	}

	int numNodes = 0;
	BVHNode* buffer = AVLBuilder(leafItems, numNodes, vertices, indices);
	log_debug("[DBG] [BVH] AVLBuilder succeeded. numPrims: %d, numNodes: %d",
		leafItems.size(), numNodes);

	LBVH* lbvh = new LBVH();
	lbvh->nodes = (BVHNode*)buffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	lbvh->numNodes = numNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;

	//lbvh->DumpToOBJ(".\\test.obj");

	return lbvh;
}

// https://github.com/embree/embree/blob/master/tutorials/bvh_builder/bvh_builder_device.cpp
struct BuildData
{
	int numVertices = 0;
	int numIndices = 0;
	//int numQBVHInnerNodes = 0;
	//int numQBVHNodes = 0;
	const XwaVector3* vertices = nullptr;
	const int* indices = nullptr;

	//LONG* pInnerNodeEncodeOfs = nullptr;
	//LONG* pLeafNodeEncodeOfs = nullptr;
	LONG* pTotalNodes = nullptr;
	std::vector<RTCBuildPrimitive> prims;
	BVHNode* QBVHBuffer = nullptr;
	QTreeNode* QTree = nullptr;
	RTCBVH bvh = nullptr;
	std::map<NodeChildKey, AABB> nodeToABBMap;

	BuildData(int numTris, const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
	{
		this->numVertices = numVertices;
		this->numIndices = numIndices;
		this->vertices = vertices;
		this->indices = indices;

		//numQBVHInnerNodes = CalcNumInnerQBVHNodes(numTris);
		//numQBVHNodes = 4 * numTris + numQBVHInnerNodes;
		//log_debug("[DBG] [BVH] [EMB] numTris: %d, numQBVHInnerNodes: %d, numQBVHNodes: %d",
		//	numTris, numQBVHInnerNodes, numQBVHNodes);

		// We can reserve the buffer for the QBVH now.
		//QBVHBuffer = new BVHNode[numQBVHNodes];

		prims.reserve(numTris);
		//prims.reserve(numTris + (int)(0.1f * numTris));
		//prims.reserve(numTris + 100);
		//prims.reserve(numTris + numTris);
		prims.resize(numTris);

		pTotalNodes = (LONG*)_aligned_malloc(sizeof(LONG), 32);
		*pTotalNodes = 0;

		//pInnerNodeEncodeOfs = (LONG*)_aligned_malloc(sizeof(LONG), 32);
		//*pInnerNodeEncodeOfs = 0;

		//pLeafNodeEncodeOfs = (LONG*)_aligned_malloc(sizeof(LONG), 32);
		// The first leaf starts at this offset:
		//*pLeafNodeEncodeOfs = numQBVHInnerNodes;

		bvh = g_rtcNewBVH(g_rtcDevice);
	}

	~BuildData()
	{
		//_aligned_free(pInnerNodeEncodeOfs);
		//_aligned_free(pLeafNodeEncodeOfs);
		_aligned_free(pTotalNodes);
		g_rtcReleaseBVH(bvh);
	}
};

#ifdef DISABLED
static void* RTCCreateNode(RTCThreadLocalAllocator alloc, unsigned int numChildren, void* userPtr)
{
	BuildData* buildData = (BuildData*)userPtr;
	BVHNode* QBVHBuffer = buildData->QBVHBuffer;

	InterlockedAdd(buildData->pTotalNodes, 1);
	// Reserve space for the new node
	LONG val = InterlockedAdd(buildData->pInnerNodeEncodeOfs, 1);
	int nodeIndex = val - 1;
	//log_debug("[DBG] [BVH] [EMB] Allocated Inner Node at offset %d, numChildren: %d, total nodes: %d",
	//	nodeIndex, numChildren, *buildData->pTotalNodes);
	if (nodeIndex >= buildData->numQBVHNodes)
	{
		log_debug("[DBG] [BVH] [EMB] ERROR: Exceeded max nodes (I), nodeIndex: %d, numQBVHNodes: %d, totalNodes:",
			nodeIndex, buildData->numQBVHNodes, *buildData->pTotalNodes);
	}
	// Initialize the inner node
	QBVHBuffer[nodeIndex].ref = -1;
	QBVHBuffer[nodeIndex].numChildren = 0;
	QBVHBuffer[nodeIndex].parent = -1;

	QBVHBuffer[nodeIndex].min[0] = 0;
	QBVHBuffer[nodeIndex].min[1] = 0;
	QBVHBuffer[nodeIndex].min[2] = 0;
	QBVHBuffer[nodeIndex].min[3] = 0;

	QBVHBuffer[nodeIndex].max[0] = 0;
	QBVHBuffer[nodeIndex].max[1] = 0;
	QBVHBuffer[nodeIndex].max[2] = 0;
	QBVHBuffer[nodeIndex].max[3] = 0;
	for (int i = 0; i < 4; i++)
		QBVHBuffer[nodeIndex].children[i] = -1;

	return (void*)&(QBVHBuffer[nodeIndex]);
}

static void RTCSetChildren(void* nodePtr, void** childPtr, unsigned int numChildren, void* userPtr)
{
	BuildData* buildData = (BuildData*)userPtr;
	BVHNode* node = (BVHNode*)nodePtr;

	node->numChildren = numChildren;
	uint32_t start_ofs = (uint32_t)(buildData->QBVHBuffer);
	for (size_t i = 0; i < numChildren; i++)
	{
		//((InnerNode*)nodePtr)->children[i] = (Node*)childPtr[i];
		uint32_t ofs = (uint32_t)(childPtr[i]) - (uint32_t)start_ofs;
		int childNodeIdx = ofs / sizeof(BVHNode);
		if (childNodeIdx >= buildData->numQBVHNodes)
		{
			log_debug("[DBG] [BVH] [EMB] ERROR: Exceeded max nodes (C), childNodeIdx: %d, numQBVHNodes: %d, totalNodes: %d",
				childNodeIdx, buildData->numQBVHNodes, *buildData->pTotalNodes);
		}
		node->children[i] = childNodeIdx;
	}
}

static void RTCSetBounds(void* nodePtr, const RTCBounds** bounds, unsigned int numChildren, void* userPtr)
{
	BuildData* buildData = (BuildData*)userPtr;
	//assert(numChildren == 2);
	BVHNode* QBVHBuffer = buildData->QBVHBuffer;
	BVHNode* node = (BVHNode*)nodePtr;
	AABB aabb;

	for (size_t i = 0; i < numChildren; i++)
	{
		int childIdx = node->children[i];
		QBVHBuffer[childIdx].min[0] = bounds[i]->lower_x;
		QBVHBuffer[childIdx].min[1] = bounds[i]->lower_y;
		QBVHBuffer[childIdx].min[2] = bounds[i]->lower_z;
		QBVHBuffer[childIdx].min[3] = 1.0f;

		QBVHBuffer[childIdx].max[0] = bounds[i]->upper_x;
		QBVHBuffer[childIdx].max[1] = bounds[i]->upper_y;
		QBVHBuffer[childIdx].max[2] = bounds[i]->upper_z;
		QBVHBuffer[childIdx].max[3] = 1.0f;

		// Update the AABB for the inner node
		aabb.Expand(bounds[i]);
	}

	node->min[0] = aabb.min.x;
	node->min[1] = aabb.min.y;
	node->min[2] = aabb.min.z;
	node->min[3] = 1.0f;

	node->max[0] = aabb.max.x;
	node->max[1] = aabb.max.y;
	node->max[2] = aabb.max.z;
	node->max[3] = 1.0f;
}

static void* RTCCreateLeaf(RTCThreadLocalAllocator alloc, const RTCBuildPrimitive* prims, size_t numPrims, void* userPtr)
{
	BuildData* buildData = (BuildData*)userPtr;
	BVHPrimNode* QBVHBuffer = (BVHPrimNode*)buildData->QBVHBuffer;

	//assert(numPrims == 1);
	//void* ptr = rtcThreadLocalAlloc(alloc, sizeof(LeafNode), 16);
	InterlockedAdd(buildData->pTotalNodes, 1);
	LONG val = InterlockedAdd(buildData->pLeafNodeEncodeOfs, 1);
	const int nodeIndex = buildData->numQBVHInnerNodes + (val - 1);
	//log_debug("[DBG] [BVH] [EMB] Allocated Leaf Node at offset %d, total nodes: %d",
	//	nodeIndex, *buildData->pTotalNodes);
	if (nodeIndex >= buildData->numQBVHNodes)
	{
		log_debug("[DBG] [BVH] [EMB] ERROR: Exceeded max nodes (L), nodeIndex: %d, numQBVHNodes: %d, totalNodes: %d",
			nodeIndex, buildData->numQBVHNodes, *buildData->pTotalNodes);
	}

	//return (void*) new (ptr) LeafNode(prims->primID, *(BBox3fa*)prims);
	const int TriID = prims->primID;
	const int i = TriID * 3;
	if (i + 2 >= buildData->numIndices)
	{
		log_debug("[DBG] [BVH] [EMB] ERROR: Exceeded maximum geometry index: %d, numIndices",
			i, buildData->numIndices);
	}

	XwaVector3 v0 = buildData->vertices[buildData->indices[i + 0]];
	XwaVector3 v1 = buildData->vertices[buildData->indices[i + 1]];
	XwaVector3 v2 = buildData->vertices[buildData->indices[i + 2]];

	QBVHBuffer[nodeIndex].ref = TriID;

	QBVHBuffer[nodeIndex].v0[0] = v0.x;
	QBVHBuffer[nodeIndex].v0[1] = v0.y;
	QBVHBuffer[nodeIndex].v0[2] = v0.z;
	QBVHBuffer[nodeIndex].v0[3] = 1.0f;

	QBVHBuffer[nodeIndex].v1[0] = v1.x;
	QBVHBuffer[nodeIndex].v1[1] = v1.y;
	QBVHBuffer[nodeIndex].v1[2] = v1.z;
	QBVHBuffer[nodeIndex].v1[3] = 1.0f;

	QBVHBuffer[nodeIndex].v2[0] = v2.x;
	QBVHBuffer[nodeIndex].v2[1] = v2.y;
	QBVHBuffer[nodeIndex].v2[2] = v2.z;
	QBVHBuffer[nodeIndex].v2[3] = 1.0f;

	return (void*)&(QBVHBuffer[nodeIndex]);
}
#endif

#undef RTC_DEBUG
//#define RTC_DEBUG_CRASH 1
#undef RTC_DEBUG_CRASH

static void* RTCCreateNode(RTCThreadLocalAllocator alloc, unsigned int numChildren, void* userPtr)
{
#ifdef RTC_DEBUG_CRASH
	if (userPtr == nullptr) log_debug("[DBG] [DBG] RTCCreateNode, userPtr == nullptr");
	//if (numChildren != 0) log_debug("[DBG] [DBG] RTCCreateNode, numChildren != 0: %d", numChildren);
#endif
	BuildData* buildData = (BuildData*)userPtr;
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

static void* RTCCreateLeaf(RTCThreadLocalAllocator alloc, const RTCBuildPrimitive* prims, size_t numPrims, void* userPtr)
{
#ifdef RTC_DEBUG_CRASH
	if (prims == nullptr) log_debug("[DBG] [DBG] RTCCreateLeaf, prims == nullptr");
	if (numPrims != 1) log_debug("[DBG] [DBG] RTCCreateLeaf, numPrims != 1: %d", numPrims);
	if (userPtr == nullptr) log_debug("[DBG] [DBG] RTCCreateLeaf, userPtr == nullptr");
#endif

	BuildData* buildData = (BuildData*)userPtr;
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

static void RTCSetChildren(void* nodePtr, void** childPtr, unsigned int numChildren, void* userPtr)
{
#ifdef RTC_DEBUG_CRASH
	if (nodePtr == nullptr) log_debug("[DBG] [DBG] RTCSetChildren, nodePtr == nullptr");
	if (childPtr == nullptr) log_debug("[DBG] [DBG] RTCSetChildren, childPtr == nullptr");
	if (userPtr == nullptr) log_debug("[DBG] [DBG] RTCSetChildren, userPtr == nullptr");
	if (numChildren == 0) log_debug("[DBG] [DBG] RTCSetChildren, numChildren == 0");
#endif

	BuildData* buildData = (BuildData*)userPtr;
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

static void RTCSetBounds(void* nodePtr, const RTCBounds** bounds, unsigned int numChildren, void* userPtr)
{
#ifdef RTC_DEBUG_CRASH
	if (nodePtr == nullptr) log_debug("[DBG] [DBG] RTCSetBounds, nodePtr == nullptr");
	if (bounds == nullptr) log_debug("[DBG] [DBG] RTCSetBounds, bounds == nullptr");
	if (userPtr == nullptr) log_debug("[DBG] [DBG] RTCSetBounds, userPtr == nullptr");
	if (numChildren == 0) log_debug("[DBG] [DBG] RTCSetBounds, numChildren == 0");
#endif
	BuildData* buildData = (BuildData*)userPtr;
	//assert(numChildren == 2);
	QTreeNode* node = (QTreeNode*)nodePtr;
	AABB aabb;

	for (size_t i = 0; i < numChildren; i++)
	{
		QTreeNode *child = node->children[i];
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

static void RTCSplitPrimitive(const RTCBuildPrimitive* prim, unsigned int dim, float pos, RTCBounds* lprim, RTCBounds* rprim, void* userPtr)
{
	BuildData* buildData = (BuildData*)userPtr;
	// This function is called many times and it may not spawn new nodes (!). We can't
	// increase pTotalNodes here.
	//InterlockedAdd(buildData->pTotalNodes, 1);
	//assert(dim < 3);
	//assert(prim->geomID == 0);

	lprim->lower_x = prim->lower_x;
	lprim->lower_y = prim->lower_y;
	lprim->lower_z = prim->lower_z;

	lprim->upper_x = prim->upper_x;
	lprim->upper_y = prim->upper_y;
	lprim->upper_z = prim->upper_z;


	rprim->lower_x = prim->lower_x;
	rprim->lower_y = prim->lower_y;
	rprim->lower_z = prim->lower_z;

	rprim->upper_x = prim->upper_x;
	rprim->upper_y = prim->upper_y;
	rprim->upper_z = prim->upper_z;

	//(&lprim->upper_x)[dim] = pos;
	//(&rprim->lower_x)[dim] = pos;
	switch (dim)
	{
	case 0:
		lprim->upper_x = pos;
		rprim->lower_x = pos;
		break;
	case 1:
		lprim->upper_y = pos;
		rprim->lower_y = pos;
		break;
	case 2:
		lprim->upper_z = pos;
		rprim->lower_z = pos;
		break;
	}
}

static bool RTCBuildProgress(void* userPtr, double f) {
	//log_debug("[DBG] [BVH] [EMB] Build Progress: %0.3f", f);
	// Return false to cancel this build
	return true;
}

LBVH* LBVH::BuildEmbree(const XwaVector3* vertices, const int numVertices, const int* indices, const int numIndices)
{
	int numTris = numIndices / 3;
	BuildData buildData(numTris, vertices, numVertices, indices, numIndices);
	//int estimatedInnerNodes = CalcNumInnerQBVHNodes(numTris);
	//log_debug("[DBG] [BVH] [EMB] Estimated nodes: %d", numTris + estimatedInnerNodes);

	for (int i = 0, TriID = 0; i < numIndices; i += 3, TriID++) {
		AABB aabb;
		XwaVector3 v0 = vertices[indices[i + 0]];
		XwaVector3 v1 = vertices[indices[i + 1]];
		XwaVector3 v2 = vertices[indices[i + 2]];
		aabb.Expand(v0);
		aabb.Expand(v1);
		aabb.Expand(v2);

		RTCBuildPrimitive prim;
		prim.geomID = 0;
		prim.primID = TriID;

		prim.lower_x = aabb.min.x;
		prim.lower_y = aabb.min.y;
		prim.lower_z = aabb.min.z;

		prim.upper_x = aabb.max.x;
		prim.upper_y = aabb.max.y;
		prim.upper_z = aabb.max.z;

		buildData.prims[TriID] = prim;
	}

	// Configure the BVH build
	RTCBuildArguments arguments = rtcDefaultBuildArguments();
	arguments.byteSize = sizeof(arguments);
	arguments.buildFlags = RTCBuildFlags::RTC_BUILD_FLAG_NONE;
	arguments.buildQuality = RTCBuildQuality::RTC_BUILD_QUALITY_HIGH;
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
	arguments.createNode = RTCCreateNode;
	arguments.setNodeChildren = RTCSetChildren;
	arguments.setNodeBounds = RTCSetBounds;
	arguments.createLeaf = RTCCreateLeaf;
	//arguments.splitPrimitive = RTCSplitPrimitive;
	arguments.splitPrimitive = nullptr;
	arguments.buildProgress = RTCBuildProgress;
	arguments.userPtr = &buildData;

	// Build the tree
	//log_debug("[DBG] [BVH] [EMB] Building the BVH using Embree");
	//BVHNode* root = (BVHNode *)g_rtcBuildBVH(&arguments);
	QTreeNode* root = (QTreeNode *)g_rtcBuildBVH(&arguments);
	int totalNodes = *(buildData.pTotalNodes);
	//int totalNodes = CountNodes(root);
	root->SetNumNodes(totalNodes);
	//log_debug("[DBG] [BVH] totalNodes: %d, CountNodes: %d", totalNodes, CountNodes(root));

	buildData.QBVHBuffer = (BVHNode *)EncodeNodes(root, vertices, indices);
	// Initialize the root
	buildData.QBVHBuffer[0].rootIdx = 0;
	DeleteTree(root);

	// Initialize the root
	//uint32_t start_ofs = (uint32_t)(buildData.QBVHBuffer);
	//uint32_t root_ofs = (uint32_t)root - start_ofs;
	//log_debug("[DBG] [BVH] [EMB] Root is at offset: %d, total nodes; %d", root_ofs, totalNodes);
	//buildData.QBVHBuffer[0].rootIdx = root_ofs;

	LBVH* lbvh = new LBVH();
	lbvh->nodes = buildData.QBVHBuffer;
	lbvh->numVertices = numVertices;
	lbvh->numIndices = numIndices;
	lbvh->numNodes = totalNodes;
	lbvh->scale = 1.0f;
	lbvh->scaleComputed = true;

	//lbvh->DumpToOBJ(".\\embree.obj", /* isTLAS */ false, /* useMetricScale */ true);
	return lbvh;
}

void DeleteTree(TreeNode* T)
{
	if (T == nullptr) return;

	DeleteTree(T->left);
	DeleteTree(T->right);
	delete T;
}

void DeleteTree(QTreeNode* Q)
{
	if (Q == nullptr)
		return;

	for (int i = 0; i < 4; i++)
		DeleteTree(Q->children[i]);

	delete Q;
}

QTreeNode *BinTreeToQTree(int curNode, bool curNodeIsLeaf, const InnerNode* innerNodes, const std::vector<LeafItem>& leafItems)
{
	QTreeNode* children[] = { nullptr, nullptr, nullptr, nullptr };
	if (curNode == -1) {
		return nullptr;
	}

	if (curNodeIsLeaf) {
		return new QTreeNode(leafItems[curNode].PrimID, leafItems[curNode].aabb);
	}

	int left = innerNodes[curNode].left;
	int right = innerNodes[curNode].right;
	int nextchild = 0;
	int nodeCounter = 0;

	// if (left != -1) // All inner nodes in the Fast LBVH have 2 children
	{
		if (innerNodes[curNode].leftIsLeaf)
		{
			children[nextchild++] = BinTreeToQTree(left, true, innerNodes, leafItems);
		}
		else
		{
			children[nextchild++] = BinTreeToQTree(innerNodes[left].left, innerNodes[left].leftIsLeaf, innerNodes, leafItems);
			children[nextchild++] = BinTreeToQTree(innerNodes[left].right, innerNodes[left].rightIsLeaf, innerNodes, leafItems);
		}
	}

	// if (right != -1) // All inner nodes in the Fast LBVH have 2 children
	{
		if (innerNodes[curNode].rightIsLeaf)
		{
			children[nextchild++] = BinTreeToQTree(right, true, innerNodes, leafItems);
		}
		else
		{
			children[nextchild++] = BinTreeToQTree(innerNodes[right].left, innerNodes[right].leftIsLeaf, innerNodes, leafItems);
			children[nextchild++] = BinTreeToQTree(innerNodes[right].right, innerNodes[right].rightIsLeaf, innerNodes, leafItems);
		}
	}

	// Compute the AABB for this node
	AABB box;
	for (int i = 0; i < nextchild; i++)
		box.Expand(children[i]->box);

	return new QTreeNode(-1, box, children, nullptr);
}

QTreeNode* BinTreeToQTree(TreeNode *T)
{
	QTreeNode* children[] = { nullptr, nullptr, nullptr, nullptr };
	if (T == nullptr) {
		return nullptr;
	}

	if (T->IsLeaf()) {
		return new QTreeNode(T->TriID, T->box);
	}

	TreeNode *left = T->left;
	TreeNode* right = T->right;
	int nextchild = 0;
	int nodeCounter = 0;

	// if (left != -1) // All inner nodes in the Fast LBVH have 2 children
	{
		if (left->IsLeaf())
		{
			children[nextchild++] = BinTreeToQTree(left);
		}
		else
		{
			children[nextchild++] = BinTreeToQTree(left->left);
			children[nextchild++] = BinTreeToQTree(left->right);
		}
	}

	// if (right != -1) // All inner nodes in the Fast LBVH have 2 children
	{
		if (right->IsLeaf())
		{
			children[nextchild++] = BinTreeToQTree(right);
		}
		else
		{
			children[nextchild++] = BinTreeToQTree(right->left);
			children[nextchild++] = BinTreeToQTree(right->right);
		}
	}

	// Compute the AABB for this node
	AABB box;
	int numNodes = 0;
	for (int i = 0; i < nextchild; i++)
	{
		box.Expand(children[i]->box);
		numNodes += children[i]->numNodes;
	}

	QTreeNode *Q = new QTreeNode(-1, box, children, nullptr);
	Q->numNodes = 1 + numNodes;
	return Q;
}

bool LoadEmbree()
{
	log_debug("[DBG] [BVH] [EMB] Looking for embree3.dll");
	hEmbree = LoadLibrary(".\\embree3.dll");
	if (hEmbree == NULL)
	{
		g_bEmbreeLoaded = false;
		g_bRTEnableEmbree = false;
		g_BVHBuilderType = DEFAULT_BVH_BUILDER;
		log_debug("[DBG] [BVH] [EMB] Embree could not be loaded. Using DEFAULT_BVH_BUILDER instead");
		return false;
	}

	g_rtcNewDevice = (rtcNewDeviceFun)GetProcAddress(hEmbree, "rtcNewDevice");
	if (g_rtcNewDevice == nullptr) return false;
	g_rtcReleaseDevice = (rtcReleaseDeviceFun)GetProcAddress(hEmbree, "rtcReleaseDevice");
	if (g_rtcReleaseDevice == nullptr) return false;
	g_rtcNewScene = (rtcNewSceneFun)GetProcAddress(hEmbree, "rtcNewScene");
	if (g_rtcNewScene == nullptr) return false;
	g_rtcReleaseScene = (rtcReleaseSceneFun)GetProcAddress(hEmbree, "rtcReleaseScene");
	if (g_rtcReleaseScene == nullptr) return false;

	g_rtcNewBVH = (rtcNewBVHFun)GetProcAddress(hEmbree, "rtcNewBVH");
	if (g_rtcNewBVH == nullptr) return false;
	g_rtcBuildBVH = (rtcBuildBVHFun)GetProcAddress(hEmbree, "rtcBuildBVH");
	if (g_rtcBuildBVH == nullptr) return false;
	g_rtcThreadLocalAlloc = (rtcThreadLocalAllocFun)GetProcAddress(hEmbree, "rtcThreadLocalAlloc");
	if (g_rtcThreadLocalAlloc == nullptr) return false;
	g_rtcReleaseBVH = (rtcReleaseBVHFun)GetProcAddress(hEmbree, "rtcReleaseBVH");
	if (g_rtcReleaseBVH == nullptr) return false;

	g_bEmbreeLoaded = true;
	log_debug("[DBG] [BVH] [EMB] Embree was loaded dynamically");
	// TODO: Enabling multiple threads causes crashes and deadlocks. I'm not sure why
	g_rtcDevice = g_rtcNewDevice("threads=1,user_threads=1");
	g_rtcScene = g_rtcNewScene(g_rtcDevice);
	log_debug("[DBG] [BVH] [EMB] rtcDevice created");
	return g_rtcDevice != nullptr && g_rtcScene != nullptr;
}

void UnloadEmbree()
{
	if (hEmbree == NULL || !g_bEmbreeLoaded)
		return;
	g_rtcReleaseScene(g_rtcScene);
	g_rtcReleaseDevice(g_rtcDevice);
	log_debug("[DBG] [BVH] [EMB] Embree objects released");
}

TreeNode* InsertKDTree(AABB &currentAABB, TreeNode* tree, LeafItem& leaf)
{
	if (tree == nullptr)
	{
		return new TreeNode(GetID(leaf), GetAABB(leaf));
	}
	else
	{
		AABB aabb = GetAABB(leaf);
		Vector3 currentCenter = currentAABB.GetCentroidVector3();
		Vector3 centroid = aabb.GetCentroidVector3();

		// Get the largest dimension of the current AABB and use that to decide
		// where to insert the leaf
		int largestDim = aabb.GetLargestDimension();
		if (centroid[largestDim] < currentCenter[largestDim])
		{
			AABB nextAABB = currentAABB;
			nextAABB.max[largestDim] = currentCenter[largestDim];
			tree->left = InsertKDTree(nextAABB, tree->left, leaf);
			return tree;
		}
		else
		{
			AABB nextAABB = currentAABB;
			nextAABB.min[largestDim] = currentCenter[largestDim];
			tree->right = InsertKDTree(nextAABB, tree->right, leaf);
			return tree;
		}
	}
}

TreeNode *BuildKDTree(AABB &sceneAABB, std::vector<LeafItem> &leafItems)
{
	TreeNode* tree = nullptr;
	for (auto& leaf : leafItems)
	{
		tree = InsertKDTree(sceneAABB, tree, leaf);
	}
	return tree;
}

//#define DEBUG_PRS 1
#undef DEBUG_PRS

TreeNode* BuildDirectBVH(AABB& sceneAABB,
	std::vector<LeafItem> &leafItems, std::vector<int> &leafIndices, int dim)
{
	uint32_t numIndices = leafIndices.size();

	if (numIndices == 0)
	{
		return nullptr;
	}

	if (numIndices == 1)
	{
#ifdef DEBUG_PRS
		log_debug("[DBG] [BVH] %s single leaf: %d", tab(level*3).c_str(),
			leafIndices[0]);
#endif
		LeafItem& leaf = leafItems[leafIndices[0]];
		return new TreeNode(GetID(leaf), GetAABB(leaf));
	}

	if (numIndices == 2)
	{
#ifdef DEBUG_PRS
		log_debug("[DBG] [BVH] %s two leaves: %d, %d", tab(level*3).c_str(),
			leafIndices[0], leafIndices[1]);
#endif
		LeafItem& leafL = leafItems[leafIndices[0]];
		LeafItem& leafR = leafItems[leafIndices[1]];
		AABB boxL = GetAABB(leafL);
		AABB boxR = GetAABB(leafR);
		AABB boxP;

		boxP.Expand(boxL);
		boxP.Expand(boxR);

		TreeNode* tree = new TreeNode(-1, boxP);
		tree->left  = new TreeNode(GetID(leafL), GetAABB(leafL));
		tree->right = new TreeNode(GetID(leafR), GetAABB(leafR));
		return tree;
	}

	std::vector<int> leftIndices, rightIndices;
	float split = 0.5f * (sceneAABB.max[dim] + sceneAABB.min[dim]);
	AABB boxL, boxR;

#ifdef DEBUG_PRS
	log_debug("[DBG] [BVH] %s sceneAABB: %s", tab(level*3).c_str(),
		sceneAABB.ToString().c_str());
	log_debug("[DBG] [BVH] %s numIndices: %d, dim: %d, split: %0.3f", tab(level*3).c_str(),
		numIndices, dim, split);
#endif

	for (int k : leafIndices)
	{
		AABB aabb = GetAABB(leafItems[k]);
		Vector3 centroid = aabb.GetCentroidVector3();
		if (centroid[dim] < split)
		{
			leftIndices.push_back(k);
			boxL.Expand(aabb);
		}
		else
		{
			rightIndices.push_back(k);
			boxR.Expand(aabb);
		}
	}

#ifdef DEBUG_PRS
	log_debug("[DBG] [BVH] %s leftIndices: %d, rightIndices: %d", tab(level * 3).c_str(),
		leftIndices.size(), rightIndices.size());
#endif

	AABB boxP;
	boxP.Expand(boxL);
	boxP.Expand(boxR);

	TreeNode* tree = new TreeNode(-1, boxP);
	tree->left  = BuildDirectBVH(boxL, leafItems, leftIndices, boxL.GetLargestDimension());
	tree->right = BuildDirectBVH(boxR, leafItems, rightIndices, boxR.GetLargestDimension());
	return tree;
}

constexpr int BU_LEFT = 0;
constexpr int BU_RIGHT = 1;
constexpr int BU_NONE = 1;
constexpr int BU_MAX_CHILDREN = 2;
constexpr int BU_MAX_CHILDREN_4 = 4;

// <parentIndex, left-or-right-child>: one entry per leaf
struct BuilderItem
{
	int parentIndex;
	int8_t side;
};

struct InnerNodeBuildData
{
	int countL;
	int countR;
	int dim;
	float split;
	AABB box;
	int numChildren;
	int fitCounter;
	AABB boxL;
	AABB boxR;
};

//#define BU_USE_UINT_MINMAX
#undef BU_USE_UINT_MINMAX

// For the GPU-Friendly version, we don't want to compute a boxL and boxR
// during each iteration (to reduce global atomics). Instead we want to
// keep track of the next split range along only one dimension.
struct InnerNodeBuildDataGPU
{
	int countL;
	int countR;
	int dim;
	float split;
	AABB box;
	int numChildren;
	int fitCounter;
	int nextDimL;
#ifdef BU_USE_UINT_MINMAX
	uint32_t nextMinL;
	uint32_t nextMaxL;
#else
	float nextMinL;
	float nextMaxL;
#endif
	int nextDimR;
#ifdef BU_USE_UINT_MINMAX
	uint32_t nextMinR;
	uint32_t nextMaxR;
#else
	float nextMinR;
	float nextMaxR;
#endif
};

constexpr int BU_PARTITIONS = 4;

// Used to build a BVH4
struct InnerNode4BuildDataGPU
{
	int   counts[BU_PARTITIONS];
	int   dims[BU_PARTITIONS - 1];
	float splits[BU_PARTITIONS - 1];
	AABB  box;
	bool  skipClassify;
	int   fitCounter;
	int   fitCounterTarget;

	int   nextDim[BU_PARTITIONS];
	float nextMin[BU_PARTITIONS];
	float nextMax[BU_PARTITIONS];
};

constexpr float BVH_NORM_FACTOR = 1048576.0f; // 2^20, same precision we use for 64-bit Morton Codes

static inline uint32_t Normalize(const Vector3 &pos, int dim, const AABB &box, const Vector3 &range)
{
	return (uint32_t)max(0.0f, min(BVH_NORM_FACTOR,
		BVH_NORM_FACTOR * (pos[dim] - box.min[dim]) / range[dim]));
}

static inline float DeNormalize(uint32_t pos, int dim, const AABB &box, const Vector3 &range)
{
	return ((float)pos / BVH_NORM_FACTOR) * range[dim] + box.min[dim];
}

static void Refit(int innerNodeIndex, InnerNode* innerNodes, InnerNodeBuildDataGPU* innerNodeBuildData, std::vector<LeafItem> &leafItems)
{
#ifdef DEBUG_BU
	int tabLevel = 1;
#endif
	const int rootIndex = 0;
	int curInnerNodeIndex = innerNodeIndex;

	while (curInnerNodeIndex >= rootIndex)
	{
		InnerNode&  innerNode = innerNodes[curInnerNodeIndex];
		const int parentIndex = innerNodes[curInnerNodeIndex].parent;
		// There's nothing to do if this node doesn't have enough data to
		// compute the fit
		if (innerNodeBuildData[curInnerNodeIndex].fitCounter < 2)
			return;

		const int  left = innerNode.left;
		const int right = innerNode.right;
		const bool  leftIsLeaf = innerNode.leftIsLeaf;
		const bool rightIsLeaf = innerNode.rightIsLeaf;

		AABB leftBox  = leftIsLeaf  ? leafItems[left].aabb  : innerNodes[left].aabb;
		AABB rightBox = rightIsLeaf ? leafItems[right].aabb : innerNodes[right].aabb;

		innerNode.aabb.Expand(leftBox);
		innerNode.aabb.Expand(rightBox);

		// The root probably doesn't need to be refit, but we need to write its AABB
		// during initialization, so either way is fine.
		if (parentIndex >= rootIndex)
			innerNodeBuildData[parentIndex].fitCounter++;

#ifdef DEBUG_BU
		int dim = innerNodeBuildData[curInnerNodeIndex].dim;
		log_debug("[DBG] [BVH] %sInner node %d has been refit, dim: %d: [%0.1f, %0.1f], parent: %d, parent fitCounter: %d",
			tab(tabLevel * 3).c_str(),
			curInnerNodeIndex, dim,
			innerNodes[curInnerNodeIndex].aabb.min[dim],
			innerNodes[curInnerNodeIndex].aabb.max[dim],
			parentIndex, parentIndex >= rootIndex ? innerNodeBuildData[parentIndex].fitCounter : -1);
		tabLevel++;
#endif

		// Continue refitting the parents if possible
		curInnerNodeIndex = parentIndex;
	}
}

static void Refit4(
	const int innerNodeIndex,
	BVHNode* buffer,
	InnerNode4BuildDataGPU* innerNodeBuildData,
	std::vector<LeafItem>& leafItems)
{
#ifdef DEBUG_BU
	int tabLevel = 1;
#endif
	const int rootIndex = 0;
	int curInnerNodeIndex = innerNodeIndex;

	while (curInnerNodeIndex >= rootIndex)
	{
		// There's nothing to do if this node doesn't have enough data to
		// compute the fit
		const int fitCounterTarget = innerNodeBuildData[curInnerNodeIndex].fitCounterTarget;
		if (innerNodeBuildData[curInnerNodeIndex].fitCounter < fitCounterTarget)
			return;

		BVHNode& node = buffer[curInnerNodeIndex];
		const int parentIndex = node.parent;
//#ifdef DEBUG_BU
//		log_debug("[DBG] [BVH] curNode: %d, parent: %d, numChildren: %d, fitCounter: %d",
//			curInnerNodeIndex, parentIndex,
//			innerNodeBuildData[curInnerNodeIndex].numChildren,
//			innerNodeBuildData[curInnerNodeIndex].fitCounter);
//#endif

		// Compress the children indices
		int tmpIndices[BU_PARTITIONS] = { -1, -1, -1, -1 };
		int destIdx = 0;
		for (int k = 0; k < BU_PARTITIONS; k++)
		{
			if (node.children[k] != -1)
				tmpIndices[destIdx++] = node.children[k];
		}
		for (int k = 0; k < BU_PARTITIONS; k++)
			node.children[k] = tmpIndices[k];

		AABB nodeBox;
		for (int k = 0; k < node.numChildren; k++)
		{
			const int  child  = node.children[k];
			const bool isLeaf = (buffer[child].ref != -1);

//#ifdef DEBUG_BU
//			log_debug("[DBG] [BVH] child: %d, isLeaf: %d, final idx: %d",
//				child, isLeaf, isLeaf ? child - numInnerNodes : child);
//#endif

			AABB box;
			if (isLeaf)
			{
				box = leafItems[buffer[child].ref].aabb;
			}
			else
			{
				for (int i = 0; i < 3; i++)
				{
					box.min[i] = buffer[child].min[i];
					box.max[i] = buffer[child].max[i];
				}
			}
//#ifdef DEBUG_BU
//			log_debug("[DBG] [BVH] box: %s", box.ToString().c_str());
//#endif
			nodeBox.Expand(box);
		}
		// Write the new box to the inner node
		for (int i = 0; i < 3; i++)
		{
			buffer[curInnerNodeIndex].min[i] = nodeBox.min[i];
			buffer[curInnerNodeIndex].max[i] = nodeBox.max[i];
		}
		buffer[curInnerNodeIndex].min[3] = 1.0f;
		buffer[curInnerNodeIndex].max[3] = 1.0f;

		if (parentIndex >= rootIndex)
		{
			innerNodeBuildData[parentIndex].fitCounter += fitCounterTarget;
		}

#ifdef DEBUG_BU
		log_debug("[DBG] [BVH] %sInner node %d has been refit, %s, parent: %d, parent fitCounter: %d",
			tab(tabLevel * 3).c_str(),
			curInnerNodeIndex,
			nodeBox.ToString().c_str(),
			parentIndex,
			parentIndex >= rootIndex ? innerNodeBuildData[parentIndex].fitCounter : -1);
		tabLevel++;
#endif

		// Continue refitting the parents if possible
		curInnerNodeIndex = parentIndex;
	}
}

static void DirectBVH2Init(AABB centroidBox,
	std::vector<LeafItem>& leafItems,
	std::vector<BuilderItem> &leafParents, InnerNodeBuildDataGPU* innerNodeBuildData,
	InnerNode *innerNodes,
	int& root_out)
{
	const int numPrimitives = (int)leafItems.size();
	const int numInnerNodes = numPrimitives - 1;

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] centroidBox: %s", centroidBox.ToString().c_str());
#endif

	root_out = 0;
	g_directBuilderNextNode = 1; // The root already exists, so the next available inner node is idx 1

	// All leafs are initially connected to the root, but they are not classified yet
	for (int i = 0; i < numPrimitives; i++)
		leafParents.push_back({ root_out, BU_NONE });

	const int   dim   = centroidBox.GetLargestDimension();
	const float split = 0.5f * (centroidBox.max[dim] + centroidBox.min[dim]);

	for (int i = 0; i < numInnerNodes; i++)
		innerNodeBuildData[i] = { 0, 0, 0, 0.0f, AABB(), 0, 0,
			// nextDimL, nextMinL, nextMaxL
			// nextDimR, nextMinR, nextMaxR
#ifdef BU_USE_UINT_MINMAX
			0, UINT32_MAX, 0,
			0, UINT32_MAX, 0
#else
			0, FLT_MAX, -FLT_MAX,
			0, FLT_MAX, -FLT_MAX
#endif
		};

	// The root will split along dim, so the left and right boxes
	// are already known:
	AABB boxL = centroidBox;
	AABB boxR = centroidBox;
	boxL.max[dim] = split;
	boxR.min[dim] = split;
	// The next split will be along these dimensions:
	const int nextDimL = boxL.GetLargestDimension();
	const int nextDimR = boxR.GetLargestDimension();
	//const float nextSplitL = 0.5f * (boxL.min[nextDimL] + boxL.max[nextDimL]);
	//const float nextSplitR = 0.5f * (boxR.min[nextDimR] + boxR.max[nextDimR]);

	// Initialize the node counters for the root, along with the box and split data
	innerNodeBuildData[root_out] = { 0, 0, dim, split, centroidBox, 0, 0,
#ifdef BU_USE_UINT_MINMAX
		nextDimL, UINT32_MAX, 0,
		nextDimR, UINT32_MAX, 0
#else
		nextDimL, FLT_MAX, -FLT_MAX,
		nextDimR, FLT_MAX, -FLT_MAX
#endif
	};
	// Initialize the root's parent (this stops the Refit() operation)
	innerNodes[root_out].parent = -1;
}

static bool DirectBVH2Classify(
	int iteration,
	std::vector<LeafItem>& leafItems,
	std::vector<BuilderItem>& leafParents,
	InnerNodeBuildDataGPU* innerNodeBuildData)
{
	const int numPrimitives = (int)leafItems.size();
	const int numInnerNodes = numPrimitives - 1;

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] --------------------------------------------");
	log_debug("[DBG] [BVH] ITER: %d, PHASE 2. Classify prims", iteration);
#endif

	bool done = true;
	for (int i = 0; i < numPrimitives; i++)
	{
		int parentNodeIndex = leafParents[i].parentIndex;
		LeafItem leaf       = leafItems[i];

		// Skip inactive primitives
		if (parentNodeIndex == -1)
			continue;

		// We're going to process at least one primitive, so we're not done yet.
		// This is a global, but it doesn't have to be an atomic because all threads
		// will attempt to write the same value.
		done = false;

		InnerNodeBuildDataGPU& innerNodeBD = innerNodeBuildData[parentNodeIndex];
		const int   dim       = innerNodeBD.dim;
		const int   nextDimL  = innerNodeBD.nextDimL;
		const int   nextDimR  = innerNodeBD.nextDimR;
		const float split     = innerNodeBD.split;
		AABB        box       = innerNodeBD.box;
		Vector3     boxRange  = box.GetRange();
		const bool  nullRange = (fabs(boxRange[dim]) < 0.0001f);
		float       key       = leaf.centroid[dim];
		float       keySplit  = split;
		//AABB boxL     = box;
		//AABB boxR     = box;
		//boxL.max[dim] = split;
		//boxR.max[dim] = split;

		if (nullRange)
		{
			key = (float)i;
			// Use InterlockedExchange() to flip between 1 and -1 in a GPU:
			if (keySplit >= 0.0f)
			{
				keySplit = key + 1.0f;
				innerNodeBuildData[parentNodeIndex].split = -1.0f;
			}
			else
			{
				keySplit = key - 1.0f;
				innerNodeBuildData[parentNodeIndex].split = 1.0f;
			}
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] WARNING! NULL RANGE! box: %s", box.ToString().c_str());
#endif
		}

		if (key < keySplit)
		{
			leafParents[i] = { parentNodeIndex, BU_LEFT };
			innerNodeBD.countL++; // ATOMIC
			//innerNodeBD.boxL.Expand(leaf.aabb); // ATOMIC
#ifdef BU_USE_UINT_MINMAX
			uint32_t code = Normalize(leaf.centroid, nextDimL, box, boxRange);
			innerNodeBD.nextMinL = min(innerNodeBD.nextMinL, code); // ATOMIC
			innerNodeBD.nextMaxL = max(innerNodeBD.nextMaxL, code); // ATOMIC
#else
			innerNodeBD.nextMinL = min(innerNodeBD.nextMinL, leaf.centroid[nextDimL]); // ATOMIC
			innerNodeBD.nextMaxL = max(innerNodeBD.nextMaxL, leaf.centroid[nextDimL]); // ATOMIC
#endif
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] (%d, %0.1f) -> L:%d, [%0.3f, %0.3f, %0.3f], nextMinMaxL: [%0.3f, %0.3f]",
				i, leaf.centroid[dim], parentNodeIndex, box.min[dim], split, box.max[dim],
				//DeNormalize(innerNodeBD.nextMinL, nextDimL, box, boxRange),
				//DeNormalize(innerNodeBD.nextMaxL, nextDimL, box, boxRange));
				innerNodeBD.nextMinL,
				innerNodeBD.nextMaxL);
#endif
		}
		else
		{
			leafParents[i] = { parentNodeIndex, BU_RIGHT };
			innerNodeBD.countR++; // ATOMIC
			//innerNodeBD.boxR.Expand(leaf.aabb); // ATOMIC
#ifdef BU_USE_UINT_MINMAX
			uint32_t code = Normalize(leaf.centroid, nextDimR, box, boxRange);
			innerNodeBD.nextMinR = min(innerNodeBD.nextMinR, code); // ATOMIC
			innerNodeBD.nextMaxR = max(innerNodeBD.nextMaxR, code); // ATOMIC
#else
			innerNodeBD.nextMinR = min(innerNodeBD.nextMinR, leaf.centroid[nextDimR]); // ATOMIC
			innerNodeBD.nextMaxR = max(innerNodeBD.nextMaxR, leaf.centroid[nextDimR]); // ATOMIC
#endif
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] (%d, %0.1f) -> R:%d, [%0.3f, %0.3f, %0.3f], nextMinMaxR: [%0.3f, %0.3f]",
				i, leaf.centroid[dim], parentNodeIndex, box.min[dim], split, box.max[dim],
				//DeNormalize(innerNodeBD.nextMinR, nextDimR, box, boxRange),
				//DeNormalize(innerNodeBD.nextMaxR, nextDimR, box, boxRange));
				innerNodeBD.nextMinR,
				innerNodeBD.nextMaxR);
#endif
		}
	}

	return done;
}

static void DirectBVH2EmitInnerNodes(int iteration,
	InnerNodeBuildDataGPU* innerNodeBuildData,
	InnerNode* innerNodes)
{
#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] --------------------------------------------");
	log_debug("[DBG] [BVH] ITER: %d, PHASE 3. Emit inner nodes", iteration);
#endif

	// We don't need to iterate over all the inner nodes, since only a fraction of them
	// will actually be active during the algorithm
	const int lastActiveInnerNode = g_directBuilderNextNode;

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] lastActiveInnerNode: %d", lastActiveInnerNode);
#endif

	for (int i = 0; i < lastActiveInnerNode; i++)
	{
		InnerNodeBuildDataGPU& innerNodeBD = innerNodeBuildData[i];
		// Skip full nodes
		if (innerNodeBD.numChildren >= BU_MAX_CHILDREN)
		{
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] Skipping inner node %d (it's full)", i);
#endif
			continue;
		}

		AABB  innerNodeBox = innerNodeBD.box; // this is the centroidBox used in the previous cut
#ifdef BU_USE_UINT_MINMAX
		Vector3 range      = innerNodeBox.GetRange();
#endif
		int   dim          = innerNodeBD.dim;
		float split        = innerNodeBD.split;

		int   nextDimL     = innerNodeBD.nextDimL;
		int   nextDimR     = innerNodeBD.nextDimR;
		//AABB  boxL         = innerNodeBD.boxL;
		//AABB  boxR         = innerNodeBD.boxR;
#ifdef BU_USE_UINT_MINMAX
		uint32_t nextMinL  = innerNodeBD.nextMinL;
		uint32_t nextMaxL  = innerNodeBD.nextMaxL;
		uint32_t nextMinR  = innerNodeBD.nextMinR;
		uint32_t nextMaxR  = innerNodeBD.nextMaxR;
#else
		float nextMinL     = innerNodeBD.nextMinL;
		float nextMaxL     = innerNodeBD.nextMaxL;
		float nextMinR     = innerNodeBD.nextMinR;
		float nextMaxR     = innerNodeBD.nextMaxR;
#endif

#ifdef DEBUG_BU
		log_debug("[DBG] [BVH] inner node %d, nextMinMaxL: [%0.3f, %0.3f], nextMinMaxR: [%0.3f, %0.3f]",
			i, nextMinL, nextMaxL, nextMinR, nextMaxR);
#endif

		// Prepare the boxes for the next split
		AABB boxL          = innerNodeBox;
		AABB boxR          = innerNodeBox;

		boxL.max[dim]      = split;
#ifdef BU_USE_UINT_MINMAX
		boxL.min[nextDimL] = DeNormalize(nextMinL, dim, innerNodeBox, range);
		boxL.max[nextDimL] = DeNormalize(nextMaxL, dim, innerNodeBox, range);
#else
		boxL.min[nextDimL] = nextMinL;
		boxL.max[nextDimL] = nextMaxL;
#endif

		boxR.min[dim]      = split;
#ifdef BU_USE_UINT_MINMAX
		boxR.min[nextDimR] = DeNormalize(nextMinR, dim, innerNodeBox, range);
		boxR.max[nextDimR] = DeNormalize(nextMaxR, dim, innerNodeBox, range);
#else
		boxR.min[nextDimR] = nextMinR;
		boxR.max[nextDimR] = nextMaxR;
#endif

#ifdef DEBUG_BU
		log_debug("[DBG] [BVH] inner node %d, counts: (%d,%d), boxL,R: %s, %s",
			i, innerNodeBD.countL, innerNodeBD.countR, boxL.ToString().c_str(), boxR.ToString().c_str());
#endif

		if (innerNodeBD.countL == 0)
		{
			// Bad split, repeat the iteration
			//int dim     = boxR.GetLargestDimension();
			int   dim   = nextDimR;
			float split = 0.5f * (boxR.max[dim] + boxR.min[dim]);

			innerNodeBuildData[i].box    = boxR;
			innerNodeBuildData[i].dim    = dim;
			innerNodeBuildData[i].split  = split;
			innerNodeBuildData[i].countR = 0;
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] ERROR: inner node %d has a bad split (countL == 0), repeat", i);
#endif
		}
		else if (innerNodeBD.countR == 0)
		{
			// Bad split, repeat the iteration
			//int dim     = boxL.GetLargestDimension();
			int dim     = nextDimL;
			float split = 0.5f * (boxL.max[dim] + boxL.min[dim]);

			innerNodeBuildData[i].box    = boxL;
			innerNodeBuildData[i].dim    = dim;
			innerNodeBuildData[i].split  = split;
			innerNodeBuildData[i].countL = 0;
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] ERROR: inner node %d has a bad split (countR == 0), repeat", i);
#endif
		}
		else
		{
			if (innerNodeBD.countL > 1)
			{
				// Emit an inner node on the left and split the subrange again
				int newNodeIndex = g_directBuilderNextNode;
				g_directBuilderNextNode++; // ATOMIC

				// Connect the current inner node to the new one
				innerNodes[i].left       = newNodeIndex;
				innerNodes[i].leftIsLeaf = false;
				innerNodes[newNodeIndex].parent = i;

				// Split the left subrange again
				//int   dim   = boxL.GetLargestDimension();
				//float split = 0.5f * (boxL.max[dim] + boxL.min[dim]);
				AABB  box     = boxL;
				int   dim     = nextDimL;
				float split   = 0.5f * (box.max[dim] + box.min[dim]);
				// Compute the boxes for the next-next iteration:
				AABB boxL     = box;
				AABB boxR     = box;
				boxL.max[dim] = split;
				boxR.min[dim] = split;
				// The next split will be along this dimension:
				int nextDimL  = boxL.GetLargestDimension();
				int nextDimR  = boxR.GetLargestDimension();

				innerNodeBuildData[i].numChildren++;
				// Enable the new inner node
				innerNodeBuildData[newNodeIndex] = { 0, 0, dim, split, box, 0, 0,
#ifdef BU_USE_UINT_MINMAX
					nextDimL, UINT32_MAX, 0,
					nextDimR, UINT32_MAX, 0
#else
					nextDimL, FLT_MAX, -FLT_MAX,
					nextDimR, FLT_MAX, -FLT_MAX
#endif
				};

#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] %d -> %d",
					innerNodes[i].left, i);
#endif
			}

			if (innerNodeBD.countR > 1)
			{
				// Emit an inner node and split the right subrange again
				int newNodeIndex = g_directBuilderNextNode;
				g_directBuilderNextNode++; // ATOMIC

				// Connect the current inner node to the new one
				innerNodes[i].right       = newNodeIndex;
				innerNodes[i].rightIsLeaf = false;
				innerNodes[newNodeIndex].parent = i;

				// Split the left subrange again
				//int   dim   = boxR.GetLargestDimension();
				//float split = 0.5f * (boxR.max[dim] + boxR.min[dim]);
				AABB box      = boxR;
				int dim       = nextDimR;
				float split   = 0.5f * (box.max[dim] + box.min[dim]);
				// Compute the boxes for the next-next iteration:
				AABB boxL     = box;
				AABB boxR     = box;
				boxL.max[dim] = split;
				boxR.min[dim] = split;
				// The next split will be along this dimension:
				int nextDimL  = boxL.GetLargestDimension();
				int nextDimR  = boxR.GetLargestDimension();

				innerNodeBuildData[i].numChildren++;
				// Enable the new inner node
				innerNodeBuildData[newNodeIndex] = { 0, 0, dim, split, box, 0, 0,
#ifdef BU_USE_UINT_MINMAX
					nextDimL, UINT32_MAX, 0,
					nextDimR, UINT32_MAX, 0
#else
					nextDimL, FLT_MAX, -FLT_MAX,
					nextDimR, FLT_MAX, -FLT_MAX
#endif
				};

#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] %d <- %d",
					i, innerNodes[i].right, nextDimL, nextDimR);
#endif
			}
		}
	}
}

static void DirectBVH2InitNextIteration(int iteration, const int numPrimitives,
	std::vector<BuilderItem>& leafParents, std::vector<LeafItem> &leafItems,
	InnerNodeBuildDataGPU* innerNodeBuildData, InnerNode* innerNodes)
{
	const int numInnerNodes = numPrimitives - 1;

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] --------------------------------------------");
	log_debug("[DBG] [BVH] ITER: %d, PHASE 4. Init next iteration, disable prims", iteration);
#endif

	for (int i = 0; i < numPrimitives; i++)
	{
		int parentIndex = leafParents[i].parentIndex;
		int side = leafParents[i].side;

		// Skip inactive primitives
		if (parentIndex == -1)
			continue;

		// Emit leaves if applicable or update parent pointers
		InnerNodeBuildDataGPU& innerNodeBD = innerNodeBuildData[parentIndex];

		if (innerNodeBD.countL == 0 || innerNodeBD.countR == 0)
		{
			leafParents[i].side = BU_NONE;
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] Skipping prim %d, it points to an inner node with a bad split", i);
#endif
			continue;
		}

		if (side == BU_LEFT)
		{
			if (innerNodeBD.countL == 1)
			{
				// This node is the left leaf of parentIndex
				innerNodes[parentIndex].left = i;
				innerNodes[parentIndex].leftIsLeaf = true;
				innerNodeBuildData[parentIndex].numChildren++;
				innerNodeBuildData[parentIndex].fitCounter++;

				// Deactivate this primitive for the next iteration
				leafParents[i].parentIndex = -1;

#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] prim %d still points to L:%d and it's a leaf, deactivated. Parent fitCounter: %d",
					i, parentIndex, innerNodeBuildData[parentIndex].fitCounter);
#endif
				if (innerNodeBuildData[parentIndex].fitCounter == BU_MAX_CHILDREN)
				{
					// The inner node can be refit
#ifdef DEBUG_BU
					log_debug("[DBG] [BVH] Refitting inner node: %d", parentIndex);
#endif
					Refit(parentIndex, innerNodes, innerNodeBuildData, leafItems);
				}
			}
			else if (innerNodeBD.countL > 1)
			{
				// This leaf a left child but its parent has too many children. Loop again.
				leafParents[i].parentIndex = innerNodes[parentIndex].left;
				leafParents[i].side = BU_NONE;
#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] prim %d now points to L:%d", i, innerNodes[parentIndex].left);
#endif
			}
		}
		else // side == BU_RIGHT
		{
			if (innerNodeBD.countR == 1)
			{
				// This node is a right leaf of parentIndex
				innerNodes[parentIndex].right = i;
				innerNodes[parentIndex].rightIsLeaf = true;
				innerNodeBuildData[parentIndex].numChildren++;
				innerNodeBuildData[parentIndex].fitCounter++;

				// Deactivate this primitive for the next iteration
				leafParents[i].parentIndex = -1;

#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] prim %d still points to R:%d and it's a leaf, deactivated. Parent fitCounter: %d",
					i, parentIndex, innerNodeBuildData[parentIndex].fitCounter);
#endif
				if (innerNodeBuildData[parentIndex].fitCounter == BU_MAX_CHILDREN)
				{
#ifdef DEBUG_BU
					log_debug("[DBG] [BVH] Refitting inner node: %d", parentIndex);
#endif
					// The inner node can be refit
					Refit(parentIndex, innerNodes, innerNodeBuildData, leafItems);
				}
			}
			else if (innerNodeBD.countR > 1)
			{
				// This leaf a right child but its parent has too many children. Loop again.
				leafParents[i].parentIndex = innerNodes[parentIndex].right;
				leafParents[i].side = BU_NONE;
#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] prim %d now points to R:%d", i, innerNodes[parentIndex].right);
#endif
			}
		}
	}
}

// CPU version of the DirectBVH2 builder. This algorithm is simpler but it's less efficient
// when it comes to memory usage and it's recursive, so it's not GPU-friendly.
bool _DirectBVH2BuilderCPU(
	int iteration,
	AABB sceneAABB,
	AABB centroidAABB,
	InnerNode* innerNodes,
	std::vector<LeafItem>& leafItems,
	std::vector<int>& leafIndices,
	int dim,
	int curParentNodeIndex)
{
	float split = 0.5f * (centroidAABB.max[dim] + centroidAABB.min[dim]);
	AABB boxL, boxR;
	AABB centroidBoxL, centroidBoxR;
	int countL = 0, countR = 0;
	std::vector<int> indicesL, indicesR;
	Vector3 centroidRange = centroidAABB.GetRange();
	const bool nullRange = (fabs(centroidRange[dim]) < 0.0001f);

	if (iteration > g_maxDirectBVHIteration)
		g_maxDirectBVHIteration = iteration;

#ifdef DEBUG_BU
	const int numInnerNodes = (int)leafItems.size() - 1;
	if (numInnerNodes < 0)
		log_debug("[DBG] [BVH] ERROR: numInnerNodes: %d", numInnerNodes);
#endif

	if (leafIndices.size() == 1)
	{
		// Single-primitive case. The leaf is the root, but there's
		// nothing to do here since there are no inner nodes.
		//log_debug("[DBG] [BVH] WARNING, leafIndices.size() = 1");
		return true;
	}

	// The current root will contain the whole AABB that is spawned by these primitives
	innerNodes[curParentNodeIndex].aabb = sceneAABB;

	if (leafIndices.size() == 2)
	{
		int left  = leafIndices[0];
		int right = leafIndices[1];
		innerNodes[curParentNodeIndex].left       = left;
		innerNodes[curParentNodeIndex].leftIsLeaf = true;

		innerNodes[curParentNodeIndex].right       = right;
		innerNodes[curParentNodeIndex].rightIsLeaf = true;
		return true;
	}

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] --------------------------------------------");
	log_debug("[DBG] [BVH] curParentNodeIndex: %d, split: %0.3f, dim: %d, [%0.3f, %0.3f]",
		curParentNodeIndex, split, dim, sceneAABB.min[dim], sceneAABB.max[dim]);
	std::string msg = "[DBG] [BVH] ";
#endif

	// Use the primitive index for areas with a null range:
	const float keySplit = nullRange ? (float)leafIndices.size() / 2.0f : split;
	for (uint32_t k = 0; k < leafIndices.size(); k++)
	{
		const uint32_t i        = leafIndices[k];
		const AABB     aabb     = leafItems[i].aabb;
		const Vector3  centroid = leafItems[i].centroid;
		const float    key      = nullRange ? (float)k : centroid[dim];

		//if (centroid[dim] < split)
		if (key < keySplit)
		{
			boxL.Expand(aabb);
			centroidBoxL.Expand(centroid);
			countL++;
			indicesL.push_back(i);
#ifdef DEBUG_BU
			msg += "(" + std::to_string(i) + "," + std::to_string(centroid[dim]) + ")->" + std::to_string(curParentNodeIndex) + ", ";
#endif
		}
		else
		{
			boxR.Expand(aabb);
			centroidBoxR.Expand(centroid);
			countR++;
			indicesR.push_back(i);
#ifdef DEBUG_BU
			msg += std::to_string(curParentNodeIndex) + "<-(" + std::to_string(i) + "," + std::to_string(centroid[dim]) + "), ";
#endif
		}
	}

#ifdef DEBUG_BU
	log_debug(msg.c_str());
	log_debug("[DBG] [BVH] countL,R: (%d, %d), boxL: [%0.3f, %0.3f], boxR: [%0.3f, %0.3f]",
		countL, countR, boxL.min[dim], boxL.max[dim], boxR.min[dim], boxR.max[dim]);
#endif

	if (countL == 0)
	{
		log_debug("[DBG] [BVH] ERROR: Bad split, countL == 0, countR: %d, dim: %d", countR, dim);
		log_debug("[DBG] [BVH] ERROR: centroidAABB: %s", centroidAABB.ToString().c_str());
		log_debug("[DBG] [BVH] ERROR: centroidBoxL: %s", centroidBoxL.ToString().c_str());
		log_debug("[DBG] [BVH] ERROR: centroidBoxR: %s", centroidBoxR.ToString().c_str());
		return false;
		// All our nodes landed on the right side, re-compute the split and try again
		//return _DirectBVH2BuilderCPU(boxR, innerNodes, leafItems, indicesR, leafParents, boxR.GetLargestDimension(), curParentNodeIndex);
	}

	if (countR == 0)
	{
		log_debug("[DBG] [BVH] ERROR: Bad split, countR == 0, countL: %d, dim: %d", countL, dim);
		log_debug("[DBG] [BVH] ERROR: centroidAABB: %s", centroidAABB.ToString().c_str());
		log_debug("[DBG] [BVH] ERROR: centroidBoxL: %s", centroidBoxL.ToString().c_str());
		log_debug("[DBG] [BVH] ERROR: centroidBoxR: %s", centroidBoxR.ToString().c_str());
		return false;
		// All our nodes landed on the left side, re-compute the split and try again
		//return _DirectBVH2BuilderCPU(boxL, innerNodes, leafItems, indicesL, leafParents, boxL.GetLargestDimension(), curParentNodeIndex);
	}

	bool result = true;
	if (countL > 1)
	{
		g_directBuilderNextNode++; // ATOMIC
#ifdef DEBUG_BU
		if (g_directBuilderNextNode >= numInnerNodes)
		{
			log_debug("[DBG] [BVH] ERROR, g_directBuilderNextNode: %d, numInnerNodes: %d",
				g_directBuilderNextNode, numInnerNodes);
		}
		log_debug("[DBG] [BVH] %d -> L:%d", g_directBuilderNextNode, curParentNodeIndex);
#endif
		// Connect g_directBuilderNextNode as the left child of curNodeIndex
		innerNodes[curParentNodeIndex].left       = g_directBuilderNextNode;
		innerNodes[curParentNodeIndex].leftIsLeaf = false;
		result = result && _DirectBVH2BuilderCPU(iteration + 1, boxL, centroidBoxL,
			innerNodes, leafItems, indicesL, centroidBoxL.GetLargestDimension(), g_directBuilderNextNode);
	}
	else // countL is 1
	{
		// The node on the left is a leaf
		innerNodes[curParentNodeIndex].left       = indicesL[0];
		innerNodes[curParentNodeIndex].leftIsLeaf = true;
	}

	if (countR > 1)
	{
		g_directBuilderNextNode++; // ATOMIC
#ifdef DEBUG_BU
		if (g_directBuilderNextNode >= numInnerNodes)
		{
			log_debug("[DBG] [BVH] ERROR, g_directBuilderNextNode: %d, numInnerNodes: %d",
				g_directBuilderNextNode, numInnerNodes);
		}
		log_debug("[DBG] [BVH] %d -> R:%d", g_directBuilderNextNode, curParentNodeIndex);
#endif
		// Connect g_directBuilderNextNode as the right child of curNodeIndex
		innerNodes[curParentNodeIndex].right       = g_directBuilderNextNode;
		innerNodes[curParentNodeIndex].rightIsLeaf = false;
		result = result && _DirectBVH2BuilderCPU(iteration + 1, boxR, centroidBoxR,
			innerNodes, leafItems, indicesR, centroidBoxR.GetLargestDimension(), g_directBuilderNextNode);
	}
	else // countR is 1
	{
		// The node on the right is a leaf
		innerNodes[curParentNodeIndex].right       = indicesR[0];
		innerNodes[curParentNodeIndex].rightIsLeaf = true;
	}

	return result;
}

InnerNode* DirectBVH2BuilderCPU(AABB sceneAABB, AABB centroidBox, std::vector<LeafItem>& leafItems, int &rootIdx)
{
	const int numPrimitives = (int)leafItems.size();
	const int numInnerNodes = numPrimitives - 1;

	InnerNode* innerNodes = new InnerNode[numInnerNodes];
	rootIdx = 0;
	g_directBuilderNextNode = 0; // This is incremented before it's used, so it should be initialized to 0
	g_maxDirectBVHIteration = -1;

	// Initialize the leaf parents and indices
	std::vector<int> leafIndices;
	for (int i = 0; i < numPrimitives; i++)
	{
		leafIndices.push_back(i);
	}

	if (!_DirectBVH2BuilderCPU(0, sceneAABB, centroidBox,
		innerNodes, leafItems, leafIndices, centroidBox.GetLargestDimension(), rootIdx))
	{
		log_debug("[DBG] [BVH] _DirectBVH2BuilderCPU failed");
		delete[] innerNodes;
		innerNodes = nullptr;
	}
	/*else
	{
		log_debug("[DBG] [BVH] _DirectBVH2BuilderCPU() succeeded. Max iteration: %d", g_maxDirectBVHIteration);
	}*/

	return innerNodes;
}

// GPU-Friendly version of DirectBVH2BuilderCPU (no recursion, static arrays, reduced use of global atomics)
InnerNode* DirectBVH2BuilderGPU(AABB centroidBox, std::vector<LeafItem> &leafItems, int &root_out)
{
	const int numPrimitives = (int)leafItems.size();
	const int numInnerNodes = numPrimitives - 1;
	InnerNode*   innerNodes = new InnerNode[numInnerNodes];

	InnerNodeBuildDataGPU* innerNodeBuildData = new InnerNodeBuildDataGPU[numInnerNodes];
	std::vector<BuilderItem> leafParents;

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] Running the DirectBVH2Builder");
#endif

	// ********************************************************
	// PHASE 1: Initialize the algorithm
	// ********************************************************
	DirectBVH2Init(centroidBox, leafItems, leafParents, innerNodeBuildData, innerNodes, root_out);

	const int maxNumIters = numPrimitives - 1; // Worst-case scenario!

	for (int iteration = 0; iteration < maxNumIters; iteration++)
	{
		// ********************************************************
		// PHASE 2: Classify the primitives
		// ********************************************************
		const bool done = DirectBVH2Classify(iteration, leafItems, leafParents, innerNodeBuildData);

		// The algorithm has finished
		if (done)
		{
//#ifdef DEBUG_BU
			//log_debug("[DBG] [BVH] Finished at iteration: %d, maxNumIters: %d", iteration, maxNumIters);
//#endif
			break;
		}

		// ********************************************************
		// PHASE 3: Emit inner nodes
		// ********************************************************
		DirectBVH2EmitInnerNodes(iteration, innerNodeBuildData, innerNodes);

		// ********************************************************
		// PHASE 4: Init the next iteration and disable primitives
		// ********************************************************
		DirectBVH2InitNextIteration(iteration, numPrimitives, leafParents, leafItems, innerNodeBuildData, innerNodes);

#ifdef DEBUG_BU
		log_debug("[DBG] [BVH] **************************************************");
#endif
	}

	return innerNodes;
}

/// <summary>
/// Computes 3 splits for a BVH4 node. The splits are hierarchical and the node must
/// have a box set before calling this function.
/// If splitDim is not specified, then the largest axis of box is used for the first
/// split. Otherwise, splitDim is used.
/// </summary>
/// <param name="splitData"></param>
/// <param name="splitDim"></param>
static void ComputeSplits4(InnerNode4BuildDataGPU& splitData, int splitDim=-1)
{
	AABB box;
	// Compute the first split
	const int   dim0   = splitDim == -1 ? splitData.box.GetLargestDimension() : splitDim;
	const float split0 = 0.5f * (splitData.box.max[dim0] + splitData.box.min[dim0]);

	splitData.dims[0] = dim0;
	splitData.splits[0] = split0;
//#ifdef DEBUG_BU
//	log_debug("[DBG] [BVH] dim[0]: %d, split: %0.3f, box: %s",
//		splitData.dims[0], split0, splitData.box.ToString().c_str());
//#endif

	// Look at the left sub box and compute the next dimension on that side
	box = splitData.box;
	box.max[dim0] = split0;
	const int   dim1   = box.GetLargestDimension();
	const float split1 = 0.5f * (box.max[dim1] + box.min[dim1]);
	splitData.dims[1]   = dim1;
	splitData.splits[1] = split1;
//#ifdef DEBUG_BU
//	log_debug("[DBG] [BVH] dim[1]: %d, split: %0.3f, box: %s",
//		splitData.dims[1], split1, box.ToString().c_str());
//#endif

	// Repeat for the right sub box
	box = splitData.box;
	box.min[dim0] = split0;
	const int   dim2   = box.GetLargestDimension();
	const float split2 = 0.5f * (box.max[dim2] + box.min[dim2]);
	splitData.dims[2]   = dim2;
	splitData.splits[2] = split2;
//#ifdef DEBUG_BU
//	log_debug("[DBG] [BVH] dim[2]: %d, split: %0.3f, box: %s",
//		splitData.dims[2], split2, box.ToString().c_str());
//#endif

	for (int i = 0; i < BU_PARTITIONS; i++)
	{
		splitData.nextMin[i] = FLT_MAX;
		splitData.nextMax[i] = -FLT_MAX;
	}
}

static void DirectBVH4Init(
	const int numPrimitives,
	const int numInnerNodes,
	AABB centroidBox,
	BuilderItem* leafParents,
	InnerNode4BuildDataGPU* innerNodeBuildData,
	int& root_out,
	BVHNode** buffer_out)
{
#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] centroidBox: %s", centroidBox.ToString().c_str());
#endif

	root_out = 0;
	g_directBuilderFirstActiveInnerNode = 0;
	g_directBuilderNextNode = 1; // The root already exists, so the next available inner node is idx 1

	// All leafs are initially connected to the root, but they are not classified yet
	for (int i = 0; i < numPrimitives; i++)
	{
		leafParents[i].parentIndex = root_out;
		leafParents[i].side = BU_NONE;
	}

	// Inner nodes need no initialization, they are filled out as needed.
	// We only need to initialize the root.
	InnerNode4BuildDataGPU rootSplitData = { 0 };
	rootSplitData.box = centroidBox;
	rootSplitData.fitCounterTarget = numPrimitives;
	innerNodeBuildData[root_out] = rootSplitData;
	ComputeSplits4(innerNodeBuildData[root_out]);

	//for (int i = 0; i < BU_PARTITIONS - 1; i++)
	//	log_debug("[DBG] [BVH] split[%d]: %0.3f", i, rootSplitData.splits[i]);

	// Initialize the root's parent (this stops the Refit() operation)
	//innerNodes[root_out].parent = -1;

	// Create the QBVH buffer and initialize the root
	(*buffer_out) = new BVHNode[numInnerNodes + numPrimitives];
	BVHNode root     = { 0 };
	root.rootIdx     = root_out;
	root.ref         = -1;
	root.parent      = -1; // Needed to stop the Refit4() operation
	for (int i = 0; i < 4; i++)
		root.children[i] = -1;
	(*buffer_out)[0] = root;
}

static bool DirectBVH4Classify(
	int iteration,
	const int numPrimitives,
	std::vector<LeafItem>& leafItems,
	BuilderItem* leafParents,
	InnerNode4BuildDataGPU* innerNodeBuildData,
	BVHNode *buffer)
{
#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] --------------------------------------------");
	log_debug("[DBG] [BVH] ITER: %d, PHASE 2. Classify prims", iteration);
#endif

	bool done = true;
	for (int primIdx = 0; primIdx < numPrimitives; primIdx++)
	{
		int parentNodeIndex = leafParents[primIdx].parentIndex;
		LeafItem leaf = leafItems[primIdx];

		// Skip inactive primitives
		if (parentNodeIndex == -1)
			continue;

		// We're going to process at least one primitive, so we're not done yet.
		// This is a global, but it doesn't have to be an atomic because all threads
		// will attempt to write the same value.
		done = false;

		InnerNode4BuildDataGPU& innerNodeBD = innerNodeBuildData[parentNodeIndex];
		AABB    box          = innerNodeBD.box;
		Vector3 boxRange     = box.GetRange();
		bool    skipClassify = innerNodeBD.skipClassify;

		if (skipClassify)
		{
			// Don't classify this primitive, just put it on the next available slot
			const int slot = buffer[parentNodeIndex].numChildren;
			buffer[parentNodeIndex].numChildren++; // ATOMIC
			innerNodeBD.counts[slot] = 1;
			leafParents[primIdx] = { parentNodeIndex, (int8_t)slot };
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] SKIP. (%d, (%0.1f, %0.1f, %0.1f)) -> slot[%d]:%d",
				primIdx, leaf.centroid[0], leaf.centroid[1], leaf.centroid[2],
				slot, parentNodeIndex);
#endif
			continue;
		}

		AABB subBox = box;
		bool comps[BU_PARTITIONS - 1] = { false };
		for (int k = 0; k < BU_PARTITIONS - 1; k++)
		{
			int   dim       = innerNodeBD.dims[k];
			float split     = innerNodeBD.splits[k];
			bool  nullRange = (fabs(boxRange[dim]) < 0.0001f);
			float keySplit  = split;
			float key       = leaf.centroid[dim];

			// Take care of null ranges
			if (nullRange)
			{
				key = (float)primIdx;
				// Use InterlockedExchange() to flip between 1 and -1 in a GPU:
				if (keySplit >= 0.0f)
				{
					keySplit = key + 1.0f;
					innerNodeBD.splits[k] = -1.0f;
				}
				else
				{
					keySplit = key - 1.0f;
					innerNodeBD.splits[k] = 1.0f;
				}
#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] WARNING! NULL RANGE! primIndex: %d, slot: %d, box: %s",
					primIdx, k, box.ToString().c_str());
#endif
			}

			comps[k] = key < keySplit;
//#ifdef DEBUG_BU
//			log_debug("[DBG] [BVH] primIdx: %d, k: %d, key: %0.3f, keySplit: %0.3f, dim: %d",
//				primIdx, k, key, keySplit, dim);
//#endif
		}

		int reduceDim = -1;
		int slot = -1;
		if (comps[0])
		{
			reduceDim = innerNodeBD.dims[1];
			slot = comps[1] ? 0 : 1;
		}
		else
		{
			reduceDim = innerNodeBD.dims[2];
			slot = comps[2] ? 2 : 3;
		}
		innerNodeBD.nextDim[slot] = reduceDim;
		innerNodeBD.nextMin[slot] = min(innerNodeBD.nextMin[slot], leaf.centroid[reduceDim]); // ATOMIC
		innerNodeBD.nextMax[slot] = max(innerNodeBD.nextMax[slot], leaf.centroid[reduceDim]); // ATOMIC
		innerNodeBD.counts[slot]++; // ATOMIC
		leafParents[primIdx] = { parentNodeIndex, (int8_t)slot };

#ifdef DEBUG_BU
		log_debug("[DBG] [BVH] comp:%d,%d,%d, (%d, (%0.1f, %0.1f, %0.1f)) -> slot[%d]:%d, reduceDim:%d, nextMinMax: [%0.3f, %0.3f]",
			comps[0], comps[1], comps[2],
			primIdx, leaf.centroid[0], leaf.centroid[1], leaf.centroid[2],
			slot, parentNodeIndex, reduceDim,
			innerNodeBD.nextMin[slot], innerNodeBD.nextMax[slot]);
#endif
	}

	return done;
}

static void DirectBVH4EmitInnerNode(
	int parentNodeIndex,
	int slot,
	int newNodeIndex,
	bool computeSplits,
	int nextDim, // Only used if computeSplits == true
	InnerNode4BuildDataGPU* innerNodeBuildData,
	AABB box,
	BVHNode* buffer)
{
	// Connect this inner node to the new one:
	buffer[parentNodeIndex].children[slot] = newNodeIndex;

	BVHNode newNode = { 0 };
	newNode.parent = parentNodeIndex;
	newNode.ref = -1;
	for (int j = 0; j < 4; j++)
		newNode.children[j] = -1;
	buffer[newNodeIndex] = newNode;

	// Connect the current inner node to the new one
	buffer[parentNodeIndex].children[slot] = newNodeIndex;
	buffer[parentNodeIndex].numChildren++;

	// Enable the new inner node
	InnerNode4BuildDataGPU newInnerNode = { 0 };
	newInnerNode.box = box;
	newInnerNode.fitCounterTarget = innerNodeBuildData[parentNodeIndex].counts[slot];
	if (computeSplits)
		ComputeSplits4(newInnerNode, nextDim);
	else
		newInnerNode.skipClassify = true;
	innerNodeBuildData[newNodeIndex] = newInnerNode;
}

static void DirectBVH4EmitInnerNodes(
	int iteration,
	const int numPrimitives,
	const int numInnerNodes,
	InnerNode4BuildDataGPU* innerNodeBuildData,
	BVHNode* buffer)
{
#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] --------------------------------------------");
	log_debug("[DBG] [BVH] ITER: %d, PHASE 3. Emit inner nodes", iteration);
#endif

	// We don't need to iterate over all the inner nodes, since only a fraction of them
	// will actually be active during the algorithm
	const int lastActiveInnerNode = g_directBuilderNextNode;

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] lastActiveInnerNode: %d", lastActiveInnerNode);
#endif

	for (int i = g_directBuilderFirstActiveInnerNode; i < lastActiveInnerNode; i++)
	{
		InnerNode4BuildDataGPU& innerNodeBD = innerNodeBuildData[i];

		// Leaves and inner nodes can be interleaved now, so we need to be careful and skip the leaves
		if (buffer[i].ref != -1)
			continue;

		// If skipClassify is set, then we temporarily used the numChildren field to place the
		// primitives into slots. We need to reset the count here.
		// ... or maybe we can just reset numChildren here anyway since the Emit operation is going
		// to take care of that for us anyway.
		if (innerNodeBD.skipClassify)
			buffer[i].numChildren = 0;

		AABB  innerNodeBox = innerNodeBD.box; // this is the centroidBox used in the previous cut
		const int   dim0   = innerNodeBD.dims[0];
		const float split0 = innerNodeBD.splits[0];

		// Count all the children. If a slot has:
		//
		// 5..N children: emit 1 new inner node for this slot, classify on the next iteration.
		// 2..4 children: emit 1 new inner node for this slot, do not classify on the next iteration.
		//    1 children: emit 1 leaf node for this slot (populate in the next phase).
		//    0 children: no new nodes are emitted for this slot.
		//
		// So, the total number of new nodes (either leaves or inner nodes) emitted by this node is equal
		// to the number of nonzero slots, and this information is known when EmitInnerNodes() is running.
		int numChildren = 0;
		for (int k = 0; k < BU_PARTITIONS; k++)
			numChildren += (innerNodeBD.counts[k] > 0) ? 1 : 0;

		/*
		int startOffset = g_directBuilderNextNode;
		g_directBuilderNextNode += numChildren; // ATOMIC: Reserve all the nodes we'll need in one go
		// Assign node offsets for each slot
		for (int k = 0; k < BU_PARTITIONS; k++)
		{
			if (innerNodeBD.counts[k] > 0)
			{
				innerNodeBD.childOffsets[k] = startOffset++;
#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] node slot[%d]:%d, childOffset: %d",
					k, i, innerNodeBD.childOffsets[k]);
#endif
			}
		}
		// Our children offsets are now contiguous and must be used for encoding on the QBVH buffer
		*/

		for (int k = 0; k < BU_PARTITIONS; k++)
		{
			// Preconditions: All active inner nodes have already been encoded.
			// If we have between 2 and 4 children, we can emit one new inner node and put
			// all those children there right away. The single-children case is handled separately
			// but maybe later that case can be factored too.
			if (1 < innerNodeBD.counts[k] && innerNodeBD.counts[k] <= BU_PARTITIONS)
			{
				// Skip Classify path.
				// Emit a new inner node for this slot but don't split the subrange anymore since
				// we can fit all the children in this slot under the new inner node.
				int newNodeIndex = g_directBuilderNextNode;
				g_directBuilderNextNode++; // ATOMIC
				DirectBVH4EmitInnerNode(i, k, newNodeIndex, false, -1, innerNodeBuildData, AABB(), buffer);

#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] QBVHIdx: (I) %d --> slot[%d]:%d, SKIP, fitCounterTarget: %d",
					newNodeIndex, k, i, newInnerNode.fitCounterTarget);
#endif
			}
			else if (innerNodeBD.counts[k] > BU_PARTITIONS)
			{
				// Emit a new inner node and split again
				int   dimk   = k < 2 ? innerNodeBD.dims[1]   : innerNodeBD.dims[2];
				float splitk = k < 2 ? innerNodeBD.splits[1] : innerNodeBD.splits[2];

				int   nextDim = innerNodeBD.nextDim[k];
				float nextMin = innerNodeBD.nextMin[k];
				float nextMax = innerNodeBD.nextMax[k];

#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] inner node %d, slot:%d, nextMinMax: [%0.3f, %0.3f]",
					i, k, nextMin, nextMax);
#endif

				// Prepare the boxes for the next split
				AABB box = innerNodeBox;
				if (k < 2) // Left side of the main split
					box.max[dim0] = split0;
				else // Right side of the main split
					box.min[dim0] = split0;

				if (k == 0 || k == 2)
					box.max[dimk] = splitk;
				else
					box.min[dimk] = splitk;
				box.min[nextDim] = nextMin;
				box.max[nextDim] = nextMax;

#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] inner node %d, slot:%d, counts: %d, box: %s",
					i, k, innerNodeBD.counts[k], box.ToString().c_str());
#endif

				// Emit a new inner node for this slot and split the subrange again
				int newNodeIndex = g_directBuilderNextNode;
				g_directBuilderNextNode++; // ATOMIC
				DirectBVH4EmitInnerNode(i, k, newNodeIndex, true, nextDim, innerNodeBuildData, box, buffer);
#ifdef DEBUG_BU
				log_debug("[DBG] [BVH] QBVHIdx: (I) %d --> %d, fitCounterTarget: %d",
					newNodeIndex, i, newInnerNode.fitCounterTarget);
#endif
			}
		}
	}

	g_directBuilderFirstActiveInnerNode = lastActiveInnerNode;
}

static void DirectBVH4EmitLeaf(
	const int slot,
	const int primIndex,
	const int parentIndex,
	const int leafIndex,
	BuilderItem* leafParents,
	std::vector<LeafItem>& leafItems,
	InnerNode4BuildDataGPU* innerNodeBuildData,
	BVHNode* buffer,
	const XwaVector3* vertices, const int* indices)
{
	buffer[parentIndex].children[slot] = leafIndex;
	buffer[parentIndex].numChildren++;

	innerNodeBuildData[parentIndex].fitCounter++;
	// Deactivate this primitive for the next iteration
	leafParents[primIndex].parentIndex = -1;

	// Encode the leaf proper at offset newNodeIndex
	EncodeLeafNode(buffer, leafItems, primIndex, leafIndex, vertices, indices);
	// Connect the new leaf to its parent
	buffer[leafIndex].parent = parentIndex;

#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] QBVHIdx: (L) %d --> %d", newNodeIndex, parentIndex);
	if (slot >= 0)
		log_debug("[DBG] [BVH] prim %d still points to slot[%d]:%d and it's a leaf, deactivated. Parent fitCounter: %d",
			primIndex, slot, parentIndex, innerNodeBuildData[parentIndex].fitCounter);
	else
		log_debug("[DBG] [BVH] prim %d now points to %d and it's a leaf, deactivated. Parent fitCounter: %d",
			primIndex, parentIndex, innerNodeBuildData[parentIndex].fitCounter);
#endif

	if (innerNodeBuildData[parentIndex].fitCounter == innerNodeBuildData[parentIndex].fitCounterTarget)
	{
		// The parent node can be refit now
#ifdef DEBUG_BU
		log_debug("[DBG] [BVH] REFITTING inner node: %d", parentIndex);
#endif
		Refit4(parentIndex, buffer, innerNodeBuildData, leafItems);
	}
}

static void DirectBVH4InitNextIteration(
	int iteration,
	const int numPrimitives,
	const int numInnerNodes,
	BuilderItem* leafParents,
	std::vector<LeafItem>& leafItems,
	InnerNode4BuildDataGPU* innerNodeBuildData,
	BVHNode *buffer,
	const XwaVector3* vertices, const int* indices)
{
#ifdef DEBUG_BU
	log_debug("[DBG] [BVH] --------------------------------------------");
	log_debug("[DBG] [BVH] ITER: %d, PHASE 4. Init next iteration, disable prims", iteration);
#endif

	for (int primIdx = 0; primIdx < numPrimitives; primIdx++)
	{
		const int parentIndex = leafParents[primIdx].parentIndex;
		const int slot = leafParents[primIdx].side;

		// Skip inactive primitives
		if (parentIndex == -1)
			continue;

		// Emit leaves if applicable or update parent pointers
		InnerNode4BuildDataGPU& innerNodeBD = innerNodeBuildData[parentIndex];

		if (innerNodeBD.counts[slot] == 1)
		{
			//const int encodeIndex = innerNodeBD.childBufOffsets[slot];
			const int leafEncodeIdx = g_directBuilderNextNode;
			g_directBuilderNextNode++; // ATOMIC

			// This node is a leaf of parentIndex at the current slot, connect it
			DirectBVH4EmitLeaf(slot, primIdx, parentIndex, leafEncodeIdx,
				leafParents, leafItems, innerNodeBuildData, buffer, vertices, indices);
		}
		else // if (innerNodeBD.counts[slot] > 1)
		{
			// The parent of this leaf emitted a new node.
			const int newParentIndex = buffer[parentIndex].children[slot];
			leafParents[primIdx].parentIndex = newParentIndex;
			// The parent of this leaf has too many children, loop again.
			leafParents[primIdx].side = BU_NONE;
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] prim %d now points to %d, prev parent: slot[%d]:%d",
				primIdx, leafParents[primIdx].parentIndex, slot, parentIndex);
#endif
		}
	}
}

#ifdef DISABLED
static void TestBVH4Buffer(InnerNode4BuildDataGPU* innerNodeBuildData, BVHNode* buffer, int curNodeIdx)
{
	int counter = 0;

	for (int i = 0; i < g_directBuilderNextNode; i++)
	{
		if (innerNodeBuildData[i].refit == 0)
			log_debug("[DBG] [BVH] ERROR: Node %d was not refit", i);
		else if (innerNodeBuildData[i].refit > 1)
			log_debug("[DBG] [BVH] ERROR: Node %d was refit %d times", i, innerNodeBuildData[i].refit);
		else
		{
			counter++;

			BVHNode& node = buffer[curNodeIdx];
			for (int i = 0; i < 3; i++)
			{
				if (node.min[i] < -10000 * METERS_TO_OPT)
					log_debug("[DBG] [BVH] min[%d] is too large", i);
				if (node.max[i] > 10000 * METERS_TO_OPT)
					log_debug("[DBG] [BVH] max[%d] is too large", i);
			}
			if (fabs(node.min[3] - 1.0f) > 0.001f)
				log_debug("[DBG] [BVH] Wrong w: %0.3f", node.min[3]);
			if (fabs(node.max[3] - 1.0f) > 0.001f)
				log_debug("[DBG] [BVH] Wrong w: %0.3f", node.max[3]);
		}

		// Check that each inner node has compressed children indices
		for (int k = 0; k < buffer[i].numChildren; k++)
		{
			if (buffer[i].children[k] < 0)
				log_debug("[DBG] [BVH] Found -1 inside numChildren for node %d", i);
		}

		for (int k = buffer[i].numChildren; i < 4; i++)
		{
			if (buffer[i].children[k] >= 0)
				log_debug("[DBG] [BVH] Found child outside numChildren range for node %d", i);
		}
	}

	log_debug("[DBG] [BVH] %d nodes out of %d were refit once", counter, g_directBuilderNextNode);
}
#endif

static AABB RefitBVH4Buffer(BVHNode* buffer, const int numPrimitives, const int numInnerNodes, int curNodeIdx,
	std::vector<LeafItem> &leafItems)
{
	AABB box;

	// Don't process leaves
	if (buffer[curNodeIdx].ref != -1)
	{
		return leafItems[curNodeIdx - numInnerNodes].aabb;
	}

	// Compress the children indices
	BVHNode& node = buffer[curNodeIdx];
	int tmpIndices[BU_PARTITIONS] = { -1, -1, -1, -1 };
	int destIdx = 0;
	for (int k = 0; k < BU_PARTITIONS; k++)
	{
		if (node.children[k] != -1)
			tmpIndices[destIdx++] = node.children[k];
	}
	for (int k = 0; k < BU_PARTITIONS; k++)
		node.children[k] = tmpIndices[k];

	for (int i = 0; i < BU_PARTITIONS; i++)
	{
		const int childIdx = buffer[curNodeIdx].children[i];
		if (childIdx != -1)
			box.Expand(RefitBVH4Buffer(buffer, numPrimitives, numInnerNodes, childIdx, leafItems));
	}

	for (int i = 0; i < 3; i++)
	{
		buffer[curNodeIdx].min[i] = box.min[i];
		buffer[curNodeIdx].max[i] = box.max[i];
	}
	buffer[curNodeIdx].min[3] = 1.0f;
	buffer[curNodeIdx].max[3] = 1.0f;

	return box;
}

BVHNode* DirectBVH4BuilderGPU(AABB centroidBox, std::vector<LeafItem>& leafItems, int& numNodes,
	const XwaVector3* vertices, const int* indices)
{
	const int numPrimitives = leafItems.size();
	const int numInnerNodes = CalcNumInnerQBVHNodes(numPrimitives);
	numNodes = numPrimitives + numInnerNodes;
	int root = -1;
	BVHNode* QBVHBuffer = nullptr;

	//log_debug("[DBG] [BVH] numTris: %d, numInnerNodes: %d, numNodes: %d",
	//	numPrimitives, numInnerNodes, numNodes);

	BuilderItem* leafParents = new BuilderItem[numPrimitives];
	InnerNode4BuildDataGPU* innerNodeBuildData = new InnerNode4BuildDataGPU[numInnerNodes + numPrimitives];
	DirectBVH4Init(numPrimitives, numInnerNodes, centroidBox, leafParents, innerNodeBuildData, root, &QBVHBuffer);

	// The worst-case scenario is that we have to iterate once for every inner node:
	const int maxIterations = numInnerNodes;
	for (int i = 0; i < maxIterations; i++)
	{
		const bool done = DirectBVH4Classify(i, numPrimitives, leafItems, leafParents, innerNodeBuildData, QBVHBuffer);
		if (done)
		{
#ifdef DEBUG_BU
			log_debug("[DBG] [BVH] All leaves have been processed.");
#endif
			break;
		}
		DirectBVH4EmitInnerNodes(i, numPrimitives, numInnerNodes, innerNodeBuildData, QBVHBuffer);
		DirectBVH4InitNextIteration(i, numPrimitives, numInnerNodes, leafParents, leafItems, innerNodeBuildData, QBVHBuffer, vertices, indices);
	}

	//log_debug("[DBG] [BVH] Inner nodes used: %d, leaves used: %d, actual nodes: %d",
	//	g_directBuilderNextNode, numPrimitives, g_directBuilderNextNode + numPrimitives);

	//RefitBVH4Buffer(QBVHBuffer, numPrimitives, numInnerNodes, root, leafItems);
	//TestBVH4Buffer(innerNodeBuildData, QBVHBuffer, root);

	delete[] leafParents;
	delete[] innerNodeBuildData;

	return QBVHBuffer;
}

BVHNode* AVLBuilder(std::vector<LeafItem>& leafItems, int &numNodes, const XwaVector3* vertices, const int* indices)
{
	const int numPrimitives = leafItems.size();
	BVHNode* QBVHBuffer = nullptr;

	//log_debug("[DBG] [BVH] numTris: %d, numInnerNodes: %d, numNodes: %d",
	//	numPrimitives, numInnerNodes, numNodes);

	TreeNode* T = nullptr;
	for (LeafItem& leaf : leafItems)
	{
		XwaVector3 centroid(leaf.centroid);
		T = InsertAVL(T, leaf.PrimID, centroid, leaf.aabb);
	}

	//log_debug("[DBG] [BVH] T->box: %s", T->box.ToString().c_str());
	QTreeNode* Q = BinTreeToQTree(T);
	DeleteTree(T);
	//log_debug("[DBG] [BVH] Q->box: %s", Q->box.ToString().c_str());

	BVHNode* result = (BVHNode *)EncodeNodes(Q, vertices, indices);
	numNodes = Q->GetNumNodes();
	result[0].rootIdx = 0;
	//log_debug("[DBG] [BVH] buffer min: %0.3f, %0.3f, %0.3f",
	//	result[0].min[0], result[0].min[1], result[0].min[2]);
	//log_debug("[DBG] [BVH] buffer max: %0.3f, %0.3f, %0.3f",
	//	result[0].max[0], result[0].max[1], result[0].max[2]);

	//log_debug("[DBG] [BVH] Inner nodes used: %d, leaves used: %d, actual nodes: %d",
	//	g_directBuilderNextNode, numPrimitives, g_directBuilderNextNode + numPrimitives);

	//RefitBVH4Buffer(QBVHBuffer, numPrimitives, numInnerNodes, root, leafItems);
	//TestBVH4Buffer(innerNodeBuildData, QBVHBuffer, root);

	return result;
}

void TestFastLBVH()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLBVH() START");
	std::vector<LeafItem> leafItems;
	AABB aabb;
	// This is the example from Apetrei 2014.
	// Here the TriID is the same as the morton code for debugging purposes.
	leafItems.push_back({ 4, Vector3(), aabb, 4 });
	leafItems.push_back({ 12, Vector3(), aabb, 12 });
	leafItems.push_back({ 3, Vector3(), aabb, 3 });
	leafItems.push_back({ 13, Vector3(), aabb, 13 });
	leafItems.push_back({5, Vector3(), aabb, 5 });
	leafItems.push_back({2, Vector3(), aabb, 2 });
	leafItems.push_back({15, Vector3(), aabb, 15 });
	leafItems.push_back({8, Vector3(), aabb, 8 });

	// Sort by the morton codes
	std::sort(leafItems.begin(), leafItems.end(), leafSorter);

	int root = -1;
	InnerNode* innerNodes = FastLBVH(leafItems, &root);

	log_debug("[DBG] [BVH] ****************************************************************");
	int numLeaves = leafItems.size();
	int numInnerNodes = numLeaves - 1;
	for (int i = 0; i < numInnerNodes; i++)
	{
		log_debug("[DBG] [BVH] node: %d, left,right: %s%d, %s%d",
			i,
			innerNodes[i].leftIsLeaf ? "(L)" : "", innerNodes[i].left,
			innerNodes[i].rightIsLeaf ? "(L)" : "", innerNodes[i].right);
	}

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing Tree");
	printTree(0, root, false, innerNodes);

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] BVH2 --> QBVH conversion");
	QTreeNode* Q = BinTreeToQTree(root, false, innerNodes, leafItems);
	delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing QTree");
	PrintTree("", Q);
	DeleteTree(Q);

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLBVH() END");
}

void TestFastLQBVH()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLQBVH() START");
	std::vector<LeafItem> leafItems;
	AABB aabb;
	// This is the example from Apetrei 2014.
	// Here the TriID is the same as the morton code for debugging purposes.
	leafItems.push_back({ 4, Vector3(), aabb, 0 });
	leafItems.push_back({ 12, Vector3(), aabb, 1 });
	leafItems.push_back({ 3, Vector3(), aabb, 2 });
	leafItems.push_back({ 13, Vector3(), aabb, 3 });
	leafItems.push_back({ 5, Vector3(), aabb, 4 });
	leafItems.push_back({ 2, Vector3(), aabb, 5 });
	leafItems.push_back({ 15, Vector3(), aabb, 6 });
	leafItems.push_back({ 8, Vector3(), aabb, 7 });

	// Sort by the morton codes
	std::sort(leafItems.begin(), leafItems.end(), leafSorter);

	int root = -1;
	InnerNode4* innerNodes = FastLQBVH(leafItems, root);
	int totalNodes = innerNodes[root].totalNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] root: %d, totalNodes: %d", root, totalNodes);
	int numLeaves = leafItems.size();
	int numInnerNodes = numLeaves - 1;
	for (int i = 0; i < numInnerNodes; i++)
	{
		std::string msg = "";
		for (int j = 0; j < innerNodes[i].numChildren; j++) {
			msg += (innerNodes[i].isLeaf[j] ? "(L)" : "") +
				   std::to_string(innerNodes[i].children[j]) + ", ";
		}

		log_debug("[DBG] [BVH] node: %d, numChildren: %d| %s",
			i, innerNodes[i].numChildren, msg.c_str());
	}

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing Tree");
	printTree(0, root, false, innerNodes);

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Encoding Buffer");
	BVHNode* buffer = (BVHNode *)EncodeNodes(root, innerNodes, leafItems, nullptr, nullptr);
	log_debug("[DBG] [BVH] Printing Buffer");
	PrintTreeBuffer("", buffer, 0);
	delete[] buffer;
	delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLQBVH() END");
}

void TestFastLQBVHEncode()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLQBVHEncode() START");
	std::vector<LeafItem> leafItems;
	AABB aabb;
	// This is the example from Apetrei 2014.
	// Here the TriID is the same as the morton code for debugging purposes.
	leafItems.push_back({ 4, Vector3(), aabb, 0 });
	leafItems.push_back({ 12, Vector3(), aabb, 1 });
	leafItems.push_back({ 3, Vector3(), aabb, 2 });
	leafItems.push_back({ 13, Vector3(), aabb, 3 });
	leafItems.push_back({ 5, Vector3(), aabb, 4 });
	leafItems.push_back({ 2, Vector3(), aabb, 5 });
	leafItems.push_back({ 15, Vector3(), aabb, 6 });
	leafItems.push_back({ 8, Vector3(), aabb, 7 });

	// Sort by the morton codes
	std::sort(leafItems.begin(), leafItems.end(), leafSorter);

	int numTris = leafItems.size();
	int numQBVHInnerNodes = CalcNumInnerQBVHNodes(numTris);
	int numQBVHNodes = numTris + numQBVHInnerNodes;
	BVHNode* QBVHBuffer = new BVHNode[numQBVHNodes];

	log_debug("[DBG] [BVH] numTris: %d, numQBVHInnerNodes: %d, numQBVHNodes: %d",
		numTris, numQBVHInnerNodes, numQBVHNodes);

	// Encode the leaves
	//int LeafOfs = numQBVHInnerNodes * sizeof(BVHNode) / 4;
	int LeafEncodeIdx = numQBVHInnerNodes;
	for (unsigned int i = 0; i < leafItems.size(); i++)
	{
		EncodeLeafNode(QBVHBuffer, leafItems, i, LeafEncodeIdx++, nullptr, nullptr);
	}

	int root = -1;
	SingleStepFastLQBVH(QBVHBuffer, numQBVHInnerNodes, leafItems, root);
	int totalNodes = numQBVHNodes - root;

	log_debug("[DBG] [BVH] root: %d, totalNodes: %d", root, totalNodes);
	log_debug("[DBG] [BVH] ****************************************************************");
	/*
	int numLeaves = leafItems.size();
	int numInnerNodes = numLeaves - 1;
	for (int i = 0; i < numInnerNodes; i++)
	{
		std::string msg = "";
		for (int j = 0; j < innerNodes[i].numChildren; j++) {
			msg += (innerNodes[i].isLeaf[j] ? "(L)" : "") +
				std::to_string(innerNodes[i].children[j]) + ", ";
		}

		log_debug("[DBG] [BVH] node: %d, numChildren: %d| %s",
			i, innerNodes[i].numChildren, msg.c_str());
	}
	*/

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Tree");
	//printTree(0, inner_root, false, innerNodes);
	//delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing Buffer");
	PrintTreeBuffer("", QBVHBuffer, root);
	delete[] QBVHBuffer;
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLQBVHEncode() END");
}

void TestRedBlackBVH()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestRedBlackBVH() START");

	AABB box;
	TreeNode* T = nullptr;
	Matrix4 m;

	/*
	T = InsertRB(T, 0, 0, box, m);
	T = InsertRB(T, 1, 1, box, m);
	T = InsertRB(T, 2, 2, box, m);
	T = InsertRB(T, 3, 3, box, m);
	T = InsertRB(T, 4, 4, box, m);
	T = InsertRB(T, 5, 5, box, m);
	T = InsertRB(T, 6, 6, box, m);
	T = InsertRB(T, 7, 7, box, m);
	T = InsertRB(T, 8, 8, box, m);
	T = InsertRB(T, 9, 9, box, m);
	T = InsertRB(T, 10, 10, box, m);
	T = InsertRB(T, 11, 11, box, m);
	T = InsertRB(T, 12, 12, box, m);
	*/

	T = InsertRB(T, 4, 4, box, m);
	T = InsertRB(T, 12, 12, box, m);
	T = InsertRB(T, 3, 3, box, m);
	T = InsertRB(T, 13, 13, box, m);
	T = InsertRB(T, 5, 5, box, m);
	T = InsertRB(T, 2, 2, box, m);
	T = InsertRB(T, 15, 15, box, m);
	T = InsertRB(T, 8, 8, box, m);

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing Tree");
	PrintTree("", T);
	DeleteTree(T);

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestRedBlackBVH() END");
}

void TestDirectBVH()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestKDTree() START");
	// using LeafItem = std::tuple<MortonCode_t, AABB, int>;
	std::vector<LeafItem> leafItems;
	std::vector<int> leafIndices;

	AABB aabb, sceneAABB;
	// This is the example from Apetrei 2014.
	// Here the TriID is the same as the morton code for debugging purposes.
	aabb.min = Vector3(-2.0f, -2.0f, 0.0f);
	aabb.max = Vector3(-1.8f, -1.75f, 0.0f);
	leafItems.push_back({ 4, Vector3(), aabb, 0 });
	leafIndices.push_back(0);
	sceneAABB.Expand(aabb);

	aabb.min = Vector3(-1.9f, -1.9f, 0.0f);
	aabb.max = Vector3(-1.7f, -1.8f, 0.0f);
	leafItems.push_back({ 12, Vector3(), aabb, 1 });
	leafIndices.push_back(1);
	sceneAABB.Expand(aabb);

	aabb.min = Vector3(1.8f, -1.6f, 0.0f);
	aabb.max = Vector3(2.1f, -1.45f, 0.0f);
	leafItems.push_back({ 3, Vector3(), aabb, 2 });
	leafIndices.push_back(2);
	sceneAABB.Expand(aabb);

	aabb.min = Vector3(1.7f, -1.3f, 0.0f);
	aabb.max = Vector3(1.8f, -1.2f, 0.0f);
	leafItems.push_back({ 13, Vector3(), aabb, 3 });
	leafIndices.push_back(3);
	sceneAABB.Expand(aabb);

	aabb.min = Vector3(-1.5f, 0.7f, 0.0f);
	aabb.max = Vector3(-1.25f, 0.95f, 0.0f);
	leafItems.push_back({ 5, Vector3(), aabb, 4 });
	leafIndices.push_back(4);
	sceneAABB.Expand(aabb);

	aabb.min = Vector3(-1.3f, 1.7f, 0.0f);
	aabb.max = Vector3(-1.4f, 1.8f, 0.0f);
	leafItems.push_back({ 2, Vector3(), aabb, 5 });
	leafIndices.push_back(5);
	sceneAABB.Expand(aabb);

	aabb.min = Vector3(-0.3f, 0.7f, 0.0f);
	aabb.max = Vector3(0.4f, 1.2f, 0.0f);
	leafItems.push_back({ 15, Vector3(), aabb, 6 });
	leafIndices.push_back(6);
	sceneAABB.Expand(aabb);

	aabb.min = Vector3(-0.2f, -0.3f, 0.0f);
	aabb.max = Vector3(0.1f, 0.2f, 0.0f);
	leafItems.push_back({ 8, Vector3(), aabb, 7 });
	leafIndices.push_back(7);
	sceneAABB.Expand(aabb);

	int numTris = leafItems.size();
	int numQBVHInnerNodes = CalcNumInnerQBVHNodes(numTris);
	int numQBVHNodes = numTris + numQBVHInnerNodes;

	log_debug("[DBG] [BVH] numTris: %d, numQBVHInnerNodes: %d, numQBVHNodes: %d",
		numTris, numQBVHInnerNodes, numQBVHNodes);
	log_debug("[DBG] [BVH] scene: %s", sceneAABB.ToString().c_str());

	//TreeNode *tree = BuildKDTree(sceneAABB, leafItems);
	TreeNode* tree = BuildDirectBVH(sceneAABB, leafItems, leafIndices, sceneAABB.GetLargestDimension());

	PrintTree("", tree);

	DeleteTree(tree);
	return;
	BVHNode* QBVHBuffer = new BVHNode[numQBVHNodes];

	// Encode the leaves
	//int LeafOfs = numQBVHInnerNodes * sizeof(BVHNode) / 4;
	int LeafEncodeIdx = numQBVHInnerNodes;
	for (unsigned int i = 0; i < leafItems.size(); i++)
	{
		EncodeLeafNode(QBVHBuffer, leafItems, i, LeafEncodeIdx++, nullptr, nullptr);
	}

	int root = -1;
	SingleStepFastLQBVH(QBVHBuffer, numQBVHInnerNodes, leafItems, root);
	int totalNodes = numQBVHNodes - root;

	log_debug("[DBG] [BVH] root: %d, totalNodes: %d", root, totalNodes);
	log_debug("[DBG] [BVH] ****************************************************************");

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Tree");
	//printTree(0, inner_root, false, innerNodes);
	//delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing Buffer");
	PrintTreeBuffer("", QBVHBuffer, root);
	delete[] QBVHBuffer;
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLQBVHEncode() END");
}

void TestImplicitMortonCodes()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestImplicitMortonCodes() START");
	// using LeafItem = std::tuple<MortonCode_t, AABB, int>;
	std::vector<LeafItem> leafItems;

	AABB aabb, sceneAABB;
	// Init the leaves
	{
		// This is the example from Apetrei 2014.
		// Here the TriID is the same as the morton code for debugging purposes.
		aabb.min = Vector3(2.0f, 0.0f, 0.0f);
		aabb.max = Vector3(4.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 0});
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(0.0f, 0.0f, 0.0f);
		aabb.max = Vector3(2.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 1 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(2.0f, 0.0f, 0.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 2 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(4.0f, 0.0f, 0.0f);
		aabb.max = Vector3(6.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(6.0f, 0.0f, 0.0f);
		aabb.max = Vector3(8.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 4 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(3.0f, 0.0f, 0.0f);
		aabb.max = Vector3(5.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 5 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(1.0f, 0.0f, 0.0f);
		aabb.max = Vector3(3.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 6 });
		sceneAABB.Expand(aabb);
	}

	AABB centroidBox;
	for (uint32_t i = 0; i < leafItems.size(); i++)
	{
		centroidBox.Expand(leafItems[i].aabb.GetCentroidVector3());
	}

	const int numPrimitives = leafItems.size();
	const int numInnerNodes = numPrimitives - 1;
	int numQBVHInnerNodes = CalcNumInnerQBVHNodes(numPrimitives);
	int numQBVHNodes = numPrimitives + numQBVHInnerNodes;

	BVHNode* QBVHBuffer = new BVHNode[numQBVHNodes];

	log_debug("[DBG] [BVH] numTris: %d, numQBVHInnerNodes: %d, numQBVHNodes: %d",
		numPrimitives, numQBVHInnerNodes, numQBVHNodes);
	log_debug("[DBG] [BVH] scene: %s", sceneAABB.ToString().c_str());

	// Encode the leaves
	/*
	int LeafEncodeIdx = numQBVHInnerNodes;
	for (unsigned int i = 0; i < leafItems.size(); i++)
	{
		EncodeLeafNode(QBVHBuffer, leafItems, i, LeafEncodeIdx++, nullptr, nullptr);
	}
	*/

	int root = -1;
	//InnerNode *innerNodes = DirectBVH2BuilderCPU(sceneAABB, centroidBox, leafItems, root);
	InnerNode* innerNodes = DirectBVH2BuilderGPU(centroidBox, leafItems, root);
	if (innerNodes == nullptr)
		return;

	log_debug("[DBG] [BVH] Tree built, printing");
	printTree(0, root, false, innerNodes);

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] BVH2 --> QBVH conversion");
	QTreeNode* Q = BinTreeToQTree(root, false, innerNodes, leafItems);

	delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing QTree");
	PrintTree("", Q);
	DeleteTree(Q);

#ifdef DISABLED
	root = -1;
	SingleStepFastLQBVH(QBVHBuffer, numQBVHInnerNodes, leafItems, root);
	int totalNodes = numQBVHNodes - root;

	log_debug("[DBG] [BVH] root: %d, totalNodes: %d", root, totalNodes);
	log_debug("[DBG] [BVH] ****************************************************************");

	//log_debug("[DBG] [BVH] ****************************************************************");
	//log_debug("[DBG] [BVH] Printing Tree");
	//printTree(0, inner_root, false, innerNodes);
	//delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing Buffer");
	PrintTreeBuffer("", QBVHBuffer, root);
	delete[] QBVHBuffer;
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestFastLQBVHEncode() END");
#endif
}

void TestImplicitMortonCodes4()
{
	// Don't forget to enable the nullptr check in EncodeLeafNode. We don't have any vertices or indices in this test!
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestImplicitMortonCodes4() START");
	// using LeafItem = std::tuple<MortonCode_t, AABB, int>;
	std::vector<LeafItem> leafItems;

	AABB aabb, sceneAABB;
	// Init the leaves
	{
		aabb.min = Vector3(2.0f, 2.0f, 1.0f);
		aabb.max = Vector3(4.0f, 2.0f, 1.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 0 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(2.2f, 2.7f, 1.3f);
		aabb.max = Vector3(2.4f, 2.8f, 1.4f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 1 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(0.0f, 4.0f, 3.0f);
		aabb.max = Vector3(2.0f, 4.0f, 3.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 2 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(2.0f, 2.0f, 1.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 2.0f, 1.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(2.0f, 2.0f, 1.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 2.0f, 1.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(2.0f, 2.0f, 1.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 2.0f, 1.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(2.0f, 2.0f, 1.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 2.0f, 1.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(2.0f, 2.0f, 1.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 2.0f, 1.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(4.0f, 3.0f, 1.2f);
		aabb.max = Vector3(6.0f, 0.0f, 1.2f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 4 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(6.0f, 5.0f, 2.5f);
		aabb.max = Vector3(8.0f, 5.0f, 2.5f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 5 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(3.0f, 2.1f, 1.3f);
		aabb.max = Vector3(5.0f, 2.1f, 1.3f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 6 });
		sceneAABB.Expand(aabb);

		aabb.min = Vector3(1.0f, 4.5f, 2.3f);
		aabb.max = Vector3(3.0f, 4.5f, 2.3f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 7 });
		sceneAABB.Expand(aabb);
	}

	AABB centroidBox;
	for (uint32_t i = 0; i < leafItems.size(); i++)
	{
		centroidBox.Expand(leafItems[i].aabb.GetCentroidVector3());
	}

	log_debug("[DBG] [BVH] scene: %s", sceneAABB.ToString().c_str());

	int root = -1;
	int numNodes = 0;
	BVHNode* QBVHBuffer = DirectBVH4BuilderGPU(centroidBox, leafItems, numNodes, nullptr, nullptr);

	// Print the inner nodes
	const int numPrimitives = leafItems.size();
	const int numInnerNodes = CalcNumInnerQBVHNodes(numPrimitives);
	for (int i = 0; i < g_directBuilderNextNode; i++)
	{
		BVHNode node = QBVHBuffer[i];
		log_debug("[DBG] [BVH] node %d, PrimID: %d, parent: %d, numChildren: %d, children: %d, %d, %d, %d",
			i, node.ref, node.parent, node.numChildren,
			node.children[0], node.children[1], node.children[2], node.children[3]);
	}
	PrintTreeBuffer("", QBVHBuffer, 0);
}

void TestAVLBuilder()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestAVLBuilder() START");
	// using LeafItem = std::tuple<MortonCode_t, AABB, int>;
	std::vector<LeafItem> leafItems;

	AABB aabb, sceneAABB;
	// Init the leaves
	{
		// 1.0
		aabb.min = Vector3(0.0f, 0.0f, 0.0f);
		aabb.max = Vector3(2.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 1 });
		sceneAABB.Expand(aabb);

		// 2.0
		aabb.min = Vector3(1.0f, 0.0f, 0.0f);
		aabb.max = Vector3(3.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 6 });
		sceneAABB.Expand(aabb);

		// 3.0
		aabb.min = Vector3(2.0f, 0.0f, 0.0f);
		aabb.max = Vector3(4.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 0 });
		sceneAABB.Expand(aabb);

		// 3.0
		aabb.min = Vector3(2.0f, 0.0f, 0.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 2 });
		sceneAABB.Expand(aabb);

		// 4.0
		aabb.min = Vector3(3.0f, 0.0f, 0.0f);
		aabb.max = Vector3(5.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 5 });
		sceneAABB.Expand(aabb);

		// 5.0
		aabb.min = Vector3(4.0f, 0.0f, 0.0f);
		aabb.max = Vector3(6.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		// 7.0
		aabb.min = Vector3(6.0f, 0.0f, 0.0f);
		aabb.max = Vector3(8.0f, 0.0f, 0.0f);
		leafItems.push_back({ 0, aabb.GetCentroidVector3(), aabb, 4 });
		sceneAABB.Expand(aabb);
	}

	const int numPrimitives = leafItems.size();

	//BVHNode* QBVHBuffer = new BVHNode[numQBVHNodes];
	/*log_debug("[DBG] [BVH] numTris: %d, numQBVHInnerNodes: %d, numQBVHNodes: %d",
		numPrimitives, numQBVHInnerNodes, numQBVHNodes);
	log_debug("[DBG] [BVH] scene: %s", sceneAABB.ToString().c_str());*/

	// Encode the leaves
	/*
	int LeafEncodeIdx = numQBVHInnerNodes;
	for (unsigned int i = 0; i < leafItems.size(); i++)
	{
		EncodeLeafNode(QBVHBuffer, leafItems, i, LeafEncodeIdx++, nullptr, nullptr);
	}
	*/

	TreeNode* T = nullptr;
	for (LeafItem &leaf : leafItems)
	{
		XwaVector3 centroid(leaf.centroid);
		log_debug("[DBG] [BVH] Inserting: %0.3f", centroid[0]);
		T = InsertAVL(T, leaf.PrimID, centroid, leaf.aabb);
		PrintTreeNode("", T);
		log_debug("[DBG] [BVH] ---------------------------------------------");
	}

	/*int root = -1;
	InnerNode* innerNodes = DirectBVH2BuilderGPU(centroidBox, leafItems, root);
	if (innerNodes == nullptr)
		return;*/

	/*log_debug("[DBG] [BVH] Tree built, printing");
	printTree(0, root, false, innerNodes);

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] BVH2 --> QBVH conversion");
	QTreeNode* Q = BinTreeToQTree(root, false, innerNodes, leafItems);

	delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing QTree");
	PrintTree("", Q);
	DeleteTree(Q);*/
}

void TestPLOC()
{
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] TestPLOC() START");
	// using LeafItem = std::tuple<MortonCode_t, AABB, int>;
	std::vector<LeafItem> leafItems;

	AABB aabb, sceneAABB;
	// Init the leaves
	{
		// 1.0
		aabb.min = Vector3(0.0f, 0.0f, 0.0f);
		aabb.max = Vector3(2.0f, 1.0f, 1.0f);
		leafItems.push_back({ 1, aabb.GetCentroidVector3(), aabb, 1 });
		sceneAABB.Expand(aabb);

		// 2.0
		aabb.min = Vector3(1.0f, 0.0f, 0.0f);
		aabb.max = Vector3(3.0f, 1.0f, 1.0f);
		leafItems.push_back({ 2, aabb.GetCentroidVector3(), aabb, 6 });
		sceneAABB.Expand(aabb);

		// 3.0
		aabb.min = Vector3(2.0f, 0.0f, 0.0f);
		aabb.max = Vector3(4.0f, 1.0f, 1.0f);
		leafItems.push_back({ 3, aabb.GetCentroidVector3(), aabb, 0 });
		sceneAABB.Expand(aabb);

		// 3.0
		aabb.min = Vector3(2.0f, 0.0f, 0.0f); // Repeated element!
		aabb.max = Vector3(4.0f, 1.0f, 1.0f);
		leafItems.push_back({ 3, aabb.GetCentroidVector3(), aabb, 2 });
		sceneAABB.Expand(aabb);

		// 4.0
		aabb.min = Vector3(3.0f, 0.0f, 0.0f);
		aabb.max = Vector3(5.0f, 1.0f, 1.0f);
		leafItems.push_back({ 4, aabb.GetCentroidVector3(), aabb, 5 });
		sceneAABB.Expand(aabb);

		// 5.0
		aabb.min = Vector3(4.0f, 0.0f, 0.0f);
		aabb.max = Vector3(6.0f, 1.0f, 1.0f);
		leafItems.push_back({ 5, aabb.GetCentroidVector3(), aabb, 3 });
		sceneAABB.Expand(aabb);

		// 7.0
		aabb.min = Vector3(6.0f, 0.0f, 0.0f);
		aabb.max = Vector3(8.0f, 1.0f, 1.0f);
		leafItems.push_back({ 7, aabb.GetCentroidVector3(), aabb, 4 });
		sceneAABB.Expand(aabb);
	}

	const int numPrimitives = leafItems.size();

	// Sort the morton codes
	std::sort(leafItems.begin(), leafItems.end(), leafSorter);
	/*TreeNode* T = nullptr;
	for (const LeafItem& leaf : leafItems)
	{
		T = InsertTree(T, leaf.PrimID, leaf.centroid, leaf.aabb);
	}
	std::vector<LeafItem> result;
	InOrder(T, result);
	DeleteTree(T);
	for (int i = 0; i < (int)result.size(); i++)
	{
		log_debug("[DBG] [BVH] sorted centroid: %0.3f", result[i].centroid[0]);
	}*/

	int root = -1;
	InnerNode *innerNodes = PLOC(leafItems, root);
	if (innerNodes == nullptr)
		return;

	log_debug("[DBG] [BVH] Tree built, printing");
	printTree(0, root, false, innerNodes);

	/*
	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] BVH2 --> QBVH conversion");
	QTreeNode* Q = BinTreeToQTree(root, false, innerNodes, leafItems);

	delete[] innerNodes;

	log_debug("[DBG] [BVH] ****************************************************************");
	log_debug("[DBG] [BVH] Printing QTree");
	PrintTree("", Q);
	DeleteTree(Q);*/
}