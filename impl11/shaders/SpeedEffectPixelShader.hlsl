#include "ShaderToyDefs.h"
#include "ShadertoyCBuffer.h"

// The position/depth buffer
Texture2D texPos : register(t4);
SamplerState sampPos : register(s4);

struct PixelShaderInput
{
	float4 pos   : SV_POSITION;
	float4 color : COLOR0;
	float2 uv    : TEXCOORD0;
	float3 pos3D : TEXCOORD1;
};

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input) {
	PixelShaderOutput output;

	// From the vertex shader, the input xy coords are normalized [-1..1] vertex
	// screen coords. So if we apply the proper transforms, we can convert them
	// into post-proc UV coords:
	const float2 uv = (input.pos3D.xy * float2(0.5, -0.5)) + 0.5;
	if (any(uv < p0) || any(uv > p1))
		discard;

	// The vertex shader passes the original metric Z coord, so here we can use it
	// to do a depth test and discard pixels that should not be rendered:
	const float  curZ  = input.pos3D.z;
	const float3 depth = texPos.Sample(sampPos, uv).xyz;
	if (curZ >= depth.z)
		discard;

	// Alpha can be 1 and that's OK: in the speed compose shader we'll use the intensity
	// of the particle for blending.
	output.color = float4(0.0, 0.0, 0.0, 1.0);
	//float L = length(input.uv) - 0.75;
	//L = (L <= 0.0) ? 1.0 : 0.0;
	// The speed is encoded in the blue component of the color, so that's why
	// we multiply by input.color.b
	output.color.rgb = input.color.b * smoothstep(0.9, 0.6, length(input.uv));
	//output.color.rg = L;
	return output;
}