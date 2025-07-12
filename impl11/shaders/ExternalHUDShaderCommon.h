/*
 * External Aiming Reticle Shader.
 *
 * You can use it under the terms of the MIT license, see LICENSE.TXT
 * (free to use even in commercial projects, attribution required)
 *
 * (c) Leo Reyes, 2020.
 */

#include "ShaderToyDefs.h"
#include "shading_system.h"
#include "ShadertoyCBuffer.h"
#include "metric_common.h"

#ifdef INSTANCED_RENDERING
 // The reticle background texture
Texture2DArray bgTex : register(t0);
SamplerState   bgSampler : register(s0);

Texture2DArray stellarBG : register(t22);
#else
 // The reticle background texture
Texture2D    bgTex : register(t0);
SamplerState bgSampler : register(s0);

Texture2D    stellarBG : register(t22);
#endif

// The reticle texture
Texture2D    reticleTex : register(t1);
SamplerState reticleSampler : register(s1);

TextureCube skybox : register(t21);

#define cursor_radius 0.04
//#define thickness 0.02 
//#define scale 2.0
#define thickness 0.007
#define scale 1.25

struct PixelShaderInput
{
	float4 pos    : SV_POSITION;
	float4 color  : COLOR0;
	float2 uv     : TEXCOORD0;
	float4 pos3D  : COLOR1;
	float4 normal : NORMAL; // hook_normals.dll populates this field
#ifdef INSTANCED_RENDERING
	uint   viewId : SV_RenderTargetArrayIndex;
#endif
};

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

// Renders DefaultStarfield.dds
// This function blends DefaultStarfield.dds with the existing planet backdrops by using
// both the alpha from the backdrops and the lightness from DefaultStarfield.dds
PixelShaderOutput RenderDefaultStarfield(PixelShaderInput input)
{
	PixelShaderOutput output;
	output.color = 0;

	// The code here comes from the HyperEntry shader:
	float2 fragCoord = input.uv * iResolution.xy;
	float2 p = (2.0 * fragCoord.xy - iResolution.xy) / min(iResolution.x, iResolution.y);
	p *= preserveAspectRatioComp;
	p += float2(0, y_center); // In XWA the aiming HUD is not at the screen's center

	// But the code here comes from PixelShaderVRGeom, specifically from the area where
	// the skybox is rendered:
	float3 V = normalize(float3(p, -FOVscale));
	// We invert V.zy here to match the direction vector used in PixelShaderVRGeom:
	V.yz = -V.yz;
	// That way, viewMat below is the same we already figured out for PixelShaderVRGeom:
	V = mul(viewMat, float4(V, 0)).xyz;

	const float3 skyBoxColor = skybox.Sample(bgSampler, V.xyz).rgb;
	const float  skyBoxVal   = dot(0.333, skyBoxColor);

#ifndef INSTANCED_RENDERING
	// Blend the skybox with the previous background:
	// DefaultStarfield.dds is rendered *after* the backdrops have already been rendered.
	// Sometimes, the backdrops (nebulae, planets) are semi-transparent. This wasn't a problem
	// before because the original XWA starfield was almost completely black with just a few
	// stars, so the semi-transparent backdrops were still visible.
	// Now, if DefaultStarfield.dds is rendered after the semi-transparent backdrops, it will
	// end up covering most of them. To solve this, we're using a custom blending mode here that
	// combines the lightness of the skybox with the inverse alpha of the existing background.
	// Opaque areas in the background are retained, and only light areas in the skybox are added.
	// This custom blending step is performed in HyperspaceCompose for the VR path.
	const float4 background = stellarBG.Sample(bgSampler, input.uv); // This layer contains planets and nebulae
	const float3 bgBlend    = saturate(background.rgb + skyBoxColor.rgb);
	output.color = float4(lerp(background.rgb, bgBlend, skyBoxVal * (1.0 - background.a)), 1);
#else
	// SteamVR mode: Return just the skybox, blending will be done later:
	output.color = float4(skyBoxColor, skyBoxVal);
#endif
	return output;
}

// Renders a custom CubeMap (I can probably merge this with RenderDefaultStarfield() later
PixelShaderOutput RenderCubeMap(PixelShaderInput input)
{
	PixelShaderOutput output;
	output.color = 0;

	// The code here comes from the HyperEntry shader:
	float2 fragCoord = input.uv * iResolution.xy;
	float2 p = (2.0 * fragCoord.xy - iResolution.xy) / min(iResolution.x, iResolution.y);
	p *= preserveAspectRatioComp;
	p += float2(0, y_center); // In XWA the aiming HUD is not at the screen's center

	// But the code here comes from PixelShaderVRGeom, specifically from the area where
	// the skybox is rendered:
	float3 V = normalize(float3(p, -FOVscale));
	// We invert V.zy here to match the direction vector used in PixelShaderVRGeom:
	V.yz = -V.yz;
	// That way, viewMat below is the same we already figured out for PixelShaderVRGeom:
	V = mul(viewMat, float4(V, 0)).xyz;

	const float3 cubeMapColor = skybox.Sample(bgSampler, V.xyz).rgb;

#ifndef INSTANCED_RENDERING
	// Blend the cube map with the previous background:
	// The cube map is rendered *after* the backdrops have already been rendered, so
	// we need to blend the existing planets/nebulae with the cube map.
	const float4 background = stellarBG.Sample(bgSampler, input.uv); // This layer contains planets and nebulae
	output.color = float4(lerp(cubeMapColor.rgb, background.rgb, background.a), 1);
#else
	// SteamVR mode: Return just the skybox, blending will be done later:
	output.color = float4(cubeMapColor, 1);
#endif
	return output;
}

/*
float sdLine(in vec2 p, in vec2 a, in vec2 b, in float ring)
{
	vec2 pa = p - a, ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
	return length(pa - ba * h) - ring;
}
*/

float sdLine(in vec2 p, in vec2 a, in vec2 b)
{
	vec2 pa = p - a, ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
	return length(pa - ba * h);
}

/*
vec3 lensflare(vec3 uv, vec3 pos, float flare_size, float ang_offset)
{
	float z = uv.z / length(uv.xy);
	vec2 main = uv.xy - pos.xy;
	float dist = length(main);
	float num_points = 2.71;
	float disk_size = 0.2;
	float inv_size = 1.0 / flare_size;
	float ang = atan2(main.y, main.x) + ang_offset;
	float fade = (z < 0.0) ? -z : 1.0;

	float f0 = 1.0 / (dist * inv_size + 1.0);
	f0 = f0 + f0 * (0.1 * sin((sin(ang*2.0 + pos.x)*4.0 - cos(ang*3.0 + pos.y)) * num_points) + disk_size);
	if (z < 0.0) // Remove the flare on the back
		return clamp(mix(f0, 0.0, 0.75 * fade), 0.0, 1.0);
	else
		return f0;
}

vec3 cc(vec3 color, float factor, float factor2) // color modifier
{
	float w = color.x + color.y + color.z;
	return mix(color, w * factor, w * factor2);
}
*/

float sdCircle(in vec2 p, in vec2 center, float radius)
{
	return length(p - center) - radius;
}

// From Inigo Quilez's: https://www.shadertoy.com/view/MldcD7
// signed distance to a 2D triangle
float sdTriangleIsosceles(in vec2 p, in vec2 q)
{
	p.x = abs(p.x);
	vec2 a = p - q * clamp(dot(p, q) / dot(q, q), 0.0, 1.0);
	vec2 b = p - q * vec2(clamp(p.x / q.x, 0.0, 1.0), 1.0);
	float k = sign(q.y);
	float d = min(dot(a, a), dot(b, b));
	float s = max(k * (p.x * q.y - p.y * q.x), k * (p.y - q.y));
	return sqrt(d) * sign(s);
}

// Translate and rotate the triangle about p
vec2 tri_transform(vec2 p, float disp, float ang) {
	float3x3 mR = float3x3(
		-sin(ang), cos(ang), 0.0,
		cos(ang), sin(ang), 0.0,
		0.0, 0.0, 1.0);

	vec3 T = mul(mR, vec3(0.0, disp, 1.0));
	vec3 q = mul(mR, vec3(p + T.xy, 1.0));
	return q.xy;
}

inline float sdBox(in vec2 p, in vec2 b)
{
	vec2 d = abs(p) - b;
	return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdCircle(in vec2 p, float radius)
{
	return length(p) - radius;
}

// Display the HUD using a hyperspace-entry-like coord sys 
PixelShaderOutput main(PixelShaderInput input) {
	PixelShaderOutput output;
	vec2 fragCoord = input.uv * iResolution.xy;
	//vec2 texCoord = (input.uv - SunCoords[0].xy) * iResolution.xy;
	output.color = 0.0;

	// DEBUG
	//if (input.uv.x > 0.5)
	//	output.color = float4(0, 1, 0, 1);
	//else
	//	output.color = float4(1, 0, 0, 1);
	//output.color = float4(input.uv, 0.0, 1.0);
	//output.color.b += 0.1;
	//return output;
	// DEBUG

	// Early exit: avoid rendering outside the original viewport edges
	if (any(input.uv < p0) || any(input.uv > p1))
		return output;

	// Special mode: we're rendering the skybox (DefaultStarfield.dds) in this path, not the reticle
	if (VRmode == 3)
	{
		return RenderDefaultStarfield(input);
	}

	if (VRmode == 4)
	{
		return RenderCubeMap(input);
	}

	// In SBS VR mode, each half-screen receives a full 0..1 uv range. So if we sample the
	// texture using input.uv, we'll get one SBS image on the left, and one SBS image on the
	// right. That's 4 images in one screen!
#ifdef INSTANCED_RENDERING
	if (VRmode == 0) output.color = bgTex.Sample(bgSampler, float3(input.uv, input.viewId));
#else
	if (VRmode == 0) output.color = bgTex.Sample(bgSampler, input.uv);
#endif

	vec2 p = (2.0 * fragCoord.xy - iResolution.xy) / min(iResolution.x, iResolution.y);
	p *= preserveAspectRatioComp;
	p += vec2(0, y_center); // In XWA the aiming HUD is not at the screen's center in cockpit view
	vec3 v = vec3(p, -FOVscale);
	v = mul(viewMat, vec4(v, 0.0)).xyz;

	/*
	float d, dm = 0.0;
	//float3 col = float3(0.2, 0.2, 0.8); // Reticle color
	float3 col = float3(0.2, 1.0, 0.2); // Reticle color

	d = sdCircle(v.xy, vec2(0.0, 0.0), scale * cursor_radius);

	dm  = smoothstep(thickness, 0.0, abs(d)); // Outer ring
	dm += smoothstep(thickness, 0.0, abs(d + scale * (cursor_radius - 0.001))); // Center dot

	d = sdLine(v.xy, scale * vec2(-0.05, 0.0), scale * vec2(-0.03, 0.0));
	dm += smoothstep(thickness, 0.0, abs(d));

	d = sdLine(v.xy, scale * vec2(0.05, 0.0), scale * vec2(0.03, 0.0));
	dm += smoothstep(thickness, 0.0, abs(d));

	// Negative Y: down
	d = sdLine(v.xy, scale * vec2(0.0, -0.05), scale * vec2(0.0, -0.03));
	dm += smoothstep(thickness, 0.0, abs(d));

	d = sdLine(v.xy, scale * vec2(0.0, 0.05), scale * vec2(0.0, 0.03));
	dm += smoothstep(thickness, 0.0, abs(d));

	dm = clamp(dm, 0.0, 1.0);
	col *= dm;
	*/

	// This shader is not intended to be run outside VR anymore
	//if (VRmode == 0) {
		//float2 reticleUV = ((input.uv - reticleCentroid.xy) * reticleCentroid.z) + reticleCentroid.xy;
		//output.color.rgb = lerp(output.color.rgb, col, 0.8 * dm);
	//}
	//else { 

		// Render the HUD
	if (SunCoords[0].w > 0.5) {
		float3 reticleCentroid = SunCoords[0].xyz;
		// The following line renders the flat reticle, without distortion due to FOV.
		//float2 reticleUV = ((input.uv - reticleCentroid.xy) * reticleCentroid.z) + reticleCentroid.xy;
		float2 reticleScale = reticleCentroid.z;

		// This path is expected to run only in VR mode, so it's safe to multiply by the VR aspect ratio
		// compensation factor here:
		reticleScale.x *= mr_vr_aspect_ratio;

		// This is the regular DirectSBS path:
		//float2 reticleUV = (v.xy * reticleScale / preserveAspectRatioComp + reticleCentroid.xy);
		// SteamVR: If PreserveAspectRatio = 0, then the following line fixes the HUD:
		//float2 reticleUV = (v.xy * reticleScale + reticleCentroid.xy);
		// SteamVR: If PreserveAspectRatio = 1, then we need to multiply by preserveAspectRatioComp:
		//float2 reticleUV = (v.xy * reticleScale * preserveAspectRatioComp + reticleCentroid.xy);

		// ORIGINAL:
		//float2 reticleUV = (v.xy * reticleScale * mr_vr_aspect_ratio_comp + reticleCentroid.xy);
		float2 reticleUV = (v.xy * reticleScale + reticleCentroid.xy);

		float4 reticle = reticleTex.Sample(reticleSampler, reticleUV);
		float alpha = 3.0 * dot(0.333, reticle);
		// DEBUG
		//output.color = float4(col, 0.8 * dm); // Render the synthetic reticle
		//output.color.rgb = lerp(output.color.rgb, reticle.rgb, alpha);
		//output.color.a = max(output.color.a, alpha);
		// DEBUG
		// Render only the scaled reticle:
		output.color = float4(reticle.rgb, alpha);
	}

	// Add the triangle pointer
	if (SunCoords[1].w > 0.5) {
		//float tri_scale = 0.25 + 0.5 * (mod(iTime, 0.5)) / 0.5;
		float tri_scale = SunCoords[1].z;
		const vec2 tri_size = tri_scale * vec2(0.05, 0.2); // width, height
		//vec2 tri_q = tri_transform(v.xy, tri_size.y + SunCoords[1].y, SunCoords[1].x);
		vec2 tri_q = tri_transform(p, tri_size.y + SunCoords[1].y, SunCoords[1].x);
		// Compute the triangle
		float d_tri = sdTriangleIsosceles(tri_q, tri_size);
		// Antialias:
		d_tri = smoothstep(0.005, 0.0, d_tri);
		// Add it to the output:
		output.color.rgb += float3(1.0, 1.0, 0.0) * d_tri;
		output.color.a = max(output.color.a, d_tri);
	}

	// DEBUG
	/*if (reticleUV.x > reticleCentroid.x)
		output.color.ba += 0.7;
	if (reticleUV.y > reticleCentroid.y)
		output.color.ga += 0.7;*/
		//output.color.rga += float3(v.xy, 0.7);
		// DEBUG
	//}

#ifdef DISPLAY_DEBUG
	// DEBUG
	// Render a square and a circle in the middle of the screen to help calibrate the aspect ratio of the reticle:
	float d, a;
	//p = 2.0 * (input.uv - 0.5);
	//p = float2(1.0, 0.84) * p; // This makes the following look square/round if in-game a/r is 1.333, this is steamVR's a/r
	//p *= preserveAspectRatioComp * float2(1.0, 1.0/0.84);
	//p += float2(0, y_center);
	const float radius = 0.1;
	p = v.xy;
	d = sdBox(p, radius);
	d = smoothstep(0.01, 0.0, abs(d)); // ring
	a = 0.8 * d;
	output.color.rgb = lerp(output.color.rgb, d, a);
	output.color.a = max(output.color.a, a);

	d = sdCircle(p, radius);
	d = smoothstep(0.01, 0.0, abs(d));
	a = 0.8 * d;
	output.color.rgb = lerp(output.color.rgb, d, a);
	output.color.a = max(output.color.a, a);
	// DEBUG
#endif

	return output;
}

/*
// Display the current MainLight, using regular UV post proc coords
PixelShaderOutput main_main_light(PixelShaderInput input) {
	PixelShaderOutput output;
	vec3 color = 0.0;
	output.color = bgTex.Sample(bgSampler, input.uv);

	// Early exit: avoid rendering outside the original viewport edges
	if (any(input.uv < p0) || any(input.uv > p1))
		return output;

	if (MainLight.z < 0.0) // Skip lights behind the camera
		return output;

	float3 col = float3(1.0, 0.1, 0.1); // Marker color
	float d, dm;
	d = sdCircle(input.uv, MainLight.xy, scale * cursor_radius);
	dm = smoothstep(thickness * 0.5, 0.0, abs(d)); // Outer ring
	dm += smoothstep(thickness * 0.5, 0.0, abs(d + 0.5 * scale * (cursor_radius - 0.001))); // Center dot

	dm = clamp(dm, 0.0, 1.0);
	col *= dm;
	output.color.rgb = lerp(output.color.rgb, col, 0.8 * dm);
	return output;
}
*/