#ifndef GUARD_HEAL_LOCATION_H
#define GUARD_HEAL_LOCATION_H

struct HealLocation
{
    MapId mapId;
    u16 x;
    u16 y;
};

// A heal location added at run time (FIRST_ADDED_HEAL_LOCATION and up).
struct ADMG_AddedHealLocation
{
    struct HealLocation location;
    MapId respawnMap;       // white-out: the map of the healer and where the player stands,
    u8 respawnNpcLocalId;   // and the healer; LOCALID_NONE: white-out returns to the location itself
    u16 respawnX;
    u16 respawnY;
};

extern const struct ADMG_AddedHealLocation gADMG_AddedHealLocations[];

u32 GetHealLocationIndexByMap(MapId mapId);
u32 GetHealLocationIndexByWarpData(struct WarpData *warp);
const struct HealLocation *GetHealLocationByMap(MapId mapId);
const struct HealLocation *GetHealLocation(u32 index);
bool32 IsLastHealLocationPlayerHouse();
void SetWhiteoutRespawnWarpAndHealerNPC(struct WarpData * warp);
u32 GetHealNpcLocalId(u32 healLocationId);

#endif // GUARD_HEAL_LOCATION_H
