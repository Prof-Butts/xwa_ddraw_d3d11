// GTAO with the indirect light (pbr_gi_*): the visibility-bitmask gather. See GTAO_GI
// in GTAOCommon.h. Shader model 5.0 (countbits).
#undef INSTANCED_RENDERING
#define GTAO_GI 1
#include "GTAOCommon.h"
