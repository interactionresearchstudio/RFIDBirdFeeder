
// Update sleep interval
void updateSleep() {
  if (millis() > SLEEP_INTERVAL) {
    ESP.deepSleep(SLEEP_INTERVAL * 1000);
  }
}
