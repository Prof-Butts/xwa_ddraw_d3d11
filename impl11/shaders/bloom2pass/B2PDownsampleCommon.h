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

// struct BloomPixelShaderCBuffer
cbuffer ConstantBuffer : register(b2)
{
    float pixelSizeX, pixelSizeY, bloomStr0, amplifyFactor;
    // 16 bytes
    float bloomStrength, uvStepSize, saturationStrength, bloomStr1;
    // 32 bytes
    float bloomStr2, bloomStr3, depth_weight;
    int debug;
    // 48 bytes
    float bloomStr4, bloomStr5, b2pSaturationStr, b2pExponent;
    // 64 bytes
};

static const float weight[3] = {0.38774, 0.24477, 0.06136};

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
    const float2 uv = (fragCoord - float2(xpos - res.x, 0)) / res;

#define BLOOM_BOOST 1.0
#ifdef INSTANCED_RENDERING
    float4 bloomInput = BLOOM_BOOST * texture0.SampleLevel(sampler0, float3(uv, viewId), 1.0);
#else
    float4 bloomInput = BLOOM_BOOST * texture0.SampleLevel(sampler0, uv, 1.0);
#endif
    bloomInput = max(0, bloomInput);

#ifdef DISABLED
    for (int curLod = lod; curLod > 0; curLod--)
    {
        const float2 uv = fragCoord / iResolution.xy;
#ifdef INSTANCED_RENDERING
        bloomInput += max(0, texture0.SampleLevel(sampler0, float3(uv, viewId), float(curLod) - 1.0));
#else
        bloomInput += max(0, texture0.SampleLevel(sampler0, uv, float(curLod) - 1.0));
#endif
    }
#endif

    // Skip blurring LOD 0 for performance
    if (lod == 0)
    {
        fragColor = bloomInput;
        return;
    }

    //const int rad = 2;
    const float lodLinear = lod / (float)(MAX_LOD - 1);
    //const float exponent = lerp(1.0, 0.6, lodLinear);
    const int rad = DOWNSAMPLE_BLUR_RADIUS;
    //const int rad = 3;
    //const int rad = max(2, ceil(DOWNSAMPLE_BLUR_RADIUS * bloomStr0 * lodLinear));
    //const int rad = 2 * lod;
    //const float sigma = float(rad); // Higher numbers do more blur
    //const float sigma = float(rad) * 0.4; // Higher numbers do more blur
    //const float sigma = float(rad) * (lod * 0.5); // Higher numbers do more blur
    //const float sigma = float(rad) * bloomStr5;
    const float sigma = float(rad) * 0.5;
    //const float bloomBoost = max(1.0, 0.4 * lod);
    //const float bloomBoost = 1.0;
    const float bloomBoost = max(1.0, 1.0 + lodLinear * 2.0);

    float w = 0.0;
    //float delta = 1.0 / lod;
    float delta = 1.0;
    //float delta = 2.0 * lod;
    float py = -delta * rad;
    for (int y = -rad; y <= rad; y++, py += delta)
    //int y = 0;
    {
        //const float wgY = weight[abs(y)];
        float px = -delta * rad;
        for (int x = -rad; x <= rad; x++, px += delta)
        //int x = 0;
        {
            //const float wgX = weight[abs(x)];
            //const float2 o = float2(x, y);
            const float2 o = float2(px, py);
            //const float wg = wgX * wgY;
            float wg = gaussian(o, sigma);
            float2 p = uv + o / float2(res);

            // Clamp to edge
            //p = clamp(p, 0.5 / res, (res - 0.5) / res);

            // Clamp to border
            const float2 q = clamp(p, 0.5 / res, (res - 0.5) / res);
            if (p.x == q.x && p.y == q.y)
            {
#ifdef INSTANCED_RENDERING
                bloomInput = bloomBoost * max(0, texture0.SampleLevel(sampler0, float3(p, viewId), float(lod)));
#else
                bloomInput = bloomBoost * max(0, texture0.SampleLevel(sampler0, p, float(lod)));
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
