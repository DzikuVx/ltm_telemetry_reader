# Light Telemetry - LTM protocol

LTM is low bandwidth one direction telemetry protocol designed to work just fine with baud rates of 1200/2400bps. Even with such low speed it can archive update frequency of 5Hz to 10Hz. Is implemented by various UAV software, both flight controller, OSD, antenna trackers, ground stations:

* [Cleanflight](https://github.com/cleanflight/cleanflight)
* [iNav](https://github.com/iNavFlight/inav)
* [Ghettostation](https://github.com/KipK/Ghettostation)
* [mwptools](https://github.com/stronnag/mwptools)
* and other

# LTM Messages

Message format: ```$Tx<payload><ck>``` where x:
* G : GPS Frame
* A : Attitude Frame
* S : Status Frame
* O : Origin Frame
* N : Navigation Frame (inav extension)

Payload is little endian data elements

Checksum is the uchar8 xor of payload

## GPS Frame (G)

Payload: 14 bytes

* Latitude, int32 [decimal degrees * 10,000,000]
* Longitude, int32 [decimal degrees * 10,000,000]
* Ground Speed, uchar [m/s]
* Altitude, uint32, [cm]
* Sats, uchar,
    * bits 0-1 : fix
    * bits 2-7 : number of satellites

## Attitide Frame (A)

Payload: 6 bytes

* Pitch, int16, [degrees]
* Roll, int16, [degrees]
* Heading, int16, [degrees]

## Status Frame (S)

Payload: 7 bytes

* Vbat, uint16, [mV]
* Current, uint16, [mA]
* RSSI, uchar
* Airspeed, uchar8, [m/s]
* Status, uchar
    * bit 0: Armed
    * bit 1: Failsafe
	* bits 2-6 : Flight mode:
        * 0 : Manual
        * 1 : Rate
	    * 2 : Angle
	    * 3 : Horizon
	    * 4 : Acro
        * 5 : Stabilized1
        * 6 : Stabilized2
        * 7 : Stabilized3
        * 8 : Altitude Hold
        * 9 : GPS Hold
        * 10 : Waypoints
        * 11 : Head free
        * 12 : Circle
        * 13 : RTH
        * 14 : Follow me
        * 15 : Land
        * 16 : Fly by wire A
        * 17 : Fly by wire B
        * 18 : Cruise
        * 19 : Unknown

## Origin Frame (O)

Payload: 14 bytes

* Latitude, int32 [decimal degrees * 10,000,000]
* Longitude, int32 [decimal degrees * 10,000,000]
* Altitude, uint32, [cm] - in case of [iNav](https://github.com/iNavFlight/inav), this is always 0
* OSD on, uchar  - always 1
* Fix, uchar, home fix status:
    * 0 : no fix

## Navigation Frame (N)

Payload: 6 bytes

* GPS mode, [uchar], with following values:
    * 0 : None
    * 1 : PosHold
    * 2 : RTH
    * 3 : Mission
* Nav mode, uchar:
    * 0 : None
    * 1 : RTH Start
    * 2 : RTH Enroute
    * 3 : PosHold infinite
    * 4 : PosHold timed
    * 5 : WP Enroute
    * 6 : Process next
    * 7 : Jump
    * 8 : Start Land
    * 9 : Land in Progress
    * 10 : Landed
    * 11 : Settling before land
    * 12 : Start descent
* Nav Action, uchar (not all used in inav):
    * 0 : UNASSIGNED
    * 1 : WAYPOINT
    * 2 : POSHOLD_UNLIM
    * 3 : POSHOLD_TIME
    * 4 : RTH
    * 5 : SET_POI
    * 6 : JUMP
    * 7 : SET_HEAD
    * 8 : LAND
* Waypoint number, uchar - number of currently processed wayoint
* Nav Error, uchar:
    * 0 : Navigation system is working
    * 1 : Next waypoint distance is more than the safety limit, aborting mission
    * 2 : GPS reception is compromised - pausing mission, COPTER IS ADRIFT!
    * 3 : Error while reading next waypoint from memory, aborting mission
    * 4 : Mission Finished
    * 5 : Waiting for timed position hold
    * 6 : Invalid Jump target detected, aborting mission
    * 7 : Invalid Mission Step Action code detected, aborting mission
    * 8 : Waiting to reach return to home altitude
    * 9 : GPS fix lost, mission aborted - COPTER IS ADRIFT!
    * 10 : Copter is disarmed, navigation engine disabled
    * 11 : Landing is in progress, check attitude if possible
* Flags, uchar

Protocol state on 23 March 2015 based on [https://github.com/stronnag/mwptools/blob/master/docs/ltm-definition.txt](https://github.com/stronnag/mwptools/blob/master/docs/ltm-definition.txt)