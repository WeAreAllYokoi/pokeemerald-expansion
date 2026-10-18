#include "global.h"
#include "malloc.h"
#include "admg_extended.h"

struct ExtendedStore
{
    u8 *data;
    u32 size;  // bytes
    u32 count; // bits
};

static EWRAM_DATA struct ExtendedStore sStores[ADMG_EXTENDED_STORE_COUNT] = {0};

static const u32 sStoreSections[ADMG_EXTENDED_STORE_COUNT] =
{
    [ADMG_EXTENDED_TRAINER_DEFEATS] = ADMG_SAVE_SECTION_EXTENDED_TRAINER_DEFEATS,
    [ADMG_EXTENDED_DEX_SEEN] = ADMG_SAVE_SECTION_EXTENDED_DEX_SEEN,
    [ADMG_EXTENDED_DEX_CAUGHT] = ADMG_SAVE_SECTION_EXTENDED_DEX_CAUGHT,
};

static void InitBitStore(enum ADMG_ExtendedStore store, u32 count)
{
    sStores[store].count = count;
    sStores[store].size = (count + 7) / 8;
    sStores[store].data = count != 0 ? AllocZeroed(sStores[store].size) : NULL;
}

void ADMG_InitExtendedStores(void)
{
    u32 addedDex = RUNTIME_NATIONAL_DEX_COUNT > FIRST_ADDED_NATIONAL_DEX ? RUNTIME_NATIONAL_DEX_COUNT - FIRST_ADDED_NATIONAL_DEX : 0;

    InitBitStore(ADMG_EXTENDED_TRAINER_DEFEATS, RUNTIME_ADDED_TRAINERS_COUNT);
    InitBitStore(ADMG_EXTENDED_DEX_SEEN, addedDex);
    InitBitStore(ADMG_EXTENDED_DEX_CAUGHT, addedDex);
}

u8 *ADMG_GetExtendedBitPointer(enum ADMG_ExtendedStore store, u32 index)
{
    if (store >= ADMG_EXTENDED_STORE_COUNT || index >= sStores[store].count || sStores[store].data == NULL)
        return NULL;
    return &sStores[store].data[index / 8];
}

u8 *ADMG_GetExtendedFlagPointer(u32 id)
{
    STATIC_ASSERT(ADMG_ADDED_TRAINER_FLAGS_START % 8 == 0, extendedFlagsStartOnAByte);
    if (id >= ADMG_ADDED_TRAINER_FLAGS_START)
        return ADMG_GetExtendedBitPointer(ADMG_EXTENDED_TRAINER_DEFEATS, id - ADMG_ADDED_TRAINER_FLAGS_START);
    return NULL;
}

u32 ADMG_GetExtendedSaveRegions(struct ADMG_SaveRegion *regions)
{
    u32 i, count = 0;
    for (i = 0; i < ADMG_EXTENDED_STORE_COUNT; i++)
    {
        if (sStores[i].size != 0)
            regions[count++] = (struct ADMG_SaveRegion){sStoreSections[i], sStores[i].data, sStores[i].size};
    }
    return count;
}
