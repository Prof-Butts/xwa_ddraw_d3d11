#pragma once

/*
 * The candidate motions of the indirect light's temporal filter (c:\temp\gi-temporal-plan.md).
 *
 * The G-buffer's positions are in XWA's camera space, and nothing says which object a pixel
 * belongs to. So each GI pixel tries a few candidate motions, from last frame's camera space to
 * this frame's, and keeps the one whose reprojected depth matches the history:
 * - "attached": the cockpit (or the player's craft in the external view), which moves with the
 *   camera; only for the pixels inside its bounding sphere, so that distant surfaces don't
 *   match it by chance while the craft turns;
 * - "world": the motion of the objects that didn't move this frame (their raw XWA pose is the
 *   same as last frame's), which is the camera's motion.
 *
 * EffectsRenderer captures, for every draw, the object -> camera transform (MeshTransform, then
 * WorldViewTransform, as the vertex shader applies them) into g_GIMotionCur; SceneBegin moves
 * it to g_GIMotionPrev. The GI pass builds the candidates from both (BuildGIMotionCandidates()).
 */

#include <unordered_map>
#include <vector>
#include <cstring>
#include <cmath>
#include "XWAObject.h"
#include "xwa_structures.h"
#include "Matrices.h"

// A 4x4 transform in the column-vector convention: p' = M * p
struct GIMat4
{
	double m[4][4];
};

inline GIMat4 GIIdentity()
{
	GIMat4 r = {};
	for (int i = 0; i < 4; i++) r.m[i][i] = 1.0;
	return r;
}

inline GIMat4 GIMul(const GIMat4& a, const GIMat4& b)
{
	GIMat4 r = {};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				r.m[i][j] += a.m[i][k] * b.m[k][j];
	return r;
}

// General 4x4 inverse (Gauss-Jordan with partial pivoting). False when singular.
inline bool GIInverse(const GIMat4& a, GIMat4& out)
{
	double t[4][8];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			t[i][j] = a.m[i][j];
			t[i][j + 4] = i == j ? 1.0 : 0.0;
		}
	for (int c = 0; c < 4; c++)
	{
		int p = c;
		for (int r = c + 1; r < 4; r++)
			if (fabs(t[r][c]) > fabs(t[p][c])) p = r;
		if (fabs(t[p][c]) < 1e-12)
			return false;
		if (p != c)
			for (int j = 0; j < 8; j++) { double x = t[c][j]; t[c][j] = t[p][j]; t[p][j] = x; }
		const double inv = 1.0 / t[c][c];
		for (int j = 0; j < 8; j++) t[c][j] *= inv;
		for (int r = 0; r < 4; r++)
		{
			if (r == c) continue;
			const double f = t[r][c];
			if (f != 0.0)
				for (int j = 0; j < 8; j++) t[r][j] -= f * t[c][j];
		}
	}
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			out.m[i][j] = t[i][j + 4];
	return true;
}

// The object -> camera transform of a draw, in XWA's OPT-scale camera space, as the vertex
// shader applies it: v' = mul(float4(v, 1), MeshTransform), then mul(float4(v', 1),
// transformWorldView). meshTransformCB is g_OPTMeshTransformCB.MeshTransform, which is stored
// transposed (the TLAS code does M.transpose(); M * v with it).
inline GIMat4 GIObjectToCamera(const XwaTransform& wv, const Matrix4& meshTransformCB)
{
	// transformWorldView: p_cam[k] = sum_j v[j] * Rotation._jk + Position[k]
	const float R[3][3] = {
		{ wv.Rotation._11, wv.Rotation._12, wv.Rotation._13 },
		{ wv.Rotation._21, wv.Rotation._22, wv.Rotation._23 },
		{ wv.Rotation._31, wv.Rotation._32, wv.Rotation._33 },
	};
	const float P[3] = { wv.Position.x, wv.Position.y, wv.Position.z };
	GIMat4 W = GIIdentity();
	for (int k = 0; k < 3; k++)
	{
		for (int j = 0; j < 3; j++)
			W.m[k][j] = R[j][k];
		W.m[k][3] = P[k];
	}
	Matrix4 Mt = meshTransformCB;
	Mt.transpose();
	const float* f = Mt.get(); // Column-major: element (row r, column c) is f[c * 4 + r]
	GIMat4 M;
	for (int r = 0; r < 4; r++)
		for (int c = 0; c < 4; c++)
			M.m[r][c] = f[c * 4 + r];
	return GIMul(W, M);
}

// OPT-scale camera space -> the G-buffer's metric space (Y+ up): x / 40.96, -y / 40.96, z / 40.96
inline GIMat4 GIOptToMetric()
{
	GIMat4 D = GIIdentity();
	const double k = 1.0 / 40.96;
	D.m[0][0] = k;
	D.m[1][1] = -k;
	D.m[2][2] = k;
	return D;
}

// The motion of an object between two frames in the G-buffer's metric space:
// P_prev = D * T_prev * T_cur^-1 * D^-1 * P_cur
inline bool GIMotionMatrix(const GIMat4& Tprev, const GIMat4& Tcur, GIMat4& out)
{
	GIMat4 TcurInv, D = GIOptToMetric(), Dinv;
	if (!GIInverse(Tcur, TcurInv) || !GIInverse(D, Dinv))
		return false;
	out = GIMul(GIMul(GIMul(D, Tprev), TcurInv), Dinv);
	return true;
}

// The raw XWA pose of an object: if it's the same in two frames, the object didn't move
struct GIObjectPose
{
	unsigned char bytes[64];
	int size;

	bool operator==(const GIObjectPose& o) const
	{
		return size == o.size && memcmp(bytes, o.bytes, size) == 0;
	}
};

inline GIObjectPose GIGetObjectPose(const XwaObject* obj)
{
	GIObjectPose pose = {};
	// PositionX/Y/Z and the headings (XwaObject_m17..m1B)
	const unsigned char* p = (const unsigned char*)&obj->PositionX;
	const int n = (int)((const unsigned char*)&obj->XwaObject_m1D - p);
	memcpy(pose.bytes, p, n);
	pose.size = n;
	// The mobile object's rotation matrix, if it has one
	if (obj->pMobileObject != 0)
	{
		const MobileObjectEntry* mo = (const MobileObjectEntry*)obj->pMobileObject;
		memcpy(pose.bytes + pose.size, &mo->transformMatrix, sizeof(RotationMatrix3D));
		pose.size += sizeof(RotationMatrix3D);
	}
	return pose;
}

struct GIObjectCapture
{
	const void* mesh;  // The first mesh drawn for this object this frame
	GIMat4 T;          // Its object -> camera transform
	GIObjectPose pose;
};

// The projection of the frame: flat mode (TransformProjection() in XwaD3dCommon.hlsl), or VR
// (projEyeMatrix[eye] * viewMatrix or fullViewMatrix)
struct GIProjection
{
	bool valid;
	float projectionValue1, projectionDeltaX, projectionDeltaY;
	float viewportScale[4];
	// The scene's viewport (D3dRenderer::_viewport): the projection's normalized coordinates map
	// to it, not to the whole buffer (XWA projects in its own display resolution, which
	// GetViewport() fits into the backbuffer, e.g. 4:3 centered when AspectRatioPreserved)
	float viewport[4]; // TopLeftX, TopLeftY, Width, Height
	Matrix4 projEye[2], viewMat, fullViewMat;
	float bFullTransform;
};

struct GIMotionFrame
{
	int presentCounter = -1;
	// The attached object: the cockpit, or the player's craft in the external view
	bool attachedValid = false;
	bool attachedIsCockpit = false;
	GIMat4 attachedT;
	bool boundsValid = false;
	double bmin[3], bmax[3]; // Its bounds in object space (OPT units), all its meshes
	// The other objects, by XWA object
	std::unordered_map<const void*, GIObjectCapture> objects;
	GIProjection proj = {};

	void Clear()
	{
		presentCounter = -1;
		attachedValid = false;
		attachedIsCockpit = false;
		boundsValid = false;
		objects.clear();
		proj.valid = false;
	}
};

inline GIMotionFrame g_GIMotionCur, g_GIMotionPrev;

// The candidates for the GI pass, in the G-buffer's metric space
struct GIMotionCandidates
{
	bool attachedValid;
	GIMat4 attached;
	double sphere[4];   // The attached object's bounding sphere this frame: center (metric camera space), radius (m)
	bool worldValid;
	GIMat4 world;
	int staticObjects;  // Objects that didn't move (and were drawn in both frames)
	int agreeing;       // How many of them agree with the world matrix chosen
};

// True when two motion matrices are the same motion: rotation parts within 1e-3, translations
// within 1 cm (far objects' transforms carry more float error in their translation)
inline bool GISameMotion(const GIMat4& a, const GIMat4& b)
{
	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 3; c++)
			if (fabs(a.m[r][c] - b.m[r][c]) > 1e-3)
				return false;
		if (fabs(a.m[r][3] - b.m[r][3]) > 0.01)
			return false;
	}
	return true;
}

inline GIMotionCandidates BuildGIMotionCandidates(const GIMotionFrame& cur, const GIMotionFrame& prev)
{
	GIMotionCandidates c = {};
	// Consecutive frames only: after a pause (menus, loading) the history doesn't apply
	if (prev.presentCounter < 0 || cur.presentCounter < 0 || prev.presentCounter != cur.presentCounter - 1)
		return c;

	// Attached: the same kind of view in both frames (cockpit or external)
	if (cur.attachedValid && prev.attachedValid && cur.attachedIsCockpit == prev.attachedIsCockpit &&
		GIMotionMatrix(prev.attachedT, cur.attachedT, c.attached))
	{
		c.attachedValid = cur.boundsValid;
		if (cur.boundsValid)
		{
			// The bounding sphere of its bounds, in this frame's metric camera space
			double center[4] = { 0.5 * (cur.bmin[0] + cur.bmax[0]), 0.5 * (cur.bmin[1] + cur.bmax[1]),
				0.5 * (cur.bmin[2] + cur.bmax[2]), 1.0 };
			double radius = 0.0;
			for (int k = 0; k < 3; k++)
				radius += 0.25 * (cur.bmax[k] - cur.bmin[k]) * (cur.bmax[k] - cur.bmin[k]);
			radius = sqrt(radius);
			const GIMat4 DT = GIMul(GIOptToMetric(), cur.attachedT);
			for (int r = 0; r < 3; r++)
			{
				c.sphere[r] = 0.0;
				for (int k = 0; k < 4; k++)
					c.sphere[r] += DT.m[r][k] * center[k];
			}
			c.sphere[3] = radius / 40.96;
		}
	}

	// World: the objects that didn't move. Their motions should all be the camera's; the one
	// most of them agree on is used (a rotating sub-mesh would give a different one).
	std::vector<GIMat4> candidates;
	for (const auto& it : cur.objects)
	{
		auto p = prev.objects.find(it.first);
		if (p == prev.objects.end() || p->second.mesh != it.second.mesh || !(p->second.pose == it.second.pose))
			continue;
		GIMat4 M;
		if (GIMotionMatrix(p->second.T, it.second.T, M))
			candidates.push_back(M);
		if (candidates.size() >= 16)
			break;
	}
	c.staticObjects = (int)candidates.size();
	int best = -1, bestCount = 0;
	for (int i = 0; i < (int)candidates.size(); i++)
	{
		int count = 0;
		for (int j = 0; j < (int)candidates.size(); j++)
			if (GISameMotion(candidates[i], candidates[j]))
				count++;
		if (count > bestCount) { bestCount = count; best = i; }
	}
	if (best >= 0)
	{
		c.worldValid = true;
		c.world = candidates[best];
		c.agreeing = bestCount;
	}
	return c;
}
