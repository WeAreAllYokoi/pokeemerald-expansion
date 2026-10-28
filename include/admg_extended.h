#ifndef GUARD_ADMG_EXTENDED_H
#define GUARD_ADMG_EXTENDED_H

// ADMG-101 extended stores: saved state indexed by content added at run time, for which the base game's fixed save
// arrays have no room (they keep everything the base game indexes). Each store is sized from the registry counts
// when the game starts, allocated once, and saved with the game as a durable region of its own; with nothing added
// every store is empty and the save holds none of them.

#include "admg_native_save.h"

enum ADMG_ExtendedStore
{
    ADMG_EXTENDED_TRAINER_DEFEATS, // A bit per added trainer: its defeat flag
    ADMG_EXTENDED_DEX_SEEN,        // A bit per added National Pokédex number
    ADMG_EXTENDED_DEX_CAUGHT,
    ADMG_EXTENDED_STORE_COUNT,
};

// Flag IDs of the added trainers' defeat flags, above every flag of the base game: FlagGet, FlagSet and FlagClear
// reach them like any other flag.
#define ADMG_ADDED_TRAINER_FLAGS_START 0x10000

// Sizes and allocates every store from the registry counts. Called once when the game starts.
void ADMG_InitExtendedStores(void);
// Clears every store, keeping its size. A new game calls it: a save loaded before keeps nothing in them.
void ADMG_ClearExtendedStores(void);
// The byte that holds bit index of a bit store (bit index % 8), or NULL if the store has no such bit.
u8 *ADMG_GetExtendedBitPointer(enum ADMG_ExtendedStore store, u32 index);
// The flag byte of an extended flag ID (the bit is id % 8), or NULL if no store holds that flag.
u8 *ADMG_GetExtendedFlagPointer(u32 id);
// Appends the save region of every non-empty store; returns how many it appended.
u32 ADMG_GetExtendedSaveRegions(struct ADMG_SaveRegion *regions);

#endif // GUARD_ADMG_EXTENDED_H
