/*
 * TimeNTP_ESP8266WiFi
 *
 * Legacy custom-provider example, kept to demonstrate a no-context
 * setSyncProvider() callback. TimeLib2 v2 officially targets ESP32
 * (architectures=esp32); this sketch is provided as a reference only.
 */

#include <TimeLib2.hpp>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

static const char ssid[] = "XXXXXXXX";      // your network SSID (name)
static const char pass[] = "YYYYYYYY";      // your network password

static const char ntpServerName[] = "us.pool.ntp.org";

const int timeZone = 1;  // hours ahead of UTC

WiFiUDP Udp;
timelib2::Clock timeClock;

static unsigned int localPort = 8888;

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress& address);

void setup() {
  Serial.begin(9600);
  delay(250);
  Serial.println("TimeNTP Example");

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());

  timeClock.setOffset(timeZone * timelib2::SECS_PER_HOUR);
  timeClock.setSyncProvider(getNtpTime);
  timeClock.setSyncInterval(300);

  Serial.println("Waiting for sync");
  timeClock.getEpochSecond();  // drive the first sync attempt
}

time_t prevDisplay = 0;

void loop() {
  const time_t utc = timeClock.getEpochSecond();

  if (timeClock.timeStatus() != timelib2::TimeStatus::NotSet && utc != prevDisplay) {
    prevDisplay = utc;
    digitalClockDisplay();
  }
}

void digitalClockDisplay() {
  Serial.print(timeClock.hour());
  printDigits(timeClock.minute());
  printDigits(timeClock.second());
  Serial.print(' ');
  Serial.print(timeClock.day());
  Serial.print('.');
  Serial.print(timeClock.month());
  Serial.print('.');
  Serial.print(timeClock.year());
  Serial.println();
}

void printDigits(int digits) {
  Serial.print(':');
  if (digits < 10) {
    Serial.print('0');
  }
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

time_t getNtpTime() {
  IPAddress ntpServerIP;

  while (Udp.parsePacket() > 0) {
  }

  Serial.println("Transmit NTP Request");
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);

  const uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    const int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);

      unsigned long secsSince1900 = static_cast<unsigned long>(packetBuffer[40]) << 24;
      secsSince1900 |= static_cast<unsigned long>(packetBuffer[41]) << 16;
      secsSince1900 |= static_cast<unsigned long>(packetBuffer[42]) << 8;
      secsSince1900 |= static_cast<unsigned long>(packetBuffer[43]);

      return static_cast<time_t>(secsSince1900 - 2208988800UL);
    }
  }

  Serial.println("No NTP Response :-(");
  return 0;
}

void sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
