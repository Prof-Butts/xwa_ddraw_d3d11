/*
 * GGX prefiltering of a cubemap, for the reflections of the new PBR shading
 * (pbr_cubemap_prefilter). It runs once when a cubemap is loaded; see PrefilterCubeMap() in
 * XwaD3dRendererHook.cpp. Each dispatch writes one mip of the prefiltered cubemap (all six
 * faces): mip 0 is the sky itself, and each following mip is the sky as reflected by a
 * rougher surface, up to roughness 1 at the last mip. The shader picks the mip from the
 * roughness (PBRAddV2Common.h).
 *
 * The method is the split-sum prefilter of UE4 (Karis, "Real Shading in Unreal Engine 4",
 * 2013): GGX importance sampling, assuming the view direction is the normal (N = V = R),
 * weighted by N.L. To avoid noise with few samples, each sample reads the source mip
 * whose texels cover the sample's solid angle ("filtered importance sampling", Krivanek &
 * Colbert, "Real-time Shading with Filtered Importance Sampling", 2008).
 *
 * The output is linear, in 16-bit float. 8-bit sources are in sRGB and are converted. The
 * boost (AllRegionsIllumBoost/RegionNIllumBoost in the mission .ini) multiplies the
 * brightest parts of 8-bit skies, which are clipped at 1, so they reflect with more
 * strength, as they would from an HDR sky.
 */
TextureCube srcCube : register(t0);
SamplerState linearSampler : register(s0);
RWTexture2DArray<float4> dstMip : register(u0);

cbuffer PrefilterCBuffer : register(b0)
{
	float roughness;     // Perceptual roughness of this mip
	float srcFaceSize;   // Size of a face of the source's mip 0, in pixels
	float dstFaceSize;   // Size of a face of this mip, in pixels
	float srcMaxMip;     // Last mip of the source
	// 16 bytes
	float boost;         // The brightest pixels are multiplied by this. 1: off.
	float srcIsSRGB;     // 1: the source is 8-bit sRGB, decode it to linear
	float sampleCount;   // GGX samples per pixel (when roughness > 0)
	float unused;
	// 32 bytes
};

static const float PI = 3.14159265359;

float3 SRGBToLinear(float3 c)
{
	return c <= 0.04045 ? c / 12.92 : pow(abs((c + 0.055) / 1.055), 2.4);
}

// The light of the source in direction dir, from mip level lod: linear, with the boost
float3 SampleSky(float3 dir, float lod)
{
	const float3 raw = srcCube.SampleLevel(linearSampler, dir, lod).rgb;
	float3 color = raw;
	float value = max(raw.r, max(raw.g, raw.b));
	if (srcIsSRGB > 0.5)
		color = SRGBToLinear(raw);
	else
		value = pow(saturate(value), 1.0 / 2.2); // The boost is defined on the 8-bit values
	// Only the bright parts are boosted: nothing below 60% of full brightness, and the full
	// boost at 100% (where 8-bit skies are clipped)
	const float t = smoothstep(0.6, 1.0, value);
	return color * lerp(1.0, boost, t * t);
}

// The direction of texel (u, v) of a face, u and v in -1..1, in the D3D cubemap convention
// (faces +X, -X, +Y, -Y, +Z, -Z). The same as ComputeCubeMapSH() in ddraw.
float3 FaceDirection(uint face, float u, float v)
{
	switch (face)
	{
	case 0:  return float3( 1, -v, -u);
	case 1:  return float3(-1, -v,  u);
	case 2:  return float3( u,  1,  v);
	case 3:  return float3( u, -1, -v);
	case 4:  return float3( u, -v,  1);
	default: return float3(-u, -v, -1);
	}
}

float2 Hammersley(uint i, uint count)
{
	return float2((float)i / (float)count, reversebits(i) * 2.3283064365386963e-10);
}

// A half vector distributed like the GGX lobe of the given alpha, around N
float3 ImportanceSampleGGX(float2 xi, float alpha, float3 N)
{
	const float phi = 2.0 * PI * xi.x;
	const float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y));
	const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	const float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
	const float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	const float3 tangentX = normalize(cross(up, N));
	const float3 tangentY = cross(N, tangentX);
	return tangentX * H.x + tangentY * H.y + N * H.z;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	const uint size = (uint)dstFaceSize;
	if (id.x >= size || id.y >= size)
		return;

	const float u = 2.0 * (id.x + 0.5) / dstFaceSize - 1.0;
	const float v = 2.0 * (id.y + 0.5) / dstFaceSize - 1.0;
	const float3 N = normalize(FaceDirection(id.z, u, v));

	float3 color;
	if (roughness <= 0.0)
	{
		// Mip 0: the sky itself, from the source mip with the same texel size
		color = SampleSky(N, clamp(log2(srcFaceSize / dstFaceSize), 0.0, srcMaxMip));
	}
	else
	{
		const float alpha = roughness * roughness;
		const float alpha2 = alpha * alpha;
		// Solid angle of a texel of the source's mip 0
		const float texelSolidAngle = 4.0 * PI / (6.0 * srcFaceSize * srcFaceSize);
		const uint count = (uint)sampleCount;
		float3 sum = 0;
		float weightSum = 0;
		for (uint i = 0; i < count; i++)
		{
			const float3 H = ImportanceSampleGGX(Hammersley(i, count), alpha, N);
			const float NdotH = saturate(dot(N, H));
			const float3 L = 2.0 * NdotH * H - N;
			const float NdotL = dot(N, L);
			if (NdotL > 0.0)
			{
				// pdf of L: D * NdotH / (4 * VdotH), with V = N
				const float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
				const float D = alpha2 / (PI * denom * denom);
				const float pdf = max(D * 0.25, 1e-6);
				const float sampleSolidAngle = 1.0 / (count * pdf);
				const float lod = clamp(0.5 * log2(sampleSolidAngle / texelSolidAngle) + 1.0, 0.0, srcMaxMip);
				sum += SampleSky(L, lod) * NdotL;
				weightSum += NdotL;
			}
		}
		color = sum / max(weightSum, 1e-6);
	}
	dstMip[id] = float4(color, 1.0);
}
