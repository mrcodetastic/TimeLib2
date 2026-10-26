/*
 * TimeLib2.cpp - ESP32-first clock implementation.
 *
 * The system clock (time()/settimeofday()) is the single source of truth.
 * A monotonic tick is used only to decide when a sync attempt is due.
 */

#include "TimeLib2.hpp"

#include <stdlib.h>
#include <sys/time.h>

#if defined(ESP32)
#include <esp_timer.h>
#endif

namespace timelib2 {
namespace {

/* Monotonic milliseconds. The value may wrap after ~49 days, which is fine
 * because all comparisons are wrap-safe. */
uint32_t monotonicMs() noexcept {
#if defined(ESP32)
  return static_cast<uint32_t>(esp_timer_get_time() / 1000);
#else
  return static_cast<uint32_t>(millis());
#endif
}

/* Epoch for 2020-01-01T00:00:00Z. Any system time at or after this is treated
 * as a real, externally-set clock (for example by ESP32 configTime/SNTP). */
constexpr time_t kMinValidEpoch = 1577836800;

}  // namespace

/*==============================================================================
 * Synchronisation
 *============================================================================*/

void Clock::maybeSync() noexcept {
  const SyncProvider provider = provider_.load(std::memory_order_acquire);
  const LegacySyncProvider legacy = legacyProvider_.load(std::memory_order_acquire);

  if (provider == nullptr && legacy == nullptr) {
    return;
  }

  const uint32_t nowMs = monotonicMs();
  if (static_cast<int32_t>(nowMs - nextSyncMs_.load(std::memory_order_relaxed)) < 0) {
    return;
  }

  bool expected = false;
  if (!syncInProgress_.compare_exchange_strong(expected, true,
                                               std::memory_order_acq_rel)) {
    return;
  }

  const time_t result =
      (provider != nullptr)
          ? provider(providerContext_.load(std::memory_order_relaxed))
          : legacy();

  if (result > 0) {
    applyTime(result);
  } else if (status_.load(std::memory_order_relaxed) != TimeStatus::NotSet) {
    status_.store(TimeStatus::NeedsSync, std::memory_order_relaxed);
  }

  /* Schedule from the end of the attempt so a slow provider does not cause an
   * immediate second sync. */
  const uint64_t interval =
      static_cast<uint64_t>(syncIntervalSeconds_.load(std::memory_order_relaxed));
  const uint64_t nextMs =
      static_cast<uint64_t>(monotonicMs()) + interval * 1000ull;
  nextSyncMs_.store(static_cast<uint32_t>(nextMs), std::memory_order_relaxed);

  syncInProgress_.store(false, std::memory_order_release);
}

void Clock::applyTime(time_t t) noexcept {
  struct timeval tv;
  tv.tv_sec = t;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  status_.store(TimeStatus::Set, std::memory_order_relaxed);
}

time_t Clock::now() noexcept {
  maybeSync();

  time_t t = time(nullptr);
  if (t < 0) {
    t = 0;
  }

  /* If the system clock was set externally (configTime/SNTP or otherwise),
   * reflect that in the status without requiring a provider. */
  if (t >= kMinValidEpoch &&
      status_.load(std::memory_order_relaxed) == TimeStatus::NotSet) {
    status_.store(TimeStatus::Set, std::memory_order_relaxed);
  }

  return t;
}

time_t Clock::getEpochSecond() noexcept { return now(); }

TimeStatus Clock::timeStatus() const noexcept {
  return status_.load(std::memory_order_relaxed);
}

void Clock::setSyncProvider(SyncProvider provider, void* context) noexcept {
  providerContext_.store(context, std::memory_order_relaxed);
  provider_.store(provider, std::memory_order_release);
  legacyProvider_.store(nullptr, std::memory_order_release);
  nextSyncMs_.store(monotonicMs(), std::memory_order_relaxed);
}

void Clock::setSyncProvider(LegacySyncProvider provider) noexcept {
  provider_.store(nullptr, std::memory_order_release);
  providerContext_.store(nullptr, std::memory_order_relaxed);
  legacyProvider_.store(provider, std::memory_order_release);
  nextSyncMs_.store(monotonicMs(), std::memory_order_relaxed);
}

void Clock::clearSyncProvider() noexcept {
  provider_.store(nullptr, std::memory_order_release);
  providerContext_.store(nullptr, std::memory_order_relaxed);
  legacyProvider_.store(nullptr, std::memory_order_release);
}

void Clock::setSyncProvider(std::nullptr_t) noexcept { clearSyncProvider(); }

void Clock::setSyncInterval(uint32_t seconds) noexcept {
  syncIntervalSeconds_.store(seconds, std::memory_order_relaxed);
  const uint64_t nextMs =
      static_cast<uint64_t>(monotonicMs()) + static_cast<uint64_t>(seconds) * 1000ull;
  nextSyncMs_.store(static_cast<uint32_t>(nextMs), std::memory_order_relaxed);
}

/*==============================================================================
 * Setting the time
 *============================================================================*/

void Clock::setTime(time_t t) noexcept { applyTime(t); }

bool Clock::setTime(int hr, int min, int sec, int day, int month, int year) noexcept {
  if (hr < 0 || hr > 23 || min < 0 || min > 59 || sec < 0 || sec > 59) {
    return false;
  }
  if (month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }

  int fullYear = year;
  if (fullYear >= 0 && fullYear < 100) {
    fullYear += 2000;
  }
  if (fullYear < 1970 || fullYear > 2225) {
    return false;
  }

  time_t epoch;
  if (tzConfigured_.load(std::memory_order_relaxed)) {
    struct tm local = {};
    local.tm_sec = sec;
    local.tm_min = min;
    local.tm_hour = hr;
    local.tm_mday = day;
    local.tm_mon = month - 1;
    local.tm_year = fullYear - 1900;
    local.tm_isdst = -1;
    epoch = mktime(&local);
    if (epoch == static_cast<time_t>(-1)) {
      return false;
    }
  } else {
    TimeElements elements;
    elements.Second = static_cast<uint8_t>(sec);
    elements.Minute = static_cast<uint8_t>(min);
    elements.Hour = static_cast<uint8_t>(hr);
    elements.Day = static_cast<uint8_t>(day);
    elements.Month = static_cast<uint8_t>(month);
    elements.Year = static_cast<uint8_t>(fullYear - 1970);
    epoch = makeTime(elements);
  }

  applyTime(epoch);
  return true;
}

/*==============================================================================
 * Local time
 *============================================================================*/

void Clock::setOffset(time_t adjustment) noexcept {
  offsetSecs_.store(static_cast<int32_t>(adjustment), std::memory_order_relaxed);
  tzConfigured_.store(false, std::memory_order_relaxed);
}

time_t Clock::offset() const noexcept {
  return static_cast<time_t>(offsetSecs_.load(std::memory_order_relaxed));
}

void Clock::setTimeZone(const char* posixTz) noexcept {
  if (posixTz != nullptr && posixTz[0] != '\0') {
    setenv("TZ", posixTz, 1);
    tzConfigured_.store(true, std::memory_order_relaxed);
  } else {
    unsetenv("TZ");
    tzConfigured_.store(false, std::memory_order_relaxed);
  }
  tzset();
}

void Clock::clearTimeZone() noexcept {
  unsetenv("TZ");
  tzset();
  tzConfigured_.store(false, std::memory_order_relaxed);
}

TimeElements Clock::elementsFor(time_t t) const noexcept {
  TimeElements elements;

  if (tzConfigured_.load(std::memory_order_relaxed)) {
    struct tm local = {};
    if (localtime_r(&t, &local) != nullptr) {
      elements.Second = static_cast<uint8_t>(local.tm_sec);
      elements.Minute = static_cast<uint8_t>(local.tm_min);
      elements.Hour = static_cast<uint8_t>(local.tm_hour);
      elements.Wday = static_cast<uint8_t>(local.tm_wday + 1);
      elements.Day = static_cast<uint8_t>(local.tm_mday);
      elements.Month = static_cast<uint8_t>(local.tm_mon + 1);
      int yearOffset = (local.tm_year + 1900) - 1970;
      if (yearOffset < 0) {
        yearOffset = 0;
      } else if (yearOffset > 255) {
        yearOffset = 255;
      }
      elements.Year = static_cast<uint8_t>(yearOffset);
      return elements;
    }
  }

  breakTime(t + offsetSecs_.load(std::memory_order_relaxed), elements);
  return elements;
}

bool Clock::localTime(time_t t, struct tm& out) const noexcept {
  if (tzConfigured_.load(std::memory_order_relaxed)) {
    return localtime_r(&t, &out) != nullptr;
  }

  const time_t shifted = t + static_cast<time_t>(offsetSecs_.load(std::memory_order_relaxed));
  return gmtime_r(&shifted, &out) != nullptr;
}

/*==============================================================================
 * Component accessors
 *============================================================================*/

uint8_t Clock::hour() noexcept { return hour(now()); }
uint8_t Clock::hour(time_t t) const noexcept { return elementsFor(t).Hour; }

uint8_t Clock::minute() noexcept { return minute(now()); }
uint8_t Clock::minute(time_t t) const noexcept { return elementsFor(t).Minute; }

uint8_t Clock::second() noexcept { return second(now()); }
uint8_t Clock::second(time_t t) const noexcept { return elementsFor(t).Second; }

uint8_t Clock::day() noexcept { return day(now()); }
uint8_t Clock::day(time_t t) const noexcept { return elementsFor(t).Day; }

uint8_t Clock::weekday() noexcept { return weekday(now()); }
uint8_t Clock::weekday(time_t t) const noexcept { return elementsFor(t).Wday; }

uint8_t Clock::month() noexcept { return month(now()); }
uint8_t Clock::month(time_t t) const noexcept { return elementsFor(t).Month; }

uint16_t Clock::year() noexcept { return year(now()); }
uint16_t Clock::year(time_t t) const noexcept {
  return static_cast<uint16_t>(1970 + elementsFor(t).Year);
}

uint8_t Clock::hourFormat12() noexcept { return hourFormat12(now()); }
uint8_t Clock::hourFormat12(time_t t) const noexcept {
  const uint8_t h = hour(t);
  if (h == 0) {
    return 12;
  }
  return (h > 12) ? static_cast<uint8_t>(h - 12) : h;
}

bool Clock::isAM() noexcept { return isAM(now()); }
bool Clock::isAM(time_t t) const noexcept { return !isPM(t); }

bool Clock::isPM() noexcept { return isPM(now()); }
bool Clock::isPM(time_t t) const noexcept { return hour(t) >= 12; }

/*==============================================================================
 * Static conversion wrappers
 *============================================================================*/

void Clock::breakTime(time_t t, TimeElements& elements) noexcept {
  timelib2::breakTime(t, elements);
}

time_t Clock::makeTime(const TimeElements& elements) noexcept {
  return timelib2::makeTime(elements);
}

}  // namespace timelib2
