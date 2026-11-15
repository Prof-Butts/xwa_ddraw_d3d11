// The emitter light pass of the indirect light (pbr_gi_emitters): the light of the true
// emitters. See the GI_EMITTER_PASS entry point in PBRAddV2Common.h.
#undef INSTANCED_RENDERING
#define GI_EMITTER_PASS 1
#include "PBRAddV2Common.h"
