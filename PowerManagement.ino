
// Update sleep interval
void updateSleep() {
  if (millis() > SLEEP_INTERVAL) {
    readRTCData();
    rtcData.unixTime += round((float)(millis() + SLEEP_INTERVAL) / 1000);
    writeRTCData();
    ESP.deepSleep(SLEEP_INTERVAL * 1000);
  }
}
