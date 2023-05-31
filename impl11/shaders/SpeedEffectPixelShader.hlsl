/*
 * Hyperspace jump trails.
 *
 * You can use it under the terms of the MIT license, see LICENSE.TXT
 * (free to use even in commercial projects, attribution required)
 *
 * Adapted from 	https://www.shadertoy.com/view/MlKBWw by Leo Reyes
 * Shadertoy implementation: https://www.shadertoy.com/view/3l3Gz8
 *
 * Lens flare from: https://www.shadertoy.com/view/XdfXRX
 * and: https://www.shadertoy.com/view/4sX3Rs
 * (c) Leo Reyes, 2019.
 */

#include "ShaderToyDefs.h"
#include "ShadertoyCBuffer.h"

 // The way this shader works is by looking at the screen as if it were a disk and then
 // this disk is split into a number of slices centered at the origin. Each slice renders
 // a single trail. So this setting controls the overall density of the effect:
//#define NUM_SLICES 125.0

// Each trail is rendered within its slice; but to avoid generating regular patterns, we
// randomly offset the trail from the center of the slice by this amount:
//#define MAX_SLICE_OFFSET 0.4

//static const vec3 white_col = vec3(0.9, 0.9, 0.9);

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float4 color  : COLOR0;
	float2 uv     : TEXCOORD0;
	//float4 pos3D  : COLOR1; //not used anymore and causing a DEVICE_SHADER_LINKAGE_SEMANTICNAME_NOT_FOUND
};

struct PixelShaderOutput
{
	float4 color    : SV_TARGET0;
};

/*
float sdLine(in vec2 p, in vec2 a, in vec2 b, in float ring)
{
	vec2 pa = p - a, ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
	return length(pa - ba * h) - ring;
}

float rand(vec2 co) {
	return fract(sin(dot(co.xy + srand, vec2(12.9898, 78.233))) * 43758.5453);
}

PixelShaderOutput main_old(PixelShaderInput input) {
	PixelShaderOutput output;
	vec4 fragColor = vec4(0.0, 0.0, 0.0, 1);
	vec2 fragCoord = input.uv * iResolution.xy;
	vec3 color = 0.0;
	output.color = 0.0;

	// DEBUG
	//output.color = bgColorTex.Sample(bgColorSampler, input.uv);
	//output.color = float4(input.uv, 0.0, 1.0);
	//return output;
	// DEBUG

	// Early exit: avoid rendering outside the original viewport edges
	//if (input.uv.x < x0 || input.uv.x > x1 ||
	//	input.uv.y < y0 || input.uv.y > y1)
	if (any(input.uv < p0) || any(input.uv > p1))
	{
		output.color = 0.0;
		return output;
	}

	float t = iTime;
	vec2 p;
	p = (2.0 * fragCoord.xy - iResolution.xy) / min(iResolution.x, iResolution.y);
	p += vec2(0, y_center); // In XWA the aiming HUD is not at the screen's center

	float p_len = length(p);
	vec3 v = vec3(p, -FOVscale);
	v = mul(viewMat, vec4(v, 0.0)).xyz;

	float trail_start, trail_end;

	float fade = smoothstep(0.2, 3.0, craft_speed);
	// Each slice renders a single trail; but we can render multiple layers of
	// slices to add more density and randomness to the effect:
	for (float i = 0.0; i < 20.0; i++)
	{
		vec3 trail_color = 0.0;
		float angle = atan2(v.y, v.x) / 3.141592 / 2.0 + 0.13 * i;

		float slice = floor(angle * NUM_SLICES);
		float slice_fract = fract(angle * NUM_SLICES);
		// Don't center the trail in the slice: wiggle it a little bit:
		float slice_offset = MAX_SLICE_OFFSET *
			rand(vec2(slice, 4.0 + i * 25.0)) - (MAX_SLICE_OFFSET / 2.0);

		// Without dist, all trails get stuck to the walls of the
		// tunnel.
		float dist = 2.0 * rand(vec2(slice, 1.0 + i * 10.0)) - 1.0;
		float z = dist * v.z / length(v.xy);

		// When dist is negative we have to invert a number of things:
		float f = sign(dist);
		if (f == 0.0) f = 1.0;

		trail_start = 10.0 * rand(vec2(slice, 0.0 + i * 10.0)) - 5.0;
		trail_start -= craft_speed * t * f;
		// Repeat trails over and over again:
		trail_start = mod(trail_start, 5.0) - 2.5;
		trail_end = trail_start - 0.01 * craft_speed;

		//float val = 0.0;
		//float trail_x = smoothstep(trail_start, trail_end, p_len);
		float trail_x = smoothstep(trail_start, trail_end, z);
		trail_color = white_col;

		// This line computes the distance from the current pixel, in "slice-coordinates"
		// to the ideal trail centered at the slice center. The last argument makes the lines
		// a bit thicker when they reach the edges as time progresses.
		float h = sdLine(
			vec2(slice_fract + slice_offset, z),
			vec2(0.5, trail_start),
			vec2(0.5, trail_end),
			0.0);

		//float threshold = mix(0.04, 0.175, smoothstep(0.0, 0.5, t));
		float threshold = 0.09;
		h = (h < 0.01) ? 1.0 : 0.85 * smoothstep(threshold, 0.0, abs(h));

		trail_color *= h;

		// Accumulate this trail with the previous ones
		color = max(color, trail_color);
	}

	output.color = vec4(fade * color, 1.0);
	return output;
}
*/

PixelShaderOutput main(PixelShaderInput input) {
	PixelShaderOutput output;
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