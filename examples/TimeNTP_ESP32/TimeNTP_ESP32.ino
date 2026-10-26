/*
 * TimeNTP_ESP32
 *
 * ESP32 SNTP sync using the system clock (configTime). TimeLib2 reads the
 * ESP32 system clock directly, so no custom sync provider is required.
 *
 * Replace the WiFi credentials before uploading.
 */

#include <TimeLib2.hpp>
#include <WiFi.h>

static const char* kWifiSsid = "XXXXXXXX";
static const char* kWifiPassword = "YYYYYYYY";

/* Note: avoid naming this variable "clock" - POSIX declares clock() and it
 * would shadow the instance. */
timelib2::Clock timeClock;

static void printLocalTime(time_t utc) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u",
           static_cast<unsigned>(timeClock.year(utc)),
           static_cast<unsigned>(timeClock.month(utc)),
           static_cast<unsigned>(timeClock.day(utc)),
           static_cast<unsigned>(timeClock.hour(utc)),
           static_cast<unsigned>(timeClock.minute(utc)),
           static_cast<unsigned>(timeClock.second(utc)));
  Serial.println(buffer);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.begin(kWifiSsid, kWifiPassword);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  /* Keep the system clock in UTC; TimeLib2 handles the local offset. */
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  /* POSIX TZ: Europe/London, with DST transitions. */
  timeClock.setTimeZone("GMT0BST,M3.5.0/1,M10.5.0");

  Serial.print("Waiting for SNTP");
  while (timeClock.year() < 2020) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  Serial.println(timeClock.timeStatus() == timelib2::TimeStatus::Set
                     ? "Clock is set"
                     : "Clock is not set");
}

void loop() {
  static time_t previous = 0;
  const time_t utc = timeClock.now();

  if (utc != previous) {
    previous = utc;
    printLocalTime(utc);
  }

  delay(200);
}
