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
    } else {
      DEBUG_PRINTLN("CRC32 check ok, data is valid.");
    }
  }
}

// Write struct data to RTC user memory. Call when all vars within struct are updated.
void writeRTCData() {
  updateCRC32();
  if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    DEBUG_PRINTLN("Written to RTC user memory.");
  }
}

// Update CRC32 validator.
void updateCRC32() {
  rtcData.crc32 = calculateCRC32(((uint8_t*) &rtcData) + 4, sizeof(rtcData) - 4);
}
