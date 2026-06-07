// Copyright (c) 2026 Peter Soetens
// Licensed under the MIT license. See LICENSE.txt
//
// WineD2DEffectShim implementation -- see WineD2DEffectShim.h for the design.
#include "common.h"
#include "DeviceResources.h"
#include "utils.h"
#include "SharedMem.h"
#include "WineD2DEffectShim.h"
#include <d2d1_1.h>
#include <initguid.h>

// hook_concourse's effect CLSID (BitmapEffect.h in xwa_hook_concourse)
DEFINE_GUID(CLSID_HookBitmapEffect, 0xB7B36C92, 0x3498, 0x4A94, 0x9E, 0x95, 0x9F, 0x24, 0x6F, 0x92, 0x45, 0xBF);
// Private marker IIDs to recognize our own objects through QueryInterface
DEFINE_GUID(IID_ShimEffectMarker, 0x7E1A1F2B, 0x4C3D, 0x4E5F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B);
DEFINE_GUID(IID_ShimBitmapMarker, 0xA2B3C4D5, 0xE6F7, 0x4081, 0x92, 0xA3, 0xB4, 0xC5, 0xD6, 0xE7, 0xF8, 0x09);

#include "../Release/ShimBitmapVS.h"
#include "../Release/ShimBitmapPS.h"

// ===========================================================================
// ShimBitmap: an ID2D1Bitmap backed by a plain D3D11 texture+SRV.
// Only the surface hook_concourse uses is functional (CopyFromMemory,
// GetSize/GetPixelSize); the rest are honest stubs.
// ===========================================================================
class ShimBitmap : public ID2D1Bitmap
{
public:
	LONG _ref;
	mutable ComPtr<ID3D11Texture2D> _tex;
	mutable ComPtr<ID3D11ShaderResourceView> _srv;
	mutable ComPtr<ID2D1Factory> _factory;
	UINT _width, _height;
	DXGI_FORMAT _format;

	// NB: the project ComPtr has NO assignment operators — `member = ptr;`
	// routes through a converting temporary whose dtor Release()s the pointer
	// (use-after-free). Always initializer-list + explicit AddRef, or
	// .Release() + *GetAddressOf() for transferring an owned reference.
	ShimBitmap(ID2D1Factory* factory) : _ref(1), _factory(factory), _width(0), _height(0), _format(DXGI_FORMAT_UNKNOWN)
	{
		if (factory) factory->AddRef();
	}

	static HRESULT Create(DeviceResources* res, ID2D1Factory* factory,
		D2D1_SIZE_U size, const void* srcData, UINT32 pitch,
		const D2D1_BITMAP_PROPERTIES* props, ShimBitmap** out)
	{
		*out = nullptr;
		DXGI_FORMAT fmt = props ? props->pixelFormat.format : DXGI_FORMAT_B8G8R8A8_UNORM;
		if (fmt == DXGI_FORMAT_UNKNOWN) fmt = DXGI_FORMAT_B8G8R8A8_UNORM;

		ShimBitmap* bmp = new ShimBitmap(factory);
		bmp->_width = size.width;
		bmp->_height = size.height;
		bmp->_format = fmt;

		D3D11_TEXTURE2D_DESC td = {};
		td.Width = size.width;
		td.Height = size.height;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = fmt;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA init = {};
		init.pSysMem = srcData;
		init.SysMemPitch = pitch;

		HRESULT hr = res->_d3dDevice->CreateTexture2D(&td, srcData ? &init : nullptr, &bmp->_tex);
		if (SUCCEEDED(hr))
			hr = res->_d3dDevice->CreateShaderResourceView(bmp->_tex, nullptr, &bmp->_srv);
		if (FAILED(hr))
		{
			log_debug("[DBG] [SHIM] ShimBitmap::Create %ux%u fmt=%d FAILED 0x%08X", size.width, size.height, fmt, hr);
			bmp->Release();
			return hr;
		}
		*out = bmp;
		return S_OK;
	}

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
	{
		if (!ppv) return E_POINTER;
		if (riid == __uuidof(IUnknown) || riid == __uuidof(ID2D1Resource) ||
			riid == __uuidof(ID2D1Image) || riid == __uuidof(ID2D1Bitmap) ||
			riid == IID_ShimBitmapMarker)
		{
			*ppv = this; AddRef(); return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() override { return (ULONG)InterlockedIncrement(&_ref); }
	STDMETHOD_(ULONG, Release)() override
	{
		ULONG r = (ULONG)InterlockedDecrement(&_ref);
		if (r == 0) delete this;
		return r;
	}

	// ID2D1Resource
	STDMETHOD_(void, GetFactory)(ID2D1Factory** factory) const override
	{
		*factory = _factory; if (_factory) ((ID2D1Factory*)_factory)->AddRef();
	}

	// ID2D1Bitmap
	STDMETHOD_(D2D1_SIZE_F, GetSize)() const override
	{
		return D2D1::SizeF((FLOAT)_width, (FLOAT)_height);
	}
	STDMETHOD_(D2D1_SIZE_U, GetPixelSize)() const override
	{
		return D2D1::SizeU(_width, _height);
	}
	STDMETHOD_(D2D1_PIXEL_FORMAT, GetPixelFormat)() const override
	{
		return D2D1::PixelFormat(_format, D2D1_ALPHA_MODE_PREMULTIPLIED);
	}
	STDMETHOD_(void, GetDpi)(FLOAT* dpiX, FLOAT* dpiY) const override
	{
		if (dpiX) *dpiX = 96.0f; if (dpiY) *dpiY = 96.0f;
	}
	STDMETHOD(CopyFromBitmap)(CONST D2D1_POINT_2U*, ID2D1Bitmap*, CONST D2D1_RECT_U*) override { return E_NOTIMPL; }
	STDMETHOD(CopyFromRenderTarget)(CONST D2D1_POINT_2U*, ID2D1RenderTarget*, CONST D2D1_RECT_U*) override { return E_NOTIMPL; }
	STDMETHOD(CopyFromMemory)(CONST D2D1_RECT_U* dstRect, CONST void* srcData, UINT32 pitch) override
	{
		if (!_tex || !srcData) return E_FAIL;
		ID3D11DeviceContext* ctx = nullptr;
		ID3D11Device* dev = nullptr;
		_tex->GetDevice(&dev);
		dev->GetImmediateContext(&ctx);
		if (dstRect)
		{
			D3D11_BOX box = { dstRect->left, dstRect->top, 0, dstRect->right, dstRect->bottom, 1 };
			ctx->UpdateSubresource(_tex, 0, &box, srcData, pitch, 0);
		}
		else
		{
			ctx->UpdateSubresource(_tex, 0, nullptr, srcData, pitch, 0);
		}
		ctx->Release();
		dev->Release();
		return S_OK;
	}
};

// ===========================================================================
// ShimEffect: stands in for the hook's BitmapEffect when the real d2d
// CreateEffect fails. Stores the input bitmap and the BlendColor value.
// GetOutput() returns its own ID2D1Image identity; the shim context
// recognizes it in DrawImage via IID_ShimEffectMarker.
// ===========================================================================
class ShimEffect : public ID2D1Effect, public ID2D1Image
{
public:
	LONG _ref;
	mutable ComPtr<ShimBitmap> _input;
	mutable ComPtr<ID2D1Factory> _factory;
	UINT32 _blendColor;

	ShimEffect(ID2D1Factory* factory) : _ref(1), _factory(factory), _blendColor(0)
	{
		if (factory) factory->AddRef();
	}

	// IUnknown (shared by both branches; disambiguate via ID2D1Effect side)
	STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
	{
		if (!ppv) return E_POINTER;
		if (riid == __uuidof(IUnknown) || riid == __uuidof(ID2D1Properties) || riid == __uuidof(ID2D1Effect))
		{
			*ppv = static_cast<ID2D1Effect*>(this); AddRef(); return S_OK;
		}
		if (riid == IID_ShimEffectMarker)
		{
			// private marker: must return the OBJECT BASE (callers cast to
			// ShimEffect*), NOT the ID2D1Image branch (this+vtable offset)!
			*ppv = this; AddRef(); return S_OK;
		}
		if (riid == __uuidof(ID2D1Resource) || riid == __uuidof(ID2D1Image))
		{
			*ppv = static_cast<ID2D1Image*>(this); AddRef(); return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)() override { return (ULONG)InterlockedIncrement(&_ref); }
	STDMETHOD_(ULONG, Release)() override
	{
		ULONG r = (ULONG)InterlockedDecrement(&_ref);
		if (r == 0) delete this;
		return r;
	}

	// ID2D1Resource (ID2D1Image branch)
	STDMETHOD_(void, GetFactory)(ID2D1Factory** factory) const override
	{
		*factory = _factory; if (_factory) ((ID2D1Factory*)_factory)->AddRef();
	}

	// ID2D1Properties -- only BlendColor (index 0) is real
	STDMETHOD_(UINT32, GetPropertyCount)() const override { return 1; }
	STDMETHOD(GetPropertyName)(UINT32 index, PWSTR name, UINT32 nameCount) const override
	{
		if (index != 0) return D2DERR_INVALID_PROPERTY;
		wcsncpy_s(name, nameCount, L"BlendColor", _TRUNCATE);
		return S_OK;
	}
	STDMETHOD_(UINT32, GetPropertyNameLength)(UINT32 index) const override { return index == 0 ? 10 : 0; }
	STDMETHOD_(D2D1_PROPERTY_TYPE, GetType)(UINT32 index) const override
	{
		return index == 0 ? D2D1_PROPERTY_TYPE_UINT32 : D2D1_PROPERTY_TYPE_UNKNOWN;
	}
	STDMETHOD_(UINT32, GetPropertyIndex)(PCWSTR name) const override
	{
		return (name && _wcsicmp(name, L"BlendColor") == 0) ? 0 : D2D1_INVALID_PROPERTY_INDEX;
	}
	STDMETHOD(SetValueByName)(PCWSTR name, D2D1_PROPERTY_TYPE type, CONST BYTE* data, UINT32 dataSize) override
	{
		return SetValue(GetPropertyIndex(name), type, data, dataSize);
	}
	STDMETHOD(SetValue)(UINT32 index, D2D1_PROPERTY_TYPE type, CONST BYTE* data, UINT32 dataSize) override
	{
		if (index != 0 || !data || dataSize != sizeof(UINT32)) return D2DERR_INVALID_PROPERTY;
		memcpy(&_blendColor, data, sizeof(UINT32));
		return S_OK;
	}
	STDMETHOD(GetValueByName)(PCWSTR name, D2D1_PROPERTY_TYPE type, BYTE* data, UINT32 dataSize) const override
	{
		return GetValue(GetPropertyIndex(name), type, data, dataSize);
	}
	STDMETHOD(GetValue)(UINT32 index, D2D1_PROPERTY_TYPE type, BYTE* data, UINT32 dataSize) const override
	{
		if (index != 0 || !data || dataSize < sizeof(UINT32)) return D2DERR_INVALID_PROPERTY;
		memcpy(data, &_blendColor, sizeof(UINT32));
		return S_OK;
	}
	STDMETHOD_(UINT32, GetValueSize)(UINT32 index) const override { return index == 0 ? sizeof(UINT32) : 0; }
	STDMETHOD(GetSubProperties)(UINT32, ID2D1Properties**) const override { return D2DERR_NO_SUBPROPERTIES; }

	// ID2D1Effect
	STDMETHOD_(void, SetInput)(UINT32 index, ID2D1Image* input, BOOL) override
	{
		if (index != 0) return;
		_input.Release();
		if (input)
		{
			ShimBitmap* bmp = nullptr;
			HRESULT qi = input->QueryInterface(IID_ShimBitmapMarker, (void**)&bmp);
			if (SUCCEEDED(qi))
				*_input.GetAddressOf() = bmp; // store the QI reference directly (no ComPtr temp!)
#if LOGGER
			static int s_siN = 0;
			const int n = s_siN++;
			if ((n < 30) || (n % 256) == 0)
				log_debug("[DBG] [SHIM] SetInput #%d: in=%p qi=0x%08X -> %s", n, input,
					qi, SUCCEEDED(qi) ? "shim bitmap" : "NON-SHIM (ignored)");
#endif
		}
	}
	STDMETHOD(SetInputCount)(UINT32) override { return S_OK; }
	STDMETHOD_(void, GetInput)(UINT32 index, ID2D1Image** input) const override
	{
		*input = nullptr;
		if (index == 0 && _input) { *input = (ShimBitmap*)_input; (*input)->AddRef(); }
	}
	STDMETHOD_(UINT32, GetInputCount)() const override { return 1; }
	STDMETHOD_(void, GetOutput)(ID2D1Image** output) const override
	{
		*output = (ID2D1Image*)this;
		(*output)->AddRef();
	}
};

// ===========================================================================
// ShimContext: ID2D1DeviceContext proxy. Forwards everything to the real
// device context; intercepts CreateEffect / CreateBitmap / DrawImage /
// clip push-pop (for the scissor rect of the native draw).
// ===========================================================================
#define FWD _real

class ShimContext : public ID2D1DeviceContext
{
public:
	LONG _ref;
	DeviceResources* _res;
	mutable ComPtr<ID2D1RenderTarget> _realRT;
	mutable ComPtr<ID2D1DeviceContext> _real;
	// STICKY + GLOBAL: the render target (and thus this proxy) is recreated
	// when missions resize buffers, but hook_concourse caches its effect
	// object forever and never calls CreateEffect again -- a fresh instance
	// flag would silently revert to the (broken) real-d2d bitmap path and
	// icons stop drawing after the first mission.
	static bool s_shimMode;

	struct ClipEntry { D2D1_RECT_F rect; D2D1_MATRIX_3X2_F xform; };
	ClipEntry _clips[16];
	int _clipCount;

	// native draw pipeline (lazy)
	bool _pipeTried, _pipeOk;
	ComPtr<ID3D11VertexShader> _vs;
	ComPtr<ID3D11PixelShader> _ps;
	ComPtr<ID3D11Buffer> _cb;
	ComPtr<ID3D11BlendState> _blend;
	ComPtr<ID3D11RasterizerState> _raster;
	ComPtr<ID3D11DepthStencilState> _depth;
	ComPtr<ID3D11SamplerState> _sampLinear;
	ComPtr<ID3D11SamplerState> _sampPoint;

	struct ShimCB
	{
		float dstRect[4];
		float uvRect[4];
		float xformRow0[4];
		float xformRow1[4];
		float blendColor[4];
	};

	ShimContext(DeviceResources* res, ID2D1RenderTarget* realRT, ID2D1DeviceContext* realDC)
		: _ref(1), _res(res), _realRT(realRT), _real(realDC),
		_clipCount(0), _pipeTried(false), _pipeOk(false)
	{
		realRT->AddRef();
		realDC->AddRef();
	}

	// ---------------- IUnknown ----------------
	STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
	{
		if (!ppv) return E_POINTER;
		if (riid == __uuidof(IUnknown) || riid == __uuidof(ID2D1Resource) ||
			riid == __uuidof(ID2D1RenderTarget) || riid == __uuidof(ID2D1DeviceContext))
		{
			*ppv = this; AddRef(); return S_OK;
		}
		// Unknown interfaces fall through to the real object (e.g. future
		// ID2D1DeviceContext1+ requests) -- the caller then talks to the
		// real context directly, losing only the shim interceptions.
		return _real->QueryInterface(riid, ppv);
	}
	STDMETHOD_(ULONG, AddRef)() override { return (ULONG)InterlockedIncrement(&_ref); }
	STDMETHOD_(ULONG, Release)() override
	{
		ULONG r = (ULONG)InterlockedDecrement(&_ref);
		if (r == 0) delete this;
		return r;
	}

	// ---------------- ID2D1Resource ----------------
	STDMETHOD_(void, GetFactory)(ID2D1Factory** factory) const override { FWD->GetFactory(factory); }

	// ---------------- ID2D1RenderTarget: interceptions ----------------
	STDMETHOD(CreateBitmap)(D2D1_SIZE_U size, CONST void* srcData, UINT32 pitch,
		CONST D2D1_BITMAP_PROPERTIES* props, ID2D1Bitmap** bitmap) override
	{
		// NB: hook_concourse ignores this HRESULT and caches the out-pointer
		// UNINITIALIZED on failure -- null it and never let the call fail
		// without trying the shim path too.
		if (bitmap) *bitmap = nullptr;
		HRESULT hr = E_FAIL;
		if (!s_shimMode)
		{
			hr = FWD->CreateBitmap(size, srcData, pitch, props, bitmap);
			if (SUCCEEDED(hr))
				return hr;
		}
		ComPtr<ID2D1Factory> factory;
		FWD->GetFactory(&factory);
		HRESULT hr2 = ShimBitmap::Create(_res, factory, size, srcData, pitch, props, (ShimBitmap**)bitmap);
#if LOGGER
		static int s_cbN = 0;
		const int n = s_cbN++;
		if ((n < 30) || (n % 256) == 0)
			log_debug("[DBG] [SHIM] CreateBitmap #%d: %ux%u fmt=%d pitch=%u shimMode=%d realHr=0x%08X shimHr=0x%08X out=%p",
				n, size.width, size.height, props ? (int)props->pixelFormat.format : -1, pitch,
				(int)s_shimMode, hr, hr2, bitmap ? *bitmap : nullptr);
#endif
		return hr2;
	}
	STDMETHOD_(void, PushAxisAlignedClip)(CONST D2D1_RECT_F* rect, D2D1_ANTIALIAS_MODE mode) override
	{
		if (_clipCount < 16 && rect)
		{
			_clips[_clipCount].rect = *rect;
			FWD->GetTransform(&_clips[_clipCount].xform);
			_clipCount++;
		}
		FWD->PushAxisAlignedClip(rect, mode);
	}
	STDMETHOD_(void, PopAxisAlignedClip)() override
	{
		if (_clipCount > 0) _clipCount--;
		FWD->PopAxisAlignedClip();
	}

	// ---------------- ID2D1DeviceContext: interceptions ----------------
	STDMETHOD(CreateEffect)(REFCLSID effectId, ID2D1Effect** effect) override
	{
		HRESULT hr = FWD->CreateEffect(effectId, effect);
		if (SUCCEEDED(hr))
			return hr;
		if (effectId == CLSID_HookBitmapEffect)
		{
			static bool logged = false;
			if (!logged)
			{
				log_debug("[DBG] [SHIM] real CreateEffect(BitmapEffect) failed 0x%08X -> using D3D11 shim effect", hr);
				logged = true;
			}
			s_shimMode = true;
			ComPtr<ID2D1Factory> factory;
			FWD->GetFactory(&factory);
			*effect = new ShimEffect(factory);
			return S_OK;
		}
		return hr;
	}
	STDMETHOD_(void, DrawImage)(ID2D1Image* image, CONST D2D1_POINT_2F* targetOffset,
		CONST D2D1_RECT_F* imageRect, D2D1_INTERPOLATION_MODE interp, D2D1_COMPOSITE_MODE composite) override
	{
		ShimEffect* eff = nullptr;
		if (image && SUCCEEDED(image->QueryInterface(IID_ShimEffectMarker, (void**)&eff)))
		{
			ShimDraw(eff, targetOffset, imageRect, interp);
			eff->Release();
			return;
		}
		FWD->DrawImage(image, targetOffset, imageRect, interp, composite);
	}

	// ---------------- the native D3D11 draw ----------------
	bool EnsurePipeline()
	{
		if (_pipeTried) return _pipeOk;
		_pipeTried = true;

		ID3D11Device* dev = _res->_d3dDevice;
		HRESULT hr = dev->CreateVertexShader(g_ShimBitmapVS, sizeof(g_ShimBitmapVS), nullptr, &_vs);
		if (SUCCEEDED(hr)) hr = dev->CreatePixelShader(g_ShimBitmapPS, sizeof(g_ShimBitmapPS), nullptr, &_ps);

		if (SUCCEEDED(hr))
		{
			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = sizeof(ShimCB);
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			hr = dev->CreateBuffer(&bd, nullptr, &_cb);
		}
		if (SUCCEEDED(hr))
		{
			// premultiplied-alpha SOURCE_OVER
			D3D11_BLEND_DESC bd = {};
			bd.RenderTarget[0].BlendEnable = TRUE;
			bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			hr = dev->CreateBlendState(&bd, &_blend);
		}
		if (SUCCEEDED(hr))
		{
			D3D11_RASTERIZER_DESC rd = {};
			rd.FillMode = D3D11_FILL_SOLID;
			rd.CullMode = D3D11_CULL_NONE;
			rd.ScissorEnable = TRUE;
			rd.DepthClipEnable = TRUE;
			hr = dev->CreateRasterizerState(&rd, &_raster);
		}
		if (SUCCEEDED(hr))
		{
			D3D11_DEPTH_STENCIL_DESC dd = {};
			dd.DepthEnable = FALSE;
			dd.StencilEnable = FALSE;
			hr = dev->CreateDepthStencilState(&dd, &_depth);
		}
		if (SUCCEEDED(hr))
		{
			D3D11_SAMPLER_DESC sd = {};
			sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.MaxLOD = D3D11_FLOAT32_MAX;
			hr = dev->CreateSamplerState(&sd, &_sampLinear);
			if (SUCCEEDED(hr))
			{
				sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
				hr = dev->CreateSamplerState(&sd, &_sampPoint);
			}
		}

		_pipeOk = SUCCEEDED(hr);
		log_debug("[DBG] [SHIM] pipeline init: %s (0x%08X)", _pipeOk ? "OK" : "FAILED", hr);
		return _pipeOk;
	}

	void ShimDraw(ShimEffect* eff, CONST D2D1_POINT_2F* targetOffset,
		CONST D2D1_RECT_F* imageRect, D2D1_INTERPOLATION_MODE interp)
	{
		if (!eff->_input) return;
		if (!EnsurePipeline()) return;

		ShimBitmap* bmp = (ShimBitmap*)eff->_input;
		D2D1_SIZE_U targetPx = _realRT->GetPixelSize();
		if (targetPx.width == 0 || targetPx.height == 0) return;

		D2D1_RECT_F src = imageRect ? *imageRect
			: D2D1::RectF(0, 0, (FLOAT)bmp->_width, (FLOAT)bmp->_height);
		D2D1_POINT_2F off = targetOffset ? *targetOffset : D2D1::Point2F(0, 0);

		D2D1_MATRIX_3X2_F m;
		FWD->GetTransform(&m);

		ShimCB cb;
		cb.dstRect[0] = off.x;
		cb.dstRect[1] = off.y;
		cb.dstRect[2] = off.x + (src.right - src.left);
		cb.dstRect[3] = off.y + (src.bottom - src.top);
		cb.uvRect[0] = src.left / (FLOAT)bmp->_width;
		cb.uvRect[1] = src.top / (FLOAT)bmp->_height;
		cb.uvRect[2] = src.right / (FLOAT)bmp->_width;
		cb.uvRect[3] = src.bottom / (FLOAT)bmp->_height;
		cb.xformRow0[0] = m._11; cb.xformRow0[1] = m._12;
		cb.xformRow0[2] = m._21; cb.xformRow0[3] = m._22;
		cb.xformRow1[0] = m._31; cb.xformRow1[1] = m._32;
		cb.xformRow1[2] = (FLOAT)targetPx.width;
		cb.xformRow1[3] = (FLOAT)targetPx.height;
		// BitmapEffect::SetBlendColor packing: LSB-first bytes / 255
		cb.blendColor[0] = (float)((eff->_blendColor) & 0xFF) / 255.0f;
		cb.blendColor[1] = (float)((eff->_blendColor >> 8) & 0xFF) / 255.0f;
		cb.blendColor[2] = (float)((eff->_blendColor >> 16) & 0xFF) / 255.0f;
		cb.blendColor[3] = (float)((eff->_blendColor >> 24) & 0xFF) / 255.0f;

		// scissor = intersection of the tracked clip stack (transformed AABBs)
		LONG sx0 = 0, sy0 = 0, sx1 = (LONG)targetPx.width, sy1 = (LONG)targetPx.height;
		for (int i = 0; i < _clipCount; i++)
		{
			const D2D1_RECT_F& r = _clips[i].rect;
			const D2D1_MATRIX_3X2_F& cm = _clips[i].xform;
			D2D1_POINT_2F pts[4] = {
				{ r.left, r.top }, { r.right, r.top }, { r.left, r.bottom }, { r.right, r.bottom } };
			FLOAT minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
			for (int k = 0; k < 4; k++)
			{
				FLOAT tx = pts[k].x * cm._11 + pts[k].y * cm._21 + cm._31;
				FLOAT ty = pts[k].x * cm._12 + pts[k].y * cm._22 + cm._32;
				if (tx < minX) minX = tx; if (tx > maxX) maxX = tx;
				if (ty < minY) minY = ty; if (ty > maxY) maxY = ty;
			}
			if ((LONG)minX > sx0) sx0 = (LONG)minX;
			if ((LONG)minY > sy0) sy0 = (LONG)minY;
			if ((LONG)maxX < sx1) sx1 = (LONG)maxX;
			if ((LONG)maxY < sy1) sy1 = (LONG)maxY;
		}
		if (sx0 >= sx1 || sy0 >= sy1) return; // fully clipped

		// order the pending d2d commands before our native draw
		_realRT->Flush(nullptr, nullptr);

		ID3D11DeviceContext* ctx = _res->_d3dDeviceContext;
		ID3D11RenderTargetView* rtv = _res->_renderTargetView;
		if (!ctx || !rtv) return;

		// ---- save the state we touch ----
		ID3D11RenderTargetView* oldRTV = nullptr; ID3D11DepthStencilView* oldDSV = nullptr;
		ctx->OMGetRenderTargets(1, &oldRTV, &oldDSV);
		D3D11_VIEWPORT oldVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		UINT oldVPCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		ctx->RSGetViewports(&oldVPCount, oldVP);
		D3D11_RECT oldSc[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		UINT oldScCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		ctx->RSGetScissorRects(&oldScCount, oldSc);
		ID3D11BlendState* oldBlend = nullptr; FLOAT oldBf[4]; UINT oldMask = 0;
		ctx->OMGetBlendState(&oldBlend, oldBf, &oldMask);
		ID3D11DepthStencilState* oldDepth = nullptr; UINT oldSref = 0;
		ctx->OMGetDepthStencilState(&oldDepth, &oldSref);
		ID3D11RasterizerState* oldRaster = nullptr;
		ctx->RSGetState(&oldRaster);
		ID3D11VertexShader* oldVS = nullptr; ctx->VSGetShader(&oldVS, nullptr, nullptr);
		ID3D11PixelShader* oldPS = nullptr; ctx->PSGetShader(&oldPS, nullptr, nullptr);
		ID3D11GeometryShader* oldGS = nullptr; ctx->GSGetShader(&oldGS, nullptr, nullptr);
		ID3D11InputLayout* oldLayout = nullptr; ctx->IAGetInputLayout(&oldLayout);
		D3D11_PRIMITIVE_TOPOLOGY oldTopo; ctx->IAGetPrimitiveTopology(&oldTopo);
		ID3D11ShaderResourceView* oldSRV = nullptr; ctx->PSGetShaderResources(0, 1, &oldSRV);
		ID3D11SamplerState* oldSamp = nullptr; ctx->PSGetSamplers(0, 1, &oldSamp);
		ID3D11Buffer* oldVSCB = nullptr; ctx->VSGetConstantBuffers(0, 1, &oldVSCB);
		ID3D11Buffer* oldPSCB = nullptr; ctx->PSGetConstantBuffers(0, 1, &oldPSCB);

		// ---- our draw ----
		ctx->UpdateSubresource(_cb, 0, nullptr, &cb, 0, 0);
		ctx->OMSetRenderTargets(1, &rtv, nullptr);
		D3D11_VIEWPORT vp = { 0.0f, 0.0f, (FLOAT)targetPx.width, (FLOAT)targetPx.height, 0.0f, 1.0f };
		ctx->RSSetViewports(1, &vp);
		D3D11_RECT sc = { sx0, sy0, sx1, sy1 };
		ctx->RSSetScissorRects(1, &sc);
		FLOAT bf[4] = { 1, 1, 1, 1 };
		ctx->OMSetBlendState(_blend, bf, 0xFFFFFFFF);
		ctx->OMSetDepthStencilState(_depth, 0);
		ctx->RSSetState(_raster);
		ctx->VSSetShader(_vs, nullptr, 0);
		ctx->PSSetShader(_ps, nullptr, 0);
		ctx->GSSetShader(nullptr, nullptr, 0);
		ctx->IASetInputLayout(nullptr);
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ID3D11ShaderResourceView* srv = bmp->_srv;
		ctx->PSSetShaderResources(0, 1, &srv);
		ID3D11SamplerState* samp = (interp == D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR) ? _sampPoint : _sampLinear;
		ctx->PSSetSamplers(0, 1, &samp);
		ID3D11Buffer* cbuf = _cb;
		ctx->VSSetConstantBuffers(0, 1, &cbuf);
		ctx->PSSetConstantBuffers(0, 1, &cbuf);
		ctx->Draw(4, 0);

		// ---- restore ----
		ctx->OMSetRenderTargets(1, &oldRTV, oldDSV);
		if (oldVPCount) ctx->RSSetViewports(oldVPCount, oldVP);
		if (oldScCount) ctx->RSSetScissorRects(oldScCount, oldSc);
		ctx->OMSetBlendState(oldBlend, oldBf, oldMask);
		ctx->OMSetDepthStencilState(oldDepth, oldSref);
		ctx->RSSetState(oldRaster);
		ctx->VSSetShader(oldVS, nullptr, 0);
		ctx->PSSetShader(oldPS, nullptr, 0);
		ctx->GSSetShader(oldGS, nullptr, 0);
		ctx->IASetInputLayout(oldLayout);
		ctx->IASetPrimitiveTopology(oldTopo);
		ctx->PSSetShaderResources(0, 1, &oldSRV);
		ctx->PSSetSamplers(0, 1, &oldSamp);
		ctx->VSSetConstantBuffers(0, 1, &oldVSCB);
		ctx->PSSetConstantBuffers(0, 1, &oldPSCB);
		if (oldRTV) oldRTV->Release();
		if (oldDSV) oldDSV->Release();
		if (oldBlend) oldBlend->Release();
		if (oldDepth) oldDepth->Release();
		if (oldRaster) oldRaster->Release();
		if (oldVS) oldVS->Release();
		if (oldPS) oldPS->Release();
		if (oldGS) oldGS->Release();
		if (oldLayout) oldLayout->Release();
		if (oldSRV) oldSRV->Release();
		if (oldSamp) oldSamp->Release();
		if (oldVSCB) oldVSCB->Release();
		if (oldPSCB) oldPSCB->Release();

#if LOGGER
		static int s_drawCount = 0;
		const int n = s_drawCount++;
		if ((n < 20) || (n % 512) == 0)
			log_debug("[DBG] [SHIM] draw #%d: %ux%u fmt=%d dst=(%.0f,%.0f-%.0f,%.0f) blend=%08X sc=(%ld,%ld-%ld,%ld)",
				n, bmp->_width, bmp->_height, bmp->_format,
				cb.dstRect[0], cb.dstRect[1], cb.dstRect[2], cb.dstRect[3],
				eff->_blendColor, sx0, sy0, sx1, sy1);
#endif
	}

	// ---------------- pure forwards: ID2D1RenderTarget ----------------
	STDMETHOD(CreateBitmapFromWicBitmap)(IWICBitmapSource* s, CONST D2D1_BITMAP_PROPERTIES* p, ID2D1Bitmap** b) override { return FWD->CreateBitmapFromWicBitmap(s, p, b); }
	STDMETHOD(CreateSharedBitmap)(REFIID riid, void* data, CONST D2D1_BITMAP_PROPERTIES* p, ID2D1Bitmap** b) override { return FWD->CreateSharedBitmap(riid, data, p, b); }
	STDMETHOD(CreateBitmapBrush)(ID2D1Bitmap* b, CONST D2D1_BITMAP_BRUSH_PROPERTIES* bp, CONST D2D1_BRUSH_PROPERTIES* p, ID2D1BitmapBrush** brush) override { return FWD->CreateBitmapBrush(b, bp, p, brush); }
	STDMETHOD(CreateSolidColorBrush)(CONST D2D1_COLOR_F* color, CONST D2D1_BRUSH_PROPERTIES* p, ID2D1SolidColorBrush** brush) override { return FWD->CreateSolidColorBrush(color, p, brush); }
	STDMETHOD(CreateGradientStopCollection)(CONST D2D1_GRADIENT_STOP* stops, UINT32 count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE mode, ID2D1GradientStopCollection** coll) override { return FWD->CreateGradientStopCollection(stops, count, gamma, mode, coll); }
	STDMETHOD(CreateLinearGradientBrush)(CONST D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES* gp, CONST D2D1_BRUSH_PROPERTIES* p, ID2D1GradientStopCollection* coll, ID2D1LinearGradientBrush** brush) override { return FWD->CreateLinearGradientBrush(gp, p, coll, brush); }
	STDMETHOD(CreateRadialGradientBrush)(CONST D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES* gp, CONST D2D1_BRUSH_PROPERTIES* p, ID2D1GradientStopCollection* coll, ID2D1RadialGradientBrush** brush) override { return FWD->CreateRadialGradientBrush(gp, p, coll, brush); }
	STDMETHOD(CreateCompatibleRenderTarget)(CONST D2D1_SIZE_F* size, CONST D2D1_SIZE_U* pxSize, CONST D2D1_PIXEL_FORMAT* fmt, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS opts, ID2D1BitmapRenderTarget** rt) override { return FWD->CreateCompatibleRenderTarget(size, pxSize, fmt, opts, rt); }
	STDMETHOD(CreateLayer)(CONST D2D1_SIZE_F* size, ID2D1Layer** layer) override { return FWD->CreateLayer(size, layer); }
	STDMETHOD(CreateMesh)(ID2D1Mesh** mesh) override { return FWD->CreateMesh(mesh); }
	STDMETHOD_(void, DrawLine)(D2D1_POINT_2F p0, D2D1_POINT_2F p1, ID2D1Brush* brush, FLOAT w, ID2D1StrokeStyle* ss) override { FWD->DrawLine(p0, p1, brush, w, ss); }
	STDMETHOD_(void, DrawRectangle)(CONST D2D1_RECT_F* r, ID2D1Brush* brush, FLOAT w, ID2D1StrokeStyle* ss) override { FWD->DrawRectangle(r, brush, w, ss); }
	STDMETHOD_(void, FillRectangle)(CONST D2D1_RECT_F* r, ID2D1Brush* brush) override { FWD->FillRectangle(r, brush); }
	STDMETHOD_(void, DrawRoundedRectangle)(CONST D2D1_ROUNDED_RECT* r, ID2D1Brush* brush, FLOAT w, ID2D1StrokeStyle* ss) override { FWD->DrawRoundedRectangle(r, brush, w, ss); }
	STDMETHOD_(void, FillRoundedRectangle)(CONST D2D1_ROUNDED_RECT* r, ID2D1Brush* brush) override { FWD->FillRoundedRectangle(r, brush); }
	STDMETHOD_(void, DrawEllipse)(CONST D2D1_ELLIPSE* e, ID2D1Brush* brush, FLOAT w, ID2D1StrokeStyle* ss) override { FWD->DrawEllipse(e, brush, w, ss); }
	STDMETHOD_(void, FillEllipse)(CONST D2D1_ELLIPSE* e, ID2D1Brush* brush) override { FWD->FillEllipse(e, brush); }
	STDMETHOD_(void, DrawGeometry)(ID2D1Geometry* g, ID2D1Brush* brush, FLOAT w, ID2D1StrokeStyle* ss) override { FWD->DrawGeometry(g, brush, w, ss); }
	STDMETHOD_(void, FillGeometry)(ID2D1Geometry* g, ID2D1Brush* brush, ID2D1Brush* opacity) override { FWD->FillGeometry(g, brush, opacity); }
	STDMETHOD_(void, FillMesh)(ID2D1Mesh* mesh, ID2D1Brush* brush) override { FWD->FillMesh(mesh, brush); }
	STDMETHOD_(void, FillOpacityMask)(ID2D1Bitmap* mask, ID2D1Brush* brush, D2D1_OPACITY_MASK_CONTENT content, CONST D2D1_RECT_F* dst, CONST D2D1_RECT_F* src) override { FWD->FillOpacityMask(mask, brush, content, dst, src); }
	STDMETHOD_(void, DrawBitmap)(ID2D1Bitmap* bmp, CONST D2D1_RECT_F* dst, FLOAT opacity, D2D1_BITMAP_INTERPOLATION_MODE mode, CONST D2D1_RECT_F* src) override { FWD->DrawBitmap(bmp, dst, opacity, mode, src); }
	STDMETHOD_(void, DrawText)(CONST WCHAR* text, UINT32 len, IDWriteTextFormat* fmt, CONST D2D1_RECT_F* rect, ID2D1Brush* brush, D2D1_DRAW_TEXT_OPTIONS opts, DWRITE_MEASURING_MODE mm) override { FWD->DrawText(text, len, fmt, rect, brush, opts, mm); }
	STDMETHOD_(void, DrawTextLayout)(D2D1_POINT_2F origin, IDWriteTextLayout* layout, ID2D1Brush* brush, D2D1_DRAW_TEXT_OPTIONS opts) override { FWD->DrawTextLayout(origin, layout, brush, opts); }
	STDMETHOD_(void, DrawGlyphRun)(D2D1_POINT_2F origin, CONST DWRITE_GLYPH_RUN* run, ID2D1Brush* brush, DWRITE_MEASURING_MODE mm) override { FWD->DrawGlyphRun(origin, run, brush, mm); }
	STDMETHOD_(void, SetTransform)(CONST D2D1_MATRIX_3X2_F* m) override { FWD->SetTransform(m); }
	STDMETHOD_(void, GetTransform)(D2D1_MATRIX_3X2_F* m) const override { FWD->GetTransform(m); }
	STDMETHOD_(void, SetAntialiasMode)(D2D1_ANTIALIAS_MODE mode) override { FWD->SetAntialiasMode(mode); }
	STDMETHOD_(D2D1_ANTIALIAS_MODE, GetAntialiasMode)() const override { return FWD->GetAntialiasMode(); }
	STDMETHOD_(void, SetTextAntialiasMode)(D2D1_TEXT_ANTIALIAS_MODE mode) override { FWD->SetTextAntialiasMode(mode); }
	STDMETHOD_(D2D1_TEXT_ANTIALIAS_MODE, GetTextAntialiasMode)() const override { return FWD->GetTextAntialiasMode(); }
	STDMETHOD_(void, SetTextRenderingParams)(IDWriteRenderingParams* params) override { FWD->SetTextRenderingParams(params); }
	STDMETHOD_(void, GetTextRenderingParams)(IDWriteRenderingParams** params) const override { FWD->GetTextRenderingParams(params); }
	STDMETHOD_(void, SetTags)(D2D1_TAG tag1, D2D1_TAG tag2) override { FWD->SetTags(tag1, tag2); }
	STDMETHOD_(void, GetTags)(D2D1_TAG* tag1, D2D1_TAG* tag2) const override { FWD->GetTags(tag1, tag2); }
	STDMETHOD_(void, PushLayer)(CONST D2D1_LAYER_PARAMETERS* params, ID2D1Layer* layer) override { FWD->PushLayer(params, layer); }
	STDMETHOD_(void, PopLayer)() override { FWD->PopLayer(); }
	STDMETHOD(Flush)(D2D1_TAG* tag1, D2D1_TAG* tag2) override { return FWD->Flush(tag1, tag2); }
	STDMETHOD_(void, SaveDrawingState)(ID2D1DrawingStateBlock* block) const override { FWD->SaveDrawingState(block); }
	STDMETHOD_(void, RestoreDrawingState)(ID2D1DrawingStateBlock* block) override { FWD->RestoreDrawingState(block); }
	STDMETHOD_(void, Clear)(CONST D2D1_COLOR_F* color) override { FWD->Clear(color); }
	STDMETHOD_(void, BeginDraw)() override { FWD->BeginDraw(); }
	STDMETHOD(EndDraw)(D2D1_TAG* tag1, D2D1_TAG* tag2) override { return FWD->EndDraw(tag1, tag2); }
	STDMETHOD_(D2D1_PIXEL_FORMAT, GetPixelFormat)() const override { return FWD->GetPixelFormat(); }
	STDMETHOD_(void, SetDpi)(FLOAT x, FLOAT y) override { FWD->SetDpi(x, y); }
	STDMETHOD_(void, GetDpi)(FLOAT* x, FLOAT* y) const override { FWD->GetDpi(x, y); }
	STDMETHOD_(D2D1_SIZE_F, GetSize)() const override { return FWD->GetSize(); }
	STDMETHOD_(D2D1_SIZE_U, GetPixelSize)() const override { return FWD->GetPixelSize(); }
	STDMETHOD_(UINT32, GetMaximumBitmapSize)() const override { return FWD->GetMaximumBitmapSize(); }
	STDMETHOD_(BOOL, IsSupported)(CONST D2D1_RENDER_TARGET_PROPERTIES* props) const override { return FWD->IsSupported(props); }

	// ---------------- pure forwards: ID2D1DeviceContext ----------------
	STDMETHOD(CreateBitmap)(D2D1_SIZE_U size, CONST void* data, UINT32 pitch, CONST D2D1_BITMAP_PROPERTIES1* props, ID2D1Bitmap1** bmp) override { return FWD->CreateBitmap(size, data, pitch, props, bmp); }
	STDMETHOD(CreateBitmapFromWicBitmap)(IWICBitmapSource* src, CONST D2D1_BITMAP_PROPERTIES1* props, ID2D1Bitmap1** bmp) override { return FWD->CreateBitmapFromWicBitmap(src, props, bmp); }
	STDMETHOD(CreateColorContext)(D2D1_COLOR_SPACE space, CONST BYTE* profile, UINT32 size, ID2D1ColorContext** cc) override { return FWD->CreateColorContext(space, profile, size, cc); }
	STDMETHOD(CreateColorContextFromFilename)(PCWSTR filename, ID2D1ColorContext** cc) override { return FWD->CreateColorContextFromFilename(filename, cc); }
	STDMETHOD(CreateColorContextFromWicColorContext)(IWICColorContext* wic, ID2D1ColorContext** cc) override { return FWD->CreateColorContextFromWicColorContext(wic, cc); }
	STDMETHOD(CreateBitmapFromDxgiSurface)(IDXGISurface* surface, CONST D2D1_BITMAP_PROPERTIES1* props, ID2D1Bitmap1** bmp) override { return FWD->CreateBitmapFromDxgiSurface(surface, props, bmp); }
	STDMETHOD(CreateGradientStopCollection)(CONST D2D1_GRADIENT_STOP* stops, UINT32 count, D2D1_COLOR_SPACE pre, D2D1_COLOR_SPACE post, D2D1_BUFFER_PRECISION prec, D2D1_EXTEND_MODE mode, D2D1_COLOR_INTERPOLATION_MODE cim, ID2D1GradientStopCollection1** coll) override { return FWD->CreateGradientStopCollection(stops, count, pre, post, prec, mode, cim, coll); }
	STDMETHOD(CreateImageBrush)(ID2D1Image* image, CONST D2D1_IMAGE_BRUSH_PROPERTIES* ip, CONST D2D1_BRUSH_PROPERTIES* bp, ID2D1ImageBrush** brush) override { return FWD->CreateImageBrush(image, ip, bp, brush); }
	STDMETHOD(CreateBitmapBrush)(ID2D1Bitmap* bmp, CONST D2D1_BITMAP_BRUSH_PROPERTIES1* bp, CONST D2D1_BRUSH_PROPERTIES* p, ID2D1BitmapBrush1** brush) override { return FWD->CreateBitmapBrush(bmp, bp, p, brush); }
	STDMETHOD(CreateCommandList)(ID2D1CommandList** list) override { return FWD->CreateCommandList(list); }
	STDMETHOD_(BOOL, IsDxgiFormatSupported)(DXGI_FORMAT fmt) const override { return FWD->IsDxgiFormatSupported(fmt); }
	STDMETHOD_(BOOL, IsBufferPrecisionSupported)(D2D1_BUFFER_PRECISION prec) const override { return FWD->IsBufferPrecisionSupported(prec); }
	STDMETHOD(GetImageLocalBounds)(ID2D1Image* image, D2D1_RECT_F* bounds) const override { return FWD->GetImageLocalBounds(image, bounds); }
	STDMETHOD(GetImageWorldBounds)(ID2D1Image* image, D2D1_RECT_F* bounds) const override { return FWD->GetImageWorldBounds(image, bounds); }
	STDMETHOD(GetGlyphRunWorldBounds)(D2D1_POINT_2F origin, CONST DWRITE_GLYPH_RUN* run, DWRITE_MEASURING_MODE mm, D2D1_RECT_F* bounds) const override { return FWD->GetGlyphRunWorldBounds(origin, run, mm, bounds); }
	STDMETHOD_(void, GetDevice)(ID2D1Device** device) const override { FWD->GetDevice(device); }
	STDMETHOD_(void, SetTarget)(ID2D1Image* image) override { FWD->SetTarget(image); }
	STDMETHOD_(void, GetTarget)(ID2D1Image** image) const override { FWD->GetTarget(image); }
	STDMETHOD_(void, SetRenderingControls)(CONST D2D1_RENDERING_CONTROLS* controls) override { FWD->SetRenderingControls(controls); }
	STDMETHOD_(void, GetRenderingControls)(D2D1_RENDERING_CONTROLS* controls) const override { FWD->GetRenderingControls(controls); }
	STDMETHOD_(void, SetPrimitiveBlend)(D2D1_PRIMITIVE_BLEND blend) override { FWD->SetPrimitiveBlend(blend); }
	STDMETHOD_(D2D1_PRIMITIVE_BLEND, GetPrimitiveBlend)() const override { return FWD->GetPrimitiveBlend(); }
	STDMETHOD_(void, SetUnitMode)(D2D1_UNIT_MODE mode) override { FWD->SetUnitMode(mode); }
	STDMETHOD_(D2D1_UNIT_MODE, GetUnitMode)() const override { return FWD->GetUnitMode(); }
	STDMETHOD_(void, DrawGlyphRun)(D2D1_POINT_2F origin, CONST DWRITE_GLYPH_RUN* run, CONST DWRITE_GLYPH_RUN_DESCRIPTION* desc, ID2D1Brush* brush, DWRITE_MEASURING_MODE mm) override { FWD->DrawGlyphRun(origin, run, desc, brush, mm); }
	STDMETHOD_(void, DrawGdiMetafile)(ID2D1GdiMetafile* metafile, CONST D2D1_POINT_2F* offset) override { FWD->DrawGdiMetafile(metafile, offset); }
	STDMETHOD_(void, DrawBitmap)(ID2D1Bitmap* bmp, CONST D2D1_RECT_F* dst, FLOAT opacity, D2D1_INTERPOLATION_MODE mode, CONST D2D1_RECT_F* src, CONST D2D1_MATRIX_4X4_F* xform) override { FWD->DrawBitmap(bmp, dst, opacity, mode, src, xform); }
	STDMETHOD_(void, PushLayer)(CONST D2D1_LAYER_PARAMETERS1* params, ID2D1Layer* layer) override { FWD->PushLayer(params, layer); }
	STDMETHOD(InvalidateEffectInputRectangle)(ID2D1Effect* effect, UINT32 input, CONST D2D1_RECT_F* rect) override { return FWD->InvalidateEffectInputRectangle(effect, input, rect); }
	STDMETHOD(GetEffectInvalidRectangleCount)(ID2D1Effect* effect, UINT32* count) override { return FWD->GetEffectInvalidRectangleCount(effect, count); }
	STDMETHOD(GetEffectInvalidRectangles)(ID2D1Effect* effect, D2D1_RECT_F* rects, UINT32 count) override { return FWD->GetEffectInvalidRectangles(effect, rects, count); }
	STDMETHOD(GetEffectRequiredInputRectangles)(ID2D1Effect* effect, CONST D2D1_RECT_F* output, CONST D2D1_EFFECT_INPUT_DESCRIPTION* descs, D2D1_RECT_F* rects, UINT32 count) override { return FWD->GetEffectRequiredInputRectangles(effect, output, descs, rects, count); }
	STDMETHOD_(void, FillOpacityMask)(ID2D1Bitmap* mask, ID2D1Brush* brush, CONST D2D1_RECT_F* dst, CONST D2D1_RECT_F* src) override { FWD->FillOpacityMask(mask, brush, dst, src); }
};

// ===========================================================================
// Movie presentation (plan magical-jumping-pascal B1).
//
// TgSmush's MF pipeline delivers 2560x1440@30fps BGRA frames into shared
// memory and presents via GDI StretchDIBits -- which runs in 0.4ms but the
// visible result is throttled to ~5-10fps by wine's GDI window-surface ->
// X11 flush cadence. This renders the shared-mem frame as a letterboxed
// D3D11 quad (reusing the shim's BitmapEffect shaders with blend=0 =
// passthrough) and presents through the game's DXVK swapchain instead.
// Called from PrimarySurface::UpdateOverlayDisplay (non-VR branch) after the
// frame has been uploaded into resources->_tgSmushTex.
// ===========================================================================

bool WineShim_TgSmushMoviePlaying()
{
	static int s_lastIndex = -1;
	static DWORD s_lastAdvance = 0;

	if (g_pSharedDataTgSmush == nullptr || g_pSharedDataTgSmush->videoFrameIndex <= 0)
	{
		s_lastIndex = -1;   // clean end-of-movie: rearm for the next one
		return false;
	}

	// Staleness guard: TgSmush clears videoFrameIndex when playback ends, but
	// if the player dies -- or its MF session never delivers the shutdown
	// callback (seen on wine-staging) -- the flag stays set forever and every
	// Flip would be dropped, freezing the screen on the last frame. Treat the
	// movie as over when the index stops advancing.
	const DWORD now = GetTickCount();
	if (g_pSharedDataTgSmush->videoFrameIndex != s_lastIndex)
	{
		s_lastIndex = g_pSharedDataTgSmush->videoFrameIndex;
		s_lastAdvance = now;
		return true;
	}
	return (now - s_lastAdvance) < 2000;
}

bool WineShim_PresentMovieFrame(DeviceResources* res)
{
	struct MovieCB
	{
		float dstRect[4];
		float uvRect[4];
		float xformRow0[4];
		float xformRow1[4];
		float blendColor[4];
	};

	static bool s_tried = false, s_ok = false;
	static ComPtr<ID3D11VertexShader> s_vs;
	static ComPtr<ID3D11PixelShader> s_ps;
	static ComPtr<ID3D11Buffer> s_cb;
	static ComPtr<ID3D11BlendState> s_blendOff;
	static ComPtr<ID3D11RasterizerState> s_raster;
	static ComPtr<ID3D11DepthStencilState> s_depthOff;
	static ComPtr<ID3D11SamplerState> s_sampLinear;
	static ID3D11Texture2D* s_srvTex = nullptr;
	static ComPtr<ID3D11ShaderResourceView> s_srv;

	ID3D11Device* dev = res->_d3dDevice;
	ID3D11DeviceContext* ctx = res->_d3dDeviceContext;
	if (!dev || !ctx || !res->_tgSmushTex || !res->_renderTargetView ||
		!res->_backBuffer || !res->_offscreenBuffer || !res->_swapChain)
		return false;

	if (!s_tried)
	{
		s_tried = true;
		HRESULT hr = dev->CreateVertexShader(g_ShimBitmapVS, sizeof(g_ShimBitmapVS), nullptr, &s_vs);
		if (SUCCEEDED(hr)) hr = dev->CreatePixelShader(g_ShimBitmapPS, sizeof(g_ShimBitmapPS), nullptr, &s_ps);
		if (SUCCEEDED(hr))
		{
			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = sizeof(MovieCB);
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			hr = dev->CreateBuffer(&bd, nullptr, &s_cb);
		}
		if (SUCCEEDED(hr))
		{
			D3D11_BLEND_DESC bd = {}; // opaque: blending off, all channels
			bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			hr = dev->CreateBlendState(&bd, &s_blendOff);
		}
		if (SUCCEEDED(hr))
		{
			D3D11_RASTERIZER_DESC rd = {};
			rd.FillMode = D3D11_FILL_SOLID;
			rd.CullMode = D3D11_CULL_NONE;
			rd.DepthClipEnable = TRUE;
			hr = dev->CreateRasterizerState(&rd, &s_raster);
		}
		if (SUCCEEDED(hr))
		{
			D3D11_DEPTH_STENCIL_DESC dd = {};
			hr = dev->CreateDepthStencilState(&dd, &s_depthOff);
		}
		if (SUCCEEDED(hr))
		{
			D3D11_SAMPLER_DESC sd = {};
			sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.MaxLOD = D3D11_FLOAT32_MAX;
			hr = dev->CreateSamplerState(&sd, &s_sampLinear);
		}
		s_ok = SUCCEEDED(hr);
		log_debug("[DBG] [SHIM] movie-present pipeline init: %s (0x%08X)", s_ok ? "OK" : "FAILED", hr);
	}
	if (!s_ok) return false;

	if (s_srvTex != (ID3D11Texture2D*)res->_tgSmushTex)
	{
		s_srv.Release();
		if (FAILED(dev->CreateShaderResourceView(res->_tgSmushTex, nullptr, &s_srv)))
		{
			log_debug("[DBG] [SHIM] movie-present: CreateShaderResourceView FAILED");
			s_srvTex = nullptr;
			return false;
		}
		s_srvTex = res->_tgSmushTex;
	}

	const float bw = (float)res->_backbufferWidth;
	const float bh = (float)res->_backbufferHeight;
	const float vw = (float)res->_tgSmushTexWidth;
	const float vh = (float)res->_tgSmushTexHeight;
	if (bw <= 0 || bh <= 0 || vw <= 0 || vh <= 0) return false;

	// letterbox, aspect preserved (TgSmush.cfg PreserveAspectRatio behavior)
	const float scale = (bw / vw < bh / vh) ? bw / vw : bh / vh;
	const float dw = vw * scale, dh = vh * scale;
	const float dx = (bw - dw) * 0.5f, dy = (bh - dh) * 0.5f;

	MovieCB cb;
	cb.dstRect[0] = dx; cb.dstRect[1] = dy;
	cb.dstRect[2] = dx + dw; cb.dstRect[3] = dy + dh;
	cb.uvRect[0] = 0.0f; cb.uvRect[1] = 0.0f; cb.uvRect[2] = 1.0f; cb.uvRect[3] = 1.0f;
	cb.xformRow0[0] = 1.0f; cb.xformRow0[1] = 0.0f; cb.xformRow0[2] = 0.0f; cb.xformRow0[3] = 1.0f;
	cb.xformRow1[0] = 0.0f; cb.xformRow1[1] = 0.0f; cb.xformRow1[2] = bw; cb.xformRow1[3] = bh;
	cb.blendColor[0] = cb.blendColor[1] = cb.blendColor[2] = cb.blendColor[3] = 0.0f;

	// ---- save the state we touch ----
	ID3D11RenderTargetView* oldRTV = nullptr; ID3D11DepthStencilView* oldDSV = nullptr;
	ctx->OMGetRenderTargets(1, &oldRTV, &oldDSV);
	D3D11_VIEWPORT oldVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	UINT oldVPCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	ctx->RSGetViewports(&oldVPCount, oldVP);
	ID3D11BlendState* oldBlend = nullptr; FLOAT oldBf[4]; UINT oldMask = 0;
	ctx->OMGetBlendState(&oldBlend, oldBf, &oldMask);
	ID3D11DepthStencilState* oldDepth = nullptr; UINT oldSref = 0;
	ctx->OMGetDepthStencilState(&oldDepth, &oldSref);
	ID3D11RasterizerState* oldRaster = nullptr;
	ctx->RSGetState(&oldRaster);
	ID3D11VertexShader* oldVS = nullptr; ctx->VSGetShader(&oldVS, nullptr, nullptr);
	ID3D11PixelShader* oldPS = nullptr; ctx->PSGetShader(&oldPS, nullptr, nullptr);
	ID3D11GeometryShader* oldGS = nullptr; ctx->GSGetShader(&oldGS, nullptr, nullptr);
	ID3D11InputLayout* oldLayout = nullptr; ctx->IAGetInputLayout(&oldLayout);
	D3D11_PRIMITIVE_TOPOLOGY oldTopo; ctx->IAGetPrimitiveTopology(&oldTopo);
	ID3D11ShaderResourceView* oldSRV = nullptr; ctx->PSGetShaderResources(0, 1, &oldSRV);
	ID3D11SamplerState* oldSamp = nullptr; ctx->PSGetSamplers(0, 1, &oldSamp);
	ID3D11Buffer* oldVSCB = nullptr; ctx->VSGetConstantBuffers(0, 1, &oldVSCB);
	ID3D11Buffer* oldPSCB = nullptr; ctx->PSGetConstantBuffers(0, 1, &oldPSCB);

	// ---- draw ----
	ctx->UpdateSubresource(s_cb, 0, nullptr, &cb, 0, 0);
	ID3D11RenderTargetView* rtv = res->_renderTargetView;
	FLOAT black[4] = { 0, 0, 0, 1 };
	ctx->ClearRenderTargetView(rtv, black);
	ctx->OMSetRenderTargets(1, &rtv, nullptr);
	D3D11_VIEWPORT vp = { 0.0f, 0.0f, bw, bh, 0.0f, 1.0f };
	ctx->RSSetViewports(1, &vp);
	FLOAT bf[4] = { 1, 1, 1, 1 };
	ctx->OMSetBlendState(s_blendOff, bf, 0xFFFFFFFF);
	ctx->OMSetDepthStencilState(s_depthOff, 0);
	ctx->RSSetState(s_raster);
	ctx->VSSetShader(s_vs, nullptr, 0);
	ctx->PSSetShader(s_ps, nullptr, 0);
	ctx->GSSetShader(nullptr, nullptr, 0);
	ctx->IASetInputLayout(nullptr);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ID3D11ShaderResourceView* srv = s_srv;
	ctx->PSSetShaderResources(0, 1, &srv);
	ID3D11SamplerState* samp = s_sampLinear;
	ctx->PSSetSamplers(0, 1, &samp);
	ID3D11Buffer* cbuf = s_cb;
	ctx->VSSetConstantBuffers(0, 1, &cbuf);
	ctx->PSSetConstantBuffers(0, 1, &cbuf);
	ctx->Draw(4, 0);

	// ---- to the swapchain ----
	ctx->ResolveSubresource(res->_backBuffer, 0, res->_offscreenBuffer, 0, BACKBUFFER_FORMAT);
	res->_swapChain->Present(1, 0);

	// ---- restore ----
	ctx->OMSetRenderTargets(1, &oldRTV, oldDSV);
	if (oldVPCount) ctx->RSSetViewports(oldVPCount, oldVP);
	ctx->OMSetBlendState(oldBlend, oldBf, oldMask);
	ctx->OMSetDepthStencilState(oldDepth, oldSref);
	ctx->RSSetState(oldRaster);
	ctx->VSSetShader(oldVS, nullptr, 0);
	ctx->PSSetShader(oldPS, nullptr, 0);
	ctx->GSSetShader(oldGS, nullptr, 0);
	ctx->IASetInputLayout(oldLayout);
	ctx->IASetPrimitiveTopology(oldTopo);
	ctx->PSSetShaderResources(0, 1, &oldSRV);
	ctx->PSSetSamplers(0, 1, &oldSamp);
	ctx->VSSetConstantBuffers(0, 1, &oldVSCB);
	ctx->PSSetConstantBuffers(0, 1, &oldPSCB);
	if (oldRTV) oldRTV->Release();
	if (oldDSV) oldDSV->Release();
	if (oldBlend) oldBlend->Release();
	if (oldDepth) oldDepth->Release();
	if (oldRaster) oldRaster->Release();
	if (oldVS) oldVS->Release();
	if (oldPS) oldPS->Release();
	if (oldGS) oldGS->Release();
	if (oldLayout) oldLayout->Release();
	if (oldSRV) oldSRV->Release();
	if (oldSamp) oldSamp->Release();
	if (oldVSCB) oldVSCB->Release();
	if (oldPSCB) oldPSCB->Release();

#if LOGGER
	static int s_presented = 0;
	const int n = s_presented++;
	if ((n < 5) || (n % 300) == 0)
		log_debug("[DBG] [SHIM] movie present #%d: %dx%d -> (%.0f,%.0f-%.0f,%.0f) of %dx%d",
			n, res->_tgSmushTexWidth, res->_tgSmushTexHeight, dx, dy, dx + dw, dy + dh,
			(int)bw, (int)bh);
#endif
	return true;
}

// ===========================================================================
// Cache + entry point
// ===========================================================================
bool ShimContext::s_shimMode = false;

static ShimContext* g_shimContext = nullptr;
static ID2D1RenderTarget* g_shimRealRT = nullptr;

// True when running under wine (any version). On real Windows this is false
// and the shim is never instantiated -- hook_concourse gets the raw render
// target, making this change a literal no-op for Windows users. Under wine
// the wrapper still tries every real d2d call first and only falls back when
// the call fails, so a wine release that implements custom effects
// automatically gets its native path back.
static bool WineShim_RunningUnderWine()
{
	static int s_isWine = -1;
	if (s_isWine < 0)
	{
		HMODULE ntdll = GetModuleHandleA("ntdll.dll");
		s_isWine = (ntdll != nullptr &&
			GetProcAddress(ntdll, "wine_get_version") != nullptr) ? 1 : 0;
	}
	return s_isWine == 1;
}

ID2D1RenderTarget* WineShim_GetRenderTarget(DeviceResources* resources)
{
	ID2D1RenderTarget* real = resources->_d2d1OffscreenRenderTarget;
	if (real == nullptr)
		return nullptr;

	if (!WineShim_RunningUnderWine())
		return real;

	if (g_shimContext != nullptr && g_shimRealRT == real)
		return g_shimContext;

	// underlying target changed (resize/recreate): drop the old shim
	if (g_shimContext != nullptr)
	{
		g_shimContext->Release();
		g_shimContext = nullptr;
		g_shimRealRT = nullptr;
	}

	ComPtr<ID2D1DeviceContext> realDC;
	if (FAILED(real->QueryInterface(IID_PPV_ARGS(&realDC))))
	{
		log_debug("[DBG] [SHIM] QI(ID2D1DeviceContext) failed; passing real RT through");
		return real;
	}

	g_shimContext = new ShimContext(resources, real, realDC);
	g_shimRealRT = real;
	log_debug("[DBG] [SHIM] wrapped d2d render target %p", real);
	return g_shimContext;
}
