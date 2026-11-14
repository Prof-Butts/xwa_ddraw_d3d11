#ifndef GTAO_CBUFFER_H
#define GTAO_CBUFFER_H

// GTAOCBuffer: the settings of the GTAO and GTAO denoise passes. See GTAOCommon.h.
cbuffer ConstantBuffer : register(b12)
{
	float2 gtao_buffer_size;    // Size of the full-resolution buffers, in pixels
	float gtao_radius;          // Radius of the effect, in meters (pbr_gtao_radius)
	float gtao_intensity;       // Power applied to the visibility (pbr_gtao_intensity)
	// 16 bytes
	float gtao_min_radius_px;   // The radius is clamped to this range, in pixels
	float gtao_max_radius_px;
	uint gtao_slices;           // Directions searched per pixel (pbr_gtao_slices)
	uint gtao_steps;            // Samples per direction and side (pbr_gtao_steps)
	// 32 bytes
	float gtao_falloff;         // Fraction of the radius where the occluders fade out
	float gtao_denoise_depth;   // Relative depth difference the denoise filter tolerates
	float gtao_scale;           // 1: full resolution, 2: half resolution (pbr_gtao_half_res)
	float gtao_angle_bias;      // Radians: occluders this close to the tangent plane are ignored
	// 48 bytes
	float gtao_height_bias;     // In 16-bit position steps: bumps this tall are ignored (pbr_gtao_height_bias)
	float gtao_gi_thickness;    // Indirect light: assumed thickness of the surfaces, in meters (pbr_gi_thickness)
	float gtao_bounce_scale;    // Indirect light: the bounce light's resolution, 1 / scale
	float gtao_thickness;       // Bitmask AO: assumed thickness of the occluders, in meters (pbr_gtao_thickness)
	// 64 bytes
};

#endif
