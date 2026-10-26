# TimeLib2

TimeLib2 is an **ESP32-only** time and date library. It provides class-based
clocks on top of the ESP32 system clock, is fully namespace-scoped, supports
64-bit `time_t`, and integrates naturally with ESP32 SNTP (`configTime`) as
well as custom time providers.

```cpp
#include <TimeLib2.hpp>

timelib2::Clock timeClock;

void setup() {
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org");                    // ESP32 SNTP, UTC
  timeClock.setTimeZone("GMT0BST,M3.5.0/1,M10.5.0");   // Europe/London + DST
}

void loop() {
  time_t utc = timeClock.now();                        // UTC epoch seconds
  Serial.printf("%04u-%02u-%02u %02u:%02u:%02u\n",
                static_cast<unsigned>(timeClock.year(utc)),
                static_cast<unsigned>(timeClock.month(utc)),
                static_cast<unsigned>(timeClock.day(utc)),
                static_cast<unsigned>(timeClock.hour(utc)),
                static_cast<unsigned>(timeClock.minute(utc)),
                static_cast<unsigned>(timeClock.second(utc)));
  delay(1000);
}
```

> **Naming note:** do not name a `Clock` instance `clock`. POSIX declares a
> `clock()` function, and the variable would shadow it. Use names such as
> `timeClock`, `rtcClock`, or `localClock`.

## Highlights

- **No global namespace pollution.** Everything lives in `namespace timelib2`.
- **ESP32-native.** Uses `time()` / `settimeofday()`; cooperates with
  `configTime()`/SNTP and survives deep sleep when the RTC keeps time.
- **64-bit `time_t`.** No hand-rolled typedef and no 2038 truncation.
- **Multiple clocks.** Each `Clock` has its own offset, status, and sync
  provider.
- **POSIX timezone/DST.** `setTimeZone()` accepts standard `TZ` strings.
- **Context-aware sync providers.** `setSyncProvider(provider, context)` binds
  per-instance state; legacy no-context providers are also supported.
- **Thread-friendly.** Getters are stateless; sync bookkeeping uses atomics.
- **Host-testable core.** Conversions are free of Arduino dependencies.

## Requirements

- **ESP32 only.** The package metadata declares `architectures=esp32`
  (`library.properties`) and `platforms: ["espressif32"]` (`library.json`).
- ESP32 Arduino core 2.x or 3.x.
- C++11 or newer (the bundled PlatformIO config uses gnu++17).

## Installation

Arduino Library Manager: search for **TimeLib2** by **Faptastic**.

PlatformIO:

```ini
lib_deps = mrfaptastic/TimeLib2
```

## Time model

| Function | Meaning |
| --- | --- |
| `now()`, `getEpochSecond()` | Current **UTC** Unix epoch seconds. |
| `hour()`, `minute()`, `second()`, `day()`, `month()`, `year()`, `weekday()` | Calendar values in the configured **local** time. |
| `setTime(t)` | Sets the system clock to UTC epoch `t`. |
| `setTime(hr, min, sec, day, month, year)` | Sets the system clock from calendar fields; returns `bool`. |
| `setOffset(seconds)` | Fixed local offset used by calendar accessors. |
| `setTimeZone(posixTz)` | POSIX timezone (DST-aware) used by calendar accessors. |
| `localTime(t, tm)` | Fills a POSIX `struct tm` using TZ or the fixed offset. |
| `elementsFor(t)` | Returns local `TimeElements` for `t`. |

> **v2 semantics:** `now()` and `getEpochSecond()` return UTC. A fixed
> `setOffset()` affects calendar accessors and `localTime()`, not the epoch
> returned by `now()`.

The component accessors accept an explicit epoch (`hour(t)`, `day(t)`, ...) so a
single timestamp can be read consistently even if the clock rolls over between
calls.

## Timezones and DST

Use a POSIX `TZ` string for automatic daylight-saving transitions:

```cpp
timeClock.setTimeZone("GMT0BST,M3.5.0/1,M10.5.0");  // Europe/London
timeClock.setTimeZone("EST5EDT,M3.2.0/2,M11.1.0/2"); // US Eastern
timeClock.setTimeZone("UTC0");                        // UTC
```

For a simple, DST-free offset:

```cpp
timeClock.setOffset(2 * timelib2::SECS_PER_HOUR);     // UTC+2
```

> `TZ` is process-global. If multiple `Clock` instances are used, prefer fixed
> offsets per instance or keep a single POSIX timezone for the application.

## Synchronisation

### ESP32 SNTP (recommended)

```cpp
configTime(0, 0, "pool.ntp.org", "time.nist.gov");
```

The system clock is updated by the ESP32 SNTP client; `Clock` reads it
directly. `timeStatus()` becomes `TimeStatus::Set` once the clock is valid.

### Custom provider

```cpp
time_t readRtc(void* context) {
  auto* rtc = static_cast<MyRtc*>(context);
  return rtc->readEpoch();
}

timeClock.setSyncProvider(readRtc, &myRtc);
timeClock.setSyncInterval(600);  // seconds between attempts
```

The provider is called from `now()` when a sync is due. Returning `0` marks the
status as `TimeStatus::NeedsSync`. Legacy providers with no context are also
supported:

```cpp
time_t getNtpTime();
timeClock.setSyncProvider(getNtpTime);
```

`timeStatus()` itself never blocks; sync attempts are driven by `now()` /
`getEpochSecond()`.

## Low-level conversion

```cpp
timelib2::TimeElements tm;
timeClock.breakTime(epoch, tm);         // UTC breakdown (reference, not pointer)
time_t epoch2 = timeClock.makeTime(tm); // UTC assembly
```

`breakTime()` and `makeTime()` are static and dependency-free, so they can be
used (and tested) outside Arduino.

## Date strings

```cpp
timelib2::monthStr(1);       // "January"
timelib2::monthShortStr(1);  // "Jan"
timelib2::dayStr(4);         // "Wednesday"
timelib2::dayShortStr(4);    // "Wed"
```

The returned pointers reference static literals: reentrant, thread-safe, and
bounds-checked for invalid indices.

## Multiple clocks

```cpp
timelib2::Clock localClock;
timelib2::Clock utcClock;

localClock.setOffset(0);       // UTC
utcClock.setTimeZone("UTC0");  // also UTC, but via TZ
```

Each instance keeps independent offset, status, and sync provider. The POSIX
timezone is process-global.

## Migration from v1

| v1 | v2 |
| --- | --- |
| `TimeLib2 clock;` | `timelib2::Clock timeClock;` (or `timelib2::TimeLib2`) |
| `timeNotSet` | `timelib2::TimeStatus::NotSet` (alias `timelib2::timeNotSet`) |
| `tmElements_t` | `timelib2::TimeElements` (alias `timelib2::tmElements_t`) |
| `SECS_PER_MIN` etc. | `timelib2::SECS_PER_MIN` etc. |
| `getEpochSecond()` includes offset | `getEpochSecond()` returns UTC; offset affects local accessors |
| `setOffset(x)` changes epoch | `setOffset(x)` changes local calendar values only |
| `breakTime(t, &tm)` | `timeClock.breakTime(t, tm)` (reference) |
| `makeTime(&tm)` | `timelib2::Clock::makeTime(tm)` |
| `monthStr(m)` | `timelib2::monthStr(m)` |
| `timeStatus_t` | `timelib2::TimeStatus` (alias `timelib2::timeStatus_t`) |

The broken `minutesToTime_t` / `hoursToTime_t` / `daysToTime_t` /
`weeksToTime_t` macros are now `constexpr` functions and actually compile:
`timelib2::minutesToTime_t(5)`.

## Examples

- `examples/TimeNTP_ESP32` - SNTP sync and a live clock.
- `examples/TimeZone_ESP32` - UTC vs POSIX local time with DST.
- `examples/DeepSleepTime_ESP32` - time persistence across deep sleep.
- `examples/TimeNTP_ESP8266WiFi` - custom NTP sync provider (legacy provider).

## Tests and CI

Host-side conversion tests live in `test/` and run with:

```sh
pio test -e native
```

GitHub Actions runs the native tests and compiles the examples for `esp32dev`.

## License

LGPL-2.1-or-later. See [LICENSE](LICENSE).

Based on a fork of the Arduino Time library by Paul Stoffregen:
https://github.com/PaulStoffregen/Time
