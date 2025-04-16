#pragma once

#include "xwa_structures.h"
#include <map>

class DeviceResources;

#pragma pack(push, 1)

#pragma pack(pop)

enum RendererType
{
	RendererType_Unknown,
	RendererType_Main,
	RendererType_Shadow,
};

extern RendererType g_rendererType;

// Normal Mapping mesh-to-FaceGroup data:
// FGData = (FGKey, numFaces)
using FGData = std::tuple<int, int>;
// Maps meshes to its list of Face Groups (including all LODs).
// This map is used to compute the tangents for normal mapping.
extern std::map<int, std::vector<FGData>> g_meshToFGMap;

void D3dRendererInitialize();
void D3dRendererUninitialize();
void D3dRendererSceneBegin(DeviceResources* deviceResources);
void D3dRendererSceneEnd();
void D3dRendererFlightStart();
void D3dRenderLasersHook(int A4);
void D3dRenderMiniatureHook(int A4, int A8, int AC, int A10, int A14);
void D3dRenderHyperspaceLinesHook(int A4);
void D3dRendererMainHook(SceneCompData* scene);
void D3dRendererShadowHook(SceneCompData* scene);
void D3dRendererOptLoadHook(int handle);
void D3dRendererOptNodeHook(OptHeader* optHeader, int nodeIndex, SceneCompData* scene);

void D3dReleaseD3DINFO(XwaD3DInfo* d3dInfo);
XwaD3DInfo* D3dOptCreateD3DfromTexture(OptNode* A4, int A8, XwaTextureDescription* AC, unsigned char* A10, unsigned char* A14, unsigned short* A18, unsigned short* A1C);
