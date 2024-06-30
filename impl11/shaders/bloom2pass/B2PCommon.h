#ifndef B2P_COMMON_H
#define B2P_COMMON_H

#define DOWNSAMPLE_BLUR_RADIUS 5
#define MAX_LOD 6

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

float3 increaseSaturation(float3 col, float saturationStrength)
{
    float3 HSV = RGBtoHSV(col);
    HSV.y = saturate(HSV.y * saturationStrength);
    return saturate(HSVtoRGB(HSV));
}

float3 increaseSaturationHDR(float3 col, float saturationStrength)
{
    float3 HSV = RGBtoHSV(col);
    HSV.y = HSV.y * saturationStrength;
    return HSVtoRGB(HSV);
}

float3 superSaturate(float3 col, float strength)
{
    //const float3 gray = dot(0.333, col);
    const float  gray /* luma */ = dot(col.rgb, float3(0.299, 0.587, 0.114));
    const float3 dir  = col - gray;
    return strength * dir + gray;
}

float3 superSaturate001(float3 col, float strength)
{
    float val = dot(0.333, col);
    float L = saturate(length(col));
    return lerp(col, strength * val * normalize(col), L);
}
#endif