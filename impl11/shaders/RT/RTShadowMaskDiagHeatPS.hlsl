// TEMPORARY diagnostic (Ctrl+Alt+T): the RT shadow mask holds the BVH nodes visited (x) and
// the triangles tested (y) by each pixel's rays, for the heat map. Not in VR.
#undef INSTANCED_RENDERING
#define RT_COUNT_VISITS 1
#define RT_DIAG_HEAT 1
#include "RTShadowMaskPSCommon.h"
