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

/* Reject invalid dates, times or weekdays, and advances past 31 December of year offset 255,
 * before calling calendar helpers. Check whole-day increments against the remaining range,
 * then combine sub-day remainders so even UINT32_MAX inputs cannot overflow the bounds
 * calculation. This also bounds the day count passed to the calendar helpers. */
static bool32 FakeRtc_CanAdvanceRecord(const struct SiiRtcInfo *rtc, u32 days, u32 hours, u32 minutes, u32 seconds)
{
    u32 daysInMonth, remaining, carry, timeOfDay;
    if (rtc == NULL || rtc->month < MONTH_JAN || rtc->month > MONTH_DEC
     || rtc->dayOfWeek >= WEEKDAY_COUNT || rtc->hour >= HOURS_PER_DAY
     || rtc->minute >= MINUTES_PER_HOUR || rtc->second >= SECONDS_PER_MINUTE)
        return FALSE;
    daysInMonth = sNumDaysInMonths[rtc->month - 1] + (rtc->month == MONTH_FEB && IsLeapYear(rtc->year));
    if (rtc->day == 0 || rtc->day > daysInMonth)
        return FALSE;
    remaining = ConvertDateToDayCount(255, MONTH_DEC, 31) - ConvertDateToDayCount(rtc->year, rtc->month, rtc->day);
    if (days > remaining)
        return FALSE;
    remaining -= days;
    carry = hours / HOURS_PER_DAY;
    if (carry > remaining)
        return FALSE;
    remaining -= carry;
    carry = minutes / (HOURS_PER_DAY * MINUTES_PER_HOUR);
    if (carry > remaining)
        return FALSE;
    remaining -= carry;
    carry = seconds / (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
    if (carry > remaining)
        return FALSE;
    remaining -= carry;
    timeOfDay = rtc->hour * 3600 + rtc->minute * 60 + rtc->second
              + (hours % HOURS_PER_DAY) * 3600
              + (minutes % (HOURS_PER_DAY * MINUTES_PER_HOUR)) * 60
              + seconds % (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
    return timeOfDay / (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE) <= remaining;
}

ADMG_NATIVE_ENTRY bool32 FakeRtc_CanAdvanceTimeBy(u32 days, u32 hours, u32 minutes, u32 seconds)
{
    return FakeRtc_CanAdvanceRecord(FakeRtc_GetCurrentTime(), days, hours, minutes, seconds);
}

ADMG_NATIVE_ENTRY bool32 FakeRtc_TryAdvanceTimeBy(u32 days, u32 hours, u32 minutes, u32 seconds)
{
    struct DateTime dateTime;
    struct SiiRtcInfo *rtc = FakeRtc_GetCurrentTime();
    if (!FakeRtc_CanAdvanceRecord(rtc, days, hours, minutes, seconds))
        return FALSE;
    ConvertRtcToDateTime(&dateTime, rtc);
    DateTime_AddSeconds(&dateTime, seconds);
    DateTime_AddMinutes(&dateTime, minutes);
    DateTime_AddHours(&dateTime, hours);
    DateTime_AddDays(&dateTime, days);
    ConvertDateTimeToRtc(rtc, &dateTime);
    return TRUE;
}

void FakeRtc_AdvanceTimeBy(u32 days, u32 hours, u32 minutes, u32 seconds)
{
    FakeRtc_TryAdvanceTimeBy(days, hours, minutes, seconds);
}

ADMG_NATIVE_ENTRY bool32 FakeRtc_TryForwardTimeTo(u32 hour, u32 minute, u32 second)
{
    struct SiiRtcInfo *rtc = FakeRtc_GetCurrentTime();
    u32 current, target, secondsPerDay;
    if (rtc == NULL || hour >= HOURS_PER_DAY || minute >= MINUTES_PER_HOUR || second >= SECONDS_PER_MINUTE)
        return FALSE;
    current = rtc->hour * MINUTES_PER_HOUR * SECONDS_PER_MINUTE + rtc->minute * SECONDS_PER_MINUTE + rtc->second;
    target = hour * MINUTES_PER_HOUR * SECONDS_PER_MINUTE + minute * SECONDS_PER_MINUTE + second;
    secondsPerDay = HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE;
    return FakeRtc_TryAdvanceTimeBy(0, 0, 0, (target + secondsPerDay - current) % secondsPerDay);
}

void FakeRtc_ForwardTimeTo(u32 hour, u32 minute, u32 second)
{
    FakeRtc_TryForwardTimeTo(hour, minute, second);
}

ADMG_NATIVE_ENTRY bool32 FakeRtc_TryManuallySetTime(u32 day, u32 hour, u32 minute, u32 second)
{
    struct SiiRtcInfo *rtc = FakeRtc_GetCurrentTime();
    struct SiiRtcInfo candidate;
    struct DateTime dateTime = gGen3Epoch;
    if (rtc == NULL)
        return FALSE;
    candidate = *rtc;
    ConvertDateTimeToRtc(&candidate, &dateTime);
    if (!FakeRtc_CanAdvanceRecord(&candidate, day, hour, minute, second))
        return FALSE;
    DateTime_AddSeconds(&dateTime, second);
    DateTime_AddMinutes(&dateTime, minute);
    DateTime_AddHours(&dateTime, hour);
    DateTime_AddDays(&dateTime, day);
    ConvertDateTimeToRtc(rtc, &dateTime);
    return TRUE;
}

void FakeRtc_ManuallySetTime(u32 day, u32 hour, u32 minute, u32 second)
{
    FakeRtc_TryManuallySetTime(day, hour, minute, second);
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
