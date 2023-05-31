/*
 * From: https://takinginitiative.wordpress.com/2011/05/15/directx10-tutorial-10-shadow-mapping/
 * Simple VertexShader needed to populate the depth stencil used
 * for shadow mapping.
 */
#include "XwaD3dCommon.hlsl"
#include "shader_common.h"
#include "shadow_mapping_common.h"

struct VertexShaderInput
{
	float4 pos      : POSITION; // The D3DRendererHook version has OPT-scale metric coords here
	//float4 color    : COLOR0;
	//float4 specular : COLOR1;
	//float2 tex      : TEXCOORD;
};

struct SHADOW_PS_INPUT
{
	float4 pos : SV_POSITION;
};

/*

Code copied from the Blender file used to run experiments
# POV values for the X-W taken from MXvTED:
POVX =   0.0
POVY =  37.0
POVZ = -31.0

POV_FACTOR_XY = 41 // This should probably be 40.96
POV_FACTOR_Z = 41  // This should probably be 40.96

shy = -0.075
sz = 0.94

POVX_OFS = POVX / POV_FACTOR_XY
POVY_OFS = POVY / POV_FACTOR_XY
POVZ_OFS = POVZ / POV_FACTOR_Z

# XWA to Shadow Model transform:
for i in range(num_verts):
	p = verts[i].co.xyz
	mesh.verts[i].co.x = p.x + POVX_OFS
	mesh.verts[i].co.y = p.y + p.z * shy + POVY_OFS
	mesh.verts[i].co.z = p.z * sz + POVZ_OFS

*/

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

// Use this shader for 3D Shadow OBJ
SHADOW_PS_INPUT main(VertexShaderInput input)
{
	SHADOW_PS_INPUT output;

	// The input is expected to be OPT-scale 3D (+/-1 on each axis, depends on the MAT file)
	// OPT (object space) to viewspace transform:
	float4 P = mul(float4(input.pos.xyz, 1.0f), transformWorldView);
	P.xyz *= 0.024414; // 1/40.96, OPT to metric conversion factor
	P.y = -P.y; // Further conversion of the coordinate system (Y+ is up)
	
	// We have transformed from OPT 3D to metric 3D, with the POV at the origin.
	// let's apply the light transform and project it from the light's point of view.

	// When computing shadows in SSDOAdd/SSAOAdd, the transformation rule starts at this point
	P = mul(lightWorldMatrix[light_index], P);

	// xy: Parallel projection
	// Map the useful Z depth into the range 0..0.98:
	//output.pos = float4(P.xy, (P.z - OBJminZ) / OBJrange * 0.98, 1.0);
	output.pos = float4(P.xy, MetricZToDepth(light_index, P.z), 1.0);
	return output;
}