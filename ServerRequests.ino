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
  if (httpCode == -1 || httpCode == 404) {
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
  DEBUG_PRINTLN("Result: ");
  DEBUG_PRINTLN(res);
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
