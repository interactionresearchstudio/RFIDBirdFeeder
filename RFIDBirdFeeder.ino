// Libraries

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "naturewatch_RFID.h"

// PI_BRIDGE - uncomment for wifi settings for programming via serial from the PI Bridge
#define PI_BRIDGE

// DEBUG - uncomment for debug info via serial
//#define DEBUG

// Uncomment to send data through LoRa module instead of WiFi.
//#define LORA

// LoRa software serial
//#ifdef LORA
#include <SoftwareSerial.h>
SoftwareSerial lora = SoftwareSerial(4, 5);
#define LORA_REQUEST_TIMEOUT 4000
#define LORA_REQUEST_ATTEMPTS 3
#define RADIOID 1
//#endif

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
char WLAN_SSID[32];
char WLAN_PASS[32];
#define HOST "http://raspberrypi.local:4000"
String FEEDERSTUB = " ";
#define HTTP_TIMEOUT 5000
#define SLEEP_INTERVAL 4000
#define WAKE_INTERVAL 250
#define NIGHT_SLEEP_INTERVAL 24000
#define WIFI_QUICK_MAX_RETRIES 100
#define WIFI_REGULAR_MAX_RETRIES 600
#define TAG_DEBOUNCE 60
#define TIME_RESYNC_INTERVAL 3600
#define REQUEST_RETRIES 2
#define VERSION "v2.0"

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
  uint8_t NIGHT_START_HOUR;
  uint8_t NIGHT_END_HOUR;
  uint8_t NIGHT_START_MINUTE;
  uint8_t NIGHT_END_MINUTE;

} rtcData;

long prevMills;
int interval = 100;
long prevMillsWifi;
int intervalWifi = 10000;
byte count = 0;
boolean isNightTime = false;


const char* lat = "";
const char* lon = "";

byte tagData[5];

void setup() {
  FEEDERSTUB = WiFi.macAddress();
  WiFi.mode(WIFI_OFF);
  digitalWrite(14, HIGH);
#ifdef DEBUG
  Serial.begin(115200);
  DEBUG_PRINTLN(" ");
  DEBUG_PRINT(VERSION);
  DEBUG_PRINT(" | MAC: ");
  DEBUG_PRINTLN(FEEDERSTUB);
  DEBUG_PRINT("Reset reason: ");
  DEBUG_PRINTLN(ESP.getResetReason());
#endif

#ifdef PI_BRIDGE
  Serial.begin(115200);
  Serial.println(FEEDERSTUB);
#endif

#ifdef LORA
  lora.begin(19200);
#endif

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
  if (hour() >= rtcData.NIGHT_START_HOUR && hour() <= 23) {
    DEBUG_PRINTLN("Night time detected.");
    updateNightTime();
  }
  if (hour() >= 0 && hour() < rtcData.NIGHT_END_HOUR) {
    DEBUG_PRINTLN("Night time detected - past midnight");
    updateNightTime();
  }

  // Time sync before sleep
  if (hour() == rtcData.NIGHT_START_HOUR && rtcData.NIGHT_START_MINUTE > 15 && minute() == rtcData.NIGHT_START_MINUTE - 15 ) {
    prepareForSleep();
  } else if (rtcData.NIGHT_START_MINUTE < 15 && hour() == rtcData.NIGHT_START_HOUR - 1 && minute() == rtcData.NIGHT_START_MINUTE + 45) {
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
  rfidModule.isModuleReady();
  updateSleep();
}
