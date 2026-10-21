#include "global.h"
#include "admg_feature_bindings.h"
#include "admg_feature_policies.h"
#include "admg_feature_state.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "datetime.h"
#include "rtc.h"
#include "fake_rtc.h"
#include "event_data.h"
#include "script.h"


void FakeRtc_Reset(void)
{
    if (ADMG_GetFeatureStateBridgeVersion() != 1)
        return;
    ADMG_Word address = ADMG_GetFeatureState(ADMG_FEATURE_STATE_CLOCK);
    if (address != 0)
        ADMG_InitializeFeatureState(ADMG_FEATURE_STATE_CLOCK, address, 1);
}

struct SiiRtcInfo *FakeRtc_GetCurrentTime(void)
{
    if (!ADMG_GetFeaturePolicy(ADMG_POLICY_FAKE_RTC) || ADMG_GetFeatureStateBridgeVersion() != 1)
        return NULL;
    return (struct SiiRtcInfo *)ADMG_GetFeatureState(ADMG_FEATURE_STATE_CLOCK);
}

bool32 FakeRtc_IsActive(void)
{
    return FakeRtc_GetCurrentTime() != NULL;
}

void FakeRtc_GetRawInfo(struct SiiRtcInfo *rtc)
{
    struct SiiRtcInfo *fakeRtc = FakeRtc_GetCurrentTime();
    if (fakeRtc != NULL && rtc != NULL)
        memcpy(rtc, fakeRtc, sizeof(struct SiiRtcInfo));
}

void FakeRtc_TickTimeForward(void)
{
    if (!FakeRtc_IsActive())
        return;

    if (FlagGet(ADMG_GetFeatureBinding(ADMG_BINDING_OW_FLAG_PAUSE_TIME)))
        return;

    FakeRtc_AdvanceTimeBy(0, 0, 0, FakeRtc_GetSecondsRatio());
}

void FakeRtc_AdvanceTimeBy(u32 days, u32 hours, u32 minutes, u32 seconds)
{
    if (!FakeRtc_IsActive())
        return;

    struct DateTime dateTime;
    struct SiiRtcInfo *rtc = FakeRtc_GetCurrentTime();

    ConvertRtcToDateTime(&dateTime, rtc);
    DateTime_AddSeconds(&dateTime, seconds);
    DateTime_AddMinutes(&dateTime, minutes);
    DateTime_AddHours(&dateTime, hours);
    DateTime_AddDays(&dateTime, days);
    ConvertDateTimeToRtc(rtc, &dateTime);
}

void FakeRtc_ForwardTimeTo(u32 hour, u32 minute, u32 second)
{
    if (!FakeRtc_IsActive() || hour >= HOURS_PER_DAY || minute >= MINUTES_PER_HOUR || second >= SECONDS_PER_MINUTE)
        return;

    struct SiiRtcInfo *rtc = FakeRtc_GetCurrentTime();
    u32 current = rtc->hour * MINUTES_PER_HOUR * SECONDS_PER_MINUTE + rtc->minute * SECONDS_PER_MINUTE + rtc->second;
    u32 target = hour * MINUTES_PER_HOUR * SECONDS_PER_MINUTE + minute * SECONDS_PER_MINUTE + second;
    u32 secondsPerDay = HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE;
    u32 delta = (target + secondsPerDay - current) % secondsPerDay;

    /* Advance synchronously on the game thread without changing the pause flag.
     * A clock paused by the caller must remain paused after this operation. */
    FakeRtc_AdvanceTimeBy(0, 0, 0, delta);
}

void FakeRtc_ManuallySetTime(u32 day, u32 hour, u32 minute, u32 second)
{
    if (!FakeRtc_IsActive())
        return;
    FakeRtc_Reset();
    FakeRtc_AdvanceTimeBy(day, hour, minute, second);
}

ADMG_NATIVE_ENTRY u32 FakeRtc_GetSecondsRatio(void)
{
    return (OW_ALTERED_TIME_RATIO == GEN_8_PLA)   ? 60 :
           (OW_ALTERED_TIME_RATIO == GEN_9)       ? 20 :
           (OW_ALTERED_TIME_RATIO == TIME_DEBUG)  ?  1 :
                                                     1;
}

STATIC_ASSERT((OW_FLAG_PAUSE_TIME == 0 || OW_USE_FAKE_RTC == TRUE), FakeRtcMustBeTrueToPauseTime);

void Script_PauseFakeRtc(void)
{
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);

    FlagSet(ADMG_GetFeatureBinding(ADMG_BINDING_OW_FLAG_PAUSE_TIME));
}

void Script_ResumeFakeRtc(void)
{
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);

    FlagClear(ADMG_GetFeatureBinding(ADMG_BINDING_OW_FLAG_PAUSE_TIME));
}

void Script_ToggleFakeRtc(void)
{
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);

    FlagToggle(ADMG_GetFeatureBinding(ADMG_BINDING_OW_FLAG_PAUSE_TIME));
}
