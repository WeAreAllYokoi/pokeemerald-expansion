#ifndef ADMG_NATIVE_H
#define ADMG_NATIVE_H

#include "gba/types.h"

/* Guest ABI words only. No host pointers or persistent layouts are defined here. */
typedef u32 ADMG_Word;
_Static_assert(sizeof(ADMG_Word) == 4, "ADMG guest words must be 32 bits");
_Static_assert(sizeof(void *) == 4, "ADMG source integration requires the GBA ABI");

/* Preserve a real named entry point through GCC interprocedural optimization. */
#define ADMG_NATIVE_ENTRY __attribute__((used, noipa, externally_visible))

/* Recipe identity only; not the capability API, state schema, or DLL version. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetIntegrationRecipeVersion(void);

#endif
