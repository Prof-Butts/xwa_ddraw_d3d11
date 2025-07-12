// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

//#include <ScreenGrab.h>
//#include <wincodec.h>

#include "common.h"
#include "DeviceResources.h"
#include "XwaD3dRendererHook.h"
#include "XwaConcourseHook.h"

#include <ScreenGrab.h>
#include <wincodec.h>

#ifdef _DEBUG
#include "../Debug/MainVertexShader.h"
#include "../Debug/MainVertexShaderVR.h"
#include "../Debug/MainPixelShader.h"
#include "../Debug/MainPixelShaderBpp2ColorKey20.h"
#include "../Debug/MainPixelShaderBpp2ColorKey00.h"
#include "../Debug/MainPixelShaderBpp4ColorKey20.h"
#include "../Debug/BarrelPixelShader.h"
#include "../Debug/SimpleResizePS.h"
#include "../Debug/SteamVRMirrorPixelShader.h"
#include "../Debug/SingleBarrelPixelShader.h"
#include "../Debug/VertexShader.h"
#include "../Debug/PassthroughVertexShader.h"
#include "../Debug/SBSVertexShader.h"
#include "../Debug/DATVertexShaderVR.h"
#include "../Debug/PixelShaderTexture.h"
#include "../Debug/PixelShaderDC.h"
#include "../Debug/PixelShaderDCHolo.h"
#include "../Debug/PixelShaderEmptyDC.h"
#include "../Debug/PixelShaderHUD.h"
#include "../Debug/EnhancedHudPS.h"
#include "../Debug/PixelShaderSolid.h"
#include "../Debug/PixelShaderClearBox.h"
#include "../Debug/BloomHGaussPS.h"
#include "../Debug/BloomHGaussPS_VR.h"
#include "../Debug/BloomVGaussPS.h"
#include "../Debug/BloomVGaussPS_VR.h"
#include "../Debug/BloomCombinePS.h"
#include "../Debug/BloomCombinePS_VR.h"
#include "../Debug/BloomBufferAddPS.h"
#include "../Debug/BloomBufferAddPS_VR.h"
#include "../Debug/SSAOPixelShader.h"
#include "../Debug/SSAOPixelShaderVR.h"
#include "../Debug/SSAOBlurPixelShader.h"
#include "../Debug/SSAOBlurPixelShaderVR.h"
#include "../Debug/SSAOAddPixelShader.h"
#include "../Debug/SSDODirect.h"
#include "../Debug/SSDODirectVR.h"
#include "../Debug/SSDOIndirect.h"
#include "../Debug/SSDOAdd.h"
#include "../Debug/SSDOAddVR.h"
#include "../Debug/SSDOBlur.h"
#include "../Debug/SSDOBlurVR.h"
#include "../Debug/DeathStarShader.h"
#include "../Debug/HyperEntry.h"
#include "../Debug/HyperExit.h"
#include "../Debug/HyperTunnel.h"
#include "../Debug/HyperCompose.h"
#include "../Debug/HyperComposeVR.h"
#include "../Debug/HyperZoom.h"
#include "../Debug/LaserPointer.h"
#include "../Debug/LaserPointerVR.h"
#include "../Debug/FXAA.h"
#include "../Debug/FXAA_VR.h"
#include "../Debug/ExternalHUDShader.h"
#include "../Debug/ExternalHUDShaderVR.h"
#include "../Debug/SunFlareShader.h"
#include "../Debug/SunFlareShaderVR.h"
#include "../Debug/SunShader.h"
#include "../Debug/SunFlareCompose.h"
#include "../Debug/SunFlareComposeVR.h"
#include "../Debug/SpeedEffectPixelShader.h"
#include "../Debug/SpeedEffectPixelShaderVR.h"
#include "../Debug/SpeedEffectVertexShader.h"
#include "../Debug/SpeedEffectVertexShaderVR.h"
#include "../Debug/AddGeometryVertexShader.h"
#include "../Debug/AddGeometryPixelShader.h"
#include "../Debug/AddGeometryComposePixelShader.h"
#include "../Debug/HeadLightsPS.h"
#include "../Debug/HeadLightsPS_VR.h"
#include "../Debug/HeadLightsSSAOPS.h"
#include "../Debug/ShadowMapPS.h"
#include "../Debug/ShadowMapVS.h"
#include "../Debug/EdgeDetector.h"
#include "../Debug/StarDebug.h"
#include "../Debug/LavaPixelShader.h"
#include "../Debug/ExplosionShader.h"
#include "../Debug/AlphaToBloomPS.h"
#include "../Debug/PixelShaderNoGlass.h"
#include "../Debug/PixelShaderAnim.h"
#include "../Debug/PixelShaderAnimDAT.h"
#include "../Debug/PixelShaderGreeble.h"
#include "../Debug/HangarShadowMapVS.h"
#include "../Debug/XwaD3dCSMVertexShader.h"
#include "../Debug/LevelsPS.h"
#include "../Debug/RTShadowMaskPS.h"
#include "../Debug/RTShadowMaskPS_VR.h"
#include "../Debug/PBRAdd.h"
#include "../Debug/PBRAddVR.h"
#include "../Debug/PixelShaderVRGeom.h"
#else
#include "../Release/MainVertexShader.h"
#include "../Release/MainVertexShaderVR.h"
#include "../Release/MainPixelShader.h"
#include "../Release/MainPixelShaderBpp2ColorKey20.h"
#include "../Release/MainPixelShaderBpp2ColorKey00.h"
#include "../Release/MainPixelShaderBpp4ColorKey20.h"
#include "../Release/BarrelPixelShader.h"
#include "../Release/SimpleResizePS.h"
#include "../Release/SteamVRMirrorPixelShader.h"
#include "../Release/SingleBarrelPixelShader.h"
#include "../Release/VertexShader.h"
#include "../Release/PassthroughVertexShader.h"
#include "../Release/SBSVertexShader.h"
#include "../Release/DATVertexShaderVR.h"
#include "../Release/PixelShaderTexture.h"
#include "../Release/PixelShaderDC.h"
#include "../Release/PixelShaderDCHolo.h"
#include "../Release/PixelShaderEmptyDC.h"
#include "../Release/PixelShaderHUD.h"
#include "../Release/EnhancedHudPS.h"
#include "../Release/PixelShaderSolid.h"
#include "../Release/PixelShaderClearBox.h"
#include "../Release/BloomHGaussPS.h"
#include "../Release/BloomHGaussPS_VR.h"
#include "../Release/BloomVGaussPS.h"
#include "../Release/BloomVGaussPS_VR.h"
#include "../Release/BloomCombinePS.h"
#include "../Release/BloomCombinePS_VR.h"
#include "../Release/BloomBufferAddPS.h"
#include "../Release/BloomBufferAddPS_VR.h"
#include "../Release/SSAOPixelShader.h"
#include "../Release/SSAOPixelShaderVR.h"
#include "../Release/SSAOBlurPixelShader.h"
#include "../Release/SSAOBlurPixelShaderVR.h"
#include "../Release/SSAOAddPixelShader.h"
#include "../Release/SSDODirect.h"
#include "../Release/SSDODirectVR.h"
#include "../Release/SSDOIndirect.h"
#include "../Release/SSDOAdd.h"
#include "../Release/SSDOAddVR.h"
#include "../Release/SSDOBlur.h"
#include "../Release/SSDOBlurVR.h"
#include "../Release/DeathStarShader.h"
#include "../Release/HyperEntry.h"
#include "../Release/HyperExit.h"
#include "../Release/HyperTunnel.h"
#include "../Release/HyperCompose.h"
#include "../Release/HyperComposeVR.h"
#include "../Release/HyperZoom.h"
#include "../Release/LaserPointer.h"
#include "../Release/LaserPointerVR.h"
#include "../Release/FXAA.h"
#include "../Release/FXAA_VR.h"
#include "../Release/ExternalHUDShader.h"
#include "../Release/ExternalHUDShaderVR.h"
#include "../Release/SunFlareShader.h"
#include "../Release/SunFlareShaderVR.h"
#include "../Release/SunShader.h"
#include "../Release/SunFlareCompose.h"
#include "../Release/SunFlareComposeVR.h"
#include "../Release/SpeedEffectPixelShader.h"
#include "../Release/SpeedEffectPixelShaderVR.h"
#include "../Release/SpeedEffectVertexShader.h"
#include "../Release/SpeedEffectVertexShaderVR.h"
#include "../Release/AddGeometryVertexShader.h"
#include "../Release/AddGeometryPixelShader.h"
#include "../Release/AddGeometryComposePixelShader.h"
#include "../Release/HeadLightsPS.h"
#include "../Release/HeadLightsPS_VR.h"
#include "../Release/HeadLightsSSAOPS.h"
#include "../Release/ShadowMapPS.h"
#include "../Release/ShadowMapVS.h"
#include "../Release/EdgeDetector.h"
#include "../Release/StarDebug.h"
#include "../Release/LavaPixelShader.h"
#include "../Release/ExplosionShader.h"
#include "../Release/AlphaToBloomPS.h"
#include "../Release/PixelShaderNoGlass.h"
#include "../Release/PixelShaderAnim.h"
#include "../Release/PixelShaderAnimDAT.h"
#include "../Release/PixelShaderGreeble.h"
#include "../Release/HangarShadowMapVS.h"
#include "../Release/XwaD3dCSMVertexShader.h"
#include "../Release/LevelsPS.h"
#include "../Release/RTShadowMaskPS.h"
#include "../Release/RTShadowMaskPS_VR.h"
#include "../Release/PBRAdd.h"
#include "../Release/PBRAddVR.h"
#include "../Release/PixelShaderVRGeom.h"
#endif

#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>
#include <openvr.h>
#include <vector>
#include "SteamVR.h"
#include "globals.h"
#include "VRConfig.h"
#include "effects.h"


inline Vector3 project(Vector3 pos3D, Matrix4 viewMatrix, Matrix4 projEyeMatrix);
float3 InverseTransformProjectionScreen(float4 input);
void SetPresentCounter(int val, int b_resetReticle);
void ReloadInterdictionMap();
void ClearGlobalTextureMap();
void ClearCachedSRVs();
void ClearGroupIdImageIdToTextureMap();
void ClearSpeciesCompMap();

bool g_bWndProcReplaced = false;
bool ReplaceWindowProc(HWND ThisWindow);
extern MainShadersCBuffer g_MSCBuffer;
extern BarrelPixelShaderCBuffer g_BarrelPSCBuffer;
extern float g_fConcourseScale, g_fConcourseAspectRatio, g_fTechLibraryParallax, g_fBrightness;
extern bool g_bRendering3D, g_bDumpDebug, g_bOverrideAspectRatio, g_bCustomFOVApplied, g_bTargetCompDrawn;
extern int g_iPresentCounter;

extern bool g_bEnableVR, g_bForceViewportChange;
extern Matrix4 g_FullProjMatrixLeft, g_FullProjMatrixRight;
extern VertexShaderMatrixCB g_VSMatrixCB;
// DYNAMIC COCKPIT
extern dc_element g_DCElements[];
extern int g_iNumDCElements;
extern char g_sCurrentCockpit[128];
extern DCHUDRegions g_DCHUDRegions;
extern DCElemSrcBoxes g_DCElemSrcBoxes;
extern float g_fCurrentShipFocalLength;
extern bool g_bExecuteBufferLock, g_bDCApplyEraseRegionCommands, g_bHUDVisibleOnStartup;

// ACTIVE COCKPIT
extern bool g_bActiveCockpitEnabled;
extern ac_element g_ACElements[];
extern int g_iNumACElements;

extern bool g_bReshadeEnabled, g_bBloomEnabled;

extern float g_fHUDDepth;
extern float g_fCurInGameWidth, g_fCurInGameHeight, g_fCurInGameAspectRatio;
extern float g_fCurScreenWidth, g_fCurScreenHeight, g_fCurScreenWidthRcp, g_fCurScreenHeightRcp;

DWORD g_FullScreenWidth = 0, g_FullScreenHeight = 0;

// Speed Effect
Vector4 g_SpeedParticles[MAX_SPEED_PARTICLES];
extern float g_fSpeedShaderParticleRange;

// Shadow Mapping
extern ShadowMapVertexShaderMatrixCB g_ShadowMapVSCBuffer;
extern bool g_bShadowMappingEnabled;

// Metric Reconstruction
extern MetricReconstructionCB g_MetricRecCBuffer;
extern bool g_bYCenterHasBeenFixed;

void ResetXWALightInfo();
void ResetObjectIndexMap();

// Raytracing

// This maps meshKeys (the vertex pointer) to MeshData tuples.
// This map is used to build a single BLAS in the Tech Room and
// gets cleared when a new OPT is loaded.
// It's also used to build Coalesced BVHs: all FGs in the same mesh
// are coalesced into a single BVH. It gets cleared when OnSizeChanged()
// is called.
std::map<int32_t, MeshData> g_LBVHMap;

// This maps (MeshKey, LOD) to BlasIds. Gets cleared when OnSizeChanged()
std::map<BLASKey_t, int> g_BLASIdMap;

// This maps BlasIds to BLASData tuples.
// This map is used to build multiple BLASes during regular flight
// (one per BlasId). It gets cleared when OnSizeChanged() is called
std::map<int, BLASData> g_BLASMap;

// This maps (BlasId, centroid) to matrix slots.
// It gets cleared at the beginning of every frame.
// The centroid is important because the same BLAS may appear multiple
// times if there are multiple ships in the same (Mesh, LOD), but in
// different locations, for instance.
std::map<IDCentroid_t, int32_t> g_TLASMap;
std::vector<Matrix4> g_TLASMatrices;
void ClearGlobalLBVHMap();

#undef DEBUG_RT
#ifdef DEBUG_RT
// DEBUG: Map of meshKey --> (OPTname, vertcount, meshIndex). Only for debugging purposes.
std::map<int32_t, std::tuple<std::string, int, int>> g_DebugMeshToNameMap;
#endif

D3D11_VIEWPORT g_concourseViewport;

/* The different types of Constant Buffers used in the Pixel Shader: */
typedef enum {
	PS_CONSTANT_BUFFER_NONE,
	PS_CONSTANT_BUFFER_BARREL,
	PS_CONSTANT_BUFFER_2D,
	PS_CONSTANT_BUFFER_3D,
	PS_CONSTANT_BUFFER_BLOOM,
	PS_CONSTANT_BUFFER_SSAO
} PSConstantBufferType;
PSConstantBufferType g_LastPSConstantBufferSet = PS_CONSTANT_BUFFER_NONE;

FILE *g_DebugFile = NULL;

class XwaTextureData;
extern std::vector<XwaTextureData*> g_AuxTextureVector;

inline float lerp(float x, float y, float s) {
	return x + s * (y - x);
}

void log_err(const char *format, ...)
{
	char buf[120];

	if (g_DebugFile == NULL)
		fopen_s(&g_DebugFile, "./vrerror.log", "wt");

	va_list args;
	va_start(args, format);

	vsprintf_s(buf, 120, format, args);
	fprintf(g_DebugFile, buf);

	va_end(args);
}

void close_error_file() {
	if (g_DebugFile != NULL)
		fclose(g_DebugFile);
}

void log_err_desc(const char *step, HWND hWnd, HRESULT hr, CD3D11_TEXTURE2D_DESC desc) {
	log_err("step: %s\n", step);
	log_err("hWnd: 0x%x\n", hWnd);
	log_err("TEXTURE2D_DESC:\n");
	log_err("hr: 0x%x, %s\n", hr, _com_error(hr).ErrorMessage());
	log_err("ArraySize: %d\n", desc.ArraySize);
	log_err("Format: %d\n", desc.Format);
	log_err("BindFlags: %d\n", desc.BindFlags);
	log_err("CPUAccessFlags: %d\n", desc.CPUAccessFlags);
	log_err("MipLevels: %d\n", desc.MipLevels);
	log_err("MiscFlags: %d\n", desc.MiscFlags);
	log_err("SampleDesc: %d\n", desc.SampleDesc);
	log_err("Usage: %d\n", desc.Usage);
	log_err("Width: %d\n", desc.Width);
	log_err("Height: %d\n", desc.Height);
	log_err("Sample Count: %d\n", desc.SampleDesc.Count);
	log_err("Sample Quality: %d\n", desc.SampleDesc.Quality);
}

void log_shaderres_view(const char *step, HWND hWnd, HRESULT hr, D3D11_SHADER_RESOURCE_VIEW_DESC desc) {
	log_err("step: %s\n", step);
	log_err("hWnd: 0x%x\n", hWnd);
	log_err("SHADER_RESOURCE_VIEW_DESC:\n");
	log_err("hr: 0x%x, %s\n", hr, _com_error(hr).ErrorMessage());
	log_err("Format: %d\n", desc.Format);
	log_err("ViewDimension: %d\n", desc.ViewDimension);
	log_err("MostDetailedMip: %d\n", desc.Texture2D.MostDetailedMip);
	log_err("MipLevels: %d\n", desc.Texture2D.MipLevels);
}

struct MainVertex
{
	float pos[2];
	float tex[2];

	MainVertex()
	{
		this->pos[0] = 0;
		this->pos[1] = 0;
		this->tex[0] = 0;
		this->tex[1] = 0;
	}

	MainVertex(float x, float y, float u, float v)
	{
		this->pos[0] = x;
		this->pos[1] = y;
		this->tex[0] = u;
		this->tex[1] = v;
	}
};

DeviceResources* g_deviceResources = nullptr;

DeviceResources::DeviceResources()
{
	this->_primarySurface = nullptr;
	this->_depthSurface = nullptr;
	this->_backbufferSurface = nullptr;
	this->_frontbufferSurface = nullptr;
	this->_offscreenSurface = nullptr;
	this->_grayNoiseTex = nullptr;
	this->_grayNoiseSRV = nullptr;
	this->_vision3DSignatureTex = nullptr;
	this->_vision3DSignatureSRV = nullptr;
	this->_tgSmushTex = nullptr;
	this->_tgSmushTexWidth = this->_tgSmushTexHeight = -1;

	this->_useAnisotropy = g_config.AnisotropicFilteringEnabled ? TRUE : FALSE;
	this->_useMultisampling = g_config.MultisamplingAntialiasingEnabled ? TRUE : FALSE;

	this->_sampleDesc.Count = 1;
	this->_sampleDesc.Quality = 0;

	this->_backbufferWidth = 0;
	this->_backbufferHeight = 0;
	this->_refreshRate = { 0, 1 };
	this->_are16BppTexturesSupported = false;
	this->_use16BppMainDisplayTexture = false;

	this->_d3dAnnotation = nullptr;

	const float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	memcpy(this->clearColor, &color, sizeof(color));

	const float colorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	memcpy(this->clearColorRGBA, &colorRGBA, sizeof(colorRGBA));

	this->clearDepth = 1.0f;

	this->sceneRendered = false;
	this->sceneRenderedEmpty = false;
	this->inScene = false;

	this->_surfaceDcCallback = nullptr;

	this->_postProcessVertBuffer = nullptr;
	this->_HUDVertexBuffer = nullptr;
	this->_clearHUDVertexBuffer = nullptr;
	this->_hyperspaceVertexBuffer = nullptr;
	this->_bHUDVerticesReady = false;
	this->_speedParticlesVertexBuffer = nullptr;
	this->_shadowMappingVSConstantBuffer = nullptr;
	this->_shadowVertexBuffer = nullptr;
	this->_shadowIndexBuffer = nullptr;
	//this->_reticleVertexBuffer = nullptr;

	for (int i = 0; i < MAX_DC_SRC_ELEMENTS; i++)
		this->dc_coverTexture[i] = nullptr;

	_extraTextures.clear();

	this->_currentPixelShader = nullptr;

	g_deviceResources = this;
}

DeviceResources::~DeviceResources()
{
	D3dRendererUninitialize();
}

HRESULT DeviceResources::Initialize()
{
	HRESULT hr;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		//D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


	this->_d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;

	hr = D3D11CreateDevice(nullptr, this->_d3dDriverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
		D3D11_SDK_VERSION, &this->_d3dDevice, &this->_d3dFeatureLevel, &this->_d3dDeviceContext);

	if (FAILED(hr))
	{
		this->_d3dDriverType = D3D_DRIVER_TYPE_WARP;
		hr = D3D11CreateDevice(nullptr, this->_d3dDriverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &this->_d3dDevice, &this->_d3dFeatureLevel, &this->_d3dDeviceContext);
	}

	this->_d3dDeviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&this->_d3dAnnotation);

	/*
	if (SUCCEEDED(hr)) {
		log_debug("[DBG] Getting the DX11.1 device");
		hr = this->_d3dDevice->QueryInterface(__uuidof(ID3D11Device1),
			reinterpret_cast<void**>(&this->_d3dDevice1));
		log_debug("[DBG] hr: 0x%x", hr);
	} else {
		this->_d3dDevice1 = NULL;
	}

	if (SUCCEEDED(hr)) {
		log_debug("[DBG] Getting ID3D11DeviceContext1 ");
		hr = this->_d3dDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1),
			reinterpret_cast<void**>(&this->_d3dDeviceContext1));
		log_debug("[DBG] hr: 0x%x", hr);
	} else {
		this->_d3dDeviceContext1 = NULL;
	}
	*/

	if (SUCCEEDED(hr))
	{
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &this->_d2d1Factory);
	}

	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(this->_dwriteFactory), (IUnknown**)&this->_dwriteFactory);
	}

	if (SUCCEEDED(hr))
	{
		this->CheckMultisamplingSupport();
	}

	if (SUCCEEDED(hr))
	{
		this->_are16BppTexturesSupported =
			this->IsTextureFormatSupported(DXGI_FORMAT_B4G4R4A4_UNORM)
			&& this->IsTextureFormatSupported(DXGI_FORMAT_B5G5R5A1_UNORM)
			&& this->IsTextureFormatSupported(DXGI_FORMAT_B5G6R5_UNORM);

		this->_use16BppMainDisplayTexture = this->_are16BppTexturesSupported && (this->_d3dFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
	}

	if (SUCCEEDED(hr))
	{
		static int counter = 0;
		hr = this->LoadMainResources();
	}

	if (SUCCEEDED(hr))
	{
		static int counter = 0;
		hr = this->LoadResources();
	}

	if (SUCCEEDED(hr))
	{
		D3dRendererInitialize();
	}

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			MessageBox(nullptr, _com_error(hr).ErrorMessage(), __FUNCTION__, MB_ICONERROR);
		}

		messageShown = true;
	}

	return hr;
}

void DeviceResources::BuildHUDVertexBuffer(float width, float height) {
	HRESULT hr;

	D3DCOLOR color = 0xFFFFFFFF; // AABBGGRR
	auto &device = this->_d3dDevice;
	//float depth = g_fHUDDepth;
	// The values for rhw_depth and sz_depth were taken from an actual sample from the X-Wing's front panel
	float rhw_depth = 34.0f;
	float sz_depth  = 0.98f;
	D3DTLVERTEX HUDVertices[6] = { 0 };

	HUDVertices[0].sx  = 0;
	HUDVertices[0].sy  = 0;
	HUDVertices[0].sz  = sz_depth;
	HUDVertices[0].rhw = rhw_depth;
	HUDVertices[0].tu  = 0;
	HUDVertices[0].tv  = 0;
	HUDVertices[0].color = color;

	HUDVertices[1].sx = width;
	HUDVertices[1].sy = 0;
	HUDVertices[1].sz  = sz_depth;
	HUDVertices[1].rhw = rhw_depth;
	HUDVertices[1].tu  = 1;
	HUDVertices[1].tv  = 0;
	HUDVertices[1].color = color;
	
	HUDVertices[2].sx = width;
	HUDVertices[2].sy = height;
	HUDVertices[2].sz  = sz_depth;
	HUDVertices[2].rhw = rhw_depth;
	HUDVertices[2].tu  = 1;
	HUDVertices[2].tv  = 1;
	HUDVertices[2].color = color;
	
	HUDVertices[3].sx = width;
	HUDVertices[3].sy = height;
	HUDVertices[3].sz  = sz_depth;
	HUDVertices[3].rhw = rhw_depth;
	HUDVertices[3].tu  = 1;
	HUDVertices[3].tv  = 1;
	HUDVertices[3].color = color;
	
	HUDVertices[4].sx = 0;
	HUDVertices[4].sy = height;
	HUDVertices[4].sz  = sz_depth;
	HUDVertices[4].rhw = rhw_depth;
	HUDVertices[4].tu  = 0;
	HUDVertices[4].tv  = 1;
	HUDVertices[4].color = color;
	
	HUDVertices[5].sx  = 0;
	HUDVertices[5].sy  = 0;
	HUDVertices[5].sz  = sz_depth;
	HUDVertices[5].rhw = rhw_depth;
	HUDVertices[5].tu  = 0;
	HUDVertices[5].tv  = 0;
	HUDVertices[5].color = color;	

	/* Create the VertexBuffer if necessary */
	if (this->_HUDVertexBuffer != NULL) {
		this->_HUDVertexBuffer->Release();
		this->_HUDVertexBuffer = NULL;
	}

	if (this->_clearHUDVertexBuffer != NULL) {
		this->_clearHUDVertexBuffer->Release();
		this->_clearHUDVertexBuffer = NULL;
	}

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(D3DTLVERTEX) * ARRAYSIZE(HUDVertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = HUDVertices;
	hr = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->_HUDVertexBuffer);
	if (FAILED(hr)) {
		log_debug("[DBG] [DC] Could not create _HUDVertexBuffer");
		this->_bHUDVerticesReady = false;
	}

	// Build the vertex buffer that will be used to clear areas of the offscreen DC buffer:
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.StructureByteStride = 0;
	hr = device->CreateBuffer(&vertexBufferDesc, nullptr, &this->_clearHUDVertexBuffer);
	if (FAILED(hr)) {
		log_debug("[DBG] [DC] Could not create _clearHUDVertexBuffer");
		this->_bHUDVerticesReady = false;
	}

	this->_bHUDVerticesReady = true;
}

void DumpHyperspaceVertexBuffer(float width, float height)
{
	D3DCOLOR color = 0xFFFFFFFF; // AABBGGRR
	// The values for rhw_depth and sz_depth were taken from the skybox
	float rhw_depth = 0.000863f; // this is the inverse of the depth (?)
	float sz_depth = 0.001839f;   // this is the Z-buffer value (?)
	D3DTLVERTEX HyperVertices[6] = { 0 };

	HyperVertices[0].sx = 0;
	HyperVertices[0].sy = 0;
	HyperVertices[0].sz = sz_depth;
	HyperVertices[0].rhw = rhw_depth;
	HyperVertices[0].tu = 0;
	HyperVertices[0].tv = 0;
	HyperVertices[0].color = color;

	HyperVertices[1].sx = width;
	HyperVertices[1].sy = 0;
	HyperVertices[1].sz = sz_depth;
	HyperVertices[1].rhw = rhw_depth;
	HyperVertices[1].tu = 1;
	HyperVertices[1].tv = 0;
	HyperVertices[1].color = color;

	HyperVertices[2].sx = width;
	HyperVertices[2].sy = height;
	HyperVertices[2].sz = sz_depth;
	HyperVertices[2].rhw = rhw_depth;
	HyperVertices[2].tu = 1;
	HyperVertices[2].tv = 1;
	HyperVertices[2].color = color;

	HyperVertices[3].sx = width;
	HyperVertices[3].sy = height;
	HyperVertices[3].sz = sz_depth;
	HyperVertices[3].rhw = rhw_depth;
	HyperVertices[3].tu = 1;
	HyperVertices[3].tv = 1;
	HyperVertices[3].color = color;

	HyperVertices[4].sx = 0;
	HyperVertices[4].sy = height;
	HyperVertices[4].sz = sz_depth;
	HyperVertices[4].rhw = rhw_depth;
	HyperVertices[4].tu = 0;
	HyperVertices[4].tv = 1;
	HyperVertices[4].color = color;

	HyperVertices[5].sx = 0;
	HyperVertices[5].sy = 0;
	HyperVertices[5].sz = sz_depth;
	HyperVertices[5].rhw = rhw_depth;
	HyperVertices[5].tu = 0;
	HyperVertices[5].tv = 0;
	HyperVertices[5].color = color;

	FILE* file = NULL;
	fopen_s(&file, "./HyperspaceVertices.obj", "wt");
	// See DumpVerticesToOBJ
	float4 input;
	for (int i = 0; i < 6; i++)
	{
		input.x = HyperVertices[i].sx;
		input.y = HyperVertices[i].sy;
		input.z = HyperVertices[i].sz;
		input.w = HyperVertices[i].rhw;
		float3 pos = InverseTransformProjectionScreen(input);
		//pos.x *= OPT_TO_METERS;
		//pos.y *= OPT_TO_METERS;
		//pos.z *= OPT_TO_METERS;
		fprintf(file, "v %0.6f %0.6f %0.6f\n", pos.x, pos.y, pos.z);
	}
	fprintf(file, "\n");
	fprintf(file, "f 1 2 3\n");
	fprintf(file, "f 4 5 6\n");
	fclose(file);
	log_debug("[DBG] Dumped Hyperspace Vertex Buffer");
}

void DeviceResources::BuildHyperspaceVertexBuffer(float width, float height) {
	HRESULT hr;
	D3DCOLOR color = 0xFFFFFFFF; // AABBGGRR
	auto &device = this->_d3dDevice;
	//float depth = g_fHUDDepth;
	// The values for rhw_depth and sz_depth were taken from the skybox
	float rhw_depth = 0.000863f; // this is the inverse of the depth (?)
	float sz_depth = 0.001839f;   // this is the Z-buffer value (?)
	// Why do I even have to bother? Can I just use my *own* vertex shader and do
	// away with all this silliness?
	D3DTLVERTEX HyperVertices[6] = { 0 };

	HyperVertices[0].sx  = 0;
	HyperVertices[0].sy  = 0;
	HyperVertices[0].sz  = sz_depth;
	HyperVertices[0].rhw = rhw_depth;
	HyperVertices[0].tu  = 0;
	HyperVertices[0].tv  = 0;
	HyperVertices[0].color = color;

	HyperVertices[1].sx  = width;
	HyperVertices[1].sy  = 0;
	HyperVertices[1].sz  = sz_depth;
	HyperVertices[1].rhw = rhw_depth;
	HyperVertices[1].tu  = 1;
	HyperVertices[1].tv  = 0;
	HyperVertices[1].color = color;

	HyperVertices[2].sx  = width;
	HyperVertices[2].sy  = height;
	HyperVertices[2].sz  = sz_depth;
	HyperVertices[2].rhw = rhw_depth;
	HyperVertices[2].tu  = 1;
	HyperVertices[2].tv  = 1;
	HyperVertices[2].color = color;

	HyperVertices[3].sx  = width;
	HyperVertices[3].sy  = height;
	HyperVertices[3].sz  = sz_depth;
	HyperVertices[3].rhw = rhw_depth;
	HyperVertices[3].tu  = 1;
	HyperVertices[3].tv  = 1;
	HyperVertices[3].color = color;

	HyperVertices[4].sx  = 0;
	HyperVertices[4].sy  = height;
	HyperVertices[4].sz  = sz_depth;
	HyperVertices[4].rhw = rhw_depth;
	HyperVertices[4].tu  = 0;
	HyperVertices[4].tv  = 1;
	HyperVertices[4].color = color;

	HyperVertices[5].sx  = 0;
	HyperVertices[5].sy  = 0;
	HyperVertices[5].sz  = sz_depth;
	HyperVertices[5].rhw = rhw_depth;
	HyperVertices[5].tu  = 0;
	HyperVertices[5].tv  = 0;
	HyperVertices[5].color = color;

	/* Create the VertexBuffer if necessary */
	if (this->_hyperspaceVertexBuffer != NULL) {
		// Calling Release here was causing a crash when the game was exiting; but
		// only if dynamic_cockpit is disabled! If DC is enabled, Release can be
		// called without problems... why?
		// Why is this not happening with the HUD vertices buffer?
		// I think the problem was that BuildHUDVertexBuffer and BuildHyperspaceVertexBuffer
		// both used d3dDevice, which probably increased the refcount for that object directly
		// without updating the refcount in DirectDraw. Making these variables members and the
		// functions methods in DeviceResources seems to have fixed these problems.
		this->_hyperspaceVertexBuffer->Release();
		this->_hyperspaceVertexBuffer = NULL;
	}

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(D3DTLVERTEX) * ARRAYSIZE(HyperVertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = HyperVertices;
	hr = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->_hyperspaceVertexBuffer);
	if (FAILED(hr)) {
		log_debug("[DBG] [DC] Could not create g_HyperspaceVertexBuffer");
	}
}

void DeviceResources::BuildPostProcVertexBuffer() 
{
	auto &device = this->_d3dDevice;
	D3D11_BUFFER_DESC vertexBufferDesc;
	MainVertex g_BarrelEffectVertices[6] = {
		MainVertex(-1, -1, 0, 1),
		MainVertex( 1, -1, 1, 1),
		MainVertex( 1,  1, 1, 0),

		MainVertex( 1,  1, 1, 0),
		MainVertex(-1,  1, 0, 0),
		MainVertex(-1, -1, 0, 1),
	};

	if (this->_postProcessVertBuffer != NULL) {
		this->_postProcessVertBuffer->Release();
		this->_postProcessVertBuffer = NULL;
	}

	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(MainVertex) * ARRAYSIZE(g_BarrelEffectVertices);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = g_BarrelEffectVertices;
	HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, this->_postProcessVertBuffer.GetAddressOf());
	if (FAILED(hr)) {
		log_debug("[DBG] Could not create _barrelEffectVertBuffer");
	}
}

void DeviceResources::InitSpeedParticlesVB()
{
	// The values for rhw_depth and sz_depth were taken from the skybox
	for (int i = 0; i < MAX_SPEED_PARTICLES; i++) {
		float x = (((float)rand() / RAND_MAX) - 0.5f);
		float y = (((float)rand() / RAND_MAX) - 0.5f);
		float z = (((float)rand() / RAND_MAX) - 0.5f);

		x *= g_fSpeedShaderParticleRange;
		y *= g_fSpeedShaderParticleRange;
		z *= g_fSpeedShaderParticleRange;

		//log_debug("[DBG] Init: %0.3f, %0.3f", x, y);
		
		g_SpeedParticles[i].x = x;
		g_SpeedParticles[i].y = y;
		g_SpeedParticles[i].z = z;
		g_SpeedParticles[i].w = 1.0f;
	}
}

void DeviceResources::BuildSpeedVertexBuffer() 
{
	HRESULT hr;
	auto &device = this->_d3dDevice;
	
	/* Create the VertexBuffer if necessary */
	if (this->_speedParticlesVertexBuffer != NULL) 
	{
		this->_speedParticlesVertexBuffer->Release();
		this->_speedParticlesVertexBuffer = NULL;
	}

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; //  D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(D3DTLVERTEX) * MAX_SPEED_PARTICLES * 12;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // 0;
	vertexBufferDesc.MiscFlags = 0;

	InitSpeedParticlesVB();

	//D3D11_SUBRESOURCE_DATA vertexBufferData;
	//ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	//vertexBufferData.pSysMem = &(g_SpeedParticles[0]);
	//hr = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->_speedParticlesVertexBuffer);
	hr = device->CreateBuffer(&vertexBufferDesc, NULL, &this->_speedParticlesVertexBuffer);
	if (FAILED(hr)) {
		log_debug("[DBG] Could not create _speedParticlesVertexBuffer");
	}
}

void DeviceResources::CreateShadowVertexIndexBuffers(D3DTLVERTEX *vertices, WORD *indices, UINT numVertices, UINT numIndices)
{
	HRESULT hr;
	auto &device = this->_d3dDevice;

	if (this->_shadowVertexBuffer != NULL)
	{
		this->_shadowVertexBuffer->Release();
		this->_shadowVertexBuffer = NULL;
	}

	if (this->_shadowIndexBuffer != NULL)
	{
		this->_shadowIndexBuffer->Release();
		this->_shadowIndexBuffer = NULL;
	}

	// Create the vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.ByteWidth = sizeof(D3DTLVERTEX) * numVertices;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	// Use these and set the vertexBufferData to NULL to change the vertices dynamically:
	//vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // D3D11_USAGE_IMMUTABLE
	//vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // 0;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Remove this block when the vertex buffer is dynamic
	D3D11_SUBRESOURCE_DATA vertexBufferData;
	vertexBufferData.pSysMem = vertices;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	if (FAILED(hr = device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->_shadowVertexBuffer))) {
		log_debug("[DBG] [SHW] Could not create _shadowVertexBuffer: 0x%x", hr);
	}
	else
		log_debug("[DBG] [SHW] _shadowVertexBuffer CREATED");

	// Create the index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.ByteWidth = sizeof(WORD) * numIndices;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	// Use the following to change the index buffer dynamically:
	//indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // D3D11_USAGE_IMMUTABLE
	//indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // 0
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Remove the block when the index buffer is dynamic
	D3D11_SUBRESOURCE_DATA indexBufferData;
	indexBufferData.pSysMem = indices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;

	// Use InitIndexBuffer(false) to set the index buffer
	if (FAILED(hr = device->CreateBuffer(&indexBufferDesc, &indexBufferData, &this->_shadowIndexBuffer))) {
		log_debug("[DBG] [SHW] Could not create _shadowIndexBuffer: 0x%x", hr);
	}
	else
		log_debug("[DBG] [SHW] _shadowIndexBuffer CREATED");

	g_ShadowMapping.NumVertices = numVertices;
	g_ShadowMapping.NumIndices = numIndices;
}

/*
void DeviceResources::FillReticleVertexBuffer(float width, float height) {
	HRESULT hr;
	D3DCOLOR color = 0xFFFFFFFF; // AABBGGRR
	auto &device = this->_d3dDevice;
	auto &context = this->_d3dDeviceContext;
	//float depth = g_fHUDDepth;
	
	// The values for rhw_depth and sz_depth were taken from the skybox
	float rhw = 0.000863f; // this is the inverse of the depth (?)
	float sz = 0.001839f;   // this is the Z-buffer value (?)
	
	D3DTLVERTEX ReticleVertices[6] = { 0 };

	ReticleVertices[0].sx = 0;
	ReticleVertices[0].sy = 0;
	ReticleVertices[0].sz = sz;
	ReticleVertices[0].rhw = rhw;
	ReticleVertices[0].tu = 0;
	ReticleVertices[0].tv = 0;
	ReticleVertices[0].color = color;

	ReticleVertices[1].sx = width;
	ReticleVertices[1].sy = 0;
	ReticleVertices[1].sz = sz;
	ReticleVertices[1].rhw = rhw;
	ReticleVertices[1].tu = 1;
	ReticleVertices[1].tv = 0;
	ReticleVertices[1].color = color;

	ReticleVertices[2].sx = width;
	ReticleVertices[2].sy = height;
	ReticleVertices[2].sz = sz;
	ReticleVertices[2].rhw = rhw;
	ReticleVertices[2].tu = 1;
	ReticleVertices[2].tv = 1;
	ReticleVertices[2].color = color;

	ReticleVertices[3].sx = width;
	ReticleVertices[3].sy = height;
	ReticleVertices[3].sz = sz;
	ReticleVertices[3].rhw = rhw;
	ReticleVertices[3].tu = 1;
	ReticleVertices[3].tv = 1;
	ReticleVertices[3].color = color;

	ReticleVertices[4].sx = 0;
	ReticleVertices[4].sy = height;
	ReticleVertices[4].sz = sz;
	ReticleVertices[4].rhw = rhw;
	ReticleVertices[4].tu = 0;
	ReticleVertices[4].tv = 1;
	ReticleVertices[4].color = color;

	ReticleVertices[5].sx = 0;
	ReticleVertices[5].sy = 0;
	ReticleVertices[5].sz = sz;
	ReticleVertices[5].rhw = rhw;
	ReticleVertices[5].tu = 0;
	ReticleVertices[5].tv = 0;
	ReticleVertices[5].color = color;

	D3D11_MAPPED_SUBRESOURCE map;
	hr = context->Map(this->_reticleVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (SUCCEEDED(hr))
	{
		size_t length = sizeof(D3DTLVERTEX) * 6;
		memcpy(map.pData, &(ReticleVertices[0]), length);
		context->Unmap(this->_reticleVertexBuffer, 0);
	}
	else
		log_debug("[DBG] Could not fill _reticleVertexBuffer: 0x%x", hr);
}
*/

/*
void DeviceResources::CreateReticleVertexBuffer()
{
	HRESULT hr;
	auto &device = this->_d3dDevice;

	// Create the VertexBuffer if necessary
	if (this->_reticleVertexBuffer != NULL)
	{
		this->_reticleVertexBuffer->Release();
		this->_reticleVertexBuffer = NULL;
	}

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(D3DTLVERTEX) * 6 * 12;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // 0;
	vertexBufferDesc.MiscFlags = 0;

	hr = device->CreateBuffer(&vertexBufferDesc, NULL, &this->_reticleVertexBuffer);
	if (FAILED(hr)) {
		log_debug("[DBG] Could not create _speedParticlesVertexBuffer");
	}
}
*/

// Sample kernel
/*
std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
std::default_random_engine generator;
std::vector<glm::vec3> ssdoKernel;
for (GLuint i = 0; i < 64; ++i)
{
	// These samples are [-1..1, -1..1, 0..1], so it looks like a hemisphere
	// I think these samples are used to compute the sssdo and bent normal
	glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
	sample = glm::normalize(sample);
	sample *= randomFloats(generator);
	GLfloat scale = GLfloat(i) / 64.0;

	// Scale samples s.t. they're more aligned to center of kernel
	scale = lerp(0.1f, 1.0f, scale * scale);
	sample *= scale;
	ssdoKernel.push_back(sample);
}
*/

void DeviceResources::CreateRandomVectorTexture() {
	/*
	const int NUM_SAMPLES = 64;
	auto& context = this->_d3dDeviceContext;
	auto& device = this->_d3dDevice;
	float rawData[NUM_SAMPLES * 3];
	D3D11_TEXTURE1D_DESC desc = { 0 };
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	D3D11_SUBRESOURCE_DATA textureData = { 0 };
	ComPtr<ID3D11Texture1D> texture = nullptr;
	ComPtr<ID3D11ShaderResourceView> textureSRV = nullptr;

	log_debug("[DBG] Creating random vector texture for SSDO");

	desc.Width = NUM_SAMPLES;
	desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	
	textureData.pSysMem = rawData;
	textureData.SysMemPitch = sizeof(float) * NUM_SAMPLES;
	textureData.SysMemSlicePitch = 0;
	
	//std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	int j = 0;
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		// These samples are [-1..1, -1..1, 0..1], so it looks like a hemisphere
		// I think these samples are used to compute the sssdo and bent normal
		//glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		Vector3 sample;
		sample.x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
		sample.y = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
		sample.x = ((float)rand() / RAND_MAX);
		sample.normalize();
		//sample *= randomFloats(generator);

		float scale = (float )i / (float )NUM_SAMPLES;
		// Scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		rawData[j++] = sample.x;
		rawData[j++] = sample.y;
		rawData[j++] = sample.z;
	}

	HRESULT hr = device->CreateTexture1D(&desc, &textureData, &texture);
	if (FAILED(hr)) {
		log_debug("[DBG] Failed when calling CreateTexture2D on SSDO kernel, reason: 0x%x",
			this->_d3dDevice->GetDeviceRemovedReason());
		goto out;
	}
	
	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	shaderResourceViewDesc.Texture1D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture1D.MipLevels = 1;
	
	hr = device->CreateShaderResourceView(texture, &shaderResourceViewDesc, &textureSRV);
	if (FAILED(hr)) {
		log_debug("[DBG] Failed when calling CreateShaderResourceView on SSDO kernel, reason: 0x%x",
			this->_d3dDevice->GetDeviceRemovedReason());
		goto out;
	}

out:
	// DEBUG
	//hr = DirectX::SaveDDSTextureToFile(context, texture, L"C:\\Temp\\_randomTex.dds");
	//log_debug("[DBG] Dumped randomTex to file, hr: 0x%x", hr);
	// DEBUG
	if (texture != nullptr) texture->Release();
	if (textureSRV != nullptr) textureSRV->Release();
	*/
}

void DeviceResources::DeleteRandomVectorTexture() {
	// TODO
}

void DeviceResources::CreateGrayNoiseTexture() {
	auto& context = this->_d3dDeviceContext;
	auto& device = this->_d3dDevice;
	const int TEX_SIZE = 256;
	const int NUM_SAMPLES = TEX_SIZE * TEX_SIZE;
	float rawData[NUM_SAMPLES];
	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	D3D11_SUBRESOURCE_DATA textureData = { 0 };
	if (_grayNoiseTex != nullptr)
		DeleteGrayNoiseTexture();
	if (g_b3DVisionEnabled && _vision3DSignatureTex != nullptr)
		Delete3DVisionTexture();

	desc.Width = TEX_SIZE;
	desc.Height = TEX_SIZE;
	//desc.Format = DXGI_FORMAT_R8_UINT;
	desc.Format = DXGI_FORMAT_R32_FLOAT;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	textureData.pSysMem = rawData;
	textureData.SysMemPitch = sizeof(uint8_t) * TEX_SIZE;
	textureData.SysMemSlicePitch = 0;

	for (int i = 0; i < NUM_SAMPLES; i++) {
		//float sample = ((float)rand() / RAND_MAX);
		//rawData[i] = (uint8_t)(255 * sample);
		rawData[i] = ((float)rand() / RAND_MAX);
	}

	if (FAILED(hr = device->CreateTexture2D(&desc, &textureData, &_grayNoiseTex))) {
		log_debug("[DBG] [NOISE] Failed when calling CreateTexture2D on gray noise texture, reason: 0x%x",
			this->_d3dDevice->GetDeviceRemovedReason());
		return;
	}

	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	if (FAILED(hr = device->CreateShaderResourceView(_grayNoiseTex, &shaderResourceViewDesc, &_grayNoiseSRV))) {
		log_debug("[DBG] [NOISE] Failed when calling CreateShaderResourceView on gray noiseB, reason: 0x%x",
			this->_d3dDevice->GetDeviceRemovedReason());
		return;
	}

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = this->_useAnisotropy ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxAnisotropy = this->_useAnisotropy ? this->GetMaxAnisotropy() : 1;
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

	if (FAILED(hr = this->_d3dDevice->CreateSamplerState(&samplerDesc, &this->_repeatSamplerState))) {
		log_debug("[DBG] [NOISE] Failed when calling CreateSamplerState on gray noiseB, hr: 0x%x", hr);
		return;
	}

//out:
	// DEBUG
	//hr = DirectX::SaveDDSTextureToFile(context, _grayNoiseTex, L"C:\\Temp\\_grayNoiseTex.dds");
	//log_debug("[DBG] [NOISE] Dumped randomTex to file, hr: 0x%x", hr);
	// DEBUG
	//if (_grayNoiseTex != nullptr) _grayNoiseTex->Release();
	//if (_grayNoiseSRV != nullptr) _grayNoiseSRV->Release();
}

void DeviceResources::DeleteGrayNoiseTexture()
{
	if (_grayNoiseSRV != nullptr) _grayNoiseSRV->Release();
	if (_grayNoiseTex != nullptr) _grayNoiseTex->Release();
	if (_repeatSamplerState != nullptr) _repeatSamplerState.Release();
	_grayNoiseTex = nullptr;
	_grayNoiseSRV = nullptr;
	_repeatSamplerState = nullptr;
}

void DeviceResources::Create3DVisionSignatureTexture() {
	auto& context = this->_d3dDeviceContext;
	auto& device = this->_d3dDevice;
	DWORD dwWidth = this->_backbufferWidth * 2;
	DWORD dwHeight = this->_backbufferHeight;
	const int NUM_SAMPLES = dwWidth * (dwHeight + 1);
	uint32_t *rawData = new uint32_t[NUM_SAMPLES];
	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	D3D11_SUBRESOURCE_DATA textureData = { 0 };
	if (_vision3DSignatureTex != nullptr)
		Delete3DVisionTexture();

	desc.Width = dwWidth;
	desc.Height = dwHeight + 1;
	desc.Format = BACKBUFFER_FORMAT;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	textureData.pSysMem = rawData;
	textureData.SysMemPitch = sizeof(uint32_t) * dwWidth;
	textureData.SysMemSlicePitch = 0;
	uint32_t LastRow = textureData.SysMemPitch * dwHeight;

	// Clear rawData
	memset(rawData, 0x0, NUM_SAMPLES);

	// Add the signature to rawData
	DWORD dwBPP = 32;
	DWORD dwFlags = 0x02; // Side by Side

	uint8_t *ptr;
	uint8_t signature[] = {
		0x4e, 0x56, 0x33, 0x44,	// NVSTEREO_IMAGE_SIGNATURE
		0x00, 0x00, 0x00, 0x00, // Width
		0x00, 0x00, 0x00, 0x00,	// Height
		0x00, 0x00, 0x00, 0x00,	// BPP
		0x00, 0x00, 0x00, 0x00,	// Flags
	};
	ptr = (uint8_t *)&dwWidth;
	for (int i = 0; i < 4; i++)
		signature[4 + i] = ptr[i];

	ptr = (uint8_t *)&dwHeight;
	for (int i = 0; i < 4; i++)
		signature[8 + i] = ptr[i];

	ptr = (uint8_t *)&dwBPP;
	for (int i = 0; i < 4; i++)
		signature[12 + i] = ptr[i];

	ptr = (uint8_t *)&dwFlags;
	for (int i = 0; i < 4; i++)
		signature[16 + i] = ptr[i];

	// DEBUG
	/*
	{
		log_debug("[DBG] [3DV] Create3DVisionSignature resources->backbuffer size: %d, %d, dwWidth,Height: %d, %d",
			this->_backbufferWidth * 2, this->_backbufferHeight, dwWidth, dwHeight);
		int Ofs = 0;
		log_debug("[DBG] [3DV] -----------------------------------");
		for (int j = 0; j < 5; j++, Ofs += 4)
			log_debug("[DBG] [3DV] 0x%02x, 0x%02x, 0x%02x, 0x%02x",
				signature[Ofs], signature[Ofs + 1], signature[Ofs + 2], signature[Ofs + 3]);
		log_debug("[DBG] [3DV] ===================================");
	}
	*/

	// Debug: the following line draws a thin white line on the top of the image. I'm using
	// this to verify that the signature is being added. We need to remove this line later.
	memset(rawData, 0xFF, 10 * dwWidth);
	// Debug: the following line should write a white line on the last row of the image:
	//memset((char *)((uint32_t)map.pData + LastRow), 0xFF, 4 * resources->_backbufferWidth);

	// Add the 3D vision signature to the last row of the image
	memcpy((byte *)((uint32_t)rawData + LastRow), signature, 20);

	// Now create the texture proper with the signature
	if (FAILED(hr = device->CreateTexture2D(&desc, &textureData, &_vision3DSignatureTex))) {
		log_debug("[DBG] [3DV] Failed when calling CreateTexture2D on _vision3DSignatureTex, reason: 0x%x",
			this->_d3dDevice->GetDeviceRemovedReason());
		goto out;
	}

	// Create the SRV for this texture
	shaderResourceViewDesc.Format = desc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	if (FAILED(hr = device->CreateShaderResourceView(_vision3DSignatureTex, &shaderResourceViewDesc, &_vision3DSignatureSRV))) {
		log_debug("[DBG] [3DV] Failed when calling CreateShaderResourceView on 3D vision signature, reason: 0x%x",
			this->_d3dDevice->GetDeviceRemovedReason());
		goto out;
	}

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.MaxAnisotropy = 1;
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

	if (FAILED(hr = this->_d3dDevice->CreateSamplerState(&samplerDesc, &this->_noInterpolationSamplerState))) {
		log_debug("[DBG] [3DV] Failed when calling CreateSamplerState on 3D vision signature, hr: 0x%x", hr);
		goto out;
	}

	// DEBUG
	//hr = DirectX::SaveWICTextureToFile(context, _vision3DSignatureTex, GUID_ContainerFormatPng, L"C:\\Temp\\_vision3DSignature.png");
	//log_debug("[DBG] [3DV] Dumped _vision3DSignature to file, hr: 0x%x", hr);
	// DEBUG
out:
	delete[] rawData;
}

void DeviceResources::Delete3DVisionTexture()
{
	if (_vision3DSignatureSRV != nullptr) _vision3DSignatureSRV->Release();
	if (_vision3DSignatureTex != nullptr) _vision3DSignatureTex->Release();
	if (_noInterpolationSamplerState != nullptr) _noInterpolationSamplerState.Release();
	_vision3DSignatureTex = nullptr;
	_vision3DSignatureSRV = nullptr;
	_noInterpolationSamplerState = nullptr;
}

void DeviceResources::CreateTgSmushTexture(DWORD width, DWORD height) {
	auto& context = this->_d3dDeviceContext;
	auto& device = this->_d3dDevice;
	DWORD dwWidth = width;
	DWORD dwHeight = height;
	const int NUM_SAMPLES = dwWidth * dwHeight;
	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
	D3D11_SUBRESOURCE_DATA textureData = { 0 };

	// If we already have a TgSmush texture with the right dimensions, just bail out.
	if (_tgSmushTex != nullptr && _tgSmushTexWidth == width && _tgSmushTexHeight == height)
		return;

	uint32_t* rawData = new uint32_t[NUM_SAMPLES];

	if (_tgSmushTex != nullptr)
		DeleteTgSmushTexture();

	desc.Width = dwWidth;
	desc.Height = dwHeight;
	desc.Format = BACKBUFFER_FORMAT;
	desc.MiscFlags = 0;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC; // CPU: write-only, GPU: read-only
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	textureData.pSysMem = rawData;
	textureData.SysMemPitch = sizeof(uint32_t) * dwWidth;
	textureData.SysMemSlicePitch = 0;

	// Clear rawData
	memset(rawData, 0xFF, NUM_SAMPLES * 4);
	// Create the texture proper
	if (FAILED(hr = device->CreateTexture2D(&desc, &textureData, &_tgSmushTex))) {
		log_debug("[DBG] [TGS] Failed when calling CreateTexture2D on _tgSmushTex, reason: 0x%x",
			this->_d3dDevice->GetDeviceRemovedReason());
		goto out;
	}
	else
		log_debug("[DBG] [TGS] Successfully created _tgSmushTex");
	_tgSmushTexWidth = width;
	_tgSmushTexHeight = height;

out:
	delete[] rawData;
}


void DeviceResources::DeleteTgSmushTexture()
{
	if (_tgSmushTex != nullptr) _tgSmushTex->Release();
	_tgSmushTex = nullptr;
	_tgSmushTexWidth = _tgSmushTexHeight = -1;
}

void DeviceResources::ClearDynCockpitVector(dc_element DCElements[], int size) {
	for (int i = 0; i < size; i++) {
		if (this->dc_coverTexture[i] != nullptr) {
			log_debug("[DBG] [DC] !!!! Releasing [%d][%s]", i, DCElements[i].coverTextureName);
			this->dc_coverTexture[i]->Release();
			//delete DCElements[i].coverTexture;
			//log_debug("[DBG] [DC] !!!! ref: %d", ref);
			this->dc_coverTexture[i] = nullptr;
		}
		DCElements[i].coverTextureName[0] = 0;
	}
	g_iNumDCElements = 0;
	//DCElements.clear();
}

void DeviceResources::ClearActiveCockpitVector(ac_element ACElements[], int size) {
	ac_element vrKeybElem;
	ac_element vrGloveElements[2];

	// Save the VR Keyboard slot
	if (g_iVRKeyboardSlot != -1)
	{
		vrKeybElem = g_ACElements[g_iVRKeyboardSlot];
	}

	// Save the VR Gloves slots
	for (int i = 0; i < 2; i++)
	{
		if (g_iVRGloveSlot[i] != -1)
		{
			vrGloveElements[i] = g_ACElements[g_iVRGloveSlot[i]];
		}
	}

	for (int i = 0; i < size; i++) {
		int numCoords = ACElements[i].coords.numCoords;
		ACElements[i].name[0] = 0;
		for (int j = 0; j < numCoords; j++) {
			ACElements[i].coords.action[j][0] = 0;
			ACElements[i].coords.action_name[j][0] = 0;
		}
		ACElements[i].coords.numCoords = 0;
	}
	g_iNumACElements = 0;

	// Restore the VR Keyboard slot
	if (g_iVRKeyboardSlot != -1)
	{
		g_iVRKeyboardSlot = g_iNumACElements++;
		g_ACElements[g_iVRKeyboardSlot] = vrKeybElem;
	}

	// Restore the VR Gloves slots
	for (int i = 0; i < 2; i++)
	{
		if (g_iVRGloveSlot[i] != -1)
		{
			g_iVRGloveSlot[i] = g_iNumACElements++;
			g_ACElements[g_iVRGloveSlot[i]] = vrGloveElements[i];
		}
	}
}

void DeviceResources::ResetDynamicCockpit() {
	// Force the recomputation of y_center for the next cockpit
	g_bYCenterHasBeenFixed = false;
	g_bRenderLaserIonEnergyLevels = false;
	g_bRenderThrottle = false;
	g_DcEnergyBarData.Reset();
	if (g_sCurrentCockpit[0] != 0) // Testing the name of the cockpit should prevent multiple resets
	{
		ResetActiveCockpit();
		log_debug("[DBG] [DC] Resetting Dynamic Cockpit");
		// Reset the cockpit name
		g_sCurrentCockpit[0] = 0;
		// Reset the current ship's custom focal length
		g_fCurrentShipFocalLength = 0.0f;
		// Reset the HUD boxes: this will force a re-compute of the boxes and the DC elements
		g_DCHUDRegions.ResetLimits();
		// Reset the Source DC elements so that we know when they get re-computed.
		g_DCElemSrcBoxes.Reset();
		// Reset the active slots in g_DCElements
		for (int i = 0; i < g_iNumDCElements; i++)
		{
			dc_element *elem = &g_DCElements[i];
			if (elem->bActive) {
				if (this->dc_coverTexture[i] != nullptr) {
					//log_debug("[DBG] [DC] Releasing [%d][%s]...", i, elem->coverTextureName);
					this->dc_coverTexture[i]->Release();
					//log_debug("[DBG] [DC] RELEASED");
					this->dc_coverTexture[i] = nullptr;
				}
				elem->bActive = false;
				elem->bNameHasBeenTested = false;
				elem->bHologram = false;
				elem->bNoisyHolo = false;
				elem->bTransparent = false;
				elem->bAlwaysVisible = false;
			}
		}
		// Reset the dynamic cockpit vector
		if (g_iNumDCElements > 0) {
			log_debug("[DBG] [DC] Clearing g_DCElements");
			ClearDynCockpitVector(g_DCElements, g_iNumDCElements);
			g_iNumDCElements = 0;
		}
	}
	// The code that captures text lines seemed to need this, but looks like this is not
	// necessary after all.
	//LoadDCInternalCoordinates();
}

/*
 * This function is called from ResetDynamicCockpit
 */
void DeviceResources::ResetActiveCockpit() {
	if (g_bActiveCockpitEnabled)
	{
		log_debug("[DBG] [AC] Resetting Active Cockpit");
		// Reset the active slots in g_ACElements
		for (int i = 0; i < g_iNumACElements; i++)
		{
			ac_element *elem = &g_ACElements[i];
			elem->bActive = false;
			elem->bNameHasBeenTested = false;
		}

		// Reset the active cockpit vector
		if (g_iNumACElements > 0) {
			log_debug("[DBG] [AC] Clearing g_ACElements");
			ClearActiveCockpitVector(g_ACElements, g_iNumACElements);
			//g_iNumACElements = 0;
		}
		g_vrKeybState.state = KBState::OFF;
	}
}

int DeviceResources::PushExtraTexture(ID3D11ShaderResourceView* srv)
{
	_extraTextures.push_back(srv);
	return _extraTextures.size() - 1;
}

void DeviceResources::ResetExtraTextures()
{
	//for (int i = 0; i < MAX_EXTRA_TEXTURES; i++) {
	//	if (this->_extraTextures[i] != nullptr) {
	//		this->_extraTextures[i]->Release();
	//		this->_extraTextures[i] = nullptr;
	//	}
	//}
	//this->_numExtraTextures = 0;

	for (uint32_t i = 0; i < _extraTextures.size(); i++)
		if (_extraTextures[i] != nullptr) {
			//int count = 0;
			// TODO: Cached SRVs increase the refcounts and rely on this code
			// to release everything. This might not be the proper solution,
			// but it seems to work.
			//do {
			//	count = _extraTextures[i]->Release();
			//} while (count > 0);
			// TODO: Cached SRVs increase the refcounts... what to do about it?
			// The previous code still crashed, so... reverting the code.
			_extraTextures[i]->Release();
			_extraTextures[i] = nullptr;
		}
	_extraTextures.clear();

	// Clear any references to the animated materials as well:
	ClearAnimatedMaterials();
}

HRESULT DeviceResources::OnSizeChanged(HWND hWnd, DWORD dwWidth, DWORD dwHeight)
{
	/*
	 * When the concourse is displayed, dwWidth,dwHeight = 640x480 regardless of actual in-game res or screen res.
	 * When 3D content is displayed, dwWidth,dwHeight = in-game resolution (1280x1024, 1600x1200, etc)
	 */
	HRESULT hr;
	const char* step = "";
	DXGI_FORMAT oldFormat;

	//log_debug("[DBG] OnSizeChanged, dwWidth,Height: %d, %d", dwWidth, dwHeight);

	// Generic VR Initialization
	// Replace the game's WndProc
	if (!g_bWndProcReplaced) {
		ReplaceWindowProc(hWnd);
		g_bWndProcReplaced = true;
	}
	
	if (g_bUseSteamVR) {
		// dwWidth, dwHeight are the in-game's resolution
		// When using SteamVR, let's override the size with the recommended size
		log_debug("[DBG] Original dwWidth, dwHeight: %d, %d", dwWidth, dwHeight);
		dwWidth = g_steamVRWidth;
		dwHeight = g_steamVRHeight;
		log_debug("[DBG] Using SteamVR settings: %u, %u", dwWidth, dwHeight);
	}

	// Reset the present counter and the reticle status
	SetPresentCounter(0, 1);
	g_bPrevPlayerInHangar = false;
	// Reset the FOV application flag
	g_bCustomFOVApplied = false;
	// Force the recomputation of y_center for the next cockpit
	g_bYCenterHasBeenFixed = false;
	// Reset scene variables
	g_SSAO_PSCBuffer.enable_dist_fade = 0.0f;
	g_b3DSunPresent = false;
	g_b3DSkydomePresent = false;
	g_SSAO_PSCBuffer.enable_dist_fade = 0.0f;
	g_bDCApplyEraseRegionCommands = !g_bHUDVisibleOnStartup;
	DCResetSubRegions();
	log_debug("[DBG] Resetting g_b3DSunPresent, g_b3DSkydomePresent");

	g_meshToFGMap.clear(); // OnSizeChanged(), called when a mission is loaded
	g_AuxTextureVector.clear();
	ClearGlobalTextureMap();
	ClearCachedSRVs();
	ClearGroupIdImageIdToTextureMap();
	DeleteRandomVectorTexture();
	ResetXWALightInfo();
	g_HiResTimer.ResetGlobalTime();
	ResetGameEvent();
	ResetObjectIndexMap();
	ReloadInterdictionMap();
	//ResetRawMouseInput();
	if (IsZIPReaderLoaded() && g_bCleanupZIPDirs)
		DeleteAllTempZIPDirectories();
	this->ResetExtraTextures();
	this->_depthStencilViewL.Release();
	this->_depthStencilViewR.Release();
	this->_depthStencilL.Release();
	this->_depthStencilR.Release();
	if (g_bUseSteamVR && g_config.HDConcourseEnabled &&
		g_config.D3dRendererTexturesHookEnabled)
	{
		if (this->_depthStencilHd)
		{
			this->_depthStencilHd.Release();
			this->_depthStencilHd = nullptr;
		}
		if (this->_depthStencilViewHd)
		{
			this->_depthStencilViewHd.Release();
			this->_depthStencilViewHd = nullptr;
		}
	}
	this->_d2d1RenderTarget.Release();
	this->_d2d1OffscreenRenderTarget.Release();
	if (g_EnhancedHUDData.Enabled)
		_d2d1EnhancedHUDRenderTarget.Release();
	this->_d2d1DCRenderTarget.Release();
	this->_renderTargetView.Release();
	if (g_bUseSteamVR)
		this->_renderTargetViewHd.Release();
	this->_renderTargetViewPost.Release();
	this->_offscreenAsInputShaderResourceView.Release();
	this->_backgroundBufferSRV.Release();
	this->_backgroundAuxBufferSRV.Release();
	this->_transp1SRV.Release();
	this->_transp2SRV.Release();
	//if (g_bUseTextureCube) this->_textureCubeSRV.Release();
	this->_offscreenBuffer.Release();
	this->_offscreenBufferHdBackground.Release();
	this->_offscreenBufferAsInput.Release();
	this->_offscreenBufferPost.Release();
	this->_backgroundBuffer.Release();
	this->_backgroundAuxBuffer.Release();
	this->_backgroundBufferAsInput.Release();
	this->_transpBuffer1.Release();
	this->_transpBuffer2.Release();
	this->_transpBufferAsInput1.Release();
	this->_transpBufferAsInput2.Release();
	//if (g_bUseTextureCube) this->_textureCube.Release();
	if (this->_useMultisampling)
		this->_shadertoyBufMSAA.Release();
	this->_shadertoyBuf.Release();
	this->_shadertoyAuxBuf.Release();
	this->_shadertoyRTV.Release();
	this->_shadertoySRV.Release();
	this->_shadertoyAuxSRV.Release();
	if (g_EnhancedHUDData.Enabled)
	{
		this->_enhancedHUDBuffer.Release();
		this->_enhancedHUDSRV.Release();
	}

	/*
	if (this->_HUDVertexBuffer)
	{
		this->_HUDVertexBuffer.Release();
		this->_HUDVertexBuffer = NULL;
	}
	if (this->_clearHUDVertexBuffer)
	{
		this->_clearHUDVertexBuffer.Release();
		this->_clearHUDVertexBuffer = NULL;
	}
	if (this->_hyperspaceVertexBuffer)
	{
		this->_hyperspaceVertexBuffer.Release();
		this->_hyperspaceVertexBuffer = NULL;
	}
	if (this->_postProcessVertBuffer != NULL) {
		this->_postProcessVertBuffer->Release();
		this->_postProcessVertBuffer = NULL;
	}
	if (this->_speedParticlesVertexBuffer != NULL)
	{
		this->_speedParticlesVertexBuffer->Release();
		this->_speedParticlesVertexBuffer = NULL;
	}
	if (_grayNoiseTex != nullptr)
		DeleteGrayNoiseTexture();
	*/

	if (g_bUseSteamVR) {
		this->_offscreenBufferHd.Release();
		this->_offscreenBufferR.Release();
		this->_offscreenBufferAsInputR.Release();
		this->_offscreenBufferPostR.Release();
		this->_offscreenBufferHdSRV.Release();
		this->_offscreenAsInputShaderResourceViewR.Release();
		this->_renderTargetViewR.Release();
		this->_renderTargetViewPostR.Release();
		this->_steamVRPresentBuffer.Release();
		this->_steamVROverlayBuffer.Release();
		this->_renderTargetViewSteamVRResize.Release();
		this->_renderTargetViewSteamVROverlayResize.Release();
		if (this->_useMultisampling)
			this->_shadertoyBufMSAA_R.Release();
		this->_shadertoyBufR.Release();
		this->_shadertoyRTV_R.Release();
		this->_shadertoySRV_R.Release();
	}
	if (g_b3DVisionEnabled) {
		log_debug("[DBG] [3DV] Releasing 3D vision buffers");
		this->_RTVvision3DPost.Release();
		this->_vision3DPost.Release();
		this->_vision3DNoMSAA.Release();
		this->_vision3DStaging.Release();

		this->_RTVvision3DPost = nullptr;
		this->_vision3DPost = nullptr;
		this->_vision3DNoMSAA = nullptr;
		this->_vision3DStaging = nullptr;
	}

	this->_DCTextMSAA.Release();
	if (g_bReshadeEnabled) {
		ResetDynamicCockpit();
		this->_renderTargetViewDynCockpit.Release();
		this->_renderTargetViewDynCockpitBG.Release();
		this->_renderTargetViewDynCockpitAsInput.Release();
		this->_renderTargetViewDynCockpitAsInputBG.Release();
		this->_offscreenBufferDynCockpit.Release();
		this->_offscreenBufferDynCockpitBG.Release();
		this->_offscreenAsInputDynCockpit.Release();
		this->_offscreenAsInputDynCockpitBG.Release();
		this->_offscreenAsInputDynCockpitSRV.Release();
		this->_offscreenAsInputDynCockpitBG_SRV.Release();
		this->_DCTextAsInput.Release();
		this->_DCTextSRV.Release();
		this->_DCTextRTV.Release();
		this->_DCTextAsInputRTV.Release();
	}

	{
		this->_ReticleBufMSAA.Release();
		this->_ReticleBufAsInput.Release();
		this->_ReticleRTV.Release();
		this->_ReticleSRV.Release();
	}

	if (g_bActiveCockpitEnabled) {
		ClearActiveCockpitVector(g_ACElements, g_iNumACElements);
	}

	// offscreenBufferBloomMask/AsInputBloomMask/SRV/RTVs are used for SSDO (and SSAO?)
	// _bloomOutput1 is used as temp buff during SSDO
	if (g_bReshadeEnabled) {
		this->_offscreenBufferBloomMask.Release();
		this->_offscreenBufferAsInputBloomMask.Release();
		this->_offscreenAsInputBloomMaskSRV.Release();
		this->_renderTargetViewBloomMask.Release();
		this->_inputBloomMaskRTV.Release();
		this->_bloomOutput1.Release();
		this->_bloomOutput1SRV.Release();

		this->_ssaoMask.Release();
		this->_ssMask.Release();
		//this->_ssEmissionMask.Release();
		this->_normBuf.Release();

		this->_ssaoMaskSRV.Release();
		this->_ssMaskSRV.Release();
		this->_normBufSRV.Release();
		//this->_ssEmissionMaskSRV.Release();
		if (this->_useMultisampling) {
			this->_ssaoMaskMSAA.Release();
			this->_ssMaskMSAA.Release();
			this->_normBufMSAA.Release();
		}

		this->_renderTargetViewNormBuf.Release();
		this->_renderTargetViewSSAOMask.Release();
		this->_renderTargetViewSSMask.Release();
		this->_transp1RTV.Release();
		this->_transp2RTV.Release();
		this->_backgroundRTV.Release();
		//this->_renderTargetViewEmissionMask.Release();
		if (g_bUseSteamVR) {
			this->_ssaoMaskR.Release();
			//this->_ssEmissionMaskR.Release();
			this->_ssMaskR.Release();
			this->_normBufR.Release();
			if (this->_useMultisampling) {
				this->_ssaoMaskMSAA_R.Release();
				this->_ssMaskMSAA_R.Release();
				this->_normBufMSAA_R.Release();
			}

			this->_ssaoMaskSRV_R.Release();
			this->_ssMaskSRV_R.Release();
			this->_normBufSRV_R.Release();
			//this->_ssEmissionMaskSRV_R.Release();

			this->_renderTargetViewNormBufR.Release();
			//this->_renderTargetViewEmissionMaskR.Release();
		}
		ClearOPTnames();
		ClearCraftMaterials();
	}

	if (g_bBloomEnabled) {
		//this->_offscreenAsInputBloomMaskSRV.Release();
		//this->_renderTargetViewBloomMask.Release();
		this->_renderTargetViewBloom1.Release();
		this->_renderTargetViewBloom2.Release();
		this->_renderTargetViewBloomSum.Release();
		//this->_bloomOutput1.Release();
		this->_bloomOutput2.Release();
		this->_bloomOutputSum.Release();
		//this->_bloomOutput1SRV.Release();
		this->_bloomOutput2SRV.Release();
		this->_bloomOutputSumSRV.Release();
		if (g_bUseSteamVR) {
			/*this->_offscreenBufferBloomMaskR.Release();
			this->_offscreenBufferAsInputBloomMaskR.Release();
			this->_offscreenAsInputBloomMaskSRV_R.Release();
			this->_renderTargetViewBloomMaskR.Release();*/
			//this->_bloomOutput1R.Release();
			//this->_bloomOutput2R.Release();
			//this->_bloomOutputSumR.Release();
			//this->_renderTargetViewBloom1R.Release();
			//this->_renderTargetViewBloom2R.Release();
			//this->_renderTargetViewBloomSumR.Release();
			//this->_bloomOutput1SRV_R.Release();
			//this->_bloomOutput2SRV_R.Release();
			//this->_bloomOutputSumSRV_R.Release();
		}
	}

	if (g_bAOEnabled) {
		this->_depthBuf.Release();
		this->_depthBufAsInput.Release();
		
		//this->_bentBuf.Release();
		//this->_bentBufR.Release(); // bentBufR is used to hold a copy of bentBuf to blur it (and viceversa)
		this->_ssaoBuf.Release();
		this->_ssaoBufR.Release(); // ssaoBufR will be used to store SSDO Indirect (and viceversa)
		this->_renderTargetViewDepthBuf.Release();
		this->_depthBufSRV.Release();
		//this->_bentBufSRV.Release();
		//this->_bentBufSRV_R.Release();
		//this->_renderTargetViewBentBuf.Release();
		//this->_renderTargetViewSSAO.Release();
		this->_renderTargetViewSSAO_R.Release();
		this->_ssaoBufSRV.Release();
		this->_ssaoBufSRV_R.Release();
		if (g_bUseSteamVR) {
			this->_depthBufR.Release();
			this->_depthBufAsInputR.Release();
			this->_renderTargetViewDepthBufR.Release();
			this->_depthBufSRV_R.Release();
			//this->_renderTargetViewBentBufR.Release();
		}
	}

	if (g_ShadowMapping.bEnabled) 
	{
		this->_shadowMap.Release();
		this->_shadowMapDebug.Release();
		this->_shadowMapArraySRV.Release();
		this->_shadowMapDSV.Release();
		this->_shadowMapArray.Release();
		if (g_ShadowMapping.bCSMEnabled) {
			this->_csmMap.Release();
			this->_csmMapDSV.Release();
			this->_csmArray.Release();
			this->_csmArraySRV.Release();
		}
	}

	if (g_bRTEnabled)
	{
		if (g_bRTEnableSoftShadows)
		{
			this->_rtShadowMask.Release();
			this->_rtShadowMaskRTV.Release();
			this->_rtShadowMaskSRV.Release();
			if (g_bUseSteamVR)
			{
				this->_rtShadowMaskR.Release();
				this->_rtShadowMaskRTV_R.Release();
				this->_rtShadowMaskSRV_R.Release();
			}
		}

		// This path is only hit when we're *not* in the Tech Room
		ClearGlobalLBVHMap();
		g_iRTTotalBLASNodesInFrame = g_iRTMaxBLASNodesSoFar = g_iRTMaxTLASNodesSoFar = 0;
		g_iRTMaxMeshesSoFar = 0;
		g_bRTReAllocateBvhBuffer = false;
		if (this->_RTBvh != nullptr)
		{
			this->_RTBvh->Release();
			this->_RTBvh = nullptr;
		}
		if (this->_RTTLASBvh != nullptr)
		{
			this->_RTTLASBvh->Release();
			this->_RTTLASBvh = nullptr;
		}
		if (this->_RTMatrices != nullptr)
		{
			this->_RTMatrices->Release();
			this->_RTMatrices = nullptr;
		}

		if (this->_RTBvhSRV != nullptr)
		{
			this->_RTBvhSRV.Release();
			this->_RTBvhSRV = nullptr;
		}
		if (this->_RTTLASBvhSRV != nullptr)
		{
			this->_RTTLASBvhSRV->Release();
			this->_RTTLASBvhSRV = nullptr;
		}
		if (this->_RTMatricesSRV != nullptr)
		{
			this->_RTMatricesSRV.Release();
			this->_RTMatricesSRV = nullptr;
		}
	}

	this->_backBuffer.Release();
	if (g_b3DVisionEnabled && g_b3DVisionForceFullScreen) {
		// Exit Full-screen mode if necessary before releasing the swapchain
		IDXGIOutput *pTarget = nullptr;
		BOOL bFullScreen = FALSE;
		if (this->_swapChain != nullptr && SUCCEEDED(this->_swapChain->GetFullscreenState(&bFullScreen, &pTarget))) {
			if (bFullScreen)
				this->_swapChain->SetFullscreenState(FALSE, NULL);
			pTarget->Release();
		}
	}
	this->_swapChain.Release();
	this->_swapChain = nullptr;

	this->_refreshRate = { 0, 1 };

	if (hWnd != nullptr)
	{
		step = "RenderTarget SwapChain";
		ComPtr<IDXGIDevice> dxgiDevice;
		ComPtr<IDXGIAdapter> dxgiAdapter;
		ComPtr<IDXGIOutput> dxgiOutput;

		hr = this->_d3dDevice.As(&dxgiDevice);

		if (SUCCEEDED(hr))
		{
			hr = dxgiDevice->GetAdapter(&dxgiAdapter);

			if (SUCCEEDED(hr))
			{
				hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
			}
		}

		DXGI_MODE_DESC md{};

		if (SUCCEEDED(hr))
		{
			DWM_TIMING_INFO timingInfo{};
			timingInfo.cbSize = sizeof(DWM_TIMING_INFO);
			DwmGetCompositionTimingInfo(nullptr, &timingInfo);

			md.Format = BACKBUFFER_FORMAT;
			md.RefreshRate.Numerator = timingInfo.rateCompose.uiNumerator;
			md.RefreshRate.Denominator = timingInfo.rateCompose.uiDenominator;

			hr = dxgiOutput->FindClosestMatchingMode(&md, &md, nullptr);
		}

		if (SUCCEEDED(hr))
		{
			DXGI_SWAP_CHAIN_DESC sd{};
			sd.BufferCount = 2;
			sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL; // ORIGINAL = 0x1
			//sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			//sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			sd.BufferDesc.Width  = g_bUseSteamVR ? g_steamVRWidth : 0;
			sd.BufferDesc.Height = g_bUseSteamVR ? g_steamVRHeight : 0;
			/*
			In the "StereoTest" program from here:

			https://gist.github.com/AvengerDr/6062614
			
			* The swap chain desc is 1920x1080 (regular size)
			* The backbuffer is also regular size (it's created from the swap chain)
			* The stereo texture is double-wide with the 3D vision signature.

			On each frame, the stereo texture is copied over the backbuffer using CopySubresourceRegion,
			even if it doesn't actually fit. The nVidia driver is supposed to catch this operation and do
			its magic.
			*/
			sd.BufferDesc.Format = BACKBUFFER_FORMAT;
			sd.BufferDesc.RefreshRate = md.RefreshRate;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = hWnd;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.Windowed = TRUE;
			log_debug("[DBG] SwapChain W,H: %d, %d", sd.BufferDesc.Width, sd.BufferDesc.Height);

			ComPtr<IDXGIFactory> dxgiFactory;
			hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

			if (SUCCEEDED(hr))
			{
				hr = dxgiFactory->CreateSwapChain(this->_d3dDevice, &sd, &this->_swapChain);
			}
			
			if (SUCCEEDED(hr))
			{
				this->_refreshRate = sd.BufferDesc.RefreshRate;
			}

			if (SUCCEEDED(hr))
			{
				DXGI_SWAP_CHAIN_DESC sd{};
				if (g_b3DVisionEnabled && g_b3DVisionForceFullScreen)
					this->_swapChain->SetFullscreenState(TRUE, NULL);

				this->_swapChain->GetDesc(&sd);
				//sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL; // I believe this is the original setting
				//sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
				log_debug("[DBG] SwapEffect: 0x%x", sd.SwapEffect);
				g_FullScreenWidth = sd.BufferDesc.Width;
				g_FullScreenHeight = sd.BufferDesc.Height;
				g_fCurScreenWidth = (float)sd.BufferDesc.Width;
				g_fCurScreenHeight = (float)sd.BufferDesc.Height;
				g_fCurScreenWidthRcp  = 1.0f / g_fCurScreenWidth;
				g_fCurScreenHeightRcp = 1.0f / g_fCurScreenHeight;
				log_debug("[DBG] g_fCurScreen W/H: %0.1f, %0.1f", g_fCurScreenWidth, g_fCurScreenHeight);
			}
		}

		if (SUCCEEDED(hr))
		{
			log_debug("[DBG] Getting _backBuffer from the _swapChain");
			hr = this->_swapChain->GetBuffer(0, IID_PPV_ARGS(&this->_backBuffer));
		}
	}
	else
	{
		step = "RenderTarget Texture2D";

		CD3D11_TEXTURE2D_DESC backBufferDesc(
			BACKBUFFER_FORMAT,
			dwWidth,
			dwHeight,
			1,
			1,
			D3D11_BIND_RENDER_TARGET,
			D3D11_USAGE_DEFAULT,
			0,
			1,
			0,
			0);

		log_debug("[DBG] backbuffer input W,H: %u, %u", dwWidth, dwHeight);
		hr = this->_d3dDevice->CreateTexture2D(&backBufferDesc, nullptr, &this->_backBuffer);
	}

	if (SUCCEEDED(hr))
	{
		D3D11_TEXTURE2D_DESC backBufferDesc;
		this->_backBuffer->GetDesc(&backBufferDesc);

		this->_backbufferWidth = backBufferDesc.Width;
		this->_backbufferHeight = backBufferDesc.Height;
		log_debug("[DBG] backbuffer actual W,H: %u, %u", this->_backbufferWidth, this->_backbufferHeight);
		//g_fCurScreenWidth = _backbufferWidth;
		//g_fCurScreenHeight = _backbufferHeight;
		//log_debug("[DBG] Screen size: %f, %f", g_fCurScreenWidth, g_fCurScreenHeight);
	}

	if (SUCCEEDED(hr))
	{
		CD3D11_TEXTURE2D_DESC desc(
			BACKBUFFER_FORMAT,
			this->_backbufferWidth,
			this->_backbufferHeight,
			g_bUseSteamVR ? 2 : 1, // If we want to render in single-pass instanced stereo, we need Texture2DArray with 1 slice per eye
			1,
			D3D11_BIND_RENDER_TARGET,
			D3D11_USAGE_DEFAULT,
			0,
			this->_sampleDesc.Count,
			this->_sampleDesc.Quality,
			0);
		oldFormat = desc.Format;
		log_debug("[DBG] [MSAA] Count: %d, Quality: %d, Use MSAA: %d",
			this->_sampleDesc.Count, this->_sampleDesc.Quality, this->_useMultisampling);
		//log_debug("[DBG] [MSAA] STD MSAA Q LEVEL: %d", D3D11_STANDARD_MULTISAMPLE_PATTERN);
		log_debug("[DBG] (in-game) display W,H: %d, %d", this->_displayWidth, this->_displayHeight);
		log_debug("[DBG] backbuffer W,H: %d, %d", this->_backbufferWidth, this->_backbufferHeight);

		// MSAA Buffers
		{
			step = "_offscreenBuffer";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBuffer);
			log_debug("[DBG] _offscreenBuffer: %u, %u", desc.Width, desc.Height);
			if (FAILED(hr)) {
				log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				log_err_desc(step, hWnd, hr, desc);
				goto out;
			}

			step = "_offscreenBufferPost";
			// offscreenBufferPost should be just like offscreenBuffer because it will be bound as a renderTarget
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferPost);
			if (FAILED(hr)) {
				log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				log_err_desc(step, hWnd, hr, desc);
				goto out;
			}

			{
				CD3D11_TEXTURE2D_DESC tmp = desc;
				if (g_bUseSteamVR)
				{
					desc.Width     = HD_CONCOURSE_WIDTH;
					desc.Height    = HD_CONCOURSE_HEIGHT;
					desc.ArraySize = 1;
					desc.SampleDesc.Count = 1;
					desc.SampleDesc.Quality = 0;
				}
				step = "OffscreenBufferHdBackground";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferHdBackground);
				log_debug("[DBG] OffscreenBufferHdBackground: %u, %u", desc.Width, desc.Height);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				desc = tmp;
			}

			step = "Background";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_backgroundBuffer);
			if (FAILED(hr))
				goto out;

			step = "Transparency1";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_transpBuffer1);
			if (FAILED(hr))
				goto out;

			step = "Transparency2";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_transpBuffer2);
			if (FAILED(hr))
				goto out;

			if (g_bUseSteamVR)
			{
				// Special buffer used in VR mode. This buffer does *not* hold 2 images on purpose.
				// The HD Concourse is rendered through the D2D1 interface, which only works on
				// regular single-slot buffers.
				{
					CD3D11_TEXTURE2D_DESC tmp = desc;
					desc.ArraySize          = 1;
					desc.Width              = HD_CONCOURSE_WIDTH;
					desc.Height             = HD_CONCOURSE_HEIGHT;
					desc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
					desc.SampleDesc.Count   = 1;
					desc.SampleDesc.Quality = 0;
					step = "_offscreenBufferHd";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferHd);
					if (FAILED(hr)) goto out;
					desc = tmp;
					log_debug("[DBG] Created _offscreenBufferHd");
				}

				step = "_offscreenBufferR";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferR);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				step = "_offscreenBufferPostR";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferPostR);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
			}

			// Some of the buffers will not be rendered to in stereo
			desc.ArraySize = 1;
			if (g_b3DVisionEnabled) {
				step = "_vision3DPost";
				desc.Width *= 2;
				desc.Height++;
				// offscreenBufferPost should be just like offscreenBuffer because it will be bound as a renderTarget
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_vision3DPost);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				log_debug("[DBG] [3DV] _vision3DPost created, size: %u, %u", desc.Width, desc.Height);
				desc.Width /= 2;
				desc.Height--;
			}

			step = "_DCTextMSAA";
			// _DCTextMSAA should be just like offscreenBuffer because it will be used as a renderTarget
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_DCTextMSAA);
			if (FAILED(hr)) {
				log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				log_err_desc(step, hWnd, hr, desc);
				goto out;
			}

			// ReticleBufMSAA
			{
				step = "_ReticleBufMSAA";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ReticleBufMSAA);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
			}

			if (g_bReshadeEnabled) {
				step = "_offscreenBufferDynCockpit";
				// _offscreenBufferDynCockpit should be just like offscreenBuffer because it will be used as a renderTarget
				// But it won't need to be an array since it will not be rendered in stereo.
				desc.ArraySize = 1;				
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferDynCockpit);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				step = "_offscreenBufferDynCockpitBG";
				// _offscreenBufferDynCockpit should be just like offscreenBuffer because it will be used as a renderTarget
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferDynCockpitBG);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
			}

			if (g_bUseSteamVR) {
				step = "_steamVRPresentBuffer";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_steamVRPresentBuffer);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				step = "_steamVRPOverlayBuffer";
				// This buffer will contain the Concourse, loading screen menu and ESC menu with the right aspect ratio (4:3) to show in VR overlay.
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_steamVROverlayBuffer);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
			}

			// Shading System, Bloom Mask, Normals Buffer, etc
			// The auxiliary shading buffers need to be in stereo
			desc.ArraySize = g_bUseSteamVR ? 2 : 1;

			if (g_bReshadeEnabled) {
				step = "_offscreenBufferBloomMask";
				// _offscreenBufferReshade should be just like offscreenBuffer because it will be used as a renderTarget
				desc.Format = BLOOM_BUFFER_FORMAT;
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferBloomMask);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				// Create ssaoMaskMSAA, ssMaskMSAA and normBufMSSA
				if (this->_useMultisampling) {
					desc.Format = AO_MASK_FORMAT;
					step = "_ssaoMaskMSAA";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssaoMaskMSAA);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}

					step = "_ssMaskMSAA";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssMaskMSAA);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}

					desc.Format = AO_DEPTH_BUFFER_FORMAT;
					desc.MipLevels = 1; // 4;
					step = "_normBufMSAA";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_normBufMSAA);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
				}

				if (g_bUseSteamVR) {
					// Create ssaoMaskMSAA_R, ssMaskMSAA_R and normBufMSSA_R
					if (this->_useMultisampling) {
						desc.Format = AO_MASK_FORMAT;
						step = "_ssaoMaskMSAA_R";
						hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssaoMaskMSAA_R);
						if (FAILED(hr)) {
							log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
							log_err_desc(step, hWnd, hr, desc);
							goto out;
						}

						step = "_ssMaskMSAA_R";
						hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssMaskMSAA_R);
						if (FAILED(hr)) {
							log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
							log_err_desc(step, hWnd, hr, desc);
							goto out;
						}

						desc.Format = AO_DEPTH_BUFFER_FORMAT;
						desc.MipLevels = 1; // 4;
						step = "_normBufMSAA_R";
						hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_normBufMSAA_R);
						if (FAILED(hr)) {
							log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
							log_err_desc(step, hWnd, hr, desc);
							goto out;
						}
					}
				}

				desc.Format = oldFormat;
			}

			if (g_bAOEnabled) {
				desc.Format = AO_DEPTH_BUFFER_FORMAT;
				desc.MipLevels = 1; // 4;

				// _depthBuf will be used as a renderTarget
				step = "_depthBuf";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_depthBuf);
				//log_debug("[DBG] [MSAA] depthBuf, samples: %d, q: %d", desc.SampleDesc.Count, desc.SampleDesc.Quality);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				if (g_bUseSteamVR) {
					// _depthBuf should be just like offscreenBuffer because it will be used as a renderTarget
					step = "_depthBufR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_depthBufR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
				}

				desc.Format = oldFormat;
				desc.MipLevels = 1;
			}

			// shadertoyMSAA
			if (this->_useMultisampling) {
				DXGI_FORMAT oldFormat = desc.Format;

				desc.Format = BACKBUFFER_FORMAT;
				step = "_shadertoyBufMSAA";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_shadertoyBufMSAA);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				if (g_bUseSteamVR) {
					step = "_shadertoyBufMSAA_R";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_shadertoyBufMSAA_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
				}

				desc.Format = oldFormat;
			}
		}

		// No MSAA after this point
		{
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
#ifdef GENMIPMAPS
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			desc.MipLevels = 0; // MAX_MIP_LEVELS
#endif

			// offscreenBufferAsInput must not have MSAA enabled since it will be used as input for the barrel shader.
			step = "offscreenBufferAsInput";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferAsInput);
			if (FAILED(hr)) {
				log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				log_err_desc(step, hWnd, hr, desc);
				goto out;
			}

			if (g_bUseSteamVR) {
				step = "_offscreenBufferAsInputR";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferAsInputR);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
			}

			step = "backgroundBufferAsInput";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_backgroundBufferAsInput);
			if (FAILED(hr))
				goto out;

			step = "_backgroundAuxBuffer";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_backgroundAuxBuffer);
			if (FAILED(hr))
				goto out;

			step = "transpBufferAsInput1";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_transpBufferAsInput1);
			if (FAILED(hr))
				goto out;

			step = "transpBufferAsInput2";
			hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_transpBufferAsInput2);
			if (FAILED(hr))
				goto out;

			// Some missions don't have a starfield (!) so we're going to have to load
			// a default starfield for those missions just in case.
			if (_textureCubeSRV == nullptr)
			{
				CD3D11_TEXTURE2D_DESC tmpDesc = desc;

				/*
				desc.ArraySize = 6;
				desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
				desc.MipLevels = 1;
				desc.Width = desc.Height = 2048;

				step = "textureCube";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_textureCube);
				if (FAILED(hr))
					goto out;
				*/

				//HRESULT res = DirectX::CreateDDSTextureFromFile(this->_d3dDevice, L"C:\\temp\\skymap.dds", (ID3D11Resource **)&_textureCube, &_textureCubeSRV);
				//HRESULT res = DirectX::CreateDDSTextureFromFile(this->_d3dDevice, L".\\Effects\\DefaultStarfield.dds", NULL, &_textureCubeSRV);
				// For CubeMaps, this is a bit of a hack. We're still creating the default cube map from DefaultStarfield.dds,
				// but we added the _textureCube reference so that we have the cubemap buffer (not just the SRV). Then, we later
				// overwrite the faces of the cube with textures loaded from disk. We should fix this later and create a buffer/SRV
				// just for cubemaps.
				HRESULT res = DirectX::CreateDDSTextureFromFile(this->_d3dDevice, L".\\Effects\\DefaultStarfield.dds", (ID3D11Resource **)&_textureCube, &_textureCubeSRV);
				if (SUCCEEDED(res))
					log_debug("[DBG] [CUBE] Loaded DefaultStarfield.dds");
				else
					log_debug("[DBG] [CUBE] ERROR 0x%d, could not load DefaultStarfield.dds", res);
				desc = tmpDesc;
			}

			// ReticleBufAsInput
			// Not rendered in stereo.
			desc.ArraySize = 1;
			{
				step = "_ReticleBufAsInput";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ReticleBufAsInput);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
			}
			desc.ArraySize = g_bUseSteamVR ? 2 : 1;

			if (g_bReshadeEnabled) {
				auto tmp = desc;
				desc.Format = BLOOM_BUFFER_FORMAT;
				desc.BindFlags |= D3D11_BIND_RENDER_TARGET; // SRV + RTV;
				step = "_offscreenBufferAsInputBloomMask";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenBufferAsInputBloomMask);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				desc = tmp;

				desc.Format = oldFormat;
			}

			// bloomOutput1, shadertoyBuf
			{
				// These guys should be the last to be created because they modify the BindFlags to
				// add D3D11_BIND_RENDER_TARGET
				UINT curFlags = desc.BindFlags;
				desc.Format = BLOOM_BUFFER_FORMAT;
				desc.BindFlags |= D3D11_BIND_RENDER_TARGET; // SRV + RTV
				log_err("Added D3D11_BIND_RENDER_TARGET flag\n");
				log_err("Flags: 0x%x\n", desc.BindFlags);

				step = "_bloomOutput1";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bloomOutput1);
				if (FAILED(hr)) {
					log_err("Failed to create _bloomOutput1\n");
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				
				desc.Format = BACKBUFFER_FORMAT;
				step = "_shadertoyBuf";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_shadertoyBuf);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				step = "_shadertoyAuxBuf";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_shadertoyAuxBuf);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				if (g_bUseSteamVR) {
					step = "_shadertoyBufR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_shadertoyBufR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
				}

				desc.Format = oldFormat;
			}

			if (g_bBloomEnabled) {
				// These guys should be the last to be created because they modify the BindFlags to
				// add D3D11_BIND_RENDER_TARGET
				desc.Format = BLOOM_BUFFER_FORMAT;
				UINT curFlags = desc.BindFlags;
				desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				log_err("Added D3D11_BIND_RENDER_TARGET flag\n");
				log_err("Flags: 0x%x\n", desc.BindFlags);

				step = "_bloomOutput2";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bloomOutput2);
				if (FAILED(hr)) {
					log_err("Failed to create _bloomOutput2\n");
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				else {
					log_err("Successfully created _bloomOutput2 with combined flags\n");
				}

				step = "_bloomOutputSum";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bloomOutputSum);
				if (FAILED(hr)) {
					log_err("Failed to create _bloomOutputSum\n");
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				else {
					log_err("Successfully created _bloomOutputSum with combined flags\n");
				}

				//if (g_bUseSteamVR) {
				//	step = "_bloomOutput1R";
				//	hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bloomOutput1R);
				//	if (FAILED(hr)) {
				//		log_err("Failed to create _bloomOutput1R\n");
				//		log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
				//		log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				//		log_err_desc(step, hWnd, hr, desc);
				//		goto out;
				//	}
				//	else {
				//		log_err("Successfully created _bloomOutput1R with combined flags\n");
				//	}

				//	step = "_bloomOutput2R";
				//	hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bloomOutput2R);
				//	if (FAILED(hr)) {
				//		log_err("Failed to create _bloomOutput2R\n");
				//		log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
				//		log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				//		log_err_desc(step, hWnd, hr, desc);
				//		goto out;
				//	}
				//	else {
				//		log_err("Successfully created _bloomOutput2R with combined flags\n");
				//	}

				//	step = "_bloomOutputSumR";
				//	hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bloomOutputSumR);
				//	if (FAILED(hr)) {
				//		log_err("Failed to create _bloomOutputSumR\n");
				//		log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
				//		log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				//		log_err_desc(step, hWnd, hr, desc);
				//		goto out;
				//	}
				//	else {
				//		log_err("Successfully created _bloomOutputSumR with combined flags\n");
				//	}
				//}

				// Restore the previous bind flags, just in case there is a dependency on these later on
				desc.BindFlags = curFlags;
				// Restore the non-float format
				desc.Format = oldFormat;
			}

			// Create the DC Input Buffers
			// No need for stereo in dynamic cockpit buffers
			desc.ArraySize = 1;
			if (g_bReshadeEnabled) {
				// This guy should be the last one to be created because it modifies the BindFlags
				// _offscreenBufferAsInputDynCockpit should have the same properties as _offscreenBufferAsInput
				UINT curFlags = desc.BindFlags;
				desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				log_err("_offscreenBufferAsInputDynCockpit: Added D3D11_BIND_RENDER_TARGET flag\n");
				log_err("Flags: 0x%x\n", desc.BindFlags);

				step = "_offscreenBufferAsInputDynCockpit";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenAsInputDynCockpit);
				if (FAILED(hr)) {
					log_err("Failed to create _offscreenBufferAsInputDynCockpit, error: 0x%x\n", hr);
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				else {
					log_err("Successfully created _offscreenBufferAsInputDynCockpit with combined flags\n");
				}

				step = "_offscreenBufferAsInputDynCockpitBG";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_offscreenAsInputDynCockpitBG);
				if (FAILED(hr)) {
					log_err("Failed to create _offscreenBufferAsInputDynCockpitBG, error: 0x%x\n", hr);
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				else {
					log_err("Successfully created _offscreenBufferAsInputDynCockpitBG with combined flags\n");
				}

				step = "_DCTextAsInput";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_DCTextAsInput);
				if (FAILED(hr)) {
					log_err("Failed to create _DCTextAsInput, error: 0x%x\n", hr);
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				else {
					log_err("Successfully created _DCTextAsInput with combined flags\n");
				}

				if (g_EnhancedHUDData.Enabled)
				{
					// desc.Width, Height is the current screen resolution.
					CD3D11_TEXTURE2D_DESC tmp = desc;
					if (g_bUseSteamVR) desc.Width = desc.Height = VR_ENHANCED_HUD_BUFFER_SIZE;

					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_enhancedHUDBuffer);
					if (FAILED(hr)) {
						log_err("Failed to create _enhancedHUDBuffer, error: 0x%x\n", hr);
						log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
					else {
						log_err("Successfully created _enhancedHUDBuffer\n");
					}
					if (g_bUseSteamVR) desc = tmp;
				}

				// Restore the previous bind flags, just in case there is a dependency on these later on
				desc.BindFlags = curFlags;
			}

			// Shading System: Create Non-MSAA SSAO/SS Mask Buffers
			// The SSAO/SS buffers need to be in stereo
			desc.ArraySize = g_bUseSteamVR ? 2 : 1;
			if (g_bReshadeEnabled) {
				UINT oldFlags = desc.BindFlags;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Format = AO_MASK_FORMAT;

				step = "_ssaoMask";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssaoMask);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				step = "_ssMask";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssMask);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				/*step = "_ssEmissionMask";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssEmissionMask);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}*/

				desc.Format = AO_DEPTH_BUFFER_FORMAT;
				desc.MipLevels = 1; // 4;
				step = "_normBuf";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_normBuf);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				if (g_bUseSteamVR) {
					desc.Format = AO_MASK_FORMAT;
					desc.MipLevels = 1; // 4;
					step = "_ssaoMaskR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssaoMaskR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}

					step = "_ssMaskR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssMaskR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}

					/*step = "_ssEmissionMaskR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssEmissionMaskR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}*/

					desc.Format = AO_DEPTH_BUFFER_FORMAT;
					step = "_normBufR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_normBufR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
				}

				desc.Format = oldFormat;
				desc.MipLevels = 1;
				desc.BindFlags = oldFlags;
			}

			// In-Flight Raytracing: Create Non-MSAA RT Shadow Buffer
			if (g_bRTEnabled && g_bRTEnableSoftShadows)
			{
				UINT oldW = desc.Width;
				UINT oldH = desc.Height;
				UINT oldFlags = desc.BindFlags;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Width /= g_RTConstantsBuffer.RTShadowMaskSizeFactor;
				desc.Height /= g_RTConstantsBuffer.RTShadowMaskSizeFactor;
				desc.Format = RT_SHADOW_FORMAT;

				step = "_rtShadowMask";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_rtShadowMask);
				if (FAILED(hr)) goto out;

				if (g_bUseSteamVR)
				{
					step = "_rtShadowMaskR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_rtShadowMaskR);
					if (FAILED(hr)) goto out;
				}

				desc.Format = oldFormat;
				desc.MipLevels = 1;
				desc.BindFlags = oldFlags;
				desc.Width = oldW;
				desc.Height = oldH;
			}

			// Create Non-MSAA AO Buffers
			if (g_bAOEnabled) {
				UINT oldFlags = desc.BindFlags;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Format = AO_DEPTH_BUFFER_FORMAT;
#ifdef GENMIPMAPS
				desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
				desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
				desc.MipLevels = 0; // MAX_MIP_LEVELS;
#else
				desc.MipLevels = 1; // 4;
#endif

				step = "_depthBufAsInput";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_depthBufAsInput);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				if (g_bUseSteamVR) {
					step = "_depthBufAsInputR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_depthBufAsInputR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
				}

				// Add the RTV flag
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
				/*
				step = "_bentBuf";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bentBuf);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				if (g_bUseSteamVR) {
					step = "_bentBufR";
					hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_bentBufR);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_err_desc(step, hWnd, hr, desc);
						goto out;
					}
				}
				*/

				desc.Format = HDR_FORMAT;
				step = "_ssaoBuf";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssaoBuf);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				// This buffer is needed in all cases, not only SteamVR/stereo rendering, since it is used later as Indirect SSDO buffer
				desc.Format = HDR_FORMAT;
				step = "_ssaoBufR";
				hr = this->_d3dDevice->CreateTexture2D(&desc, nullptr, &this->_ssaoBufR);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}

				desc.Format = oldFormat;
				desc.BindFlags = oldFlags;
				desc.MipLevels = 1;
			}

			// Create Non-MSAA buffers for 3D Vision
			// 3D Vision uses double-wide rendering, not instanced stereo
			desc.ArraySize = 1;
			if (g_b3DVisionEnabled) {
				D3D11_TEXTURE2D_DESC backBufferDesc;
				log_debug("[DBG] [3DV] 3D Vision is enabled, creating buffers...");

				this->_backBuffer->GetDesc(&backBufferDesc);
				backBufferDesc.Width *= 2;
				backBufferDesc.Height++;

				step = "_vision3DNoMSAA";
				hr = this->_d3dDevice->CreateTexture2D(&backBufferDesc, nullptr, &this->_vision3DNoMSAA);
				if (FAILED(hr)) {
					log_debug("[DBG] [3DV] Failed to create _vision3DNoMSAA: hr: 0x%x", hr);
					log_err("Failed to create _vision3DStaging\n");
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				else {
					log_debug("[DBG] [3DV] Succesfully created _vision3DNoMSAA, size: %d, %d",
						backBufferDesc.Width, backBufferDesc.Height);
				}

				backBufferDesc.BindFlags = 0;
				backBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				backBufferDesc.Usage = D3D11_USAGE_STAGING;

				step = "_vision3DStaging";
				hr = this->_d3dDevice->CreateTexture2D(&backBufferDesc, nullptr, &this->_vision3DStaging);
				if (FAILED(hr)) {
					log_debug("[DBG] [3DV] Failed to create _vision3DStaging: hr: 0x%x", hr);
					log_err("Failed to create _vision3DStaging\n");
					log_err("GetDeviceRemovedReason: 0x%x\n", this->_d3dDevice->GetDeviceRemovedReason());
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_err_desc(step, hWnd, hr, desc);
					goto out;
				}
				else {
					log_debug("[DBG] [3DV] Succesfully created _vision3DStaging, size: %d, %d",
						backBufferDesc.Width, backBufferDesc.Height);
				}
			}
			desc.ArraySize = g_bUseSteamVR ? 2 : 1;
		}

		/* SRVs */
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = desc.Format;
			shaderResourceViewDesc.ViewDimension = g_bUseSteamVR ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY : D3D11_SRV_DIMENSION_TEXTURE2D;
			if (g_bUseSteamVR) {
				shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
				shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
				shaderResourceViewDesc.Texture2DArray.ArraySize = 2;
				shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
			}
			else {
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
			}
			D3D11_SRV_DIMENSION curDimension = shaderResourceViewDesc.ViewDimension;

#ifdef GENMIPMAPS
			shaderResourceViewDesc.Texture2D.MipLevels = -1; // MAX_MIP_LEVELS
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
#endif

			// Create the shader resource view for offscreenBufferAsInput
			step = "offscreenAsInputShaderResourceView";
			hr = this->_d3dDevice->CreateShaderResourceView(this->_offscreenBufferAsInput,
				&shaderResourceViewDesc, &this->_offscreenAsInputShaderResourceView);
			if (FAILED(hr)) {
				log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
				goto out;
			}

			if (g_bUseSteamVR)
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC tmp = shaderResourceViewDesc;
				shaderResourceViewDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
				shaderResourceViewDesc.Texture2D.MipLevels       = 1;
				// Create the shader resource view for _offscreenBufferHd
				step = "offscreenBufferHdSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_offscreenBufferHd,
					&shaderResourceViewDesc, &this->_offscreenBufferHdSRV);
				if (FAILED(hr)) goto out;
				log_debug("[DBG] Created _offscreenBufferHdSRV");
				shaderResourceViewDesc = tmp;
			}

			step = "backgroundBufferSRV";
			hr = this->_d3dDevice->CreateShaderResourceView(this->_backgroundBufferAsInput,
				&shaderResourceViewDesc, &this->_backgroundBufferSRV);
			if (FAILED(hr))
				goto out;

			step = "_backgroundAuxBufferSRV";
			hr = this->_d3dDevice->CreateShaderResourceView(this->_backgroundAuxBuffer,
				&shaderResourceViewDesc, &this->_backgroundAuxBufferSRV);
			if (FAILED(hr))
				goto out;

			step = "transp1SRV";
			if (FAILED(this->_d3dDevice->CreateShaderResourceView(this->_transpBufferAsInput1,
				&shaderResourceViewDesc, &this->_transp1SRV)))
				goto out;

			step = "transp2SRV";
			if (FAILED(this->_d3dDevice->CreateShaderResourceView(this->_transpBufferAsInput2,
				&shaderResourceViewDesc, &this->_transp2SRV)))
				goto out;

			// Use either this version or the one above where a DDS is loaded. Can't use
			// both!
			/*if (g_bUseTextureCube)
			{
				step = "_textureCubeSRV";

				D3D11_SHADER_RESOURCE_VIEW_DESC tmpDesc = shaderResourceViewDesc;
				shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				shaderResourceViewDesc.TextureCube.MipLevels = 1;
				shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;

				if (FAILED(this->_d3dDevice->CreateShaderResourceView(this->_textureCube,
					&shaderResourceViewDesc, &this->_textureCubeSRV)))
					goto out;
				shaderResourceViewDesc = tmpDesc;
			}*/

			step = "_shadertoySRV";
			hr = this->_d3dDevice->CreateShaderResourceView(this->_shadertoyBuf, &shaderResourceViewDesc, &this->_shadertoySRV);
			if (FAILED(hr)) {
				log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
				goto out;
			}

			step = "_shadertoyAuxSRV";
			hr = this->_d3dDevice->CreateShaderResourceView(this->_shadertoyAuxBuf, &shaderResourceViewDesc, &this->_shadertoyAuxSRV);
			if (FAILED(hr)) {
				log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
				goto out;
			}

			// _ReticleSRV
			// The reticle is not rendered in stereo:
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			{
				step = "_ReticleSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_ReticleBufAsInput, &shaderResourceViewDesc, &this->_ReticleSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}
			}
			shaderResourceViewDesc.ViewDimension = curDimension;

			if (g_bUseSteamVR) {
				// Create the shader resource view for offscreenBufferAsInputR
				step = "offscreenAsInputShaderResourceViewR";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_offscreenBufferAsInputR,
					&shaderResourceViewDesc, &this->_offscreenAsInputShaderResourceViewR);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				step = "_shadertoySRV_R";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_shadertoyBufR, &shaderResourceViewDesc, &this->_shadertoySRV_R);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}
			}

			// Bloom SRVs
			if (g_bReshadeEnabled)
			{
				shaderResourceViewDesc.Format = BLOOM_BUFFER_FORMAT;
				step = "_offscreenAsInputBloomMaskSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_offscreenBufferAsInputBloomMask,
					&shaderResourceViewDesc, &this->_offscreenAsInputBloomMaskSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				//if (g_bUseSteamVR)
				//{
				//	step = "_offscreenAsInputBloomSRV_R";
				//	hr = this->_d3dDevice->CreateShaderResourceView(this->_offscreenBufferAsInputBloomMaskR,
				//		&shaderResourceViewDesc, &this->_offscreenAsInputBloomMaskSRV_R);
				//	if (FAILED(hr)) {
				//		log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				//		log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
				//		goto out;
				//	}
				//}

				step = "_bloomOutput1SRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_bloomOutput1,
					&shaderResourceViewDesc, &this->_bloomOutput1SRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}
				shaderResourceViewDesc.Format = oldFormat;
			}

			// Bloom intermediate (bloomOutput1, bloomOutput2) SRVs
			if (g_bBloomEnabled) {
				shaderResourceViewDesc.Format = BLOOM_BUFFER_FORMAT;
				step = "_bloomOutput2SRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_bloomOutput2,
					&shaderResourceViewDesc, &this->_bloomOutput2SRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				step = "_bloomOutputSumSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_bloomOutputSum,
					&shaderResourceViewDesc, &this->_bloomOutputSumSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				//if (g_bUseSteamVR) {
				//	shaderResourceViewDesc.Format = BLOOM_BUFFER_FORMAT;

				//	step = "_bloomOutput1SRV_R";
				//	hr = this->_d3dDevice->CreateShaderResourceView(this->_bloomOutput1R,
				//		&shaderResourceViewDesc, &this->_bloomOutput1SRV_R);
				//	if (FAILED(hr)) {
				//		log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				//		log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
				//		goto out;
				//	}

				//	step = "_bloomOutput2SRV_R";
				//	hr = this->_d3dDevice->CreateShaderResourceView(this->_bloomOutput2R,
				//		&shaderResourceViewDesc, &this->_bloomOutput2SRV_R);
				//	if (FAILED(hr)) {
				//		log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				//		log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
				//		goto out;
				//	}

				//	step = "_bloomOutputSumSRV_R";
				//	hr = this->_d3dDevice->CreateShaderResourceView(this->_bloomOutputSumR,
				//		&shaderResourceViewDesc, &this->_bloomOutputSumSRV_R);
				//	if (FAILED(hr)) {
				//		log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
				//		log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
				//		goto out;
				//	}
				//}
				shaderResourceViewDesc.Format = oldFormat;
			}

			// Shading System SRVs
			if (g_bReshadeEnabled) {
				DXGI_FORMAT oldFormat = shaderResourceViewDesc.Format;
				shaderResourceViewDesc.Format = AO_MASK_FORMAT;

				step = "_ssaoMaskSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_ssaoMask, &shaderResourceViewDesc, &this->_ssaoMaskSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				step = "_ssMaskSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_ssMask, &shaderResourceViewDesc, &this->_ssMaskSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				/*step = "_ssEmissionMaskSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_ssEmissionMask, &shaderResourceViewDesc, &this->_ssEmissionMaskSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}*/

				shaderResourceViewDesc.Format = AO_DEPTH_BUFFER_FORMAT;
				shaderResourceViewDesc.Texture2D.MipLevels = 1; // 4;
				step = "_normBufSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_normBuf, &shaderResourceViewDesc, &this->_normBufSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				if (g_bUseSteamVR) {
					shaderResourceViewDesc.Format = AO_MASK_FORMAT;
					shaderResourceViewDesc.Texture2D.MipLevels = 1;
					step = "_ssaoMaskSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_ssaoMaskR, &shaderResourceViewDesc, &this->_ssaoMaskSRV_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}

					step = "_ssMaskSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_ssMaskR, &shaderResourceViewDesc, &this->_ssMaskSRV_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}

					/*step = "_ssEmissionMaskSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_ssEmissionMaskR, &shaderResourceViewDesc, &this->_ssEmissionMaskSRV_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}*/

					shaderResourceViewDesc.Format = AO_DEPTH_BUFFER_FORMAT;
					shaderResourceViewDesc.Texture2D.MipLevels = 1; // 4;
					step = "_normBufSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_normBufR, &shaderResourceViewDesc, &this->_normBufSRV_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}
				}
				shaderResourceViewDesc.Format = oldFormat;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
			}

			// Raytracing Shadow Mask SRVs
			if (g_bRTEnabled && g_bRTEnableSoftShadows) {
				DXGI_FORMAT oldFormat = shaderResourceViewDesc.Format;
				shaderResourceViewDesc.Format = RT_SHADOW_FORMAT;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
				shaderResourceViewDesc.ViewDimension = curDimension;
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

				step = "_rtShadowMaskSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_rtShadowMask, &shaderResourceViewDesc, &this->_rtShadowMaskSRV);
				if (FAILED(hr)) goto out;

				if (g_bUseSteamVR) {
					step = "_rtShadowMaskSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_rtShadowMaskR, &shaderResourceViewDesc, &this->_rtShadowMaskSRV_R);
					if (FAILED(hr)) goto out;
				}

				shaderResourceViewDesc.Format = oldFormat;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
			}

			// AO SRVs
			if (g_bAOEnabled) {
				shaderResourceViewDesc.Format = AO_DEPTH_BUFFER_FORMAT;
#ifdef GENMIPMAPS
				shaderResourceViewDesc.Texture2D.MipLevels = -1; // MAX_MIP_LEVELS;
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
#else
				shaderResourceViewDesc.Texture2D.MipLevels = 1; // 4;
#endif

				step = "_depthBufSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_depthBufAsInput,
					&shaderResourceViewDesc, &this->_depthBufSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				if (g_bUseSteamVR) {
					step = "_depthBufSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_depthBufAsInputR,
						&shaderResourceViewDesc, &this->_depthBufSRV_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}
				}

				/*
				step = "_bentBufSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_bentBuf, &shaderResourceViewDesc, &this->_bentBufSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				if (g_bUseSteamVR) {
					step = "_bentBufSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_bentBufR, &shaderResourceViewDesc, &this->_bentBufSRV_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}
				}
				*/

				shaderResourceViewDesc.Format = HDR_FORMAT;
				step = "_ssaoBufSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_ssaoBuf, &shaderResourceViewDesc, &this->_ssaoBufSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				if (g_bUseSteamVR) {
					step = "_ssaoBufSRV_R";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_ssaoBufR, &shaderResourceViewDesc, &this->_ssaoBufSRV_R);
					if (FAILED(hr)) {
						log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}
				}

				shaderResourceViewDesc.Format = oldFormat;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
			}

			// Dynamic Cockpit SRVs
			// They are not rendered in stereo.
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			if (g_bReshadeEnabled) {
				step = "_offscreenAsInputDynCockpitSRV";

				hr = this->_d3dDevice->CreateShaderResourceView(this->_offscreenAsInputDynCockpit,
					&shaderResourceViewDesc, &this->_offscreenAsInputDynCockpitSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				step = "_offscreenAsInputDynCockpitBG_SRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_offscreenAsInputDynCockpitBG,
					&shaderResourceViewDesc, &this->_offscreenAsInputDynCockpitBG_SRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				step = "_DCTextSRV";
				hr = this->_d3dDevice->CreateShaderResourceView(this->_DCTextAsInput,
					&shaderResourceViewDesc, &this->_DCTextSRV);
				if (FAILED(hr)) {
					log_err("dwWidth, Height: %u, %u\n", dwWidth, dwHeight);
					log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
					goto out;
				}

				if (g_EnhancedHUDData.Enabled)
				{
					step = "_enhancedHUDBuffer";
					hr = this->_d3dDevice->CreateShaderResourceView(this->_enhancedHUDBuffer,
						&shaderResourceViewDesc, &this->_enhancedHUDSRV);
					if (FAILED(hr)) {
						log_shaderres_view(step, hWnd, hr, shaderResourceViewDesc);
						goto out;
					}
				}
			}

			shaderResourceViewDesc.ViewDimension = curDimension;
		}
	}

	// Build the vertex buffers
	if (SUCCEEDED(hr))
	{
		float W = (float )_displayWidth, H = (float )_displayHeight;
		if (g_bUseSteamVR) {
			W /= g_fCurInGameAspectRatio * g_fCurInGameAspectRatio;
			H /= g_fCurInGameAspectRatio * g_fCurInGameAspectRatio;
		}
		g_fCurInGameWidth = (float)_displayWidth;
		g_fCurInGameHeight = (float)_displayHeight;
		g_fCurInGameAspectRatio = g_fCurInGameWidth / g_fCurInGameHeight;

		BuildHUDVertexBuffer(g_fCurInGameWidth, g_fCurInGameHeight);
		BuildHyperspaceVertexBuffer(g_fCurInGameWidth, g_fCurInGameHeight);
		BuildPostProcVertexBuffer();
		BuildSpeedVertexBuffer();
		CreateRandomVectorTexture();
		CreateGrayNoiseTexture();
		if (g_b3DVisionEnabled)
			Create3DVisionSignatureTexture();
	}

	/* RTVs */
	if (SUCCEEDED(hr))
	{
		// This lambda function will return the right RtvDesc according to the requested parameters.	
		auto GetRtvDesc = [](
			bool useMultisampling,
			bool instancedStereoRendering,
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN
			)
		{
			CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
			if (useMultisampling) {
				if (instancedStereoRendering)
				{
					rtvDesc = CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY, format);
					rtvDesc.Texture2DMSArray.ArraySize = 2;
				}
				else {
					rtvDesc = CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2DMS, format);
				}
			}
			else {
				if (instancedStereoRendering) {
					rtvDesc = CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, format);
					rtvDesc.Texture2DArray.ArraySize = 2;
				}
				else
					rtvDesc = CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2D, format);
			}
			return rtvDesc;
		};

		step = "_renderTargetView";
		hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBuffer, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, BACKBUFFER_FORMAT), &this->_renderTargetView);
		if (FAILED(hr)) goto out;

		if (g_bUseSteamVR)
		{
			step = "_renderTargetViewHd";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferHd,
				&GetRtvDesc(this->_useMultisampling, false /* no instanced rendering */, BACKBUFFER_FORMAT), &this->_renderTargetViewHd);
			if (FAILED(hr)) goto out;
		}

		step = "_renderTargetViewPost";
		hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferPost, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, BACKBUFFER_FORMAT), &this->_renderTargetViewPost);
		if (FAILED(hr)) goto out;

		if (g_b3DVisionEnabled) {
			// The 3D vision RTV does not use instanced stereo rendering
			step = "_RTVvision3DPost";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_vision3DPost, &GetRtvDesc(this->_useMultisampling, false), &this->_RTVvision3DPost);
			if (FAILED(hr)) goto out;
		}

		step = "_shadertoyRTV";
		hr = this->_d3dDevice->CreateRenderTargetView(
			this->_useMultisampling ? this->_shadertoyBufMSAA : this->_shadertoyBuf,
			&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR), 
			&this->_shadertoyRTV);
		if (FAILED(hr)) {
			log_debug("[DBG] [ST] _shadertoyRTV FAILED");
			goto out;
		}

		step = "_transp1RTV";
		if (FAILED(this->_d3dDevice->CreateRenderTargetView(this->_transpBuffer1,
			&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR), &this->_transp1RTV)))
			goto out;

		step = "_transp2RTV";
		if (FAILED(this->_d3dDevice->CreateRenderTargetView(this->_transpBuffer2,
			&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR), &this->_transp2RTV)))
			goto out;

		step = "_backgroundRTV";
		if (FAILED(this->_d3dDevice->CreateRenderTargetView(this->_backgroundBuffer,
			&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR), &this->_backgroundRTV)))
			goto out;

		// _ReticleRTV
		{
			step = "_ReticleRTV";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_ReticleBufMSAA, &GetRtvDesc(this->_useMultisampling, false), &this->_ReticleRTV);
			if (FAILED(hr)) goto out;
		}

		if (g_bUseSteamVR) {
			step = "RenderTargetViewR";
			CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc = GetRtvDesc(this->_useMultisampling, g_bUseSteamVR);
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferR, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR), &this->_renderTargetViewR);
			if (FAILED(hr)) goto out;

			step = "RenderTargetViewPostR";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferPostR, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR), &this->_renderTargetViewPostR);
			if (FAILED(hr)) goto out;

			step = "renderTargetViewSteamVRResize";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_steamVRPresentBuffer, &GetRtvDesc(this->_useMultisampling, false), &this->_renderTargetViewSteamVRResize);
			if (FAILED(hr)) goto out;

			step = "renderTargetViewSteamVRResizeOverlay";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_steamVROverlayBuffer, &GetRtvDesc(this->_useMultisampling, false), &this->_renderTargetViewSteamVROverlayResize);
			if (FAILED(hr)) goto out;

			step = "_shadertoyRTV_R";
			// This RTV writes to a non-MSAA texture
			hr = this->_d3dDevice->CreateRenderTargetView(
				this->_useMultisampling ? this->_shadertoyBufMSAA_R : this->_shadertoyBufR,
				&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR),
				&this->_shadertoyRTV_R);
			if (FAILED(hr)) {
				log_debug("[DBG] [ST] _shadertoyRTV_R FAILED");
				goto out;
			}
		}

		// Dynamic Cockpit RTVs
		if (g_bReshadeEnabled) {
			step = "_renderTargetViewDynCockpit";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferDynCockpit, &GetRtvDesc(this->_useMultisampling, false), &this->_renderTargetViewDynCockpit);
			if (FAILED(hr)) {
				log_debug("[DBG] [DC] _renderTargetViewDynCockpit FAILED");
				goto out;
			}

			step = "_renderTargetViewDynCockpitBG";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferDynCockpitBG, &GetRtvDesc(this->_useMultisampling, false), &this->_renderTargetViewDynCockpitBG);
			if (FAILED(hr)) {
				log_debug("[DBG] [DC] _renderTargetViewDynCockpitBG FAILED");
				goto out;
			}

			step = "_DCTextRTV";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_DCTextMSAA, &GetRtvDesc(this->_useMultisampling, false), &this->_DCTextRTV);
			if (FAILED(hr)) {
				log_debug("[DBG] [DC] _DCTextRTV FAILED");
				goto out;
			}
			
			step = "_renderTargetViewDynCockpitAsInput";
			// This RTV writes to a monoscopic non-MSAA texture
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenAsInputDynCockpit, &GetRtvDesc(false, false),
				&this->_renderTargetViewDynCockpitAsInput);
			if (FAILED(hr)) {
				log_debug("[DBG] [DC] _renderTargetViewDynCockpitAsInput FAILED");
				goto out;
			}

			step = "_renderTargetViewDynCockpitAsInputBG";
			// This RTV writes to a monoscopic non-MSAA texture
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenAsInputDynCockpitBG, &GetRtvDesc(false, false),
				&this->_renderTargetViewDynCockpitAsInputBG);
			if (FAILED(hr)) {
				log_debug("[DBG] [DC] _renderTargetViewDynCockpitAsInputBG FAILED");
				goto out;
			}

			step = "_DCTextAsInputRTV";
			// This RTV writes to a non-MSAA texture
			hr = this->_d3dDevice->CreateRenderTargetView(this->_DCTextAsInput, &GetRtvDesc(false, false), &_DCTextAsInputRTV);
			if (FAILED(hr)) {
				log_debug("[DBG] [DC] _DCTextAsInputRTV FAILED");
				goto out;
			}
		}

		if (g_bReshadeEnabled) {
			
			step = "_renderTargetViewBloomMask";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferBloomMask,
				&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, BLOOM_BUFFER_FORMAT),
				&this->_renderTargetViewBloomMask);
			if (FAILED(hr)) goto out;

			step = "_inputBloomMaskRTV";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferAsInputBloomMask,
				&GetRtvDesc(false, g_bUseSteamVR, BLOOM_BUFFER_FORMAT),
				&this->_inputBloomMaskRTV);
			if (FAILED(hr)) goto out;

			step = "_renderTargetViewSSAOMask";
			hr = this->_d3dDevice->CreateRenderTargetView(
				this->_useMultisampling ? this->_ssaoMaskMSAA : this->_ssaoMask, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, AO_MASK_FORMAT), &this->_renderTargetViewSSAOMask);
			if (FAILED(hr)) goto out;

			step = "_renderTargetViewSSMask";
			hr = this->_d3dDevice->CreateRenderTargetView(
				this->_useMultisampling ? this->_ssMaskMSAA : this->_ssMask, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, AO_MASK_FORMAT), &this->_renderTargetViewSSMask);
			if (FAILED(hr)) goto out;

			//step = "_renderTargetViewEmissionMask";
			//hr = this->_d3dDevice->CreateRenderTargetView(this->_ssEmissionMask, &renderTargetViewDescNoMSAA, 	&this->_renderTargetViewEmissionMask);
			//if (FAILED(hr)) goto out;

			step = "_renderTargetViewNormBuf";
			hr = this->_d3dDevice->CreateRenderTargetView(
				this->_useMultisampling ? this->_normBufMSAA : this->_normBuf,
				&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, AO_DEPTH_BUFFER_FORMAT),
				&this->_renderTargetViewNormBuf);
			if (FAILED(hr)) goto out;

			if (g_bUseSteamVR) {
				//step = "_renderTargetViewBloomMaskR";
				//hr = this->_d3dDevice->CreateRenderTargetView(this->_offscreenBufferBloomMaskR,
				//	&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, BLOOM_BUFFER_FORMAT),
				//	&this->_renderTargetViewBloomMaskR);
				//if (FAILED(hr)) goto out;

				//step = "_renderTargetViewEmissionMaskR";
				//hr = this->_d3dDevice->CreateRenderTargetView(this->_ssEmissionMaskR, &renderTargetViewDescNoMSAA, &this->_renderTargetViewEmissionMaskR);
				//if (FAILED(hr)) goto out;

				step = "_renderTargetViewNormBufR";
				hr = this->_d3dDevice->CreateRenderTargetView(
					this->_useMultisampling ? this->_normBufMSAA_R : this->_normBufR,
					&GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, AO_DEPTH_BUFFER_FORMAT),
					&this->_renderTargetViewNormBufR);
				if (FAILED(hr)) goto out;
			}
		}

		// Bloom RTVs
		if (g_bBloomEnabled) {
			// These RTVs render to non-MSAA buffers
			step = "_renderTargetViewBloom1";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_bloomOutput1, &GetRtvDesc(false, g_bUseSteamVR, BLOOM_BUFFER_FORMAT), &this->_renderTargetViewBloom1);
			if (FAILED(hr)) goto out;
			
			step = "_renderTargetViewBloom2";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_bloomOutput2, &GetRtvDesc(false, g_bUseSteamVR, BLOOM_BUFFER_FORMAT), &this->_renderTargetViewBloom2);
			if (FAILED(hr)) goto out;

			step = "_renderTargetViewBloomSum";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_bloomOutputSum, &GetRtvDesc(false, g_bUseSteamVR, BLOOM_BUFFER_FORMAT), &this->_renderTargetViewBloomSum);
			if (FAILED(hr)) goto out;

			//if (g_bUseSteamVR) {
			//	step = "_renderTargetViewBloom1R";
			//	hr = this->_d3dDevice->CreateRenderTargetView(this->_bloomOutput1R, &GetRtvDesc(false, g_bUseSteamVR, BLOOM_BUFFER_FORMAT), &this->_renderTargetViewBloom1R);
			//	if (FAILED(hr)) goto out;

			//	step = "_renderTargetViewBloom2R";
			//	hr = this->_d3dDevice->CreateRenderTargetView(this->_bloomOutput2R, &GetRtvDesc(false, g_bUseSteamVR, BLOOM_BUFFER_FORMAT), &this->_renderTargetViewBloom2R);
			//	if (FAILED(hr)) goto out;

			//	step = "_renderTargetViewBloomSumR";
			//	hr = this->_d3dDevice->CreateRenderTargetView(this->_bloomOutputSumR, &GetRtvDesc(false, g_bUseSteamVR, BLOOM_BUFFER_FORMAT), &this->_renderTargetViewBloomSumR);
			//	if (FAILED(hr)) goto out;
			//}
		}

		// SSAO RTVs
		if (g_bAOEnabled) {
			step = "_renderTargetViewDepthBuf";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_depthBuf, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, AO_DEPTH_BUFFER_FORMAT), &this->_renderTargetViewDepthBuf);
			if (FAILED(hr)) goto out;

			/*
			step = "_renderTargetViewBentBuf";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_bentBuf, &renderTargetViewDescNoMSAA, &this->_renderTargetViewBentBuf);
			if (FAILED(hr)) goto out;
			*/

			if (g_bUseSteamVR) {
				step = "_renderTargetViewDepthBufR";
				hr = this->_d3dDevice->CreateRenderTargetView(this->_depthBufR, &GetRtvDesc(this->_useMultisampling, g_bUseSteamVR, AO_DEPTH_BUFFER_FORMAT), &this->_renderTargetViewDepthBufR);
				if (FAILED(hr)) goto out;

				/*
				step = "_renderTargetViewBentBufR";
				hr = this->_d3dDevice->CreateRenderTargetView(this->_bentBufR, &renderTargetViewDescNoMSAA, &this->_renderTargetViewBentBufR);
				if (FAILED(hr)) goto out;
				*/
			}
			step = "_renderTargetViewSSAO";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_ssaoBuf, &GetRtvDesc(false, g_bUseSteamVR, HDR_FORMAT), &this->_renderTargetViewSSAO);
			if (FAILED(hr)) goto out;

			// This buffer is needed in all cases, not only SteamVR/stereo rendering, since it is used later as Indirect SSDO buffer
			step = "_renderTargetViewSSAO_R";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_ssaoBufR, &GetRtvDesc(false, g_bUseSteamVR, HDR_FORMAT), &this->_renderTargetViewSSAO_R);
			if (FAILED(hr)) goto out;
		}

		// Raytracing Shadow Mask RTVs
		if (g_bRTEnabled && g_bRTEnableSoftShadows) {
			//renderTargetViewDescNoMSAA.Format = RT_SHADOW_FORMAT;
			step = "_rtShadowMaskRTV";
			hr = this->_d3dDevice->CreateRenderTargetView(this->_rtShadowMask, &GetRtvDesc(false, g_bUseSteamVR, RT_SHADOW_FORMAT), &this->_rtShadowMaskRTV);
			if (FAILED(hr)) goto out;

			if (g_bUseSteamVR) {
				//renderTargetViewDescNoMSAA.Format = RT_SHADOW_FORMAT;
				step = "_rtShadowMaskRTV_R";
				hr = this->_d3dDevice->CreateRenderTargetView(this->_rtShadowMaskR, &GetRtvDesc(false, g_bUseSteamVR, RT_SHADOW_FORMAT), &this->_rtShadowMaskRTV_R);
				if (FAILED(hr)) goto out;
			}
		}
	}

	/* depth stencil */
	if (SUCCEEDED(hr))
	{
		step = "DepthStencil";
		D3D11_TEXTURE2D_DESC depthStencilDesc;
		depthStencilDesc.Width = this->_backbufferWidth;
		depthStencilDesc.Height = this->_backbufferHeight;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = g_bUseSteamVR ? 2 : 1;
		depthStencilDesc.Format = DEPTH_BUFFER_FORMAT;
		depthStencilDesc.SampleDesc.Count = this->_sampleDesc.Count;
		depthStencilDesc.SampleDesc.Quality = this->_sampleDesc.Quality;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_depthStencilL);

		// In VR mode, we can keep _depthStencilViewR as it was before: a non-array single texture. We
		// can then use it for things that only render once even in VR, like the CMD miniature.
		depthStencilDesc.ArraySize = 1;
		hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_depthStencilR);

		if (g_bUseSteamVR && g_config.HDConcourseEnabled &&
			g_config.D3dRendererTexturesHookEnabled)
		{
			auto tmp = depthStencilDesc;
			depthStencilDesc.Width     = HD_CONCOURSE_WIDTH;
			depthStencilDesc.Height    = HD_CONCOURSE_HEIGHT;
			depthStencilDesc.ArraySize = 1;
			depthStencilDesc.SampleDesc.Count   = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
			hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_depthStencilHd);
			depthStencilDesc = tmp;
		}

		// This lambda function will return the right SrvDesc according to the requested parameters.	
		auto GetSrvDesc = [](
			bool useMultisampling,
			bool instancedStereoRendering
			)
		{
			CD3D11_DEPTH_STENCIL_VIEW_DESC srvDesc;
			if (useMultisampling) {
				if (instancedStereoRendering) {
					srvDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY);
					srvDesc.Texture2DMSArray.ArraySize = 2;
				}
				else
					srvDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2DMS);
			}
			else {
				if (instancedStereoRendering) {
					srvDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2DARRAY);
					srvDesc.Texture2DArray.ArraySize = 2;
				}
				else
					srvDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D);
			}
			return srvDesc;
		};

		if (SUCCEEDED(hr))
		{
			step = "DepthStencilViewL";
			CD3D11_DEPTH_STENCIL_VIEW_DESC srvDesc = GetSrvDesc(this->_useMultisampling, g_bUseSteamVR);
			hr = this->_d3dDevice->CreateDepthStencilView(this->_depthStencilL, &GetSrvDesc(this->_useMultisampling, g_bUseSteamVR), &this->_depthStencilViewL);
			if (FAILED(hr)) goto out;
		}

		if (SUCCEEDED(hr))
		{
			step = "DepthStencilViewR";
			hr = this->_d3dDevice->CreateDepthStencilView(this->_depthStencilR, &GetSrvDesc(this->_useMultisampling, false), &this->_depthStencilViewR);
			if (FAILED(hr)) goto out;
		}

		if (SUCCEEDED(hr) && g_bUseSteamVR &&
			g_config.HDConcourseEnabled && g_config.D3dRendererTexturesHookEnabled)
		{
			step = "DepthStencilViewHd";
			hr = this->_d3dDevice->CreateDepthStencilView(this->_depthStencilHd, &GetSrvDesc(this->_useMultisampling, false), &this->_depthStencilViewHd);
			if (FAILED(hr)) goto out;
		}

		// Shadow Mapping Textures
		{
			depthStencilDesc.Width = g_ShadowMapping.ShadowMapSize;
			depthStencilDesc.Height = g_ShadowMapping.ShadowMapSize;
			depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
			depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthStencilDesc.ArraySize = 1;
			depthStencilDesc.SampleDesc.Count = 1; // The ShadowMap DSV is always going to be Non-MSAA
			depthStencilDesc.SampleDesc.Quality = 0;

			step = "_shadowMap";
			hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_shadowMap);
			if (FAILED(hr)) goto out;

			if (g_ShadowMapping.bCSMEnabled) {
				step = "_csmMap";
				depthStencilDesc.Width = CSM_MAP_SIZE;
				depthStencilDesc.Height = CSM_MAP_SIZE;
				hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_csmMap);
				if (FAILED(hr)) goto out;
			}

			step = "_shadowMapDSV";
			CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
			hr = this->_d3dDevice->CreateDepthStencilView(this->_shadowMap, &depthStencilViewDesc, &this->_shadowMapDSV);
			if (FAILED(hr)) goto out;

			if (g_ShadowMapping.bCSMEnabled) {
				step = "_csmMapDSV";
				CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
				hr = this->_d3dDevice->CreateDepthStencilView(this->_shadowMap, &depthStencilViewDesc, &this->_csmMapDSV);
				if (FAILED(hr)) goto out;
			}

			depthStencilDesc.Width = g_ShadowMapping.ShadowMapSize;
			depthStencilDesc.Height = g_ShadowMapping.ShadowMapSize;
			depthStencilDesc.Format = DXGI_FORMAT_R32_FLOAT;
			depthStencilDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			step = "_shadowMapDebug";
			hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_shadowMapDebug);
			if (FAILED(hr)) goto out;

			// TEXTURE ARRAY FROM THIS POINT ON
			step = "_shadowMapArray";
			depthStencilDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			depthStencilDesc.ArraySize = MAX_XWA_LIGHTS;
			hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_shadowMapArray);
			if (FAILED(hr)) goto out;

			if (g_ShadowMapping.bCSMEnabled) {
				// Inherit previous values
				step = "_csmArray";
				depthStencilDesc.Width = CSM_MAP_SIZE;
				depthStencilDesc.Height = CSM_MAP_SIZE;
				depthStencilDesc.ArraySize = MAX_CSM_LEVELS;
				hr = this->_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &this->_csmArray);
				if (FAILED(hr)) goto out;
			}

			step = "_shadowMapArraySRV";
			D3D11_SHADER_RESOURCE_VIEW_DESC depthStencilSRVDesc;
			depthStencilSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
			depthStencilSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			depthStencilSRVDesc.Texture2DArray.MipLevels = depthStencilDesc.MipLevels;
			depthStencilSRVDesc.Texture2DArray.MostDetailedMip = 0;
			depthStencilSRVDesc.Texture2DArray.FirstArraySlice = 0;
			depthStencilSRVDesc.Texture2DArray.ArraySize = MAX_XWA_LIGHTS;
			hr = this->_d3dDevice->CreateShaderResourceView(this->_shadowMapArray, &depthStencilSRVDesc, &this->_shadowMapArraySRV);
			if (FAILED(hr)) goto out;

			if (g_ShadowMapping.bCSMEnabled) {
				// Inherit previous values
				step = "_csmArraySRV";
				depthStencilSRVDesc.Texture2DArray.ArraySize = MAX_CSM_LEVELS;
				hr = this->_d3dDevice->CreateShaderResourceView(this->_csmArray, &depthStencilSRVDesc, &this->_csmArraySRV);
				if (FAILED(hr)) goto out;
			}

			/*
			step = "_shadowMapSingleSRV";
			depthStencilSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
			depthStencilSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			depthStencilSRVDesc.Texture2D.MipLevels = depthStencilDesc.MipLevels;
			depthStencilSRVDesc.Texture2D.MostDetailedMip = 0;
			hr = this->_d3dDevice->CreateShaderResourceView(this->_shadowMap, &depthStencilSRVDesc, &this->_shadowMapSingleSRV);
			if (FAILED(hr)) goto out;
			*/
		}
	}

	/* viewport */
	if (SUCCEEDED(hr))
	{
		step = "Viewport";
		this->_d3dDeviceContext->OMSetRenderTargets(1, this->_renderTargetView.GetAddressOf(), this->_depthStencilViewL.Get());

		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (float)this->_backbufferWidth;
		viewport.Height = (float)this->_backbufferHeight;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;

		this->InitViewport(&viewport);
	}

	if (SUCCEEDED(hr))
	{
		step = "Texture2D displayWidth x displayHeight";

		this->_mainDisplayTextureBpp = (this->_displayBpp == 2 && this->_use16BppMainDisplayTexture) ? 2 : 4;

		D3D11_TEXTURE2D_DESC textureDesc;
		textureDesc.Width = this->_displayWidth;
		textureDesc.Height = this->_displayHeight;
		textureDesc.Format = this->_mainDisplayTextureBpp == 2 ? DXGI_FORMAT_B5G6R5_UNORM : BACKBUFFER_FORMAT;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		hr = this->_d3dDevice->CreateTexture2D(&textureDesc, nullptr, &this->_mainDisplayTexture);

		if (SUCCEEDED(hr))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc{};
			textureViewDesc.Format = textureDesc.Format;
			textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			textureViewDesc.Texture2D.MipLevels = 1;
			textureViewDesc.Texture2D.MostDetailedMip = 0;

			hr = this->_d3dDevice->CreateShaderResourceView(this->_mainDisplayTexture, &textureViewDesc, &this->_mainDisplayTextureView);
		}
	}

	this->_displayTempWidth = 0;
	this->_displayTempHeight = 0;
	this->_displayTempBpp = 0;

	if (SUCCEEDED(hr))
	{
		// https://docs.microsoft.com/en-us/windows/win32/direct2d/improving-direct2d-performance
		// https://stackoverflow.com/questions/4055456/is-tdirect2dcanvas-slow-or-am-i-doing-something-wrong
		// https://www.gamedev.net/forums/topic/552948-direct2d-performance-issue/
		// https://stackoverflow.com/questions/21981886/how-to-select-a-the-gpu-with-direct2ds-d2d1createfactory
		// https://docs.microsoft.com/en-us/windows/win32/direct2d/direct2d-and-direct3d-interoperation-overview?redirectedfrom=MSDN
		step = "CreateDxgiSurfaceRenderTarget";
		ComPtr<IDXGISurface> surface;
		ComPtr<IDXGISurface> offscreenSurface;
		ComPtr<IDXGISurface> DCSurface;
		ComPtr<IDXGISurface> enhancedHUDSurface;
		// The logic for the Dynamic Cockpit expects the text to be in its own
		// buffer so that the alpha can be fixed and the text can be blended
		// properly. So, instead of using _offscreenBuffer, we always write to
		// _DCTextMSAA instead and then blend that to the _offscreenBuffer as if
		// DC was always enabled.
		hr = this->_DCTextMSAA.As(&surface);
		// This surface is used with _d2d1OffscreenRenderTarget to render directly to the
		// _offscreenBuffer. This is used to render things like the brackets that shouldn't
		// be captured in the DC buffers. Currently only used in PrimarySurface::RenderBracket()
		if (g_bUseSteamVR)
			hr = this->_offscreenBufferHd.As(&offscreenSurface);
		else
			hr = this->_offscreenBuffer.As(&offscreenSurface);

		if (g_EnhancedHUDData.Enabled)
			this->_enhancedHUDBuffer.As(&enhancedHUDSurface);

		// This surface can be used to render directly to the DC foreground buffer
		//if (g_bDynCockpitEnabled)
			hr = this->_offscreenBufferDynCockpit.As(&DCSurface);
		//else
		//	hr = this->_offscreenBuffer.As(&DCSurface);

		if (SUCCEEDED(hr))
		{
			auto properties = D2D1::RenderTargetProperties();
			properties.pixelFormat = D2D1::PixelFormat(BACKBUFFER_FORMAT, D2D1_ALPHA_MODE_PREMULTIPLIED);			
			hr = this->_d2d1Factory->CreateDxgiSurfaceRenderTarget(surface, properties, &this->_d2d1RenderTarget);
			hr = this->_d2d1Factory->CreateDxgiSurfaceRenderTarget(DCSurface, properties, &this->_d2d1DCRenderTarget);
			hr = this->_d2d1Factory->CreateDxgiSurfaceRenderTarget(offscreenSurface, properties, &this->_d2d1OffscreenRenderTarget);
			if (g_EnhancedHUDData.Enabled)
				hr = this->_d2d1Factory->CreateDxgiSurfaceRenderTarget(enhancedHUDSurface, properties, &this->_d2d1EnhancedHUDRenderTarget);

			if (SUCCEEDED(hr))
			{
				this->_d2d1RenderTarget->SetAntialiasMode(g_config.Geometry2DAntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
				this->_d2d1RenderTarget->SetTextAntialiasMode(g_config.Text2DAntiAlias ? D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);

				this->_d2d1OffscreenRenderTarget->SetAntialiasMode(g_config.Geometry2DAntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
				this->_d2d1OffscreenRenderTarget->SetTextAntialiasMode(g_config.Text2DAntiAlias ? D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);

				if (g_EnhancedHUDData.Enabled)
				{
					this->_d2d1EnhancedHUDRenderTarget->SetAntialiasMode(g_config.Geometry2DAntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
					// The Cleartype mode destroys the alpha component (the alpha is _always_ 0 after rendering the text).
					// For the Enhanced HUD, we might want to add transparency or even render a dark rect below the text
					// to make it easier to read. So, let's use a different antialias mode that writes the alpha channel:
					this->_d2d1EnhancedHUDRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
				}

				this->_d2d1DCRenderTarget->SetAntialiasMode(g_config.Geometry2DAntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
				this->_d2d1DCRenderTarget->SetTextAntialiasMode(g_config.Text2DAntiAlias ? D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE : D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		step = "CreateDrawingStateBlock";
		hr = this->_d2d1Factory->CreateDrawingStateBlock(&this->_d2d1DrawingStateBlock);
	}

	if (SUCCEEDED(hr))
	{
		D3dRendererFlightStart();
	}

out:
	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			char text[512];
			close_error_file();
			strcpy_s(text, step);
			strcat_s(text, "\n");
			strcat_s(text, _com_error(hr).ErrorMessage());

			MessageBox(nullptr, text, __FUNCTION__, MB_ICONERROR);
		}

		messageShown = true;
	}

	return hr;
}

HRESULT DeviceResources::LoadMainResources()
{
	HRESULT hr = S_OK;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_MainVertexShader, sizeof(g_MainVertexShader), nullptr, &_mainVertexShader)))
		return hr;

	const D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	if (FAILED(hr = this->_d3dDevice->CreateInputLayout(vertexLayoutDesc, ARRAYSIZE(vertexLayoutDesc), g_MainVertexShader, sizeof(g_MainVertexShader), &_mainInputLayout)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_MainPixelShader, sizeof(g_MainPixelShader), nullptr, &_mainPixelShader)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SteamVRMirrorPixelShader, sizeof(g_SteamVRMirrorPixelShader), nullptr, &_steamVRMirrorPixelShader)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BarrelPixelShader, sizeof(g_BarrelPixelShader), nullptr, &_barrelPixelShader)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SimpleResizePS, sizeof(g_SimpleResizePS), nullptr, &_simpleResizePS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_DeathStarShader, sizeof(g_DeathStarShader), nullptr, &_deathStarPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperEntry, sizeof(g_HyperEntry), nullptr, &_hyperEntryPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperExit, sizeof(g_HyperExit), nullptr, &_hyperExitPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperTunnel, sizeof(g_HyperTunnel), nullptr, &_hyperTunnelPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperZoom, sizeof(g_HyperZoom), nullptr, &_hyperZoomPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunShader, sizeof(g_SunShader), nullptr, &_sunShaderPS)))
		return hr;

	if (g_bUseSteamVR || g_bEnableVR)
	{
		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_MainVertexShaderVR, sizeof(g_MainVertexShaderVR), nullptr, &_mainVertexShaderVR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperComposeVR, sizeof(g_HyperComposeVR), nullptr, &_hyperComposePS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_SpeedEffectVertexShaderVR, sizeof(g_SpeedEffectVertexShaderVR), nullptr, &_speedEffectVS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SpeedEffectPixelShaderVR, sizeof(g_SpeedEffectPixelShaderVR), nullptr, &_speedEffectPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_AddGeometryVertexShader, sizeof(g_AddGeometryVertexShader), nullptr, &_addGeomVS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_AddGeometryComposePixelShader, sizeof(g_AddGeometryComposePixelShader), nullptr, &_addGeomComposePS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_FXAA_VR, sizeof(g_FXAA_VR), nullptr, &_fxaaPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareShaderVR, sizeof(g_SunFlareShaderVR), nullptr, &_sunFlareShaderPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareComposeVR, sizeof(g_SunFlareComposeVR), nullptr, &_sunFlareComposeShaderPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ExternalHUDShaderVR, sizeof(g_ExternalHUDShaderVR), nullptr, &_externalHUDPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HeadLightsPS_VR, sizeof(g_HeadLightsPS_VR), nullptr, &_headLightsPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_LaserPointerVR, sizeof(g_LaserPointerVR), nullptr, &_laserPointerPS_VR)))
			return hr;
	}
	else
	{
		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperCompose, sizeof(g_HyperCompose), nullptr, &_hyperComposePS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_SpeedEffectVertexShader, sizeof(g_SpeedEffectVertexShader), nullptr, &_speedEffectVS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SpeedEffectPixelShader, sizeof(g_SpeedEffectPixelShader), nullptr, &_speedEffectPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_FXAA, sizeof(g_FXAA), nullptr, &_fxaaPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareShader, sizeof(g_SunFlareShader), nullptr, &_sunFlareShaderPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareCompose, sizeof(g_SunFlareCompose), nullptr, &_sunFlareComposeShaderPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ExternalHUDShader, sizeof(g_ExternalHUDShader), nullptr, &_externalHUDPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HeadLightsPS, sizeof(g_HeadLightsPS), nullptr, &_headLightsPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_LaserPointer, sizeof(g_LaserPointer), nullptr, &_laserPointerPS)))
			return hr;
	}

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_AddGeometryPixelShader, sizeof(g_AddGeometryPixelShader), nullptr, &_addGeomPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HeadLightsSSAOPS, sizeof(g_HeadLightsSSAOPS), nullptr, &_headLightsSSAOPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ShadowMapPS, sizeof(g_ShadowMapPS), nullptr, &_shadowMapPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_ShadowMapVS, sizeof(g_ShadowMapVS), nullptr, &_shadowMapVS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_HangarShadowMapVS, sizeof(g_HangarShadowMapVS), nullptr, &_hangarShadowMapVS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_XwaD3dCSMVertexShader, sizeof(g_XwaD3dCSMVertexShader), nullptr, &_csmVS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_EdgeDetector, sizeof(g_EdgeDetector), nullptr, &_edgeDetectorPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_StarDebug, sizeof(g_StarDebug), nullptr, &_starDebugPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_LavaPixelShader, sizeof(g_LavaPixelShader), nullptr, &_lavaPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ExplosionShader, sizeof(g_ExplosionShader), nullptr, &_explosionPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_AlphaToBloomPS, sizeof(g_AlphaToBloomPS), nullptr, &_alphaToBloomPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderNoGlass, sizeof(g_PixelShaderNoGlass), nullptr, &_noGlassPS)))
		return hr;

	if (g_bBloomEnabled)
	{
		if (g_bUseSteamVR || g_bEnableVR)
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomHGaussPS_VR, sizeof(g_BloomHGaussPS_VR), nullptr, &_bloomHGaussPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomVGaussPS_VR, sizeof(g_BloomVGaussPS_VR), nullptr, &_bloomVGaussPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomBufferAddPS_VR, sizeof(g_BloomBufferAddPS_VR), nullptr, &_bloomBufferAddPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomCombinePS_VR, sizeof(g_BloomCombinePS_VR), nullptr, &_bloomCombinePS_VR)))
				return hr;
		}
		else
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomHGaussPS, sizeof(g_BloomHGaussPS), nullptr, &_bloomHGaussPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomVGaussPS, sizeof(g_BloomVGaussPS), nullptr, &_bloomVGaussPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomBufferAddPS, sizeof(g_BloomBufferAddPS), nullptr, &_bloomBufferAddPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomCombinePS, sizeof(g_BloomCombinePS), nullptr, &_bloomCombinePS)))
				return hr;
		}
	}

	if (g_bAOEnabled)
	{

		if (g_bUseSteamVR || g_bEnableVR)
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOBlurPixelShaderVR, sizeof(g_SSAOBlurPixelShaderVR), nullptr, &_ssaoBlurPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOPixelShaderVR, sizeof(g_SSAOPixelShaderVR), nullptr, &_ssaoPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOBlurVR, sizeof(g_SSDOBlurVR), nullptr, &_ssdoBlurPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAddVR, sizeof(g_SSDOAddVR), nullptr, &_ssdoAddPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirectVR, sizeof(g_SSDODirectVR), nullptr, &_ssdoDirectPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PBRAddVR, sizeof(g_PBRAddVR), nullptr, &_pbrAddPS_VR)))
				return hr;
		}
		else
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOBlurPixelShader, sizeof(g_SSAOBlurPixelShader), nullptr, &_ssaoBlurPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOPixelShader, sizeof(g_SSAOPixelShader), nullptr, &_ssaoPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOBlur, sizeof(g_SSDOBlur), nullptr, &_ssdoBlurPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAdd, sizeof(g_SSDOAdd), nullptr, &_ssdoAddPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirect, sizeof(g_SSDODirect), nullptr, &_ssdoDirectPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PBRAdd, sizeof(g_PBRAdd), nullptr, &_pbrAddPS)))
				return hr;
		}

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOAddPixelShader, sizeof(g_SSAOAddPixelShader), nullptr, &_ssaoAddPS)))
			return hr;

		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirectBentNormals, sizeof(g_SSDODirectBentNormals), nullptr, &_ssdoDirectBentNormalsPS)))
		//	return hr;
		
		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirectHDR, sizeof(g_SSDODirectHDR), nullptr, &_ssdoDirectHDRPS)))
		//	return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOIndirect, sizeof(g_SSDOIndirect), nullptr, &_ssdoIndirectPS)))
			return hr;

		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAddHDR, sizeof(g_SSDOAddHDR), nullptr, &_ssdoAddHDRPS)))
		//	return hr;

		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAddBentNormals, sizeof(g_SSDOAddBentNormals), nullptr, &_ssdoAddBentNormalsPS)))
		//	return hr;
	}

	if (this->_d3dFeatureLevel >= D3D_FEATURE_LEVEL_10_0)
	{
		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_MainPixelShaderBpp2ColorKey20, sizeof(g_MainPixelShaderBpp2ColorKey20), nullptr, &_mainPixelShaderBpp2ColorKey20)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_MainPixelShaderBpp2ColorKey00, sizeof(g_MainPixelShaderBpp2ColorKey00), nullptr, &_mainPixelShaderBpp2ColorKey00)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_MainPixelShaderBpp4ColorKey20, sizeof(g_MainPixelShaderBpp4ColorKey20), nullptr, &_mainPixelShaderBpp4ColorKey20)))
			return hr;
	}

	D3D11_RASTERIZER_DESC rsDesc;
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthBias = 0;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	rsDesc.DepthClipEnable = TRUE;
	rsDesc.ScissorEnable = FALSE;
	rsDesc.MultisampleEnable = TRUE;
	rsDesc.AntialiasedLineEnable = FALSE;

	if (FAILED(hr = this->_d3dDevice->CreateRasterizerState(&rsDesc, &this->_mainRasterizerState)))
		return hr;

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = this->_useAnisotropy ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxAnisotropy = this->_useAnisotropy ? this->GetMaxAnisotropy() : 1;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = FLT_MAX;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;

	if (FAILED(hr = this->_d3dDevice->CreateSamplerState(&samplerDesc, &this->_mainSamplerState)))
		return hr;

	// This sampler is used when doing the SSAO/Deferred Pass in PrimarySurface so that samples
	// can be mirrrored across the boundary of the viewport.
	samplerDesc.Filter = this->_useAnisotropy ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxAnisotropy = this->_useAnisotropy ? this->GetMaxAnisotropy() : 1;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = FLT_MAX;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;

	if (FAILED(hr = this->_d3dDevice->CreateSamplerState(&samplerDesc, &_mirrorSamplerState)))
		return hr;

	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(hr = this->_d3dDevice->CreateBlendState(&blendDesc, &this->_mainBlendState)))
		return hr;

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depthDesc.StencilEnable = FALSE;

	if (FAILED(hr = this->_d3dDevice->CreateDepthStencilState(&depthDesc, &this->_mainDepthState)))
		return hr;

	MainVertex vertices[4] =
	{
		MainVertex(-1, -1, 0, 1),
		MainVertex( 1, -1, 1, 1),
		MainVertex( 1,  1, 1, 0),
		MainVertex(-1,  1, 0, 0),
	};

	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.ByteWidth = sizeof(MainVertex) * ARRAYSIZE(vertices);
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	vertexBufferData.pSysMem = vertices;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &this->_mainVertexBuffer)))
		return hr;
	this->_steamVRPresentBuffer = NULL;

	WORD indices[6] =
	{
		0, 1, 2,
		0, 2, 3,
	};

	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	indexBufferData.pSysMem = indices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;

	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &this->_mainIndexBuffer)))
		return hr;

	return hr;
}

HRESULT DeviceResources::LoadResources()
{
	HRESULT hr = S_OK;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_VertexShader, sizeof(g_VertexShader), nullptr, &_vertexShader)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_PassthroughVertexShader, sizeof(g_PassthroughVertexShader), nullptr, &_passthroughVertexShader)))
		return hr;

	const D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	if (FAILED(hr = this->_d3dDevice->CreateInputLayout(vertexLayoutDesc, ARRAYSIZE(vertexLayoutDesc), g_VertexShader, sizeof(g_VertexShader), &_inputLayout)))
		return hr;

	// Input layout specific for shadowmap, as the input (shadow OBJ) only contains position information
	const D3D11_INPUT_ELEMENT_DESC shadowVertexLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	if (FAILED(hr = this->_d3dDevice->CreateInputLayout(shadowVertexLayoutDesc, ARRAYSIZE(shadowVertexLayoutDesc), g_ShadowMapVS, sizeof(g_ShadowMapVS), &_shadowMapInputLayout)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderTexture, sizeof(g_PixelShaderTexture), nullptr, &_pixelShaderTexture)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderAnim, sizeof(g_PixelShaderAnim), nullptr, &_pixelShaderAnim)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderAnimDAT, sizeof(g_PixelShaderAnimDAT), nullptr, &_pixelShaderAnimDAT)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderGreeble, sizeof(g_PixelShaderGreeble), nullptr, &_pixelShaderGreeble)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_LevelsPS, sizeof(g_LevelsPS), nullptr, &_levelsPS)))
		return hr;

	//if (g_bDynCockpitEnabled)
	{
		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderDC, sizeof(g_PixelShaderDC), nullptr, &_pixelShaderDC)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderDCHolo, sizeof(g_PixelShaderDCHolo), nullptr, &_pixelShaderDCHolo)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderEmptyDC, sizeof(g_PixelShaderEmptyDC), nullptr, &_pixelShaderEmptyDC)))
			return hr;
	}

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderHUD, sizeof(g_PixelShaderHUD), nullptr, &_pixelShaderHUD)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_EnhancedHudPS, sizeof(g_EnhancedHudPS), nullptr, &_enhancedHudPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderSolid, sizeof(g_PixelShaderSolid), nullptr, &_pixelShaderSolid)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderClearBox, sizeof(g_PixelShaderClearBox), nullptr, &_pixelShaderClearBox)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BarrelPixelShader, sizeof(g_BarrelPixelShader), nullptr, &_barrelPixelShader)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SimpleResizePS, sizeof(g_SimpleResizePS), nullptr, &_simpleResizePS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SingleBarrelPixelShader, sizeof(g_SingleBarrelPixelShader), nullptr, &_singleBarrelPixelShader)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_DeathStarShader, sizeof(g_DeathStarShader), nullptr, &_deathStarPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperEntry, sizeof(g_HyperEntry), nullptr, &_hyperEntryPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperExit, sizeof(g_HyperExit), nullptr, &_hyperExitPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperTunnel, sizeof(g_HyperTunnel), nullptr, &_hyperTunnelPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperZoom, sizeof(g_HyperZoom), nullptr, &_hyperZoomPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunShader, sizeof(g_SunShader), nullptr, &_sunShaderPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SpeedEffectPixelShader, sizeof(g_SpeedEffectPixelShader), nullptr, &_speedEffectPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderVRGeom, sizeof(g_PixelShaderVRGeom), nullptr, &_pixelShaderVRGeom)))
		return hr;

	if (g_bEnableVR)
	{
		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_SBSVertexShader, sizeof(g_SBSVertexShader), nullptr, &_sbsVertexShader)))
			return hr;
	}

	if (g_bUseSteamVR || g_bEnableVR)
	{
		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_DATVertexShaderVR, sizeof(g_DATVertexShaderVR), nullptr, &_datVertexShaderVR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperComposeVR, sizeof(g_HyperComposeVR), nullptr, &_hyperComposePS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_SpeedEffectVertexShaderVR, sizeof(g_SpeedEffectVertexShaderVR), nullptr, &_speedEffectVS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_AddGeometryVertexShader, sizeof(g_AddGeometryVertexShader), nullptr, &_addGeomVS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_AddGeometryComposePixelShader, sizeof(g_AddGeometryComposePixelShader), nullptr, &_addGeomComposePS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_FXAA_VR, sizeof(g_FXAA_VR), nullptr, &_fxaaPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareShaderVR, sizeof(g_SunFlareShaderVR), nullptr, &_sunFlareShaderPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareComposeVR, sizeof(g_SunFlareComposeVR), nullptr, &_sunFlareComposeShaderPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ExternalHUDShaderVR, sizeof(g_ExternalHUDShaderVR), nullptr, &_externalHUDPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HeadLightsPS_VR, sizeof(g_HeadLightsPS_VR), nullptr, &_headLightsPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_RTShadowMaskPS_VR, sizeof(g_RTShadowMaskPS_VR), nullptr, &_rtShadowMaskPS_VR)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_LaserPointerVR, sizeof(g_LaserPointerVR), nullptr, &_laserPointerPS_VR)))
			return hr;
	}
	else
	{
		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HyperCompose, sizeof(g_HyperCompose), nullptr, &_hyperComposePS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_SpeedEffectVertexShader, sizeof(g_SpeedEffectVertexShader), nullptr, &_speedEffectVS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_FXAA, sizeof(g_FXAA), nullptr, &_fxaaPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareShader, sizeof(g_SunFlareShader), nullptr, &_sunFlareShaderPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SunFlareCompose, sizeof(g_SunFlareCompose), nullptr, &_sunFlareComposeShaderPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ExternalHUDShader, sizeof(g_ExternalHUDShader), nullptr, &_externalHUDPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HeadLightsPS, sizeof(g_HeadLightsPS), nullptr, &_headLightsPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_RTShadowMaskPS, sizeof(g_RTShadowMaskPS), nullptr, &_rtShadowMaskPS)))
			return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_LaserPointer, sizeof(g_LaserPointer), nullptr, &_laserPointerPS)))
			return hr;
	}

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_AddGeometryPixelShader, sizeof(g_AddGeometryPixelShader), nullptr, &_addGeomPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_HeadLightsSSAOPS, sizeof(g_HeadLightsSSAOPS), nullptr, &_headLightsSSAOPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ShadowMapPS, sizeof(g_ShadowMapPS), nullptr, &_shadowMapPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_ShadowMapVS, sizeof(g_ShadowMapVS), nullptr, &_shadowMapVS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_HangarShadowMapVS, sizeof(g_HangarShadowMapVS), nullptr, &_hangarShadowMapVS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreateVertexShader(g_XwaD3dCSMVertexShader, sizeof(g_XwaD3dCSMVertexShader), nullptr, &_csmVS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_EdgeDetector, sizeof(g_EdgeDetector), nullptr, &_edgeDetectorPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_StarDebug, sizeof(g_StarDebug), nullptr, &_starDebugPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_LavaPixelShader, sizeof(g_LavaPixelShader), nullptr, &_lavaPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_ExplosionShader, sizeof(g_ExplosionShader), nullptr, &_explosionPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_AlphaToBloomPS, sizeof(g_AlphaToBloomPS), nullptr, &_alphaToBloomPS)))
		return hr;

	if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PixelShaderNoGlass, sizeof(g_PixelShaderNoGlass), nullptr, &_noGlassPS)))
		return hr;

	if (g_bBloomEnabled)
	{
		if (g_bUseSteamVR || g_bEnableVR)
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomHGaussPS_VR, sizeof(g_BloomHGaussPS_VR), nullptr, &_bloomHGaussPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomVGaussPS_VR, sizeof(g_BloomVGaussPS_VR), nullptr, &_bloomVGaussPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomBufferAddPS_VR, sizeof(g_BloomBufferAddPS_VR), nullptr, &_bloomBufferAddPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomCombinePS_VR, sizeof(g_BloomCombinePS_VR), nullptr, &_bloomCombinePS_VR)))
				return hr;
		}
		else
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomHGaussPS, sizeof(g_BloomHGaussPS), nullptr, &_bloomHGaussPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomVGaussPS, sizeof(g_BloomVGaussPS), nullptr, &_bloomVGaussPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomBufferAddPS, sizeof(g_BloomBufferAddPS), nullptr, &_bloomBufferAddPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_BloomCombinePS, sizeof(g_BloomCombinePS), nullptr, &_bloomCombinePS)))
				return hr;
		}
	}

	if (g_bAOEnabled)
	{
		if (g_bUseSteamVR || g_bEnableVR)
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOBlurPixelShaderVR, sizeof(g_SSAOBlurPixelShaderVR), nullptr, &_ssaoBlurPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOPixelShaderVR, sizeof(g_SSAOPixelShaderVR), nullptr, &_ssaoPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOBlurVR, sizeof(g_SSDOBlurVR), nullptr, &_ssdoBlurPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAddVR, sizeof(g_SSDOAddVR), nullptr, &_ssdoAddPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirectVR, sizeof(g_SSDODirectVR), nullptr, &_ssdoDirectPS_VR)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PBRAddVR, sizeof(g_PBRAddVR), nullptr, &_pbrAddPS_VR)))
				return hr;
		}
		else
		{
			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOBlurPixelShader, sizeof(g_SSAOBlurPixelShader), nullptr, &_ssaoBlurPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOPixelShader, sizeof(g_SSAOPixelShader), nullptr, &_ssaoPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOBlur, sizeof(g_SSDOBlur), nullptr, &_ssdoBlurPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAdd, sizeof(g_SSDOAdd), nullptr, &_ssdoAddPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirect, sizeof(g_SSDODirect), nullptr, &_ssdoDirectPS)))
				return hr;

			if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_PBRAdd, sizeof(g_PBRAdd), nullptr, &_pbrAddPS)))
				return hr;
		}

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSAOAddPixelShader, sizeof(g_SSAOAddPixelShader), nullptr, &_ssaoAddPS)))
			return hr;

		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirectBentNormals, sizeof(g_SSDODirectBentNormals), nullptr, &_ssdoDirectBentNormalsPS)))
		//	return hr;

		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDODirectHDR, sizeof(g_SSDODirectHDR), nullptr, &_ssdoDirectHDRPS)))
		//	return hr;

		if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOIndirect, sizeof(g_SSDOIndirect), nullptr, &_ssdoIndirectPS)))
			return hr;

		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAddHDR, sizeof(g_SSDOAddHDR), nullptr, &_ssdoAddHDRPS)))
		//	return hr;

		//if (FAILED(hr = this->_d3dDevice->CreatePixelShader(g_SSDOAddBentNormals, sizeof(g_SSDOAddBentNormals), nullptr, &_ssdoAddBentNormalsPS)))
		//	return hr;
	}

	D3D11_RASTERIZER_DESC rsDesc;
	rsDesc.FillMode = g_config.WireframeFillMode ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FrontCounterClockwise = TRUE;
	rsDesc.DepthBias = 0;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	rsDesc.DepthClipEnable = TRUE;
	rsDesc.ScissorEnable = FALSE;
	rsDesc.MultisampleEnable = TRUE;
	rsDesc.AntialiasedLineEnable = FALSE;

	if (FAILED(hr = this->_d3dDevice->CreateRasterizerState(&rsDesc, &this->_rasterizerState)))
		return hr;

	/*
	// Create the rasterizer state for shadow mapping
	log_debug("[DBG] [SHW] Create SM RState. Bias: %0.3f", g_ShadowMapVSCBuffer.sm_bias);
	rsDesc.DepthBias = g_ShadowMapping.DepthBias;
	rsDesc.DepthBiasClamp = g_ShadowMapping.DepthBiasClamp;
	rsDesc.SlopeScaledDepthBias = g_ShadowMapping.SlopeScaledDepthBias;
	rsDesc.MultisampleEnable = FALSE;
	if (FAILED(hr = this->_d3dDevice->CreateRasterizerState(&rsDesc, &this->_sm_rasterizerState)))
		return hr;
	*/

	/*
	D3D11_SAMPLER_DESC SamDescShad =
	{
		D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,// D3D11_FILTER Filter;
		D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressU;
		D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressV;
		D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressW;
		0,//FLOAT MipLODBias;
		0,//UINT MaxAnisotropy;
		D3D11_COMPARISON_LESS , //D3D11_COMPARISON_FUNC ComparisonFunc;
		0.0,0.0,0.0,0.0,//FLOAT BorderColor[ 4 ];
		0,//FLOAT MinLOD;
		0//FLOAT MaxLOD;   
	};
	*/

	// Create Sampler State for Shadow Mapping PCF
	D3D11_SAMPLER_DESC samplerDescPCF;
	ZeroMemory(&samplerDescPCF, sizeof(samplerDescPCF));
	samplerDescPCF.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	//samplerDescPCF.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samplerDescPCF.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDescPCF.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDescPCF.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDescPCF.BorderColor[0] = 1.0f;
	samplerDescPCF.BorderColor[1] = 1.0f;
	samplerDescPCF.BorderColor[2] = 1.0f;
	samplerDescPCF.BorderColor[3] = 1.0f;
	samplerDescPCF.ComparisonFunc = D3D11_COMPARISON_LESS;
	//samplerDescPCF.ComparisonFunc = D3D11_COMPARISON_GREATER_EQUAL;

	if (FAILED(hr = this->_d3dDevice->CreateSamplerState(&samplerDescPCF, &this->_shadowPCFSamplerState)))
		return hr;
	else
		log_debug("[DBG] [SHW] PCF Sampler State created");

	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	constantBufferDesc.ByteWidth = 96;
	// This was the original constant buffer. Now it's called _VSConstantBuffer
	static_assert(sizeof(VertexShaderCBuffer) == 96, "sizeof(VertexShaderCBuffer) must be 96");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_VSConstantBuffer)))
		return hr;

	constantBufferDesc.ByteWidth = 272; // 4x4 elems in a matrix = 16 elems. Each elem is a float, so 4 bytes * 16 = 64 bytes per matrix. This is a multiple of 16
	// 256 bytes is 2+1+1 matrices + 16 bytes for extra information
	static_assert(sizeof(VertexShaderMatrixCB) == 272, "sizeof(VertexShaderMatrixCB) must be 272");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_VSMatrixBuffer)))
		return hr;

	constantBufferDesc.ByteWidth = 816; // 4x4 elems in a matrix = 16 elems. Each elem is a float, so 4 bytes * 16 = 64 bytes per matrix. This is a multiple of 16
	static_assert(sizeof(ShadowMapVertexShaderMatrixCB) == 816, "sizeof(ShadowMapVertexShaderMatrixCB) must be 816");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_shadowMappingVSConstantBuffer)))
		return hr;
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_shadowMappingPSConstantBuffer)))
		return hr;

	constantBufferDesc.ByteWidth = 48;
	static_assert(sizeof(MetricReconstructionCB) == 48, "sizeof(MetricReconstructionCB) must be 48");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_metricRecVSConstantBuffer)))
		return hr;
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_metricRecPSConstantBuffer)))
		return hr;

	// Create the constant buffer for the (3D) textured pixel shader
	constantBufferDesc.ByteWidth = 224;
	static_assert(sizeof(PixelShaderCBuffer) == 224, "sizeof(PixelShaderCBuffer) must be 224");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_PSConstantBuffer)))
		return hr;

	// Create the constant buffer for the (3D) textured pixel shader -- Dynamic Cockpit data
	constantBufferDesc.ByteWidth = 464;
	static_assert(sizeof(DCPixelShaderCBuffer) == 464, "sizeof(DCPixelShaderCBuffer) must be 464");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_PSConstantBufferDC)))
		return hr;

	// Create the constant buffer for the barrel pixel shader
	constantBufferDesc.ByteWidth = 16;
	static_assert(sizeof(BarrelPixelShaderCBuffer) == 16, "sizeof(BarrelPixelShaderCBuffer) must be 16");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_barrelConstantBuffer)))
		return hr;

	// Create the constant buffer for the bloom pixel shader
	constantBufferDesc.ByteWidth = 48;
	static_assert(sizeof(BloomPixelShaderCBuffer) == 48, "sizeof(BloomPixelShaderCBuffer) must be 48");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_bloomConstantBuffer)))
		return hr;

	// Create the Hyperspace (ShaderToy) constant buffer
	constantBufferDesc.ByteWidth = 272;
	static_assert(sizeof(ShadertoyCBuffer) == 272, "sizeof(ShadertoyCBuffer) must be 272");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_hyperspaceConstantBuffer)))
		return hr;

	// Laser Pointer (Active Cockpit) constant buffer
	constantBufferDesc.ByteWidth = 288;
	static_assert(sizeof(LaserPointerCBuffer) == 288, "sizeof(LaserPointerCBuffer) must be 288");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_laserPointerConstantBuffer)))
		return hr;

	// Create the constant buffer for the SSAO pixel shader
	constantBufferDesc.ByteWidth = 176;
	static_assert(sizeof(SSAOPixelShaderCBuffer) == 176, "sizeof(SSAOPixelShaderCBuffer) must be 176");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_ssaoConstantBuffer)))
		return hr;

	constantBufferDesc.ByteWidth = 1120;
	static_assert(sizeof(PSShadingSystemCB) == 1120, "sizeof(PSShadingSystemCB) must be 1120");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_shadingSysBuffer)))
		return hr;

	// Create the constant buffer for the main pixel shader
	constantBufferDesc.ByteWidth = 32;
	static_assert(sizeof(MainShadersCBuffer) == 32, "sizeof(MainShadersCBStruct) must be 32");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &this->_mainShadersConstantBuffer)))
		return hr;

	// Create the constant buffer for the main pixel shader
	constantBufferDesc.ByteWidth = 64;
	static_assert(sizeof(OPTMeshTransformCBuffer) == 64, "sizeof(OPTMeshTransformCBuffer) must be 64");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &_OPTMeshTransformCB)))
		return hr;

	// Create the constant buffer for the ray-tracer
	constantBufferDesc.ByteWidth = 48;
	static_assert(sizeof(RTConstantsBuffer) == 48, "sizeof(RTConstantsBuffer) must be 48");
	if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &_RTConstantsBuffer)))
		return hr;

	// Create the constant buffer for the VR Geometry
	//if (g_bUseSteamVR)
	{
		constantBufferDesc.ByteWidth = 224;
		static_assert(sizeof(VRGeometryCBuffer) == 224, "sizeof(VRGeometryCBuffer) must be 224");
		if (FAILED(hr = this->_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &_VRGeometryCBuffer)))
			return hr;
	}

	log_debug("[DBG] [MAT] Initializing OPTnames and Materials");
	InitOPTnames();
	InitCraftMaterials();
	// Apply the external FOV parameters
	// LoadFocalLength(); // This doesn't work: looks like XWA overwrites these params after this call.
	return hr;
}

void DeviceResources::InitInputLayout(ID3D11InputLayout* inputLayout)
{
	static ID3D11InputLayout* currentInputLayout = nullptr;

	//if (inputLayout != currentInputLayout)
	{
		currentInputLayout = inputLayout;
		this->_d3dDeviceContext->IASetInputLayout(inputLayout);
	}
}

void DeviceResources::InitVertexShader(ID3D11VertexShader* vertexShader)
{
	static ID3D11VertexShader* currentVertexShader = nullptr;

	//if (vertexShader != currentVertexShader)
	{
		currentVertexShader = vertexShader;
		this->_d3dDeviceContext->VSSetShader(vertexShader, nullptr, 0);
	}
}

void DeviceResources::InitPixelShader(ID3D11PixelShader* pixelShader)
{
	//if (pixelShader != _currentPixelShader)
	{
		_currentPixelShader = pixelShader;
		this->_d3dDeviceContext->PSSetShader(pixelShader, nullptr, 0);
	}
}

ID3D11PixelShader *DeviceResources::GetCurrentPixelShader()
{
	return _currentPixelShader;
}

void DeviceResources::InitGeometryShader(ID3D11GeometryShader* geometryShader)
{
	static ID3D11GeometryShader* currentGeometryShader = nullptr;

	if (geometryShader != currentGeometryShader)
	{
		currentGeometryShader = geometryShader;
		this->_d3dDeviceContext->GSSetShader(geometryShader, nullptr, 0);
	}
}

void DeviceResources::InitTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
	D3D_PRIMITIVE_TOPOLOGY currentTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	//if (topology != currentTopology)
	{
		currentTopology = topology;
		this->_d3dDeviceContext->IASetPrimitiveTopology(topology);
	}
}

void DeviceResources::InitRasterizerState(ID3D11RasterizerState* state)
{
	static ID3D11RasterizerState* currentState = nullptr;

	//if (state != currentState)
	{
		currentState = state;
		this->_d3dDeviceContext->RSSetState(state);
	}
}

void DeviceResources::InitPSShaderResourceView(ID3D11ShaderResourceView* texView, ID3D11ShaderResourceView* texView2)
{
	//static ID3D11ShaderResourceView* currentTexView = nullptr;
	//static ID3D11ShaderResourceView* currentTexView2 = nullptr;

	//if (texView != currentTexView || texView2 != currentTexView2) // Temporarily allow setting this all the time
	{
		ID3D11ShaderResourceView* view[2] = { texView , texView2 };

		this->_d3dDeviceContext->PSSetShaderResources(0, 2, view);
		//currentTexView = texView;
		//currentTexView2 = texView2;
	}
}

HRESULT DeviceResources::InitSamplerState(ID3D11SamplerState** sampler, D3D11_SAMPLER_DESC* desc)
{
	static ID3D11SamplerState** currentSampler = nullptr;
	static D3D11_SAMPLER_DESC currentDesc{};

	if (sampler == nullptr && desc != nullptr)
	{
		//if (memcmp(desc, &currentDesc, sizeof(D3D11_SAMPLER_DESC)) != 0)
		{
			HRESULT hr;
			ComPtr<ID3D11SamplerState> tempSampler;
			if (FAILED(hr = this->_d3dDevice->CreateSamplerState(desc, &tempSampler)))
				return hr;

			currentDesc = *desc;
			currentSampler = tempSampler.GetAddressOf();
			this->_d3dDeviceContext->PSSetSamplers(0, 1, currentSampler);
		}
	}
	else
	{
		//if (sampler != currentSampler)
		{
			currentDesc = {};
			currentSampler = sampler;
			this->_d3dDeviceContext->PSSetSamplers(0, 1, currentSampler);
		}
	}

	return S_OK;
}

HRESULT DeviceResources::InitBlendState(ID3D11BlendState* blend, D3D11_BLEND_DESC* desc)
{
	static ID3D11BlendState* currentBlend = nullptr;
	static D3D11_BLEND_DESC currentDesc{};

	if (blend == nullptr && desc != nullptr)
	{
		//if (memcmp(desc, &currentDesc, sizeof(D3D11_BLEND_DESC)) != 0)
		{
			HRESULT hr;
			ComPtr<ID3D11BlendState> tempBlend;
			if (FAILED(hr = this->_d3dDevice->CreateBlendState(desc, &tempBlend)))
				return hr;

			currentDesc = *desc;
			currentBlend = tempBlend;

			static const FLOAT factors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			UINT mask = 0xffffffff;
			this->_d3dDeviceContext->OMSetBlendState(currentBlend, factors, mask);
		}
	}
	else
	{
		//if (blend != currentBlend)
		{
			currentDesc = {};
			currentBlend = blend;

			static const FLOAT factors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			UINT mask = 0xffffffff;
			this->_d3dDeviceContext->OMSetBlendState(currentBlend, factors, mask);
		}
	}

	return S_OK;
}

HRESULT DeviceResources::InitDepthStencilState(ID3D11DepthStencilState* depthState, D3D11_DEPTH_STENCIL_DESC* desc, UINT stencilReference)
{
	static ID3D11DepthStencilState* currentDepthState = nullptr;
	static D3D11_DEPTH_STENCIL_DESC currentDesc{};

	if (depthState == nullptr && desc != nullptr)
	{
		//if (memcmp(desc, &currentDesc, sizeof(D3D11_DEPTH_STENCIL_DESC)) != 0)
		{
			HRESULT hr;
			ComPtr<ID3D11DepthStencilState> tempDepthState;
			if (FAILED(hr = this->_d3dDevice->CreateDepthStencilState(desc, &tempDepthState)))
				return hr;

			currentDesc = *desc;
			currentDepthState = tempDepthState;
			this->_d3dDeviceContext->OMSetDepthStencilState(currentDepthState, stencilReference);
		}
	}
	else
	{
		//if (depthState != currentDepthState)
		{
			currentDesc = {};
			currentDepthState = depthState;
			this->_d3dDeviceContext->OMSetDepthStencilState(currentDepthState, stencilReference);
		}
	}

	return S_OK;
}

void DeviceResources::InitVertexBuffer(ID3D11Buffer** buffer, UINT* stride, UINT* offset)
{
	static ID3D11Buffer** currentBuffer = nullptr;

	//if (buffer != currentBuffer)
	{
		currentBuffer = buffer;

		if (buffer)
		{
			this->_d3dDeviceContext->IASetVertexBuffers(0, 1, buffer, stride, offset);
		}
	}
}

void DeviceResources::InitIndexBuffer(ID3D11Buffer* buffer, bool isFormat32)
{
	static ID3D11Buffer* currentBuffer = nullptr;

	//if (buffer != currentBuffer)
	{
		currentBuffer = buffer;

		if (buffer)
		{
			this->_d3dDeviceContext->IASetIndexBuffer(buffer, isFormat32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);
		}
	}
}

void DeviceResources::InitViewport(D3D11_VIEWPORT* viewport)
{
	//static D3D11_VIEWPORT currentViewport{};

	// For the D3dRendererHook we're now switching between the old and new methods, but sometimes
	// the viewport isn't set properly. I'm disabling this check now to ensure that we always
	// set the viewport
	//if (memcmp(viewport, &currentViewport, sizeof(D3D11_VIEWPORT)) != 0)
	{
		//currentViewport = *viewport;
		this->_d3dDeviceContext->RSSetViewports(1, viewport);
	}
}

void DeviceResources::InitVSConstantBuffer3D(ID3D11Buffer** buffer, const VertexShaderCBuffer* vsCBuffer)
{
	_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, vsCBuffer, 0, 0);
	_d3dDeviceContext->VSSetConstantBuffers(1, 1, buffer);
}

void DeviceResources::InitVSConstantBufferMatrix(ID3D11Buffer** buffer, const VertexShaderMatrixCB* vsCBuffer)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, vsCBuffer, 0, 0);
	this->_d3dDeviceContext->VSSetConstantBuffers(2, 1, buffer);
}

void DeviceResources::InitPSConstantShadingSystem(ID3D11Buffer** buffer, const PSShadingSystemCB* psCBuffer)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psCBuffer, 0, 0);
	this->_d3dDeviceContext->PSSetConstantBuffers(4, 1, buffer);
}

void DeviceResources::InitVSConstantBuffer2D(ID3D11Buffer** buffer, const float parallax,
	const float aspect_ratio, const float scale, const float brightness, const float use_3D)
{
	g_MSCBuffer.scale = scale;
	g_MSCBuffer.aspect_ratio = aspect_ratio;
	g_MSCBuffer.parallax = parallax;
	g_MSCBuffer.brightness = brightness;
	g_MSCBuffer.use_3D = use_3D;
	_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, &g_MSCBuffer, 0, 0);
	_d3dDeviceContext->VSSetConstantBuffers(3, 1, buffer);
}

void DeviceResources::InitVSConstantBufferHyperspace(ID3D11Buffer ** buffer, const ShadertoyCBuffer * psConstants)
{
	_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);
	_d3dDeviceContext->VSSetConstantBuffers(7, 1, buffer);
}

void DeviceResources::InitVSConstantOPTMeshTransform(ID3D11Buffer ** buffer, const OPTMeshTransformCBuffer * vsConstants)
{
	//static OPTMeshTransformCBuffer OPTMeshTransform{};
	//if (memcmp(&OPTMeshTransform, vsConstants, sizeof(OPTMeshTransformCBuffer)) != 0)
	{
		_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, vsConstants, 0, 0);
		_d3dDeviceContext->VSSetConstantBuffers(8, 1, buffer);
		//memcpy(&OPTMeshTransform, vsConstants, sizeof(OPTMeshTransformCBuffer));
	}
}

void DeviceResources::InitPSRTConstantsBuffer(ID3D11Buffer **buffer, const RTConstantsBuffer *psConstants)
{
	//static RTConstantsBuffer RTConstants{};
	//if (memcmp(&OPTMeshTransform, vsConstants, sizeof(OPTMeshTransformCBuffer)) != 0)
	{
		_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);
		_d3dDeviceContext->PSSetConstantBuffers(10, 1, buffer);
		//memcpy(&OPTMeshTransform, vsConstants, sizeof(OPTMeshTransformCBuffer));
	}
}

void DeviceResources::InitVRGeometryCBuffer(ID3D11Buffer** buffer, const VRGeometryCBuffer* psConstants)
{
	{
		_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);
		_d3dDeviceContext->PSSetConstantBuffers(11, 1, buffer);
		//memcpy(&OPTMeshTransform, vsConstants, sizeof(OPTMeshTransformCBuffer));
	}
}

void DeviceResources::InitPSConstantBuffer2D(ID3D11Buffer** buffer, const float parallax,
	const float aspect_ratio, const float scale, const float brightness, float inv_scale, float techRoomRatio)
{
	g_MSCBuffer.scale = scale;
	g_MSCBuffer.aspect_ratio = aspect_ratio;
	g_MSCBuffer.parallax = parallax;
	g_MSCBuffer.brightness = brightness;
	g_MSCBuffer.inv_scale = inv_scale;
	g_MSCBuffer.techRoomRatio = techRoomRatio;
	_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, &g_MSCBuffer, 0, 0);
	_d3dDeviceContext->PSSetConstantBuffers(0, 1, buffer);
}

void DeviceResources::InitPSConstantBufferBarrel(ID3D11Buffer** buffer, const float k1, const float k2, const float k3)
{
	static ID3D11Buffer** currentBuffer = nullptr;
	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_BARREL ||
		g_BarrelPSCBuffer.k1 != k1 ||
		g_BarrelPSCBuffer.k2 != k2 ||
		g_BarrelPSCBuffer.k3 != k3)
	{
		g_BarrelPSCBuffer.k1 = k1;
		g_BarrelPSCBuffer.k2 = k2;
		g_BarrelPSCBuffer.k3 = k3;
		this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, &g_BarrelPSCBuffer, 0, 0);
	}

	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_BARREL ||
		buffer != currentBuffer)
	{
		currentBuffer = buffer;
		this->_d3dDeviceContext->PSSetConstantBuffers(0, 1, buffer);
	}
	g_LastPSConstantBufferSet = PS_CONSTANT_BUFFER_BARREL;
}

void DeviceResources::InitPSConstantBufferBloom(ID3D11Buffer** buffer, const BloomPixelShaderCBuffer *psConstants)
{
	static BloomPixelShaderCBuffer currentPSConstants = { 0 };
	static int sizeof_constants = sizeof(BloomPixelShaderCBuffer);

	static ID3D11Buffer** currentBuffer = nullptr;
	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_BLOOM ||
		memcmp(psConstants, &currentPSConstants, sizeof_constants) != 0)
		this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);

	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_BLOOM ||
		buffer != currentBuffer)
	{
		currentBuffer = buffer;
		this->_d3dDeviceContext->PSSetConstantBuffers(2, 1, buffer);
	}
	g_LastPSConstantBufferSet = PS_CONSTANT_BUFFER_BLOOM;
}

void DeviceResources::InitPSConstantBufferSSAO(ID3D11Buffer** buffer, const SSAOPixelShaderCBuffer *psConstants)
{
	static SSAOPixelShaderCBuffer currentPSConstants = { 0 };
	static int sizeof_constants = sizeof(SSAOPixelShaderCBuffer);

	static ID3D11Buffer** currentBuffer = nullptr;
	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_SSAO ||
		memcmp(psConstants, &currentPSConstants, sizeof_constants) != 0)
		this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);

	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_SSAO ||
		buffer != currentBuffer)
	{
		currentBuffer = buffer;
		this->_d3dDeviceContext->PSSetConstantBuffers(3, 1, buffer);
	}
	g_LastPSConstantBufferSet = PS_CONSTANT_BUFFER_SSAO;
}

void DeviceResources::InitPSConstantBufferHyperspace(ID3D11Buffer ** buffer, const ShadertoyCBuffer * psConstants)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);
	this->_d3dDeviceContext->PSSetConstantBuffers(7, 1, buffer);
}

void DeviceResources::InitPSConstantBufferLaserPointer(ID3D11Buffer ** buffer, const LaserPointerCBuffer * psConstants)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);
	this->_d3dDeviceContext->PSSetConstantBuffers(8, 1, buffer);
}

void DeviceResources::InitPSConstantBuffer3D(ID3D11Buffer** buffer, const PixelShaderCBuffer* psConstants)
{
	static ID3D11Buffer** currentBuffer = nullptr;
	static PixelShaderCBuffer currentPSConstants = {0};
	static int sizeof_constants = sizeof(PixelShaderCBuffer);

	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_3D ||
		memcmp(psConstants, &currentPSConstants, sizeof_constants) != 0)
	{
		memcpy(&currentPSConstants, psConstants, sizeof_constants);
		this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);
	}

	if (g_LastPSConstantBufferSet == PS_CONSTANT_BUFFER_NONE ||
		g_LastPSConstantBufferSet != PS_CONSTANT_BUFFER_3D ||
		buffer != currentBuffer)
	{
		currentBuffer = buffer;
		this->_d3dDeviceContext->PSSetConstantBuffers(9, 1, buffer);
	}
	g_LastPSConstantBufferSet = PS_CONSTANT_BUFFER_3D;
}

void DeviceResources::InitPSConstantBufferDC(ID3D11Buffer** buffer, const DCPixelShaderCBuffer* psConstants)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psConstants, 0, 0);
	this->_d3dDeviceContext->PSSetConstantBuffers(1, 1, buffer);
}

void DeviceResources::InitVSConstantBufferShadowMap(ID3D11Buffer **buffer, const ShadowMapVertexShaderMatrixCB *vsCBuffer)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, vsCBuffer, 0, 0);
	this->_d3dDeviceContext->VSSetConstantBuffers(5, 1, buffer);
}

void DeviceResources::InitPSConstantBufferShadowMap(ID3D11Buffer **buffer, const ShadowMapVertexShaderMatrixCB *psCBuffer)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psCBuffer, 0, 0);
	this->_d3dDeviceContext->PSSetConstantBuffers(5, 1, buffer);
}

void DeviceResources::InitVSConstantBufferMetricRec(ID3D11Buffer **buffer, const MetricReconstructionCB *vsCBuffer)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, vsCBuffer, 0, 0);
	this->_d3dDeviceContext->VSSetConstantBuffers(METRIC_REC_CB_SLOT, 1, buffer);
}

void DeviceResources::InitPSConstantBufferMetricRec(ID3D11Buffer **buffer, const MetricReconstructionCB *psCBuffer)
{
	this->_d3dDeviceContext->UpdateSubresource(buffer[0], 0, nullptr, psCBuffer, 0, 0);
	this->_d3dDeviceContext->PSSetConstantBuffers(METRIC_REC_CB_SLOT, 1, buffer);
}

void DeviceResources::InitScissorRect(D3D11_RECT* rect)
{
	//static D3D11_RECT currentRect{};

	//if (memcmp(rect, &currentRect, sizeof(D3D11_RECT)) != 0)
	{
		//currentRect = *rect;
		this->_d3dDeviceContext->RSSetScissorRects(1, rect);
	}
}

HRESULT DeviceResources::RenderMain(char* src, DWORD width, DWORD height, DWORD bpp, RenderMainColorKeyType useColorKey)
{
	HRESULT hr = S_OK;
	const char* step = "";

	D3D11_MAPPED_SUBRESOURCE displayMap;
	DWORD pitchDelta;

	ID3D11Texture2D* tex = nullptr;
	ID3D11ShaderResourceView* texView = nullptr;

	BeginAnnotatedEvent(L"RenderMain");

	if (SUCCEEDED(hr))
	{
		if ((width == this->_displayWidth) && (height == this->_displayHeight) && (bpp == this->_mainDisplayTextureBpp))
		{
			step = "DisplayTexture displayWidth x displayHeight";
			hr = this->_d3dDeviceContext->Map(this->_mainDisplayTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &displayMap);

			if (SUCCEEDED(hr))
			{
				pitchDelta = displayMap.RowPitch - width * bpp;
				tex = this->_mainDisplayTexture;
				texView = this->_mainDisplayTextureView.Get();
			}
		}
		else
		{
			step = "DisplayTexture temp";

			if (width != this->_displayTempWidth || height != this->_displayTempHeight || bpp != this->_displayTempBpp)
			{
				D3D11_TEXTURE2D_DESC textureDesc;
				textureDesc.Width = width;
				textureDesc.Height = height;
				textureDesc.Format = (bpp == 2 && this->_use16BppMainDisplayTexture) ? DXGI_FORMAT_B5G6R5_UNORM : BACKBUFFER_FORMAT;
				textureDesc.Usage = D3D11_USAGE_DYNAMIC;
				textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				textureDesc.MiscFlags = 0;
				textureDesc.MipLevels = 1;
				textureDesc.ArraySize = 1;
				textureDesc.SampleDesc.Count = 1;
				textureDesc.SampleDesc.Quality = 0;
				textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				hr = this->_d3dDevice->CreateTexture2D(&textureDesc, nullptr, &this->_mainDisplayTextureTemp);

				if (SUCCEEDED(hr))
				{
					D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc{};
					textureViewDesc.Format = textureDesc.Format;
					textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					textureViewDesc.Texture2D.MipLevels = 1;
					textureViewDesc.Texture2D.MostDetailedMip = 0;

					hr = this->_d3dDevice->CreateShaderResourceView(this->_mainDisplayTextureTemp, &textureViewDesc, &this->_mainDisplayTextureViewTemp);
				}

				if (SUCCEEDED(hr))
				{
					this->_displayTempWidth = width;
					this->_displayTempHeight = height;
					this->_displayTempBpp = bpp;
				}
			}

			if (SUCCEEDED(hr))
			{
				hr = this->_d3dDeviceContext->Map(this->_mainDisplayTextureTemp, 0, D3D11_MAP_WRITE_DISCARD, 0, &displayMap);

				if (SUCCEEDED(hr))
				{
					pitchDelta = displayMap.RowPitch - width * ((bpp == 2 && this->_use16BppMainDisplayTexture) ? 2 : 4);
					// Looks like this texture is used to store the loading menu
					tex = this->_mainDisplayTextureTemp;
					texView = this->_mainDisplayTextureViewTemp.Get();
				}
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		if (bpp == 2)
		{
			if (this->_use16BppMainDisplayTexture)
			{
				if (pitchDelta == 0)
				{
					memcpy(displayMap.pData, src, width * height * 2);
				}
				else
				{
					unsigned short* srcColors = (unsigned short*)src;
					unsigned short* colors = (unsigned short*)displayMap.pData;

					for (DWORD y = 0; y < height; y++)
					{
						memcpy(colors, srcColors, width * 2);
						srcColors += width;
						colors = (unsigned short*)((char*)(colors + width) + pitchDelta);
					}
				}
			}
			else
			{
				if (useColorKey == RENDERMAIN_COLORKEY_20)
				{
					unsigned short* srcColors = (unsigned short*)src;
					unsigned int* colors = (unsigned int*)displayMap.pData;

					for (DWORD y = 0; y < height; y++)
					{
						for (DWORD x = 0; x < width; x++)
						{
							unsigned short color16 = srcColors[x];

							if (color16 == 0x2000)
							{
								colors[x] = 0xff000000;
							}
							else
							{
								colors[x] = convertColorB5G6R5toB8G8R8X8(color16);
							}
						}

						srcColors += width;
						colors = (unsigned int*)((char*)(colors + width) + pitchDelta);
					}
				}
				else if (useColorKey == RENDERMAIN_COLORKEY_00)
				{
					unsigned short* srcColors = (unsigned short*)src;
					unsigned int* colors = (unsigned int*)displayMap.pData;

					for (DWORD y = 0; y < height; y++)
					{
						for (DWORD x = 0; x < width; x++)
						{
							unsigned short color16 = srcColors[x];

							if (color16 == 0)
							{
								colors[x] = 0xff000000;
							}
							else
							{
								colors[x] = convertColorB5G6R5toB8G8R8X8(color16);
							}
						}

						srcColors += width;
						colors = (unsigned int*)((char*)(colors + width) + pitchDelta);
					}
				}
				else
				{
					unsigned short* srcColors = (unsigned short*)src;
					unsigned int* colors = (unsigned int*)displayMap.pData;

					for (DWORD y = 0; y < height; y++)
					{
						for (DWORD x = 0; x < width; x++)
						{
							unsigned short color16 = srcColors[x];

							colors[x] = convertColorB5G6R5toB8G8R8X8(color16);
						}

						srcColors += width;
						colors = (unsigned int*)((char*)(colors + width) + pitchDelta);
					}
				}
			}
		}
		else
		{
			if (useColorKey && (this->_d3dFeatureLevel < D3D_FEATURE_LEVEL_10_0))
			{
				unsigned int* srcColors = (unsigned int*)src;
				unsigned int* colors = (unsigned int*)displayMap.pData;

				__m128i key = _mm_set1_epi32(0x200000);
				__m128i colorMask = _mm_set1_epi32(0xffffff);

				for (DWORD y = 0; y < height; y++)
				{
					DWORD x = 0;
					for (; x < (width & ~3); x += 4)
					{
						__m128i color = _mm_load_si128((const __m128i*)(srcColors + x));
						__m128i transparent = _mm_cmpeq_epi32(color, key);

						color = _mm_andnot_si128(transparent, color);
						color = _mm_and_si128(color, colorMask);

						transparent = _mm_slli_epi32(transparent, 24);

						color = _mm_or_si128(color, transparent);
						_mm_store_si128((__m128i*)(colors + x), color);
					}

					for (; x < width; x++)
					{
						unsigned int color32 = srcColors[x];

						if (color32 == 0x200000)
						{
							colors[x] = 0xff000000;
						}
						else
						{
							colors[x] = color32 & 0xffffff;
						}
					}

					srcColors += width;
					colors = (unsigned int*)((char*)(colors + width) + pitchDelta);
				}
			}
			else
			{
				if (pitchDelta == 0)
				{
					memcpy(displayMap.pData, src, width * height * 4);
				}
				else
				{
					unsigned int* srcColors = (unsigned int*)src;
					unsigned int* colors = (unsigned int*)displayMap.pData;

					for (DWORD y = 0; y < height; y++)
					{
						memcpy(colors, srcColors, width * 4);
						srcColors += width;
						colors = (unsigned int*)((char*)(colors + width) + pitchDelta);
					}
				}
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		this->_d3dDeviceContext->Unmap(tex, 0);
	}

	this->InitInputLayout(this->_mainInputLayout);
	this->InitTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->InitVertexShader(g_bUseSteamVR ? this->_mainVertexShaderVR : this->_mainVertexShader);

	if (bpp == 2)
	{
		if (useColorKey && this->_use16BppMainDisplayTexture)
		{
			switch (useColorKey)
			{
			case RENDERMAIN_COLORKEY_20:
				this->InitPixelShader(this->_mainPixelShaderBpp2ColorKey20);
				break;

			case RENDERMAIN_COLORKEY_00:
				this->InitPixelShader(this->_mainPixelShaderBpp2ColorKey00);
				break;

			default:
				this->InitPixelShader(this->_mainPixelShader);
				break;
			}
		}
		else
		{
			this->InitPixelShader(this->_mainPixelShader);
		}
	}
	else
	{
		if (useColorKey && this->_d3dFeatureLevel >= D3D_FEATURE_LEVEL_10_0)
		{
			this->InitPixelShader(this->_mainPixelShaderBpp4ColorKey20);
		}
		else
		{
			this->InitPixelShader(this->_mainPixelShader);
		}
	}

	UINT w;
	UINT h;

	if (g_config.AspectRatioPreserved)
	{
		if (this->_backbufferHeight * width <= this->_backbufferWidth * height)
		{
			w = this->_backbufferHeight * width / height;
			h = this->_backbufferHeight;
		}
		else
		{
			w = this->_backbufferWidth;
			h = this->_backbufferWidth * height / width;
		}
		
		if (!g_bOverrideAspectRatio) { // Only compute the aspect ratio if we didn't read it off of the config file
			g_fConcourseAspectRatio = 2.0f * w / this->_backbufferWidth;
			if (g_fConcourseAspectRatio < 2.0f)
				g_fConcourseAspectRatio = 2.0f;
		}
	}
	else
	{
		w = this->_backbufferWidth;
		h = this->_backbufferHeight;
		
		if (!g_bOverrideAspectRatio) {
			float original_aspect = (float)this->_displayWidth / (float)this->_displayHeight;
			float actual_aspect = (float)this->_backbufferWidth / (float)this->_backbufferHeight;
			g_fConcourseAspectRatio = 2.0f; // * (1.0f + (actual_aspect - original_aspect));
			//g_fConcourseAspectRatio = 2.0f * actual_aspect / original_aspect;
		}
	}

	UINT left = (this->_backbufferWidth - w) / 2;
	UINT top = (this->_backbufferHeight - h) / 2;
	const bool bRenderToDC = g_bRendering3D && g_bExecuteBufferLock;
	const bool bMapMode = (g_playerIndex != nullptr) && (PlayerDataTable[*g_playerIndex].mapState != 0);

	if (g_bEnableVR && !bRenderToDC) { // SteamVR and DirectSBS modes
		InitVSConstantBuffer2D(this->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, g_fConcourseAspectRatio, g_fConcourseScale, g_fBrightness, 1.0f); // Use 3D projection matrices
		InitPSConstantBuffer2D(this->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, g_fConcourseAspectRatio, g_fConcourseScale, g_fBrightness, 1.0f);
		//InitVSConstantBuffer2D(this->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, g_fConcourseAspectRatio, 1, g_fBrightness, 0.0f); // Use 3D projection matrices
		//InitPSConstantBuffer2D(this->_mainShadersConstantBuffer.GetAddressOf(), 0.0f, g_fConcourseAspectRatio, 1, g_fBrightness);
	} 
	else {
		InitVSConstantBuffer2D(this->_mainShadersConstantBuffer.GetAddressOf(), 0, 1, 1, g_fBrightness, 0.0f); // Don't use 3D projection matrices when VR is disabled
		InitPSConstantBuffer2D(this->_mainShadersConstantBuffer.GetAddressOf(), 0, 1, 1, g_fBrightness, 1.0f);
	}

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = (float)left;
	viewport.TopLeftY = (float)top;
	viewport.Width = (float)w;
	viewport.Height = (float)h;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	this->InitViewport(&viewport);
	g_concourseViewport = viewport;

	if (SUCCEEDED(hr))
	{
		step = "States";
		this->InitRasterizerState(this->_mainRasterizerState);
		this->InitSamplerState(this->_mainSamplerState.GetAddressOf(), nullptr);
		this->InitBlendState(this->_mainBlendState, nullptr);
		this->InitDepthStencilState(this->_mainDepthState, nullptr);
	}

	if (SUCCEEDED(hr))
	{
		step = "Texture2D ShaderResourceView";

		this->InitPSShaderResourceView(texView);
	}

	if (SUCCEEDED(hr))
	{
		step = "Draw";
		D3D11_VIEWPORT viewport = { 0 };
		UINT stride = sizeof(MainVertex);
		UINT offset = 0;
		auto& context = _d3dDeviceContext;

		// [XWA-MAP-RENDER] This path renders the square grid and the vertical lines when the map is displayed.

		// RenderMain() will render the sub-component CMD *iff* the hook_d3d is disabled.
		if (bRenderToDC) {
			_d3dDeviceContext->OMSetRenderTargets(1, _renderTargetViewDynCockpit.GetAddressOf(), NULL);
		}

		this->InitVertexBuffer(this->_mainVertexBuffer.GetAddressOf(), &stride, &offset);
		this->InitIndexBuffer(this->_mainIndexBuffer, false);

		float screen_res_x = (float)this->_backbufferWidth;
		float screen_res_y = (float)this->_backbufferHeight;
		const bool bDirectSBS = g_bEnableVR && !g_bUseSteamVR;

		if (!g_bEnableVR || bRenderToDC)
		{
			// The CMD sub-component bracket are drawn here... maybe the default starfield too?
			// The map lines (both the grid and the vertical lines) are drawn here.
			// In VR mode, when the map is enabled, we render to the dynamic cockpit buffer. That way,
			// we can use RenderHUD() to display the map as a flat surface.
			if (g_bUseSteamVR && bMapMode)
			{
				context->OMSetRenderTargets(1, _renderTargetViewDynCockpit.GetAddressOf(), NULL);
				this->_d3dDeviceContext->DrawIndexedInstanced(6, 1, 0, 0, 0);
				goto out;
			}
			else
			{
				context->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilViewL.Get());
				this->_d3dDeviceContext->DrawIndexed(6, 0, 0);
			}

			if (bRenderToDC)
			{
				//log_debug("[DBG] viewport: %0.3f, %0.3f, %0.3f, %0.3f",
				//	viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height);
				//log_debug("[DBG] viewport: %d, %d, %d, %d", left, top, w, h);

				// The following block will dump an texture defined in in-game resolution that has the
				// sub-component CMD bracket
				// DEBUG
				/*
				static bool bFirstTime = true;
				if (bFirstTime) {
					//ID3D11Resource *res = NULL;
					//texView->GetResource(&res);
					//DirectX::SaveWICTextureToFile(this->_d3dDeviceContext, res, GUID_ContainerFormatPng,
					//	L"C:\\Temp\\_DCTexViewSubCMD.png");
					// This actually saved the loading screen:
					//DirectX::SaveWICTextureToFile(this->_d3dDeviceContext, this->_mainDisplayTextureTemp, GUID_ContainerFormatPng,
					//	L"C:\\Temp\\_DCTexViewSubCMD.png");

					// This texture has the sub-component bracket:
					// How does this work? I believe the src argument provided for RenderMain contains the color data with
					// the bracket already rendered in there. Then, these colors are copied to the _mainDisplayTexture by using
					// a Map/Unmap operation. The VS/PS simply blit these colors on top of the screen/buffer that was rendered
					// up to this point.
					DirectX::SaveWICTextureToFile(this->_d3dDeviceContext, this->_mainDisplayTexture, GUID_ContainerFormatPng,
						L"C:\\Temp\\_DCTexViewSubCMD.png");
					log_debug("[DBG] Captured texture used in DC sub-component");
					bFirstTime = false;
				} */
				// DEBUG

				// We have just drawn something to the DC buffer, we need to resolve it before the next frame
				// NOTE: This is executed *before* we run Execute(), so we can't clear the RTVs before this point 
				//       or this resolve operation will also clear the SRVs!
				this->_d3dDeviceContext->ResolveSubresource(this->_offscreenAsInputDynCockpit,
					0, this->_offscreenBufferDynCockpit, 0, BACKBUFFER_FORMAT);
			}
			goto out;
		}

		// Let's do VR stereo rendering here. That'll make it compatible with the Tech Library where
		// both 2D and 3D are mixed.
		// Left viewport
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		if (g_bUseSteamVR)
			viewport.Width = screen_res_x;
		else
			viewport.Width = screen_res_x / 2.0f;
		viewport.Height = screen_res_y;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		this->InitViewport(&viewport);

		if (g_iDraw2DCounter > 0)		
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ComPtr<ID3D11DepthStencilState> depthState;
			desc.DepthEnable = FALSE;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			desc.StencilEnable = FALSE;
			this->InitDepthStencilState(depthState, &desc);
		}
		
		if (bDirectSBS) {
			this->InitVSConstantBuffer2D(_mainShadersConstantBuffer.GetAddressOf(),
				g_fTechLibraryParallax * g_iDraw2DCounter, g_fConcourseAspectRatio, g_fConcourseScale, g_fBrightness,
				1.0f); // Use 3D projection matrices
		}
		else {
			this->InitVSConstantBuffer2D(_mainShadersConstantBuffer.GetAddressOf(),
				1, 1, 1, g_fBrightness,
				0.0f); // Do not use 3D projection matrices
		}
		// The Concourse and 2D menu are drawn here... maybe the default starfield too?
		// When the map is active, all the lines are rendered here
		// When SteamVR is not used, the RenderTargets are set in the OnSizeChanged() event above
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		// In SteamVR mode, we need to clear fullViewMat or the ships in the Tech Room
		// will float all over the place.
		if (!bDirectSBS)
			g_VSMatrixCB.fullViewMat.identity();
		InitVSConstantBufferMatrix(_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);
		_d3dDeviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilViewL.Get());

		bool bDefaultStarField = (g_bRendering3D && g_ExecuteCount == 0);
		if (g_bUseSteamVR)
		{
			// In VR mode, do not render the default starfield. Most of the time, some backdrop will
			// cover it, but in some missions, there's backdrops with transparency and the default
			// starfield shows through. We don't want that.
			if (!bDefaultStarField)
				this->_d3dDeviceContext->DrawIndexedInstanced(6, 2, 0, 0, 0); // if (g_bUseSteamVR)
		}
		else
			this->_d3dDeviceContext->DrawIndexed(6, 0, 0);

		// Right viewport
		if (g_bUseSteamVR)
			goto out; // The right eye was already rendered in DrawIndexedInstanced above

		// DirectSBS case
		viewport.TopLeftX = screen_res_x / 2.0f;
		viewport.Width = screen_res_x / 2.0f;
		viewport.TopLeftY = 0;
		viewport.Height = screen_res_y;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		this->InitViewport(&viewport);

		this->InitVSConstantBuffer2D(this->_mainShadersConstantBuffer.GetAddressOf(),
			g_fTechLibraryParallax * g_iDraw2DCounter, g_fConcourseAspectRatio, g_fConcourseScale, g_fBrightness,
			1.0f); // Use 3D projection matrices

		// The Concourse and 2D menu are drawn here... maybe the default starfield too?
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
		InitVSConstantBufferMatrix(_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);
		_d3dDeviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilViewL.Get());
		this->_d3dDeviceContext->DrawIndexed(6, 0, 0);
	out:
		// Increase the 2D DrawCounter -- this is used in the Tech Library to increase the parallax when the second 2D
		// layer is rendered on top of the 3D floating object.
		g_iDraw2DCounter++;
	}

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			char text[512];
			strcpy_s(text, step);
			strcat_s(text, "\n");
			strcat_s(text, _com_error(hr).ErrorMessage());

			MessageBox(nullptr, text, __FUNCTION__, MB_ICONERROR);
		}

		messageShown = true;
	}

	this->EndAnnotatedEvent();

	return hr;
}

HRESULT DeviceResources::RetrieveBackBuffer(char* buffer, DWORD width, DWORD height, DWORD bpp)
{
	HRESULT hr = S_OK;
	const char* step = "";

	memset(buffer, 0, width * height * bpp);

	D3D11_TEXTURE2D_DESC textureDescription;
	this->_backBuffer->GetDesc(&textureDescription);

	textureDescription.BindFlags = 0;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;

	ComPtr<ID3D11Texture2D> backBuffer;
	textureDescription.Usage = D3D11_USAGE_DEFAULT;
	textureDescription.CPUAccessFlags = 0;

	step = "Resolve BackBuffer";

	if (SUCCEEDED(hr = this->_d3dDevice->CreateTexture2D(&textureDescription, nullptr, &backBuffer)))
	{
		this->_d3dDeviceContext->ResolveSubresource(backBuffer, D3D11CalcSubresource(0, 0, 1), this->_backBuffer, D3D11CalcSubresource(0, 0, 1), textureDescription.Format);

		step = "Staging Texture2D";

		ComPtr<ID3D11Texture2D> texture;
		textureDescription.Usage = D3D11_USAGE_STAGING;
		textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		if (SUCCEEDED(hr = this->_d3dDevice->CreateTexture2D(&textureDescription, nullptr, &texture)))
		{
			this->_d3dDeviceContext->CopyResource(texture, backBuffer);

			step = "Map";

			D3D11_MAPPED_SUBRESOURCE map;
			if (SUCCEEDED(hr = this->_d3dDeviceContext->Map(texture, 0, D3D11_MAP_READ, 0, &map)))
			{
				step = "copy";

				if (bpp == 4 && width == this->_backbufferWidth && height == this->_backbufferHeight && this->_backbufferWidth * 4 == map.RowPitch)
				{
					memcpy(buffer, map.pData, width * height * 4);
				}
				else
				{
					if (this->_backbufferWidth * 4 == map.RowPitch)
					{
						scaleSurface(buffer, width, height, bpp, (char*)map.pData, this->_backbufferWidth, this->_backbufferHeight, 4);
					}
					else
					{
						char* buffer2 = new char[this->_backbufferWidth * this->_backbufferHeight * 4];

						unsigned int* srcColors = (unsigned int*)map.pData;
						unsigned int* colors = (unsigned int*)buffer2;

						for (DWORD y = 0; y < this->_backbufferHeight; y++)
						{
							memcpy(colors, srcColors, this->_backbufferWidth * 4);

							srcColors = (unsigned int*)((char*)srcColors + map.RowPitch);
							colors += this->_backbufferWidth;
						}

						scaleSurface(buffer, width, height, bpp, buffer2, this->_backbufferWidth, this->_backbufferHeight, 4);

						delete[] buffer2;
					}
				}

				this->_d3dDeviceContext->Unmap(texture, 0);
			}
		}
	}

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			char text[512];
			strcpy_s(text, step);
			strcat_s(text, "\n");
			strcat_s(text, _com_error(hr).ErrorMessage());

			MessageBox(nullptr, text, __FUNCTION__, MB_ICONERROR);
		}

		messageShown = true;
	}

	return hr;
}

HRESULT DeviceResources::RetrieveTextureBuffer(ID3D11Texture2D* textureBuffer, char* buffer, DWORD width, DWORD height, DWORD bpp)
{
	HRESULT hr = S_OK;
	const char* step = "";

	memset(buffer, 0, width * height * bpp);

	D3D11_TEXTURE2D_DESC textureDescription;
	textureBuffer->GetDesc(&textureDescription);

	textureDescription.BindFlags = 0;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;

	ComPtr<ID3D11Texture2D> offBuffer;
	textureDescription.Usage = D3D11_USAGE_DEFAULT;
	textureDescription.CPUAccessFlags = 0;

	step = "Resolve OffscreenBuffer";

	if (SUCCEEDED(hr = this->_d3dDevice->CreateTexture2D(&textureDescription, nullptr, &offBuffer)))
	{
		this->_d3dDeviceContext->ResolveSubresource(offBuffer, D3D11CalcSubresource(0, 0, 1), textureBuffer, D3D11CalcSubresource(0, 0, 1), textureDescription.Format);

		step = "Staging Texture2D";

		ComPtr<ID3D11Texture2D> texture;
		textureDescription.Usage = D3D11_USAGE_STAGING;
		textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		if (SUCCEEDED(hr = this->_d3dDevice->CreateTexture2D(&textureDescription, nullptr, &texture)))
		{
			this->_d3dDeviceContext->CopyResource(texture, offBuffer);

			step = "Map";

			D3D11_MAPPED_SUBRESOURCE map;
			if (SUCCEEDED(hr = this->_d3dDeviceContext->Map(texture, 0, D3D11_MAP_READ, 0, &map)))
			{
				step = "copy";

				if (bpp == 4 && width == this->_backbufferWidth && height == this->_backbufferHeight && this->_backbufferWidth * 4 == map.RowPitch)
				{
					memcpy(buffer, map.pData, width * height * 4);
				}
				else
				{
					if (this->_backbufferWidth * 4 == map.RowPitch)
					{
						scaleSurface(buffer, width, height, bpp, (char*)map.pData, this->_backbufferWidth, this->_backbufferHeight, 4);
					}
					else
					{
						char* buffer2 = new char[this->_backbufferWidth * this->_backbufferHeight * 4];

						unsigned int* srcColors = (unsigned int*)map.pData;
						unsigned int* colors = (unsigned int*)buffer2;

						for (DWORD y = 0; y < this->_backbufferHeight; y++)
						{
							memcpy(colors, srcColors, this->_backbufferWidth * 4);


							srcColors = (unsigned int*)((char*)srcColors + map.RowPitch);
							colors += this->_backbufferWidth;
						}

						scaleSurface(buffer, width, height, bpp, buffer2, this->_backbufferWidth, this->_backbufferHeight, 4);

						delete[] buffer2;
					}
				}

				this->_d3dDeviceContext->Unmap(texture, 0);
			}
		}
	}

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			char text[512];
			strcpy_s(text, step);
			strcat_s(text, "\n");
			strcat_s(text, _com_error(hr).ErrorMessage());

			MessageBox(nullptr, text, __FUNCTION__, MB_ICONERROR);
		}

		messageShown = true;
	}

	return hr;
}

UINT DeviceResources::GetMaxAnisotropy()
{
	return this->_d3dFeatureLevel >= D3D_FEATURE_LEVEL_9_2 ? D3D11_MAX_MAXANISOTROPY : D3D_FL9_1_DEFAULT_MAX_ANISOTROPY;
}

void DeviceResources::CheckMultisamplingSupport()
{
	this->_sampleDesc.Count = 1;
	this->_sampleDesc.Quality = 0;

	if (!this->_useMultisampling)
	{
		return;
	}

	UINT formatSupport;

	if (FAILED(this->_d3dDevice->CheckFormatSupport(BACKBUFFER_FORMAT, &formatSupport)))
	{
		return;
	}

	bool supported = (formatSupport & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE) && (formatSupport & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);

	if (supported)
	{
		for (UINT i = 2; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; i *= 2)
		{
			UINT numQualityLevels = 0;

			HRESULT hr = this->_d3dDevice->CheckMultisampleQualityLevels(BACKBUFFER_FORMAT, i, &numQualityLevels);

			if (SUCCEEDED(hr) && (numQualityLevels > 0))
			{
				this->_sampleDesc.Count = i;
				this->_sampleDesc.Quality = numQualityLevels - 1;
				//this->_sampleDesc.Quality = D3D11_STANDARD_MULTISAMPLE_PATTERN;
				// Stop checking for MSAA levels if we reached the limit set by the user
				if (g_config.MSAACount > 0 && this->_sampleDesc.Count == g_config.MSAACount)
					break;
			}
		}
	}

	if (this->_sampleDesc.Count <= 1)
	{
		this->_useMultisampling = FALSE;
	}
}

bool DeviceResources::IsTextureFormatSupported(DXGI_FORMAT format)
{
	UINT formatSupport;

	if (FAILED(this->_d3dDevice->CheckFormatSupport(format, &formatSupport)))
	{
		return false;
	}

	const UINT expected = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_MIP | D3D11_FORMAT_SUPPORT_SHADER_LOAD | D3D11_FORMAT_SUPPORT_CPU_LOCKABLE;

	return (formatSupport & expected) == expected;
}

bool DeviceResources::BeginAnnotatedEvent(_In_ LPCWSTR Name)
{
	if (_d3dAnnotation != nullptr)
	{
		_d3dAnnotation->BeginEvent(Name);
		return true;
	}
	else
		return false;
}

bool DeviceResources::EndAnnotatedEvent()
{
	if (_d3dAnnotation != nullptr)
	{
		_d3dAnnotation->EndEvent();
		return true;
	}
	else
		return false;
}

bool DeviceResources::IsInConcourseHd()
{
	g_callDrawCursor = true;

	// This ddraw is *always* going to be for XWA
	/*if (!_IsXwaExe)
	{
		return false;
	}*/

	if (!g_config.HDConcourseEnabled)
	{
		return false;
	}

	const int currentGameState = *(int*)(0x009F60E0 + 0x25FA9);
	const int updateCallback = *(int*)(0x009F60E0 + 0x25FB1 + 0x850 * currentGameState + 0x0844);
	const bool isConfigMenuGameStateUpdate = updateCallback == 0x0051D100;
	const bool isMessageBoxGameStateUpdate = updateCallback == 0x005595A0 || updateCallback == 0x00541260;

	unsigned char XwaGlobalVariables_m00F2F = *(unsigned char*)(0x009F60E0 + 0x0F2F);
	const bool isConcourse = this->_displayWidth == 640 && this->_displayHeight == 480 && this->_displayBpp == 2;
	const int frameIndex = *(int*)(0x009F60E0 + 0x2B361);

	if (XwaGlobalVariables_m00F2F == 0)
	{
		return false;
	}

	if (isConfigMenuGameStateUpdate)
	{
		if (isConcourse)
		{
			g_callDrawCursor = false;
		}

		return true;
	}

	if (isMessageBoxGameStateUpdate)
	{
		if (isConcourse)
		{
			g_callDrawCursor = false;
		}

		return true;
	}

	if (!inScene)
	{
		g_callDrawCursor = false;
		return true;
	}

	if (!isConcourse)
	{
		return false;
	}

	g_callDrawCursor = false;

	return true;
}
