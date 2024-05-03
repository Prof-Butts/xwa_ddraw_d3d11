/*
 This shader does a simple resize for 3D vision. We need this because 3D vision buffers are
 twice as wide as the backbuffer. This shader is used to resize the SBS data in _offscreenBuffer
 into the _vision3DPost buffer. It's based on the BarrelPixelShader and it's used in
 barrelEffect2D() and barrelEffect3D().
 */
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

Texture2D signatureTex : register(t1);
SamplerState signatureSamp : register(s1) =
	sampler_state {
		Filter = MIN_MAG_MIP_POINT; // No interpolation
	};

cbuffer ConstantBuffer : register(b0)
{
	float v; // The v coordinate where the signature must be added
	float unused0, unused1, unused2; // Unused, we might need these later to implement custom scaling.
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 texPos = input.tex;
	const float scale = 0.5f;

	if (texPos.x <= 0.5) {
		float2 LC = { 0.25, 0.5 };
		texPos = (texPos - LC) * float2(2.0 * scale, scale) + LC;
	}
	else {
		float2 LC = { 0.75, 0.5 };
		texPos = (texPos - LC) * float2(2.0 * scale, scale) + LC;
	}

	float4 texelColor = texture0.Sample(sampler0, texPos);
	if (input.tex.y < v)
		return float4(texelColor.rgb, 1.0f);
	else
		//return float4(1, 0, 0, 1);
		return signatureTex.Sample(signatureSamp, input.tex);
}