#ifndef ADMG_FEATURE_STATE_H
#define ADMG_FEATURE_STATE_H
#include "admg_native.h"

/* Version 1 keeps a non-inlined call to FakeRtc_GetSecondsRatio in the tick path
 * and normalizes 32-bit time increments before storing byte-sized calendar fields. */
#define ADMG_CLOCK_RATE_DISPATCH_VERSION 1

/* NativeHost allocates these records when first requested and saves them through
 * a NativeSaveStore platform section. It supplies the state getter and bridge version;
 * the C defaults return zero to indicate that the bridge and state are unavailable. */
struct ADMG_DexNavSearchLevel
{
    u8 level;
};

struct ADMG_ItemSeenByte
{
    u8 bits;
};

/* Store NPC follower state separately from the conditional SaveBlock3 layout.
 * Content IDs remain 32-bit, and movement fields keep their original byte-sized representation. */
struct ADMG_NPCFollower
{
    u8 inProgress;
    u8 warpEnd;
    u8 createSurfBlob;
    u8 comeOutDoorStairs;
    u8 forcedMovement;
    u8 objId;
    u8 currentSprite;
    u8 delayedState;
    struct Coords16 log;
    const u8 *script;
    u32 flag;
    u32 graphicsId;
    u32 flags;
    u32 battlePartner;
};

enum ADMG_FeatureStateId
{
    ADMG_FEATURE_STATE_CLOCK = 1,
    ADMG_FEATURE_STATE_ITEMS_SEEN = 2,
    ADMG_FEATURE_STATE_NPC_FOLLOWER = 3,
    ADMG_FEATURE_STATE_DEXNAV_SEARCH_LEVELS = 4,
};
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateBridgeVersion(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureState(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_ResetFeatureState(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateCount(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateRecordSize(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_InitializeFeatureState(ADMG_Word state, ADMG_Word address, ADMG_Word count);
#endif
