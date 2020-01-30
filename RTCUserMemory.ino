boolean rtcValid = false;

// Check validity of data saved in RTC user memory.
uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

// Read struct data from RTC user memory.
void readRTCData() {
  if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    DEBUG_PRINTLN("Read from RTC user memory.");
    uint32_t crcOfData = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
    DEBUG_PRINT("CRC32 of data: ");
    DEBUG_PRINTHEX(crcOfData);
    DEBUG_PRINTLN("");
    DEBUG_PRINT("CRC32 read from RTC: ");
    DEBUG_PRINTHEX(rtcData.crc32);
    DEBUG_PRINTLN("");
    
    if (crcOfData != rtcData.crc32) {
      DEBUG_PRINTLN("CRC32 in RTC memory doesn't match CRC32 of data. Data is invalid!");
      rtcValid = false;
    } else {
      DEBUG_PRINTLN("CRC32 check ok, data is valid.");
      rtcValid = true;
    }
  }
}

// Write struct data to RTC user memory. Call when all vars within struct are updated.
void writeRTCData() {
  updateCRC32();
  if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    DEBUG_PRINTLN("Written to RTC user memory.");
  }
  yield();
}

// Update CRC32 validator.
void updateCRC32() {
  rtcData.crc32 = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
}

// Return true if RTC data is valid.
boolean isRTCValid() {
  return rtcValid;
}

// Cache tag within RTC memory if HTTP request fails.
void cacheTag(uint8_t tag[]) {
  if (rtcData.numOfCachedTags < 4) {

    // Find an empty slot
    int cacheIndex = -1;
    for (int j = 0; j < 4; j++) {
      int numOfZeros = 0;
      for (int i = 0; i < 5; i++) {
        if (rtcData.cachedTags[j][i] == 0) {
          numOfZeros++;
        }
        if (numOfZeros == 5) {
          cacheIndex = j;
          break;
        }
      }
      if (cacheIndex != -1) {
        break;
      }
    }

    // Save tag
    if (cacheIndex >= 0 && cacheIndex < 4) {
      for (int i = 0; i < 5; i++) {
        rtcData.cachedTags[cacheIndex][i] = tag[i];
      }
      rtcData.cachedTimes[cacheIndex] = now();
      rtcData.numOfCachedTags++;
      DEBUG_PRINTLN("Saved tag to empty slot.");
    }
  }
  else {
    DEBUG_PRINTLN("Cached slots full. Can't cache tag.");
  }
}
