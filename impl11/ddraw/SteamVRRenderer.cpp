/*
 * TODO:
 * - The hangar backdrop "swims" when the external camera is activated. This happens with
 *	 camera roll only.
 *	 Probable fix: remove the backdrop altogether and substitute with an environment map.
 *	 Make the environment map respond to head tracking motion.
 *   The position of the camera is not at the origin in external view. We need to compensate for that.
 * - Disable head tracking while rendering the miniature.
 */
#include "common.h"
#include "SteamVRRenderer.h"

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

SteamVRRenderer *g_steamvr_renderer = nullptr;

SteamVRRenderer::SteamVRRenderer() : EffectsRenderer() {
}

void SteamVRRenderer::CreateShaders()
{
	ID3D11Device* device = _deviceResources->_d3dDevice;

	EffectsRenderer::CreateShaders();

	if (g_bUseSteamVR)
	{
		device->CreateVertexShader(g_XwaD3dVertexShaderVR, sizeof(g_XwaD3dVertexShaderVR), nullptr, &_vertexShaderVR);
		device->CreateVertexShader(g_XwaD3dShadowVertexShaderVR, sizeof(g_XwaD3dShadowVertexShaderVR), nullptr, &_shadowVertexShaderVR);
	}
}

void SteamVRRenderer::SceneBegin(DeviceResources* deviceResources)
{
	EffectsRenderer::SceneBegin(deviceResources);
}

void SteamVRRenderer::SceneEnd()
{
	EffectsRenderer::SceneEnd();
}

/// <summary>
/// When the HD Concourse is enabled and D3dRendererTexturesHookEnabled is enabled too,
/// hook_concourse.dll will make a quick render in the Skirmish OPT selection screen.
/// However, this quick render is done into _offscreenBuffer in pancake mode and that won't
/// work in VR mode because we use array textures to enable instanced rendering. Instead, we
/// need to use a different path in VR that won't use any array textures and render to
/// _offscreenBufferHd instead. This method takes care of that:
/// </summary>
void SteamVRRenderer::RenderSkirmishOPT()
{
	auto &resources = _deviceResources;
	auto &context   = resources->_d3dDeviceContext;

	resources->InitVertexShader(_vertexShader);

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width    = HD_CONCOURSE_WIDTH;
	viewport.Height   = HD_CONCOURSE_HEIGHT;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	resources->InitViewport(&viewport);

	// Ensure that we're not overriding regular depth in this path
	g_VSCBuffer.z_override = -1.0f;
	g_VSCBuffer.sz_override = -1.0f;
	g_VSCBuffer.mult_z_override = -1.0f;
	// Add extra depth to Floating GUI elements
	resources->InitVSConstantBuffer3D(resources->_VSConstantBuffer.GetAddressOf(), &g_VSCBuffer);

	// I don't like doing this because it's hacky, but this works, so...
	// The skirmish OPT is rendered too big/outside the viewport for high VR resolutions.
	// To fix this, I'm scaling down the OPT, and moving it in 3D so that it more-or-less
	// lands in the proper area and isn't clipped. The numbers here were eyeballed (that's
	// the hacky part) because I don't know what's the proper bounds used for OPT rendering.
	// Like I said, it works, gets the job done, so moving on for now...
	// The 1.35f below is a generic scale to make the OPT a bit bigger:
	const float scale = 1.35f * (float)HD_CONCOURSE_HEIGHT / (float)g_steamVRHeight;
	const float xDisp = lerp(3.0f, 10.0f, (g_steamVRWidth - 1500.0f) / (2600.0f - 1500.0f));
	// 1.333f is for the aspect ratio
	Matrix4 S = Matrix4().scale(scale, scale, 1.333f * scale);
	Matrix4 T = Matrix4().translate(xDisp * METERS_TO_OPT, 0, 3.0f * METERS_TO_OPT);
	g_OPTMeshTransformCB.MeshTransform.identity();
	g_OPTMeshTransformCB.MeshTransform = T * S;
	// This transpose is needed because the vertex shader does a post-multiplication:
	g_OPTMeshTransformCB.MeshTransform.transpose();
	resources->InitVSConstantOPTMeshTransform(resources->_OPTMeshTransformCB.GetAddressOf(), &g_OPTMeshTransformCB);

	ID3D11RenderTargetView* rtvs[7] = {
		resources->_renderTargetViewHd.Get(),
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
	};
	context->OMSetRenderTargets(7, rtvs, resources->_depthStencilViewHd.Get());
	context->DrawIndexed(_trianglesCount * 3, 0, 0);
}

void SteamVRRenderer::RenderScene(bool bBindTranspLyr1)
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

	if (g_bInSkirmishShipScreen && g_config.HDConcourseEnabled && g_config.D3dRendererTexturesHookEnabled)
	{
		RenderSkirmishOPT();
		return;
	}

	auto &resources = _deviceResources;
	auto &context = resources->_d3dDeviceContext;

#ifdef DISABLED
	if (g_iD3DExecuteCounter == 0 && !g_bInTechRoom)
	{
		float blankMaterial[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float shadelessMaterial[4] = { 0.0f, 0.0f, 1.0f, 1.0f };

		if (!g_bMapMode)
		{
			context->ClearRenderTargetView(resources->_renderTargetViewSSAOMask, blankMaterial);
			context->ClearRenderTargetView(resources->_renderTargetViewSSMask, shadelessMaterial);

			context->CopyResource(resources->_backgroundBuffer, resources->_offscreenBuffer);
			// Wipe out the background:
			context->ClearRenderTargetView(resources->_renderTargetView, resources->clearColor);
		}

		if (g_bDumpSSAOBuffers)
			DirectX::SaveDDSTextureToFile(context, resources->_backgroundBuffer, L"c:\\temp\\_backgroundBuffer.dds");
		g_bBackgroundCaptured = true;
	}
#endif

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
	if (!g_bInTechRoom)
		// Regular VR path
		resources->InitVertexShader(_vertexShaderVR);
	else
		// When the Tech Room is enabled, the 3D content is mixed with the 2D content.
		// However, the 2D content is displayed as an VROverlay texture. To properly
		// mix them together we need to render the VR path as if it were the regular
		// non-VR path (otherwise the left eye is blended with the Tech Room and the
		// 3D looks smaller and displaced to the right). To solve that, we're using
		// the non-VR _vertexShader here:
		resources->InitVertexShader(_vertexShader);

	D3D11_VIEWPORT viewport;
	viewport.Width = (float)resources->_backbufferWidth;
	viewport.Height = (float)resources->_backbufferHeight;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = D3D11_MIN_DEPTH;
	viewport.MaxDepth = D3D11_MAX_DEPTH;
	resources->InitViewport(&viewport);

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
		ID3D11RenderTargetView *rtvs[7] = {
			SelectOffscreenBuffer(),
			resources->_renderTargetViewBloomMask.Get(),
			resources->_renderTargetViewDepthBuf.Get(), 
			// The normals hook should not be allowed to write normals for light textures. This is now implemented
			// in XwaD3dPixelShader
			_deviceResources->_renderTargetViewNormBuf.Get(),
			// Blast Marks are confused with glass because they are not shadeless; but they have transparency
			_bIsBlastMark ? NULL : resources->_renderTargetViewSSAOMask.Get(),
			_bIsBlastMark ? NULL : resources->_renderTargetViewSSMask.Get(),
			bBindTranspLyr1 ? resources->_transp1RTV.Get() : NULL,
		};
		context->OMSetRenderTargets(7, rtvs, resources->_depthStencilViewL.Get());

		// Set the left projection matrix		
		g_VSMatrixCB.projEye[0] = g_FullProjMatrixLeft;
		g_VSMatrixCB.projEye[1] = g_FullProjMatrixRight;
		// The viewMatrix is set at the beginning of the frame
		resources->InitVSConstantBufferMatrix(resources->_VSMatrixBuffer.GetAddressOf(), &g_VSMatrixCB);

		//context->DrawIndexed(_trianglesCount * 3, 0, 0);
		context->DrawIndexedInstanced(_trianglesCount * 3, 2, 0, 0, 1); // Already in the SteamVR path
	}

	g_iD3DExecuteCounter++;
	g_iDrawCounter++; // We need this counter to enable proper Tech Room detection
}
