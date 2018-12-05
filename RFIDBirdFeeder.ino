// Libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "naturewatch_RFID.h"

// DEBUG - uncomment for debug info via serial
#define DEBUG

// Debug print macros
#ifdef DEBUG
#define DEBUG_PRINTLN(x)  Serial.println(x)
#else
#define DEBUG_PRINTLN(x)
#endif
#ifdef DEBUG
#define DEBUG_PRINT(x)  Serial.print(x)
#else
#define DEBUG_PRINT(x)
#endif
#ifdef DEBUG
#define DEBUG_PRINTHEX(x)  Serial.print(x, 16)
#else
#define DEBUG_PRINTHEX(x)
#endif

// CONFIG DEFINES
#define WLAN_SSID "IRS Wireless"
#define WLAN_PASS "xxxxxxxxxx"
#define HOST "http://feedernet.herokuapp.com"
#define FEEDERSTUB "TestFeeder"
#define HTTP_TIMEOUT 5000
#define SLEEP_INTERVAL 500
#define NIGHT_SLEEP_INTERVAL 1800000
#define WIFI_QUICK_MAX_RETRIES 100
#define WIFI_REGULAR_MAX_RETRIES 600
#define NIGHT_START 20
#define NIGHT_END 4

RFID rfidModule(1.1);

// RTC data
struct {
  uint32_t crc32;
  uint32_t unixTime;
  uint32_t unixTimeRemainder;
  uint16_t channel;
  uint8_t bssid[6];
  uint8_t previousTag[5];
  uint8_t rtcBuffer[3];
} rtcData;

long prevMills;
int interval = 100;
long prevMillsWifi;
int intervalWifi = 10000;
byte count = 0;
boolean isNightTime = false;

byte tagData[5];

void setup() {
  WiFi.mode(WIFI_OFF);
  digitalWrite(14, HIGH);
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DEBUG_PRINTLN("Start up");
  DEBUG_PRINT("Reset reason: ");
  DEBUG_PRINTLN(ESP.getResetReason());

  readRTCData();
  setTime(getUnixTime());

  if (ESP.getResetReason() != "Deep-Sleep Wake") {
    DEBUG_PRINTLN("Reset from Powerup. Getting time...");
    connectToWiFi();
    rtcData.unixTime = getTime();
    rtcData.unixTimeRemainder = 0;
    setTime(rtcData.unixTime);
    DEBUG_PRINT(hour());
    DEBUG_PRINT(" : ");
    DEBUG_PRINTLN(minute());
    sendPing();
  }

  DEBUG_PRINT(hour());
  DEBUG_PRINT(" : ");
  DEBUG_PRINT(minute());
  DEBUG_PRINT(" : ");
  DEBUG_PRINTLN(second());

  if (hour() >= NIGHT_START && hour() <= 23) {
    DEBUG_PRINTLN("Night time detected.");
    updateNightTime(); 
  }
  if (hour() >= 0 && hour() < NIGHT_END) {
    DEBUG_PRINTLN("Night time detected - past midnight");
    updateNightTime(); 
  }

  if (hour() == NIGHT_START-1 && minute() == 40) {
    DEBUG_PRINTLN("Getting time before sleep...");
    connectToWiFi();
    rtcData.unixTime = getTime();
    rtcData.unixTimeRemainder = 0;
    setTime(rtcData.unixTime);
    DEBUG_PRINT(hour());
    DEBUG_PRINT(" : ");
    DEBUG_PRINTLN(minute());
    sendPing();
  }

  // Turn on RFID
  digitalWrite(14, LOW);
}

void loop() {
  updateRfid();
  delay(30);
  updateSleep();
}
