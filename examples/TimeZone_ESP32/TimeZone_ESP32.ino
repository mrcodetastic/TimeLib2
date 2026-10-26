/*
 * TimeZone_ESP32
 *
 * Demonstrates the difference between the UTC epoch returned by now() and the
 * DST-aware local calendar values provided by the component accessors.
 *
 * Replace the WiFi credentials before uploading.
 */

#include <TimeLib2.hpp>
#include <WiFi.h>

static const char* kWifiSsid = "XXXXXXXX";
static const char* kWifiPassword = "YYYYYYYY";

timelib2::Clock timeClock;

static void printTm(const char* label, const struct tm& value) {
  char buffer[40];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", &value);
  Serial.printf("%s: %s\n", label, buffer);
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

  configTime(0, 0, "pool.ntp.org");

  /* Change this to your own POSIX timezone. US Eastern is shown as a second
   * example in the comment below. */
  timeClock.setTimeZone("GMT0BST,M3.5.0/1,M10.5.0");      // Europe/London
  // timeClock.setTimeZone("EST5EDT,M3.2.0/2,M11.1.0/2"); // US Eastern

  Serial.print("Waiting for SNTP");
  while (timeClock.year() < 2020) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
}

void loop() {
  const time_t utcEpoch = timeClock.now();

  struct tm utcTm = {};
  gmtime_r(&utcEpoch, &utcTm);

  struct tm localTm = {};
  timeClock.localTime(utcEpoch, localTm);

  Serial.printf("UTC epoch: %lld\n", static_cast<long long>(utcEpoch));
  printTm("UTC  ", utcTm);
  printTm("Local", localTm);

  /* The component accessors use the same local time as localTime(). */
  Serial.printf("Accessors: %04u-%02u-%02u %02u:%02u:%02u\n",
                static_cast<unsigned>(timeClock.year(utcEpoch)),
                static_cast<unsigned>(timeClock.month(utcEpoch)),
                static_cast<unsigned>(timeClock.day(utcEpoch)),
                static_cast<unsigned>(timeClock.hour(utcEpoch)),
                static_cast<unsigned>(timeClock.minute(utcEpoch)),
                static_cast<unsigned>(timeClock.second(utcEpoch)));
  Serial.println();

  delay(1000);
}
