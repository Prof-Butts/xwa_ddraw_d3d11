// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

#pragma once
#include "Matrices.h"
#include "../shaders/material_defs.h"
#include "../shaders/shader_common.h"
#include <vector>
#include "common.h"
#include "effects.h"

enum RenderMainColorKeyType
{
	RENDERMAIN_NO_COLORKEY,
	RENDERMAIN_COLORKEY_20,
	RENDERMAIN_COLORKEY_00,
};

class PrimarySurface;
class DepthSurface;
class BackbufferSurface;
class FrontbufferSurface;
class OffscreenSurface;

#define BACKBUFFER_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM
#define DEPTH_BUFFER_FORMAT DXGI_FORMAT_D32_FLOAT
//#define DEPTH_BUFFER_FORMAT DXGI_FORMAT_D24_UNORM_S8_UINT
//#define BLOOM_BUFFER_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM
#define BLOOM_BUFFER_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
#define AO_DEPTH_BUFFER_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
//#define AO_DEPTH_BUFFER_FORMAT DXGI_FORMAT_R32G32B32A32_FLOAT
//#define AO_MASK_FORMAT DXGI_FORMAT_R8_UINT
#define AO_MASK_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM
#define HDR_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
#define RT_SHADOW_FORMAT DXGI_FORMAT_R16G16_FLOAT

/*
 * Used to store a list of textures for fast lookup. For instance, all suns must
 * have their associated lights reset after jumping through hyperspace; and all
 * textures with materials can be placed here so that material properties can be
 * re-applied while flying.
 */
/*
typedef struct AuxTextureDataStruct {
	Direct3DTexture *texture;
} AuxTextureData;
*/

// Text Rendering
// Font indices that can be used with the PrimarySurface::AddText() methods (and others) below
#define FONT_MEDIUM_IDX 0
#define FONT_LARGE_IDX 1
#define FONT_SMALL_IDX 2
#define FONT_BLUE_COLOR 0x5555FF

// Use this when using a static array for _extraTextures:
//const int MAX_EXTRA_TEXTURES = 40;

class TimedMessage {
public:
	time_t t_exp;
	char msg[128];
	short y;
	short font_size_idx;
	uint32_t color;

	TimedMessage() {
		this->msg[0] = 0;
		this->y = 200;
		this->color = FONT_BLUE_COLOR;
		this->font_size_idx = FONT_LARGE_IDX;
	}

	inline bool IsExpired() {
		return this->msg[0] == 0;
	}

	inline void SetMsg(char *msg, time_t seconds, short y, short font_size_idx, uint32_t color) {
		strcpy_s(this->msg, 128, msg);
		this->t_exp = time(NULL) + seconds;
		this->y = y;
		this->font_size_idx = font_size_idx;
		this->color = color;
	}

	inline void Tick() {
		time_t t = time(NULL);
		if (t > this->t_exp)
			this->msg[0] = 0;
	}
};
const int MAX_TIMED_MESSAGES = 3;

/*
  Only rows 0..2 are available
 */
void DisplayTimedMessage(uint32_t seconds, int row, char *msg);

class DeviceResources
{
public:
	DeviceResources();

	~DeviceResources();

	HRESULT Initialize();

	HRESULT OnSizeChanged(HWND hWnd, DWORD dwWidth, DWORD dwHeight);

	HRESULT LoadMainResources();

	HRESULT LoadResources();

	void InitInputLayout(ID3D11InputLayout* inputLayout);
	void InitVertexShader(ID3D11VertexShader* vertexShader);
	void InitPixelShader(ID3D11PixelShader* pixelShader);
	ID3D11PixelShader *GetCurrentPixelShader();
	void InitGeometryShader(ID3D11GeometryShader* geometryShader);
	void InitTopology(D3D_PRIMITIVE_TOPOLOGY topology);
	void InitRasterizerState(ID3D11RasterizerState* state);
	void InitPSShaderResourceView(ID3D11ShaderResourceView* texView, ID3D11ShaderResourceView* texView2 = nullptr);
	HRESULT InitSamplerState(ID3D11SamplerState** sampler, D3D11_SAMPLER_DESC* desc);
	HRESULT InitBlendState(ID3D11BlendState* blend, D3D11_BLEND_DESC* desc);
	HRESULT InitDepthStencilState(ID3D11DepthStencilState* depthState, D3D11_DEPTH_STENCIL_DESC* desc, UINT stencilReference = 0);
	void InitVertexBuffer(ID3D11Buffer** buffer, UINT* stride, UINT* offset);
	void InitIndexBuffer(ID3D11Buffer* buffer, bool isFormat32);
	void InitViewport(D3D11_VIEWPORT* viewport);
	void InitVSConstantBuffer3D(ID3D11Buffer** buffer, const VertexShaderCBuffer* vsCBuffer);
	void InitVSConstantBufferMatrix(ID3D11Buffer** buffer, const VertexShaderMatrixCB* vsCBuffer);
	void InitPSConstantShadingSystem(ID3D11Buffer** buffer, const PSShadingSystemCB* psCBuffer);
	void InitVSConstantBuffer2D(ID3D11Buffer** buffer, const float parallax, const float aspectRatio, const float scale, const float brightness, const float use_3D);
	void InitVSConstantBufferHyperspace(ID3D11Buffer ** buffer, const ShadertoyCBuffer * psConstants);
	void InitVSConstantOPTMeshTransform(ID3D11Buffer ** buffer, const OPTMeshTransformCBuffer *vsConstants);
	void InitPSRTConstantsBuffer(ID3D11Buffer ** buffer, const RTConstantsBuffer *psConstants);
	void InitPSConstantBuffer2D(ID3D11Buffer** buffer, const float parallax, const float aspectRatio, const float scale, const float brightness, float inv_scale = 1.0f);
	void InitPSConstantBufferBarrel(ID3D11Buffer** buffer, const float k1, const float k2, const float k3);
	void InitPSConstantBufferBloom(ID3D11Buffer ** buffer, const BloomPixelShaderCBuffer * psConstants);
	void InitPSConstantBufferSSAO(ID3D11Buffer ** buffer, const SSAOPixelShaderCBuffer * psConstants);
	void InitPSConstantBuffer3D(ID3D11Buffer** buffer, const PixelShaderCBuffer *psConstants);
	void InitPSConstantBufferDC(ID3D11Buffer** buffer, const DCPixelShaderCBuffer * psConstants);
	void InitPSConstantBufferHyperspace(ID3D11Buffer ** buffer, const ShadertoyCBuffer * psConstants);
	void InitPSConstantBufferLaserPointer(ID3D11Buffer ** buffer, const LaserPointerCBuffer * psConstants);
	// Shadow Mapping CBs
	void InitVSConstantBufferShadowMap(ID3D11Buffer **buffer, const ShadowMapVertexShaderMatrixCB *vsCBuffer);
	void InitPSConstantBufferShadowMap(ID3D11Buffer **buffer, const ShadowMapVertexShaderMatrixCB *psCBuffer);
	// Metric Reconstruction CBs
	void InitVSConstantBufferMetricRec(ID3D11Buffer **buffer, const MetricReconstructionCB *vsCBuffer);
	void InitPSConstantBufferMetricRec(ID3D11Buffer **buffer, const MetricReconstructionCB *psCBuffer);

	void BuildHUDVertexBuffer(float width, float height);
	void BuildHyperspaceVertexBuffer(float width, float height);
	void BuildPostProcVertexBuffer();
	void InitSpeedParticlesVB();
	void BuildSpeedVertexBuffer();
	void CreateShadowVertexIndexBuffers(D3DTLVERTEX *vertices, WORD *indices, UINT numVertices, UINT numIndices);
	//void FillReticleVertexBuffer(float width, float height /*float sz, float rhw*/);
	//void CreateReticleVertexBuffer();
	void CreateRandomVectorTexture();
	void DeleteRandomVectorTexture();
	void CreateGrayNoiseTexture();
	void DeleteGrayNoiseTexture();
	void Create3DVisionSignatureTexture();
	void Delete3DVisionTexture();
	void CreateTgSmushTexture(DWORD width, DWORD height);
	void DeleteTgSmushTexture();
	void ClearDynCockpitVector(dc_element DCElements[], int size);
	void ClearActiveCockpitVector(ac_element ACElements[], int size);

	void ResetDynamicCockpit();

	void ResetActiveCockpit();

	void ResetExtraTextures();
	void InitScissorRect(D3D11_RECT* rect);

	HRESULT RenderMain(char* buffer, DWORD width, DWORD height, DWORD bpp, RenderMainColorKeyType useColorKey = RENDERMAIN_COLORKEY_20);

	HRESULT RetrieveBackBuffer(char* buffer, DWORD width, DWORD height, DWORD bpp);
	HRESULT RetrieveTextureBuffer(ID3D11Texture2D* textureBuffer, char* buffer, DWORD width, DWORD height, DWORD bpp);

	UINT GetMaxAnisotropy();

	void CheckMultisamplingSupport();

	bool IsTextureFormatSupported(DXGI_FORMAT format);

	bool BeginAnnotatedEvent(_In_ LPCWSTR Name);
	bool EndAnnotatedEvent();

	bool IsInConcourseHd();

	DWORD _displayWidth;
	DWORD _displayHeight;
	DWORD _displayBpp;
	DWORD _displayTempWidth;
	DWORD _displayTempHeight;
	DWORD _displayTempBpp;

	D3D_DRIVER_TYPE _d3dDriverType;
	D3D_FEATURE_LEVEL _d3dFeatureLevel;
	ComPtr<ID3D11Device> _d3dDevice;
	ComPtr<ID3D11DeviceContext> _d3dDeviceContext;
	ComPtr<ID3DUserDefinedAnnotation> _d3dAnnotation;
	ComPtr<IDXGISwapChain> _swapChain = nullptr;
	// Buffers/Textures
	ComPtr<ID3D11Texture2D> _backBuffer;
	ComPtr<ID3D11Texture2D> _offscreenBuffer;
	ComPtr<ID3D11Texture2D> _offscreenBufferHdBackground;
	ComPtr<ID3D11Texture2D> _offscreenBufferR; // When SteamVR is used, _offscreenBuffer becomes the left eye and this one becomes the right eye
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInput;
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInputR; // When SteamVR is used, this is the right eye as input buffer
	// Dynamic Cockpit
	ComPtr<ID3D11Texture2D> _offscreenBufferDynCockpit;    // Used to render the targeting computer dynamically <-- Need to re-check this claim
	ComPtr<ID3D11Texture2D> _offscreenBufferDynCockpitBG;  // Used to render the targeting computer dynamically <-- Need to re-check this claim
	ComPtr<ID3D11Texture2D> _offscreenAsInputDynCockpit;   // HUD elements buffer
	ComPtr<ID3D11Texture2D> _offscreenAsInputDynCockpitBG; // HUD element backgrounds buffer
	ComPtr<ID3D11Texture2D> _DCTextMSAA;				   // "RTV" to render text
	ComPtr<ID3D11Texture2D> _DCTextAsInput;				   // Resolved from DCTextMSAA for use in shaders
	ComPtr<ID3D11Texture2D> _ReticleBufMSAA;			   // "RTV" to render the HUD in VR mode
	ComPtr<ID3D11Texture2D> _ReticleBufAsInput;			   // Resolved from DCTextMSAA for use in shaders
	// Barrel effect
	ComPtr<ID3D11Texture2D> _offscreenBufferPost;  // This is the output of the barrel effect
	ComPtr<ID3D11Texture2D> _offscreenBufferPostR; // This is the output of the barrel effect for the right image when using SteamVR
	ComPtr<ID3D11Texture2D> _steamVRPresentBuffer; // This is the buffer that will be presented in the monitor mirror window for SteamVR
	ComPtr<ID3D11Texture2D> _steamVROverlayBuffer; // This is the buffer that will be presented as a VR overlay.

	// ShaderToy effects
	ComPtr<ID3D11Texture2D> _shadertoyBufMSAA;
	ComPtr<ID3D11Texture2D> _shadertoyBufMSAA_R;
	ComPtr<ID3D11Texture2D> _shadertoyBuf;      // No MSAA
	ComPtr<ID3D11Texture2D> _shadertoyBufR;     // No MSAA
	ComPtr<ID3D11Texture2D> _shadertoyAuxBuf;   // No MSAA
	ComPtr<ID3D11Texture2D> _shadertoyAuxBufR;  // No MSAA
	// Bloom
	ComPtr<ID3D11Texture2D> _offscreenBufferBloomMask;  // Used to render the bloom mask
	ComPtr<ID3D11Texture2D> _offscreenBufferBloomMaskR; // Used to render the bloom mask to the right image (SteamVR)
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInputBloomMask;  // Used to resolve offscreenBufferBloomMask
	ComPtr<ID3D11Texture2D> _offscreenBufferAsInputBloomMaskR; // Used to resolve offscreenBufferBloomMaskR
	ComPtr<ID3D11Texture2D> _bloomOutput1; // Output from bloom pass 1
	ComPtr<ID3D11Texture2D> _bloomOutput2; // Output from bloom pass 2
	ComPtr<ID3D11Texture2D> _bloomOutputSum; // Bloom accummulator
	ComPtr<ID3D11Texture2D> _bloomOutput1R; // Output from bloom pass 1, right image (SteamVR)
	ComPtr<ID3D11Texture2D> _bloomOutput2R; // Output from bloom pass 2, right image (SteamVR)
	ComPtr<ID3D11Texture2D> _bloomOutputSumR; // Bloom accummulator (SteamVR)
	// Ambient Occlusion
	ComPtr<ID3D11Texture2D> _depthBuf;
	ComPtr<ID3D11Texture2D> _depthBufR;
	ComPtr<ID3D11Texture2D> _depthBufAsInput;
	ComPtr<ID3D11Texture2D> _depthBufAsInputR; // Used in SteamVR mode
	ComPtr<ID3D11Texture2D> _bentBuf;		// No MSAA
	ComPtr<ID3D11Texture2D> _bentBufR;		// No MSAA
	ComPtr<ID3D11Texture2D> _ssaoBuf;		// No MSAA
	ComPtr<ID3D11Texture2D> _ssaoBufR;		// No MSAA
	// Shading System
	ComPtr<ID3D11Texture2D> _normBufMSAA;
	ComPtr<ID3D11Texture2D> _normBufMSAA_R;
	ComPtr<ID3D11Texture2D> _normBuf;		 // No MSAA so that it can be both bound to RTV and SRV
	ComPtr<ID3D11Texture2D> _normBufR;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssaoMaskMSAA;
	ComPtr<ID3D11Texture2D> _ssaoMaskMSAA_R;
	ComPtr<ID3D11Texture2D> _ssaoMask;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssaoMaskR;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssMaskMSAA;	
	ComPtr<ID3D11Texture2D> _ssMaskMSAA_R;
	ComPtr<ID3D11Texture2D> _ssMask;		 // No MSAA
	ComPtr<ID3D11Texture2D> _ssMaskR;		 // No MSAA
	// Shadow Mapping
	ComPtr<ID3D11Texture2D> _shadowMap;
	ComPtr<ID3D11Texture2D> _shadowMapArray;
	ComPtr<ID3D11Texture2D> _shadowMapDebug; // TODO: Disable this before release
	ComPtr<ID3D11Texture2D> _csmMap;		 // Main render texture for CSM.
	ComPtr<ID3D11Texture2D> _csmArray;       // Cascaded Shadow Map array
	// Raytracing
	ComPtr<ID3D11Texture2D> _rtShadowMask;
	ComPtr<ID3D11Texture2D> _rtShadowMaskR;
	// Generated/Procedural Textures
	ComPtr<ID3D11Texture2D> _grayNoiseTex;
	// 3D Vision
	// double-wide version of _offscreenBufferPost. We use this buffer to resize the _offscreenBuffer
	// to double-wide size while doing the barrel effect
	ComPtr<ID3D11Texture2D> _vision3DPost; 
	// We use this buffer to resolve _vision3DPost into a Non-MSAA buffer:
	ComPtr<ID3D11Texture2D> _vision3DNoMSAA;
	// We use this buffer to add the 3D vision signature
	ComPtr<ID3D11Texture2D> _vision3DStaging;
	// This texture stores the 3D vision signature.
	ComPtr<ID3D11Texture2D> _vision3DSignatureTex;
	// TgSmush
	ComPtr<ID3D11Texture2D> _tgSmushTex;

	// RTVs
	ComPtr<ID3D11RenderTargetView> _renderTargetView;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewR; // When SteamVR is used, _renderTargetView is the left eye, and this one is the right eye
	// Dynamic Cockpit
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpit; // Used to render the HUD to an offscreen buffer
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpitBG; // Used to render the HUD to an offscreen buffer
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpitAsInput; // RTV that writes to _offscreenBufferAsInputDynCockpit directly
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDynCockpitAsInputBG; // RTV that writes to _offscreenBufferAsInputDynCockpitBG directly
	ComPtr<ID3D11RenderTargetView> _DCTextRTV;
	ComPtr<ID3D11RenderTargetView> _DCTextAsInputRTV;
	ComPtr<ID3D11RenderTargetView> _ReticleRTV;
	// Barrel Effect
	ComPtr<ID3D11RenderTargetView> _renderTargetViewPost;  // Used for the barrel effect
	ComPtr<ID3D11RenderTargetView> _renderTargetViewPostR; // Used for the barrel effect (right image) when SteamVR is used.
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSteamVRResize; // Used to resize the image before presenting it in the monitor window.
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSteamVROverlayResize; // Used to resize the image before copying it to the VR overlay to present 2D mode images.

	// ShaderToy
	ComPtr<ID3D11RenderTargetView> _shadertoyRTV;
	ComPtr<ID3D11RenderTargetView> _shadertoyRTV_R;
	// Bloom
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomMask  = NULL; // Renders to _offscreenBufferBloomMask
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomMaskR = NULL; // Renders to _offscreenBufferBloomMaskR
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom1;    // Renders to bloomOutput1
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom2;    // Renders to bloomOutput2
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomSum;  // Renders to bloomOutputSum
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom1R;   // Renders to bloomOutput1R
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloom2R;   // Renders to bloomOutput2R
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBloomSumR; // Renders to bloomOutputSumR
	// Ambient Occlusion
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBuf;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBufR;
	//ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBuf2;
	//ComPtr<ID3D11RenderTargetView> _renderTargetViewDepthBuf2R;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBentBuf;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewBentBufR;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAO;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAO_R;
	// Shading System
	ComPtr<ID3D11RenderTargetView> _renderTargetViewNormBuf;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewNormBufR;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAOMask;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSAOMaskR;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSMask;
	ComPtr<ID3D11RenderTargetView> _renderTargetViewSSMaskR;
	// Raytracing
	ComPtr<ID3D11RenderTargetView> _rtShadowMaskRTV;
	ComPtr<ID3D11RenderTargetView> _rtShadowMaskRTV_R;
	// 3D vision
	ComPtr<ID3D11RenderTargetView> _RTVvision3DPost; // Used for the "barrel" effect in 3D vision

	// SRVs
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputShaderResourceView;
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputShaderResourceViewR; // When SteamVR is enabled, this is the SRV for the right eye
	// Dynamic Cockpit
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputDynCockpitSRV;    // SRV for HUD elements without background
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputDynCockpitBG_SRV; // SRV for HUD element backgrounds
	ComPtr<ID3D11ShaderResourceView> _DCTextSRV;						// SRV for the HUD text
	ComPtr<ID3D11ShaderResourceView> _ReticleSRV;						// SRV for the HUD text
	// Shadertoy
	ComPtr<ID3D11ShaderResourceView> _shadertoySRV;
	ComPtr<ID3D11ShaderResourceView> _shadertoySRV_R;
	ComPtr<ID3D11ShaderResourceView> _shadertoyAuxSRV;
	ComPtr<ID3D11ShaderResourceView> _shadertoyAuxSRV_R;
	// Bloom
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputBloomMaskSRV;
	ComPtr<ID3D11ShaderResourceView> _offscreenAsInputBloomMaskSRV_R;
	ComPtr<ID3D11ShaderResourceView> _bloomOutput1SRV;     // SRV for bloomOutput1
	ComPtr<ID3D11ShaderResourceView> _bloomOutput2SRV;     // SRV for bloomOutput2
	ComPtr<ID3D11ShaderResourceView> _bloomOutputSumSRV;   // SRV for bloomOutputSum
	ComPtr<ID3D11ShaderResourceView> _bloomOutput1SRV_R;   // SRV for bloomOutput1R
	ComPtr<ID3D11ShaderResourceView> _bloomOutput2SRV_R;   // SRV for bloomOutput2R
	ComPtr<ID3D11ShaderResourceView> _bloomOutputSumSRV_R; // SRV for bloomOutputSumR
	// Ambient Occlusion
	ComPtr<ID3D11ShaderResourceView> _depthBufSRV;    // SRV for depthBufAsInput
	ComPtr<ID3D11ShaderResourceView> _depthBufSRV_R;  // SRV for depthBufAsInputR
	ComPtr<ID3D11ShaderResourceView> _bentBufSRV;     // SRV for bentBuf
	ComPtr<ID3D11ShaderResourceView> _bentBufSRV_R;   // SRV for bentBufR
	ComPtr<ID3D11ShaderResourceView> _ssaoBufSRV;     // SRV for ssaoBuf
	ComPtr<ID3D11ShaderResourceView> _ssaoBufSRV_R;   // SRV for ssaoBuf
	// Shading System
	ComPtr<ID3D11ShaderResourceView> _normBufSRV;     // SRV for normBuf
	ComPtr<ID3D11ShaderResourceView> _normBufSRV_R;   // SRV for normBufR
	ComPtr<ID3D11ShaderResourceView> _ssaoMaskSRV;    // SRV for ssaoMask
	ComPtr<ID3D11ShaderResourceView> _ssaoMaskSRV_R;  // SRV for ssaoMaskR
	ComPtr<ID3D11ShaderResourceView> _ssMaskSRV;      // SRV for ssMask
	ComPtr<ID3D11ShaderResourceView> _ssMaskSRV_R;    // SRV for ssMaskR
	// Shadow Mapping
	ComPtr<ID3D11ShaderResourceView> _shadowMapArraySRV; // This is an array SRV
	ComPtr<ID3D11ShaderResourceView> _csmArraySRV;       // Cascaded Shadow Map array SRV
	//ComPtr<ID3D11ShaderResourceView> _shadowMapSingleSRV;
	//ComPtr<ID3D11ShaderResourceView> _shadowMapSRV_R;
	// Generated/Procedural Textures SRVs
	ComPtr<ID3D11ShaderResourceView> _grayNoiseSRV; // SRV for _grayNoise
	// 3D Vision
	ComPtr<ID3D11ShaderResourceView> _vision3DSignatureSRV; // SRV for _vision3DSignature
	// Raytracing
	ComPtr<ID3D11ShaderResourceView> _RTBvhSRV;
	ComPtr<ID3D11ShaderResourceView> _RTTLASBvhSRV;
	ComPtr<ID3D11ShaderResourceView> _RTMatricesSRV;
	// Raytracing Shadow Mask
	ComPtr<ID3D11ShaderResourceView> _rtShadowMaskSRV;
	ComPtr<ID3D11ShaderResourceView> _rtShadowMaskSRV_R;

	ComPtr<ID3D11Texture2D> _depthStencilL;
	ComPtr<ID3D11Texture2D> _depthStencilR;
	ComPtr<ID3D11DepthStencilView> _depthStencilViewL;
	ComPtr<ID3D11DepthStencilView> _depthStencilViewR;
	ComPtr<ID3D11DepthStencilView> _shadowMapDSV;
	ComPtr<ID3D11DepthStencilView> _csmMapDSV;
	//ComPtr<ID3D11DepthStencilView> _shadowMapDSV_R; // Do I really need a shadow map for the right eye? I don't think so...

	ComPtr<ID2D1Factory> _d2d1Factory;
	ComPtr<IDWriteFactory> _dwriteFactory;
	ComPtr<ID2D1RenderTarget> _d2d1RenderTarget;
	ComPtr<ID2D1RenderTarget> _d2d1OffscreenRenderTarget;
	ComPtr<ID2D1RenderTarget> _d2d1DCRenderTarget;
	ComPtr<ID2D1DrawingStateBlock> _d2d1DrawingStateBlock;

	ComPtr<ID3D11VertexShader> _mainVertexShader;
	ComPtr<ID3D11InputLayout> _mainInputLayout;
	ComPtr<ID3D11PixelShader> _mainPixelShader;
	ComPtr<ID3D11PixelShader> _mainPixelShaderBpp2ColorKey20;
	ComPtr<ID3D11PixelShader> _mainPixelShaderBpp2ColorKey00;
	ComPtr<ID3D11PixelShader> _mainPixelShaderBpp4ColorKey20;
	ComPtr<ID3D11PixelShader> _steamVRMirrorPixelShader;
	ComPtr<ID3D11PixelShader> _barrelPixelShader;
	ComPtr<ID3D11PixelShader> _simpleResizePS;
	ComPtr<ID3D11PixelShader> _bloomHGaussPS;
	ComPtr<ID3D11PixelShader> _bloomVGaussPS;
	ComPtr<ID3D11PixelShader> _bloomCombinePS;
	ComPtr<ID3D11PixelShader> _bloomBufferAddPS;
	ComPtr<ID3D11PixelShader> _ssaoPS;
	ComPtr<ID3D11PixelShader> _ssaoBlurPS;
	ComPtr<ID3D11PixelShader> _ssaoAddPS;
	ComPtr<ID3D11PixelShader> _ssdoDirectPS;
	ComPtr<ID3D11PixelShader> _ssdoIndirectPS;
	ComPtr<ID3D11PixelShader> _ssdoAddPS;
	ComPtr<ID3D11PixelShader> _pbrAddPS;
	ComPtr<ID3D11PixelShader> _headLightsPS;
	ComPtr<ID3D11PixelShader> _headLightsSSAOPS;
	ComPtr<ID3D11PixelShader> _ssdoBlurPS;
	ComPtr<ID3D11PixelShader> _deathStarPS;
	ComPtr<ID3D11PixelShader> _hyperEntryPS;
	ComPtr<ID3D11PixelShader> _hyperExitPS;
	ComPtr<ID3D11PixelShader> _hyperTunnelPS;
	ComPtr<ID3D11PixelShader> _hyperZoomPS;
	ComPtr<ID3D11PixelShader> _hyperComposePS;
	ComPtr<ID3D11PixelShader> _laserPointerPS;
	ComPtr<ID3D11PixelShader> _fxaaPS;
	ComPtr<ID3D11PixelShader> _externalHUDPS;
	ComPtr<ID3D11PixelShader> _sunShaderPS;
	ComPtr<ID3D11PixelShader> _sunFlareShaderPS;
	ComPtr<ID3D11PixelShader> _sunFlareComposeShaderPS;
	ComPtr<ID3D11PixelShader> _edgeDetectorPS;
	ComPtr<ID3D11PixelShader> _starDebugPS;
	ComPtr<ID3D11PixelShader> _lavaPS;
	ComPtr<ID3D11PixelShader> _explosionPS;
	ComPtr<ID3D11PixelShader> _alphaToBloomPS;
	ComPtr<ID3D11PixelShader> _noGlassPS;
	// The following is not a mistake. We need an ID3D11PixelShader *, not a
	// ComPtr<ID3D11PixelShader>. Not entirely sure why, but I believe using
	// the ComPtr inadvertently messes up the refcount and causes a crash when
	// starting the game. Just as quickly as InitPixelShader() is called.
	ID3D11PixelShader *_currentPixelShader;
	ComPtr<ID3D11SamplerState> _repeatSamplerState;
	ComPtr<ID3D11SamplerState> _noInterpolationSamplerState;
	
	ComPtr<ID3D11PixelShader> _speedEffectPS;
	ComPtr<ID3D11PixelShader> _speedEffectComposePS;
	ComPtr<ID3D11PixelShader> _addGeomPS;
	ComPtr<ID3D11PixelShader> _addGeomComposePS;

	ComPtr<ID3D11PixelShader> _shadowMapPS;
	ComPtr<ID3D11SamplerState> _shadowPCFSamplerState;

	ComPtr<ID3D11PixelShader> _singleBarrelPixelShader;
	ComPtr<ID3D11RasterizerState> _mainRasterizerState;
	ComPtr<ID3D11SamplerState> _mainSamplerState;
	ComPtr<ID3D11SamplerState> _mirrorSamplerState;
	ComPtr<ID3D11BlendState> _mainBlendState;
	ComPtr<ID3D11DepthStencilState> _mainDepthState;
	ComPtr<ID3D11Buffer> _mainVertexBuffer;
	ComPtr<ID3D11Buffer> _steamVRPresentVertexBuffer; // Used in SteamVR mode to correct the image presented on the monitor
	ComPtr<ID3D11Buffer> _mainIndexBuffer;
	ComPtr<ID3D11Texture2D> _mainDisplayTexture;
	ComPtr<ID3D11ShaderResourceView> _mainDisplayTextureView;
	ComPtr<ID3D11Texture2D> _mainDisplayTextureTemp;
	ComPtr<ID3D11ShaderResourceView> _mainDisplayTextureViewTemp;

	ComPtr<ID3D11VertexShader> _vertexShader;
	ComPtr<ID3D11VertexShader> _sbsVertexShader;
	ComPtr<ID3D11VertexShader> _datVertexShaderVR;
	ComPtr<ID3D11VertexShader> _passthroughVertexShader;
	ComPtr<ID3D11VertexShader> _speedEffectVS;
	ComPtr<ID3D11VertexShader> _addGeomVS;
	ComPtr<ID3D11VertexShader> _shadowMapVS;
	ComPtr<ID3D11VertexShader> _hangarShadowMapVS;
	ComPtr<ID3D11VertexShader> _csmVS;
	ComPtr<ID3D11InputLayout> _inputLayout;
	ComPtr<ID3D11InputLayout> _shadowMapInputLayout;
	ComPtr<ID3D11PixelShader> _pixelShaderTexture;
	ComPtr<ID3D11PixelShader> _pixelShaderDC;
	ComPtr<ID3D11PixelShader> _pixelShaderDCHolo;
	ComPtr<ID3D11PixelShader> _pixelShaderEmptyDC;
	ComPtr<ID3D11PixelShader> _pixelShaderHUD;
	ComPtr<ID3D11PixelShader> _pixelShaderSolid;
	ComPtr<ID3D11PixelShader> _pixelShaderClearBox;
	ComPtr<ID3D11PixelShader> _pixelShaderAnim;
	ComPtr<ID3D11PixelShader> _pixelShaderAnimDAT;
	ComPtr<ID3D11PixelShader> _pixelShaderGreeble;
	ComPtr<ID3D11PixelShader> _levelsPS;
	ComPtr<ID3D11PixelShader> _rtShadowMaskPS;

	ComPtr<ID3D11RasterizerState> _rasterizerState;
	//ComPtr<ID3D11RasterizerState> _sm_rasterizerState; // TODO: Remove this if proven useless
	ComPtr<ID3D11Buffer> _VSConstantBuffer;
	ComPtr<ID3D11Buffer> _VSMatrixBuffer;
	ComPtr<ID3D11Buffer> _shadingSysBuffer;
	ComPtr<ID3D11Buffer> _PSConstantBuffer;
	ComPtr<ID3D11Buffer> _PSConstantBufferDC;
	ComPtr<ID3D11Buffer> _barrelConstantBuffer;
	ComPtr<ID3D11Buffer> _bloomConstantBuffer;
	ComPtr<ID3D11Buffer> _ssaoConstantBuffer;
	ComPtr<ID3D11Buffer> _hyperspaceConstantBuffer;
	ComPtr<ID3D11Buffer> _laserPointerConstantBuffer;
	ComPtr<ID3D11Buffer> _mainShadersConstantBuffer;
	ComPtr<ID3D11Buffer> _shadowMappingVSConstantBuffer;
	ComPtr<ID3D11Buffer> _shadowMappingPSConstantBuffer;
	ComPtr<ID3D11Buffer> _metricRecVSConstantBuffer;
	ComPtr<ID3D11Buffer> _metricRecPSConstantBuffer;

	ComPtr<ID3D11Buffer> _postProcessVertBuffer;
	ComPtr<ID3D11Buffer> _HUDVertexBuffer;
	ComPtr<ID3D11Buffer> _clearHUDVertexBuffer;
	ComPtr<ID3D11Buffer> _hyperspaceVertexBuffer;
	ComPtr<ID3D11Buffer> _speedParticlesVertexBuffer;
	ComPtr<ID3D11Buffer> _shadowVertexBuffer;
	ComPtr<ID3D11Buffer> _shadowIndexBuffer;
	ComPtr<ID3D11Buffer> _OPTMeshTransformCB;

	// Raytracing
	ComPtr<ID3D11Buffer> _RTConstantsBuffer;
	ComPtr<ID3D11Buffer> _RTBvh;
	ComPtr<ID3D11Buffer> _RTTLASBvh;
	ComPtr<ID3D11Buffer> _RTMatrices;

	//ComPtr<ID3D11Buffer> _reticleVertexBuffer;
	bool _bHUDVerticesReady;

	// Dynamic Cockpit coverTextures:
	// The line below had a hard-coded max of 40. I think it should be 
	// MAX_DC_SRC_ELEMENTS instead... but if something explodes in DC, it
	// might be because of this.
	// I think it was just dumb luck that I put the "40" in there and didn't
	// overflow it when I added more slots.
	ComPtr<ID3D11ShaderResourceView> dc_coverTexture[MAX_DC_SRC_ELEMENTS];

	// Extra textures. For now, these textures will be used to do animations.
	// We'll simply put the animation textures here, one after the other, no
	// matter where they come from. The materials should have pointers/indices
	// into this vector and it should be cleared every time a new mission
	// is loaded (or just piggy-back where we clear the material properties)
	//std::vector<ComPtr<ID3D11ShaderResourceView>> _extraTextures;
	std::vector<ID3D11ShaderResourceView *> _extraTextures;

	//ComPtr<ID3D11ShaderResourceView> _extraTextures[MAX_EXTRA_TEXTURES];
	//int _numExtraTextures;

	BOOL _useAnisotropy;
	BOOL _useMultisampling;
	DXGI_SAMPLE_DESC _sampleDesc;
	UINT _backbufferWidth;
	UINT _backbufferHeight;
	DXGI_RATIONAL _refreshRate;
	bool _are16BppTexturesSupported;
	bool _use16BppMainDisplayTexture;
	DWORD _mainDisplayTextureBpp;

	float clearColor[4];
	float clearColorRGBA[4];
	float clearDepth;
	bool sceneRendered;
	bool sceneRenderedEmpty;
	bool inScene;
	bool inSceneBackbufferLocked;

	int _tgSmushTexWidth;
	int _tgSmushTexHeight;

	PrimarySurface* _primarySurface;
	DepthSurface* _depthSurface;
	BackbufferSurface* _backbufferSurface;
	FrontbufferSurface* _frontbufferSurface;
	OffscreenSurface* _offscreenSurface;

	void (*_surfaceDcCallback)();
};
