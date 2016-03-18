#include <SoftwareSerial.h>

SoftwareSerial ltmSerial(8, 9);

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    delay(100);
  }

  ltmSerial.begin(9600);
}

enum ltmStates {
  IDLE,
  HEADER_START1,
  HEADER_START2,
  HEADER_MSGTYPE,
  HEADER_DATA
};

#define LONGEST_FRAME_LENGTH 18 

/*
 * LTM based on https://github.com/KipK/Ghettostation/blob/master/GhettoStation/LightTelemetry.cpp implementation
 */

#define GFRAMELENGTH 18
#define AFRAMELENGTH 10
#define SFRAMELENGTH 11
#define OFRAMELENGTH 18
#define NFRAMELENGTH 10

uint8_t serialBuffer[LONGEST_FRAME_LENGTH];
uint8_t state = IDLE;
char frameType;
byte frameLength;
byte receiverIndex;

byte readByte(uint8_t offset) {
  return serialBuffer[offset];
}

int readInt(uint8_t offset) {
  return (int) serialBuffer[offset] + ((int) serialBuffer[offset + 1] << 8);
}

void loop() {

  if (ltmSerial.available()) {

    char data = ltmSerial.read();

    if (state == IDLE) {
      if (data == '$') {
        state = HEADER_START1;
      }
    } else if (state == HEADER_START1) {
      if (data == 'T') {
        state = HEADER_START2;
      } else {
        state = IDLE;
      }
    } else if (state == HEADER_START2) {
      frameType = data;
      state = HEADER_MSGTYPE;
      receiverIndex = 0;

      switch (data) {

        case 'G':
          frameLength = GFRAMELENGTH;
          break;
        case 'A':
          frameLength = AFRAMELENGTH;
          break;
        case 'S':
          frameLength = SFRAMELENGTH;
          break;
        case 'O':
          frameLength = OFRAMELENGTH;
          break;
        case 'N':
          frameLength = NFRAMELENGTH;
          break;
        default:
          state = IDLE;
      }
      
    } else if (state == HEADER_MSGTYPE) {

      /*
       * Check if last payload byte has been received.
       */
      if (receiverIndex == frameLength - 4) {
        /*
         * If YES, check checksum and execute data processing
         */
 
        
        if (frameType == 'A') {
          Serial.println(readInt(0));
          /*
          Serial.print(frameType);
          
          for (byte i = 0; i < receiverIndex; i++) {
            Serial.print(serialBuffer[i]);
            Serial.print(" ");
          }
  
          Serial.println("");*/
         }
        state = IDLE;
        memset(serialBuffer, 0, LONGEST_FRAME_LENGTH); 

      } else {
        /*
         * If no, put data into buffer
         */
        serialBuffer[receiverIndex++] = data;
      }
      
    }

  }

}
