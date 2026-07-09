// Copyright (c) 2026 Peter Soetens
// Licensed under the MIT license. See LICENSE.txt
//
// WineD2DEffectShim
//
// hook_concourse renders the whole HD concourse through a custom Direct2D
// effect (BitmapEffect). wine-8's d2d1 returns E_NOTIMPL from
// ID2D1DeviceContext::CreateEffect for custom registered effects; the hook
// ignores the hr and silently skips every draw -> the HD concourse renders
// black on Linux/wine.
//
// This shim wraps the ID2D1RenderTarget that ddraw hands to hook_concourse
// via SurfaceDC (GetDC) -- ONLY when running under wine; on Windows the raw
// render target is returned and none of this code is active. Under wine the
// wrapper forwards everything to the real d2d objects, except:
//   * CreateEffect(CLSID_BitmapEffect): tries the REAL call first; only when
//     that fails (wine) it returns a shim effect object, so a wine release
//     that implements custom effects gets its native path back automatically.
//   * CreateBitmap (once in shim mode): wraps a plain D3D11 texture
//     (B8G8R8A8 or BC3 -- BC3 is native to D3D11, dodging a second wine-d2d
//     gap) instead of a d2d bitmap.
//   * DrawImage(shim effect): Flush()es the real render target (preserves
//     draw ordering), then renders the bitmap as a native D3D11 textured
//     quad with a pixel shader replicating BitmapEffect's math
//     (Release/ShimBitmap{VS,PS}.h, source shaders/ShimBitmap{VS,PS}.hlsl).
#pragma once

#include "DeviceResources.h"

// Returns the render target to put into SurfaceDC::d2d1RenderTarget.
// On Windows: returns resources->_d2d1OffscreenRenderTarget unchanged.
// Under wine: wraps it; caches the wrapper and recreates it automatically
// when the underlying target changes (resize). Returns the raw target
// unwrapped only if wrapper creation fails.
ID2D1RenderTarget* WineShim_GetRenderTarget(DeviceResources* resources);

// True while TgSmush is actively delivering movie frames into shared memory:
// videoFrameIndex > 0 AND the index advanced within the last ~2 seconds.
// The staleness check guards against a TgSmush that died (or whose MF
// session never delivered the shutdown callback, as seen on wine-staging)
// without clearing the flag -- otherwise every Flip would be dropped
// forever, freezing the screen on the last presented frame.
bool WineShim_TgSmushMoviePlaying();

// Presents the current TgSmush shared-memory movie frame as a letterboxed
// quad through the D3D11 swapchain (gated by ddraw.cfg
// TgSmushSwapchainPresentEnabled; pairs with TgSmush.cfg MFD3DPresent=1).
bool WineShim_PresentMovieFrame(DeviceResources* resources);
