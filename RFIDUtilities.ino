// Scan for a tag
void updateRfid() {

  //scan for a tag - if a tag is sucesfully scanned, return a 'true' and proceed
  if  (scanForTag(tagData) == true)
  {
    digitalWrite(14, 1);
    DEBUG_PRINTLN("RFID Tag ID:"); //print a header to the Serial port.
    //loop through the byte array
    String rfid;
    for (int n = 0; n < 5; n++)
    {
      if (tagData[n] < 0x10) {
        DEBUG_PRINTHEX(0); //print the byte in Decimal format
        DEBUG_PRINTHEX(tagData[n]); //print the byte in Decimal format
        rfid += String("0");
        rfid += String(tagData[n], HEX);
      } else {
        DEBUG_PRINTHEX(tagData[n]); //print the byte in Decimal format
        rfid += String(tagData[n], HEX);
      }
      if (n < 4) //only print the comma on the first 4 nunbers
      {
        DEBUG_PRINT(",");
      }
    }
    DEBUG_PRINT("\n\r");//return character for next line

    DEBUG_PRINT("Last tag read: ");
    DEBUG_PRINTLN(rtcData.previousTagTime);
    DEBUG_PRINT("Last tag: ");
    for (int i = 0; i < 5; i++) DEBUG_PRINTHEX(tagData[i]);
    DEBUG_PRINTLN();

    // Debouncing
    for (int i = 0; i < 5; i++) {
      if (tagData[i] != rtcData.previousTag[i]) {
        DEBUG_PRINTLN("New tag");
        break;
      }
      if (i == 4) {
        // Same tag
        DEBUG_PRINTLN("Same tag");
        if (now() - rtcData.previousTagTime < TAG_DEBOUNCE) {
          DEBUG_PRINTLN("Same tag within minute");
          updateTime(SLEEP_INTERVAL);
          writeRTCData();
          ESP.deepSleepInstant(SLEEP_INTERVAL * 1000);
          return;
        }
      }
    }
#ifndef LORA
    connectToWiFi();
    DEBUG_PRINTLN("connecting to wifi");
#endif
    postTrack(rfid);
    for (int i = 0; i < 5; i++) {
      rtcData.previousTag[i] = tagData[i];
    }
    rtcData.previousTagTime = getUnixTime();
    updateSleep();
  }
}
