#include "global.h"
#include "admg_region.h"
#include "admg_state.h"
#include "main.h"
#include "new_game.h"
#include "overworld.h"
#include "regions.h"
#include "script.h"

extern const u8 EventScript_ResetAllMapFlagsFrlg[];

bool32 ADMG_IsFrlgContext(void)
{
    return IS_FRLG || (gMapHeader.mapLayout != NULL && gMapHeader.mapLayout->isFrlg);
}

ADMG_Word ADMG_GetMapRegion(ADMG_Word mapId)
{
    if (!ADMG_IsMapLoadable(mapId))
        return REGION_NONE;
    return GetRegionForSectionId(GetMapHeader(mapId)->regionMapSectionId);
}

ADMG_Word ADMG_GetCurrentRegion(void)
{
    return GetCurrentRegion();
}

bool32 ADMG_IsRegionInitialized(ADMG_Word region)
{
    return region < REGIONS_COUNT && (gADMG_RegionalContext[0] & (1u << region)) != 0;
}

ADMG_Word ADMG_InitRegionalContent(ADMG_Word region)
{
    // Only Kanto needs this extra reset; an FRLG new game already runs its own
    if (region != REGION_KANTO || IS_FRLG || gSaveBlock1Ptr == NULL || gMain.callback2 == CB2_NewGame)
        return ADMG_REGION_INIT_REFUSED;
    if (ADMG_IsRegionInitialized(region))
        return ADMG_REGION_ALREADY_INITIALIZED;
    RunScriptImmediately(EventScript_ResetAllMapFlagsFrlg);
    gADMG_RegionalContext[0] |= 1u << region;
    return ADMG_REGION_INITIALIZED;
}
