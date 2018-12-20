// Basic GET request.
String getRequest(char* endpoint, int *httpCode) {
  HTTPClient http;
  // Connect to host
  http.begin(String(HOST) + String(endpoint));

  *httpCode = http.GET();

  String payload;
  if (httpCode > 0) {
    DEBUG_PRINT("HTTP code: ");
    DEBUG_PRINTLN(*httpCode);

    if (*httpCode == HTTP_CODE_OK) {
      payload = http.getString();
      DEBUG_PRINTLN(payload);
      return payload;
    }
  }
  else {
    DEBUG_PRINT("HTTP GET failed. Code: ");
    DEBUG_PRINTLN(*httpCode);
  }
}

// Basic POST request.
String postRequest(char* endpoint, String request, int *httpCode) {
  HTTPClient http;

  http.begin(String(HOST) + String(endpoint));
  http.addHeader("Content-Type", "application/json");

  *httpCode = http.POST(request);

  String result;
  if (*httpCode > 0) {
    DEBUG_PRINT("HTTP code: ");
    DEBUG_PRINTLN(*httpCode);

    if (*httpCode == HTTP_CODE_OK) {
      result = http.getString();
      DEBUG_PRINTLN(result);
      return result;
    }
  }
  else {
    DEBUG_PRINT("HTTP POST failed. Code: ");
    DEBUG_PRINTLN(*httpCode);
  }
}

// Get Unix time from server.
unsigned long getTime() {
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  int httpCode;
  String timeJson = getRequest("/api/time", &httpCode);
  if (httpCode != 200) {
    DEBUG_PRINTLN("Could not retrieve time from server.");
    return 0;
  }
  JsonObject& root = jsonBuffer.parseObject(timeJson);
  if (!root.success()) {
    DEBUG_PRINTLN("Could not parse JSON object.");
    return 0;
  }
  return root["time"];
}

// Send tracking event to server.
void postTrack(String rfid) {
  const size_t bufferSize = JSON_OBJECT_SIZE(3);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();
  root["datetime"] = " ";
  root["rfid"] = rfid;
  root["stub"] = FEEDERSTUB;

  String payload;
  root.printTo(payload);
  DEBUG_PRINT("Payload: ");
  DEBUG_PRINTLN(payload);
  DEBUG_PRINTLN("Posting track...");
  int httpCode;
  String res = postRequest("/api/recordTrack", payload, &httpCode);
  if (httpCode != 200) {
    DEBUG_PRINTLN("Post request failed. Caching track...");
    cacheTag(tagData);
  }
}

// Send tracking event to server, with local time.
int postCachedTrack(String rfid, String datetime) {
  const size_t bufferSize = JSON_OBJECT_SIZE(3);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();
  root["datetime"] = datetime;
  root["rfid"] = rfid;
  root["stub"] = FEEDERSTUB;

  String payload;
  root.printTo(payload);
  DEBUG_PRINT("Payload: ");
  DEBUG_PRINTLN(payload);
  DEBUG_PRINTLN("Posting track...");
  int httpCode;
  String res = postRequest("/api/recordTrack", payload, &httpCode);
  if (httpCode != 200) {
    DEBUG_PRINTLN("Cached Post request failed.");
  }
  return httpCode;
}

// Send ping to server
void sendPing() {
  const size_t bufferSize = JSON_OBJECT_SIZE(1);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();
  root["stub"] = FEEDERSTUB;

  String payload;
  root.printTo(payload);
  DEBUG_PRINT("Payload: ");
  DEBUG_PRINTLN(payload);
  DEBUG_PRINTLN("Sending Ping...");
  int httpCode;
  String res = postRequest("/api/ping", payload, &httpCode);
  DEBUG_PRINTLN("Result: ");
  DEBUG_PRINTLN(res);
}

// Send powerup event to server
void sendPowerup() {
  const size_t bufferSize = JSON_OBJECT_SIZE(2);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();
  root["stub"] = FEEDERSTUB;
  root["type"] = "powerup";

  String payload;
  root.printTo(payload);
  DEBUG_PRINT("Payload: ");
  DEBUG_PRINTLN(payload);
  DEBUG_PRINTLN("Sending Powerup Event...");
  int httpCode;
  String res = postRequest("/api/ping", payload, &httpCode);
  DEBUG_PRINTLN("Result: ");
  DEBUG_PRINTLN(res);
}

// Offload cached readings to server
void syncCache() {
  // Loop through cache array
  for (int j = 0; j < 4; j++) {
    int numOfZeros = 0;
    String rfid;
    for (int i = 0; i < 5; i++) {
      if (rtcData.cachedTags[j][i] == 0) {
        numOfZeros++;
      }
      rfid += String(rtcData.cachedTags[j][i], HEX);
    }

    // If tag is non-empty, send it to the server.
    if (numOfZeros < 5) {
      int httpCode = postCachedTrack(rfid, String(rtcData.cachedTimes[j]));
      if (httpCode == 200) {
        DEBUG_PRINTLN("Posted cached tag.");
        for (int i = 0; i < 5; i++) rtcData.cachedTags[j][i] = 0;
        rtcData.cachedTimes[j] = 0;
        rtcData.numOfCachedTags--;
      }
      else DEBUG_PRINTLN("Could not post cached tag. Leaving it there.");
      delay(1000);
    }
  }
}

void checkForUpdate() {
  WiFiClient client;
  
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://feedernet.herokuapp.com/api/update", VERSION);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}
