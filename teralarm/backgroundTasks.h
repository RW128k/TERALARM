/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   backgroundTasks.h - The header file containing functions which handle and
     manipulate manual / automatic brightness as well as capturing user input
     in a manner that does not halt execution.
     External Variables / Constants:
       lcd - Hardware object representing LCD.
       brightness - Brightness setting value (synchronised with EEPROM).
     Third Party Includes:
       Arduino.h - The main Arduino library containing all platform specific
         functions and constants.
       EEPROM.h - Adruino library used to write brightness to EEPROM.
       LiquidCrystal_I2C.h - Library used to interface with the LCD over the
         I2C bus.
     Local Includes:
       extendedFunctionality.h - Provides function for debug mode, accessible
         via brightness UI.
       backgroundTasks.h - Own header file.

   (C) RW128k 2022
*/

#ifndef BACKGROUNDTASKS_H
#define BACKGROUNDTASKS_H

#include <LiquidCrystal_I2C.h>

extern LiquidCrystal_I2C lcd;
extern byte brightness;

byte brightCurve(short sensor);
void background(unsigned short sleepDuration);
byte getPressed();
bool updateBrightness();

#endif
