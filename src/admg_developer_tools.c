#include "global.h"
#include "admg_developer_tools.h"
#include "admg_retention.h"

// Keep this a real, non-inlined call so mods can change the enabled tools at runtime
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetDeveloperTools(void)
{
    return ADMG_DEVELOPER_TOOLS_DEFAULT;
}
ADMG_RETAIN_FUNCTION(ADMG_GetDeveloperTools);
