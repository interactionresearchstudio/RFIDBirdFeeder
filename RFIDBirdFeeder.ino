#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define DEBUG
#include <naturewatch_RFID.h>
WiFiUDP Udp;

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
  Serial.println("start up");
#endif
}

void loop() {
  //digitalWrite(2,HIGH);
  //scan for a tag - if a tag is sucesfully scanned, return a 'true' and proceed
  if (myRFIDuino.scanForTag(tagData) == true)
  {
    digitalWrite(14, 1);
#ifdef DEBUG
    Serial.print("RFID Tag ID:"); //print a header to the Serial port.
#endif
    //loop through the byte array
    for (int n = 0; n < 5; n++)
    {
#ifdef DEBUG
      Serial.print(tagData[n], 16); //print the byte in Decimal format
#endif
      if (n < 4) //only print the comma on the first 4 nunbers
      {
#ifdef DEBUG
        Serial.print(",");
#endif
      }
    }
#ifdef DEBUG
    Serial.print("\n\r");//return character for next line
#endif
  //  sendWIFI();
  }
  // }
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
#ifdef DEBUG
    Serial.print(".");
#endif
  }
#ifdef DEBUG
  Serial.println(" connected");
#endif

  Udp.begin(localUdpPort);
#ifdef DEBUG
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
#endif
  Udp.beginPacket("192.28.1.113", 800);
  Udp.write(replyPacket);
  Udp.endPacket();
  digitalWrite(14, 0);

  WiFi.mode(WIFI_OFF);
}


