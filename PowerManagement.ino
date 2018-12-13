
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
    uint32_t newTime = getTime();
    // Only save time if successfully retrieved from the server.
    if (rtcData.unixTime != 0) {
      rtcData.unixTime = newTime;
      rtcData.unixTimeRemainder = 0;
      setTime(rtcData.unixTime);
    }
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

void powerup() {
  DEBUG_PRINTLN("Reset from Powerup. Getting time...");
  connectToWiFi();
  uint32_t newTime = getTime();
  // If server time has failed, set time to NIGHT_END.
  if (newTime == 0) {
    DEBUG_PRINTLN("Setting time to epoch + NIGHT_END...");
    rtcData.unixTime = NIGHT_END * 3600;
    rtcData.timeError = 1;
  }
  else {
    rtcData.unixTime = newTime;
  }
  rtcData.unixTimeRemainder = 0;

  // Clear previous tags
  for (int i = 0; i < 5; i++) rtcData.previousTag[i] = 0;
  setTime(rtcData.unixTime);
  rtcData.previousTagTime = now() - 60;
  DEBUG_PRINT(hour());
  DEBUG_PRINT(" : ");
  DEBUG_PRINTLN(minute());
  sendPowerup();
}

void prepareForSleep() {
  DEBUG_PRINTLN("Getting time before sleep...");
  connectToWiFi();
  uint32_t newTime;
  newTime = getTime();
  if (newTime == 0) {
    DEBUG_PRINTLN("Forwarding time by 10 minutes...");
    rtcData.unixTime += 600;
  }
  else {
    rtcData.unixTime = newTime;
  }
  rtcData.unixTimeRemainder = 0;
  setTime(rtcData.unixTime);
  DEBUG_PRINT(hour());
  DEBUG_PRINT(" : ");
  DEBUG_PRINTLN(minute());
  sendPing();
}

void prepareForDaytime() {
  DEBUG_PRINTLN("Awaken from night-time sleep");
  connectToWiFi();
  uint32_t newTime;
  newTime = getTime();
  // If server time has failed, set time to NIGHT_END.
  if (newTime == 0) {
    DEBUG_PRINTLN("Setting time to epoch + NIGHT_END...");
    rtcData.unixTime = NIGHT_END * 3600;
    rtcData.timeError = 1;
  }
  else {
    rtcData.unixTime = newTime;
  }
  rtcData.unixTimeRemainder = 0;
  setTime(rtcData.unixTime);
  DEBUG_PRINT(hour());
  DEBUG_PRINT(" : ");
  DEBUG_PRINTLN(minute());
  sendPing();
  rtcData.sleeping = 0;
}

void resyncTime() {
  DEBUG_PRINTLN("Resyncing time...");
  connectToWiFi();
  uint32_t newTime;
  newTime = getTime();
  // If server time has failed, set time to NIGHT_END.
  if (newTime == 0) {
    DEBUG_PRINTLN("Forwarding time by 1 minute...");
    rtcData.unixTime += 61;
    rtcData.timeError = 1;
  }
  else {
    rtcData.unixTime = newTime;
    rtcData.timeError = 0;
  }
  rtcData.unixTimeRemainder = 0;
  setTime(rtcData.unixTime);
  DEBUG_PRINT(hour());
  DEBUG_PRINT(" : ");
  DEBUG_PRINTLN(minute());
  sendPing();
  rtcData.sleeping = 0;
}

