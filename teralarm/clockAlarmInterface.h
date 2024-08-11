/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   clockAlarmInterface.cpp - The header file containing the functions which
     draw the clockface and run both front and backend logic for sounding the
     alarm.
     External Variables / Constants:
       lcd - Hardware object representing LCD.
       rtc - Hardware object representing RTC.
       alarmMins - The minutes value of the time the alarm is set for
         (synchronised with EEPROM).
       alarmHrs - The hours value of the time the alarm is set for
         (synchronised with EEPROM).
       alarmChallenge - The challenge value of the alarm (synchronised with
         EEPROM).
       alarmState - Boolean state value determining whether the alarm is
         enabled or disabled (synchronised with EEPROM).
       brightness - Brightness setting value (synchronised with EEPROM).
       timeObj - Shared current Date / Time object across sources.
       dows - Constant array of strings holding the days of the week.
     Third Party Includes:
       Arduino.h - The main Arduino library containing all platform specific
         functions and constants.
       LiquidCrystal_I2C.h - Library used to interface with the LCD over the
         I2C bus.
       DS3231.h - Library used to interface with the RTC over the I2C bus.
     Local Includes:
       backgroundTasks.h - Handles brightness related requirements and reads
         user input from buttons in a non-blocking way.
       clockAlarmInterface.h - Own header file.

   (C) RW128k 2022
*/

#ifndef CLOCKALARMINTERFACE_H
#define CLOCKALARMINTERFACE_H

#include <DS3231.h>

#include "BufferedLCD.h"

extern BufferedLCD lcd;
extern DS3231 rtc;

extern byte alarmMins;
extern byte alarmHrs;
extern byte alarmChallenge;
extern bool alarmState;
extern byte brightness;

extern Time timeObj;
extern const char *dows[7];
extern const char *months[12];

void updateTime();
void soundAlarm();

#endif
