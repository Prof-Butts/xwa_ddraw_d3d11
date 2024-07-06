#ifndef SHADER_COMMON_H
#define SHADER_COMMON_H

/*

PS Constant Buffer Registers

Several buffers         : register(b0)
BloomPixelShaderCBuffer : register(b2)
SSAOPixelShaderCBuffer  : register(b3)
PSShadingSystemCB       : register(b4)
MetricReconstructionCB  : register(b6)
ShadertoyCBuffer        : register(b7)
LaserPointerCBuffer     : register(b8)
PixelShaderCBuffer      : register(b9) (Used to be b0, but the D3DRendererHook is using that now)
RTConstantsBuffer       : register(b10)
VRGeometryCBuffer       : register(b11)

VS Constant Buffer Registers

D3dConstants                  : register(b0)
VertexShaderCBuffer           : register(b1)
VertexShaderMatrixCB          : register(b2)
MainShadersCBStruct           : register(b3)
ShadowMapVertexShaderMatrixCB : register(b5)
MetricReconstructionCB        : register(b6)
ShadertoyCBuffer              : register(b7)
OPTMeshTransformCBuffer       : register(b8)

*/

// Scale factor used to reconstruct metric 3D for VR and other uses
// This is now OBSOLETE in the VR path. Only used in non-VR mode and this will probably
// be removed later as well
#define METRIC_SCALE_FACTOR 25.0

// Only used in the non-VR path, and it should be removed later once the non-VR path
// is fully metric as well.
#define DEFAULT_FOCAL_DIST 2.0

// This is the limit, in meters, when we start fading out effects like SSAO and SSDO:
#define INFINITY_Z0 10000 // Used to be 15000 in release 1.1.2
// This is the limit, in meters, when the SSAO/SSDO effects are completely faded out
#define INFINITY_Z1 20000
// This is simply INFINITY_Z1 - INFINITY_Z0: It's used to fade the effect
#define INFINITY_FADEOUT_RANGE 10000 // Used to be 5000 in release 1.1.2

// Dynamic Cockpit: Maximum Number of DC elements per texture
#define MAX_DC_COORDS_PER_TEXTURE 12

// Maximum sun flares (not supported yet)
#define MAX_SUN_FLARES 4

#define MAX_XWA_LIGHTS 8
// Currently only used for the lasers
#define MAX_CB_POINT_LIGHTS 16

#define MAX_CSM_LEVELS 4

// Used in the special_control CB field in the pixel shader
// These flags are mutually-exclusive, so be careful when setting them. Only a handful are used
// in the same shader though.
#define SPECIAL_CONTROL_XWA_SHADOW          1
#define SPECIAL_CONTROL_GLASS               2
#define SPECIAL_CONTROL_BACKGROUND          3
#define SPECIAL_CONTROL_SMOKE               4
#define SPECIAL_CONTROL_HIGHLIGHT           5 // Used for debugging purposes (to highlight specific elements)
#define SPECIAL_CONTROL_NO_COLOR_ALPHA      6 // When this is set, the alpha value for the color output is forced to 0. Used in AlphaToBloom.hlsl
#define SPECIAL_CONTROL_EXPLOSION           7
#define SPECIAL_CONTROL_BLACK_TO_ALPHA      8 // Used when rendering animated textures so that black becomes transparent
#define SPECIAL_CONTROL_ALPHA_IS_BLOOM_MASK 9
#define SPECIAL_CONTROL_GRAYSCALE           10 // Used in the Tech Room to display grayscale models. Only for modelling/debugging purposes
#define SPECIAL_CONTROL_DEBUG               11 // Generic debug flag. Only used when developing or testing features.
#define SPECIAL_CONTROL_MISSILE             12
// These are the bits used for exclusive special control flags
#define SPECIAL_CONTROL_EXCLUSIVE_MASK      0x0FF
// The following are bits in the special_control field. They need to be blended with the rest of the exclusive
// flags
#define SPECIAL_CONTROL_BLAST_MARK 0x100 // Set when the current texture is a blast mark
// New flag should start at 0x200 and take one bit. So they would be 0x200, 0x400, 0x800, etc.

// Register slot for the metric reconstruction constant buffer
#define METRIC_REC_CB_SLOT 6

// If this value is too high, then DX will fail when creating textures.
// 2^9 = 512, that's probably the smallest dimension the desktop can be
// (either width or height)
// Or we can use 0 when creating the textures and -1 when creating the SRVs
// that seems to work without specifying the maximum mip levels.
#define MAX_MIP_LEVELS 9

#define MAX_BLOOM_MIP_LEVELS 6

// Labels defined at the C++ preprocessor level are not seen in shaders. Therefore,
// we have to define them here so that both C++ files and HLSL files can see these
// definitions.
//#define GENMIPMAPS

// Enable the following define and look at any sun to trigger the SunFlareShader.
// That'll show mip-map level 5 -- it's a nice way to debug mip maps.
//#define GENMIPMAPS_DEBUG

// Flags used to control overlay textures. These textures are used in
// PixelShaderAnim.hlsl to display hull damage textures and shields down
// effects.
// The multiplier texture is active and should be mixed with the color buffer
#define OVERLAY_CTRL_MULT 0x1
// The screen texture is active and should be mixed with the color buffer
#define OVERLAY_CTRL_SCREEN 0x2
// The multiplier texture is active and should be mixed with the lightmap buffer
#define OVERLAY_ILLUM_CTRL_MULT 0x4
// The screen texture is active and should be mixed with the lightmap buffer
#define OVERLAY_ILLUM_CTRL_SCREEN 0x8

// Maximum number of highlighted regions for the VR Keyboard.
#define MAX_VRKEYB_REGIONS 4
#endif