#ifndef GUARD_CONSTANTS_RTC_H
#define GUARD_CONSTANTS_RTC_H

#define RTC_INIT_ERROR         0x0001
#define RTC_INIT_WARNING       0x0002

#define RTC_ERR_12HOUR_CLOCK   0x0010
#define RTC_ERR_POWER_FAILURE  0x0020
#define RTC_ERR_INVALID_YEAR   0x0040
#define RTC_ERR_INVALID_MONTH  0x0080
#define RTC_ERR_INVALID_DAY    0x0100
#define RTC_ERR_INVALID_HOUR   0x0200
#define RTC_ERR_INVALID_MINUTE 0x0400
#define RTC_ERR_INVALID_SECOND 0x0800

#define RTC_ERR_FLAG_MASK      0x0FF0

// Read the selected generation's hours through ADMG_GetTimeOfDayHour in rtc.c
// These are C expressions, not compile-time constants
#define ADMG_MORNING_HOUR_BEGIN 0
#define ADMG_MORNING_HOUR_END   1
#define ADMG_DAY_HOUR_BEGIN     2
#define ADMG_DAY_HOUR_END       3
#define ADMG_EVENING_HOUR_BEGIN 4
#define ADMG_EVENING_HOUR_END   5
#define ADMG_NIGHT_HOUR_BEGIN   6
#define ADMG_NIGHT_HOUR_END     7

#define MORNING_HOUR_BEGIN ADMG_GetTimeOfDayHour(ADMG_MORNING_HOUR_BEGIN)
#define MORNING_HOUR_END   ADMG_GetTimeOfDayHour(ADMG_MORNING_HOUR_END)
#define DAY_HOUR_BEGIN     ADMG_GetTimeOfDayHour(ADMG_DAY_HOUR_BEGIN)
#define DAY_HOUR_END       ADMG_GetTimeOfDayHour(ADMG_DAY_HOUR_END)
#define EVENING_HOUR_BEGIN ADMG_GetTimeOfDayHour(ADMG_EVENING_HOUR_BEGIN)
#define EVENING_HOUR_END   ADMG_GetTimeOfDayHour(ADMG_EVENING_HOUR_END)
#define NIGHT_HOUR_BEGIN   ADMG_GetTimeOfDayHour(ADMG_NIGHT_HOUR_BEGIN)
#define NIGHT_HOUR_END     ADMG_GetTimeOfDayHour(ADMG_NIGHT_HOUR_END)

// TIMES_OF_DAY_COUNT must be last or things will break
enum TimeOfDay
{
    TIME_MORNING,
    TIME_DAY,
    TIME_EVENING,
    TIME_NIGHT,
    TIMES_OF_DAY_COUNT,
};

// for incrementing/decrementing
#define TIME_FIRST 0
#define TIME_LAST (TIMES_OF_DAY_COUNT - 1)

#define TIME_OF_DAY_DEFAULT    0

#endif // GUARD_CONSTANTS_RTC_H
