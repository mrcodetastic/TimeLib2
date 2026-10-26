/*
 * TimeConversions.cpp - UTC conversion routines for TimeLib2.
 *
 * These functions have no Arduino dependency and operate on a 64-bit-capable
 * time_t, so they can be unit-tested on a host machine.
 */

#include "TimeConversions.hpp"

namespace timelib2 {

void breakTime(time_t timeInput, TimeElements& elements) noexcept {
  int64_t seconds = static_cast<int64_t>(timeInput);
  if (seconds < 0) {
    seconds = 0;
  }

  const int64_t days = seconds / SECS_PER_DAY;
  const int64_t remainder = seconds % SECS_PER_DAY;

  elements.Hour   = static_cast<uint8_t>(remainder / SECS_PER_HOUR);
  elements.Minute = static_cast<uint8_t>((remainder % SECS_PER_HOUR) / SECS_PER_MIN);
  elements.Second = static_cast<uint8_t>(remainder % SECS_PER_MIN);
  elements.Wday   = static_cast<uint8_t>((((days + 4) % 7) + 7) % 7 + 1);

  /* Civil-from-days algorithm (Howard Hinnant), valid for the full range of
   * int64_t days. 719468 shifts the epoch to 0000-03-01. */
  const int64_t z   = days + 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const uint32_t doe = static_cast<uint32_t>(z - era * 146097);
  const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int64_t year       = static_cast<int64_t>(yoe) + era * 400;
  const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const uint32_t mp  = (5 * doy + 2) / 153;
  const uint8_t day  = static_cast<uint8_t>(doy - (153 * mp + 2) / 5 + 1);
  const int month    = static_cast<int>(mp) + ((mp < 10) ? 3 : -9);

  year += (month <= 2) ? 1 : 0;

  int64_t yearOffset = year - 1970;
  if (yearOffset < 0) {
    yearOffset = 0;
  } else if (yearOffset > 255) {
    yearOffset = 255;  // representable limit of the uint8_t Year field
  }

  elements.Year  = static_cast<uint8_t>(yearOffset);
  elements.Month = static_cast<uint8_t>(month);
  elements.Day   = day;
}

time_t makeTime(const TimeElements& elements) noexcept {
  const int month = (elements.Month >= 1 && elements.Month <= 12)
                        ? elements.Month
                        : 1;
  const uint32_t dayOfMonth = (elements.Day >= 1)
                                  ? static_cast<uint32_t>(elements.Day - 1)
                                  : 0;

  int64_t year = 1970 + static_cast<int64_t>(elements.Year);
  year -= (month <= 2) ? 1 : 0;

  const int64_t era  = (year >= 0 ? year : year - 399) / 400;
  const uint32_t yoe = static_cast<uint32_t>(year - era * 400);
  const uint32_t doy = (153u * static_cast<uint32_t>(month + (month > 2 ? -3 : 9)) + 2u) / 5u + dayOfMonth;
  const uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  const int64_t days = era * 146097 + static_cast<int64_t>(doe) - 719468;

  const int64_t seconds = days * SECS_PER_DAY +
                          static_cast<int64_t>(elements.Hour) * SECS_PER_HOUR +
                          static_cast<int64_t>(elements.Minute) * SECS_PER_MIN +
                          elements.Second;

  return static_cast<time_t>(seconds);
}

}  // namespace timelib2
