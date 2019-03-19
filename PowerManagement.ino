// Main daytime routine
void updateSleep() {
  if (millis() >= WAKE_INTERVAL) {

    updateTime(SLEEP_INTERVAL);
    writeRTCData();
    ESP.deepSleepInstant(SLEEP_INTERVAL * 1000);
  }
}

// Main night-time routine
void updateNightTime() {
  // Check if it's one hour before wake-up
  rtcData.sleeping = 1;
  if (hour() == rtcData.NIGHT_END_HOUR - 1 && minute() == 45) {
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
  ESP.deepSleepInstant(NIGHT_SLEEP_INTERVAL * 1000);
}

// Update timekeeping (internally)
void updateTime(uint32_t sleepInterval) {
  rtcData.unixTimeRemainder += (millis() + sleepInterval);
  uint16_t modulo = rtcData.unixTimeRemainder % 1000;
  rtcData.unixTime += (rtcData.unixTimeRemainder - modulo) / 1000;
  rtcData.unixTimeRemainder = modulo;
  DEBUG_PRINT("Time remainder: ");
  DEBUG_PRINTLN(rtcData.unixTimeRemainder);
}

time_t getUnixTime() {
  return rtcData.unixTime + ((rtcData.unixTimeRemainder + millis()) / 1000);
}

// UART functions
void updateUart() {
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == 'W') {
      readCredentialsFromUart();
    }
  }
}

// Powerup event
void powerup() {
  DEBUG_PRINTLN("Reset from Powerup. Press W to change WiFi credentials...");
  delay(1500);
  updateUart();
  connectToWiFi();
  uint32_t newTime = getTime();
  // If server time has failed, set time to NIGHT_END.
  if (newTime == 0) {
    DEBUG_PRINTLN("Setting time to epoch + NIGHT_END...");
    rtcData.unixTime = rtcData.NIGHT_END_HOUR * 3600;
    rtcData.timeError = 1;
  }
  else {
    rtcData.unixTime = newTime;
  }
  rtcData.unixTimeRemainder = 0;

  // Clear previous tags
  for (int i = 0; i < 5; i++) rtcData.previousTag[i] = 0;
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 5; i++) {
      rtcData.cachedTags[j][i] = 0;
    }
    rtcData.cachedTimes[j] = 0;
  }
  rtcData.numOfCachedTags = 0;
  setTime(rtcData.unixTime);
  rtcData.previousTagTime = now() - 60;
  DEBUG_PRINT(hour());
  DEBUG_PRINT(" : ");
  DEBUG_PRINTLN(minute());

  getSunriseSunset();
  sendPowerup();
  checkForUpdate();
}

// Pre-sleep event
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
  syncCache();
}

// Post-sleep event
void prepareForDaytime() {
  DEBUG_PRINTLN("Awaken from night-time sleep");
  connectToWiFi();
  checkForUpdate();
  uint32_t newTime;
  newTime = getTime();
  // If server time has failed, set time to NIGHT_END.
  if (newTime == 0) {
    DEBUG_PRINTLN("Setting time to epoch + NIGHT_END...");
    rtcData.unixTime = rtcData.NIGHT_END_HOUR * 3600;
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
  getSunriseSunset();
  sendPing();
  rtcData.sleeping = 0;
}

// Try to resync time with server in the event of a time error.
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
