#pragma once

/*
 * TimeLib2 v2 - dependency-free core types, constants and helpers.
 *
 * This header intentionally does not include <Arduino.h> so the conversion
 * logic can be compiled and unit-tested on a host machine.
 */

#include <stdint.h>
#include <time.h>

namespace timelib2 {

/*==============================================================================
 * Clock status
 *============================================================================*/

enum class TimeStatus : uint8_t {
  NotSet    = 0,
  NeedsSync = 1,
  Set       = 2
};

/* Legacy (namespace-scoped) value aliases. These do NOT pollute the global
 * namespace because they live inside timelib2. */
constexpr TimeStatus timeNotSet    = TimeStatus::NotSet;
constexpr TimeStatus timeNeedsSync = TimeStatus::NeedsSync;
constexpr TimeStatus timeSet       = TimeStatus::Set;

/*==============================================================================
 * Day of week
 *============================================================================*/

enum class DayOfWeek : uint8_t {
  Invalid   = 0,
  Sunday    = 1,
  Monday    = 2,
  Tuesday   = 3,
  Wednesday = 4,
  Thursday  = 5,
  Friday    = 6,
  Saturday  = 7
};

constexpr DayOfWeek dowInvalid   = DayOfWeek::Invalid;
constexpr DayOfWeek dowSunday    = DayOfWeek::Sunday;
constexpr DayOfWeek dowMonday    = DayOfWeek::Monday;
constexpr DayOfWeek dowTuesday   = DayOfWeek::Tuesday;
constexpr DayOfWeek dowWednesday = DayOfWeek::Wednesday;
constexpr DayOfWeek dowThursday  = DayOfWeek::Thursday;
constexpr DayOfWeek dowFriday    = DayOfWeek::Friday;
constexpr DayOfWeek dowSaturday  = DayOfWeek::Saturday;

/*==============================================================================
 * Compact time elements
 *
 * Year is stored as an offset from 1970 (matching the historical Time
 * library layout). The uint8_t field supports calendar years 1970..2225.
 *============================================================================*/

struct TimeElements {
  uint8_t Second = 0;
  uint8_t Minute = 0;
  uint8_t Hour   = 0;
  uint8_t Wday   = 0;  // day of week, Sunday is 1
  uint8_t Day    = 1;
  uint8_t Month  = 1;  // January is 1
  uint8_t Year   = 0;  // offset from 1970
};

using tmElements_t    = TimeElements;
using TimeElementsLegacy = TimeElements;
using tmElementsPtr_t = TimeElements*;

using timeStatus_t   = TimeStatus;
using getExternalTime = time_t (*)();

/* Legacy field index enum, retained inside the namespace for source
 * compatibility with code that indexed time elements. */
enum tmByteFields : uint8_t {
  tmSecond,
  tmMinute,
  tmHour,
  tmWday,
  tmDay,
  tmMonth,
  tmYear,
  tmNbrFields
};

/*==============================================================================
 * Time constants (constexpr, not macros)
 *============================================================================*/

constexpr time_t SECS_PER_MIN  = 60;
constexpr time_t SECS_PER_HOUR = 3600;
constexpr time_t SECS_PER_DAY  = SECS_PER_HOUR * 24;
constexpr time_t DAYS_PER_WEEK = 7;
constexpr time_t SECS_PER_WEEK = SECS_PER_DAY * DAYS_PER_WEEK;
constexpr time_t SECS_PER_YEAR = SECS_PER_DAY * 365;
constexpr time_t SECS_YR_2000  = 946684800;

constexpr uint8_t dt_MAX_STRING_LEN = 9;

/*==============================================================================
 * Conversion helpers (constexpr functions replace the old macros)
 *============================================================================*/

constexpr time_t minutesToTime_t(time_t minutes) noexcept { return minutes * SECS_PER_MIN; }
constexpr time_t hoursToTime_t(time_t hours) noexcept     { return hours * SECS_PER_HOUR; }
constexpr time_t daysToTime_t(time_t days) noexcept       { return days * SECS_PER_DAY; }
constexpr time_t weeksToTime_t(time_t weeks) noexcept     { return weeks * SECS_PER_WEEK; }

constexpr time_t numberOfSeconds(time_t t) noexcept { return t % SECS_PER_MIN; }
constexpr time_t numberOfMinutes(time_t t) noexcept { return (t / SECS_PER_MIN) % 60; }
constexpr time_t numberOfHours(time_t t) noexcept   { return (t % SECS_PER_DAY) / SECS_PER_HOUR; }

constexpr uint8_t dayOfWeek(time_t t) noexcept {
  return static_cast<uint8_t>((((t / SECS_PER_DAY) + 4) % DAYS_PER_WEEK) + 1);
}

constexpr time_t elapsedDays(time_t t) noexcept      { return t / SECS_PER_DAY; }
constexpr time_t elapsedSecsToday(time_t t) noexcept { return t % SECS_PER_DAY; }

/* The following assume a clock set to a date later than Jan 1 1971. */
constexpr time_t previousMidnight(time_t t) noexcept { return (t / SECS_PER_DAY) * SECS_PER_DAY; }
constexpr time_t nextMidnight(time_t t) noexcept     { return previousMidnight(t) + SECS_PER_DAY; }

constexpr time_t elapsedSecsThisWeek(time_t t) noexcept {
  return elapsedSecsToday(t) + ((dayOfWeek(t) - 1) * SECS_PER_DAY);
}

constexpr time_t previousSunday(time_t t) noexcept { return t - elapsedSecsThisWeek(t); }
constexpr time_t nextSunday(time_t t) noexcept     { return previousSunday(t) + SECS_PER_WEEK; }

/* Year conversions */
constexpr int tmYearToCalendar(int year) noexcept      { return year + 1970; }
constexpr uint8_t CalendarYrToTm(int year) noexcept    { return static_cast<uint8_t>(year - 1970); }
constexpr int tmYearToY2k(int year) noexcept           { return year - 30; }
constexpr uint8_t y2kYearToTm(int year) noexcept       { return static_cast<uint8_t>(year + 30); }

/*==============================================================================
 * Low-level time conversion
 *============================================================================*/

/* Break a Unix epoch value into UTC calendar elements. */
void breakTime(time_t time, TimeElements& elements) noexcept;

/* Assemble UTC calendar elements into a Unix epoch value.
 * Month is clamped to 1..12 and Day to >= 1; other fields are used as-is. */
time_t makeTime(const TimeElements& elements) noexcept;

/*==============================================================================
 * Date strings
 *
 * Returned pointers reference static string literals and are valid for the
 * lifetime of the program. The functions are reentrant and thread-safe.
 *============================================================================*/

const char* monthStr(uint8_t month) noexcept;
const char* monthShortStr(uint8_t month) noexcept;
const char* dayStr(uint8_t day) noexcept;
const char* dayShortStr(uint8_t day) noexcept;

}  // namespace timelib2
