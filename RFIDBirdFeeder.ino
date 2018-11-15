#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define DEBUG
#include "naturewatch_RFID.h"
WiFiUDP Udp;

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
  
const char* ssid     = "piNet";
const char* password = "XXXXXXXXXX";
static unsigned int localUdpPort = 4210;
char incomingPacket[20];
char  replyPacket[] = "Hi there!";

long prevMills;
int interval = 100;
long prevMillsWifi;
int intervalWifi = 10000;
byte count = 0;

RFID myRFIDuino(1.1);
byte tagData[5]; //Holds the ID numbers from the tag

void setup() {
  WiFi.mode(WIFI_OFF);
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DEBUG_PRINTLN("start up");
}

void loop() {
  
  //scan for a tag - if a tag is sucesfully scanned, return a 'true' and proceed
  if (myRFIDuino.scanForTag(tagData) == true)
  {
    digitalWrite(14, 1);
    DEBUG_PRINTLN("RFID Tag ID:"); //print a header to the Serial port.
    //loop through the byte array
    for (int n = 0; n < 5; n++)
    {
      DEBUG_PRINTHEX(tagData[n]); //print the byte in Decimal format
      if (n < 4) //only print the comma on the first 4 nunbers
      {
        DEBUG_PRINT(",");
      }
    }
    DEBUG_PRINT("\n\r");//return character for next line
  //  sendWIFI();
  }
  delay(30);
  if (millis() > 400) {
    prevMills = millis();
    ESP.deepSleep(400000);
  }
}// end loop()

void sendWIFI() {
  //digitalWrite(2, 1);
  prevMillsWifi = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINTLN(" connected");

  Udp.begin(localUdpPort);
  DEBUG_PRINTLN(("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort));
  Udp.beginPacket("192.28.1.113", 800);
  Udp.write(replyPacket);
  Udp.endPacket();
  digitalWrite(14, 0);

  WiFi.mode(WIFI_OFF);
}
