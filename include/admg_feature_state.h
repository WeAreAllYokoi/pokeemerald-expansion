#ifndef ADMG_FEATURE_STATE_H
#define ADMG_FEATURE_STATE_H
#include "admg_native.h"

/* Version 1 keeps a non-inlined call to FakeRtc_GetSecondsRatio in the tick path
 * and normalizes 32-bit time increments before storing byte-sized calendar fields. */
#define ADMG_CLOCK_RATE_DISPATCH_VERSION 1

/* NativeHost allocates these records when first requested and saves them through
 * a NativeSaveStore platform section. It supplies the state getter and bridge version;
 * the C defaults return zero to indicate that the bridge and state are unavailable. */
struct ADMG_ItemSeenByte
{
    u8 bits;
};

enum ADMG_FeatureStateId
{
    ADMG_FEATURE_STATE_CLOCK = 1,
    ADMG_FEATURE_STATE_ITEMS_SEEN = 2,
};
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateBridgeVersion(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureState(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateCount(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateRecordSize(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_InitializeFeatureState(ADMG_Word state, ADMG_Word address, ADMG_Word count);
#endif
