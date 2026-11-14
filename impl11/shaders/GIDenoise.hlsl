// Denoise filter for the indirect light (pbr_gi_*): the GTAO denoise filter for colors.
// See GTAO_GI_DENOISE in GTAODenoiseCommon.h.
#undef INSTANCED_RENDERING
#define GTAO_GI_DENOISE 1
#include "GTAODenoiseCommon.h"
