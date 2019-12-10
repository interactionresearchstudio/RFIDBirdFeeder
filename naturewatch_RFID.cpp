/****************************************************************************************************
  rfiduino.h - RFIDuino Library Header File
  RFID transfer code modified from RFIDuino Library by TrossenRobotics / RobotGeek

****************************************************************************************************/


#include "Arduino.h"
#include "naturewatch_RFID.h"

RFID::RFID(float version)
{
  demodOut = 13;
  shd = 14;
  rdyClk = 12;
  //set pin modes on RFID pins
  pinMode(shd, OUTPUT);
  pinMode(demodOut, INPUT);
  pinMode(rdyClk, INPUT);

  //set shd low to prepare for reading
  digitalWrite(shd, HIGH);
  //  digitalWrite(mod, LOW);
  digitalWrite(shd, LOW);
}

bool RFID::isModuleReady() {
  static bool moduleOn;
  if (digitalRead(rdyClk) == 1 && moduleOn == false) {
    moduleOn = true;
    return true;
  }
  else if (digitalRead(rdyClk) == 0 && moduleOn == false) {
    return false;
  } else {
    return true;
  }
}
//Manchester decode. Supply the function an array to store the tags ID in
bool RFID::decodeTag(unsigned char *buf)
{
  unsigned char i = 0;
  unsigned short timeCount;
  unsigned char timeOutFlag = 0;
  unsigned char row, col;
  unsigned char row_parity;
  unsigned char col_parity[5];
  unsigned char dat;
  unsigned char searchCount = 0;
  unsigned char j;
  while (1)
  {
    timeCount = 0;
    while (0 == digitalRead(demodOut)) //watch for demodOut to go low
    {

      if (timeCount >= TIMEOUT) //if we pass TIMEOUT counter, break out of the loop
      {
        break;
      }
      else
      {
        timeCount++;
      }
    }

    if (timeCount >= 600)
    {
      return false;
    }
    timeCount = 0;

    delayMicroseconds(DELAYVAL);
    if (digitalRead(demodOut))
    {
      for (i = 0; i < 8; i++) // 9 header bits
      {
        timeCount = 0; //restart counting
        while (1 == digitalRead(demodOut)) //while DEMOD out is high
        {
          if (timeCount == TIMEOUT)
          {
            timeOutFlag = 1;
            break;
          }
          else
          {
            timeCount++;
          }
        }

        if (timeOutFlag)
        {
          break;
        }
        else
        {
          delayMicroseconds(DELAYVAL);
          if ( 0 == digitalRead(demodOut) )
          {
            break;
          }
        }
      }//end for loop

      if (timeOutFlag)
      {
        timeOutFlag = 0;
        return false;
      }

      if (i == 8) //Receive the data
      {
        timeOutFlag = 0;
        timeCount = 0;
        while (1 == digitalRead(demodOut))
        {
          if (timeCount == TIMEOUT)
          {
            timeOutFlag = 1;
            break;
          }
          else
          {
            timeCount++;
          }

          if (timeOutFlag)
          {
            timeOutFlag = 0;
            return false;
          }
        }

        col_parity[0] = 0;
        col_parity[1] = 0;
        col_parity[2] = 0;
        col_parity[3] = 0;
        col_parity[4] = 0;

        for (row = 0; row < 11; row++)
        {
          row_parity = 0;
          j = row >> 1;

          for (col = 0, row_parity = 0; col < 5; col++)
          {
            delayMicroseconds(DELAYVAL);
            if (digitalRead(demodOut))
            {
              dat = 1;
            }
            else
            {
              dat = 0;
            }

            if (col < 4 && row < 10)
            {
              buf[j] <<= 1;
              buf[j] |= dat;
            }

            row_parity += dat;
            col_parity[col] += dat;
            timeCount = 0;
            while (digitalRead(demodOut) == dat)
            {
              if (timeCount == TIMEOUT)
              {
                timeOutFlag = 1;
                break;
              }
              else
              {
                timeCount++;
              }
            }
            if (timeOutFlag)
            {
              break;
            }
          }

          if (row < 10)
          {
            if ((row_parity & 0x01) || timeOutFlag) //Row parity
            {
              timeOutFlag = 1;
              break;
            }
          }
        }

        if ( timeOutFlag || (col_parity[0] & 0x01) || (col_parity[1] & 0x01) || (col_parity[2] & 0x01) || (col_parity[3] & 0x01) ) //Column parity
        {
          timeOutFlag = 0;
          return false;
        }
        else
        {
          return true;
        }

      }//end if(i==8)

      return false;

    }//if(digitalRead(demodOut))
  } //while(1)
};




//function to compare 2 byte arrays. Returns true if the two arrays match, false of any numbers do not match
bool RFID::compareTagData(byte * tagData1, byte * tagData2)
{
  for (int j = 0; j < 5; j++)
  {
    if (tagData1[j] != tagData2[j])
    {
      return false; //if any of the ID numbers are not the same, return a false
    }
  }

  return true;  //all id numbers have been verified
}

//function to transfer one byte array to a secondary byte array.
//source -> tagData
//destination -> tagDataBuffer
void RFID::transferToBuffer(byte * tagData, byte * tagDataBuffer)
{
  for (int j = 0; j < 5; j++)
  {
    tagDataBuffer[j] = tagData[j];
  }
}

bool RFID::scanForTag(byte * tagData)
{
  static byte tagDataBuffer[2][5];      //A Buffer for verifying the tag data. 'static' so that the data is maintained the next time the loop is called
  static int readCount = 0;          //the number of times a tag has been read. 'static' so that the data is maintained the next time the loop is called
  boolean verifyRead = false; //true when a tag's ID matches a previous read, false otherwise
  boolean tagCheck = false;   //true when a tag has been read, false otherwise

  tagCheck = decodeTag(tagData); //run the decodetag to check for the tag
  if (tagCheck == true) //if 'true' is returned from the decodetag function, a tag was succesfully scanned
  {
    isHalfRead = true;
    readCount++;      //increase count since we've seen a tag
    switch (readCount) {
      default:
        break;
      case (1):
        transferToBuffer(tagData, tagDataBuffer[0]);
        break;
      case (2):
        transferToBuffer(tagData, tagDataBuffer[1]);
        break;
    }
    if (readCount == 3) {
      for (byte i = 0; i < 2; i++) {
        verifyRead = compareTagData(tagData, tagDataBuffer[i]);
        if (verifyRead == false) {
          break;
        }
      }
      if (verifyRead == true ) //if a 'true' is returned by compareTagData, the current read matches the last read
      {
        readCount = 0; //because a tag has been succesfully verified, reset the readCount to '0' for the next tag
        return true;
      }
    } else {
      return false;
    }
  }
  else
  {
    if (millis() > 80 && isHalfRead == false) {
      digitalWrite(14,1);
      //updateTime(1000);
    //  writeRTCData();
      ESP.deepSleepInstant(4000 * 1000);
    }
    return false;
  }
}
