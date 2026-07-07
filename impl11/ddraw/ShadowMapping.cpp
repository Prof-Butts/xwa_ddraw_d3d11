#include "common.h"
#include "ShadowMapping.h"
#include "utils.h"

/*********************************************************/
// SHADOW MAPPING
ShadowMappingData g_ShadowMapping;
float g_fShadowMapAngleY = 0.0f, g_fShadowMapAngleX = 0.0f, g_fShadowMapDepthTrans = 0.0f, g_fShadowMapScale = 0.5f;

// TODO: The VR path doesn't need this scale. We should get rid of it
// in the regular path too. This constant factor will affect how the POV
// translation is computed for both paths, and it may also affect the
// projection formulas in the regular path.
float SHADOW_OBJ_SCALE = 1.64f; // TODO: These scale params should be in g_ShadowMapping

bool g_bShadowMapDebug = false;
bool g_bShadowMappingInvertCameraMatrix = false;
bool g_bShadowMapEnablePCSS = false;
bool g_bShadowMapEnable = false;
bool g_bShadowMapHardwarePCF = false;
std::vector<Vector4> g_OBJLimits; // Box limits of the OBJ loaded. This is used to compute the Z range of the shadow map

Vector3 g_SunCentroids[MAX_XWA_LIGHTS]; // Stores all the sun centroids seen in this frame in in-game coords
Vector2 g_SunCentroids2D[MAX_XWA_LIGHTS]; // Stores all the 2D sun centroids seen in this frame in in-game coords
int g_iNumSunCentroids = 0;

/*********************************************************/

void DisplayTimedMessage(uint32_t seconds, int row, const char* msg);

void ToggleCSM()
{
	g_ShadowMapping.bCSMEnabled = !g_ShadowMapping.bCSMEnabled;
	if (g_ShadowMapping.bCSMEnabled)
		DisplayTimedMessage(3, 0, "CSM Enabled");
	else
		DisplayTimedMessage(3, 0, "CSM Disabled");
}

