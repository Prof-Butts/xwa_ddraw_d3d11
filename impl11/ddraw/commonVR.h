#pragma once

#include <Windows.h>
#include <stdio.h>
#include <openvr.h>
#include "common.h"
#include "utils.h"
#include "config.h"
#include "SteamVR.h"
#include "DirectSBS.h"
#include "EffectsCommon.h"
#include "FreePIE.h"
#include "Vectors.h"
#include "Matrices.h"
#include "xwa_structures.h"

const float MAX_BRIGHTNESS = 1.0f;
extern const float RAD_TO_DEG;
extern const float DEG_TO_RAD;
extern const float DEFAULT_IPD; // Ignored in SteamVR mode.
extern const float IPD_SCALE_FACTOR;
extern Matrix4 g_EyeMatrixLeftInv, g_EyeMatrixRightInv;
extern Matrix4 g_projLeft, g_projRight;
extern Matrix4 g_FullProjMatrixLeft, g_FullProjMatrixRight, g_viewMatrix;
extern Vector3 g_headCenter; // The head's center: this value should be re-calibrated whenever we set the headset
extern bool g_bResetHeadCenter; // Reset the head center on startup
extern float g_fVR_FOV;
extern float g_fIPD;
extern float g_fHalfIPD;
extern bool g_bInTechRoom; // Set to true in PrimarySurface Present 2D (Flip)
extern bool g_bInBriefingRoom;
extern bool g_bInSkirmishShipScreen;

extern float g_fPitchMultiplier, g_fYawMultiplier, g_fRollMultiplier;
extern float g_fYawOffset, g_fPitchOffset;
extern float g_fPosXMultiplier, g_fPosYMultiplier, g_fPosZMultiplier;
extern float g_fMinPositionX, g_fMaxPositionX;
extern float g_fMinPositionY, g_fMaxPositionY;
extern float g_fMinPositionZ, g_fMaxPositionZ;
extern float g_fFrameTimeRemaining;
extern float g_fSteamVRMirrorWindow3DScale, g_fSteamVRMirrorWindowAspectRatio;
extern Vector3 g_headCenter;
extern bool g_bSteamVRPosFromFreePIE, g_bReshadeEnabled, g_bSteamVRDistortionEnabled, g_bSteamVRYawPitchRollFromMouseLook;
extern vr::IVRSystem* g_pHMD;
extern int g_iFreePIESlot, g_iSteamVR_Remaining_ms, g_iSteamVR_VSync_ms;
extern bool g_bDirectSBSInitialized, g_bSteamVRInitialized;
extern bool g_bRendering3D; // Used to distinguish between 2D (Concourse/Menus) and 3D rendering (main in-flight game)

void EvaluateIPD(float NewIPD);
void IncreaseIPD(float Delta);

typedef enum {
	TRACKER_NONE,
	TRACKER_FREEPIE,
	TRACKER_STEAMVR,
	TRACKER_TRACKIR
} TrackerType;

namespace VRGlovesProfile
{
	enum VRGlovesProfile
	{
		NEUTRAL = 0,
		POINT,
		GRASP,
		MAX // Sentinel, do not remove
	};
};

struct VRGlovesMesh
{
	// Gloved hands:
	ComPtr<ID3D11Buffer> vertexBuffer;
	ComPtr<ID3D11Buffer> indexBuffer;
	//ComPtr<ID3D11Buffer> meshVerticesBuffer;
	ComPtr<ID3D11Buffer> meshVerticesBuffers[VRGlovesProfile::MAX];
	ComPtr<ID3D11Buffer> meshNormalsBuffer;
	ComPtr<ID3D11Buffer> meshTexCoordsBuffer;
	//ComPtr<ID3D11ShaderResourceView> meshVerticesSRV;
	ComPtr<ID3D11ShaderResourceView> meshVerticesSRVs[VRGlovesProfile::MAX];
	ComPtr<ID3D11ShaderResourceView> meshNormalsSRV;
	ComPtr<ID3D11ShaderResourceView> meshTexCoordsSRV;
	ComPtr<ID3D11ShaderResourceView> textureSRV;
	int numTriangles;
	Matrix4 pose;
	bool visible;
	bool rendered;
	float forwardPmeters[VRGlovesProfile::MAX]; // The forward-most point in this mesh, in meters. Used to push buttons
	char texName[128];
	int texGroupId, texImageId;
	void* bvh;
	std::vector<XwaTextureVertex> texCoords;
	std::vector<int> texIndices;
};

extern VRGlovesMesh g_vrGlovesMeshes[2];
