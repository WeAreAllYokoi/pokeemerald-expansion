#ifndef GUARD_CONSTANTS_MAPS_H
#define GUARD_CONSTANTS_MAPS_H

#include "map_groups.h"

enum
{
    // Warps using this map will instead use the warp data stored in gSaveBlock1Ptr->dynamicWarp.
    // Used for warps that need to change destinations, e.g. when stepping off an elevator.
    MAP_DYNAMIC = (0x7F | (0x7F << 8)),

    MAP_UNDEFINED = (0xFF | (0xFF << 8)),
};

// A MapId is 32-bit: (group << 8) | num. The base maps keep their values; maps added at run time start at
// FIRST_ADDED_MAP, so their "groups" are 256 and up and never index gMapGroups.
#define MAP_GROUP(map) ((map) >> 8)
#define MAP_NUM(map) ((map) & 0xFF)
#define MAP_ID(group, num) (((group) << 8) | (num))
#define FIRST_ADDED_MAP 0x10000
// A MapId kept in two 16-bit sprite or task data slots
#define MAP_ID_LO(map) ((u16)(map))
#define MAP_ID_HI(map) ((u16)((u32)(map) >> 16))
#define MAP_ID_FROM_HALVES(lo, hi) ((u32)(u16)(lo) | ((u32)(u16)(hi) << 16))

// IDs for dynamic warps. Both are used in the dest_warp_id field for warp events, but they
// are never read in practice. A dest_map of MAP_DYNAMIC is used to indicate that a
// dynamic warp should be used, at which point the warp id is ignored. They can be passed
// as the argument to SetWarpDestinationToDynamicWarp, but this argument is unused.
// As only one dynamic warp is saved at a time there's no need to distinguish between them.
#define WARP_ID_SECRET_BASE 0x7E
#define WARP_ID_DYNAMIC     0x7F

// Used to indicate an invalid warp id, for dummy warps or when a warp should
// use the given coordinates rather than the coordinates of a target warp.
#define WARP_ID_NONE (-1)

#endif  // GUARD_CONSTANTS_MAPS_H
