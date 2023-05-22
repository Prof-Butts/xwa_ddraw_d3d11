#pragma once

#include <map>
#include "LBVH.h"

#pragma pack(push, 1)

struct D3dGlobalLight
{
	float direction[4];
	float color[4];
};

struct D3dLocalLight
{
	float position[4];
	float color[4];
};

struct D3dConstants
{
	float viewportScale[4];
	float projectionValue1;
	float projectionValue2;
	float projectionDeltaX;
	float projectionDeltaY;
	float projectionParameterA;
	float projectionParameterB;
	float projectionParameterC;
	float projectionParameterD;
	float floorLevel;
	float cameraPositionX;
	float cameraPositionY;
	float hangarShadowAccStartEnd;
	float sx1;
	float sx2;
	float sy1;
	float sy2;
	float hangarShadowView[16];
	float transformWorldView[16];
	int globalLightsCount;
	int localLightsCount;
	int renderType;
	int renderTypeIllum;
	D3dGlobalLight globalLights[8];
	D3dLocalLight localLights[8];
};

static_assert(sizeof(D3dConstants) % 16 == 0, "D3dConstants size must be multiple of 16");

struct D3dVertex
{
	int iV;
	int iN;
	int iT;
	int c;
};

struct D3dTriangle
{
	int v1;
	int v2;
	int v3;
};

#pragma pack(pop)

struct DrawCommand {
	Direct3DTexture *colortex, *lighttex;
	ID3D11ShaderResourceView *vertexSRV, *normalsSRV, *texturesSRV, *tangentsSRV;
	ID3D11Buffer *vertexBuffer, *indexBuffer;
	int trianglesCount;
	D3dConstants constants;
	// Extra fields needed for transparent layers
	ID3D11ShaderResourceView *SRVs[2];
	PixelShaderCBuffer PSCBuffer;
	DCPixelShaderCBuffer DCPSCBuffer;
	bool bIsCockpit, bIsGunner, bIsBlastMark;
	ComPtr<ID3D11PixelShader> pixelShader;
	Matrix4 meshTransformMatrix;
};

enum class D3dRendererType
{
	STANDARD,
	EFFECTS,
	STEAMVR,
	DIRECTSBS
};

extern D3dRendererType g_D3dRendererType;

class D3dRenderer
{
public:
	int _currentOptMeshIndex;
	LBVH *_lbvh; // Global LBVH, used in the Tech Room
	bool _isRTInitialized;

	D3dRenderer();
	virtual void SceneBegin(DeviceResources* deviceResources);
	virtual void SceneEnd();
	void FlightStart();
	virtual void MainSceneHook(const SceneCompData* scene);
	virtual void HangarShadowSceneHook(const SceneCompData* scene);
	virtual void UpdateTextures(const SceneCompData* scene);
	bool ComputeTangents(const SceneCompData* scene, XwaVector3 *tangents);
	void UpdateMeshBuffers(const SceneCompData* scene);
	void ResizeDataVector(const SceneCompData* scene);
	void CreateDataScene(const SceneCompData* scene);
	void UpdateVertexAndIndexBuffers(const SceneCompData* scene);
	void UpdateConstantBuffer(const SceneCompData* scene);
	virtual void RenderScene();
	virtual void RenderDeferredDrawCalls();
	void BuildGlowMarks(SceneCompData* scene);
	void RenderGlowMarks();
	void Initialize();
	void CreateConstantBuffer();
	virtual void CreateStates();
	virtual void CreateShaders();
	void GetViewport(D3D11_VIEWPORT* viewport);
	void GetViewportScale(float* viewportScale);
	void SetRenderTypeIllum(int type);

protected:
	DeviceResources* _deviceResources;
	ID3D11Buffer *_lastVertexBuffer, *_lastIndexBuffer;

	int _totalVerticesCount;
	int _totalTrianglesCount;

	int _currentFaceIndex;

	int _verticesCount;
	std::vector<D3dVertex> _vertices;
	int _trianglesCount;
	std::vector<D3dTriangle> _triangles;

	std::vector<XwaD3dVertex> _glowMarksVertices;
	std::vector<XwaD3dTriangle> _glowMarksTriangles;

	bool _isInitialized;
	//UINT _meshBufferInitialCount;
	std::map<int, ComPtr<ID3D11ShaderResourceView>> _meshVerticesViews;
	std::map<int, ComPtr<ID3D11ShaderResourceView>> _meshNormalsViews;
	std::map<int, ComPtr<ID3D11ShaderResourceView>> _meshTangentsViews;
	std::map<int, XwaVector3*> _tangentMap;
	std::map<int, ComPtr<ID3D11ShaderResourceView>> _meshTextureCoordsViews;
	std::map<int, AABB> _AABBs;
	std::map<int, XwaVector3> _centers; // Center-of-mass for each mesh, *not* the centroids.
	std::map<int, LBVH*> _LBVHs;
	XwaVector3* _lastMeshVertices;
	ID3D11ShaderResourceView* _lastMeshVerticesView;
	XwaVector3* _lastMeshVertexNormals;
	ID3D11ShaderResourceView* _lastMeshVertexNormalsView;
	ID3D11ShaderResourceView* _lastMeshVertexTangentsView;
	XwaTextureVertex* _lastMeshTextureVertices;
	ID3D11ShaderResourceView* _lastMeshTextureVerticesView;
	std::map<int, ComPtr<ID3D11Buffer>> _vertexBuffers;
	std::map<int, ComPtr<ID3D11Buffer>> _triangleBuffers;
	std::map<int, std::pair<int, int>> _vertexCounters;
	D3dConstants _constants;
	ComPtr<ID3D11Buffer> _constantBuffer;
	ComPtr<ID3D11RasterizerState> _rasterizerState;
	ComPtr<ID3D11RasterizerState> _rasterizerStateCull;
	ComPtr<ID3D11RasterizerState> _rasterizerStateWireframe;
	ComPtr<ID3D11SamplerState> _samplerState;
	ComPtr<ID3D11BlendState> _solidBlendState;
	ComPtr<ID3D11DepthStencilState> _solidDepthState;
	ComPtr<ID3D11BlendState> _transparentBlendState;
	ComPtr<ID3D11DepthStencilState> _transparentDepthState;
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11VertexShader> _vertexShader;
	ComPtr<ID3D11PixelShader> _pixelShader;
	ComPtr<ID3D11VertexShader> _shadowVertexShader;
	ComPtr<ID3D11PixelShader> _shadowPixelShader;
	ComPtr<ID3D11PixelShader> _techRoomPixelShader;
	D3D11_VIEWPORT _viewport;
};

extern bool g_isInRenderLasers;
// The following flag cannot be used reliably to tell when the miniature is being rendered: it's
// false if we're in External Camera mode even if a craft is targeted.
extern bool g_isInRenderMiniature;
extern bool g_isInRenderHyperspaceLines;

extern std::map<int, int> g_MeshTagMap;
extern std::map<int, int> g_FGToLODMap;

void ClearCachedSRVs();