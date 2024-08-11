/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   backgroundTasks.cpp - The source file containing functions which handle and
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

#include <Arduino.h>
#include <EEPROM.h>

#include "extendedFunctionality.h"
#include "backgroundTasks.h"

#define button1 2
#define button2 3
#define button3 4
#define button4 5
#define lcdLED 10
#define ldr A0

byte brightCurve(short sensor) {
  /* brightCurve - Function that converts a sensor value (usually read from the
       LDR) to a value suitable for writing to the LCD backlight to control
       brightness. Uses the reciprocal brightness equation.
       Parameters:
         sensor - An integer usually between 0 and 1023 (from analogRead) to
           convert to a backlight value.
       Returns: An integer of range 1 -> 255 suitable for analogWriting to the
         LCD Backlight.
  */

  if (sensor >= 729) {return 255;}
  if (sensor <= 110) {return 1;}
  return (100.0 / (4.0 - (0.005 * sensor))) - 28.0;
  //     ^        ^      ^              ^^^
  //     |        |      |______________|||
  //     |        |______________________||
  //     |________________________________|
}

void background(unsigned short sleepDuration) {
  /* background - Function which halts execution for the supplied number of
       milliseconds while still absorbing button presses, recording light
       intensity values and adjusting brightness. Similar to delay() but
       carries out background tasks.
       Parameters:
         sleepDuration - Integer number of milliseconds to halt for.
       Returns: N/A
  */
  unsigned long sleepTimer = millis();
  while (millis() - sleepTimer < sleepDuration) {
    getPressed();
  }
}

byte getPressed() {
  /* getPressed - The function which handles reading button presses and setting
        the LCD Backlight brightness automatically if required. Records the
        time when a button was last pressed/released and only reports a change
        of state after 100ms to avoid debounce. Also records the highest and
        lowest light intensity values observed within 1 second and passes the
        average to the reciprocal brightness equation for setting automatic
        brightness. This function should be called at every iteration of an
        'infinite' loop to insure user input and brightness is not blocked.
        Parameters: N/A
        Returns: Integer representing number of button pressed. 0 if no button
          is pressed or if number has already been returned by a prior call
          (absorbed).   
  */

  // initialise brightness related variables and record current light intensity
  static short minSensor = 1024;
  static short maxSensor = 0;
  static unsigned long brightTimer = millis();
  short curSensor = analogRead(ldr);

  // initialise button press related variables
  static unsigned long pressTimer = 0;
  static byte lastPressed = 0;
  byte curPressed;

  // set the highest and lowest sensor values if a new boundary is recorded
  if (curSensor < minSensor) {minSensor = curSensor;}
  if (curSensor > maxSensor) {maxSensor = curSensor;}

  // every second set the LCD brightness to the average light intensity if
  // automatic brightness is enabled and reset boundaries and timer
  if (millis() - brightTimer >= 1000) {
    if (brightness == 0) {analogWrite(lcdLED, brightCurve((maxSensor + minSensor) / 2));}
    minSensor = 1024;
    maxSensor = 0;
    brightTimer = millis();
  }

  // identify the currently pressed button (0 = none pressed)
  if (digitalRead(button1) == LOW) {curPressed = 1;}
  else if (digitalRead(button2) == LOW) {curPressed = 2;}  
  else if (digitalRead(button3) == LOW) {curPressed = 3;}
  else if (digitalRead(button4) == LOW) {curPressed = 4;}
  else {curPressed = 0;}

  // return the currently pressed button if 100ms has passed since the last
  // release and the press has not yet been recorded and returned
  if (curPressed > 0 && lastPressed == 0 && millis() - pressTimer >= 100) { // ALLOW PRESS 100MS AFTER RELEASE
    lastPressed = curPressed;
    pressTimer = millis();
    return curPressed;
  // mark buttons as unpressed (release) if no button is currently pressed and
  // 100ms has passed since the last initial press
  } else if (curPressed == 0 && lastPressed > 0 && millis() - pressTimer >= 100) { // ALLOW RELEASE 100MS AFTER PRESS
    lastPressed = 0;
    pressTimer = millis();
  }

  // only return >0 the first time button is pressed, otherwise always 0
  return 0;
}

bool updateBrightness() {
  /* updateBrightness - Function which sets the LCD backlight brightness to a
       value based on the global 'brightness' variable or on the current light
       intensity if automatic brightness (0) is set. Displays the brightness
       UI for 2 seconds, allowing the user to further increment/decrement the
       variable or access the debug mode by holding both buttons 1 and 2.
       Parameters: N/A
       Returns: Boolean which is true when the user has changed the brightness
         so the UI needs to be redrawn and backlight updated. Avoids recursion
         by letting the caller handle redraws.
  */

  // store current brightness value in EEPROM
  EEPROM.update(4, brightness);  

  // set up LCD with UI title
  lcd.setCursor(5, 0);
  lcd.print(F("BRIGHTNESS"));

  lcd.setCursor(1, 2);

  // buffer for holding 'progress bar'
  char bar[19];

  // automatic brightness: set bar to reflect this and backlight based on light
  if (brightness == 0) {
    strcpy_P(bar, reinterpret_cast<const char *>(F("\2      AUTO      \4")));
    analogWrite(lcdLED, brightCurve(analogRead(ldr)));
  } else {
    // manual brightness: turn backlight off if brightness is 1 or use
    // reciprocal brightness equation to set it if between 2 and 17
    analogWrite(lcdLED, brightness == 1 ? 0 : brightCurve(41.3 * (brightness - 2) + 110));

    // construct bar buffer with correct number of block characters
    bar[0] = 2; // LEFT BOUND
    bar[17] = 4; // RIGHT BOUND
    bar[18] = 0; // NULL TERMINATOR
    
    memset(bar + 1, 3, brightness - 1); // POINTER ARITHMETIC!!!
    memset(bar + brightness, ' ', 17 - brightness);
  }

  // print bar to LCD - final UI element
  lcd.print(bar);

  // wait for 2 seconds before returning false unless brightness is changed via
  // buttons 3 or 4 which returns true prematurely prompting redraw
  unsigned long prev = millis();
  while (millis() - prev <= 2000) {
    switch (getPressed()) {
      case 3: {
        // BUTTON 3: increment brightness in range 0 -> 17
        brightness = (brightness + 1) % 18;
        return true;
      } case 4: {
        // BUTTON 4: decrement brightness in range 0 -> 17
        brightness = (brightness + 17) % 18;
        return true;
      }
    }
  }

  // enter debug mode if both buttons 1 and 2 are held at the end of the 2s
  if (digitalRead(button1) == LOW && digitalRead(button2) == LOW) {debug();}

  // no further change to brightness has been made, redraw not required
  return false;
}
