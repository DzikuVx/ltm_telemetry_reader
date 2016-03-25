#include <SoftwareSerial.h>
#include "printf.h"

SoftwareSerial ltmSerial(8, 9);

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    delay(100);
  }

  ltmSerial.begin(9600);

  printf_begin();
  
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

const char* flightModes[] = {
  "Manual",
  "Rate",
  "Angle",
  "Horizon",
  "Acro",
  "Stabilized1",
  "Stabilized2",
  "Stabilized3",
  "Altitude Hold",
  "GPS Hold",
  "Waypoints",
  "Head free",
  "Circle",
  "RTH",
  "Follow me",
  "Land",
  "Fly by wire A",
  "Fly by wire B",
  "Cruise",
  "Unknown"
};

typedef struct remoteData_s {
  int pitch;
  int roll;
  int heading;
  uint16_t voltage;
  byte rssi;
  bool armed;
  bool failsafe;
  byte flightmode;
} remoteData_t;

remoteData_t remoteData;

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

uint32_t nextDisplay = 0; 

void loop() {

  if (millis() >= nextDisplay) {
//    Serial.print("Pitch:");
//    Serial.println(remoteData.pitch);
//    Serial.print("Roll:");
//    Serial.println(remoteData.roll);
//    Serial.print("Heading:");
//    Serial.println(remoteData.heading);
    Serial.println(flightModes[remoteData.flightmode]);

    nextDisplay = millis() + 1000;
  }

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
            remoteData.pitch = readInt(0);
            remoteData.roll = readInt(2);
            remoteData.heading = readInt(4);
        }

        if (frameType == 'S') {
            remoteData.voltage = readInt(0);
            remoteData.rssi = readByte(4);

            byte raw = readByte(6);
            remoteData.flightmode = raw >> 2;            
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
