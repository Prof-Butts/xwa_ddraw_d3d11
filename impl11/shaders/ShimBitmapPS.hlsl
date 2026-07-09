// WineD2DEffectShim pixel shader.
// Byte-faithful port of BitmapPixelShader.hlsl from xwa_hook_concourse (the
// hook's custom D2D BitmapEffect, author Jeremy Ansel): optional
// black-keying, optional monochrome tint by max(r,g,b), and
// premultiplied-alpha output. NOTE: xwa_hook_concourse carries no license
// file; this port's redistribution terms follow the original author's --
// see the pull-request discussion before reusing this file elsewhere.
cbuffer ShimCB : register(b0)
{
    float4 dstRect;
    float4 uvRect;
    float4 xformRow0;
    float4 xformRow1;
    float4 blendColor; // packed from uint32 LSB-first /255 like BitmapEffect::SetBlendColor
};

Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct VSOut
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(VSOut i) : SV_TARGET
{
    float4 color = tex0.Sample(samp0, i.uv);

    float blendB = blendColor.r;
    float blendG = blendColor.g;
    float blendR = blendColor.b;
    bool hasBlendColor = blendB != 0 || blendG != 0 || blendR != 0;
    bool isBlackTransparent = blendColor.a >= 0.5f;

    float b = color.b;
    float g = color.g;
    float r = color.r;
    float a = color.a;

    if (isBlackTransparent)
    {
        if (b == 0 && g == 0 && r == 0)
        {
            a = 0;
        }
    }

    if (a != 0 && hasBlendColor)
    {
        float s = max(b, max(g, r));

        b = s * blendB;
        g = s * blendG;
        r = s * blendR;
    }

    if (a == 0)
    {
        b = 0;
        g = 0;
        r = 0;
        a = 0;
    }

    return float4(r * a, g * a, b * a, a);
}
