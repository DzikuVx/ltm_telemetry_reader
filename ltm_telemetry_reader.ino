#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
//#include "printf.h"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

SoftwareSerial ltmSerial(8, 9);

String fixTypes[3] = {
  "NO",
  "2D",
  "3D"
};

void setup() {
  Serial.begin(9600);

  ltmSerial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();

//  printf_begin();
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
#define XFRAMELENGTH 10

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

  int32_t latitude;
  int32_t longitude;
  int32_t altitude;
  uint8_t groundSpeed; 
  int16_t hdop;
  uint8_t gpsFix;
  uint8_t gpsSats;

  int32_t homeLatitude;
  int32_t homeLongitude;

  uint8_t sensorStatus;
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

int32_t readInt32(uint8_t offset) {
  return (int32_t) serialBuffer[offset] + ((int32_t) serialBuffer[offset + 1] << 8) + ((int32_t) serialBuffer[offset + 2] << 16) + ((int32_t) serialBuffer[offset + 3] << 24);
}

uint32_t nextDisplay = 0;


/*************************************************************************
 * //Function to calculate the distance between two waypoints
 *************************************************************************/
float calc_dist(float flat1, float flon1, float flat2, float flon2)
{
    float dist_calc=0;
    float dist_calc2=0;
    float diflat=0;
    float diflon=0;

    //I've to spplit all the calculation in several steps. If i try to do it in a single line the arduino will explode.
    diflat=radians(flat2-flat1);
    flat1=radians(flat1);
    flat2=radians(flat2);
    diflon=radians((flon2)-(flon1));

    dist_calc = (sin(diflat/2.0)*sin(diflat/2.0));
    dist_calc2= cos(flat1);
    dist_calc2*=cos(flat2);
    dist_calc2*=sin(diflon/2.0);
    dist_calc2*=sin(diflon/2.0);
    dist_calc +=dist_calc2;

    dist_calc=(2*atan2(sqrt(dist_calc),sqrt(1.0-dist_calc)));

    dist_calc*=6371000.0; //Converting to meters
    //Serial.println(dist_calc);
    return dist_calc;
}


void loop() {

  if (millis() >= nextDisplay) {

    display.clearDisplay();

    display.setCursor(0,0);
    display.print("Lat:");
    display.print(remoteData.latitude);

    display.setCursor(0,9);
    display.print("Lon:");
    display.print(remoteData.longitude);

    display.setCursor(0,18);
    display.print("HDOP:");
    display.print(remoteData.hdop);

    display.setCursor(0, 27);
    display.print("Fix:");
    display.print(fixTypes[remoteData.gpsFix]);

    display.setCursor(64, 27);
    display.print("Sat:");
    display.print(remoteData.gpsSats);

    display.setCursor(0, 36);
    display.print("Spd:");
    display.print(remoteData.groundSpeed);

    display.setCursor(64, 36);
    display.print("Alt:");
    display.print(remoteData.altitude);

    display.setCursor(0, 45);
    display.print("Roll:");
    display.print(remoteData.roll);


    display.display();

    nextDisplay = millis() + 50;
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
        case 'X':
          frameLength = XFRAMELENGTH;
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

        if (frameType == 'G') {
            remoteData.latitude = readInt32(0);
            remoteData.longitude = readInt32(4);
            remoteData.groundSpeed = readByte(8);
            remoteData.altitude = readInt32(9);

            uint8_t raw = readByte(13);
            remoteData.gpsSats = raw >> 2;
            remoteData.gpsFix = raw & 0x03;
        }

        if (frameType == 'X') {
            remoteData.hdop = readInt(0);
            remoteData.sensorStatus = readByte(2);
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
