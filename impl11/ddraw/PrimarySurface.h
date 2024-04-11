// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019
#pragma once

#include "EffectsCommon.h"

class BackbufferSurface;
class Direct3DTexture;

void InitHeadingMatrix();
Matrix4 GetCurrentHeadingMatrix(Vector4 &Rs, Vector4 &Us, Vector4 &Fs, bool invert, bool debug);
Matrix4 GetCurrentHeadingViewMatrix();
void UpdateViewMatrix();

class PrimarySurface : public IDirectDrawSurface
{
	// These variables are used with SaveContext() and RestoreContext()
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
	D3D11_PRIMITIVE_TOPOLOGY _oldTopology;
	UINT _oldStencilRef, _oldSampleMask;
	FLOAT _oldBlendFactor[4];
	UINT _oldStride, _oldOffset, _oldIOffset;
	DXGI_FORMAT _oldFormat;
	D3D11_VIEWPORT _oldViewports[2];
	UINT _oldNumViewports = 2;

public:
	PrimarySurface(DeviceResources* deviceResources, bool hasBackbufferAttached);

	virtual ~PrimarySurface();

	/*** IUnknown methods ***/

	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj);

	STDMETHOD_(ULONG, AddRef) (THIS);

	STDMETHOD_(ULONG, Release) (THIS);

	/*** IDirectDrawSurface methods ***/

	STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE);

	STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT);

	STDMETHOD(Blt)(THIS_ LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);

	STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD);

	STDMETHOD(BltFast)(THIS_ DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD);

	STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD, LPDIRECTDRAWSURFACE);

	STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID, LPDDENUMSURFACESCALLBACK);

	STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD, LPVOID, LPDDENUMSURFACESCALLBACK);

	void SetScissoRectFullScreen();
	void SaveContext();
	void RestoreContext();

	void barrelEffect2D(int iteration);

	void resizeForSteamVR(int iteration, bool is_2D);

	void barrelEffect3D();

	void barrelEffectSteamVR();

	void BloomBasicPass(int pass, float fZoomFactor);

	void BloomPyramidLevelPass(int PyramidLevel, int AdditionalPasses, float fZoomFactor, bool debug);

	void capture(int time_delay, ComPtr<ID3D11Texture2D> buffer, const wchar_t *filename);

	void ClearBox(uvfloat4 box, D3D11_VIEWPORT * viewport, D3DCOLOR clearColor);

	int ClearHUDRegions();

	void DrawHUDVertices();

	//void SetLights(float fSSDOEnabled);
	
	void SSAOPass(float fZoomFactor);

	void SSDOPass(float fZoomFactor, float fZoomFactor2);

	void DeferredPass();

	//void InitHeadingMatrix();

	//Matrix4 GetCurrentHeadingMatrix(Vector4 &Rs, Vector4 &Us, Vector4 &Fs, bool invert, bool debug);

	//Matrix4 GetCurrentHeadingViewMatrix();

	//void GetCockpitViewMatrix(Matrix4 * result, bool invert);

	void GetHyperspaceEffectMatrix(Matrix4 *result);

	//void GetCockpitViewMatrixSpeedEffect(Matrix4 * result, bool invert);

	//void GetGunnerTurretViewMatrix(Matrix4 * result);

	//void GetCraftViewMatrix(Matrix4 *result);

	void RenderHyperspaceEffect(D3D11_VIEWPORT *lastViewport,
		ID3D11PixelShader *lastPixelShader, Direct3DTexture *lastTextureSelected,
		ID3D11Buffer *lastVertexBuffer, UINT *lastVertexBufStride, UINT *lastVertexBufOffset);

	void RenderFXAA();

	void RenderLevels();

	void RenderStarDebug();

	void RenderExternalHUD();

	inline void AddSpeedPoint(const Matrix4 &H, D3DTLVERTEX *particles, Vector4 Q, float zdisp, int ofs, float craft_speed);

	int AddGeometry(const Matrix4 & ViewMatrix, D3DTLVERTEX * particles, Vector4 Q, float zdisp, int ofs);

	Matrix4 ComputeAddGeomViewMatrix(Matrix4 *HeadingMatrix, Matrix4 *CockpitMatrix);

	void RenderAdditionalGeometry();

	//Matrix4 GetShadowMapLimits(Matrix4 L, float *OBJrange, float *OBJminZ);

	void OldTagXWALights();

	void TagXWALights();

	void TagAndFadeXWALights();

	void RenderSpeedEffect();

	D3DCOLOR EncodeNormal(Vector3 N);

	void ProjectCentroidToPostProc(Vector3 Centroid, float *u, float *v);

	void RenderSunFlare();

	void OPTVertexToSteamVRPostProcCoords(Vector4 P, Vector4 pos2D[2]);

	void RenderLaserPointer(D3D11_VIEWPORT * lastViewport, ID3D11PixelShader * lastPixelShader, Direct3DTexture * lastTextureSelected, ID3D11Buffer * lastVertexBuffer, UINT * lastVertexBufStride, UINT * lastVertexBufOffset);

	void Add3DVisionSignature();

	void RenderRTShadowMask();

	void RenderEdgeDetector();

	STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD);

	STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *);

	STDMETHOD(GetBltStatus)(THIS_ DWORD);

	STDMETHOD(GetCaps)(THIS_ LPDDSCAPS);

	STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*);

	STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY);

	STDMETHOD(GetDC)(THIS_ HDC FAR *);

	STDMETHOD(GetFlipStatus)(THIS_ DWORD);

	STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG);

	STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*);

	STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT);

	STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC);

	STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC);

	STDMETHOD(IsLost)(THIS);

	STDMETHOD(Lock)(THIS_ LPRECT, LPDDSURFACEDESC, DWORD, HANDLE);

	STDMETHOD(ReleaseDC)(THIS_ HDC);

	STDMETHOD(Restore)(THIS);

	STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER);

	STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY);

	STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG);

	STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE);

	STDMETHOD(Unlock)(THIS_ LPVOID);

	STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDOVERLAYFX);

	STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD);

	STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE);

	void RenderText();

	void RenderRadar();

	void RenderBracket();

	void CacheBracketsVR();

	void RenderSkyBoxVR(bool debug);

	void RenderSynthDCElems();

	ULONG _refCount;

	DeviceResources* _deviceResources;

	bool _hasBackbufferAttached;
	ComPtr<BackbufferSurface> _backbufferSurface;

	UINT _flipFrames;
};

void GetCockpitViewMatrixSpeedEffect(Matrix4* result, bool invert);
void SetLights(DeviceResources *resources, float fSSDOEnabled);