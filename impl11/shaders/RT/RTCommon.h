#ifndef RT_COMMON_H
#define RT_COMMON_H
// Copied and adapted from GPU Pro 3, the code is in the public domain, so that's
// OK. I should know, since I'm one of the co-authors!

#define MAX_RT_STACK 32
#define RT_MAX_DIST 5000.0f

// TEMPORARY diagnostic (Ctrl+Alt+T, the heat map, RTShadowMaskDiagHeatPS): the nodes fetched
// by the wide traversals below (wide nodes, TLAS leaves and triangles) and the triangles tested.
// Nothing is counted in the other shaders.
#ifdef RT_COUNT_VISITS
static uint g_RTNodeVisits = 0;
static uint g_RTTriangleTests = 0;
#define RT_COUNT_NODE g_RTNodeVisits++
#define RT_COUNT_TRIANGLE g_RTTriangleTests++
#else
#define RT_COUNT_NODE
#define RT_COUNT_TRIANGLE
#endif

// RTConstantsBuffer
cbuffer ConstantBuffer : register(b10) {
	bool bRTEnabled;
	bool bRTAllowShadowMapping;
	bool RTUnused0;
	bool RTGetBestIntersection;
	// 16 bytes
	bool  RTEnableSoftShadows;
	uint  RTShadowMaskSizeFactor;
	float RTShadowMaskPixelSizeX;
	float RTShadowMaskPixelSizeY;
	// 32 bytes
	float RTSoftShadowThresholdMult;
	float RTGaussFactor;
	float RTUnused2;
	float RTUnused3;
	// 48 bytes
};

struct Ray				// 28 bytes
{
	float3 origin;		// 12 bytes
	float3 dir;			// 12 bytes
	float max_dist;		//  4 bytes
};

struct Intersection		// 16 bytes
{
	int TriID;			//  4 bytes
	float U, V, T;		// 12 bytes
};

// BVH2 node, deprecated, BVH4 is more better
#ifdef DISABLED
struct BVHNode {
	int ref;   // -1 for internal nodes, Triangle index for leaves
	int left;
	int right;
	int padding;
	// 16 bytes

	// AABB
	float4 min;
	// 32 bytes
	float4 max;
	// 48 bytes
};
#endif

// BVH4 node
struct BVHNode {
	int ref; // TriID: -1 for internal nodes, Triangle index for leaves
	int parent; // Not used at this point
	int rootIdx;
	int numChildren;
	// 16 bytes
	float4 min;
	// 32 bytes
	float4 max;
	// 48 bytes
	int4 children;
	// 64 bytes
};

// TLAS leaves use the following fields for a different purpose:
#define BLASMatrixSlot rootIdx
#define BLASBaseNodeOffset numChildren

// BVH, slot 14
StructuredBuffer<BVHNode> g_BVH : register(t14);
// Matrices, slot 15
StructuredBuffer<matrix> g_Matrices : register(t15);
// TLAS BVH, slot 16
StructuredBuffer<BVHNode> g_TLAS : register(t16);

// In flight, g_BVH and g_TLAS hold the trees in the "wide" layout (EncodeWideBVH() in
// EffectsRenderer.cpp): every inner node holds the boxes of its 4 children, one float4 per
// coordinate (one component per child), so popping a node fetches the 4 boxes at once and
// tests them together. In the 64-byte layout, testing a child's box took a fetch of its own.
// A wide node takes two consecutive entries of the buffer; the leaves follow the wide nodes,
// unchanged (BLAS: the triangles; TLAS: the OBB, matrix slot and BLAS offset). Each tree
// begins with its root's wide node: at its BLASBaseNodeOffset in g_BVH, at 0 in g_TLAS.
// The Tech Room keeps the 64-byte layout (TraceRaySimpleHit()).
struct BVHWideNode {
	float4 minX, minY, minZ;
	float4 maxX, maxY, maxZ;
	// Relative to the start of the tree: >= 0, an inner child (the entry where its wide node
	// begins); <= -2, a leaf at entry -(c + 2); -1, no child
	int4 children;
};

// Reads the wide node that begins at entry i (two BVHNodes)
BVHWideNode LoadWideNode(in StructuredBuffer<BVHNode> BVH, in int i)
{
	const BVHNode a = BVH[i];
	const BVHNode b = BVH[i + 1];
	BVHWideNode node;
	node.minX     = asfloat(int4(a.ref, a.parent, a.rootIdx, a.numChildren));
	node.minY     = a.min;
	node.minZ     = a.max;
	node.maxX     = asfloat(a.children);
	node.maxY     = asfloat(int4(b.ref, b.parent, b.rootIdx, b.numChildren));
	node.maxZ     = b.min;
	node.children = asint(b.max);
	return node;
}
// * During regular flight, we have multiple BLASes in a single buffer. Thus, multiple roots may exist in a single
//   buffer and each BVH will have its own starting offset to which all other internal offsets are relative to.

// Vertices, slot 15
//Buffer<float3> g_Vertices : register(t15);
// Indices, slot 16
//Buffer<int> g_Indices : register(t16);

// ------------------------------------------
// Checks if the ray hits the node's AABB
// ------------------------------------------
// Returns: T[0] the min distance to the box
//          T[1] the max distance to the box
// if T[1] >= T[0], then there's an intersection.
// See also: https://tavianator.com/2011/ray_box.html
float2 _BVHIntersectBox(in float3 box_min, in float3 box_max, in float3 start, in float3 inv_dir)
{
	const float3 diff_max = (box_max.xyz - start) * inv_dir;
	const float3 diff_min = (box_min.xyz - start) * inv_dir;
	float2 T;

	T[0] = min(diff_min.x, diff_max.x);
	T[1] = max(diff_min.x, diff_max.x);

	T[0] = max(T[0], min(diff_min.y, diff_max.y));
	T[1] = min(T[1], max(diff_min.y, diff_max.y));

	T[0] = max(T[0], min(diff_min.z, diff_max.z));
	T[1] = min(T[1], max(diff_min.z, diff_max.z));

	return T;
}

float2 BVHIntersectBox(in StructuredBuffer<BVHNode> BVH, in float3 start, in float3 inv_dir, in int node)
{
	const float3 box_min = BVH[node].min.xyz;
	const float3 box_max = BVH[node].max.xyz;
	return _BVHIntersectBox(box_min, box_max, start, inv_dir);
}

#ifdef DISABLED
bool InsideBox(in float3 min, in float3 max, in float3 P)
{
	if (min.x <= P.x && P.x <= max.x &&
		min.y <= P.y && P.y <= max.y &&
		min.z <= P.z && P.z <= max.z)
	{
		return true;
	}
	return false;
}

bool InsideBox(StructuredBuffer<BVHNode> BVH, int curnode, in float3 P)
{
	return InsideBox(BVH[curnode].min.xyz, BVH[curnode].max.xyz, P);
}
#endif

// ------------------------------------------
// Gets the current ray intersection
// ------------------------------------------
Intersection getIntersection(Ray ray, float3 A, float3 B, float3 C)
{
	Intersection inters;
	inters.TriID = -1;

	float3 P, T, Q;
	float3 E1 = B - A;
	float3 E2 = C - A;
	P = cross(ray.dir, E2);
	float det = 1.0f / dot(E1, P);
	T = ray.origin - A;
	inters.U = dot(T, P) * det;
	Q = cross(T, E1);
	inters.V = dot(ray.dir, Q) * det;
	inters.T = dot(E2, Q) * det;

	return inters;
}

// ------------------------------------------
// Ray-Triangle intersection test
// ------------------------------------------
bool RayTriangleTest(in Intersection inters)
{
	return (
		(inters.U >= 0.0f) &&
		(inters.V >= 0.0f) &&
		((inters.U + inters.V) <= 1.0f) &&
		(inters.T >= 0.0f)
	);
}

// Iterate over all the primitives until we find a hit. Only useful for debugging
// purposes
/*
Intersection _NaiveSimpleHit(Ray ray) {
	Intersection best_inters;
	best_inters.TriID = -1;
	for (int TriID = 0; TriID < g_NumTriangles; TriID++) {
		int ofs = TriID * 3;
		float3 A = g_Vertices[g_Indices[ofs]];
		float3 B = g_Vertices[g_Indices[ofs + 1]];
		float3 C = g_Vertices[g_Indices[ofs + 2]];

		Intersection inters = getIntersection(ray, A, B, C);
		if (RayTriangleTest(inters)) {
			inters.TriID = TriID;
			best_inters = inters;
			break;
		}
	}
	return best_inters;
}
*/

// Ray traversal, Indexed Geometry version
#ifdef DISABLED
// ray is in OPT coords. We can traverse the BVH now
Intersection _TraceRaySimpleHit(Ray ray) {
	int stack[MAX_RT_STACK];
	int stack_top = 0;
	int curnode = -1;
	float3 inv_dir = 1.0f / ray.dir;
	Intersection best_inters;
	best_inters.TriID = -1;

	stack[stack_top++] = 0; // Push the root on the stack
	while (stack_top > 0) {
		// Pop a node from the stack
		curnode = stack[--stack_top];
		const BVHNode node = g_BVH[curnode];
		const int TriID = node.ref;

		if (TriID == -1) {
			// This node is a box. Do the ray-box intersection test
			const float2 T = BVHIntersectBox(ray.origin, inv_dir, curnode);

			// T[1] >= T[0] is the standard test, but lines are infinite, so it will also intersect
			// boxes behind the ray. To skip those boxes, we need to add T[1] >= 0, to make sure
			// the box is in front of the ray (for rays originating inside a box, we'll have T[0] < 0,
			// so we can't use that test).
			if (T[1] >= 0 && T[1] >= T[0])
			{
				// Ray intersects the box
				// Inner node: push the children of this node on the stack
				if (TriID == -1 && stack_top + 4 < MAX_RT_STACK) {
					// Push the valid children of this node into the stack
					for (int i = 0; i < 4; i++) {
						int child = node.children[i];
						if (child == -1) break;
						stack[stack_top++] = child;
					}
				}
			}
		}
		else {
			// This node is a triangle. Do the ray-triangle intersection test.
			const float3 A = node.min.xyz;
			const float3 B = node.max.xyz;
			const float3 C = float3(
				asfloat(node.children[0]),
				asfloat(node.children[1]),
				asfloat(node.children[2]));

			Intersection inters = getIntersection(ray, A, B, C);
			if (RayTriangleTest(inters)) {
				inters.TriID = TriID;
				best_inters = inters;
				return best_inters;
			}
		}
	}
#endif

// Ray traversal, Embedded Geometry version, in the 64-byte layout: only the Tech Room uses it
// (TraceRaySimpleHit(), with its single BLAS). In flight, see _TraceRaySimpleHitWide().
Intersection _TraceRaySimpleHit(Ray ray, in int Offset) {
	int stack[MAX_RT_STACK];
	int stack_top = 0;
	int curnode = -1;
	float3 inv_dir = 1.0f / ray.dir;
	Intersection best_inters;
	best_inters.TriID = -1;
	best_inters.T = RT_MAX_DIST;

	// Read the padding from the first BVHNode. It will contain the location of the root
	//int root = g_BVH[0].rootIdx;
	int root = g_BVH[Offset].rootIdx + Offset;

	stack[stack_top++] = root; // Push the root on the stack
	while (stack_top > 0) {
		// Pop a node from the stack
		curnode = stack[--stack_top];
		const BVHNode node = g_BVH[curnode];
		const int TriID = node.ref;

		if (TriID == -1) {
			// This node is a box. Do the ray-box intersection test
			const float2 T = BVHIntersectBox(g_BVH, ray.origin, inv_dir, curnode);

			// T[1] >= T[0] is the standard test, but lines are infinite, so it will also intersect
			// boxes behind the ray. To skip those boxes, we need to add T[1] >= 0, to make sure
			// the box is in front of the ray (for rays originating inside a box, we'll have T[0] < 0,
			// so we can't use that test).
			if (T[1] >= 0 && T[1] >= T[0])
			{
				// Ray intersects the box
				// Inner node: push the children of this node on the stack
				if (stack_top + 4 < MAX_RT_STACK) {
					// Push the valid children of this node into the stack
					for (int i = 0; i < 4; i++) {
						int child = node.children[i];
						if (child == -1) break;
						stack[stack_top++] = child + Offset;
					}
				}
			}
		}
		else {
			// This node is a triangle. Do the ray-triangle intersection test.
			const float3 A = node.min.xyz;
			const float3 B = node.max.xyz;
			const float3 C = float3(
				asfloat(node.children[0]),
				asfloat(node.children[1]),
				asfloat(node.children[2]));

			Intersection inters = getIntersection(ray, A, B, C);
			if (inters.T > 0 && // Don't bother testing triangles if the intersection is behind the ray's origin
				RayTriangleTest(inters))
			{
				inters.TriID = TriID;
				best_inters = inters;
				return best_inters;
			}
		}
	}

	return best_inters;
}

// Tech Room ray casting.
// Trace a ray and return as soon as we hit geometry (Tech Room version)
// This version expects a single matrix in g_Matrices that applies to the
// whole g_BVH tree
Intersection TraceRaySimpleHit(Ray ray) {
	float3 pos3D = ray.origin;
	// ray.origin is in the pos3D frame, we need to invert it into OPT coords
	pos3D.y = -pos3D.y;
	pos3D *= 40.96f;
	pos3D = mul(float4(pos3D, 1.0f), g_Matrices[0]).xyz;

	float3 dir = ray.dir;
	// ray.dir is in the pos3D frame, we need to transform it into OPT coords
	// Here the transform rule is slightly different because I hate myself and
	// I altered how lights work in the Tech Room. XWA fixes the lights to the
	// object, but I wanted them fixed in space so that we could really see the
	// shading. See D3dRenderer::UpdateConstantBuffer for more details
	dir.yz = -dir.yz;
	dir = mul(float4(dir, 0), g_Matrices[0]).xyz;

	// pos3D and dir are  now in OPT coords. We can cast the ray
	ray.origin    = pos3D;
	ray.dir       = dir;
	ray.max_dist *= 40.96f;;
	return _TraceRaySimpleHit(ray, 0);
	//return _NaiveSimpleHit(ray);
}

// The ray against the 4 child boxes of a wide node: true for the children it hits
bool4 WideBoxHit(in BVHWideNode node, in float3 start, in float3 inv_dir)
{
	const float4 tx0 = (node.minX - start.x) * inv_dir.x;
	const float4 tx1 = (node.maxX - start.x) * inv_dir.x;
	const float4 ty0 = (node.minY - start.y) * inv_dir.y;
	const float4 ty1 = (node.maxY - start.y) * inv_dir.y;
	const float4 tz0 = (node.minZ - start.z) * inv_dir.z;
	const float4 tz1 = (node.maxZ - start.z) * inv_dir.z;
	const float4 tmin = max(max(min(tx0, tx1), min(ty0, ty1)), min(tz0, tz1));
	const float4 tmax = min(min(max(tx0, tx1), max(ty0, ty1)), max(tz0, tz1));
	// As in _BVHIntersectBox()'s callers: T[1] >= 0 skips the boxes behind the ray
	return tmax >= 0 && tmax >= tmin && node.children != -1;
}

// BLAS traversal, wide layout: true when the ray hits a triangle. Offset: where the BLAS
// begins in g_BVH (its root's wide node). Only the children whose box the ray hits are
// pushed; the triangles too, and they're tested when they're popped, so the triangle test is
// inlined once. Note: fxc dropped a whole traversal once when its child loop had [unroll]
// with a break and an early return inside; check the asm (loops, dcl_indexableTemp) after
// changing these loops.
bool _TraceRaySimpleHitWide(Ray ray, in int Offset)
{
	int stack[MAX_RT_STACK];
	int stack_top = 0;
	const float3 inv_dir = 1.0f / ray.dir;
	stack[stack_top++] = 0; // The root
	while (stack_top > 0)
	{
		const int cur = stack[--stack_top];
		RT_COUNT_NODE;
		if (cur >= 0)
		{
			// An inner node: push the children that the ray hits
			const BVHWideNode node = LoadWideNode(g_BVH, Offset + cur);
			const bool4 hit = WideBoxHit(node, ray.origin, inv_dir);
			[unroll]
			for (int i = 0; i < 4; i++)
			{
				if (hit[i] && stack_top < MAX_RT_STACK)
					stack[stack_top++] = node.children[i];
			}
		}
		else
		{
			// A triangle whose box the ray hits: its vertices are min, max and children[0..2]
			RT_COUNT_TRIANGLE;
			const BVHNode tri = g_BVH[Offset - (cur + 2)];
			const float3 C = float3(asfloat(tri.children[0]), asfloat(tri.children[1]), asfloat(tri.children[2]));
			const Intersection inters = getIntersection(ray, tri.min.xyz, tri.max.xyz, C);
			if (inters.T > 0 && RayTriangleTest(inters))
				return true;
		}
	}
	return false;
}

// TLAS traversal, wide layout. The leaves whose AABB the ray hits are pushed too, and a popped
// leaf tests the ray against its OBB (in the BLAS's space), then traces its BLAS: a single call
// site, because each inlined copy of the BLAS traversal would get its own stack. Only TriID
// matters to the callers (-1: no hit).
Intersection _TLASTraceRaySimpleHit(Ray ray) {
	Intersection result;
	result.TriID = -1;
	result.U = 0;
	result.V = 0;
	result.T = RT_MAX_DIST;
	int stack[MAX_RT_STACK];
	int stack_top = 0;
	const float3 inv_dir = 1.0f / ray.dir;
	stack[stack_top++] = 0; // The root
	while (stack_top > 0)
	{
		const int cur = stack[--stack_top];
		RT_COUNT_NODE;
		if (cur >= 0)
		{
			// An inner node: push the children that the ray hits
			const BVHWideNode node = LoadWideNode(g_TLAS, cur);
			const bool4 hit = WideBoxHit(node, ray.origin, inv_dir);
			[unroll]
			for (int i = 0; i < 4; i++)
			{
				if (hit[i] && stack_top < MAX_RT_STACK)
					stack[stack_top++] = node.children[i];
			}
			continue;
		}

		// A leaf whose AABB the ray hits
		const BVHNode leaf = g_TLAS[-(cur + 2)];
		const int BLASOffset = leaf.BLASBaseNodeOffset;
		const int matrixSlot = leaf.BLASMatrixSlot;
		if (BLASOffset == -1 || matrixSlot == -1)
			continue;
		const matrix Matrix = g_Matrices[matrixSlot];
		const float3 obb_min = float3(leaf.min[3], leaf.max[3], asfloat(leaf.children[3]));
		const float3 obb_max = float3(asfloat(leaf.children[0]), asfloat(leaf.children[1]), asfloat(leaf.children[2]));
		// Transform the ray into the same coord sys as the OBB
		Ray new_ray;
		new_ray.origin   = mul(Matrix, float4(ray.origin, 1)).xyz;
		new_ray.dir      = mul(Matrix, float4(ray.dir, 0)).xyz;
		new_ray.max_dist = ray.max_dist;
		// Before traversing the BLAS, check if the ray intersects the OBB
		const float2 T2 = _BVHIntersectBox(obb_min, obb_max, new_ray.origin, 1.0f / new_ray.dir);
		if (!(T2[1] >= 0 && T2[1] >= T2[0]))
			continue;
		if (_TraceRaySimpleHitWide(new_ray, BLASOffset))
		{
			result.TriID = 0; // Any hit
			return result;
		}
	}
	return result;
}

// Trace a ray and return as soon as we hit geometry.
// The Intersection's ID isn't actually used to index anything -- it might as well be a bool.
Intersection TLASTraceRaySimpleHit(Ray ray) {
	float3 pos3D = ray.origin;
	// ray.origin is in the pos3D frame (Metric, Y+ is up, Z+ is forward)
	// we need to revert it into OPT coords, but the TLAS and the ray are
	// in the same coord sys. There's no need to transform anything
	pos3D.y = -pos3D.y;
	pos3D  *= 40.96f;

	// pos3D and dir are now in WorldView coords. We can cast the ray
	ray.origin    = pos3D;
	// Ray needs to have its direction inverted. Also Y is flipped for the same reason
	// pos3D.y is flipped (Y+ is up)
	ray.max_dist *= 40.96f;
	return _TLASTraceRaySimpleHit(ray);
}

#endif