#include "ESP8266WiFi.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <SocketIOClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
}

// SD --------
File configFile;
File logFile;
int csPin = 15;

// WiFi ------
WiFiClient _client;
//char* ssid;
char* pass;
String host;
String feederName;
int connectAttempts;
SocketIOClient socket;

// RTC -------
RTC_PCF8523 rtc;

// Program variables
String incomingID;

// Timing
unsigned long heartbeatFrequency;
unsigned long lastHeartbeat;
int powerTimeout;

void emitID(String data) {
  Serial.println("Sending feeder id to server...");
  String payload = "{\"feederName\":\"" + feederName + "\"}";
  socket.emit("idTransmit", payload);
}



// SSID to connect to
static const char* ssid = "eduroam";
// Username for authentification
static const char* username = "xxxx@campus.goldsmiths.ac.uk";
// Password for authentification
static const char* password = "xxxxxxxxxxxx";


void setup() {
  //Debug internet stuff
  Serial.begin(9600);
  //Serial.setDebugOutput(true);

  initialiseRTC();
  initialiseSD(csPin);

  //EDUROAM SETTINGS
  wifi_set_opmode(STATION_MODE);
  struct station_config wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  strcpy((char*)wifi_config.ssid, ssid);
  wifi_station_set_config(&wifi_config);
  wifi_station_clear_cert_key();
  wifi_station_clear_enterprise_ca_cert();
  wifi_station_set_wpa2_enterprise_auth(1);
  wifi_station_set_enterprise_identity((uint8*)username, strlen(username));
  wifi_station_set_enterprise_username((uint8*)username, strlen(username));
  wifi_station_set_enterprise_password((uint8*)password, strlen(password));
  wifi_station_set_enterprise_new_password((uint8*)password, strlen(password));
  wifi_station_connect();

  // Wait for connection AND IP address from DHCP
  Serial.println();
  Serial.println("Waiting for connection and IP Address from DHCP");
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(host);
  //Turn on socket
  socket.on("idRequest", emitID);
  socket.connect(host, 80);
  delay(3000);
}

void loop() {
  unsigned long currentMillis = millis();
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == 0x03) {
      storeID(incomingID);
      incomingID = "";
    }
    else incomingID += inChar;
  }

  socket.monitor();
}

void storeID(String id) {
  id.trim();
  id = id.substring(1);
  Serial.println("ID detected!");

  Serial.println("Sending track to server...");

  String status = "Offline";

  if (WiFi.status() == WL_CONNECTED) {
    String payload =
      "{\"feederName\":\"" + feederName +
      "\", \"timedate\":\"" + generateTimestamp(rtc.now()) +
      "\", \"rfid\":\"" + id +
      "\"}";
    socket.emit("newTrack", payload);
    status = "Online";
  }

  logToSD(rtc.now(), id, feederName, status);

  Serial.println("Done.");
  delay(100);
}

void logToSD(DateTime now, String id, String feeder, String wifiStatus) {
  File f = SD.open("log.txt", FILE_WRITE);

  if (f) {
    Serial.println("Opened log file. Creating log...");
    String s =
      generateTimestamp(now) + "," +
      id + "," +
      feeder + "," +
      wifiStatus;
    f.println(s);
    f.close();
    Serial.println("Done writing log to SD.");
  }
}

String generateTimestamp(DateTime now) {
  return String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "-" +
         String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
}

// Initialises the SD card and reads the configuration file.
void initialiseSD(int pin) {
  if (!SD.begin(pin)) {
    Serial.println("SD initialisation failed!");
    defaultSettings();
  }
  else {
    File f;
    Serial.println("SD initialised.");
    f = SD.open("config.txt");

    if (f) {
      Serial.println("Config file found. Loading settings...");
      loadSettings(f);

    }
    else {
      Serial.println("Config file not found! Defaulting...");
      defaultSettings();
    }
  }

}

void logBird(DateTime now, String id, String feeder, String wifiStatus) {
  File f = SD.open("log.txt", FILE_WRITE);

  if (f) {
    Serial.println("Opened log file. Creating log...");
    String s =
      String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "," +
      String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + ":" + "," +
      id + "," +
      feeder + "," +
      wifiStatus;
    f.println(s);
    f.close();
    Serial.println("Done writing log to SD.");
  }
}

// Loads default settings if a config file is not present.
void defaultSettings() {
  ssid = "eduroam";
  pass = " ";
  host = "irs-feedernet.eu-west-1.elasticbeanstalk.com";
  feederName = "studio-test-unit";
  powerTimeout = 20000;
  connectAttempts = 8;
}

// Loads settings from SD card.
void loadSettings(File f) {
  int fileIndex = 0;
  String currentSetting;
  if (f) {
    while (f.available()) {
      char incoming = f.read();
      if (incoming == '\n') {
        saveSetting(fileIndex, currentSetting);
        currentSetting = "";
        fileIndex++;
      }
      else currentSetting += incoming;
    }
  }
}

// Saves an individual setting to RAM.
void saveSetting(int index, String s) {
  s.trim();
  if (index % 2) {
    switch (index) {
      case 1:
        // ssid = s;
        Serial.print("$SSID: ");
        Serial.println(ssid);
        break;
      case 3:
        // pass = s;
        Serial.print("$Password: ");
        Serial.println(pass);
        break;
      case 5:
        host = s;
        Serial.print("$Host: ");
        Serial.println(host);
        break;
      case 7:
        feederName = s;
        Serial.print("$Feeder name: ");
        Serial.println(feederName);
        break;
      case 9:
        connectAttempts = s.toInt();
        Serial.print("$Connect attempts: ");
        Serial.println(connectAttempts);
        break;
      case 11:
        // powerTimeout = String(s).toInt();
        Serial.print("$Power timeout: ");
        Serial.println(powerTimeout);
        break;
    }
  }
}

// Removes white spaces from char array.
char* removeSpaces(char* s) {
  char* cpy = s;  // an alias to iterate through s without moving s
  char* temp = s;

  while (*cpy)
  {
    if (*cpy != 32 && *cpy != 9 && *cpy != 11 && *cpy != 12 && *cpy != 13 && *cpy != 10)
      *temp++ = *cpy;
    cpy++;
  }
  *temp = 0;
  return s;
}

// Initialises the real time clock.
void initialiseRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
  }
  if (!rtc.initialized()) {
    Serial.println("RTC is not running. Setting date and time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}
