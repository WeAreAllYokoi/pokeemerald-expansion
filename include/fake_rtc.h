#ifndef GUARD_FAKE_RTC_UTIL_H
#define GUARD_FAKE_RTC_UTIL_H

#include "siirtc.h"

void FakeRtc_Reset(void);
bool32 FakeRtc_IsActive(void);
struct SiiRtcInfo *FakeRtc_GetCurrentTime(void);
void FakeRtc_GetRawInfo(struct SiiRtcInfo *rtc);
bool32 FakeRtc_CanAdvanceTimeBy(u32 days, u32 hours, u32 minutes, u32 seconds);
bool32 FakeRtc_TryAdvanceTimeBy(u32 days, u32 hours, u32 minutes, u32 seconds);
bool32 FakeRtc_TryManuallySetTime(u32 day, u32 hour, u32 minute, u32 second);
bool32 FakeRtc_TryForwardTimeTo(u32 hour, u32 minute, u32 second);
void FakeRtc_AdvanceTimeBy(u32 days, u32 hours, u32 minutes, u32 seconds);
void FakeRtc_ManuallySetTime(u32 day, u32 hour, u32 minute, u32 second);
void FakeRtc_ForwardTimeTo(u32 hour, u32 minute, u32 second);
void FakeRtc_TickTimeForward(void);
u32 FakeRtc_GetSecondsRatio(void);
void Script_PauseFakeRtc(void);
void Script_ResumeFakeRtc(void);
void Script_ToggleFakeRtc(void);

#endif // GUARD_FAKE_RTC_UTIL_H
