//static const float weight[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216}; // Original (approx kernel size 9, sigma = 1.75)
//static const float weight[4] = {0.383103	, 0.241843,	0.060626, 0.00598}; // Kernel size 7, sigma = 1.0
static const float weight[3] = {0.38774, 0.24477, 0.06136}; // Kernel size 5, sigma = 1.0
//static const float weight[2] = {0.44198, 0.27901}; // Kernel size 3, sigma = 1.0


//static const float weight[5]   = {0.382928, 0.241732,  0.060598,  0.005977, 0.000229}; // Sigma 1, kernel size 9

//static const float weight[5] = { 0.170793, 0.157829, 0.124548, 0.08393, 0.048297 }; // Sigma 2.5, kernel size 9
// 0.20236	0.179044	0.124009	0.067234	0.028532 // Sigma 2, kernel size 9

// This works fine; but looks square on 1 pass (looks fine on 12 passes):
// Sigma 5, kernel size 9
//static const float weight[5] = { 0.126061, 0.123573, 0.1164, 0.105358, 0.091637 };

//static const float weight[6] = {0.382925, 0.24173,   0.060598,  0.005977, 0.000229, 0.000003};

static float BLOOM_INTENSITY      =  1.2; // Original value: 1.2
static float BLOOM_CURVE          = 10.0; // Original value: 1.5
static float BLOOM_SAT            = 16.0; // Original value: 2.0
static float BLOOM_LAYER_MULT_1   =  0.05;
static float BLOOM_LAYER_MULT_2   =  0.05;
static float BLOOM_LAYER_MULT_3   =  0.05;
static float BLOOM_LAYER_MULT_4   =  0.1;
static float BLOOM_LAYER_MULT_5   =  0.5;
static float BLOOM_LAYER_MULT_6   =  0.01;
static float BLOOM_LAYER_MULT_7   =  0.01;
static float BLOOM_ADAPT_STRENGTH =  0.5;
static float BLOOM_ADAPT_EXPOSURE =  0.0;
static float BLOOM_ADAPT_SPEED    =  2.0;
static float BLOOM_TONEMAP_COMPRESSION = 4.0; // Original value was 4.0

/*
float4 downsample(Texture2D tex, SamplerState s, float2 tex_size, float2 uv)
{
	float2 offset_uv = 0;
	float2 kernel_small_offsets = float2(2.0, 2.0) / tex_size;
	float4 kernel_center = tex.SampleLevel(s, uv, 0);
	float4 kernel_small = 0;

	offset_uv.xy  = uv + kernel_small_offsets;
	kernel_small += tex.SampleLevel(s, offset_uv, 0); //++
	offset_uv.x   = uv.x - kernel_small_offsets.x;
	kernel_small += tex.SampleLevel(s, offset_uv, 0); //-+
	offset_uv.y   = uv.y - kernel_small_offsets.y;
	kernel_small += tex.SampleLevel(s, offset_uv, 0); //--
	offset_uv.x   = uv.x + kernel_small_offsets.x;
	kernel_small += tex.SampleLevel(s, offset_uv, 0); //+-

	return (kernel_center + kernel_small) / 5.0;
}

float3 upsample(Texture2D tex, SamplerState s, float2 texel_size, float2 uv)
{
	float2 offset_uv = 0;
	float4 kernel_small_offsets;
	kernel_small_offsets.xy = 1.5 * texel_size;
	kernel_small_offsets.zw = kernel_small_offsets.xy * 2;

	float3 kernel_center = tex.SampleLevel(s, uv, 0).rgb;
	float3 kernel_small_1 = 0;

	offset_uv.xy    = uv.xy - kernel_small_offsets.xy;
	kernel_small_1 += tex.SampleLevel(s, offset_uv, 0).rgb; //--
	offset_uv.x    += kernel_small_offsets.z;
	kernel_small_1 += tex.SampleLevel(s, offset_uv, 0).rgb; //+-
	offset_uv.y    += kernel_small_offsets.w;
	kernel_small_1 += tex.SampleLevel(s, offset_uv, 0).rgb; //++
	offset_uv.x    -= kernel_small_offsets.z;
	kernel_small_1 += tex.SampleLevel(s, offset_uv, 0).rgb; //-+

	return (kernel_center + kernel_small_1) / 5.0;
}
*/