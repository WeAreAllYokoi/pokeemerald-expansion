#include "global.h"
#include "admg_feature_records.h"
#include "admg_retention.h"
#include "pokemon.h"

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFormChangeTargetSpecies(const struct FormChangeContext *context)
{
    if (context == NULL)
        return SPECIES_NONE;
    return GetFormChangeTargetSpecies_Internal(*context);
}

ADMG_RETAIN_FUNCTION(ADMG_GetFormChangeTargetSpecies);
ADMG_RETAIN_FUNCTION(ADMG_CopyStaticOutbreak);
