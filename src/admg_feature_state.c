#include "global.h"
#include "admg_feature_state.h"
#include "admg_retention.h"
#include "admg_registry.h"
#include "datetime.h"
#include "dexnav.h"
#include "siirtc.h"

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateBridgeVersion(void)
{
    return 0;
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureState(ADMG_Word state)
{
    return 0;
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_ResetFeatureState(ADMG_Word state)
{
    return 0;
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateCount(ADMG_Word state)
{
    switch (state)
    {
    case ADMG_FEATURE_STATE_CLOCK:
    case ADMG_FEATURE_STATE_NPC_FOLLOWER:
    case ADMG_FEATURE_STATE_DEXNAV_SELECTION:
    case ADMG_FEATURE_STATE_APRICORN_TREES: return 1;
    case ADMG_FEATURE_STATE_DEXNAV_SEARCH_LEVELS: return RUNTIME_SPECIES_COUNT;
    case ADMG_FEATURE_STATE_ITEMS_SEEN: return RUNTIME_ITEMS_COUNT / 8 + (RUNTIME_ITEMS_COUNT % 8 != 0);
    default: return 0;
    }
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateRecordSize(ADMG_Word state)
{
    switch (state)
    {
    case ADMG_FEATURE_STATE_CLOCK: return sizeof(struct SiiRtcInfo);
    case ADMG_FEATURE_STATE_ITEMS_SEEN: return sizeof(struct ADMG_ItemSeenByte);
    case ADMG_FEATURE_STATE_NPC_FOLLOWER: return sizeof(struct ADMG_NPCFollower);
    case ADMG_FEATURE_STATE_DEXNAV_SEARCH_LEVELS: return sizeof(struct ADMG_DexNavSearchLevel);
    case ADMG_FEATURE_STATE_DEXNAV_SELECTION: return sizeof(struct ADMG_DexNavSelection);
    case ADMG_FEATURE_STATE_APRICORN_TREES: return sizeof(struct ADMG_ApricornTreesPicked);
    default: return 0;
    }
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_InitializeFeatureState(ADMG_Word state, ADMG_Word address, ADMG_Word count)
{
    if (state == ADMG_FEATURE_STATE_CLOCK && address != 0 && count == 1)
    {
        struct SiiRtcInfo *rtc = (struct SiiRtcInfo *)address;
        memset(rtc, 0, sizeof(*rtc));
        rtc->year = 0;
        rtc->month = gGen3Epoch.month;
        rtc->day = gGen3Epoch.day;
        rtc->dayOfWeek = gGen3Epoch.dayOfWeek;
        return 1;
    }
    if (state == ADMG_FEATURE_STATE_NPC_FOLLOWER && address != 0 && count == 1)
    {
        struct ADMG_NPCFollower *follower = (struct ADMG_NPCFollower *)address;
        memset(follower, 0, sizeof(*follower));
        follower->objId = OBJECT_EVENTS_COUNT;
        return 1;
    }
    if (state == ADMG_FEATURE_STATE_DEXNAV_SELECTION && address != 0 && count == 1)
    {
        struct ADMG_DexNavSelection *selection = (struct ADMG_DexNavSelection *)address;
        selection->environment = ENCOUNTER_TYPE_LAND;
        return 1;
    }
    if (state == ADMG_FEATURE_STATE_DEXNAV_SEARCH_LEVELS && address != 0 && count != 0 && count == ADMG_GetFeatureStateCount(state))
    {
        memset((void *)address, 0, count * sizeof(struct ADMG_DexNavSearchLevel));
        return 1;
    }
    if (state == ADMG_FEATURE_STATE_APRICORN_TREES && address != 0 && count == 1)
    {
        memset((void *)address, 0, sizeof(struct ADMG_ApricornTreesPicked));
        return 1;
    }
    if (state == ADMG_FEATURE_STATE_ITEMS_SEEN && address != 0 && count != 0 && count == ADMG_GetFeatureStateCount(state))
    {
        memset((void *)address, 0, count * sizeof(struct ADMG_ItemSeenByte));
        return 1;
    }
    return 0;
}
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureStateBridgeVersion);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureState);
ADMG_RETAIN_FUNCTION(ADMG_ResetFeatureState);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureStateCount);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureStateRecordSize);
ADMG_RETAIN_FUNCTION(ADMG_InitializeFeatureState);
