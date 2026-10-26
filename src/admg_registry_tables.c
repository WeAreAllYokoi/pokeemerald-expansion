#include "global.h"
#include "heal_location.h"
#include "region_map.h"
#include "constants/event_objects.h"
#include "constants/heal_locations.h"
#include "constants/maps.h"

// Placeholders for tables relocated by the native registry, with counts in gADMG_RegistryCounts
// Keep these definitions out of the files that read them
// Otherwise, the compiler can fold a lookup to the known placeholder value

static const u8 sText_AddedPlaceholderName[] = _("");

// Layouts for IDs starting at FIRST_ADDED_LAYOUT; NULL entries have no layout
const struct MapLayout *const gADMG_AddedLayouts[1] = {0};

// Map headers for IDs starting at FIRST_ADDED_MAP; NULL entries have no map
const struct MapHeader *const gADMG_AddedMaps[1] = {0};

// Map sections for IDs starting at FIRST_ADDED_MAPSEC
// The placeholder has an empty name, no map area and no Fly destination
const struct ADMG_AddedMapSection gADMG_AddedMapSections[1] =
{
    {
        .location = { .name = sText_AddedPlaceholderName },
        .healLocation = HEAL_LOCATION_NONE,
        .flyMap = MAP_UNDEFINED,
    },
};

// Heal locations for IDs starting at FIRST_ADDED_HEAL_LOCATION
// The placeholder has no map, respawn map or respawn NPC
const struct ADMG_AddedHealLocation gADMG_AddedHealLocations[1] =
{
    {
        .location = { .mapId = MAP_UNDEFINED },
        .respawnMap = MAP_UNDEFINED,
        .respawnNpcLocalId = LOCALID_NONE,
    },
};
