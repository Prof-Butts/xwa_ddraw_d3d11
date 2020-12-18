// "Volumetric explosion" by Duke
// https://www.shadertoy.com/view/lsySzd
//-------------------------------------------------------------------------------------
// Based on "Supernova remnant" (https://www.shadertoy.com/view/MdKXzc) 
// and other previous shaders 
// otaviogood's "Alien Beacon" (https://www.shadertoy.com/view/ld2SzK)
// and Shane's "Cheap Cloud Flythrough" (https://www.shadertoy.com/view/Xsc3R4) shaders
// Some ideas came from other shaders from this wonderful site
// Press 1-2-3 to zoom in and zoom out.
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
//-------------------------------------------------------------------------------------
// Adapted to XWA by Leo Reyes, 2020
#include "shader_common.h"
#include "HSV.h"
#include "shading_system.h"
#include "PixelShaderTextureCommon.h"

#include "ShaderToyDefs.h"
#include "ShadertoyCBuffer.h"

#define pi 3.14159265
// 0: Original texture, 1: Blend with procedural explosion, 2: Use procedural explosions only
// 0 won't even activate this shader at all.
#define ExplosionBlendMode bDisneyStyle
#define ExplosionScale twirl // Controls the scale of the explosion inside the rendered polygon
#define ExplosionTime tunnel_speed // Controls variable n in SpiralNoiseC

Texture2D    texture0 : register(t0);
SamplerState sampler0 : register(s0);

// Gray noise texture
Texture2D	 noiseTex  : register(t1);
SamplerState noiseSamp : register(s1);


// pos3D/Depth buffer has the following coords:
// X+: Right
// Y+: Up
// Z+: Away from the camera
// (0,0,0) is the camera center, (0,0,Z) is the center of the screen

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float4 color  : COLOR0;
	float2 tex    : TEXCOORD0;
	float4 pos3D  : COLOR1;
	float4 normal : NORMAL;
};

struct PixelShaderOutput
{
	float4 color    : SV_TARGET0;
	float4 bloom    : SV_TARGET1;
	float4 pos3D    : SV_TARGET2;
	float4 normal   : SV_TARGET3;
	float4 ssaoMask : SV_TARGET4;
	float4 ssMask   : SV_TARGET5;
};

// iq's noise -- uses channel 0
float noise(in vec3 x)
{
	vec3 p = floor(x);
	vec3 f = fract(x);
	f = f * f*(3.0 - 2.0*f);
	vec2 uv = (p.xy + vec2(37.0, 17.0)*p.z) + f.xy;
	vec2 rg = noiseTex.SampleLevel(noiseSamp, (uv + 0.5) / 256.0, 0.0).rr;
	return 1.0 - 0.82*mix(rg.x, rg.y, f.z);
}

float fbm(vec3 p)
{
	return noise(p*.06125)*.5 +
		noise(p*.125)*.25 +
		noise(p*.25)*.125 +
		noise(p*.4)*.2;
}

float Sphere(vec3 p, float r)
{
	return length(p) - r;
}

//==============================================================
// otaviogood's noise from https://www.shadertoy.com/view/ld2SzK
//--------------------------------------------------------------
// This spiral noise works by successively adding and rotating sin waves while increasing frequency.
// It should work the same on all computers since it's not based on a hash function like some other noises.
// It can be much faster than other noise functions if you're ok with some repetition.
static float nudge = 4.0;	// size of perpendicular vector
static float normalizer = 1.0 / sqrt(1.0 + nudge * nudge);	// pythagorean theorem on that perpendicular to maintain scale
float SpiralNoiseC(vec3 p, float n)
{
	//float n = -mod(iTime * 0.2,-2.); // noise amount
	//float n = abs(sin(iTime));
	float iter = 2.0;
	// Variable n controls the evolution of the explosion
	//float n = 3.0 * sin(iTime * 2.0);
	// n = 4.0 -- Very faint
	// n = 3.0 -- Faint
	// n = 2.5 -- Bright, focal, expl starting
	// n = 2.0 -- Bright, bigger
	// n = 1.0 -- Full expanding expl
	// Enable the following line to see one expanding explosion that repeats
	//float n = 4.0 - mod(iTime, 5.0);
	// The line above makes n take the range 4..-1

	// The following lines show continuous on-going explosions:
	//float n = 1.0; // Big explosion <-- Use this for the reactor core explosion
	//float n = 2.25; // Small explosions

	for (int i = 0; i < 8; i++)
	{
		// add sin and cos scaled inverse with the frequency
		n += -abs(sin(p.y*iter) + cos(p.x*iter)) / iter;	// abs for a ridged look
		// rotate by adding perpendicular and scaling down
		p.xy += vec2(p.y, -p.x) * nudge;
		p.xy *= normalizer;
		// rotate on other axis
		p.xz += vec2(p.z, -p.x) * nudge;
		p.xz *= normalizer;
		// increase the frequency
		iter *= 1.733733;
		//iter *= 1.83;
	}
	return n;
}

float VolumetricExplosion(vec3 p)
{
	float final = Sphere(p, 4.0);
	final += noise(p * 12.5) * 0.2;

	// Add a displacement on the Z-axis to keep the explosion "moving":
	final += SpiralNoiseC(p.zxy * 0.4132 + 333.0 + vec3(1.5 * iTime, 0, 0), ExplosionTime) * 3.0;
	return final;
}

float map(vec3 p)
{
	// This is where we can rotate p to look at the explosion from other angles:
	//R(p.xz, iMouse.x*0.008*pi+iTime*0.1);

	//return VolumetricExplosion(p / 0.5) * 0.5; // scale
	return VolumetricExplosion(p * ExplosionScale) * 0.5;
}
//--------------------------------------------------------------

// assign color to the media
vec3 computeColor(float density, float radius)
{
	// color based on density alone, gives impression of occlusion within
	// the media
	vec3 result = mix(vec3(1.0, 0.9, 0.8), vec3(0.4, 0.15, 0.1), density);

	// color added to the media
	vec3 colCenter = 7.0*vec3(0.8, 1.0, 1.0);
	vec3 colEdge = 1.5*vec3(0.48, 0.53, 0.5);
	result *= mix(colCenter, colEdge, min((radius + .05) / .9, 1.15));

	return result;
}

bool RaySphereIntersect(vec3 org, vec3 dir, out float near, out float far)
{
	float b = dot(dir, org);
	float c = dot(org, org) - 8.0;
	float delta = b * b - c;
	if (delta < 0.0)
		return false;
	float deltasqrt = sqrt(delta);
	near = -b - deltasqrt;
	far = -b + deltasqrt;
	return far > 0.0;
}


PixelShaderOutput main(PixelShaderInput input)
{
	PixelShaderOutput output;
	float4 texelColor = texture0.Sample(sampler0, input.tex);
	float  alpha = 0;
	//float  value = dot(0.333, texelColor.rgb);

	vec2 uv = input.tex.xy;
	// ro: ray origin
	// rd: direction of the ray
	vec3 rd = normalize(vec3(uv - 0.5, 1.0));
	vec3 ro = vec3(0., 0., -6.0);

	// ld, td: local, total density 
	// w: weighting factor
	float ld = 0.0, td = 0.0, w = 0.0;

	// t: length of the ray
	// d: distance function
	float d = 1.0, t = 0.0;
	const float h = 0.1;
	vec4 sum = 0.0;
	float min_dist = 0.0, max_dist = 0.0;

	if (RaySphereIntersect(ro, rd, min_dist, max_dist)) {
		t = min_dist * step(t, min_dist);

		// raymarch loop
		for (int i = 0; i < 60; i++) {
			vec3 pos = ro + t * rd;

			// Loop break conditions.
			if (td > 0.9 || d < 0.12 * t || t > 10.0 || sum.a > 0.99 || t > max_dist)
				break;

			// evaluate distance function
			float d = map(pos);
			d = abs(d) + 0.07;
			// change this string to control density 
			d = max(d, 0.03);

			// point light calculations
			//vec3 ldst = 0.0 - pos;
			vec3 ldst = -pos;
			float lDist = max(length(ldst), 0.001);

			// the color of light 
			vec3 lightColor = vec3(1.0, 0.5, 0.25);

			sum.rgb += (lightColor / exp(lDist * lDist * lDist * 0.08) / 30.0); // bloom

			if (d < h) {
				// compute local density 
				ld = h - d;

				// compute weighting factor 
				w = (1. - td) * ld;

				// accumulate density
				td += w + 1. / 200.;

				vec4 col = vec4(computeColor(td, lDist), td);

				// emission
				sum += sum.a * vec4(sum.rgb, 0.0) * 0.2 / lDist;

				// uniform scale density
				col.a *= 0.2;
				// colour by alpha
				col.rgb *= col.a;
				// alpha blend in contribution
				sum = sum + col * (1.0 - sum.a);
			}

			td += 1.0 / 70.0;

			// try to optimize step size
			t += max(d*0.25, 0.01);
		}

		// simple scattering
		sum *= 1.0 / exp(ld * 0.2) * 0.8;

		sum = clamp(sum, 0.0, 1.0);
		sum.xyz = sum.xyz*sum.xyz*(3.0 - 2.0 * sum.xyz);
	}

	alpha = 2.0 * dot(0.333, sum.xyz);
	output.color = vec4(sum.xyz, alpha);
	output.bloom = fBloomStrength * alpha * output.color;

	// Mix the procedural explosion with the original texture:
	// 0: Original texture, 1: Blend with procedural explosion, 2: Use procedural explosions only
	if (ExplosionBlendMode == 1) {
		// Compute the original texture color and bloom:
		// Disable depth-buffer write for engine glow textures
		float4 orig_color = texelColor;
		if (bIsEngineGlow > 1) {
			// Enhance the glow in 32-bit mode
			float3 HSV = RGBtoHSV(orig_color.rgb);
			HSV.y *= 1.25;
			HSV.z *= 1.25;
			orig_color.rgb = HSVtoRGB(HSV);
		}
		/*
		// This game uses white for the fully-transparent areas of an explosion. We normally
		// don't see this because the alpha inhibits these areas. Enable the following
		// code to see how textures look:
		output.color = orig_color;
		output.color.a = 1.0;
		output.bloom = 0;
		*/

		// Because of the above, we need to premultiply the color info with the alpha so
		// that regular blending modes work well:
		orig_color.rgb = orig_color.a * orig_color.rgb;

		// Mix the original color/bloom with their procedural versions.

		// The following selects the image with the highest alpha value
		//if (orig_color.a > output.color.a) output.color = orig_color;

		// Screen blend mode:
		//output.color = 1.0 - (1.0 - orig_color) * (1.0 - output.color);

		// Direct addition blending mode:
		output.color = saturate(output.color + orig_color);

		// Multiplicative blending mode:
		//output.color = saturate(output.color * orig_color);

		// Common code, regardless of blending mode:
		output.bloom = float4(fBloomStrength * output.color.rgb, output.color.a);
		alpha = output.color.a;
	}
	output.ssaoMask = float4(0.0, 0.0, 0.0, alpha);
	output.ssMask = float4(0.0, 0.0, 1.0, alpha);
	output.normal = 0;
	output.pos3D = 0;
	return output;
}
