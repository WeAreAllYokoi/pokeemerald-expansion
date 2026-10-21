#ifndef ADMG_FEATURE_STATE_H
#define ADMG_FEATURE_STATE_H
#include "admg_native.h"

/* NativeHost allocates these records when first requested and saves them through
 * a NativeSaveStore platform section. It supplies the state getter and bridge version;
 * the C defaults return zero to indicate that the bridge and state are unavailable. */
enum ADMG_FeatureStateId
{
    ADMG_FEATURE_STATE_CLOCK = 1,
};
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateBridgeVersion(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureState(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateCount(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateRecordSize(ADMG_Word state);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_InitializeFeatureState(ADMG_Word state, ADMG_Word address, ADMG_Word count);
#endif
