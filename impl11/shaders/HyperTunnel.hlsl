/*
 * Hyperspace Tunnel
 * Based on theGiallo's shader: https://www.shadertoy.com/view/MttSz2
 * blue_max and keiranhalcyon7, 2019, see https://www.shadertoy.com/view/Wtd3Wr
 */
#include "ShaderToyDefs.h"
#include "ShadertoyCBuffer.h"

//static const float3 blue_color = float3(0.15, 0.35, 0.8);
static const float3 blue_color = float3(0.15, 0.4, 0.9);
static const float3 red_color = float3(0.9, 0.15, 0.15);
static const float3 yellow_color = float3(0.7, 0.65, 0.15);
static const float rotation_speed = 2.3;
static const float t2 = 4.0;

// The Foreground Color Buffer (_shadertoyBuf)
//Texture2D fgColorTex : register(t0);
//SamplerState fgColorSampler : register(s0);
//
//// The Background Color Buffer (_shadertoyAuxBuf)
//Texture2D bgColorTex : register(t1);
//SamplerState bgColorSampler : register(s1);

// From https://www.shadertoy.com/view/4sc3z2
// and https://www.shadertoy.com/view/XsX3zB
#define MOD3 vec3(.1031,.11369,.13787)
vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * MOD3);
	p3 += dot(p3, p3.yxz + 19.19);
	return -1.0 + 2.0 * fract(vec3((p3.x + p3.y)*p3.z, (p3.x + p3.z)*p3.y, (p3.y + p3.z)*p3.x));
}

float simplexNoise(vec3 p)
{
	const float K1 = 0.333333333;
	const float K2 = 0.166666667;

	vec3 i = floor(p + (p.x + p.y + p.z) * K1);
	vec3 d0 = p - (i - (i.x + i.y + i.z) * K2);

	vec3 e = step(0.0, d0 - d0.yzx);
	vec3 i1 = e * (1.0 - e.zxy);
	vec3 i2 = 1.0 - e.zxy * (1.0 - e);

	vec3 d1 = d0 - (i1 - 1.0 * K2);
	vec3 d2 = d0 - (i2 - 2.0 * K2);
	vec3 d3 = d0 - (1.0 - 3.0 * K2);

	vec4 h = max(0.6 - vec4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
	vec4 n = h * h * h * h * vec4(dot(d0, hash33(i)), dot(d1, hash33(i + i1)), dot(d2, hash33(i + i2)), dot(d3, hash33(i + 1.0)));

	return dot(31.316, n);
}

float fBm(in vec3 p, float scale, float scale_mult)
{
	//p += vec2(sin(iTime * .7), cos(iTime * .45))*(.1) + iMouse.xy*.1/iResolution.xy;
	float f = 0.0;
	// Change starting scale to any integer value...
	//float scale = 13.0; // Original
	p = mod(p, scale);
	float amp = 0.75;

	for (int i = 0; i < 5; i++)
	{
		f += simplexNoise(p * scale) * amp;
		amp *= 0.5;
		// Scale must be multiplied by an integer value...
		//scale *= 2.0;
		scale *= scale_mult;
	}
	// Clamp it just in case....
	return min(f, 1.0);
}

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float4 color  : COLOR0;
	float2 uv     : TEXCOORD0;
	float4 pos3D  : COLOR1;
};

struct PixelShaderOutput
{
	float4 color    : SV_TARGET0;
};

PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	vec4 fragColor = vec4(0.0, 0.0, 0.0, 1);
	vec2 fragCoord = input.uv * iResolution.xy;	
	vec4 col = vec4(0, 0, 0, 1), col2 = 0, col3 = 0;
	//output.bloom = 0;

	vec2 p = (2.0 * fragCoord.xy - iResolution.xy) / min(iResolution.y, iResolution.x);
	p *= preserveAspectRatioComp;
	p += vec2(0, y_center); // In XWA the aiming HUD is not at the screen's center

	// Early exit: avoid rendering outside the original viewport edges
	//if (input.uv.x < x0 || input.uv.x > x1 ||
	//	input.uv.y < y0 || input.uv.y > y1)
	if (any(input.uv < p0) ||
		any(input.uv > p1))
	{
		output.color = 0.0;
		return output;
	}

	//float4 fgcol = fgColorTex.Sample(fgColorSampler, input.uv);
	float t = mod(iTime, t2) / t2; // Normalized time

	vec3 v = vec3(p, -FOVscale);
	v = mul(viewMat, vec4(v, 0.0)).xyz;
	float v_xy = length(v.xy);
	float z = v.z / v_xy;

	// The following worked just fine to sample the texture cube:
	//output.color = skybox.Sample(samplerColor, v);
	//return output;

	// The focal_depth controls how "deep" the tunnel looks. Lower values
	// provide more depth.
	float focal_depth  = mix(0.15, 0.015, smoothstep(0.65, 0.9, t));
	float focal_depth2 = 0.125;
	float focal_depth3 = 0.015;
	const float streak_freq = mix(30.0, 60.0, smoothstep(0.65, 0.9, t));

	vec2 polar, polar2, polar3;
	//float p_len = length(p);
	float p_len = length(v.xy);
	float a = atan2(v.y, v.x);
	a -= iTime * rotation_speed;
	float x = fract(a / TAU);

	//polar.y = focal_depth / p_len + iTime * speed;
	polar.y = z * focal_depth + iTime * tunnel_speed;
	polar.x = x + twirl * polar.y / TAU * 1.25; // Original period: 4
	polar *= vec2(1.0, 0.2);
	float3 xyt = vec3(polar, 0.15 * iTime /* * forward_speed */);

	polar2.y = z * focal_depth2 + iTime * tunnel_speed;
	polar2.x = polar.x;
	polar2 *= vec2(1.0, 0.2);
	float3 xyt2 = vec3(polar2, 0.15 * iTime /* * forward_speed */);

	polar3.y = z * focal_depth3 + iTime * tunnel_speed;
	polar3.x = x + polar.y / TAU * 1.25; // Original period: 4
	polar3 *= vec2(1.0, 0.2);
	float3 xyt3 = vec3(polar3, 0.15 * iTime /* * forward_speed */);

	// Blend two periods of noise together to eliminate the radial seam
	float val = mix(fBm(xyt + vec3(1.0, 0.0, 0.0), 13.0, 2.0),
					fBm(xyt, 13.0, 2.0),
					smoothstep(0.0, 1.0, x));
	val = saturate(0.45 + 0.55 * val);
	// Colorize blue
	col.rgb = 1.25 * blue_color * val;

	// Calc the white spots
	float white_spot = 0.65 * smoothstep(0.55, 1.0, val);

	// Render the interdiction
	if (Style == 2) {
		float val2, val3, val4;
		float white_spot3;
		const float t2 = smoothstep(0, 1, min(1.0, t * 1.5));

		val2 = mix(fBm(xyt2 + vec3(1.0, 0.0, 0.0), 6.0, 2.0),
				   fBm(xyt2, 6.0, 2.0),
				   smoothstep(0.0, 1.0, x));
		val3 = mix(fBm(xyt3 + vec3(1.0, 0.0, 0.0), streak_freq, 1.0),
				   fBm(xyt3, streak_freq, 1.0),
				   smoothstep(0.0, 1.0, x));

		val2 = saturate(0.45 + 0.55 * val2);
		val3 = saturate(0.45 + 0.55 * val3);

		val2 *= val2 * val2;
		val3 = pow(abs(val3), 10.0);

		val2 = saturate(val2);
		val3 = saturate(val3);

		// Colorize red and yellow
		col2.rgb = 1.20 * red_color * val2;
		col3.rgb = 1.25 * yellow_color * val3;

		// Mix red and yellow with the regular tunnel
		col.rgb  = lerp(col.rgb, col2.rgb, t2);
		col.rgb += lerp(col.rgb, col3.rgb, t2);

		col.rgb = saturate(col.rgb);

		white_spot3 = 0.55 * smoothstep(0.55, 1.0, val3);
		white_spot = lerp(white_spot, white_spot3, t2);
		//white_spot = white_spot3;
	}

	// Add the white spot
	col.rgb += white_spot;

	//output.bloom = white_spot;
	//output.bloom.rgb *= 2.5;

	float w_total = 0.0, w_out = 0.0, w_in;
	// Fade in and out from white every t seconds
	w_in = abs(1.0 - 1.0 * smoothstep(0.0, 0.25, t));
	w_out = abs(1.0 * smoothstep(0.8, 1.0, t));
	w_total = max(w_in, w_out);

	// Add the white disk at the center
	float disk_size = max(0.025, 1.5 * w_out);
	float disk_col = exp(-(p_len - disk_size) * 4.0);
	col.rgb = clamp(col.rgb + disk_col, 0.0, 1.0);

	// Whiteout
	col.rgb = mix(col.rgb, 1.0, w_total);

	// Mask the bloom
	//output.bloom *= (1.0 - fgcol.a);

	fragColor = vec4(col.rgb, 1);
	output.color = fragColor;
	return output;
}
