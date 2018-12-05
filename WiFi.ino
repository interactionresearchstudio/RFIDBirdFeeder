// Connect to WiFi - attempt at quick connect, fall back to normal connect.
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  if (isRTCValid()) {
    DEBUG_PRINT("Performing quick connect. Channel: ");
    DEBUG_PRINT(rtcData.channel);
    DEBUG_PRINT(" | BSSID: ");
    for(int i=0; i<6; i++) DEBUG_PRINT(rtcData.bssid[i]);
    DEBUG_PRINTLN();
    WiFi.begin(WLAN_SSID, WLAN_PASS, rtcData.channel, rtcData.bssid, true);
  }
  else {
    DEBUG_PRINTLN("Performing regular connect...");
    WiFi.begin(WLAN_SSID, WLAN_PASS);
  }

  int retries = 0;
  while(WiFi.status() != WL_CONNECTED) {
    retries++;
    DEBUG_PRINT("Attempt ");
    DEBUG_PRINTLN(retries);
    if (retries == WIFI_QUICK_MAX_RETRIES) {
      DEBUG_PRINTLN("Retrying with regular connect...");
      WiFi.disconnect();
      delay(10);
      WiFi.forceSleepBegin();
      delay(10);
      WiFi.forceSleepWake();
      delay(10);
      WiFi.begin(WLAN_SSID, WLAN_PASS);
    }

    if (retries == WIFI_REGULAR_MAX_RETRIES) {
      DEBUG_PRINTLN("Giving up on WiFi...");
      WiFi.disconnect(true);
      delay(1);
      WiFi.mode(WIFI_OFF);
      return;
    }
    delay(50);
  }
  DEBUG_PRINTLN("Connected to WiFi.");
  rtcData.channel = WiFi.channel();
  memcpy(rtcData.bssid, WiFi.BSSID(), 6);
}
