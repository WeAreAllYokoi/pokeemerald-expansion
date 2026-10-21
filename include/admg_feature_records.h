#ifndef ADMG_FEATURE_RECORDS_H
#define ADMG_FEATURE_RECORDS_H

#include "admg_native.h"

struct FormChangeContext;
struct MassOutbreak;

/* Pass records by pointer and return 32-bit values so callers do not pass or return structs by value. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFormChangeTargetSpecies(const struct FormChangeContext *context);
/* Returns TRUE after copying the selected outbreak into *outbreak.
 * Returns FALSE without writing if outbreak is NULL or index is out of range. */
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CopyStaticOutbreak(ADMG_Word index, struct MassOutbreak *outbreak);

#endif
