/****************************************************************************************************
* rfiduino.h - RFIDuino Library Header File
* RFID transfer code modified from RFIDuino Library by TrossenRobotics / RobotGeek
*
****************************************************************************************************/

#include "Arduino.h"

#ifndef Naturewatch_RFID
#define Naturewatch_RFID 

#define DELAYVAL    320  
#define TIMEOUT     1000  

class RFID
{
  public:

    RFID(float verison);

    bool decodeTag(unsigned char *buf); 
    void transferToBuffer(byte *tagData, byte *tagDataBuffer);
    bool compareTagData(byte *tagData1, byte *tagData2);    

    bool scanForTag(byte *tagData);

    bool isModuleReady();


  private:

    int demodOut;
    int shd;
    int mod;
    int rdyClk;

};

#endif
