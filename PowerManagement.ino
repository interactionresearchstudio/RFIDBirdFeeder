
// Update sleep interval
void updateSleep() {
  if (millis() > SLEEP_INTERVAL) {
    
    updateTime();
    writeRTCData();
    ESP.deepSleep(SLEEP_INTERVAL * 1000);
  }
}

void updateTime() {
  rtcData.unixTimeRemainder += (millis() + SLEEP_INTERVAL);
  
  if (rtcData.unixTimeRemainder % 1000 == 0) {
    // Rounded second. Time to add!
    DEBUG_PRINTLN("Time rounded to the second.");
    rtcData.unixTime += rtcData.unixTimeRemainder / 1000;
    rtcData.unixTimeRemainder = 0;
  }
  
  DEBUG_PRINT("Time remainder: ");
  DEBUG_PRINTLN(rtcData.unixTimeRemainder);
}

uint32_t getUnixTime() {
  return rtcData.unixTime + (rtcData.unixTimeRemainder / 1000);
}
