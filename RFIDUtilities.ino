// Scan for a tag
void updateRfid() {
  //scan for a tag - if a tag is sucesfully scanned, return a 'true' and proceed
  if (rfid.scanForTag(tagData) == true)
  {
    digitalWrite(14, 1);
    DEBUG_PRINTLN("RFID Tag ID:"); //print a header to the Serial port.
    //loop through the byte array
    for (int n = 0; n < 5; n++)
    {
      DEBUG_PRINTHEX(tagData[n]); //print the byte in Decimal format
      if (n < 4) //only print the comma on the first 4 nunbers
      {
        DEBUG_PRINT(",");
      }
    }
    DEBUG_PRINT("\n\r");//return character for next line
  }
}