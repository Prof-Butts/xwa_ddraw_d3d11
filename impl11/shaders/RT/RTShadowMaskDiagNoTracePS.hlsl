// TEMPORARY diagnostic (Ctrl+Alt+T): the RT shadow mask without the BVH traversal (every
// ray misses), to measure the pass's fixed cost. Not in VR.
#undef INSTANCED_RENDERING
#define RT_DIAG_NO_TRACE 1
#include "RTShadowMaskPSCommon.h"
