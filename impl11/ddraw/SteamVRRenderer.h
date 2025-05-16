#pragma once

#include "EffectsRenderer.h"

class SteamVRRenderer : public EffectsRenderer {
public:
	SteamVRRenderer();
	virtual void SceneBegin(DeviceResources* deviceResources);
	virtual void SceneEnd();
	void RenderSkirmishOPT();
	virtual void RenderScene(bool bBindTranspLyr1);
	virtual void CreateShaders();

protected:
	ComPtr<ID3D11VertexShader> _vertexShaderVR;
	ComPtr<ID3D11VertexShader> _shadowVertexShaderVR;
};

extern SteamVRRenderer *g_steamvr_renderer;
