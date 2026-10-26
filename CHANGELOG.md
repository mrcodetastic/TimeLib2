# Changelog

All notable changes to this project are documented here. This project follows
[Semantic Versioning](https://semver.org/).

## [2.0.0] - 2026-10-26

### Changed (breaking)

- All public symbols now live in `namespace timelib2`; the global namespace is
  no longer polluted.
- The primary class is `timelib2::Clock`; `timelib2::TimeLib2` is provided as an
  alias.
- `timeStatus_t`, `tmElements_t`, `getExternalTime`, and the status/day enums
  are now scoped aliases inside `timelib2`.
- Time constants and conversion helpers are `constexpr` functions/values instead
  of macros.
- `getEpochSecond()`/`now()` return UTC epoch seconds. A fixed `setOffset()` no
  longer changes the returned epoch; it only affects calendar component
  accessors and `localTime()`.
- `breakTime()`/`makeTime()` are static and `breakTime()` takes a reference
  (`TimeElements&`) rather than a pointer.
- `isAM()`/`isPM()` return `bool`; component accessors return `uint8_t`;
  `year()` returns `uint16_t`.
- Date string helpers return `const char*` to static literals, are reentrant,
  bounds-checked, and no longer use a shared mutable buffer or `PROGMEM`.
- `setTime(hr, min, sec, day, month, year)` validates its arguments and returns
  `bool`; two-digit years map to 2000..2099 and four-digit years must be
  1970..2225.

### Added

- ESP32-native time backend based on `time()`/`settimeofday()` and
  `esp_timer_get_time()`.
- 64-bit `time_t` support (no hand-rolled typedef, no 2038 truncation).
- Optional sync provider with a `void*` context parameter; legacy no-context
  providers remain supported.
- POSIX timezone/DST support via `setTimeZone()` and `localTime()`.
- `elementsFor()` helper for local calendar access.
- `clearSyncProvider()` and `clearTimeZone()`.
- Host-side unit tests for the conversion layer.
- ESP32 examples for SNTP, timezones, and deep sleep.

### Fixed

- Replaced the `millis()` busy-wait catch-up loop with arithmetic scheduling,
  eliminating watchdog risk after long blocking calls or sleep.
- Removed the incorrect `time_t` typedef guard that conflicted with newlib.
- Fixed the broken `minutesToTime_t`/`hoursToTime_t`/`daysToTime_t`/
  `weeksToTime_t` macros.
- Fixed out-of-bounds date string lookups.
- Fixed the `TimeLib2::TimeLib2::minute()` double qualification.
- `setOffset()` no longer leaves a stale sync deadline.
- `timeStatus()` no longer triggers a blocking sync as a side effect.
- Removed the ESP8266 `PROGMEM` workaround.

### Packaging

- Corrected `includes=TimeLib2.hpp`.
- Added license and platform metadata.
- Updated `keywords.txt` and the README, including a v1 migration guide.
