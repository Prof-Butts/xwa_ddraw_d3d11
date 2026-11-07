// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
#include "common.h"
#include "globals.h"
#include "TextureSurface.h"
#include "MipmapSurface.h"
#include "Direct3DTexture.h"

std::vector<char>* g_textureSurfaceBuffer = nullptr;

// Measurements for LogTextureBufferPeaks(): the largest size each texture buffer reached and
// how many times the buffers were freed, since the last log. The buffers only grow until
// ClearTextureBuffers() frees them, so sampling them before they're freed is enough.
static size_t s_textureBufferPeaks[5] = {};
static int s_textureBufferFrees = 0;
static int s_largeTextureBufferReleases = 0;
// The buffers as they were when the last log was written, so they aren't counted again
static const void* s_loggedBufferData[5] = {};
static size_t s_loggedBufferSize[5] = {};

static void GetTextureBuffers(const void* data[5], size_t sizes[5])
{
	data[0] = g_textureSurfaceBuffer != nullptr ? g_textureSurfaceBuffer->data() : nullptr;
	data[1] = g_d3dTextureBuffer     != nullptr ? g_d3dTextureBuffer->data()     : nullptr;
	data[2] = g_colorMapBuffer       != nullptr ? g_colorMapBuffer->data()       : nullptr;
	data[3] = g_illumMapBuffer       != nullptr ? g_illumMapBuffer->data()       : nullptr;
	data[4] = g_loadDatImageBuffer   != nullptr ? g_loadDatImageBuffer->data()   : nullptr;
	sizes[0] = g_textureSurfaceBuffer != nullptr ? g_textureSurfaceBuffer->capacity() : 0;
	sizes[1] = g_d3dTextureBuffer     != nullptr ? g_d3dTextureBuffer->capacity()     : 0;
	sizes[2] = g_colorMapBuffer       != nullptr ? g_colorMapBuffer->capacity()       : 0;
	sizes[3] = g_illumMapBuffer       != nullptr ? g_illumMapBuffer->capacity()       : 0;
	sizes[4] = g_loadDatImageBuffer   != nullptr ? g_loadDatImageBuffer->capacity()   : 0;
}

static void UpdateTextureBufferPeaks()
{
	const void* data[5];
	size_t sizes[5];
	GetTextureBuffers(data, sizes);
	for (int i = 0; i < 5; i++)
	{
		// Skip the buffers that haven't changed since the last log: they were already reported
		if (data[i] != nullptr && data[i] == s_loggedBufferData[i] && sizes[i] == s_loggedBufferSize[i])
			continue;
		if (sizes[i] > s_textureBufferPeaks[i])
			s_textureBufferPeaks[i] = sizes[i];
	}
}

void LogTextureBufferPeaks(const char* tag)
{
	UpdateTextureBufferPeaks();
	const void* data[5];
	size_t sizes[5];
	GetTextureBuffers(data, sizes);
	if (g_config.LogAddressSpace)
	{
		const double MB = 1024.0 * 1024.0;
		log_debug("[DBG] [MEM] %s: texture buffer peaks: surface %0.1f MB, D3D texture %0.1f MB, "
			"color map %0.1f MB, illum map %0.1f MB, DAT image %0.1f MB; freed %d times, big buffers freed after use %d times",
			tag, s_textureBufferPeaks[0] / MB, s_textureBufferPeaks[1] / MB, s_textureBufferPeaks[2] / MB,
			s_textureBufferPeaks[3] / MB, s_textureBufferPeaks[4] / MB, s_textureBufferFrees, s_largeTextureBufferReleases);
		log_debug("[DBG] [MEM] %s: texture buffers allocated now: surface %0.1f MB, D3D texture %0.1f MB, "
			"color map %0.1f MB, illum map %0.1f MB, DAT image %0.1f MB",
			tag, sizes[0] / MB, sizes[1] / MB, sizes[2] / MB, sizes[3] / MB, sizes[4] / MB);
	}
	for (int i = 0; i < 5; i++)
	{
		s_textureBufferPeaks[i] = 0;
		s_loggedBufferData[i] = data[i];
		s_loggedBufferSize[i] = sizes[i];
	}
	s_textureBufferFrees = 0;
	s_largeTextureBufferReleases = 0;
}

template<typename T>
static void ReleaseLargeTextureBufferImpl(std::vector<T>*& buffer)
{
	const size_t LARGE_TEXTURE_BUFFER_SIZE = 16 * 1024 * 1024;
	if (buffer == nullptr || buffer->capacity() <= LARGE_TEXTURE_BUFFER_SIZE)
		return;

	// Record the size before the buffer goes away
	UpdateTextureBufferPeaks();
	for (int i = 0; i < 5; i++)
		if (s_loggedBufferData[i] == buffer->data())
			s_loggedBufferData[i] = nullptr;

	delete buffer;
	buffer = nullptr;
	s_largeTextureBufferReleases++;
}

void ReleaseLargeTextureBuffer(std::vector<char>*& buffer)
{
	ReleaseLargeTextureBufferImpl(buffer);
}

void ReleaseLargeTextureBuffer(std::vector<unsigned char>*& buffer)
{
	ReleaseLargeTextureBufferImpl(buffer);
}

void ClearTextureBuffers()
{
	UpdateTextureBufferPeaks();
	if (g_textureSurfaceBuffer != nullptr || g_d3dTextureBuffer != nullptr || g_colorMapBuffer != nullptr ||
		g_illumMapBuffer != nullptr || g_loadDatImageBuffer != nullptr)
		s_textureBufferFrees++;
	// The buffers are freed below, their memory can be reused by new buffers
	for (int i = 0; i < 5; i++)
		s_loggedBufferData[i] = nullptr;

	if (g_textureSurfaceBuffer != nullptr)
		delete g_textureSurfaceBuffer;
	if (g_loadDatImageBuffer != nullptr)
		delete g_loadDatImageBuffer;
	if (g_d3dTextureBuffer != nullptr)
		delete g_d3dTextureBuffer;
	if (g_colorMapBuffer != nullptr)
		delete g_colorMapBuffer;
	if (g_illumMapBuffer != nullptr)
		delete g_illumMapBuffer;

	g_textureSurfaceBuffer = nullptr;
	g_loadDatImageBuffer = nullptr;
	g_d3dTextureBuffer = nullptr;
	g_colorMapBuffer = nullptr;
	g_illumMapBuffer = nullptr;
	//g_textureSurfaceBuffer.swap(std::vector<char>());
}

TextureSurface::TextureSurface(DeviceResources* deviceResources, bool allocOnLoad, DWORD width, DWORD height, DDPIXELFORMAT& pixelFormat, DWORD mipmapCount, const char* name)
{
	this->_refCount = 1;
	this->_deviceResources = deviceResources;

	this->_allocOnLoad = allocOnLoad;

	this->_width = width;
	this->_height = height;
	this->_pixelFormat = pixelFormat;
	this->_mipmapCount = mipmapCount;

	if (name != nullptr)
	{
		this->_name = std::string(name);
	}

	if (this->_allocOnLoad || (this->_pixelFormat.dwRGBBitCount == 32 && this->_mipmapCount == 1))
	{
		this->_bufferSize = 0;
		this->_buffer = nullptr;
	}
	else
	{
		this->_bufferSize = this->_width * this->_height * this->_pixelFormat.dwRGBBitCount / 8 * 2;
		this->_buffer = new char[this->_bufferSize];
		//memset(this->_buffer, 0, this->_bufferSize);

		if (this->_mipmapCount > 1)
		{
			*this->_mipmap.GetAddressOf() = new MipmapSurface(this->_deviceResources, this, max(this->_width / 2, 1), max(this->_height / 2, 1), this->_pixelFormat, this->_mipmapCount - 1, this->_buffer + this->_bufferSize / 2);
		}
	}

	*this->_d3dTexture.GetAddressOf() = new Direct3DTexture(this->_deviceResources, this);
}

TextureSurface::~TextureSurface()
{
	if (this->_buffer != nullptr)
	{
		delete[] this->_buffer;
		this->_buffer = nullptr;
	}
}

HRESULT TextureSurface::QueryInterface(
	REFIID riid,
	LPVOID* obp
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;

	if (riid == IID_IDirect3DTexture)
	{
		str << " IDirect3DTexture";
	}
	else
	{
		str << " " << tostr_IID(riid);
	}

	LogText(str.str());
#endif

	if (riid == IID_IDirect3DTexture)
	{
		*obp = this->_d3dTexture;
		this->_d3dTexture->AddRef();

#if LOGGER
		str.str("");
		str << "\tDirect3DTexture\t" << *obp;
		LogText(str.str());
#endif

		return S_OK;
	}

#if LOGGER
	str.str("\tE_NOINTERFACE");
	LogText(str.str());
#endif

	return E_NOINTERFACE;
}

ULONG TextureSurface::AddRef()
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

ULONG TextureSurface::Release()
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
		//log_debug("[DBG] [DC] Releasing TextureSurface [%s], 0x%x", this->_name.c_str(), this);
		// This if() does not appear in Jeremy's code, it's just "delete this;"
		// However, I found out that for textures with no name, calling "delete this;" causes an
		// underflow (the texture is released even when it's refcount is already 0).
		// So, I don't understand why, but this avoids the underflow for now:
		if (this->_name.size() > 0)
			delete this;
		return 0;
	}

	return this->_refCount;
}

HRESULT TextureSurface::AddAttachedSurface(
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

HRESULT TextureSurface::AddOverlayDirtyRect(
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

HRESULT TextureSurface::Blt(
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
	LogText(str.str());
#endif

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT TextureSurface::BltBatch(
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

HRESULT TextureSurface::BltFast(
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

HRESULT TextureSurface::DeleteAttachedSurface(
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

HRESULT TextureSurface::EnumAttachedSurfaces(
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

HRESULT TextureSurface::EnumOverlayZOrders(
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

HRESULT TextureSurface::Flip(
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

HRESULT TextureSurface::GetAttachedSurface(
	LPDDSCAPS lpDDSCaps,
	LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpDDSCaps == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	if (lplpDDAttachedSurface == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	if (!this->_allocOnLoad && (lpDDSCaps->dwCaps & DDSCAPS_TEXTURE))
	{
		if (this->_mipmapCount <= 1)
		{
#if LOGGER
			str.str("\tDDERR_INVALIDOBJECT");
			LogText(str.str());
#endif

			return DDERR_INVALIDOBJECT;
		}

		*lplpDDAttachedSurface = this->_mipmap.Get();
		this->_mipmap->AddRef();

#if LOGGER
		str.str("");
		str << "\tMipmapSurface\t" << *lplpDDAttachedSurface;
		LogText(str.str());
#endif

		return DD_OK;
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT TextureSurface::GetBltStatus(
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

HRESULT TextureSurface::GetCaps(
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

HRESULT TextureSurface::GetClipper(
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

HRESULT TextureSurface::GetColorKey(
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

HRESULT TextureSurface::GetDC(
	HDC FAR *lphDC
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

HRESULT TextureSurface::GetFlipStatus(
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

HRESULT TextureSurface::GetOverlayPosition(
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

HRESULT TextureSurface::GetPalette(
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

HRESULT TextureSurface::GetPixelFormat(
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

HRESULT TextureSurface::GetSurfaceDesc(
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
	lpDDSurfaceDesc->dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_MIPMAPCOUNT;
	lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
	lpDDSurfaceDesc->ddpfPixelFormat = this->_pixelFormat;
	lpDDSurfaceDesc->dwHeight = this->_height;
	lpDDSurfaceDesc->dwWidth = this->_width;
	lpDDSurfaceDesc->lPitch = this->_width * this->_pixelFormat.dwRGBBitCount / 8;
	lpDDSurfaceDesc->dwMipMapCount = this->_mipmapCount;

#if LOGGER
	str.str("");
	str << "\t" << tostr_DDSURFACEDESC(lpDDSurfaceDesc);
	LogText(str.str());
#endif

	return DD_OK;
}

HRESULT TextureSurface::Initialize(
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

HRESULT TextureSurface::IsLost()
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

HRESULT TextureSurface::Lock(
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

	if (!this->_allocOnLoad || (this->_pixelFormat.dwRGBBitCount == 32 && this->_mipmapCount == 1))
	{
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
			if (this->_buffer == nullptr)
			{
				this->_bufferSize = this->_width * this->_height * this->_pixelFormat.dwRGBBitCount / 8 * 2;
				//this->_buffer = new char[this->_bufferSize];

				if (g_textureSurfaceBuffer == nullptr)
					g_textureSurfaceBuffer = new std::vector<char>();

				if (g_textureSurfaceBuffer->capacity() < this->_bufferSize)
				{
					g_textureSurfaceBuffer->reserve(this->_bufferSize);
				}

				this->_buffer = g_textureSurfaceBuffer->data();
			}

			*lpDDSurfaceDesc = {};
			lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
			lpDDSurfaceDesc->dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
			lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
			lpDDSurfaceDesc->ddpfPixelFormat = this->_pixelFormat;
			lpDDSurfaceDesc->dwHeight = this->_height;
			lpDDSurfaceDesc->dwWidth = this->_width;
			lpDDSurfaceDesc->lPitch = this->_width * this->_pixelFormat.dwRGBBitCount / 8;
			lpDDSurfaceDesc->lpSurface = this->_buffer;

			return DD_OK;
		}
	}

#if LOGGER
	str.str("\tDDERR_UNSUPPORTED");
	LogText(str.str());
#endif

	return DDERR_UNSUPPORTED;
}

HRESULT TextureSurface::ReleaseDC(
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

HRESULT TextureSurface::Restore()
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

HRESULT TextureSurface::SetClipper(
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

HRESULT TextureSurface::SetColorKey(
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

HRESULT TextureSurface::SetOverlayPosition(
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

HRESULT TextureSurface::SetPalette(
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

HRESULT TextureSurface::Unlock(
	LPVOID lpSurfaceData
	)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	// do not free memory for ModelIndex_418_16000_0_ResData_Fonts
	if (this->_pixelFormat.dwRGBBitCount == 32 && this->_mipmapCount == 1)
	{
		//log_debug("[DBG] [DC] TextureSurface Load");
		this->_d3dTexture->Load(this->_d3dTexture);

		if (this->_buffer != nullptr)
		{
			//delete[] this->_buffer;
			this->_buffer = nullptr;
			this->_bufferSize = 0;
			this->_width = 0;
			this->_height = 0;
		}
		// The texture is loaded, so the shared buffer isn't needed anymore
		ReleaseLargeTextureBuffer(g_textureSurfaceBuffer);
	}

	return DD_OK;
}

HRESULT TextureSurface::UpdateOverlay(
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

HRESULT TextureSurface::UpdateOverlayDisplay(
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

HRESULT TextureSurface::UpdateOverlayZOrder(
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
