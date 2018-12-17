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
#define WLAN_SSID "xxxxxxx"
#define WLAN_PASS "xxxxxxx"
#define HOST "http://feedernet.herokuapp.com"
#define FEEDERSTUB "FeederXX"
#define HTTP_TIMEOUT 5000
#define SLEEP_INTERVAL 2200
#define WAKE_INTERVAL 400
#define NIGHT_SLEEP_INTERVAL 24000
#define WIFI_QUICK_MAX_RETRIES 100
#define WIFI_REGULAR_MAX_RETRIES 600
#define NIGHT_START 17
#define NIGHT_END 6
#define TAG_DEBOUNCE 60
#define TIME_RESYNC_INTERVAL 3600

RFID rfidModule(1.1);

// RTC data
struct {
  uint32_t crc32;
  uint32_t unixTime;
  uint32_t unixTimeRemainder;
  uint16_t channel;
  uint8_t bssid[6];
  uint8_t previousTag[5];
  uint8_t sleeping;
  uint32_t previousTagTime;
  uint8_t timeError;
  uint8_t cachedTags[4][5];
  uint32_t cachedTimes[4];
  uint8_t numOfCachedTags;
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
    powerup();
  }

  DEBUG_PRINT(hour());
  DEBUG_PRINT(" : ");
  DEBUG_PRINT(minute());
  DEBUG_PRINT(" : ");
  DEBUG_PRINTLN(second());

  // Night time
  if (hour() >= NIGHT_START && hour() <= 23) {
    DEBUG_PRINTLN("Night time detected.");
    updateNightTime(); 
  }
  if (hour() >= 0 && hour() < NIGHT_END) {
    DEBUG_PRINTLN("Night time detected - past midnight");
    updateNightTime(); 
  }

  // Time sync before sleep
  if (hour() == NIGHT_START-1 && minute() == 45) {
    prepareForSleep();
  }

  // Check if awaken from night time.
  if (rtcData.sleeping == 1) {
    prepareForDaytime();
  }

  // Check if time error exists
  if (rtcData.timeError == 1) {
    // If time is greater than 0 and is close to the resync interval, try to resync.
    if (now() > 0 && now() % TIME_RESYNC_INTERVAL > 0 && now() % TIME_RESYNC_INTERVAL < 30) {
      resyncTime();
    }
  }

  // Turn on RFID
  digitalWrite(14, LOW);
}

void loop() {
  updateRfid();
  updateSleep();
}
