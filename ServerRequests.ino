// Basic GET request.
String getRequest(char* endpoint, int *httpCode, byte maxRetries) {
  HTTPClient http;

  for (int i = 0; i < maxRetries; i++) {
    http.begin(String(HOST) + String(endpoint));

    *httpCode = http.GET();

    String payload;
    if (*httpCode == HTTP_CODE_OK || *httpCode == 400) {
      DEBUG_PRINT("HTTP code: ");
      DEBUG_PRINTLN(*httpCode);
      payload = http.getString();
      DEBUG_PRINTLN(payload);
      return payload;
    }
    else {
      DEBUG_PRINT("HTTP GET failed. Code: ");
      DEBUG_PRINTLN(*httpCode);
      if (maxRetries > 0) {
        DEBUG_PRINT("Retry #");
        DEBUG_PRINTLN(i + 1);
      }
    }
  }

  return String("");
}

// Request data from LoRa base.
String requestFromRadio(int destinationId, int originId, char command, String message, int timeout, int maxRetries) {
  String payload =
    String(destinationId) + "," +
    String(originId) + "," +
    command + "," +
    message;

  //uint32_t checksum = calculateCRC32(payload.toCharArray(), payload.length());

  DEBUG_PRINTLN("Payload: " + payload);

  boolean waitingForReply = true;
  String packet;
  unsigned long timeSent = millis();
  lora.print(payload);
  lora.write(0x04);

  for (int i = 0; i < maxRetries; i++) {
    // Wait for reply.
    while (waitingForReply) {
      if (lora.available() > 0) {
        char inChar = lora.read();
        Serial.print(inChar);
        if (inChar == 0x04) {
          DEBUG_PRINTLN("Received packet " + packet);
          if (isThisMyPacket(packet, RADIOID)) {
            // Received my reply!
            DEBUG_PRINTLN("Received packet " + packet + " for myself.");
            return packet;
          }
          else {
            // Not my packet. Clear packet and start listening again.
            packet = "";
          }
        }
        else packet += inChar;
      }
      if (millis() - timeSent >= timeout) {
        DEBUG_PRINTLN("Waiting for LoRa reply timed out.");
        waitingForReply = false;
      }
      delay(0);
    }
  }

  // Return nothing if max retries have run their course.
  return String("");
}

// Check if packet is destined for this feeder.
boolean isThisMyPacket(String packet, int destinationId) {
  int firstSeparator = packet.indexOf((char)',');
  int packetDestination = packet.substring(0, firstSeparator).toInt();
  if (packetDestination == destinationId) return true;
  else return false;
}

// Basic POST request.
String postRequest(char* endpoint, String request, int *httpCode, byte maxRetries) {
  HTTPClient http;

  for (int i = 0; i < maxRetries; i++) {
    http.begin(String(HOST) + String(endpoint));
    http.addHeader("Content-Type", "application/json");

    *httpCode = http.POST(request);

    String result;
    if (*httpCode == 200) {
      DEBUG_PRINT("HTTP code: ");
      DEBUG_PRINTLN(*httpCode);
      result = http.getString();
      DEBUG_PRINTLN(result);
      return result;
    }
    else {
      DEBUG_PRINT("HTTP POST failed. Code: ");
      DEBUG_PRINTLN(*httpCode);
      if (maxRetries > 0) {
        DEBUG_PRINT("Retry #");
        DEBUG_PRINTLN(i + 1);
      }
    }
  }

  return String("");
}

// Get Unix time from server.
unsigned long getTime() {
  const size_t bufferSize = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  int httpCode;
  String timeJson = getRequest("/api/time", &httpCode, REQUEST_RETRIES);
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
#ifdef LORA
  requestFromRadio(100, RADIOID, 'R', rfid, 4000, 3);
  //if (reply != "") DEBUG_PRINTLN("Radio request failed.");
#else
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
  String res = postRequest("/api/recordTrack", payload, &httpCode, REQUEST_RETRIES);
  if (httpCode != 200) {
    DEBUG_PRINTLN("Post request failed. Caching track...");
    cacheTag(tagData);
  }
#endif
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
  String res = postRequest("/api/recordTrack", payload, &httpCode, REQUEST_RETRIES);
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
  String res = postRequest("/api/ping", payload, &httpCode, REQUEST_RETRIES);
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
  String res = postRequest("/api/ping", payload, &httpCode, REQUEST_RETRIES);
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
      if (rtcData.cachedTags[j][i] < 0x10) {
        rfid += String(0);
        rfid += String(rtcData.cachedTags[j][i], HEX);
      } else {
        rfid += String(rtcData.cachedTags[j][i], HEX);
      }
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
      //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      DEBUG_PRINTLN("HTTP_UPDATE_FAILED Error");
      break;

    case HTTP_UPDATE_NO_UPDATES:
      DEBUG_PRINTLN("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      DEBUG_PRINTLN("HTTP_UPDATE_OK");
      break;
  }
}
