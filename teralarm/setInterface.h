/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   setInterface.cpp - The header file containing the functions which draw the
     User Interfaces for altering various settings and handle their frontend
     logic, as well as some helper functions for showing confirmation /
     cancellation feedback.
     External Variables / Constants:
       lcd - Hardware object representing LCD.
     Third Party Includes:
       Arduino.h - The main Arduino library containing all platform specific
         functions and constants.
       LiquidCrystal_I2C.h - Library used to interface with the LCD over the
         I2C bus.
     Local Includes:
       backgroundTasks.h - Handles brightness related requirements and reads
         user input from buttons in a non-blocking way.
       setInterface.h - Own header file.

   (C) RW128k 2022
*/

#ifndef SETINTERFACE_H
#define SETINTERFACE_H

#include "BufferedLCD.h"

extern BufferedLCD lcd;

bool chTime(byte &setHrs, byte &setMins);
bool chMinsSecs(byte &setMins, byte &setSecs);
bool chDate(byte &setDay, byte &setMonth, short &setYear);
bool chArray(const char *iter[], byte bound, byte &setIndex);
bool chChallenge(byte &setNum);

void confirm();
void cancel();

#endif
