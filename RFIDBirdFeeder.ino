// Libraries
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "RFID.h"

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
#define WLAN_SSID "piNet"
#define WLAN_PASS "xxxxxxxx"
#define SLEEP_INTERVAL 500
#define WIFI_QUICK_MAX_RETRIES 100
#define WIFI_REGULAR_MAX_RETRIES 600

// RTC data
struct {
  uint32_t crc32;
  uint32_t unixTime;
  uint16_t channel;
  uint8_t bssid[6];
} rtcData;


// WiFi settings
const char* ssid     = "piNet";
const char* password = "XXXXXXXXXX";

long prevMills;
int interval = 100;
long prevMillsWifi;
int intervalWifi = 10000;
byte count = 0;

RFID rfid(1.1);
byte tagData[5];

void setup() {
  WiFi.mode(WIFI_OFF);
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DEBUG_PRINTLN("Start up");

  readRTCData();

  connectToWiFi();
  
  if(rtcData.unixTime < 1542640000 || rtcData.unixTime > 1900000000) {
    // Forgotten time.
    DEBUG_PRINTLN("Resetting Unix time.");
    rtcData.unixTime = 1542648526;
    writeRTCData();
  }
  else {
    rtcData.unixTime++;
    writeRTCData();
    DEBUG_PRINT("Unix Time: ");
    DEBUG_PRINTLN(rtcData.unixTime);
  }
}

void loop() {
  updateRfid();
  delay(30);
  updateSleep();
}
