// Copyright (c) 2014 J�r�my Ansel
// Licensed under the MIT license. See LICENSE.txt

#include "common.h"
#include "DeviceResources.h"
#include "OffscreenSurface.h"
#include "SurfaceDC.h"

OffscreenSurface::OffscreenSurface(DeviceResources* deviceResources)
{
	this->_refCount = 1;
	this->_deviceResources = deviceResources;

	this->_bufferSize = this->_deviceResources->_displayHeight * this->_deviceResources->_displayWidth * 2;
	this->_buffer = new char[this->_bufferSize];
	memset(this->_buffer, 0, this->_bufferSize);
}

OffscreenSurface::~OffscreenSurface()
{
	if (this->_deviceResources->_offscreenSurface == this)
	{
		this->_deviceResources->_offscreenSurface = nullptr;
	}

	delete[] this->_buffer;
}

HRESULT OffscreenSurface::QueryInterface(
	REFIID riid,
	LPVOID* obp
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tE_NOINTERFACE");
	LogText(str.str());
#endif

	return E_NOINTERFACE;
}

ULONG OffscreenSurface::AddRef()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount++;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	return this->_refCount;
}

ULONG OffscreenSurface::Release()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	this->_refCount--;

#if LOGGER
	str.str("");
	str << "\t" << this->_refCount;
	LogText(str.str());
#endif

	if (this->_refCount == 0)
	{
		delete this;
		return 0;
	}

	return this->_refCount;
}

HRESULT OffscreenSurface::AddAttachedSurface(
	LPDIRECTDRAWSURFACE lpDDSAttachedSurface
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::AddOverlayDirtyRect(
	LPRECT lpRect
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::Blt(
	LPRECT lpDestRect,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DWORD dwFlags,
	LPDDBLTFX lpDDBltFx
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	str << tostr_RECT(lpDestRect);
	str << " " << lpDDSrcSurface;
	str << tostr_RECT(lpSrcRect);

	if ((dwFlags & DDBLT_COLORFILL) != 0 && lpDDBltFx != nullptr)
	{
		str << " " << (void*)lpDDBltFx->dwFillColor;
	}

	LogText(str.str());
#endif

	if (dwFlags & DDBLT_COLORFILL)
	{
		if (lpDDBltFx == nullptr)
		{
#if LOGGER
			str.str("\tDDERR_INVALIDPARAMS");
			LogText(str.str());
#endif

			return DDERR_INVALIDPARAMS;
		}

		unsigned short* buffer = (unsigned short*)this->_buffer;
		int length = this->_bufferSize / 2;
		unsigned short color = (unsigned short)lpDDBltFx->dwFillColor;

		for (int i = 0; i < length; i++)
		{
			buffer[i] = color;
		}

		return DD_OK;
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::BltBatch(
	LPDDBLTBATCH lpDDBltBatch,
	DWORD dwCount,
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::BltFast(
	DWORD dwX,
	DWORD dwY,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DWORD dwTrans
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::DeleteAttachedSurface(
	DWORD dwFlags,
	LPDIRECTDRAWSURFACE lpDDSAttachedSurface
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::EnumAttachedSurfaces(
	LPVOID lpContext,
	LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::EnumOverlayZOrders(
	DWORD dwFlags,
	LPVOID lpContext,
	LPDDENUMSURFACESCALLBACK lpfnCallback
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::Flip(
	LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride,
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetAttachedSurface(
	LPDDSCAPS lpDDSCaps,
	LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetBltStatus(
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetCaps(
	LPDDSCAPS lpDDSCaps
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetClipper(
	LPDIRECTDRAWCLIPPER FAR *lplpDDClipper
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetColorKey(
	DWORD dwFlags,
	LPDDCOLORKEY lpDDColorKey
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetDC(
	HDC FAR *lphDC
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (this->_deviceResources->IsInConcourseHd())
	{
		if (lphDC)
		{
			SurfaceDC* pDC = (SurfaceDC*)lphDC;

			pDC->buffer32 = 0;
			if (g_bUseSteamVR)
			{
				// For the HD Concourse, we need to specify HD width/height or else
				// the resulting image will be cropped
				pDC->width = HD_CONCOURSE_WIDTH;
				pDC->height = HD_CONCOURSE_HEIGHT;
			}
			else
			{
				pDC->width = this->_deviceResources->_backbufferWidth;
				pDC->height = this->_deviceResources->_backbufferHeight;
			}

			pDC->displayWidth = this->_deviceResources->_displayWidth;
			pDC->displayHeight = this->_deviceResources->_displayHeight;
			pDC->aspectRatioPreserved = g_config.AspectRatioPreserved;
			pDC->callback = &this->_deviceResources->_surfaceDcCallback;

			// FX ddraw version:
			pDC->d2d1RenderTarget = this->_deviceResources->_d2d1OffscreenRenderTarget;
			// Golden ddraw version:
			//pDC->d2d1RenderTarget = this->_deviceResources->_d2d1RenderTarget;

			pDC->d3d11RenderTargetView = (g_bUseSteamVR && g_config.HDConcourseEnabled) ?
				this->_deviceResources->_renderTargetViewHd :
				this->_deviceResources->_renderTargetView;
			pDC->d3d11Device = this->_deviceResources->_d3dDevice;
			pDC->d3d11DeviceContext = this->_deviceResources->_d3dDeviceContext;
		}

		return DD_OK;
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetFlipStatus(
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetOverlayPosition(
	LPLONG lplX,
	LPLONG lplY
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetPalette(
	LPDIRECTDRAWPALETTE FAR *lplpDDPalette
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetPixelFormat(
	LPDDPIXELFORMAT lpDDPixelFormat
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::GetSurfaceDesc(
	LPDDSURFACEDESC lpDDSurfaceDesc
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpDDSurfaceDesc == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	*lpDDSurfaceDesc = {};
	lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
	lpDDSurfaceDesc->dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH;
	lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = 16;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0xF800;
	lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x7E0;
	lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x1F;
	lpDDSurfaceDesc->dwHeight = this->_deviceResources->_displayHeight;
	lpDDSurfaceDesc->dwWidth = this->_deviceResources->_displayWidth;
	lpDDSurfaceDesc->lPitch = this->_deviceResources->_displayWidth * 2;

#if LOGGER
	str.str("");
	str << "\t" << tostr_DDSURFACEDESC(lpDDSurfaceDesc);
	LogText(str.str());
#endif

	return DD_OK;
}

HRESULT OffscreenSurface::Initialize(
	LPDIRECTDRAW lpDD,
	LPDDSURFACEDESC lpDDSurfaceDesc
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::IsLost()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::Lock(
	LPRECT lpDestRect,
	LPDDSURFACEDESC lpDDSurfaceDesc,
	DWORD dwFlags,
	HANDLE hEvent
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	str << tostr_RECT(lpDestRect);
	LogText(str.str());
#endif

	if (lpDDSurfaceDesc == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	if (lpDestRect == nullptr)
	{
		*lpDDSurfaceDesc = {};
		lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
		lpDDSurfaceDesc->dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
		lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
		lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = 16;
		lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0xF800;
		lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x7E0;
		lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x1F;
		lpDDSurfaceDesc->dwHeight = this->_deviceResources->_displayHeight;
		lpDDSurfaceDesc->dwWidth = this->_deviceResources->_displayWidth;
		lpDDSurfaceDesc->lPitch = this->_deviceResources->_displayWidth * 2;
		lpDDSurfaceDesc->lpSurface = this->_buffer;

		return DD_OK;
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::ReleaseDC(
	HDC hDC
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::Restore()
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::SetClipper(
	LPDIRECTDRAWCLIPPER lpDDClipper
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::SetColorKey(
	DWORD dwFlags,
	LPDDCOLORKEY lpDDColorKey
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	str << " " << (void*)lpDDColorKey->dwColorSpaceLowValue << " " << (void*)lpDDColorKey->dwColorSpaceHighValue;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDD_OK");
	LogText(str.str());
#endif

	return DD_OK;
}

HRESULT OffscreenSurface::SetOverlayPosition(
	LONG lX,
	LONG lY
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::SetPalette(
	LPDIRECTDRAWPALETTE lpDDPalette
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::Unlock(
	LPVOID lpSurfaceData
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (g_config.HDConcourseEnabled)
	{
		const int frameIndex = *(int*)(0x009F60E0 + 0x2B361);
		const int currentGameState = *(int*)(0x009F60E0 + 0x25FA9);
		const int updateCallback = *(int*)(0x009F60E0 + 0x25FB1 + 0x850 * currentGameState + 0x0844);
		const bool isConfigMenuGameStateUpdate = updateCallback == 0x0051D100;
		const bool isMessageBoxGameStateUpdate = updateCallback == 0x005595A0 || updateCallback == 0x00541260;
		const bool isTechLibraryGameStateUpdate = updateCallback == 0x00574D70;
		const bool isL00559E10GameStateUpdate = updateCallback == 0x00559E10;

		bool copyHdBackground = true;

		if (isConfigMenuGameStateUpdate || isMessageBoxGameStateUpdate || isL00559E10GameStateUpdate)
		{
			copyHdBackground = false;
		}
		else if (!this->_deviceResources->IsInConcourseHd())
		{
			copyHdBackground = false;
		}
		else if (isTechLibraryGameStateUpdate && frameIndex != 0)
		{
			copyHdBackground = false;
		}

		if (copyHdBackground)
		{
			if (g_bUseSteamVR)
			{
				extern bool g_bInSkirmishShipScreen;
				extern bool g_bRendering3D;

				// The sequence of draw calls when the Skirmish OPT is rendered is weird. The draw calls
				// happen in SteamVRRenderer::RenderSkirmishOPT(), and the OPT itself appears to be rendered
				// twice. At the middle point, _this_ method is called twice and _offscreenBufferHdBackground is
				// overwritten. When this happens, for some reason, the existing OPT render gets wiped out and
				// we end up with an empty render. I suspect that it gets wiped out because the HdBackground is
				// rendered as a quad with a depth value that erases the OPT (i.e. the quad is "on top" of the
				// OPT). To prevent this, I'm clearing the _depthStencilViewHd so that the second time the OPT
				// is rendered, it can start on a clear depth stencil. Seems to work, but it's still weird.
				if (!g_bRendering3D &&
					g_config.HDConcourseEnabled &&
					g_config.D3dRendererTexturesHookEnabled &&
					g_bInSkirmishShipScreen)
				{
					auto& resources = _deviceResources;
					auto& context   = resources->_d3dDeviceContext;
					context->ClearDepthStencilView(resources->_depthStencilViewHd, D3D11_CLEAR_DEPTH, 0.0f, 0);
				}

				this->_deviceResources->_d3dDeviceContext->CopyResource(this->_deviceResources->_offscreenBufferHdBackground, this->_deviceResources->_offscreenBufferHd);
			}
			else
				this->_deviceResources->_d3dDeviceContext->CopyResource(this->_deviceResources->_offscreenBufferHdBackground, this->_deviceResources->_offscreenBuffer);
		}
	}

	return DD_OK;
}

HRESULT OffscreenSurface::UpdateOverlay(
	LPRECT lpSrcRect,
	LPDIRECTDRAWSURFACE lpDDDestSurface,
	LPRECT lpDestRect,
	DWORD dwFlags,
	LPDDOVERLAYFX lpDDOverlayFx
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::UpdateOverlayDisplay(
	DWORD dwFlags
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT OffscreenSurface::UpdateOverlayZOrder(
	DWORD dwFlags,
	LPDIRECTDRAWSURFACE lpDDSReference
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}
