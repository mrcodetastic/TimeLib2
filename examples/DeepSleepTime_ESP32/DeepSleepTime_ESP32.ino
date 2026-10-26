/*
 * DeepSleepTime_ESP32
 *
 * Shows that ESP32 system time is kept across deep sleep, so TimeLib2 keeps
 * reporting the correct time after waking without re-syncing.
 *
 * On the first boot the example syncs via SNTP. It then sleeps for 10 seconds
 * and wakes repeatedly. Replace the WiFi credentials before uploading.
 */

#include <TimeLib2.hpp>
#include <WiFi.h>
#include <esp_sleep.h>

static const char* kWifiSsid = "XXXXXXXX";
static const char* kWifiPassword = "YYYYYYYY";

timelib2::Clock timeClock;

RTC_DATA_ATTR static int bootCount = 0;

static void waitForSntp() {
  WiFi.begin(kWifiSsid, kWifiPassword);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  configTime(0, 0, "pool.ntp.org");

  Serial.print("Waiting for SNTP");
  while (timeClock.year() < 2020) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  ++bootCount;
  Serial.printf("Boot #%d, wake cause: %d\n", bootCount,
                static_cast<int>(esp_sleep_get_wakeup_cause()));

  if (bootCount == 1) {
    waitForSntp();
  }

  timeClock.setTimeZone("UTC0");
  const time_t utc = timeClock.now();

  Serial.printf("UTC epoch: %lld\n", static_cast<long long>(utc));
  Serial.printf("Local: %04u-%02u-%02u %02u:%02u:%02u\n",
                static_cast<unsigned>(timeClock.year(utc)),
                static_cast<unsigned>(timeClock.month(utc)),
                static_cast<unsigned>(timeClock.day(utc)),
                static_cast<unsigned>(timeClock.hour(utc)),
                static_cast<unsigned>(timeClock.minute(utc)),
                static_cast<unsigned>(timeClock.second(utc)));
  Serial.flush();

  esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
  Serial.println("Entering deep sleep for 10 seconds...");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
  // Never reached: setup() always enters deep sleep.
}
