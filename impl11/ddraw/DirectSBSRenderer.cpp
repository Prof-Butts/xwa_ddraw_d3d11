#include "common.h"
#include "DirectSBSRenderer.h"

#ifdef _DEBUG
#include "../Debug/XwaD3dVertexShaderVR.h"
#include "../Debug/XwaD3dShadowVertexShaderVR.h"
#else
#include "../Release/XwaD3dVertexShaderVR.h"
#include "../Release/XwaD3dShadowVertexShaderVR.h"
#endif


// ****************************************************
// External variables
// ****************************************************

//************************************************************************
// SteamVR Renderer
//************************************************************************

DirectSBSRenderer *g_directsbs_renderer = nullptr;

DirectSBSRenderer::DirectSBSRenderer() : EffectsRenderer() {
}

void DirectSBSRenderer::CreateShaders()
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	EffectsRenderer::CreateShaders();

	if (g_bEnableVR)
	{
		device->CreateVertexShader(g_XwaD3dVertexShaderVR, sizeof(g_XwaD3dVertexShaderVR), nullptr, &_vertexShaderVR);
		device->CreateVertexShader(g_XwaD3dShadowVertexShaderVR, sizeof(g_XwaD3dShadowVertexShaderVR), nullptr, &_shadowVertexShaderVR);
	}
}

void DirectSBSRenderer::SceneBegin(DeviceResources* deviceResources)
{
	EffectsRenderer::SceneBegin(deviceResources);
}

void DirectSBSRenderer::SceneEnd()
{
	EffectsRenderer::SceneEnd();
}

void DirectSBSRenderer::RenderScene()
{
	if (_deviceResources->_displayWidth == 0 || _deviceResources->_displayHeight == 0)
	{
		return;
	}

	if (g_rendererType == RendererType_Shadow)
		// Using the _shadowVertexShaderVR is too expensive: we end up rendering the same scene 4 times.
		// Instead, let's do nothing here and just use the new hangar soft shadow system.
		// resources->InitVertexShader(_shadowVertexShaderVR);
		return;

	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;

	/*
	unsigned short scissorLeft = *(unsigned short*)0x07D5244;
	unsigned short scissorTop = *(unsigned short*)0x07CA354;
	unsigned short scissorWidth = *(unsigned short*)0x08052B8;
	unsigned short scissorHeight = *(unsigned short*)0x07B33BC;
	float scaleX = _viewport.Width / _deviceResources->_displayWidth;
	float scaleY = _viewport.Height / _deviceResources->_displayHeight;
	D3D11_RECT scissor{};
	scissor.left = (LONG)(_viewport.TopLeftX + scissorLeft * scaleX + 0.5f);
	scissor.top = (LONG)(_viewport.TopLeftY + scissorTop * scaleY + 0.5f);
	scissor.right = scissor.left + (LONG)(scissorWidth * scaleX + 0.5f);
	scissor.bottom = scissor.top + (LONG)(scissorHeight * scaleY + 0.5f);
	*/
	/*
	// The scissor rect needs more work in StemVR mode. It's in screen coords, so
	// It must use the dimensions of the SteamVR viewport, but it must also clip
	// the CMD when rendering the miniature. So the following won't really fix all
	// the cases:
	D3D11_RECT scissor{};
	scissor.left = 0; scissor.top = 0;
	scissor.right = g_steamVRWidth;
	scissor.bottom = g_steamVRHeight;
	*/

	//_deviceResources->InitScissorRect(&scissor);

	// TODO: Implement instanced rendering so that we issue only one draw call to
	// render both eyes.
	// https://github.com/Prof-Butts/xwa_ddraw_d3d11/issues/48

	
	// Regular VR path
	resources->InitVertexShader(_vertexShaderVR);

	D3D11_VIEWPORT viewport;
	// Ensure that we're not overriding regular depth in this path
	g_VSCBuffer.z_override = -1.0f;
	g_VSCBuffer.sz_override = -1.0f;
	g_VSCBuffer.mult_z_override = -1.0f;
	// Add extra depth to Floating GUI elements
	if (g_bIsFloating3DObject) {
		_bModifiedShaders = true;
		g_VSCBuffer.z_override += g_fFloatingGUIObjDepth;
	}
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);

	// ****************************************************************************
	// Render the left image
	// ****************************************************************************
	{
		viewport.Width = (float)resources->_backbufferWidth / 2.0f;
		viewport.Height = (float)resources->_backbufferHeight;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		ID3D11RenderTargetView *rtvs[6] = {
			// TODO: Check SelectOffscreenBuffer for DirectSBS mode
			SelectOffscreenBuffer(_bIsCockpit || _bIsGunner),
			resources->_renderTargetViewBloomMask.Get(),
			resources->_renderTargetViewDepthBuf.Get(), 
			// The normals hook should not be allowed to write normals for light textures. This is now implemented
			// in XwaD3dPixelShader
			_deviceResources->_renderTargetViewNormBuf.Get(),
			// Blast Marks are confused with glass because they are not shadeless; but they have transparency
			_bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMask.Get(),
			_bIsBlastMark ? NULL : resources->_renderTargetViewSSMask.Get(),
		};
		context->OMSetRenderTargets(6, rtvs, resources->_depthStencilViewL.Get());

		// Set the left projection matrix
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		// The viewMatrix is set at the beginning of the frame
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		context->DrawIndexed(_trianglesCount * 3, 0, 0);
	}

	// ****************************************************************************
	// Render the right image
	// ****************************************************************************
	{
		viewport.Width = (float)resources->_backbufferWidth / 2.0f;
		viewport.Height = (float)resources->_backbufferHeight;
		viewport.TopLeftX = viewport.TopLeftX = viewport.Width;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = D3D11_MIN_DEPTH;
		viewport.MaxDepth = D3D11_MAX_DEPTH;
		resources->InitViewport(&viewport);

		ID3D11RenderTargetView *rtvs[6] = {
			// TODO: Check SelectOffscreenBuffer for DirectSBS mode
			SelectOffscreenBuffer(_bIsCockpit || _bIsGunner),
			resources->_renderTargetViewBloomMask.Get(),
			resources->_renderTargetViewDepthBuf.Get(),
			// The normals hook should not be allowed to write normals for light textures. This is now implemented
			// in XwaD3dPixelShader
			_deviceResources->_renderTargetViewNormBuf.Get(),
			// Blast Marks are confused with glass because they are not shadeless; but they have transparency
			_bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMask.Get(),
			_bIsBlastMark ? NULL : resources->_renderTargetViewSSMask.Get(),
		};
		context->OMSetRenderTargets(6, rtvs, resources->_depthStencilViewL.Get());

		// Set the right projection matrix
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixRight;
		// The viewMatrix is set at the beginning of the frame
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		context->DrawIndexed(_trianglesCount * 3, 0, 0);
	}

//out:
	g_iD3DExecuteCounter++;
	g_iDrawCounter++; // We need this counter to enable proper Tech Room detection

}
