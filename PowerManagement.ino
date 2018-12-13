
// Update sleep interval
void updateSleep() {
  if (millis() >= WAKE_INTERVAL) {

    updateTime(SLEEP_INTERVAL);
    writeRTCData();
    ESP.deepSleep(SLEEP_INTERVAL * 1000);
  }
}

void updateNightTime() {
  // Check if it's one hour before wake-up
  rtcData.sleeping = 1;
  if (hour() == NIGHT_END - 1 && minute() == 45) {
    DEBUG_PRINTLN("Getting time before wake-up...");
    connectToWiFi();
    rtcData.unixTime = getTime();
    rtcData.unixTimeRemainder = 0;
    setTime(rtcData.unixTime);
  }
  updateTime(NIGHT_SLEEP_INTERVAL);
  writeRTCData();
  ESP.deepSleep(NIGHT_SLEEP_INTERVAL * 1000);
}

void updateTime(uint32_t sleepInterval) {
  rtcData.unixTimeRemainder += (millis() + sleepInterval);

  if (rtcData.unixTimeRemainder % 1000 == 0) {
    // Rounded second. Time to add!
    DEBUG_PRINTLN("Time rounded to the second.");
    rtcData.unixTime += rtcData.unixTimeRemainder / 1000;
    rtcData.unixTimeRemainder = 0;
  }

  DEBUG_PRINT("Time remainder: ");
  DEBUG_PRINTLN(rtcData.unixTimeRemainder);
}

time_t getUnixTime() {
  return rtcData.unixTime + ((rtcData.unixTimeRemainder + millis()) / 1000);
}
