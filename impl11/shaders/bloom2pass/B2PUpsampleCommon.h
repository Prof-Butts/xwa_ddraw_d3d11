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

// From http://www.chilliant.com/rgb2hsv.html
static float Epsilon = 1e-10;

float3 RGBtoHCV(in float3 RGB)
{
    // Based on work by Sam Hocevar and Emil Persson
    float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0 / 3.0) : float4(RGB.gb, 0.0, -1.0 / 3.0);
    float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
    return float3(H, C, Q.x);
}

float3 HUEtoRGB(in float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return saturate(float3(R, G, B));
}

float3 RGBtoHSV(in float3 RGB)
{
    float3 HCV = RGBtoHCV(RGB);
    float S = HCV.y / (HCV.z + Epsilon);
    return float3(HCV.x, S, HCV.z);
}

float3 HSVtoRGB(in float3 HSV)
{
    float3 RGB = HUEtoRGB(HSV.x);
    return ((RGB - 1) * HSV.y + 1) * HSV.z;
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

float3 ReinhardExtLuma(float3 col, const float w)
{
    float l = luminance(col);
    float n = l * (1.0 + l / (w * w));
    float ln = n / (1.0 + l);
    return col * ln / l;
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

    nuv = clamp(nuv, 0.5, nres - 0.5);
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

float4 mainImage(in float2 fragCoord, const uint viewId)
{
    const float2 uv = fragCoord / iResolution.xy;
    float3 col = 0;

    // Skip 3x3 blur on first 3 mips
    /*col += SampleLod(uv, iResolution.xy, 0, viewId).rgb;
    col += SampleLod(uv, iResolution.xy, 1, viewId).rgb;
    col += SampleLod(uv, iResolution.xy, 2, viewId).rgb;

    col += SampleLodBlurred(uv, iResolution.xy, 3, viewId).rgb;
    col += SampleLodBlurred(uv, iResolution.xy, 4, viewId).rgb;
    col += SampleLodBlurred(uv, iResolution.xy, 5, viewId).rgb;*/

    /*col += bloomStr0 * SampleLodBlurred(uv, iResolution.xy, 0, viewId).rgb;
    col += bloomStr1 * SampleLodBlurred(uv, iResolution.xy, 1, viewId).rgb;
    col += bloomStr2 * SampleLodBlurred(uv, iResolution.xy, 2, viewId).rgb;

    col += bloomStr3 * SampleLodBlurred(uv, iResolution.xy, 3, viewId).rgb;
    col += bloomStr4 * SampleLodBlurred(uv, iResolution.xy, 4, viewId).rgb;
    col += bloomStr5 * SampleLodBlurred(uv, iResolution.xy, 5, viewId).rgb;*/

    col += pow(abs(SampleLodBlurred(uv, iResolution.xy, 0, viewId).rgb), bloomStr0);
    col += pow(abs(SampleLodBlurred(uv, iResolution.xy, 1, viewId).rgb), bloomStr1);
    col += pow(abs(SampleLodBlurred(uv, iResolution.xy, 2, viewId).rgb), bloomStr2);

    col += pow(abs(SampleLodBlurred(uv, iResolution.xy, 3, viewId).rgb), bloomStr3);
    col += pow(abs(SampleLodBlurred(uv, iResolution.xy, 4, viewId).rgb), bloomStr4);
    col += pow(abs(SampleLodBlurred(uv, iResolution.xy, 5, viewId).rgb), bloomStr5);

    //col = max(0, col) / 6.0;
    col = max(0, col);
    //col = 16.0 * max(0, col);
    //col = ReinhardExtLuma(col, 2.5);
    //col = ReinhardExtLuma(col, 5.5);
    //float3 bloom = linearTosRGB(max(0, col)); // This line fixes the black haloes

    //float3 bloom = pow(abs(linearTosRGB(col / (col + 1))), b2pExponent);
    float3 bloom = pow(abs(col / (col + 1)), b2pExponent);
    float3 HSV = RGBtoHSV(bloom);
    HSV.y = saturate(lerp(HSV.y, HSV.y * b2pSaturationStr, HSV.z));
    bloom = saturate(HSVtoRGB(HSV));

#ifdef INSTANCED_RENDERING
    float4 ofsColor = offscreenBuf.Sample(sampler0, float3(uv, viewId));
#else
    float4 ofsColor = offscreenBuf.Sample(sampler0, uv);
#endif
    // Screen blending mode
    ofsColor.rgb = 1 - (1 - ofsColor.rgb) * (1 - bloom);

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

float4 main(PixelShaderInput input) : SV_TARGET
{
    const float2 fragCoord = input.uv * iResolution.xy;

#ifdef INSTANCED_RENDERING
    return mainImage(fragCoord, input.viewId);
#else
    return mainImage(fragCoord, 0);
#endif
}
