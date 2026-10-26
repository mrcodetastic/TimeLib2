/*
 * DateStrings.cpp - reentrant date string helpers for TimeLib2.
 *
 * The strings are static literals, so no shared mutable buffer is required.
 * On ESP32 string literals live in the memory-mapped flash and PROGMEM is
 * unnecessary.
 */

#include "TimeConversions.hpp"

namespace timelib2 {
namespace {

const char* const kMonthNames[13] = {
    "",        "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",  "September", "October",  "November", "December"};

const char* const kMonthShortNames[13] = {
    "",   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

const char* const kDayNames[8] = {
    "Err", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const char* const kDayShortNames[8] = {
    "Err", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

}  // namespace

const char* monthStr(uint8_t month) noexcept {
  return (month >= 1 && month <= 12) ? kMonthNames[month] : "";
}

const char* monthShortStr(uint8_t month) noexcept {
  return (month >= 1 && month <= 12) ? kMonthShortNames[month] : "";
}

const char* dayStr(uint8_t day) noexcept {
  return (day >= 1 && day <= 7) ? kDayNames[day] : "Err";
}

const char* dayShortStr(uint8_t day) noexcept {
  return (day >= 1 && day <= 7) ? kDayShortNames[day] : "Err";
}

}  // namespace timelib2
