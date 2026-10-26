#pragma once

/*
 * TimeLib2 v2 - ESP32-first time and date library.
 *
 * All public names live in namespace timelib2. The library uses the system
 * clock (time()/settimeofday()) as the source of truth, so it cooperates with
 * ESP32 SNTP (configTime) and survives deep sleep when the RTC keeps time.
 *
 * Typical use:
 *
 *     timelib2::Clock clock;
 *     clock.setTimeZone("GMT0BST,M3.5.0/1,M10.5.0");  // Europe/London
 *     // ... after SNTP sync ...
 *     Serial.println(clock.hour());
 */

#include <Arduino.h>
#include <atomic>
#include <cstddef>
#include <time.h>

#include "TimeConversions.hpp"

namespace timelib2 {

class Clock {
 public:
  /** Provider called by now() when a sync is due. May return 0 on failure. */
  using SyncProvider = time_t (*)(void* context);

  /** Legacy provider with no context argument. */
  using LegacySyncProvider = time_t (*)();

  Clock() = default;
  ~Clock() = default;

  Clock(const Clock&) = delete;
  Clock& operator=(const Clock&) = delete;

  /*----------------------------------------------------------------------------
   * Current time (UTC epoch seconds)
   *--------------------------------------------------------------------------*/

  /** Return the current UTC epoch and attempt a sync if one is due. */
  time_t now() noexcept;

  /** Alias for now(), retained from TimeLib2 v1. */
  time_t getEpochSecond() noexcept;

  /*----------------------------------------------------------------------------
   * Calendar component accessors
   *
   * These return values in the configured local time: either a POSIX timezone
   * set with setTimeZone(), or the fixed offset set with setOffset().
   *--------------------------------------------------------------------------*/

  uint8_t hour() noexcept;
  uint8_t hour(time_t t) const noexcept;

  uint8_t hourFormat12() noexcept;
  uint8_t hourFormat12(time_t t) const noexcept;

  bool isAM() noexcept;
  bool isAM(time_t t) const noexcept;
  bool isPM() noexcept;
  bool isPM(time_t t) const noexcept;

  uint8_t minute() noexcept;
  uint8_t minute(time_t t) const noexcept;

  uint8_t second() noexcept;
  uint8_t second(time_t t) const noexcept;

  uint8_t day() noexcept;
  uint8_t day(time_t t) const noexcept;

  uint8_t weekday() noexcept;
  uint8_t weekday(time_t t) const noexcept;

  uint8_t month() noexcept;
  uint8_t month(time_t t) const noexcept;

  uint16_t year() noexcept;
  uint16_t year(time_t t) const noexcept;

  /** Break an epoch value into local calendar elements. */
  TimeElements elementsFor(time_t t) const noexcept;

  /** Fill a POSIX struct tm using TZ (if configured) or the fixed offset. */
  bool localTime(time_t t, struct tm& out) const noexcept;

  /*----------------------------------------------------------------------------
   * Setting the time
   *--------------------------------------------------------------------------*/

  /** Set the system clock from a UTC epoch value. */
  void setTime(time_t t) noexcept;

  /**
   * Set the system clock from calendar fields.
   *
   * Accepts two-digit years (0..99 -> 2000..2099) or four-digit years
   * (1970..2225). Returns false if any field is out of range.
   */
  bool setTime(int hr, int min, int sec, int day, int month, int year) noexcept;

  /*----------------------------------------------------------------------------
   * Synchronisation
   *--------------------------------------------------------------------------*/

  /** Return the cached status. now()/getEpochSecond() drives sync attempts. */
  TimeStatus timeStatus() const noexcept;

  /** Install a provider with an optional user context. */
  void setSyncProvider(SyncProvider provider, void* context = nullptr) noexcept;

  /** Install a legacy no-context provider. */
  void setSyncProvider(LegacySyncProvider provider) noexcept;

  /** Remove any configured sync provider. */
  void clearSyncProvider() noexcept;

  /** Allow setSyncProvider(nullptr) to clear the provider unambiguously. */
  void setSyncProvider(std::nullptr_t) noexcept;

  /** Set the minimum number of seconds between sync attempts. */
  void setSyncInterval(uint32_t seconds) noexcept;

  /*----------------------------------------------------------------------------
   * Local time configuration
   *--------------------------------------------------------------------------*/

  /** Apply a fixed offset (in seconds) to component accessors. */
  void setOffset(time_t adjustment) noexcept;

  /** Return the fixed offset in seconds. */
  time_t offset() const noexcept;

  /**
   * Configure a POSIX TZ string, e.g. "GMT0BST,M3.5.0/1,M10.5.0".
   * TZ is process-global; component accessors become DST-aware.
   */
  void setTimeZone(const char* posixTz) noexcept;

  /** Clear the POSIX timezone and fall back to the fixed offset. */
  void clearTimeZone() noexcept;

  /*----------------------------------------------------------------------------
   * Low-level conversion (static, no instance state)
   *--------------------------------------------------------------------------*/

  static void breakTime(time_t t, TimeElements& elements) noexcept;
  static time_t makeTime(const TimeElements& elements) noexcept;

 private:
  void maybeSync() noexcept;
  void applyTime(time_t t) noexcept;

  std::atomic<int32_t> offsetSecs_{0};
  std::atomic<bool> tzConfigured_{false};
  std::atomic<uint32_t> syncIntervalSeconds_{300};
  std::atomic<uint32_t> nextSyncMs_{0};
  std::atomic<bool> syncInProgress_{false};
  std::atomic<TimeStatus> status_{TimeStatus::NotSet};
  std::atomic<SyncProvider> provider_{nullptr};
  std::atomic<void*> providerContext_{nullptr};
  std::atomic<LegacySyncProvider> legacyProvider_{nullptr};
};

/** Discoverability alias: timelib2::TimeLib2 works like timelib2::Clock. */
using TimeLib2 = Clock;

}  // namespace timelib2
