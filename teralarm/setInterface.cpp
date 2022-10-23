/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   setInterface.cpp - The source file containing the functions which draw the
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

#include <Arduino.h>

#include "backgroundTasks.h"
#include "setInterface.h"

#define buzzer 8
#define redLED 11
#define blueLED 12

bool chTime(byte &setHrs, byte &setMins) {
  /* chTime - Function which provides a user interface to alter a specified
       time value. The confirm button can be used to move between hours and
       minutes and save the values while the up and down buttons can be used to
       increase and decrease the values respectively and the cancel button
       discards the values. Hours have a range of 0 to 23 and minutes have a
       range 0 to 59 which roll over / wrap. No LCD clearing or title is
       provided by this function and the values are simply repainted at the
       same position, so the LCD background (clear and title) should be set
       before calling. The currently selected value blinks every 0.5 seconds.
       Parameters:
         setHrs - An integer passed by reference which holds the hours value to
           be modified. Should be in range 0 -> 23.
         setMins - An integer passed by reference which holds the minutes value
           to be modified. Should be in range 0 -> 59.
       Returns:
         A boolean which is true when the values are to be saved (confirm
         button pressed when minutes selected) and false when the values are to
         be discarded (cancel button pressed).
  */

  // initialise variables for timing value blinking and tracking selected value
  unsigned long prev = 0;
  bool blinkText = false;
  bool set = true;
  
  // loop (blinking and handling input) until canceled or both values confirmed
  while (true) {

    // blink hours / minutes every 500ms
    if (millis() - prev >= 250) {

      // print flashing cursor if flag is true and update flag
      if (blinkText){
        // print cursor at hours or minutes position based on selected value
        lcd.setCursor(set ? 7 : 10, 2);
        lcd.print("\1\1");

        blinkText = false;
      // print selected value if flag is false and update flag
      } else {
        // format and repaint entire time string
        char setStr[6];
        sprintf(setStr, "%02d:%02d", setHrs, setMins);
        lcd.setCursor(7, 2);
        lcd.print(setStr);

        blinkText = true;
      }
      prev = millis();
    }
    
    // handle user input and run background tasks on every iteration of loop
    switch (getPressed()) {
      // BUTTON 1: confirm
      case 1: {
        // return save signal if last value selected else select next value
        if (set) {
          set = false;
          break;
        } else {
          return true;
        }

      // BUTTON 2: cancel and return discard signal
      } case 2: {
        return false;

      // BUTTON 3: increment selected value, respecting allowed ranges
      } case 3: {
        if (set) {
          setHrs = (setHrs + 1) % 24; // increase hours in range 0 -> 23
        } else {
          setMins = (setMins + 1) % 60; // increase minutes in range 0 -> 59
        }
        break;

      // BUTTON 4: decrement selected value, respecting allowed ranges
      } case 4: {
        if (set) {
          setHrs = (setHrs + 23) % 24; // decrease hours in range 0 -> 23
        } else {
          setMins = (setMins + 59) % 60; // decrease minutes in range 0 -> 59
        }
        break;
      }
    }    
  }
}

bool chDate(byte &setDay, byte &setMonth, short &setYear) {
  /* chDate - Function which provides a user interface to alter a specified
       date value. The confirm button can be used to move between days, months
       and years and save the values while the up and down buttons can be used
       to increase and decrease the values respectively and the cancel button
       discards the values. Days have a range of 1 to 31, months have a range 1
       to 12 and years have a range 1000 to 9999 which all roll over / wrap
       except for years which will not increment or decrement further than
       their range. No LCD clearing or title is provided by this function and
       the values are simply repainted at the same position, so the LCD
       background (clear and title) should be set before calling. The currently
       selected value blinks every 0.5 seconds.
       Parameters:
         setDay - An integer passed by reference which holds the day value to
           be modified. Should be in range 1 -> 31.
         setMonth - An integer passed by reference which holds the month value
           to be modified. Should be in range 1 -> 12.
         setYear - An integer passed by reference which holds the year value to
           to be modified. Should be in range 1000 -> 9999.
       Returns:
         A boolean which is true when the values are to be saved (confirm
         button pressed when year selected) and false when the values are to
         be discarded (cancel button pressed).
  */

  // initialise variables for timing value blinking and tracking selected value
  unsigned long prev = 0;
  bool blinkText = false;
  byte set = 0;
  
  // loop (blinking and handling input) until canceled or all values confirmed
  while (true) {
    
    // blink day / month / year every 500ms
    if (millis() - prev >= 250) {

      // print flashing cursor if flag is true and update flag
      if (blinkText) {
        // print cursor at day, month or year position based on selected value
        if (set == 0) {
          lcd.setCursor(5, 2);
          lcd.print("\1\1");
        } else if (set == 1) {
          lcd.setCursor(8, 2);
          lcd.print("\1\1");
        } else if (set == 2) {
          lcd.setCursor(11, 2);
          lcd.print("\1\1\1\1"); // appropriate size for year
        }

        blinkText = false;
      // print selected value if flag is false and update flag
      } else {
        // format and repaint entire date string
        char setStr[11];
        sprintf(setStr, "%02d/%02d/%d", setDay, setMonth, setYear);
        lcd.setCursor(5, 2);
        lcd.print(setStr);

        blinkText = true;
      }
      prev = millis();
    }

    // handle user input and run background tasks on every iteration of loop
    switch (getPressed()) {
      // BUTTON 1: confirm
      case 1: {
        // return save signal if last value selected else select next value
        set++;
        if (set == 3) {
          return true;
        }
        break;

      // BUTTON 2: cancel and return discard signal
      } case 2: {
        return false;         

      // BUTTON 3: increment selected value, respecting allowed ranges / limits
      } case 3: {
        if (set == 0) {
          setDay = (setDay % 31) + 1; // increase day in range 1 -> 31
        } else if (set == 1) {
          setMonth = (setMonth % 12) + 1; // increase month in range 1 -> 12
        } else if (setYear < 9999){
          setYear++; // increase year with upper limit of 9999
        }
        break;

      // BUTTON 4: decrement selected value, respecting allowed ranges / limits
      } case 4: {
        if (set == 0) {
          setDay = ((setDay + 29) % 31) + 1; // decrease day in range 1 -> 31
        } else if (set == 1) {
          setMonth = ((setMonth + 10) % 12) + 1; // decrease month in range 1 -> 12
        } else if (setYear > 1000){
          setYear--; // increase year with lower limit of 1000
        }
        break;
      }
    }
  }
}

bool chArray(const char *iter[], byte bound, byte &setIndex) {
  /* chArray - Function which provides a user interface to alter a specified
       array index while showing the contents at the current index. The confirm
       button can be used to save the index while the while the up and down
       buttons can be used to move between positions in the array (increase and
       decrease the index) and the cancel button discards the new index. The
       array index has a range 0 to the size specified by the bound parameter
       which rolls over / wraps. No LCD clearing or title is provided by this
       function and the contents at current index is simply repainted at the
       same position, so the LCD background (clear and title) should be set
       before calling. The array contents at index blinks every 0.5 seconds.
       Parameters:
         iter - A constant string array used to set the index for. Strings in
           this array should be 20 characters or less as this is the LCD width.
           The string at selected index by user will be shown on screen.
         bound - The highest index to increment to before wrapping back to 0.
           Usually this will be the length of the array however it could be
           smaller to omit the tail but it must not be larger as rubbish from
           memory will be printed.
         setIndex - An integer passed by reference which holds the index of
           array passed as iter to be modified. Should be in range 1 -> bound
           as one 1-indexed (subtract 1 to get actual index).
       Returns:
         A boolean which is true when the index is to be saved (confirm button
         pressed) and false when the index is to be discarded (cancel button
         pressed).
  */

  // initialise variables for timing value blinking
  unsigned long prev = 0;
  bool blinkText = false;
  
  // loop (blinking and handling input) until canceled or index confirmed
  while (true) {

    // blink array contents at index every 500ms
    if (millis() - prev >= 250) { 

      // clear entire second line to avoid leftovers from a previous index
      lcd.setCursor(0, 2);
      lcd.print("                    ");

      // set cursor based on size of contents at index so it prints in centre
      lcd.setCursor(int((20 - strlen(iter[setIndex - 1])) / 2), 2);

      // print flashing cursor if flag is true and update flag
      if (blinkText){
        // create buffer for storing cursor characters and fill it with as many
        // as the length of the string in array at index and print
        char dowBlink[21];
        memset(dowBlink, 1, strlen(iter[setIndex - 1])); 
        dowBlink[strlen(iter[setIndex - 1])] = 0; // null terminator
        lcd.print(dowBlink);

        blinkText = false;
      // print value at current index of array if flag is false and update flag
      } else {
        lcd.print(iter[setIndex - 1]);
        blinkText = true;
      }
      prev = millis();
    }
    
    // handle user input and run background tasks on every iteration of loop
    switch (getPressed()) {
      // BUTTON 1: confirm and return save signal
      case 1: {
        return true;

      // BUTTON 2: cancel and return discard signal
      } case 2: {
        return false;         

      // BUTTON 3: increment index in range 1 -> bound
      } case 3: {
        setIndex = (setIndex % bound) + 1;
        break;

      // BUTTON 3: decrement index in range 1 -> bound
      } case 4: {
        setIndex = ((setIndex + bound - 2) % bound) + 1;
        break;
      }
    }    
  }  
}

bool chChallenge(byte &setNum) {
  /* chChallenge - Function which provides a user interface to alter a
       specified challenge value. The confirm button can be used to save the
       challenge while the up and down buttons increase and decrease it
       respectively and the cancel button discards the challenge. The challenge
       has a range 0 to 99 which rolls over / wraps and displays 0 as NONE. No
       LCD clearing or title is provided by this function and the value is
       simply repainted at the same position, so the LCD background (clear and
       title) should be set before calling. The challenge value blinks every
       0.5 seconds.
       Parameters:
         setNum - An integer passed by reference which holds the challenge
           value be modified. Should be in range 0 -> 99.
       Returns:
         A boolean which is true when the challenge is to be saved (confirm
         button pressed) and false when the challenge is to be discarded
         (cancel button pressed).
  */

  // initialise variables for timing value blinking
  unsigned long prev = 0;
  bool blinkText = false;
  
  // loop (blinking and handling input) until canceled or challenge confirmed
  while (true) {

    // blink challenge every 500ms
    if (millis() - prev >= 250) { 

      // clear entire second line to avoid leftovers from a previous challenge
      lcd.setCursor(0, 2);
      lcd.print("                    ");

      // set cursor based on if number or 'NONE' is to be printed in centre
      lcd.setCursor(setNum == 0 ? 8 : 9, 2);

      // print flashing cursor if flag is true and update flag
      if (blinkText){
        // print cursor of size same as number of digits / letters in challenge
        lcd.print(setNum == 0 ? "\1\1\1\1" : setNum < 10 ? "\1" : "\1\1");

        blinkText = false;
      // print challenge if flag is false and update flag
      } else {
        // format and repaint numerical challenge or 'NONE' if 0
        char setStr[5];
        sprintf(setStr, setNum == 0 ? "NONE" : "%d", setNum);
        lcd.print(setStr);

        blinkText = true;
      }
      prev = millis();
    }
    
    // handle user input and run background tasks on every iteration of loop
    switch (getPressed()) {
      // BUTTON 1: confirm and return save signal
      case 1: {
        return true;

      // BUTTON 2: cancel and return discard signal
      } case 2: {
        return false;         

      // BUTTON 3: increment challenge in range 0 -> 99
      } case 3: {
        setNum = (setNum + 1) % 100;
        break;

      // BUTTON 4: decrement challenge in range 0 -> 99
      } case 4: {
        setNum = (setNum + 99) % 100;
        break;
      }
    }    
  }
}

void confirm() {
  /* confirm - Function that simply sounds the buzzer and flashes the blue LED
       2 times. Each buzz / flash lasts for 200ms with a 400ms gap in between.
       There is also a 400ms and 800ms delay at the start and end of the
       function respectively. This function carries out background tasks when
       idle to ensure automatic brightness is correct and to absorb button
       presses. Used to alert the user that the values have been saved. Nothing
       is printed on the LCD as text may be specific to the context.
       Parameters: N/A
       Returns: N/A
  */

  background(400);
  digitalWrite(buzzer, LOW); //on
  digitalWrite(blueLED, HIGH);
  background(200);
  digitalWrite(buzzer, HIGH); // off
  digitalWrite(blueLED, LOW);
  background(400);
  digitalWrite(buzzer, LOW); // on
  digitalWrite(blueLED, HIGH);
  background(200);
  digitalWrite(buzzer, HIGH); // off
  digitalWrite(blueLED, LOW);
  background(800);     
}

void cancel() {
  /* cancel - Function that simply sounds the buzzer and flashes the red LED
       for 1 second, while showing a cancelled message on the blank LCD. There
       is an additional 1 second delay at the end of the function. This
       function carries out background tasks when idle to ensure automatic
       brightness is correct and to absorb button presses. Used to alert the
       user that the values have been discarded.
       Parameters: N/A
       Returns: N/A
  */

  // clear and display LCD message
  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.print("CANCELLED!");

  digitalWrite(buzzer, LOW); // on
  digitalWrite(redLED, HIGH);
  background(1000);
  digitalWrite(buzzer, HIGH); // off
  digitalWrite(redLED, LOW);
  background(1000);
}
