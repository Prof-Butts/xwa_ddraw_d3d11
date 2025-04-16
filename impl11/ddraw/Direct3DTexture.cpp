// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes, 2019, 2020, 2021

#include "common.h"
#include "../shaders/material_defs.h"
#include "DeviceResources.h"
#include "Direct3DTexture.h"
#include "TextureSurface.h"
#include "MipmapSurface.h"
#include "bc7_main.h"
#include <comdef.h>
//#include <shlwapi.h>
#include "effects.h"

#include <ScreenGrab.h>
#include <WICTextureLoader.h>
#include <wincodec.h>
#include <vector>
#include <filesystem>
#include "globals.h"

namespace fs = std::filesystem;

#ifdef DBG_VR
/*
void DumpTexture(ID3D11DeviceContext *context, ID3D11Resource *texture, int index) {
	// DBG: Hack: Save the texture and its CRC
	wchar_t filename[80];
	swprintf_s(filename, 80, L"c:\\temp\\Load-img-%d.png", index);
	DirectX::SaveWICTextureToFile(context, texture, GUID_ContainerFormatPng, filename);
	log_debug("[DBG] Saved Texture: %d", index);
}
*/
#endif

char* convertFormat(char* src, DWORD width, DWORD height, DXGI_FORMAT format)
{
	int length = width * height;
	char* buffer = new char[length * 4];

	if (format == BACKBUFFER_FORMAT)
	{
		memcpy(buffer, src, length * 4);
	}
	else if (format == DXGI_FORMAT_B4G4R4A4_UNORM)
	{
		unsigned short* srcBuffer = (unsigned short*)src;
		unsigned int* destBuffer = (unsigned int*)buffer;

		for (int i = 0; i < length; ++i)
		{
			unsigned short color16 = srcBuffer[i];

			destBuffer[i] = convertColorB4G4R4A4toB8G8R8A8(color16);
		}
	}
	else if (format == DXGI_FORMAT_B5G5R5A1_UNORM)
	{
		unsigned short* srcBuffer = (unsigned short*)src;
		unsigned int* destBuffer = (unsigned int*)buffer;

		for (int i = 0; i < length; ++i)
		{
			unsigned short color16 = srcBuffer[i];

			destBuffer[i] = convertColorB5G5R5A1toB8G8R8A8(color16);
		}
	}
	else if (format == DXGI_FORMAT_B5G6R5_UNORM)
	{
		unsigned short* srcBuffer = (unsigned short*)src;
		unsigned int* destBuffer = (unsigned int*)buffer;

		for (int i = 0; i < length; ++i)
		{
			unsigned short color16 = srcBuffer[i];

			destBuffer[i] = convertColorB5G6R5toB8G8R8A8(color16);
		}
	}
	else
	{
		memset(buffer, 0, length * 4);
	}

	return buffer;
}

Direct3DTexture::Direct3DTexture(DeviceResources* deviceResources, TextureSurface* surface)
{
	this->_refCount = 1;
	this->_deviceResources = deviceResources;
	this->_surface = surface;
	this->_textureData._name = surface->_name;
	this->_textureData._width = surface->_width;
	this->_textureData._height = surface->_height;
	this->_textureData._deviceResources = deviceResources;
}

Direct3DTexture::~Direct3DTexture()
{
	// I added this to fix some release underflows; but it appears to be causing some
	// textures to be released when they are still being used.
	// See commit c7226e59 for more details.
	//*this->_textureView.GetAddressOf() = nullptr;
}

HRESULT Direct3DTexture::QueryInterface(
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

ULONG Direct3DTexture::AddRef()
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

ULONG Direct3DTexture::Release()
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

HRESULT Direct3DTexture::Initialize(
	LPDIRECT3DDEVICE lpD3DDevice,
	LPDIRECTDRAWSURFACE lpDDSurface)
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

HRESULT Direct3DTexture::GetHandle(
	LPDIRECT3DDEVICE lpDirect3DDevice,
	LPD3DTEXTUREHANDLE lpHandle)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	LogText(str.str());
#endif

	if (lpHandle == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	*lpHandle = (D3DTEXTUREHANDLE)this;

	return D3D_OK;
}

HRESULT Direct3DTexture::PaletteChanged(
	DWORD dwStart,
	DWORD dwCount)
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

HRESULT Direct3DTexture::Load(
	LPDIRECT3DTEXTURE lpD3DTexture)
{
#if LOGGER
	std::ostringstream str;
	str << this << " " << __FUNCTION__;
	str << " " << lpD3DTexture;
	LogText(str.str());
#endif

	if (lpD3DTexture == nullptr)
	{
#if LOGGER
		str.str("\tDDERR_INVALIDPARAMS");
		LogText(str.str());
#endif

		return DDERR_INVALIDPARAMS;
	}

	Direct3DTexture* d3dTexture = (Direct3DTexture*)lpD3DTexture;
	TextureSurface* surface = d3dTexture->_surface;

	//log_debug("[DBG] Loading %s", surface->name);
	// The changes from Jeremy's commit fe50cc59e03225bb7e39ae2852e87d305e7c7891 to reduce
	// memory usage cause mipmapped textures to call Load() again. So we must copy all the
	// settings from the input texture to this level.
	d3dTexture->_textureData.Clone(&this->_textureData);

	// g_AuxTextureVector will keep a list of references to textures that have associated materials.
	// We use this list to reload materials by pressing Ctrl+Alt+L.
	// If d3dTexture has already been inserted in g_AuxTextureVector, then we need to update the 
	// reference in g_AuxTextureVector so that it points to *this* object now:
	if (this->_textureData.AuxVectorIndex != -1 && this->_textureData.AuxVectorIndex < (int)g_AuxTextureVector.size())
		g_AuxTextureVector[this->_textureData.AuxVectorIndex] = &this->_textureData;

	// TODO: Instead of copying textures, let's have a single pointer shared by all instances
	// Actually, it looks like we need to copy the texture names in order to have them available
	// during 3D rendering. This makes them available both in the hangar and after launching from
	// the hangar.
	//log_debug("[DBG] [DC] Load Copying name: [%s]", d3dTexture->_surface->_name);

	//strncpy_s(this->_surface->_cname, d3dTexture->_surface->_cname, MAX_TEXTURE_NAME);

	// DEBUG
	// Looks like we always tag the color texture before the light texture.
	// Then we Load() the color and light textures.
	//if (this->is_CockpitTex) {
		//	log_debug("[DBG] [AC] Loading: %s", surface->_name);
	//}

	// DEBUG

	if (d3dTexture->_textureData._textureView)
	{
#if LOGGER
		str.str("\tretrieve existing texture");
		LogText(str.str());
#endif

		d3dTexture->_textureData._textureView->AddRef();
		this->_textureData._textureView = d3dTexture->_textureData._textureView.Get();

		return D3D_OK;
	}

#if LOGGER
	str.str("\tcreate new texture");
	LogText(str.str());
#endif

#if LOGGER
	str.str("");
	str << "\t" << surface->_pixelFormat.dwRGBBitCount;
	str << " " << surface->_width << "x" << surface->_height;
	str << " " << (void*)surface->_pixelFormat.dwRBitMask;
	str << " " << (void*)surface->_pixelFormat.dwGBitMask;
	str << " " << (void*)surface->_pixelFormat.dwBBitMask;
	str << " " << (void*)surface->_pixelFormat.dwRGBAlphaBitMask;
	LogText(str.str());
#endif

	if (surface->_pixelFormat.dwRGBBitCount == 32 && surface->_width != 0 && surface->_height != 0 && surface->_pixelFormat.dwSize != 32 && surface->_pixelFormat.dwSize != 0 && surface->_pixelFormat.dwSize < surface->_width * surface->_height * 4)
	{
		int size = surface->_pixelFormat.dwSize;
		int width = surface->_width;
		int height = surface->_height;

		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Format = DXGI_FORMAT_BC7_UNORM;
		textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		int blocksWidth = (width + 3) / 4;
		int blocksHeight = (height + 3) / 4;

		D3D11_SUBRESOURCE_DATA textureData{};
		textureData.pSysMem = surface->_buffer;
		textureData.SysMemPitch = blocksWidth * 16;
		textureData.SysMemSlicePitch = 0;

		ComPtr<ID3D11Texture2D> texture;

		char* data = nullptr;

		if (width % 4 != 0 || height % 4 != 0)
		{
			data = new char[width * height * 4];
			BC7_Decode(surface->_buffer, data, width, height);

			textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			textureData.pSysMem = data;
			textureData.SysMemPitch = width * 4;
		}

		HRESULT hr = this->_deviceResources->_d3dDevice->CreateTexture2D(&textureDesc, &textureData, &texture);

		if (data)
		{
			delete[] data;
		}

		if (FAILED(hr))
		{
			static bool messageShown = false;

			if (!messageShown)
			{
				MessageBox(nullptr, _com_error(hr).ErrorMessage(), __FUNCTION__, MB_ICONERROR);
			}

			messageShown = true;

			return D3DERR_TEXTURE_LOAD_FAILED;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc{};
		textureViewDesc.Format = textureDesc.Format;
		textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		textureViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
		textureViewDesc.Texture2D.MostDetailedMip = 0;

		if (FAILED(this->_deviceResources->_d3dDevice->CreateShaderResourceView(texture, &textureViewDesc, &d3dTexture->_textureData._textureView)))
		{
			return D3DERR_TEXTURE_LOAD_FAILED;
		}

		d3dTexture->_textureData._textureView->AddRef();
		this->_textureData._textureView = d3dTexture->_textureData._textureView.Get();

		return D3D_OK;
	}

	DWORD bpp = surface->_pixelFormat.dwRGBBitCount == 32 ? 4 : 2;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	if (bpp == 4)
	{
		format = BACKBUFFER_FORMAT;
	}
	else
	{
		DWORD alpha = surface->_pixelFormat.dwRGBAlphaBitMask;

		if (alpha == 0x0000F000)
		{
			format = DXGI_FORMAT_B4G4R4A4_UNORM;
		}
		else if (alpha == 0x00008000)
		{
			format = DXGI_FORMAT_B5G5R5A1_UNORM;
		}
		else
		{
			format = DXGI_FORMAT_B5G6R5_UNORM;
		}
	}

	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = surface->_width;
	textureDesc.Height = surface->_height;
	textureDesc.Format = (this->_deviceResources->_are16BppTexturesSupported || format == BACKBUFFER_FORMAT) ? format : BACKBUFFER_FORMAT;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	textureDesc.MipLevels = surface->_mipmapCount;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA* textureData = new D3D11_SUBRESOURCE_DATA[textureDesc.MipLevels];

	bool useBuffers = !this->_deviceResources->_are16BppTexturesSupported && format != BACKBUFFER_FORMAT;
	char** buffers = nullptr;

	if (useBuffers)
	{
		buffers = new char* [textureDesc.MipLevels];
		buffers[0] = convertFormat(surface->_buffer, surface->_width, surface->_height, format);
	}

	textureData[0].pSysMem = useBuffers ? buffers[0] : surface->_buffer;
	textureData[0].SysMemPitch = surface->_width * (useBuffers ? 4 : bpp);
	textureData[0].SysMemSlicePitch = 0;

	MipmapSurface* mipmap = surface->_mipmap;
	for (DWORD i = 1; i < textureDesc.MipLevels; i++)
	{
		if (useBuffers)
		{
			buffers[i] = convertFormat(mipmap->_buffer, mipmap->_width, mipmap->_height, format);
		}

		textureData[i].pSysMem = useBuffers ? buffers[i] : mipmap->_buffer;
		textureData[i].SysMemPitch = mipmap->_width * (useBuffers ? 4 : bpp);
		textureData[i].SysMemSlicePitch = 0;

		mipmap = mipmap->_mipmap;
	}

	// This is where the various textures are created from data already loaded into RAM
	ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = this->_deviceResources->_d3dDevice->CreateTexture2D(&textureDesc, textureData, &texture);
	if (FAILED(hr)) {
		log_debug("[DBG] Failed when calling CreateTexture2D, reason: 0x%x",
			this->_deviceResources->_d3dDevice->GetDeviceRemovedReason());
		goto out;
	}

	this->_textureData.TagTexture();

out:
	if (useBuffers)
	{
		for (DWORD i = 0; i < textureDesc.MipLevels; i++)
		{
			delete[] buffers[i];
		}

		delete[] buffers;
	}

	delete[] textureData;

	if (FAILED(hr))
	{
		static bool messageShown = false;

		if (!messageShown)
		{
			MessageBox(nullptr, _com_error(hr).ErrorMessage(), __FUNCTION__, MB_ICONERROR);
		}

		messageShown = true;

		return D3DERR_TEXTURE_LOAD_FAILED;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc{};
	textureViewDesc.Format = textureDesc.Format;
	textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	textureViewDesc.Texture2D.MostDetailedMip = 0;

	if (FAILED(this->_deviceResources->_d3dDevice->CreateShaderResourceView(texture, &textureViewDesc, &d3dTexture->_textureData._textureView)))
	{
#if LOGGER
		str.str("\tD3DERR_TEXTURE_LOAD_FAILED");
		LogText(str.str());
#endif

		return D3DERR_TEXTURE_LOAD_FAILED;
	}

	d3dTexture->_textureData._textureView->AddRef();
	this->_textureData._textureView = d3dTexture->_textureData._textureView.Get();

	return D3D_OK;
}

HRESULT Direct3DTexture::Unload()
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
