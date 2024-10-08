/* TERALARM (FIRMWARE 3) - The effective alarm clock

   extendedFunctionality.cpp - The header file containing functions which
     provide features that are hidden / not included in the standard UI, namely
     a 100 second countdown timer and a debug mode.
     External Variables / Constants:
       lcd - Hardware object representing LCD.
       rtc - Hardware object representing RTC.
       alarmMins - The minutes value of the time the alarm is set for
         (synchronised with EEPROM).
       alarmHrs - The hours value of the time the alarm is set for
         (synchronised with EEPROM).
       alarmChallenge - The challenge value of the alarm (synchronised with
         EEPROM).
       alarmSnoozeSecs - The seconds value of the time period to snooze for
         (synchronised with EEPROM).
       alarmSnoozeMins - The minutes value of the time period to snooze for
         (synchronised with EEPROM).
       alarmState - Boolean state value determining whether the alarm is
         enabled or disabled (synchronised with EEPROM).
       brightness - Brightness setting value (synchronised with EEPROM).
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
       extendedFunctionality.h - Own header file.

   (C) RW128k 2022
*/

#ifndef EXTENDEDFUNCTIONALITY_H
#define EXTENDEDFUNCTIONALITY_H

#include <DS3231.h>

#include "BufferedLCD.h"

extern BufferedLCD lcd;
extern DS3231 rtc;
extern byte alarmMins;
extern byte alarmHrs;
extern byte alarmChallenge;
extern byte alarmSnoozeSecs;
extern byte alarmSnoozeMins;
extern bool alarmState;
extern byte brightness;
extern const char *dows[7];

void secretTimer();
void debug();

#endif
