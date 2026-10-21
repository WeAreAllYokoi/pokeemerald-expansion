#include "global.h"
#include "admg_feature_state.h"
#include "admg_retention.h"
#include "datetime.h"
#include "siirtc.h"

ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateBridgeVersion(void)
{
    return 0;
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureState(ADMG_Word state)
{
    return 0;
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateCount(ADMG_Word state)
{
    return state == ADMG_FEATURE_STATE_CLOCK ? 1 : 0;
}
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetFeatureStateRecordSize(ADMG_Word state)
{
    return state == ADMG_FEATURE_STATE_CLOCK ? sizeof(struct SiiRtcInfo) : 0;
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
    return 0;
}
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureStateBridgeVersion);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureState);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureStateCount);
ADMG_RETAIN_FUNCTION(ADMG_GetFeatureStateRecordSize);
ADMG_RETAIN_FUNCTION(ADMG_InitializeFeatureState);
