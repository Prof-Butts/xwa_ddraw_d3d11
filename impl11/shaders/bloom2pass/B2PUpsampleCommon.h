// 2-pass bloom, using mip-maps for the pyramid. Based on:
// https://www.shadertoy.com/view/cty3R3

#include "../ShaderToyDefs.h"
#include "../ShadertoyCBuffer.h"
#include "B2PCommon.h"

#ifdef INSTANCED_RENDERING
Texture2DArray texture0     : register(t0);
Texture2DArray offscreenBuf : register(t1);
#else
Texture2D texture0     : register(t0);
Texture2D offscreenBuf : register(t1);
#endif
SamplerState sampler0 : register(s0);

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

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
    float4 bloom : SV_TARGET1; // This is not needed, it's just for debugging the bloom buffer
};

float3 ReHue(float3 RGB)
{
    float3 HSV = RGBtoHSV(RGB);
    float h1 = HSV.x - 0.16666f;
    float h2 = HSV.x - 1.16666f;
    float ah1 = abs(h1);
    float ah2 = abs(h2);
    float h = ah1 < ah2 ? h1 : h2;
    float s = sign(h);
    h = abs(h);
    // Compress the hue around the new origin
    //h = s * sqrt(h);
    //h = s * pow(h, 0.85);
    h = s * pow(h, 0.9);
    //h = s * pow(h, 1.0 - bloomStr5);
    h = saturate(h + 0.16666f);
    return HSVtoRGB(float3(h, HSV.yz));
}

inline float luminance(float3 col)
{
    return dot(col, float3(0.2126729, 0.7151522, 0.0721750));\
}

float3 linearTosRGB(float3 col)
{
    float3 lt = float3(col.x < 0.0031308, col.y < 0.0031308, col.z < 0.0031308);
    return lerp(1.055 * pow(abs(col), 1.0 / 2.4) - 0.055, col * 12.92, lt);
}

// https://64.github.io/tonemapping/
// https://imdoingitwrong.wordpress.com/2010/08/19/why-reinhard-desaturates-my-blacks-3
vec3 ReinhardExt(vec3 col, const float w)
{
    vec3 n = col * (1.0 + col / (w * w));
    return n / (1.0 + col);
}

float3 ReinhardExtLuma(float3 col, const float w)
{
    float l = luminance(col);
    float n = l * (1.0 + l / (w * w));
    float ln = n / (1.0 + l);
    return col * ln / l;
}

float3 Uncharted2TonemapPartial(float3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float3 Uncharted2Tonemap(float3 x)
{
    const float E = 2.5;
    const float W = 11.2;

    return Uncharted2TonemapPartial(x * E) / Uncharted2TonemapPartial(W);
}

// ACES tone mapping curve fit to go from HDR to LDR
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
}

float4 SampleLod(float2 uv, float2 res, const int lod, const uint viewId)
{
    float2 hres = floor(res / 2.0);

    float2 nres = hres;
    float  xpos = 0.0;
    int i = 0;
    [unroll]
    for (; i < lod; i++)
    {
        xpos += nres.x;
        nres = floor(nres / 2.0);
    }

    float2 nuv = uv * float2(nres);

    nuv  = clamp(nuv, 0.5, nres - 0.5);
    nuv += float2(xpos, 0);

#ifdef INSTANCED_RENDERING
    return texture0.Sample(sampler0, float3(nuv / res, viewId));
#else
    return texture0.Sample(sampler0, nuv / res);
#endif
}

float4 SampleLodBlurred(float2 uv, float2 res, const int lod, const uint viewId)
{
    float4 result = 0;
    float  sc = exp2(float(lod));
    float2 nres = floor(res / sc * 0.5);

    // Optimized blur using bilinear filtering
    // https://john-chapman.github.io/2019/03/29/convolution.html
    const float2 offsets[4] = {
        float2(-1.0 / 3.0, -1.0 / 3.0),
        float2( 1.0 / 3.0, -1.0 / 3.0),
        float2(-1.0 / 3.0,  1.0 / 3.0),
        float2( 1.0 / 3.0,  1.0 / 3.0)
    };

    for (int i = 0; i < 4; i++)
    {
        vec2 o = offsets[i];
        vec2 p = uv + o / nres;

        result += SampleLod(saturate(p), iResolution.xy, lod, viewId);
    }
    result *= 0.25;

    return result;
}

float4 mainImage(in float2 fragCoord, const uint viewId, out float4 bloom_out)
{
    const float2 uv = fragCoord / iResolution.xy;
    float3 bloom = 0;

    // Skip 3x3 blur on first 3 mips
    bloom += bloomStr0 * SampleLod(uv, iResolution.xy, 0, viewId).rgb;
    bloom += bloomStr1 * SampleLod(uv, iResolution.xy, 1, viewId).rgb;
    bloom += bloomStr2 * SampleLod(uv, iResolution.xy, 2, viewId).rgb;

    //bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 0, viewId).rgb), bloomStr0);
    //bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 1, viewId).rgb), bloomStr1);
    //bloom += pow(abs(SampleLod(uv, iResolution.xy, 2, viewId).rgb), bloomStr2);
    //bloom += bloomStr2 * SampleLod(uv, iResolution.xy, 2, viewId).rgb;

    bloom += bloomStr3 * SampleLod(uv, iResolution.xy, 3, viewId).rgb;
    bloom += bloomStr4 * SampleLod(uv, iResolution.xy, 4, viewId).rgb;
    bloom += bloomStr5 * SampleLod(uv, iResolution.xy, 5, viewId).rgb;

    bloom_out = float4(bloom, 1);

    //bloom += bloomStr0 * SampleLodBlurred(uv, iResolution.xy, 0, viewId).rgb;
    //bloom += bloomStr1 * SampleLodBlurred(uv, iResolution.xy, 1, viewId).rgb;
    //bloom += bloomStr2 * SampleLodBlurred(uv, iResolution.xy, 2, viewId).rgb;

    //bloom += bloomStr3 * SampleLodBlurred(uv, iResolution.xy, 3, viewId).rgb;
    //bloom += bloomStr4 * SampleLodBlurred(uv, iResolution.xy, 4, viewId).rgb;
    ////bloom += bloomStr5 * SampleLodBlurred(uv, iResolution.xy, 5, viewId).rgb;

    /*bloom += bloomStr0 * SampleLod(uv, iResolution.xy, 0, viewId).rgb;
    bloom += bloomStr1 * SampleLod(uv, iResolution.xy, 1, viewId).rgb;
    bloom += bloomStr2 * SampleLod(uv, iResolution.xy, 2, viewId).rgb;

    bloom += bloomStr3 * SampleLod(uv, iResolution.xy, 3, viewId).rgb;
    bloom += bloomStr4 * SampleLod(uv, iResolution.xy, 4, viewId).rgb;
    bloom += bloomStr5 * SampleLod(uv, iResolution.xy, 5, viewId).rgb;*/

    /*bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 0, viewId).rgb), bloomStr0);
    bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 1, viewId).rgb), bloomStr1);
    bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 2, viewId).rgb), bloomStr2);

    bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 3, viewId).rgb), bloomStr3);
    bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 4, viewId).rgb), bloomStr4);
    bloom += pow(abs(SampleLodBlurred(uv, iResolution.xy, 5, viewId).rgb), bloomStr5);*/

    /*bloom += pow(abs(SampleLod(uv, iResolution.xy, 0, viewId).rgb), bloomStr0);
    bloom += pow(abs(SampleLod(uv, iResolution.xy, 1, viewId).rgb), bloomStr1);
    bloom += pow(abs(SampleLod(uv, iResolution.xy, 2, viewId).rgb), bloomStr2);

    bloom += pow(abs(SampleLod(uv, iResolution.xy, 3, viewId).rgb), bloomStr3);
    bloom += pow(abs(SampleLod(uv, iResolution.xy, 4, viewId).rgb), bloomStr4);
    bloom += pow(abs(SampleLod(uv, iResolution.xy, 5, viewId).rgb), bloomStr5);*/

    //col = max(0, col) / 6.0;
    //bloom = max(0, bloom);
    //bloom_out = float4(bloom, 1);
    //col = 16.0 * max(0, col);
    //col = ReinhardExtLuma(col, 2.5);
    //col = ReinhardExtLuma(col, 5.5);
    //float3 bloom = linearTosRGB(max(0, col)); // This line fixes the black haloes

    //float3 bloom = pow(abs(linearTosRGB(col / (col + 1))), b2pExponent);

    // Apply tone mapping
    bloom = bloom / (bloom + 1);
    //bloom = ReinhardExtLuma(bloom, 2.5);
    //bloom = ReinhardExt(bloom, 2.5);
    //bloom = ACESFilm(bloom);
    //bloom = Uncharted2TonemapPartial(bloom);
    //bloom = Uncharted2Tonemap(bloom);

    // Increase color saturation
    //bloom = increaseSaturation(bloom, b2pSaturationStr);
    bloom = superSaturate(bloom, b2pSaturationStr);

    //bloom = increaseSaturation(bloom, bloomStr0);
    //bloom = ReHue(increaseSaturation(bloom, b2pSaturationStr, bloomStr5));
    //bloom = increaseSaturation(bloom, saturationStrength);

    /*float3 HSV = RGBtoHSV(bloom);
    HSV.y = saturate(lerp(HSV.y, HSV.y * saturationStrength, HSV.z));
    bloom = saturate(HSVtoRGB(HSV));*/

    // Gamma correction (approx)
    //bloom = sqrt(bloom);
    //bloom = pow(abs(bloom), b2pExponent);

#ifdef INSTANCED_RENDERING
    float4 ofsColor = offscreenBuf.Sample(sampler0, float3(uv, viewId));
#else
    float4 ofsColor = offscreenBuf.Sample(sampler0, uv);
#endif
    // Screen blending mode
    ofsColor.rgb = 1 - (1 - ofsColor.rgb) * (1 - bloom);

    //bloom_out = float4(bloom, 1);
    return ofsColor;
}

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;

    const float2 fragCoord = input.uv * iResolution.xy;
    output.color = 0;
    output.bloom = 0;

#ifdef INSTANCED_RENDERING
    output.color = mainImage(fragCoord, input.viewId, output.bloom);
#else
    output.color = mainImage(fragCoord, 0, output.bloom);
#endif
    //output.color = output.bloom;
    return output;
}
