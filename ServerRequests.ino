// Basic GET request.
String getRequest(char* endpoint) {
  HTTPClient http;
  // Connect to host
  http.begin(String(HOST) + String(endpoint));

  int httpCode = http.GET();

  String payload;
  if (httpCode > 0) {
    DEBUG_PRINT("HTTP code: ");
    DEBUG_PRINTLN(httpCode);

    if (httpCode == HTTP_CODE_OK) {
      payload = http.getString();
      DEBUG_PRINTLN(payload);
      return payload;
    }
  }
  else {
    DEBUG_PRINTLN("HTTP GET failed.");
  }
}

// Get Unix time from server.
unsigned long getTime() {
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  String timeJson = getRequest("/api/time");
  JsonObject& root = jsonBuffer.parseObject(timeJson);
  if (!root.success()) {
    DEBUG_PRINTLN("Could not parse JSON object.");
    return 0;
  }
  return root["time"];
}

// Report power up event to server.
void reportPowerup() {

}

// Send tracked bird to server.
void sendTrackEvent() {

}

// Send ping to server
void sendPing() {

}
