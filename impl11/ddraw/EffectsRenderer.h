#pragma once

#include <vector>
#include <ScreenGrab.h>
#include <wincodec.h>

#include "globals.h"
#include "common.h"
#include "commonVR.h"

#include "XwaD3dRendererHook.h"
#include "D3dRenderer.h"
#include "DeviceResources.h"
#include "Direct3DTexture.h"
#include "Matrices.h"
#include "joystick.h"

// ****************************************************
// Embree
// ****************************************************
#include <rtcore.h> //Embree

/*
 * How to compile 32-bit Embree:
 * https://github.com/morallo/xwa_ddraw_d3d11/wiki/Compile-Embree-for-32bit
 * https://github.com/morallo/xwa_ddraw_d3d11/wiki/Ray-Tracing
 * https://www.embree.org/api.html
 * https://github.com/embree/embree/tree/master/tutorials/bvh_builder
 */

// ****************************************************
// External variables
// ****************************************************
extern bool g_bIsTargetHighlighted, g_bPrevIsTargetHighlighted, g_bAOEnabled;
extern bool g_bExplosionsDisplayedOnCurrentFrame, g_bEnableVR, g_bProceduralLava;
extern bool g_bIsScaleableGUIElem, g_bScaleableHUDStarted, g_bIsTrianglePointer;
extern float g_fSSAOAlphaOfs, g_fFloatingGUIObjDepth;

// ****************************************************
// Debug variables
// ****************************************************
extern bool g_bDumpSSAOBuffers;
extern int g_iD3DExecuteCounter, g_iD3DExecuteCounterSkipHi, g_iD3DExecuteCounterSkipLo;

// ****************************************************
// Temporary, we may not need these here
// ****************************************************
extern bool g_bPrevIsScaleableGUIElem, g_bStartedGUI;
extern bool g_bIsFloating3DObject, g_bPrevIsFloatingGUI3DObject;
extern bool g_bTargetCompDrawn;
extern bool g_bEnableAnimations;
extern HyperspacePhaseEnum g_HyperspacePhaseFSM;

bool IsInsideTriangle(Vector2 P, Vector2 A, Vector2 B, Vector2 C, float *u, float *v);
Matrix4 ComputeLightViewMatrix(int idx, Matrix4 &Heading, bool invert);

class EffectsRenderer : public D3dRenderer
{
protected:
	bool _bLastTextureSelectedNotNULL, _bLastLightmapSelectedNotNULL, _bIsLaser, _bIsCockpit, _bIsExterior;
	bool _bIsGunner, _bIsExplosion, _bIsBlastMark, _bHasMaterial, _bDCIsTransparent, _bDCElemAlwaysVisible;
	bool _bModifiedShaders, _bModifiedPixelShader, _bModifiedBlendState, _bModifiedSamplerState, _bIsActiveCockpit;
	bool _bIsNoisyHolo, _bWarheadLocked, _bIsTargetHighlighted, _bIsHologram, _bRenderingLightingEffect;
	bool _bExternalCamera, _bCockpitDisplayed, _bIsTransparentCall;
	bool _bShadowsRenderedInCurrentFrame, _bJoystickTransformReady;
	bool _bHangarShadowsRenderedInCurrentFrame;

	Direct3DTexture *_lastTextureSelected = nullptr;
	Direct3DTexture *_lastLightmapSelected = nullptr;
	std::vector<DrawCommand> _LaserDrawCommands;
	std::vector<DrawCommand> _TransparentDrawCommands;
	std::vector<DrawCommand> _ShadowMapDrawCommands;
	Matrix4 _joystickMeshTransform;
	Matrix4 _throttleMeshTransform;
	AABB _hangarShadowAABB;
	Matrix4 _hangarShadowMapRotation;
	TransparentLayerSelector _overrideRTV = TRANSP_LYR_NONE;

	VertexShaderCBuffer _oldVSCBuffer;
	PixelShaderCBuffer _oldPSCBuffer;
	DCPixelShaderCBuffer _oldDCPSCBuffer;
	ComPtr<ID3D11Buffer> _oldVSConstantBuffer;
	ComPtr<ID3D11Buffer> _oldPSConstantBuffer;
	ComPtr<ID3D11ShaderResourceView> _oldVSSRV[3];
	ComPtr<ID3D11ShaderResourceView> _oldPSSRV[13];
	ComPtr<ID3D11VertexShader> _oldVertexShader;
	ComPtr<ID3D11PixelShader> _oldPixelShader;
	ComPtr<ID3D11SamplerState> _oldPSSamplers[2];
	ComPtr<ID3D11RenderTargetView> _oldRTVs[8];
	ComPtr<ID3D11DepthStencilView> _oldDSV;
	ComPtr<ID3D11DepthStencilState> _oldDepthStencilState;
	ComPtr<ID3D11BlendState> _oldBlendState;
	ComPtr<ID3D11InputLayout> _oldInputLayout;
	ComPtr<ID3D11Buffer> _oldVertexBuffer, _oldIndexBuffer;
	Matrix4 _oldPose;
	float _oldTransformWorldView[16];

	ComPtr<ID3D11Buffer> _vrKeybVertexBuffer;
	ComPtr<ID3D11Buffer> _vrKeybIndexBuffer;
	ComPtr<ID3D11Buffer> _vrKeybMeshVerticesBuffer;
	ComPtr<ID3D11Buffer> _vrKeybMeshTexCoordsBuffer;
	ComPtr<ID3D11ShaderResourceView> _vrKeybMeshVerticesSRV;
	ComPtr<ID3D11ShaderResourceView> _vrKeybMeshTexCoordsSRV;
	ComPtr<ID3D11ShaderResourceView> _vrKeybTextureSRV;

	ComPtr<ID3D11Buffer> _vrDotVertexBuffer;
	ComPtr<ID3D11Buffer> _vrDotIndexBuffer;
	ComPtr<ID3D11Buffer> _vrDotMeshVerticesBuffer;
	ComPtr<ID3D11Buffer> _vrDotMeshTexCoordsBuffer;
	ComPtr<ID3D11ShaderResourceView> _vrDotMeshVerticesSRV;
	ComPtr<ID3D11ShaderResourceView> _vrDotMeshTexCoordsSRV;
	ComPtr<ID3D11ShaderResourceView> _vrGreenCirclesSRV;

	// Background meshes
	// Caps
	ComPtr<ID3D11Buffer> _bgCapVertexBuffer;
	ComPtr<ID3D11Buffer> _bgCapIndexBuffer;
	ComPtr<ID3D11Buffer> _bgCapMeshVerticesBuffer;
	ComPtr<ID3D11Buffer> _bgCapTexCoordsBuffer;
	ComPtr<ID3D11ShaderResourceView> _bgCapMeshVerticesSRV;
	ComPtr<ID3D11ShaderResourceView> _bgCapMeshTexCoordsSRV;

	// Sides (planets, etc)
	ComPtr<ID3D11Buffer> _bgSideVertexBuffer;
	ComPtr<ID3D11Buffer> _bgSideIndexBuffer;
	ComPtr<ID3D11Buffer> _bgSideMeshVerticesBuffer;
	ComPtr<ID3D11Buffer> _bgSideTexCoordsBuffer;
	ComPtr<ID3D11ShaderResourceView> _bgSideMeshVerticesSRV;
	ComPtr<ID3D11ShaderResourceView> _bgSideMeshTexCoordsSRV;

	// Cylinder Sides
	ComPtr<ID3D11Buffer> _bgCylVertexBuffer;
	ComPtr<ID3D11Buffer> _bgCylIndexBuffer;
	ComPtr<ID3D11Buffer> _bgCylMeshVerticesBuffer;
	ComPtr<ID3D11Buffer> _bgCylTexCoordsBuffer;
	ComPtr<ID3D11ShaderResourceView> _bgCylMeshVerticesSRV;
	ComPtr<ID3D11ShaderResourceView> _bgCylMeshTexCoordsSRV;

	bool _bDotsbRendered;
	bool _bHUDRendered;
	bool _bBracketsRendered;

	D3D11_PRIMITIVE_TOPOLOGY _oldTopology;
	UINT _oldStencilRef, _oldSampleMask;
	FLOAT _oldBlendFactor[4];
	UINT _oldStride, _oldOffset, _oldIOffset;
	DXGI_FORMAT _oldFormat;
	D3D11_VIEWPORT _oldViewports[2];
	UINT _oldNumViewports = 2;
	D3D11_RECT _oldScissorRect;

	// Set to true in the current frame if the BLAS needs to be updated.
	bool _BLASNeedsUpdate;

	void OBJDumpD3dVertices(const SceneCompData *scene, const Matrix4 &A);
	void SingleFileOBJDumpD3dVertices(const SceneCompData* scene, int trianglesCount, const std::string& name);
	HRESULT QuickSetZWriteEnabled(BOOL Enabled);
	void EnableTransparency();
	void EnableHoloTransparency();
	inline ID3D11RenderTargetView *SelectOffscreenBuffer();
	Matrix4 GetShadowMapLimits(Matrix4 L, float *OBJrange, float *OBJminZ);
	Matrix4 GetShadowMapLimits(const AABB &aabb, float *OBJrange, float *OBJminZ);

	void SaveContext();
	void RestoreContext();

	HRESULT CreateSRVFromBuffer(uint8_t* Buffer, int BufferLength, int Width, int Height, ID3D11ShaderResourceView** srv);
	int LoadDATImage(char* sDATFileName, int GroupId, int ImageId, ID3D11ShaderResourceView** srv,
		short* Width_out=nullptr, short* Height_out=nullptr);
	int LoadOBJ(int gloveIdx, Matrix4 R, char* sFileName, int profile, bool buildBVH);
	void IntersectVRGeometry();

public:
	bool _bCockpitConstantsCaptured;
	bool _bExteriorConstantsCaptured;
	D3dConstants _CockpitConstants;
	D3dConstants _ExteriorConstants;
	XwaTransform _CockpitWorldView;

	// Captured at the beginning of every frame:
	D3dConstants _frameConstants;
	VertexShaderCBuffer _frameVSCBuffer;

	EffectsRenderer();
	virtual void CreateShaders();
	virtual void SceneBegin(DeviceResources* deviceResources);
	virtual void SceneEnd();
	virtual void MainSceneHook(const SceneCompData* scene);
	virtual void HangarShadowSceneHook(const SceneCompData* scene);
	virtual void RenderScene(bool bBindTranspLyr1);
	virtual void UpdateTextures(const SceneCompData* scene);
	
	// State Management
	void DoStateManagement(const SceneCompData* scene);
	void ApplyMaterialProperties();
	void ApplySpecialMaterials();
	void ApplyBloomSettings(float bloomOverride);
	void ApplyDiegeticCockpit();
	void ApplyMeshTransform();
	void ApplyActiveCockpit(const SceneCompData* scene);
	void DCCaptureMiniature();
	// Returns true if the current draw call needs to be skipped
	bool DCReplaceTextures();
	virtual void ExtraPreprocessing();
	void AddLaserLights(const SceneCompData* scene);
	void ApplyProceduralLava();
	void ApplyGreebles();
	void ApplyAnimatedTextures(int objectId, bool bInstanceEvent, FixedInstanceData* fixedInstanceData);
	void ApplyNormalMapping();

	// Raytracing
	LBVH* BuildBVH(const std::vector<XwaVector3>& vertices, const std::vector<int>& indices);
	void BuildSingleBLASFromCurrentBVHMap();
	void BuildMultipleBLASFromCurrentBLASMap();
	void ReAllocateAndPopulateBvhBuffers(const int numNodes);
	void ReAllocateAndPopulateTLASBvhBuffers();
	void ReAllocateAndPopulateMatrixBuffer();
	void ApplyRTShadowsTechRoom(const SceneCompData* scene);
	//void AddAABBToTLAS(const Matrix4& WorldViewTransform, int meshID, AABB aabb, int matrixSlot);
	bool GetOPTNameFromLastTextureSelected(char* OPTname);
	void UpdateBVHMaps(const SceneCompData* scene, int LOD);
	bool RTCheckExcludeMesh(const SceneCompData* scene);

	// Per-texture, per-instance effects
	CraftInstance *ObjectIDToCraftInstance(int objectId, MobileObjectEntry** mobileObject_out);
	InstanceEvent *ObjectIDToInstanceEvent(int objectId, uint32_t materialId);
	FixedInstanceData* ObjectIDToFixedInstanceData(int objectId, uint32_t materialId);

	void CreateRectangleMesh(
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
		/* out */ ComPtr<ID3D11ShaderResourceView>& texCoordsSRV);

	void CreateFlatRectangleMesh(
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
		/* out */ ComPtr<ID3D11ShaderResourceView>& texCoordsSRV);

	void CreateCylinderSideMesh(
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
		/* out */ ComPtr<ID3D11ShaderResourceView>& texCoordsSRV);

	void CreateVRMeshes();
	void CreateBackgroundMeshes();
	void CreateBackdropIdMapping();

	// Deferred rendering
	void RenderLasers();
	void RenderTransparency();
	void RenderVRDots();
	void RenderVRBrackets();
	void RenderVRHUD();
	void RenderVRSkyBox(bool debug);
	void RenderVRKeyboard();
	void RenderVRGloves();
	void RenderSkyCylinder();
	void RenderCockpitShadowMap();
	void RenderHangarShadowMap();
	void StartCascadedShadowMap();
	void RenderCascadedShadowMap();
	void EndCascadedShadowMap();
	void RenderDeferredDrawCalls();
};

extern EffectsRenderer *g_effects_renderer;

extern D3dRenderer *g_current_renderer;