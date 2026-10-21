#include "global.h"
#include "event_data.h"
#include "heal_location.h"
#include "constants/event_objects.h"
#include "constants/heal_locations.h"
#include "constants/maps.h"

#include "data/heal_locations.h"

u32 GetHealLocationIndexByMap(MapId mapId)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sHealLocations); i++)
    {
        if (sHealLocations[i].mapId == mapId)
            return i + 1;
    }
    return HEAL_LOCATION_NONE;
}

const struct HealLocation *GetHealLocationByMap(MapId mapId)
{
    u32 index = GetHealLocationIndexByMap(mapId);

    if (index == HEAL_LOCATION_NONE)
        return NULL;
    else
        return &sHealLocations[index - 1];
}

u32 GetHealLocationIndexByWarpData(struct WarpData *warp)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sHealLocations); i++)
    {
        if (sHealLocations[i].mapId == warp->mapId
        && sHealLocations[i].x == warp->x
        && sHealLocations[i].y == warp->y)
            return i + 1;
    }
    return HEAL_LOCATION_NONE;
}

const struct HealLocation *GetHealLocation(u32 index)
{
    if (index == HEAL_LOCATION_NONE)
        return NULL;
    else if (index > ARRAY_COUNT(sHealLocations))
        return NULL;
    else
        return &sHealLocations[index - 1];
}

static bool32 IsLastHealLocation(u32 healLocation)
{
    const struct HealLocation *loc = GetHealLocation(healLocation);
    const struct WarpData *warpData = &gSaveBlock1Ptr->lastHealLocation;

    return warpData->mapId == loc->mapId
        && warpData->warpId == WARP_ID_NONE
        && warpData->x == loc->x
        && warpData->y == loc->y;
}

bool32 IsLastHealLocationPlayerHouse()
{
    if (IsLastHealLocation(HEAL_LOCATION_LITTLEROOT_TOWN_MAYS_HOUSE)
        || IsLastHealLocation(HEAL_LOCATION_LITTLEROOT_TOWN_MAYS_HOUSE_2F)
        || IsLastHealLocation(HEAL_LOCATION_LITTLEROOT_TOWN_BRENDANS_HOUSE)
        || IsLastHealLocation(HEAL_LOCATION_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F)
        || IsLastHealLocation(HEAL_LOCATION_PALLET_TOWN))
        return TRUE;

    return FALSE;
}

u32 GetHealNpcLocalId(u32 healLocationId)
{
    if (healLocationId == HEAL_LOCATION_NONE || healLocationId >= NUM_HEAL_LOCATIONS)
        return LOCALID_NONE;

    return sWhiteoutRespawnHealerNpcIds[healLocationId - 1];
}

void SetWhiteoutRespawnWarpAndHealerNPC(struct WarpData *warp)
{
    u32 healLocationId = GetHealLocationIndexByWarpData(&gSaveBlock1Ptr->lastHealLocation);
    u32 healNpcLocalId = GetHealNpcLocalId(healLocationId);

    if (!healNpcLocalId)
    {
        *(warp) = gSaveBlock1Ptr->lastHealLocation;
        return;
    }

    warp->mapId = MAP_ID(sWhiteoutRespawnHealCenterMapIdxs[healLocationId - 1][0], sWhiteoutRespawnHealCenterMapIdxs[healLocationId - 1][1]);
    warp->warpId = WARP_ID_NONE;
    warp->x = sWhiteoutRespawnHealCenterMapIdxs[healLocationId - 1][2];
    warp->y = sWhiteoutRespawnHealCenterMapIdxs[healLocationId - 1][3];
    gSpecialVar_LastTalked = healNpcLocalId;
    gSpecialVar_0x800B = healNpcLocalId;
}
