// 2-pass bloom, using mip-maps for the pyramid. Based on:
// https://www.shadertoy.com/view/cty3R3

#include "../ShaderToyDefs.h"
#include "../ShadertoyCBuffer.h"
#include "B2PCommon.h"

#ifdef INSTANCED_RENDERING
Texture2DArray texture0 : register(t0);
#else
Texture2D      texture0 : register(t0);
#endif
SamplerState   sampler0 : register(s0);

float gaussian(float2 i, float sigma) {
    return exp(-dot(i,i) / (2.0 * sigma*sigma));
}

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

void mainImage(in float2 fragCoord, in uint viewId, out float4 fragColor)
{
    float2 hres = floor(iResolution.xy / 2.0);
    float2 res = hres;

    float xpos = 0.0;
    int lod = 0;
    for (; lod < MAX_LOD; lod++)
    {
        xpos += res.x;

        if (xpos > fragCoord.x || res.x <= 1.0)
            break;
        res = floor(res / 2.0);
    }

    if (fragCoord.y >= res.y)
    {
        fragColor = 0;
        return;
    }

    fragColor = 0;

    float2 px = 1.0 / iResolution.xy;
    float2 p  = (fragCoord - float2(xpos - res.x, 0)) / iResolution.xy;
    float2 uv = (fragCoord - float2(xpos - res.x, 0)) / res;

#define BLOOM_BOOST 1.0
#ifdef INSTANCED_RENDERING
    float4 bloomInput = BLOOM_BOOST * texture0.SampleLevel(sampler0, float3(uv, viewId), 1.0);
#else
    float4 bloomInput = BLOOM_BOOST * texture0.SampleLevel(sampler0, uv, 1.0);
#endif
    bloomInput = max(0, bloomInput);

    // Skip blurring LOD 0 for performance
    if (lod == 0)
    {
        fragColor = bloomInput;
        return;
    }

    const int   rad   = DOWNSAMPLE_BLUR_RADIUS;
    const float sigma = float(rad) * 0.4;

    float w = 0.0;
    for (int x = -rad; x <= rad; x++)
    {
        for (int y = -rad; y <= rad; y++)
        {
            float2 o = float2(x, y);
            float wg = gaussian(o, sigma);
            //float wg = exp(-dot(o, o) * 0.125);
            const float2 p = uv + o / float2(res);

            // Clamp to edge
            //p = clamp(p, 0.5 / res, (res - 0.5) / res);

            // Clamp to border
            const float2 q = clamp(p, 0.5 / res, (res - 0.5) / res);
            if (p.x == q.x && p.y == q.y)
            {
#ifdef INSTANCED_RENDERING
                bloomInput = BLOOM_BOOST * max(0, texture0.SampleLevel(sampler0, float3(p, viewId), float(lod)));
#else
                bloomInput = BLOOM_BOOST * max(0, texture0.SampleLevel(sampler0, p, float(lod)));
#endif
                fragColor += wg * bloomInput;
            }
            w += wg;
        }
    }
    fragColor /= w;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 fragColor = 0;
    float2 fragCoord = input.uv * iResolution.xy;

#ifdef INSTANCED_RENDERING
    mainImage(fragCoord, input.viewId, fragColor);
#else
    mainImage(fragCoord, 0, fragColor);
#endif

    return fragColor;
}
