#ifndef GUARD_ADMG_REGION_H
#define GUARD_ADMG_REGION_H

#include "admg_native.h"

// Queries for a map's region and FRLG layout, plus Kanto's once-per-save reset
// Kanto story flags and vars use the separate banks declared in admg_state.h

// TRUE in an FRLG build, or when the loaded map has an FRLG layout
ADMG_NATIVE_ENTRY bool32 ADMG_IsFrlgContext(void);

// Get a map's region from its map section, including added sections
// Return REGION_NONE if the map cannot be loaded
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetMapRegion(ADMG_Word mapId);

// A named entry for GetCurrentRegion, which is otherwise inline
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetCurrentRegion(void);

enum ADMG_RegionInitResult
{
    ADMG_REGION_INITIALIZED,         // The Kanto reset ran now
    ADMG_REGION_ALREADY_INITIALIZED, // Already initialized in this save; nothing changed
    ADMG_REGION_INIT_REFUSED,        // Not Kanto, an FRLG build, no SaveBlock1, or CB2_NewGame is active
};

// Initialize Kanto's retained story flags and vars once per save in an Emerald build
// EventScript_ResetAllMapFlagsFrlg uses the Kanto bank, leaving Hoenn flags and vars alone
// Refuse while CB2_NewGame is active or SaveBlock1 is unavailable
// Record completion in the saved gADMG_RegionalContext bits
ADMG_NATIVE_ENTRY ADMG_Word ADMG_InitRegionalContent(ADMG_Word region);

// Check the saved initialization bit; return FALSE for an out-of-range region
ADMG_NATIVE_ENTRY bool32 ADMG_IsRegionInitialized(ADMG_Word region);

#endif // GUARD_ADMG_REGION_H
