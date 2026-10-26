#include "admg_native.h"

#ifndef ADMG_INTEGRATION_RECIPE_VERSION
#error "Compile with the committed ADMG integration recipe identity"
#endif

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetIntegrationRecipeVersion(void)
{
    return ADMG_INTEGRATION_RECIPE_VERSION;
}
