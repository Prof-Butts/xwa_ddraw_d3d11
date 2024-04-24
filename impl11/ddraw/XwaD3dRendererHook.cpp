/*
 * List of known issues:
 * - The Map was semi-fixed apparently as a side-effect of fixing the artifacts in the CMD.
 *   Turns out the map lines were being captured in the Dynamic Cockpit FG buffer, probably
 *   because we didn't detect the miniature being rendered when in map mode.
 * - The Map sometimes does not render the wireframe model.
 *
 * New ideas that might be possible now:
 *
 * - Control the position of ships hypering in or out. Make them *snap* into place
 * - Aiming Reticle with lead
 * - Global shadow mapping?
 * - Parallax mapping: we have the OPT -> Viewspace transform matrix now!
 *
 * - Universal head tracking through the tranformWorldView matrix. DONE -- thanks m0rgg.
 * - Ray-tracing? Do we have all the unculled, unclipped 3D data now?
 *		Use Intel Embree to build the BVH, one ray per pixel to compute global shadows
 *		outside the cockpit. DONE.
 * - Animate the 3D cockpit. Moving joysticks, targetting computer, etc. DONE.
 * - Per-craft damage textures. DONE.

 The opt filename is stored in char* s_XwaIOFileName = (char*)0x0080DA60;

JeremyaFr:
@blue_max Hello, Here is some infos about the backdrops.
To preview backdrops for a given mission, you can take a look to the source code
of XwaMission3DViewer: https://github.com/JeremyAnsel/XwaMission3DViewer
To see how starfield backdrops are added to skirmish mission, you can take a look to the
source code of the backdrops hook: https://github.com/JeremyAnsel/xwa_hooks/tree/master/xwa_hook_backdrops
When the game creates the backdrops, it can write debug messages. You can see them with
the deusdbg command line by enabling the Backdrops messages: https://www.xwaupgrade.com/phpBB3008/viewtopic.php?f=10&t=10516
Do you want to know how the game generate these messages?

Backdrops messages look like:
- "Created backdrop %d : %d, side %d, from world %d, %d, %d to world %d, %d, %d.\n"
- "Set backdrop scale %f, intensity %f, R%f, G%f, B%f, flags %d, imgnum %d.\n"

 */
#include "common.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <tuple>

#include "XwaD3dRendererHook.h"
#include "EffectsRenderer.h"
#include "SteamVRRenderer.h"
#include "DirectSBSRenderer.h"

#ifdef _DEBUG
#include "../Debug/XwaD3dVertexShader.h"
#include "../Debug/XwaD3dPixelShader.h"
#include "../Debug/XwaD3dShadowVertexShader.h"
#include "../Debug/XwaD3dShadowPixelShader.h"
#include "../Debug/XwaD3DTechRoomPixelShader.h"
#else
#include "../Release/XwaD3dVertexShader.h"
#include "../Release/XwaD3dPixelShader.h"
#include "../Release/XwaD3dShadowVertexShader.h"
#include "../Release/XwaD3dShadowPixelShader.h"
#include "../Release/XwaD3DTechRoomPixelShader.h"
#endif

#undef LOGGER_DUMP
#define LOGGER_DUMP 0

#if LOGGER_DUMP

static int s_currentFrameIndex;
static std::string g_currentTextureName;

std::ofstream& get_d3d_file()
{
	static std::ofstream g_d3d_file;
	static bool initialized = false;

	if (!initialized)
	{
		initialized = true;
		g_d3d_file.open("ddraw_d3d.txt");
	}

	return g_d3d_file;
}

void DumpConstants(D3dConstants& constants)
{
	std::ostringstream str;
	str << std::setprecision(30);

	str << "\tConstants" << std::endl;
	str << "\t" << constants.viewportScale[0] << "; " << constants.viewportScale[1] << "; " << constants.viewportScale[2] << "; " << constants.viewportScale[3] << std::endl;
	str << "\t" << constants.projectionValue1 << std::endl;
	str << "\t" << constants.projectionValue2 << std::endl;
	str << "\t" << constants.projectionDeltaX << std::endl;
	str << "\t" << constants.projectionDeltaY << std::endl;

	str << "\t" << constants.transformWorldView[0] << "; " << constants.transformWorldView[1] << "; " << constants.transformWorldView[2] << "; " << constants.transformWorldView[3] << std::endl;
	str << "\t" << constants.transformWorldView[4] << "; " << constants.transformWorldView[5] << "; " << constants.transformWorldView[6] << "; " << constants.transformWorldView[7] << std::endl;
	str << "\t" << constants.transformWorldView[8] << "; " << constants.transformWorldView[9] << "; " << constants.transformWorldView[10] << "; " << constants.transformWorldView[11] << std::endl;
	str << "\t" << constants.transformWorldView[12] << "; " << constants.transformWorldView[13] << "; " << constants.transformWorldView[14] << "; " << constants.transformWorldView[15] << std::endl;

	get_d3d_file() << str.str() << std::endl;
}

void DumpFrame()
{
	std::ostringstream str;

	str << "\tFRAME " << s_currentFrameIndex;
	str << std::endl;

	get_d3d_file() << str.str() << std::endl;

	s_currentFrameIndex++;
}

void DumpSurface()
{
	std::ostringstream str;

	str << "\tSURFACE " << g_currentTextureName;
	str << std::endl;

	get_d3d_file() << str.str() << std::endl;
}

void DumpVector3(XwaVector3* vertices, int count)
{
	std::ostringstream str;
	str << std::setprecision(30);

	for (int index = 0; index < count; index++)
	{
		XwaVector3 v = vertices[index];

		str << "\tVERT" << index << ":";
		str << "\t( " << v.x << "\t; " << v.y << "\t; " << v.z << " )";
		str << std::endl;
	}

	get_d3d_file() << str.str() << std::endl;
}

void DumpTextureVertices(XwaTextureVertex* vertices, int count)
{
	std::ostringstream str;
	str << std::setprecision(30);

	for (int index = 0; index < count; index++)
	{
		XwaTextureVertex v = vertices[index];

		str << "\tTEX" << index << ":";
		str << "\t( " << v.u << "\t; " << v.v << " )";
		str << std::endl;
	}

	get_d3d_file() << str.str() << std::endl;
}

void DumpD3dVertices(D3dVertex* vertices, int count)
{
	std::ostringstream str;

	for (int index = 0; index < count; index++)
	{
		D3dVertex v = vertices[index];

		str << "\tTRI" << index << ":";
		str << "\t( " << v.iV << "\t; " << v.iT << " )";
		str << std::endl;
	}

	get_d3d_file() << str.str() << std::endl;
}

#endif

#define VERBOSE_OPT_OUTPUT 0

D3dRendererType g_D3dRendererType;

bool g_isInRenderLasers = false;
bool g_isInRenderMiniature = false;
bool g_isInRenderHyperspaceLines = false;

RendererType g_rendererType = RendererType_Unknown;

bool g_bDumpOptNodes = false;
char g_curOPTLoaded[MAX_OPT_NAME];
// Used to tag which meshes have been parsed by the Opt Parser.
// This map is used just to avoid parsing the same mesh more than once.
// Parsing is needed for BVH construction and Tangent maps
std::map<int, int> g_MeshTagMap;
// Maps Face Groups to LOD.
std::map<int, int> g_FGToLODMap;

// Maps meshes to its list of Face Groups (including all LODs).
// This map is used to compute the tangents for normal mapping.
std::map<int, std::vector<FGData>> g_meshToFGMap;

std::map<int, int> g_BackdropIdToGroupId;
std::map<int, bool> g_StarfieldGroupIdImageIdMap;

int DumpTriangle(const std::string& name, FILE* file, int OBJindex, const XwaVector3& v0, const XwaVector3& v1, const XwaVector3& v2);
int32_t MakeMeshKey(const SceneCompData* scene);
void RTResetBlasIDs();
void ComputeTreeStats(IGenericTreeNode* root);

XwaVector3 cross(const XwaVector3 &v0, const XwaVector3 &v1)
{
	float x = v0.y * v1.z - v0.z * v1.y;
	float y = v0.z * v1.x - v0.x * v1.z;
	float z = v0.x * v1.y - v0.y * v1.x;

	return XwaVector3(x, y, z);
}

static XwaVector3 orthogonalize(XwaVector3 N, XwaVector3 T)
{
	// Another way to orthogonalize T would be:
	// T = normalize(T - N * dot(N, T));
	XwaVector3 B = cross(T, N);
	return cross(N, B);
}

bool LoadTangentMap(uint32_t ID, uint32_t &NumTangents, XwaVector3 **Tangents) {
	// This mesh may have a tangent map. Let's try and load it here.
	char sTanFile[256];
	sprintf_s(sTanFile, 256, "Effects\\TangentMaps\\%x.tan", ID);
	
	NumTangents = 0;
	*Tangents = nullptr;

	FILE *file = nullptr;
	fopen_s(&file, sTanFile, "rb");
	if (file == nullptr)
		return false;
	
	fread_s(&NumTangents, sizeof(uint32_t), sizeof(uint32_t), 1, file);
	//log_debug("[DBG] Tangent Id: %x, NumTangents: %u", ID, NumTangents);
	
	float *buffer = new float[NumTangents * 3];
	fread_s(buffer, NumTangents * sizeof(float) * 3, sizeof(float), NumTangents * 3, file);

	*Tangents = (XwaVector3 *)buffer;
	//delete[] buffer;

	fclose(file);

	return true;
}

D3dRenderer::D3dRenderer()
{
	_isInitialized = false;
	_isRTInitialized = false;
	//_meshBufferInitialCount = 65536; // Not actually used anywhere at all
	_lastMeshVertices = nullptr;
	_lastMeshVerticesView = nullptr;
	_lastMeshVertexNormals = nullptr;
	_lastMeshVertexNormalsView = nullptr;
	_lastMeshVertexTangentsView = nullptr;
	_lastMeshTextureVertices = nullptr;
	_lastMeshTextureVerticesView = nullptr;
	_constants = {};
	_viewport = {};
	_currentOptMeshIndex = -1;
}

void D3dRenderer::SceneBegin(DeviceResources* deviceResources)
{
	_deviceResources = deviceResources;

	_deviceResources->BeginAnnotatedEvent(L"D3dRendererScene");

	if (!_isInitialized)
	{
		Initialize();
		_isInitialized = true;
	}

	_totalVerticesCount = 0;
	_totalTrianglesCount = 0;

	_lastMeshVertices = nullptr;
	_lastMeshVerticesView = nullptr;
	_lastMeshVertexNormals = nullptr;
	_lastMeshVertexNormalsView = nullptr;
	_lastMeshVertexTangentsView = nullptr;
	_lastMeshTextureVertices = nullptr;
	_lastMeshTextureVerticesView = nullptr;

	GetViewport(&_viewport);
	GetViewportScale(_constants.viewportScale);
	// Update g_bInTechGlobe
	InTechGlobe();
	InBriefingRoom();

#if LOGGER_DUMP
	DumpFrame();
#endif
}

void D3dRenderer::SceneEnd()
{
	_deviceResources->EndAnnotatedEvent();
}

void D3dRenderer::FlightStart()
{
	_lastMeshVertices = nullptr;
	_lastMeshVertexNormals = nullptr;
	_lastMeshTextureVertices = nullptr;

	_meshVerticesViews.clear();
	_meshNormalsViews.clear();
	_meshTangentsViews.clear();
	_meshTextureCoordsViews.clear();
	_vertexBuffers.clear();
	_triangleBuffers.clear();
	_vertexCounters.clear();
	_AABBs.clear();
	_centers.clear();
	_LBVHs.clear();
	// g_meshToFGMap is cleared in DeviceResources::OnSizeChanged().
	// The reason for that is that pressing H to restart a mission will cause this class to
	// be re-created; but the D3dRendererOptNodeHook() will not get called again and that's
	// where g_meshToFGMap is populated. Hence, if we clear g_meshToFGMap here, it won't be
	// repopulated when the mission restarts this way.

#if LOGGER_DUMP
	s_currentFrameIndex = 0;
#endif
}

void D3dRenderer::MainSceneHook(const SceneCompData* scene)
{
	ID3D11DeviceContext* context = _deviceResources->_d3dDeviceContext;

	ComPtr<ID3D11Buffer> oldVSConstantBuffer;
	ComPtr<ID3D11Buffer> oldPSConstantBuffer;
	ComPtr<ID3D11ShaderResourceView> oldVSSRV[3];
	context->VSGetConstantBuffers(0, 1, oldVSConstantBuffer.GetAddressOf());
	context->PSGetConstantBuffers(0, 1, oldPSConstantBuffer.GetAddressOf());
	context->VSGetShaderResources(0, 3, oldVSSRV[0].GetAddressOf());

	context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	_deviceResources->InitRasterizerState(g_isInRenderLasers ? _rasterizerState : _rasterizerStateCull);
	_deviceResources->InitSamplerState(_samplerState.GetAddressOf(), nullptr);

	if (scene->TextureAlphaMask == 0)
	{
		_deviceResources->InitBlendState(_solidBlendState, nullptr);
		_deviceResources->InitDepthStencilState(_solidDepthState, nullptr);
	}
	else
	{
		_deviceResources->InitBlendState(_transparentBlendState, nullptr);
		_deviceResources->InitDepthStencilState(_transparentDepthState, nullptr);
	}

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_vertexShader);
	_deviceResources->InitPixelShader(_pixelShader);

	UpdateTextures(scene);
	UpdateMeshBuffers(scene);
	UpdateVertexAndIndexBuffers(scene);
	UpdateConstantBuffer(scene);
	RenderScene();

	context->VSSetConstantBuffers(0, 1, oldVSConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, oldPSConstantBuffer.GetAddressOf());
	context->VSSetShaderResources(0, 3, oldVSSRV[0].GetAddressOf());

#if LOGGER_DUMP
	DumpConstants(_constants);
	DumpSurface();
	DumpVector3(scene->MeshVertices, *(int*)((int)scene->MeshVertices - 8));
	DumpTextureVertices(scene->MeshTextureVertices, *(int*)((int)scene->MeshTextureVertices - 8));
	DumpD3dVertices(_vertices.data(), _verticesCount);
#endif
}

void D3dRenderer::HangarShadowSceneHook(const SceneCompData* scene)
{
	ID3D11DeviceContext* context = _deviceResources->_d3dDeviceContext;

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

	_deviceResources->InitBlendState(_solidBlendState, nullptr);
	_deviceResources->InitDepthStencilState(_solidDepthState, nullptr, 0U);

	_deviceResources->InitViewport(&_viewport);
	_deviceResources->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceResources->InitInputLayout(_inputLayout);
	_deviceResources->InitVertexShader(_shadowVertexShader);
	_deviceResources->InitPixelShader(_shadowPixelShader);

	UpdateTextures(scene);
	UpdateMeshBuffers(scene);
	UpdateVertexAndIndexBuffers(scene);
	UpdateConstantBuffer(scene);
	RenderScene();

	context->VSSetConstantBuffers(0, 1, oldVSConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, oldPSConstantBuffer.GetAddressOf());
	context->VSSetShaderResources(0, 3, oldVSSRV[0].GetAddressOf());
}

void D3dRenderer::UpdateTextures(const SceneCompData* scene)
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

	if (g_isInRenderLasers)
	{
		_constants.renderType = 2;
	}

	if (isProjectile)
	{
		_constants.renderType = 2;
	}

	if (scene->D3DInfo != nullptr)
	{
		surface = scene->D3DInfo->ColorMap[0];

		if (scene->D3DInfo->LightMap[0] != nullptr)
		{
			surface2 = scene->D3DInfo->LightMap[0];
			_constants.renderTypeIllum = 1;
		}
	}
	else
	{
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

#if LOGGER_DUMP
	g_currentTextureName = texture->_name;
#endif
}

// Returns true if all the tangents for the current mesh have been computed.
#ifdef DISABLED
bool D3dRenderer::ComputeTangents(const SceneCompData* scene, XwaVector3 *tangents)
{
	// Fetch all the FGs for this mesh -- including all LODs.
	const int meshKey = MakeMeshKey(scene);
	const auto& it = g_meshToFGMap.find(meshKey);
	if (it == g_meshToFGMap.end())
		return false;

	XwaVector3* vertices = scene->MeshVertices;
	XwaVector3* normals = scene->MeshVertexNormals;
	XwaTextureVertex* textureCoords = scene->MeshTextureVertices;
	XwaVector3 v0, v1, v2;
	XwaTextureVertex uv0, uv1, uv2;

	int normalsCount = *(int*)((int)normals - 8);

	// Compute a tangent for each face in each face group in each LOD
	for (const FGData fgData : it->second)
	{
		OptFaceDataNode_01_Data_Indices* faceIndices = (OptFaceDataNode_01_Data_Indices *)std::get<0>(fgData);
		const int numFaces = std::get<1>(fgData);
		//for (int faceIndex = 0; faceIndex < scene->FacesCount; faceIndex++)
		for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
		{
			//OptFaceDataNode_01_Data_Indices& faceData = scene->FaceIndices[faceIndex];
			OptFaceDataNode_01_Data_Indices& faceData = faceIndices[faceIndex];
			const int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;
			XwaVector3 T;

			{
				uv0 = textureCoords[faceData.TextureVertex[0]];
				uv1 = textureCoords[faceData.TextureVertex[1]];
				uv2 = textureCoords[faceData.TextureVertex[2]];
				v0 = vertices[faceData.Vertex[0]];
				v1 = vertices[faceData.Vertex[1]];
				v2 = vertices[faceData.Vertex[2]];

				XwaVector3 deltaPos1 = v1 - v0;
				XwaVector3 deltaPos2 = v2 - v0;
				XwaVector3 deltaUV1(uv1.u - uv0.u, uv1.v - uv0.v, 0);
				XwaVector3 deltaUV2(uv2.u - uv0.u, uv2.v - uv0.v, 0);

				// We're not going to worry about a division by zero here. If that happens, we have either
				// a collapsed triangle or a triangle with collapsed UVs. If it's a collapsed triangle, we
				// won't see it as it will render as a line. If it's collapsed UVs, then the modeller must
				// fix the UVs anyway.
				float denom = (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
				float r = 1.0f / denom;
				T = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
				T.normalize();
				//////////////////////////////////////////////////
			}

			// T has the "flat" tangent for the current face, we can now write it to the
			// tangent list.
			for (int i = 0; i < edgesCount; i++) {
				int idx = faceData.VertexNormal[i];
				// Let's assume that this mesh already has smoothed normals. In that case,
				// we can "transfer" the smooth groups to the tangent map by re-orthogonalizing
				// T with N (doing the cross product with N twice).
				if (idx < normalsCount)
				{
					tangents[idx] = orthogonalize(normals[idx], T);
				}
			}
		}
	}

	// Return true if all tangents have been tagged
	// All normals should have a tangent if we iterate over all the FGs in all LODs
	return true;
}
#endif

void D3dRenderer::UpdateMeshBuffers(const SceneCompData* scene)
{
	ID3D11Device* device = _deviceResources->_d3dDevice;
	ID3D11DeviceContext* context = _deviceResources->_d3dDeviceContext;
	auto& resources = _deviceResources;

	XwaVector3* vertices = scene->MeshVertices;
	XwaVector3* normals = scene->MeshVertexNormals;
	XwaTextureVertex* textureCoords = scene->MeshTextureVertices;

	if (vertices != _lastMeshVertices)
	{
		_lastMeshVertices = vertices;
		_currentFaceIndex = 0;

		auto it = _meshVerticesViews.find((int)vertices);

		if (it != _meshVerticesViews.end())
		{
			_lastMeshVerticesView = it->second;
		}
		else
		{
			int verticesCount = *(int*)((int)vertices - 8);

			D3D11_SUBRESOURCE_DATA initialData;
			initialData.pSysMem = vertices;
			initialData.SysMemPitch = 0;
			initialData.SysMemSlicePitch = 0;

			ComPtr<ID3D11Buffer> meshVerticesBuffer;
			device->CreateBuffer(&CD3D11_BUFFER_DESC(verticesCount * sizeof(XwaVector3), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &meshVerticesBuffer);

			ID3D11ShaderResourceView* meshVerticesView;
			device->CreateShaderResourceView(meshVerticesBuffer, &CD3D11_SHADER_RESOURCE_VIEW_DESC(meshVerticesBuffer, DXGI_FORMAT_R32G32B32_FLOAT, 0, verticesCount), &meshVerticesView);

			AABB aabb;
			aabb.SetInfinity();
			XwaVector3 center(0, 0, 0);
			for (int i = 0; i < verticesCount; i++)
			{
				aabb.Expand(vertices[i]);
				center = center + vertices[i];
			}
			center = center * (1.0f / (float)verticesCount);

			_meshVerticesViews.insert(std::make_pair((int)vertices, meshVerticesView));
			_AABBs.insert(std::make_pair((int)vertices, aabb));
			_centers.insert(std::make_pair((int)vertices, center));
			_lastMeshVerticesView = meshVerticesView;

			// Compute the RTScale for this OPT
			// According to Jeremy, only the Tech Room and the Briefings change the scale.
			/*
			if (g_bRTEnabled && _lbvh != nullptr && !_lbvh->scaleComputed) {
				double s_XwaOptScale = *(double*)0x007825F0;
				_lbvh->scale = (float )(1.0 / s_XwaOptScale);
				_lbvh->scaleComputed = true;
			}
			*/

			// Compute RTScale for this OPT
#ifdef DISABLED
			if (g_bRTEnabled && lbvh != nullptr && !lbvh->scaleComputed && _currentOptMeshIndex < lbvh->numVertexCounts) {
				if (lbvh->vertexCounts[_currentOptMeshIndex] == verticesCount) {
					log_debug("[DBG] [BVH] _currentOptMeshIndex: %d, verticesCount: %d matches lbvh->vertexCounts. Computing scale",
						_currentOptMeshIndex, verticesCount, lbvh->vertexCounts[_currentOptMeshIndex]);
					Vector3 bvhMeshMax = lbvh->meshMinMaxs[_currentOptMeshIndex].max;
					Vector3 bvhMeshMin = lbvh->meshMinMaxs[_currentOptMeshIndex].min;
					Vector3 bvhMeshRange = bvhMeshMax - bvhMeshMin;
					Vector3 aabbRange = aabb.GetRange();
					double s_XwaOptScale = *(double*)0x007825F0;

					/*
					log_debug("[DBG] bvhMesh AABB: (%0.4f, %0.4f, %0.4f)-(%0.4f, %0.4f, %0.4f)",
						bvhMeshMin.x, bvhMeshMin.y, bvhMeshMin.z,
						bvhMeshMax.x, bvhMeshMax.y, bvhMeshMax.z	);
					log_debug("[DBG] AABB: (%0.4f, %0.4f, %0.4f)-(%0.4f, %0.4f, %0.4f)",
						aabb.min.x, aabb.min.y, aabb.min.z,
						aabb.max.x, aabb.max.y, aabb.max.z);
					*/

					/*
					log_debug("[DBG] [BVH] bvhRange: %0.4f, %0.4f, %0.4f",
						bvhMeshRange.x, bvhMeshRange.y, bvhMeshRange.z);
					log_debug("[DBG] [BVH] aabbRange: %0.4f, %0.4f, %0.4f",
						aabbRange.x, aabbRange.y, aabbRange.z);
					*/

					// Use the side with the maximum length to compute the scale
					int axis = -1;
					float bvhMax = -FLT_MAX, aabbMax = -FLT_MAX;
					for (int i = 0; i < 3; i++) {
						if (bvhMeshRange[i] > bvhMax) {
							bvhMax = bvhMeshRange[i];
							aabbMax = aabbRange[i];
						}
					}
					lbvh->scale = bvhMax / aabbMax;
					lbvh->scaleComputed = true;
					log_debug("[DBG] [BVH] RTScale: %0.6f", lbvh->scale);
					log_debug("[DBG] [BVH] s_XwaOptScale: %0.6f", 1.0f / s_XwaOptScale);
				}
			}
#endif

		}
	}

	if (normals != _lastMeshVertexNormals)
	{
		_lastMeshVertexNormals = normals;

		auto it = _meshNormalsViews.find((int)normals);

		if (it != _meshNormalsViews.end())
		{
			_lastMeshVertexNormalsView = it->second;
		}
		else
		{
			int normalsCount = *(int*)((int)normals - 8);

			D3D11_SUBRESOURCE_DATA initialData;
			initialData.pSysMem = normals;
			initialData.SysMemPitch = 0;
			initialData.SysMemSlicePitch = 0;

			ComPtr<ID3D11Buffer> meshNormalsBuffer;
			device->CreateBuffer(&CD3D11_BUFFER_DESC(normalsCount * sizeof(XwaVector3), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &meshNormalsBuffer);

			ID3D11ShaderResourceView* meshNormalsView;
			device->CreateShaderResourceView(meshNormalsBuffer, &CD3D11_SHADER_RESOURCE_VIEW_DESC(meshNormalsBuffer, DXGI_FORMAT_R32G32B32_FLOAT, 0, normalsCount), &meshNormalsView);

			_meshNormalsViews.insert(std::make_pair((int)normals, meshNormalsView));
			_lastMeshVertexNormalsView = meshNormalsView;
		}
	}

	// Tangent maps are optional, so in this case, we default to nullptr
	_lastMeshVertexTangentsView = nullptr;
	// Tangent maps are no longer computed by the CPU. They are computed as needed in the GPU,
	// in the forward rendering pass (see XwaD3dPixelShader.hlsl). The main reason for this is
	// that OPT profiles apparently mess up the Mesh -> Tangent Buffer mapping, so it's no longer
	// possible to tell what was the original mesh that was associated with a texture. This sometimes
	// causes crashes, so it's better not to do this.
	// Plus, the hangar reloads the meshes every time a view changes, causing memory leaks as the tangent
	// maps were also recomputed.
#ifdef DISABLED
	// Tangent maps
	if (!(*g_playerInHangar))
	{
		// The key for the tangent map is going to be the same key we use for the normals.
		// We do this because the scene object does not have a tangent map.
		auto it_tan = _meshTangentsViews.find((int)normals);
		if (it_tan != _meshTangentsViews.end())
		{
			// The tangents SRV is ready: fetch it
			_lastMeshVertexTangentsView = it_tan->second;
		}
		else
		{
			// The tangents SRV isn't ready yet. Try computing it
			int normalsCount = *(int*)((int)normals - 8);
			XwaVector3 *tangents = tangents = new XwaVector3[normalsCount];
			if (tangents != nullptr)
			{
				// If the entry in g_meshToFGMap for this mesh hasn't been populated yet (meaning
				// that we don't know all the FGs in all the LODs for the current mesh yet), then
				// we won't be able to compute the tangents. Otherwise, the tangents will be populated
				// and we can create the SRV
				if (ComputeTangents(scene, tangents))
				{
					D3D11_SUBRESOURCE_DATA initialData;
					initialData.pSysMem = tangents;
					initialData.SysMemPitch = 0;
					initialData.SysMemSlicePitch = 0;

					ComPtr<ID3D11Buffer> meshTangentsBuffer;
					device->CreateBuffer(&CD3D11_BUFFER_DESC(normalsCount * sizeof(XwaVector3),
						D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE),
						&initialData, &meshTangentsBuffer);

					ID3D11ShaderResourceView* meshTangentsView;
					device->CreateShaderResourceView(meshTangentsBuffer,
						&CD3D11_SHADER_RESOURCE_VIEW_DESC(meshTangentsBuffer, DXGI_FORMAT_R32G32B32_FLOAT, 0, normalsCount),
						&meshTangentsView);
					_meshTangentsViews.insert(std::make_pair((int)normals, meshTangentsView));
					_lastMeshVertexTangentsView = meshTangentsView;

					delete[] tangents;
					tangents = nullptr;
				}
			}
		}
	}
#endif

	if (textureCoords != _lastMeshTextureVertices)
	{
		_lastMeshTextureVertices = textureCoords;

		auto it = _meshTextureCoordsViews.find((int)textureCoords);

		if (it != _meshTextureCoordsViews.end())
		{
			_lastMeshTextureVerticesView = it->second;
		}
		else
		{
			int textureCoordsCount = *(int*)((int)textureCoords - 8);

			D3D11_SUBRESOURCE_DATA initialData;
			initialData.pSysMem = textureCoords;
			initialData.SysMemPitch = 0;
			initialData.SysMemSlicePitch = 0;

			ComPtr<ID3D11Buffer> meshTextureCoordsBuffer;
			device->CreateBuffer(&CD3D11_BUFFER_DESC(textureCoordsCount * sizeof(XwaTextureVertex), D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE), &initialData, &meshTextureCoordsBuffer);

			ID3D11ShaderResourceView* meshTextureCoordsView;
			device->CreateShaderResourceView(meshTextureCoordsBuffer, &CD3D11_SHADER_RESOURCE_VIEW_DESC(meshTextureCoordsBuffer, DXGI_FORMAT_R32G32_FLOAT, 0, textureCoordsCount), &meshTextureCoordsView);

			_meshTextureCoordsViews.insert(std::make_pair((int)textureCoords, meshTextureCoordsView));
			_lastMeshTextureVerticesView = meshTextureCoordsView;
		}
	}

	ID3D11ShaderResourceView* vsSSRV[4] = { _lastMeshVerticesView, _lastMeshVertexNormalsView, _lastMeshTextureVerticesView, _lastMeshVertexTangentsView };
	context->VSSetShaderResources(0, 4, vsSSRV);

	// Create the buffers for the BVH -- this code path only applies when the
	// Tech Room is active.
	if (g_bInTechRoom && _lbvh != nullptr && !_isRTInitialized) {
		HRESULT hr;

		// TODO: This path needs to be unified with EffectsRenderer::ReAllocateBvhBuffers() at
		//       some point, to avoid code duplicity.
		// Create the BVH buffers
		{
			const int numNodes = _lbvh->numNodes;

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

			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = sizeof(BVHNode) * numNodes;
			desc.Usage = D3D11_USAGE_DYNAMIC; // CPU: Write, GPU: Read
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			desc.StructureByteStride = sizeof(BVHNode);

			//hr = device->CreateBuffer(&desc, &initialData, &_RTBvhNodes);
			hr = device->CreateBuffer(&desc, nullptr, &(resources->_RTBvh));
			if (FAILED(hr)) {
				log_debug("[DBG] [BVH] Failed when creating BVH buffer: 0x%x", hr);
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(srvDesc));
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = numNodes;

			hr = device->CreateShaderResourceView(resources->_RTBvh, &srvDesc, &(resources->_RTBvhSRV));
			if (FAILED(hr)) {
				log_debug("[DBG] [BVH] Failed when creating BVH SRV: 0x%x", hr);
			}
			else {
				log_debug("[DBG] [BVH] BVH buffers created");
			}

			// Initialize the BVH
			D3D11_MAPPED_SUBRESOURCE map;
			ZeroMemory(&map, sizeof(D3D11_MAPPED_SUBRESOURCE));
			hr = context->Map(resources->_RTBvh.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
			if (SUCCEEDED(hr)) {
				memcpy(map.pData, _lbvh->nodes, sizeof(BVHNode) * _lbvh->numNodes);
				context->Unmap(resources->_RTBvh.Get(), 0);
			}
			else
				log_debug("[DBG] [BVH] Failed when mapping BVH nodes: 0x%x", hr);
		}
		
		// Create the matrices buffer
		{
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

			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			const int numMatrices = 1;
			desc.ByteWidth = sizeof(Matrix4) * numMatrices;
			desc.Usage = D3D11_USAGE_DYNAMIC; // CPU: Write, GPU: Read
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			desc.StructureByteStride = sizeof(Matrix4);

			hr = device->CreateBuffer(&desc, nullptr, &(resources->_RTMatrices));
			if (FAILED(hr)) {
				log_debug("[DBG] [BVH] Failed when creating _RTMatrices: 0x%x", hr);
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(srvDesc));
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = numMatrices;

			hr = device->CreateShaderResourceView(resources->_RTMatrices, &srvDesc, &(resources->_RTMatricesSRV));
			if (FAILED(hr)) {
				log_debug("[DBG] [BVH] Failed when creating _RTMatricesSRV: 0x%x", hr);
			}
			else {
				log_debug("[DBG] [BVH] _RTMatricesSRV created");
			}
		}

		_isRTInitialized = true;
	}
}

void D3dRenderer::ResizeDataVector(const SceneCompData* scene)
{
	size_t verticesRequiredSize = scene->FacesCount * 4;
	size_t trianglesRequiredSize = scene->FacesCount * 2;

	if (_vertices.capacity() < verticesRequiredSize)
	{
		_vertices.reserve(verticesRequiredSize * 2);
	}

	if (_triangles.capacity() < trianglesRequiredSize)
	{
		_triangles.reserve(trianglesRequiredSize * 2);
	}
}

void D3dRenderer::CreateDataScene(const SceneCompData* scene)
{
	const unsigned char ShipCategory_PlayerProjectile = 6;
	const unsigned char ShipCategory_OtherProjectile = 7;
	unsigned char category = scene->pObject->ShipCategory;
	int isProjectile = (category == ShipCategory_PlayerProjectile || category == ShipCategory_OtherProjectile) ? 1 : 0;

	D3dVertex* v = _vertices.data();
	D3dTriangle* t = _triangles.data();
	int verticesIndex = 0;
	_verticesCount = 0;
	_trianglesCount = 0;

	for (int faceIndex = 0; faceIndex < scene->FacesCount; faceIndex++)
	{
		OptFaceDataNode_01_Data_Indices& faceData = scene->FaceIndices[faceIndex];
		int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;

		for (int vertexIndex = 0; vertexIndex < edgesCount; vertexIndex++)
		{
			v->iV = faceData.Vertex[vertexIndex];
			v->iN = faceData.VertexNormal[vertexIndex];
			v->iT = faceData.TextureVertex[vertexIndex];
			//v->c = isProjectile;
			v->c = 0;
			v++;
		}

		_verticesCount += edgesCount;

		// This converts quads into 2 tris if necessary
		for (int edge = 2; edge < edgesCount; edge++)
		{
			t->v1 = verticesIndex;
			t->v2 = verticesIndex + edge - 1;
			t->v3 = verticesIndex + edge;
			t++;
		}

		verticesIndex += edgesCount;
		_trianglesCount += edgesCount - 2;
	}

	_totalVerticesCount += _verticesCount;
	_totalTrianglesCount += _trianglesCount;
}

void D3dRenderer::UpdateVertexAndIndexBuffers(const SceneCompData* scene)
{
	ID3D11Device* device = _deviceResources->_d3dDevice;
	OptFaceDataNode_01_Data_Indices* faceData = scene->FaceIndices;

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	int verticesCount;
	int trianglesCount;

	auto itVertex = _vertexBuffers.find((int)faceData);
	auto itTriangle = _triangleBuffers.find((int)faceData);
	auto itCounters = _vertexCounters.find((int)faceData);

	if (itVertex != _vertexBuffers.end())
	{
		vertexBuffer = itVertex->second;
		indexBuffer = itTriangle->second;
		verticesCount = itCounters->second.first;
		trianglesCount = itCounters->second.second;
	}
	else
	{
		ResizeDataVector(scene);
		CreateDataScene(scene);

		D3D11_SUBRESOURCE_DATA initialData;
		initialData.SysMemPitch = 0;
		initialData.SysMemSlicePitch = 0;

		initialData.pSysMem = _vertices.data();
		device->CreateBuffer(&CD3D11_BUFFER_DESC(_verticesCount * sizeof(D3dVertex), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &vertexBuffer);

		initialData.pSysMem = _triangles.data();
		device->CreateBuffer(&CD3D11_BUFFER_DESC(_trianglesCount * sizeof(D3dTriangle), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE), &initialData, &indexBuffer);

		verticesCount = _verticesCount;
		trianglesCount = _trianglesCount;

		_vertexBuffers.insert(std::make_pair((int)faceData, vertexBuffer));
		_triangleBuffers.insert(std::make_pair((int)faceData, indexBuffer));
		_vertexCounters.insert(std::make_pair((int)faceData, std::make_pair(_verticesCount, _trianglesCount)));
	}

	UINT vertexBufferStride = sizeof(D3dVertex);
	UINT vertexBufferOffset = 0;
	_deviceResources->InitVertexBuffer(nullptr, nullptr, nullptr);
	_deviceResources->InitVertexBuffer(&vertexBuffer, &vertexBufferStride, &vertexBufferOffset);
	_deviceResources->InitIndexBuffer(nullptr, true);
	_deviceResources->InitIndexBuffer(indexBuffer, true);

	_lastVertexBuffer = vertexBuffer;
	_lastIndexBuffer = indexBuffer;
	_verticesCount = verticesCount;
	_trianglesCount = trianglesCount;

	unsigned int& s_SceneNumVerts = *(unsigned int*)0x009EAA00;
	unsigned int& s_SceneNumTris = *(unsigned int*)0x009EA96C;
	unsigned int& s_SceneNumTexChanges = *(unsigned int*)0x009EA984;
	unsigned int& s_SceneNumStateChanges = *(unsigned int*)0x009EA980;

	s_SceneNumVerts += verticesCount;
	s_SceneNumTris += trianglesCount;
	s_SceneNumStateChanges += 1;
}

void D3dRenderer::UpdateConstantBuffer(const SceneCompData* scene)
{
	ID3D11DeviceContext* context = _deviceResources->_d3dDeviceContext;

	_constants.projectionParameterA = g_config.ProjectionParameterA;
	_constants.projectionParameterB = g_config.ProjectionParameterB;
	_constants.projectionParameterC = g_config.ProjectionParameterC;
	_constants.projectionParameterD = 0;

	switch (g_rendererType)
	{
	case RendererType_Main:
	default:
		_constants.projectionValue1 = *(float*)0x08B94CC; // Znear
		_constants.projectionValue2 = *(float*)0x05B46B4; // Zfar
		_constants.projectionDeltaX = *(float*)0x08C1600 + *(float*)0x0686ACC;
		_constants.projectionDeltaY = *(float*)0x080ACF8 + *(float*)0x07B33C0 + *(float*)0x064D1AC;
		_constants.floorLevel = 0.0f;
		_constants.cameraPositionX = (float)(*(int*)(0x08B94E0 + *(int*)0x08C1CC8 * 0xBCF + 0xB48 + 0x00));
		_constants.cameraPositionY = (float)(*(int*)(0x08B94E0 + *(int*)0x08C1CC8 * 0xBCF + 0xB48 + 0x04));
		_constants.hangarShadowAccStartEnd = 0.0f;
		break;

	case RendererType_Shadow:
		_constants.projectionValue1 = *(float*)0x08B94CC;
		_constants.projectionValue2 = *(float*)0x05B46B4;
		_constants.projectionDeltaX = (float)(*(short*)0x08052B8 / 2) + *(float*)0x0686ACC;
		_constants.projectionDeltaY = (float)(*(short*)0x07B33BC / 2 + *(int*)0x080811C) + *(float*)0x064D1AC;
		_constants.floorLevel = *(float*)0x0686B34 - -1.5f - (float)(*(int*)(0x08B94E0 + *(int*)0x08C1CC8 * 0xBCF + 0xB48 + 0x08));
		_constants.cameraPositionX = (float)(*(int*)(0x08B94E0 + *(int*)0x08C1CC8 * 0xBCF + 0xB48 + 0x00));
		_constants.cameraPositionY = (float)(*(int*)(0x08B94E0 + *(int*)0x08C1CC8 * 0xBCF + 0xB48 + 0x04));
		_constants.hangarShadowAccStartEnd = *(short*)0x0686B38 != 0 ? 1.0f : 0.0f;
		_constants.sx1 = *(float*)0x0662814;
		_constants.sx2 = *(float*)0x064D1A4;
		_constants.sy1 = *(float*)0x06626F4;
		_constants.sy2 = *(float*)0x063D194;
		break;
	}

	// This matrix is probably used to transform the objects in the hangar so that
	// they lie "top-down". Shadows seem to be produced by flattening the objects
	// and then projecting them to the camera plane
	_constants.hangarShadowView[0] = *(float*)0x007B4BEC;
	_constants.hangarShadowView[1] = *(float*)0x007B6FF8;
	_constants.hangarShadowView[2] = *(float*)0x007B33DC;
	_constants.hangarShadowView[3] = 0.0f;
	_constants.hangarShadowView[4] = *(float*)0x007B4BE8;
	_constants.hangarShadowView[5] = *(float*)0x007B6FF0;
	_constants.hangarShadowView[6] = *(float*)0x007B33D8;
	_constants.hangarShadowView[7] = 0.0f;
	_constants.hangarShadowView[8] = *(float*)0x007B4BF4;
	_constants.hangarShadowView[9] = *(float*)0x007B33D4;
	_constants.hangarShadowView[10] = *(float*)0x007B4BE4;
	_constants.hangarShadowView[11] = 0.0f;
	_constants.hangarShadowView[12] = 0.0f;
	_constants.hangarShadowView[13] = 0.0f;
	_constants.hangarShadowView[14] = 0.0f;
	_constants.hangarShadowView[15] = 1.0f;

	_constants.transformWorldView[0] = scene->WorldViewTransform.Rotation._11;
	_constants.transformWorldView[1] = scene->WorldViewTransform.Rotation._21;
	_constants.transformWorldView[2] = scene->WorldViewTransform.Rotation._31;
	_constants.transformWorldView[3] = scene->WorldViewTransform.Position.x;
	_constants.transformWorldView[4] = scene->WorldViewTransform.Rotation._12;
	_constants.transformWorldView[5] = scene->WorldViewTransform.Rotation._22;
	_constants.transformWorldView[6] = scene->WorldViewTransform.Rotation._32;
	_constants.transformWorldView[7] = scene->WorldViewTransform.Position.y;
	_constants.transformWorldView[8] = scene->WorldViewTransform.Rotation._13;
	_constants.transformWorldView[9] = scene->WorldViewTransform.Rotation._23;
	_constants.transformWorldView[10] = scene->WorldViewTransform.Rotation._33;
	_constants.transformWorldView[11] = scene->WorldViewTransform.Position.z;
	_constants.transformWorldView[12] = 0.0f;
	_constants.transformWorldView[13] = 0.0f;
	_constants.transformWorldView[14] = 0.0f;
	_constants.transformWorldView[15] = 1.0f;

	int s_XwaGlobalLightsCount = *(int*)0x00782848;
	XwaGlobalLight* s_XwaGlobalLights = (XwaGlobalLight*)0x007D4FA0;
	int s_XwaLocalLightsCount = *(int*)0x00782844;
	XwaLocalLight* s_XwaLocalLights = (XwaLocalLight*)0x008D42C0;

	_constants.globalLightsCount = s_XwaGlobalLightsCount;

	for (int i = 0; i < 8; i++)
	{
		if (!g_bInTechRoom) {
			_constants.globalLights[i].direction[0] = s_XwaGlobalLights[i].DirectionX;
			_constants.globalLights[i].direction[1] = s_XwaGlobalLights[i].DirectionY;
			_constants.globalLights[i].direction[2] = s_XwaGlobalLights[i].DirectionZ;
			_constants.globalLights[i].direction[3] = 1.0f;
		}
		else {
			// In the Tech Room, global lights are "attached" to the ship, so that they rotate
			// along with it. I don't think that's terribly useful, so I'm inverting the
			// World transform to fix the lights instead.
			Vector4 L = Vector4(s_XwaGlobalLights[i].DirectionX, s_XwaGlobalLights[i].DirectionY, s_XwaGlobalLights[i].DirectionZ, 0.0f);
			Matrix4 W = Matrix4(_constants.transformWorldView);
			W = W.transpose(); // This is effectively an inverse of W
			L = W * L;
			// The coordinate system in the XwaD3DTechRoomPixelShader has YZ inverted, let's do it here as well:
			_constants.globalLights[i].direction[0] =  L.x;
			_constants.globalLights[i].direction[1] = -L.y;
			_constants.globalLights[i].direction[2] = -L.z;
			_constants.globalLights[i].direction[3] =  0.0f;
		}

		_constants.globalLights[i].color[0] = s_XwaGlobalLights[i].ColorR;
		_constants.globalLights[i].color[1] = s_XwaGlobalLights[i].ColorG;
		_constants.globalLights[i].color[2] = s_XwaGlobalLights[i].ColorB;
		_constants.globalLights[i].color[3] = s_XwaGlobalLights[i].Intensity;
	}

	_constants.localLightsCount = s_XwaLocalLightsCount;

	for (int i = 0; i < 8; i++)
	{
		_constants.localLights[i].position[0] = s_XwaLocalLights[i].fPositionX;
		_constants.localLights[i].position[1] = s_XwaLocalLights[i].fPositionY;
		_constants.localLights[i].position[2] = s_XwaLocalLights[i].fPositionZ;
		_constants.localLights[i].position[3] = 1.0f;

		_constants.localLights[i].color[0] = s_XwaLocalLights[i].ColorR;
		_constants.localLights[i].color[1] = s_XwaLocalLights[i].ColorG;
		_constants.localLights[i].color[2] = s_XwaLocalLights[i].ColorB;
		_constants.localLights[i].color[3] = s_XwaLocalLights[i].m18;
	}

	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_constants, 0, 0);
}

void D3dRenderer::RenderScene()
{
	if (_deviceResources->_displayWidth == 0 || _deviceResources->_displayHeight == 0)
	{
		return;
	}

	ID3D11DeviceContext* context = _deviceResources->_d3dDeviceContext;
	unsigned short scissorLeft = *(unsigned short*)0x07D5244;
	unsigned short scissorTop = *(unsigned short*)0x07CA354;
	unsigned short scissorWidth = *(unsigned short*)0x08052B8;
	unsigned short scissorHeight = *(unsigned short*)0x07B33BC;
	float scaleX = _viewport.Width / _deviceResources->_displayWidth;
	float scaleY = _viewport.Height / _deviceResources->_displayHeight;

	if (_deviceResources->IsInConcourseHd())
	{
		scissorLeft = 0;
		scissorTop = 0;
		scissorWidth = 640;
		scissorHeight = 480;
	}

	D3D11_RECT scissor{};
	scissor.left = (LONG)(_viewport.TopLeftX + scissorLeft * scaleX + 0.5f);
	scissor.top = (LONG)(_viewport.TopLeftY + scissorTop * scaleY + 0.5f);
	scissor.right = scissor.left + (LONG)(scissorWidth * scaleX + 0.5f);
	scissor.bottom = scissor.top + (LONG)(scissorHeight * scaleY + 0.5f);
	_deviceResources->InitScissorRect(&scissor);

	context->DrawIndexed(_trianglesCount * 3, 0, 0);
}

void D3dRenderer::RenderDeferredDrawCalls()
{

}

void D3dRenderer::BuildGlowMarks(SceneCompData* scene)
{
	_glowMarksVertices.clear();
	_glowMarksTriangles.clear();

	const auto XwaD3dTextureCacheUpdateOrAdd = (void(*)(XwaTextureSurface*))0x00597784;
	const auto XwaVector3Transform = (void(*)(XwaVector3*, const XwaMatrix3x3*))0x00439B30;
	const auto L004E8110 = (void(*)(SceneCompData*, XwaKnockoutData*))0x004E8110;

	const XwaObject* s_XwaObjects = *(XwaObject**)0x007B33C4;
	const XwaObject3D* s_XwaObjects3D = *(XwaObject3D**)0x00782854;
	const int s_XwaIsInConcourse = *(int*)0x005FFD9C;

	scene->GlowMarksCount = 0;

	if (s_XwaIsInConcourse == 0)
	{
		int objectIndex = scene->pObject - s_XwaObjects;
		XwaKnockoutData* knockout = s_XwaObjects3D[objectIndex].pKnockoutData;

		if (knockout != nullptr)
		{
			L004E8110(scene, knockout);
		}
	}

	for (int glowMarkIndex = 0; glowMarkIndex < scene->GlowMarksCount; glowMarkIndex++)
	{
		XwaGlowMark* glowMark = scene->GlowMarks[glowMarkIndex];

		for (int glowMarkItemIndex = 0; glowMarkItemIndex < glowMark->Count; glowMarkItemIndex++)
		{
			XwaGlowMarkItem* glowMarkItem = &glowMark->Items[glowMarkItemIndex];

			if (glowMarkItem->pTextureSurface == nullptr)
			{
				continue;
			}

			XwaD3dTextureCacheUpdateOrAdd(glowMarkItem->pTextureSurface);

			int baseFaceIndex = _currentFaceIndex;

			for (int faceIndex = 0; faceIndex < scene->FacesCount; faceIndex++)
			{
				unsigned short faceOn = glowMark->FaceOn[baseFaceIndex + faceIndex];

				if (faceOn == 0)
				{
					continue;
				}

				OptFaceDataNode_01_Data_Indices& faceData = scene->FaceIndices[faceIndex];
				int edgesCount = faceData.Edge[3] == -1 ? 3 : 4;
				int verticesIndex = _glowMarksVertices.size();

				for (int vertexIndex = 0; vertexIndex < edgesCount; vertexIndex++)
				{
					int iV = faceData.Vertex[vertexIndex];
					int iN = faceData.VertexNormal[vertexIndex];
					int iT = faceData.TextureVertex[vertexIndex];

					XwaD3dVertex v;
					v.x = scene->MeshVertices[iV].x;
					v.y = scene->MeshVertices[iV].y;
					v.z = scene->MeshVertices[iV].z;
					v.rhw = 0;
					v.color = glowMarkItem->Color;
					v.specular = 0;
					v.tu = glowMark->UVArray[iV].u * glowMarkItem->Size + 0.5f;
					v.tv = glowMark->UVArray[iV].v * glowMarkItem->Size + 0.5f;

					XwaVector3Transform((XwaVector3*)&v.x, &scene->WorldViewTransform.Rotation);
					v.x += scene->WorldViewTransform.Position.x;
					v.y += scene->WorldViewTransform.Position.y;
					v.z += scene->WorldViewTransform.Position.z;
					v.z += g_fGlowMarkZOfs;

					float st0 = _constants.projectionValue1 / v.z;
					v.x = v.x * st0 + _constants.projectionDeltaX;
					v.y = v.y * st0 + _constants.projectionDeltaY;
					v.z = (st0 * _constants.projectionValue2) / (abs(st0) * _constants.projectionValue2 + _constants.projectionValue1);
					v.rhw = st0;

					_glowMarksVertices.push_back(v);
				}

				for (int edge = 2; edge < edgesCount; edge++)
				{
					XwaD3dTriangle t;

					t.v1 = verticesIndex;
					t.v2 = verticesIndex + edge - 1;
					t.v3 = verticesIndex + edge;
					t.RenderStatesFlags = *(unsigned int*)0x06626F0;
					t.pTexture = &glowMarkItem->pTextureSurface->D3dTexture;

					_glowMarksTriangles.push_back(t);
				}
			}
		}
	}

	_currentFaceIndex += scene->FacesCount;
}

void D3dRenderer::RenderGlowMarks()
{
	if (_glowMarksTriangles.size() == 0)
	{
		return;
	}

	if (!g_bDisplayGlowMarks)
		return;

	const auto XwaD3dExecuteBufferLock = (char(*)())0x00595006;
	const auto XwaD3dExecuteBufferAddVertices = (char(*)(void*, int))0x00595095;
	const auto XwaD3dExecuteBufferProcessVertices = (char(*)())0x00595106;
	const auto XwaD3dExecuteBufferAddTriangles = (char(*)(void*, int))0x00595191;
	const auto XwaD3dExecuteBufferUnlockAndExecute = (char(*)())0x005954D6;

	XwaD3dExecuteBufferLock();
	XwaD3dExecuteBufferAddVertices(_glowMarksVertices.data(), _glowMarksVertices.size());
	XwaD3dExecuteBufferProcessVertices();
	XwaD3dExecuteBufferAddTriangles(_glowMarksTriangles.data(), _glowMarksTriangles.size());
	XwaD3dExecuteBufferUnlockAndExecute();

	_glowMarksVertices.clear();
	_glowMarksTriangles.clear();
}

// Clears g_LBVHMap and g_BLASMap
void ClearGlobalLBVHMap()
{
	// The following code is for the Tech Room/Cockpit/Coalesced BVHs:
	// Release any BLASes that may be stored in the BVH map
	for (auto& it : g_LBVHMap)
	{
		MeshData& meshData = it.second;
		LBVH* bvh = (LBVH*)GetLBVH(meshData);
		if (bvh != nullptr) {
			delete bvh;
		}
		GetLBVH(meshData) = nullptr;
	}
	g_LBVHMap.clear();

	// The following code is for regular flight:
	// Release any BLASes that may be stored in the BLAS map
	for (auto& it : g_BLASMap)
	{
		BLASData& blasData = it.second;
		LBVH* bvh = (LBVH*)BLASGetBVH(blasData);
		if (bvh != nullptr) {
			delete bvh;
		}
		BLASGetBVH(blasData) = nullptr;
	}
	g_BLASMap.clear();

	RTResetBlasIDs();
	g_BLASIdMap.clear();
	g_FGToLODMap.clear();
	g_MeshTagMap.clear();
	g_RTExcludeMeshes.clear();

#undef DEBUG_RT
#ifdef DEBUG_RT
	g_DebugMeshToNameMap.clear();
#endif
}

void D3dRenderer::Initialize()
{
	// This method is called whenever 3D is first displayed. It can either be the Tech Room
	// or regular flight. Either way, it's only called once

	if (_deviceResources->_d3dFeatureLevel < D3D_FEATURE_LEVEL_10_0)
	{
		MessageBox(nullptr, "The D3d renderer hook requires a graphic card that supports D3D_FEATURE_LEVEL_10_0.", "X-Wing Alliance DDraw", MB_ICONWARNING);
		return;
	}

	CreateConstantBuffer();
	CreateStates();
	CreateShaders();

	if (g_bRTEnabledInTechRoom || g_bRTEnabled || g_bActiveCockpitEnabled) {
		ClearGlobalLBVHMap();
		_lbvh = nullptr;
		// Reserve some space for the matrices we'll use to build the TLAS
		g_TLASMatrices.reserve(1024);

		/*
		_RTBvh = nullptr;
		_RTBvhSRV = nullptr;
		_RTMatrices = nullptr;
		_RTMatricesSRV = nullptr;
		g_TLASTree = nullptr;
		*/
	}
}

void D3dRenderer::CreateConstantBuffer()
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	// constant buffer
	device->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(D3dConstants), D3D11_BIND_CONSTANT_BUFFER), nullptr, &_constantBuffer);
}

void D3dRenderer::CreateStates()
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	D3D11_RASTERIZER_DESC rsDesc{};
	rsDesc.FillMode = g_config.WireframeFillMode ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	
	// Original settings by Jeremy: no culling.
	// Turns out that some people have reported gaps in the cockpits when culling is on. So, for now
	// I'm re-enabling no culling as the default setting. This can now be toggled in DDraw.cfg.
	if (!g_config.CullBackFaces) {
		rsDesc.CullMode = D3D11_CULL_NONE;
		rsDesc.FrontCounterClockwise = FALSE;
	}
	else {
		// New settings: let's cull back faces!
		rsDesc.CullMode = D3D11_CULL_BACK;
		rsDesc.FrontCounterClockwise = TRUE;
	}

	rsDesc.DepthBias = 0;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	rsDesc.DepthClipEnable = TRUE;
	// Disable the scissor rect in VR mode. More work is needed for a proper fix
	rsDesc.ScissorEnable = g_bEnableVR ? FALSE : TRUE;
	rsDesc.MultisampleEnable = FALSE;
	rsDesc.AntialiasedLineEnable = FALSE;
	device->CreateRasterizerState(&rsDesc, &_rasterizerState);

	rsDesc.CullMode = D3D11_CULL_FRONT;
	device->CreateRasterizerState(&rsDesc, &_rasterizerStateCull);

	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	rsDesc.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&rsDesc, &_rasterizerStateWireframe);

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = _deviceResources->_useAnisotropy ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxAnisotropy = _deviceResources->_useAnisotropy ? _deviceResources->GetMaxAnisotropy() : 1;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = FLT_MAX;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;
	device->CreateSamplerState(&samplerDesc, &_samplerState);

	D3D11_BLEND_DESC solidBlendDesc{};
	solidBlendDesc.AlphaToCoverageEnable = FALSE;
	solidBlendDesc.IndependentBlendEnable = FALSE;
	solidBlendDesc.RenderTarget[0].BlendEnable = FALSE;
	solidBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	solidBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	solidBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	solidBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	solidBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	solidBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	solidBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&solidBlendDesc, &_solidBlendState);

	D3D11_DEPTH_STENCIL_DESC solidDepthDesc{};
	solidDepthDesc.DepthEnable = TRUE;
	solidDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	solidDepthDesc.DepthFunc = D3D11_COMPARISON_GREATER;
	solidDepthDesc.StencilEnable = FALSE;
	device->CreateDepthStencilState(&solidDepthDesc, &_solidDepthState);

	D3D11_BLEND_DESC transparentBlendDesc{};
	transparentBlendDesc.AlphaToCoverageEnable = FALSE;
	transparentBlendDesc.IndependentBlendEnable = FALSE;
	transparentBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	transparentBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparentBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparentBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparentBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;

	// Old version: this causes some artifacts between holograms/transparent surfaces and the hyperspace effect:
	//transparentBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	//transparentBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	// These settings work better for blending transparent surfaces
	transparentBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
	transparentBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;

	transparentBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&transparentBlendDesc, &_transparentBlendState);

	D3D11_DEPTH_STENCIL_DESC transparentDepthDesc{};
	transparentDepthDesc.DepthEnable = TRUE;
	transparentDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	transparentDepthDesc.DepthFunc = D3D11_COMPARISON_GREATER;
	transparentDepthDesc.StencilEnable = FALSE;
	device->CreateDepthStencilState(&transparentDepthDesc, &_transparentDepthState);
}

void D3dRenderer::CreateShaders()
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	device->CreateVertexShader(g_XwaD3dVertexShader, sizeof(g_XwaD3dVertexShader), nullptr, &_vertexShader);

	const D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	device->CreateInputLayout(vertexLayoutDesc, ARRAYSIZE(vertexLayoutDesc), g_XwaD3dVertexShader, sizeof(g_XwaD3dVertexShader), &_inputLayout);

	device->CreatePixelShader(g_XwaD3dPixelShader, sizeof(g_XwaD3dPixelShader), nullptr, &_pixelShader);
	device->CreatePixelShader(g_XwaD3DTechRoomPixelShader, sizeof(g_XwaD3DTechRoomPixelShader), nullptr, &_techRoomPixelShader);

	device->CreateVertexShader(g_XwaD3dShadowVertexShader, sizeof(g_XwaD3dShadowVertexShader), nullptr, &_shadowVertexShader);
	device->CreatePixelShader(g_XwaD3dShadowPixelShader, sizeof(g_XwaD3dShadowPixelShader), nullptr, &_shadowPixelShader);
}

void D3dRenderer::GetViewport(D3D11_VIEWPORT* viewport)
{
	UINT w;
	UINT h;

	if (g_config.AspectRatioPreserved)
	{
		if (_deviceResources->_backbufferHeight * _deviceResources->_displayWidth <= _deviceResources->_backbufferWidth * _deviceResources->_displayHeight)
		{
			w = _deviceResources->_backbufferHeight * _deviceResources->_displayWidth / _deviceResources->_displayHeight;
			h = _deviceResources->_backbufferHeight;
		}
		else
		{
			w = _deviceResources->_backbufferWidth;
			h = _deviceResources->_backbufferWidth * _deviceResources->_displayHeight / _deviceResources->_displayWidth;
		}
	}
	else
	{
		w = _deviceResources->_backbufferWidth;
		h = _deviceResources->_backbufferHeight;
	}

	UINT left = (_deviceResources->_backbufferWidth - w) / 2;
	UINT top = (_deviceResources->_backbufferHeight - h) / 2;

	viewport->TopLeftX = (float)left;
	viewport->TopLeftY = (float)top;
	viewport->Width = (float)w;
	viewport->Height = (float)h;
	viewport->MinDepth = D3D11_MIN_DEPTH;
	viewport->MaxDepth = D3D11_MAX_DEPTH;
}

void D3dRenderer::GetViewportScale(float* viewportScale)
{
	float scale;

	if (_deviceResources->_frontbufferSurface == nullptr)
	{
		scale = 1.0f;
	}
	else
	{
		if (_deviceResources->_backbufferHeight * _deviceResources->_displayWidth <= _deviceResources->_backbufferWidth * _deviceResources->_displayHeight)
		{
			scale = (float)_deviceResources->_backbufferHeight / (float)_deviceResources->_displayHeight;
		}
		else
		{
			scale = (float)_deviceResources->_backbufferWidth / (float)_deviceResources->_displayWidth;
		}

		scale *= g_config.Concourse3DScale;
	}

	viewportScale[0] = 2.0f / (float)_deviceResources->_displayWidth;
	viewportScale[1] = -2.0f / (float)_deviceResources->_displayHeight;
	viewportScale[2] = scale;
	viewportScale[3] = 0.0f;
}

void D3dRenderer::SetRenderTypeIllum(int type)
{
	auto context = _deviceResources->_d3dDeviceContext;
	_constants.renderTypeIllum = type;
	context->UpdateSubresource(_constantBuffer, 0, nullptr, &_constants, 0, 0);
}

// TODO: Select the appropriate renderer depending on the current config
//#define ORIGINAL_D3D_RENDERER_SHADERS
#ifdef ORIGINAL_D3D_RENDERER_SHADERS
D3dRenderer* g_current_renderer = g_xwa_d3d_renderer;
#endif
D3dRenderer* g_current_renderer = nullptr;

void RenderDeferredDrawCalls() {
	g_current_renderer->RenderDeferredDrawCalls();
}

//************************************************************************
// Hooks
//************************************************************************

void D3dRendererInitialize()
{
	if (!g_config.D3dRendererHookEnabled)
	{
		return;
	}

	if (!g_current_renderer)
	{
		switch (g_D3dRendererType)
		{
		case D3dRendererType::EFFECTS:
			g_current_renderer = new EffectsRenderer();
			break;
		case D3dRendererType::STEAMVR:
			g_current_renderer = new SteamVRRenderer();
			break;
		case D3dRendererType::DIRECTSBS:
			g_current_renderer = new DirectSBSRenderer();
			break;
		default:
			g_current_renderer = new D3dRenderer();
			break;
		}
	}
}

void D3dRendererUninitialize()
{
	if (!g_config.D3dRendererHookEnabled)
	{
		return;
	}

	if (g_current_renderer != nullptr)
	{
		delete g_current_renderer;
		g_current_renderer = nullptr;
	}
}

void D3dRendererSceneBegin(DeviceResources* deviceResources)
{
	if (!g_config.D3dRendererHookEnabled)
	{
		return;
	}

	g_current_renderer->SceneBegin(deviceResources);
}

void D3dRendererSceneEnd()
{
	if (!g_config.D3dRendererHookEnabled)
	{
		return;
	}

	g_current_renderer->SceneEnd();
}

void D3dRendererFlightStart()
{
	if (!g_config.D3dRendererHookEnabled)
	{
		return;
	}

	g_current_renderer->FlightStart();
}

void D3dRenderLasersHook(int A4)
{
	const auto L0042BBA0 = (void(*)(int))0x0042BBA0;

	g_isInRenderLasers = true;
	L0042BBA0(A4);
	g_isInRenderLasers = false;
}

void D3dRenderMiniatureHook(int A4, int A8, int AC, int A10, int A14)
{
	const auto L00478490 = (void(*)(int, int, int, int, int))0x00478490;

	g_isInRenderMiniature = true;
	L00478490(A4, A8, AC, A10, A14);
	g_isInRenderMiniature = false;
}

void D3dRenderHyperspaceLinesHook(int A4)
{
	const auto L00480A80 = (void(*)(int))0x00480A80;

	g_isInRenderHyperspaceLines = true;
	L00480A80(A4);
	g_isInRenderHyperspaceLines = false;
}


// This is hooking 2 calls to the function that processes OptNodes with NodeType==FaceData
void D3dRendererMainHook(SceneCompData* scene)
{
	if (*(int*)0x06628E0 != 0)
	{
		const auto XwaD3dExecuteBufferLock = (char(*)())0x00595006;
		const auto XwaD3dExecuteBufferAddVertices = (char(*)(void*, int))0x00595095;
		const auto XwaD3dExecuteBufferProcessVertices = (char(*)())0x00595106;
		const auto XwaD3dExecuteBufferAddTriangles = (char(*)(void*, int))0x00595191;
		const auto XwaD3dExecuteBufferUnlockAndExecute = (char(*)())0x005954D6;

		XwaD3dExecuteBufferLock();
		XwaD3dExecuteBufferAddVertices(*(void**)0x064D1A8, *(int*)0x06628E0);
		XwaD3dExecuteBufferProcessVertices();
		XwaD3dExecuteBufferAddTriangles(*(void**)0x066283C, *(int*)0x0686ABC);
		XwaD3dExecuteBufferUnlockAndExecute();

		*(int*)0x0686ABC = 0;
		*(int*)0x06628E0 = 0;
	}

	const auto L00480370 = (void(*)(const SceneCompData* scene))0x00480370;

	if (g_isInRenderHyperspaceLines)
	{
		// The hyperspace effect is super slow for some reason. Let's disable it if we're rendering the new
		// version
		if (!g_bReshadeEnabled)
			L00480370(scene);
		return;
	}

	g_rendererType = RendererType_Main;
	g_current_renderer->MainSceneHook(scene);
	g_current_renderer->BuildGlowMarks(scene);
	g_current_renderer->RenderGlowMarks();
}

void D3dRendererShadowHook(SceneCompData* scene)
{
	g_rendererType = RendererType_Shadow;
	g_current_renderer->HangarShadowSceneHook(scene);
}

LBVH *LoadLBVH(char *s_XwaIOFileName) {
	char sBVHFileName[MAX_OPT_NAME];
	// s_XwaIOFileName includes the relative path, like "FlightModels\AWingExterior.opt"
	int len = strlen(s_XwaIOFileName);
	sprintf_s(sBVHFileName, "%s", s_XwaIOFileName);

	int i = len - 1;
	while (i > 0 && sBVHFileName[i] != '.') i--;
	i++;
	sBVHFileName[i++] = 'b';
	sBVHFileName[i++] = 'v';
	sBVHFileName[i++] = 'h';
	sBVHFileName[i++] = 0;
	//log_debug("[DBG] [BVH] Loading BVH: [%s]", sBVHFileName);
	return LBVH::LoadLBVH(sBVHFileName, true, false);
}

// This path is hit both in the Tech Room and during regular flight
void D3dRendererOptLoadHook(int handle)
{
	const auto GetSizeFromHandle = (int(*)(int))0x0050E3B0;

	// XwaIOFileName contains the filename of the loaded OPT
	// const char* XwaIOFileName = (const char*)0x0080DA60;
	char* s_XwaIOFileName = (char*)0x0080DA60;
	GetSizeFromHandle(handle);
	sprintf_s(g_curOPTLoaded, MAX_OPT_NAME, s_XwaIOFileName);
	//log_debug("[DBG] Loading [%s]", s_XwaIOFileName);
	// s_XwaIOFileName includes the relative path, like "FlightModels\AWingExterior.opt"
	// Here we can side-load additional data for this OPT, like tangent maps (?) or
	// pre-computed BVH data.
	if (g_bRTEnabledInTechRoom && InTechGlobe()) {
		if (g_current_renderer->_lbvh != nullptr)
		{
			// Re-create the LBVH
			delete g_current_renderer->_lbvh;
			g_current_renderer->_lbvh = nullptr;
		}
		// Re-create the BVH buffers:
		g_current_renderer->_isRTInitialized = false;
		ClearGlobalLBVHMap();
		g_meshToFGMap.clear(); // Called only in the Tech Room to properly compute the tangent map each time an OPT is loaded
	}

	// This hook is called every time an OPT is loaded. This can happen in the
	// Tech Room and during regular flight. Either way, the cached meshes are
	// cleared. This prevents artifacts in the Tech Room (and possibly during
	// regular flight as well).
	D3dRendererFlightStart();
}

char* OptNodeTypeToStr(int type)
{
	switch (type) {
	case OptNode_NodeGroup:
		return "NodeGroup";
	case OptNode_FaceData:
		return "FaceData";
	case OptNode_TransformPositionRotation:
		return "TransformPositionRotation";
	case OptNode_MeshVertices:
		return "MeshVertices";
	case OptNode_TransformPosition:
		return "TransformPosition";
	case OptNode_TransformRotation:
		return "TransformRotation";
	case OptNode_TransformScale:
		return "TransformScale";
	case OptNode_NodeReference:
		return "NodeReference";
	case OptNode_Unknown9:
		return "Unknown9";
	case OptNode_Unknown10:
		return "Unknown10";
	case OptNode_VertexNormals:
		return "VertexNormals";
	case OptNode_Unknown12:
		return "Unknown12";
	case OptNode_TextureVertices:
		return "TextureVertices";
	case OptNode_Unknown14:
		return "Unknown14";
	case OptNode_FaceData_0F:
		return "FaceData_0F";
	case OptNode_FaceData_10:
		return "FaceData_10";
	case OptNode_FaceData_11:
		return "FaceData_11";
	case OptNode_Unknown19:
		return "Unknown19";
	case OptNode_Texture:
		return "Texture";
	case OptNode_FaceGrouping:
		return "FaceGrouping";
	case OptNode_Hardpoint:
		return "Hardpoint";
	case OptNode_RotationScale:
		return "RotationScale";
	case OptNode_NodeSwitch:
		return "NodeSwitch";
	case OptNode_MeshDescriptor:
		return "MeshDescriptor";
	case OptNode_TextureAlpha:
		return "TextureAlpha";
	case OptNode_D3DTexture:
		return "D3DTexture";
	case OptNode_EngineGlow:
		return "EngineGlow";
	};
	return "Invalid";
}

void ParseOptNode(OptNode* node, std::string prefix)
{
	if (node == nullptr)
		return;

	log_debug("[DBG] %sname: %s, "
		"type: %d:%s, "
		"p1: %d, p2: %d, "
		"numNodes: %d",
		prefix.c_str(),
		node->Name, node->NodeType, OptNodeTypeToStr(node->NodeType),
		node->Parameter1, node->Parameter2,
		node->NumOfNodes);

	if (node->NumOfNodes > 0)
	{
		if (node->NodeType == OptNode_FaceGrouping)
		{
			int LODs = node->NumOfNodes;
			if (LODs > 1)
				log_debug("[DBG] %sLODs: %d", prefix.c_str(), LODs);
		}

		for (int i = 0; i < node->NumOfNodes; i++)
			ParseOptNode(node->Nodes[i], prefix + "   ");
	}
}

void ParseOptFaceGrouping(int meshKey, OptNode* outerNode)
{
	if (outerNode->NodeType != OptNode_FaceGrouping) {
		log_debug("[DBG] [OPT] ERROR: Tried to parse wrong type");
		return;
	}

	// The number of nodes here tells us the number of LODs
	int numLODs = outerNode->NumOfNodes;
#if VERBOSE_OPT_OUTPUT
	if (g_bDumpOptNodes) log_debug("[DBG] [OPT]        numLODs: %d", numLODs);
#endif

	for (int lodID = 0; lodID < numLODs; lodID++)
	{
		OptNode* node = outerNode->Nodes[lodID];
		if (node == nullptr) continue;

#if VERBOSE_OPT_OUTPUT
		if (g_bDumpOptNodes) log_debug("[DBG] [OPT]        Parsing LOD: %d", lodID);
#endif
		if (node->NodeType == OptNode_NodeGroup)
		{
			for (int j = 0; j < node->NumOfNodes; j++)
			{
				OptNode* subnode = node->Nodes[j];
				if (subnode == nullptr) continue;

				if (subnode->NodeType == OptNode_FaceData)
				{
					// This is not an error, the actual pointer to the scene->FaceIndices starts
					// 4 bytes after Parameter2.
					// scene->FaceIndices is the key used in g_FGToLODMap, see MakeFaceGroupKey().
					int faceGroupID = subnode->Parameter2 + 4;
					int numFaces = subnode->Parameter1;
					// g_FGToLODMap is used to coalesce vertices that belong on the same LOD when building the BVH (RT)
					g_FGToLODMap[faceGroupID] = lodID;

					auto& it = g_meshToFGMap.find(meshKey);
					if (it == g_meshToFGMap.end())
					{
						// Initialize this entry
						g_meshToFGMap[meshKey] = std::vector<FGData>({ FGData(faceGroupID, numFaces) });
					}
					else
					{
						// Add this FG to the existing entry
						it->second.push_back(FGData(faceGroupID, numFaces));
					}

#if VERBOSE_OPT_OUTPUT
					if (g_bDumpOptNodes)
					{
						log_debug("[DBG] [OPT]            Adding FaceGroup: 0x%x, LOD: %d",
							faceGroupID, lodID);
					}
#endif
				}
			}
		}
	}
}

void ParseOptNodeGroup(int meshKey, OptNode* outerNode)
{
	for (int i = 0; i < outerNode->NumOfNodes; i++) {
		OptNode* node = outerNode->Nodes[i];
		if (node == nullptr) continue;
		if (node->NodeType == OptNode_FaceGrouping)
		{
#if VERBOSE_OPT_OUTPUT
			if (g_bDumpOptNodes) log_debug("[DBG] [OPT]    FaceGrouping, numNodes: %d", node->NumOfNodes);
#endif
			ParseOptFaceGrouping(meshKey, node);
		}
	}
}

// Finds and parses the NodeGroup entry in a mesh node.
// outerNode should correspond to a mesh entry and it should be a
// direct child of the OptHeader.
void ParseOptMeshEntry(int meshKey, OptNode *outerNode)
{
	for (int i = 0; i < outerNode->NumOfNodes; i++)
	{
		OptNode* node = outerNode->Nodes[i];
		if (node == nullptr) continue;

		if (node->NodeType == OptNode_NodeGroup)
		{
			ParseOptNodeGroup(meshKey, node);
		}
	}
}

void D3dRendererOptNodeHook(OptHeader* optHeader, int nodeIndex, SceneCompData* scene)
{
	const auto L00482000 = (void(*)(OptHeader*, OptNode*, SceneCompData*))0x00482000;

	OptNode* node = optHeader->Nodes[nodeIndex];
	g_current_renderer->_currentOptMeshIndex =
		(node->NodeType == OptNode_Texture || node->NodeType == OptNode_D3DTexture) ?
		(nodeIndex - 1) : nodeIndex;

	{
		// scene->MeshVertices is always 0 at this point, so it can't be used as a key.
		// Instead, we need to find the actual MeshVertices by looking at the data in
		// this node:
		if (node->NodeType == OptNode_NodeGroup)
		{
			for (int j = 0; j < node->NumOfNodes; j++)
			{
				OptNode* subnode = node->Nodes[j];
				if (subnode == nullptr) continue;

				/*
				if (subnode->NodeType == OptNode_VertexNormals)
				{
					int numNormals = subnode->Parameter1;
				}

				if (subnode->NodeType == OptNode_TextureVertices)
				{
					int numUVCoords = subnode->Parameter1;
				}
				*/

				if (subnode->NodeType == OptNode_MeshVertices)
				{
					//int numVertices = subnode->Parameter1;
					int meshKey = subnode->Parameter2;
					auto& it = g_MeshTagMap.find(meshKey);
					if (it == g_MeshTagMap.end())
					{
						// DEBUG: Dump the node hierarchy when a hotkey is pressed
#ifdef DISABLED
						if (g_bDumpOptNodes && nodeIndex == 0)
						{
							log_debug("[DBG] [OPT] nodeIndex: %d, numNodes: %d",
								nodeIndex, optHeader->NumOfNodes);
							for (int i = 0; i < optHeader->NumOfNodes; i++)
							{
								ParseOptNode(optHeader->Nodes[i], "");
							}
						}
#endif

#if VERBOSE_OPT_OUTPUT
						if (g_bDumpOptNodes)
						{
							log_debug("[DBG] [OPT] Adding MeshVertices: 0x%x",
								(int)(subnode->Parameter2));
						}
#endif
						g_MeshTagMap[meshKey] = 1;
						ParseOptMeshEntry(meshKey, node);

						// DEBUG: Display the g_meshToFGMap to confirm it was built properly
#ifdef DISABLED
						if (g_bDumpOptNodes)
						{
							std::string msg = "";
							for (const FGData fgData : g_meshToFGMap[meshKey])
								msg += std::to_string(std::get<1>(fgData)) + ", ";

							log_debug("[DBG] mesh: 0x%x has %d FGs. numFaces: %s",
								meshKey, g_meshToFGMap[meshKey].size(), msg.c_str());
						}
#endif
					}
				}
			}
		}
	}

	L00482000(optHeader, node, scene);
}

#ifdef DISABLED
//***************************************************************
// VertexMap implementation
//***************************************************************
VertexMap::~VertexMap()
{
	_map.clear();
}

// Returns false if Vertex isn't in the map. Otherwise returns
// true and Normal is also populated
bool VertexMap::find(const Vertex3 &Vertex, Vertex3 &Normal)
{
	Vertex3 V = Vertex;
	_adjustPrecision(V);
	std::string hash = hash_value(V);
	auto it = _map.find(hash);
	if (it == _map.end())
		return false;
	Normal = _map[hash];
	return true;
}

void VertexMap::_adjustPrecision(Vertex3 &Vertex)
{
	constexpr float precision = 1000.0f;
	float X = Vertex.x * precision;
	float Y = Vertex.y * precision;
	float Z = Vertex.z * precision;
	X = (float)floor(X);
	Y = (float)floor(Y);
	Z = (float)floor(Z);
	Vertex.x = X / precision;
	Vertex.y = Y / precision;
	Vertex.z = Z / precision;
}

void VertexMap::insert(const Vertex3 &Vertex, const Vertex3 &Normal)
{
	Vertex3 V, temp;
	V = Vertex;
	_adjustPrecision(V);
	if (find(V, temp))
		return;

	_map[hash_value(V)] = Normal;
}

void VertexMap::insert_or_add(const Vertex3 &Vertex, const Vertex3 &Normal)
{
	bool alreadyInMap;
	insert_or_add(Vertex, Normal, alreadyInMap);
}

void VertexMap::insert_or_add(const Vertex3 &Vertex, const Vertex3 &Normal, bool &alreadyInMap)
{
	Vertex3 V, temp;
	V = Vertex;
	_adjustPrecision(V);
	if (find(V, temp)) {
		temp.x += Normal.x;
		temp.y += Normal.y;
		temp.z += Normal.z;
		_map[hash_value(V)] = temp;
		alreadyInMap = true;
		return;
	}

	_map[hash_value(V)] = Normal;
	alreadyInMap = false;
}

void VertexMap::set(const Vertex3 &Vertex, const Vertex3 &Normal)
{
	Vertex3 V, temp;
	V = Vertex;
	_adjustPrecision(V);
	_map[hash_value(V)] = Normal;
}
#endif
