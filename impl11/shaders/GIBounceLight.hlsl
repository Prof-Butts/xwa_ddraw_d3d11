// The bounce light pass of the indirect light (pbr_gi_*): the diffuse light that each
// surface sends back out. See the GI_BOUNCE_LIGHT_PASS entry point in PBRAddV2Common.h.
#undef INSTANCED_RENDERING
#define GI_BOUNCE_LIGHT_PASS 1
#include "PBRAddV2Common.h"
