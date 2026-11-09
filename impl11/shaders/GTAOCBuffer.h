#ifndef GTAO_CBUFFER_H
#define GTAO_CBUFFER_H

// GTAOCBuffer: the settings of the GTAO and GTAO denoise passes. See GTAOCommon.h.
cbuffer ConstantBuffer : register(b12)
{
	float2 gtao_buffer_size;    // Size of the GTAO buffers, in pixels
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
	float gtao_unused0, gtao_unused1;
	// 48 bytes
};

#endif
