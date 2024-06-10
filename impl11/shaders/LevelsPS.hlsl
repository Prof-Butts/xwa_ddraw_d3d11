/*
 * Levels.fx, by Christian Cann Schuldt Jensen ~ CeeJay.dk
 * Adapted from:
 * https://github.com/Otakumouse/stormshade/blob/master/v3.X/reshade-shaders/Shader%20Library/Alternate/Levels.fx
 */
#include "ShaderToyDefs.h"
#include "ShadertoyCBuffer.h"

// The texture to process
Texture2D    colorTex  : register(t0);
SamplerState colorSamp : register(s0);

//#define MIP_MAP_DEBUG_W_LEVELSPS 1
#ifdef MIP_MAP_DEBUG_W_LEVELSPS
Texture2D auxTex : register(t1);
#endif

// We-re co-opting some of the hyperspace constants here for other
// purposes. A bit dirty, but saves us the trouble of creating yet
// another CB
#define WhitePoint twirl
#define BlackPoint bloom_strength

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 uv  : TEXCOORD;
};

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;

	// DEBUG
	//output.color = colorTex.Sample(colorSamp, input.uv);
	//output.color.b += 0.1;
	//return output;
	// DEBUG

	float black_point_float = BlackPoint / 255.0;
	float white_point_float = WhitePoint == BlackPoint ? (255.0 / 0.00025) : (255.0 / (WhitePoint - BlackPoint)); // Avoid division by zero if the white and black point are the same

	float3 color = colorTex.Sample(colorSamp, input.uv).rgb;
#ifdef MIP_MAP_DEBUG_W_LEVELSPS
	float4 auxColor = auxTex.SampleLevel(colorSamp, input.uv, 4);
	if (auxColor.a > 0)
		color.rgb = lerp(color.rgb, float3(0, 1, 0), saturate(4.0 * auxColor.a));
#endif

	color = saturate(color * white_point_float - (black_point_float *  white_point_float));

	// The following block is for debugging only. It's used to display which areas of
	// the image became saturated. This block only works if saturarte() is removed
	// from the previous expression.
	/*
	if (HighlightClipping)
	{
		float3 clipped_colors;

		clipped_colors = any(color > saturate(color)) // any colors whiter than white?
			? float3(1.0, 0.0, 0.0)
			: color;
		clipped_colors = all(color > saturate(color)) // all colors whiter than white?
			? float3(1.0, 1.0, 0.0)
			: clipped_colors;
		clipped_colors = any(color < saturate(color)) // any colors blacker than black?
			? float3(0.0, 0.0, 1.0)
			: clipped_colors;
		clipped_colors = all(color < saturate(color)) // all colors blacker than black?
			? float3(0.0, 1.0, 1.0)
			: clipped_colors;

		color = clipped_colors;
	}
	*/

	output.color = float4(color, 1);
	return output;
}
