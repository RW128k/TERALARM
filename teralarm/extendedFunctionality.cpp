/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   extendedFunctionality.cpp - The source file containing functions which
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

#include <Arduino.h>

#include "backgroundTasks.h"
#include "extendedFunctionality.h"

#define buzzer 8
#define redLED 11
#define blueLED 12
#define ldr A0

void secretTimer() {
  /* secretTimer - Function that provides a UI to start and monitor a 100 
       second countdown timer. A blinking instruction is shown on the LCD until
       any button is pressed and the timer is started, displaying remaining
       time on the screen with 0.1 second precision. The red LED and buzzer are
       flashed and sounded at a decreasing interval, until the timer ends and
       the blue LED flashes infinitely. This function will never return and
       requires the system to be reset to resume normal operation.
       Parameters: N/A
       Returns: N/A (NEVER)
  */

  // initialise variables for timing and tracking user interface events
  unsigned long prev = 0;
  bool blinkText = false;

  // set up LCD with initial UI title
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("100 SECOND COUNTDOWN"));

  // loop (wait and blink text) until a button is pressed
  while(getPressed() == 0) {
    // only execute loop body every 0.75 seconds
    if (millis() - prev < 750) {continue;}

    // toggle printing instruction to screen based on flag
    lcd.setCursor(2, 2);
    lcd.print(blinkText ? F("                ") : F("PRESS ANY BUTTON"));
    lcd.setCursor(6, 3);
    lcd.print(blinkText ? F("        ") : F("TO START"));
    
    // update flag and set last blinked time to now (to wait 0.75 seconds)
    blinkText = !blinkText;
    prev = millis();
  }

  // once a button has been pressed, prepare to start the countdown
  lcd.clear();

  // declare elapsed to hold the time since prev (when timer began) in seconds
  prev = millis();
  float elapsed;

  // loop for duration of timer (100 seconds)
  while((elapsed = (millis() - prev) / 1000.0) < 100) {
    // calculate the floating point remainder of the elapsed time divided by
    // the current interval between flashes/buzzes. the current interval is
    // determined by a range of elapsed times (eg 20->40s elapsed: 4s interval)
    float remainder;
    if (elapsed >= 97) {
      remainder = fmod(elapsed, 0.1);
    } else if (elapsed >= 90) { //^ 0.1 (INTERVAL) DIVISOR ALWAYS YIELDS < 0.2 FOR CONSTANT BUZZ
      remainder = fmod(elapsed, 0.25);
    } else if (elapsed >= 85) {
      remainder = fmod(elapsed, 0.5);
    } else if (elapsed >= 80) {
      remainder = fmod(elapsed, 0.625);
    } else if (elapsed >= 70) {
      remainder = fmod(elapsed, 1);
    } else if (elapsed >= 60) {
      remainder = fmod(elapsed, 2);
    } else if (elapsed >= 40) {
      remainder = fmod(elapsed, 2.5);
    } else if (elapsed >= 20) {
      remainder = fmod(elapsed, 4);
    } else {
      remainder = fmod(elapsed, 5);

    // buzz and flash for 0.2 seconds (until 0.2s after interval point)
    // remainder gives time passed since points defined by interval
    if (remainder >= 0 && remainder <= 0.2) {
      digitalWrite(buzzer, LOW);
      digitalWrite(redLED, HIGH);
    } else {
      digitalWrite(buzzer, HIGH);
      digitalWrite(redLED, LOW);
    }

    // buffer for holding remaining seconds string
    char timerStr[5];

    // put remaining seconds with 0.1 precision string in the buffer, avoiding
    // 100s (too many digits) and right aligning to pad with 0 if necessary
    dtostrf(99.95 - elapsed, 0, 1, elapsed >= 90 && elapsed <= 99.95 ? timerStr + 1 : timerStr);
    if (elapsed >= 90) {
      // pad with 0 if <10 seconds remain or overwrite minus with 0 if negative
      timerStr[0] = '0';
    }

    // print REMAINING TIME in seconds to the upper centre of the LCD
    lcd.setCursor(8, 1);
    lcd.print(timerStr);
  }

  // silence buzzer and turn of red LED when timer has ended
  digitalWrite(buzzer, HIGH);
  digitalWrite(redLED, LOW);
  
  blinkText = false;
  // loop (blinking time on screen and LED) forever. will never terminate
  while (true) {
    // only execute loop body every 0.75 seconds
    if (millis() - prev < 750) {continue;}
    
    // toggle printing remaining time (0s) to screen and blue LED based on flag
    lcd.setCursor(8, 1);
    lcd.print(blinkText ? F("    ") : F("00.0"));
    digitalWrite(blueLED, blinkText ? LOW : HIGH);

    // update flag and set last blinked time to now (to wait 0.75 seconds)
    blinkText = !blinkText;
    prev = millis();      
  }
}

void debug() {
  /* debug - Function which displays various measurements and settings in their
       internal/raw form. The top line of the LCD displays a carousel of
       settings stored in EEPROM and data held in the RTC registers while the
       remaining 3 lines show dynamic measurements (light intensity,
       temperature and uptime). Each item in the carousel is shown for 2
       seconds. Raw light intensity measurements are printed over serial on
       every loop. Debug mode can be exited by pressing any button.
       Parameters: N/A
       Returns: N/A
  */

  // initialise variables for timing and tracking carousel position
  unsigned long prev = 0;
  byte carousel = 0;

  // loop (drawing debug UI) until a button is pressed
  while(getPressed() == 0) {
    // print raw light intensity value followed by newline over serial
    Serial.println(analogRead(A0));

    // only execute loop body and redraw every 1 second
    if (millis() - prev < 1000) {continue;}
    lcd.clear();

    // buffer for holding the string of the currently processing line
    char lineBuff[21];

    // fill buffer for TOP LINE based on carousel position (divided by 2 as
    // carousel increments every second but should display for 2 seconds)
    switch (carousel / 2) {
      case 0: {
        // static debug mode title
        strcpy(lineBuff, "\1\1\1\1\1DEBUG MODE\1\1\1\1\1");
        break;
      } case 1: {
        // unix time from RTC
        sprintf(lineBuff, "UNIX: %ld", rtc.getUnixTime(rtc.getTime()));
        break;
      } case 2: {
        // numerical day of week from RTC and (textual version)
        byte numDow = rtc.getTime().dow;
        sprintf(lineBuff, "DAY: %d (%s)", numDow, dows[numDow - 1]);
        break;
      } case 3: {
        // alarm time from RAM (synced with EEPROM)
        sprintf(lineBuff, "ALARM TIME: %02d:%02d", alarmHrs, alarmMins);
        break;
      } case 4: {
        // alarm challenge from RAM (synced with EEPROM)
        sprintf(lineBuff, "ALARM CHALLENGE: %d", alarmChallenge);
        break;
      } case 5: {
        // alarm state from RAM (synced with EEPROM) numerical and textual form
        strcpy(lineBuff, alarmState ? "ALARM STATE: 1 (ON)" : "ALARM STATE: 0 (OFF)");
        break;
      } case 6: {
        // internal numerical brightness from RAM (synced with EEPROM)
        sprintf(lineBuff, "BRIGHTNESS: %d", brightness);
        strcat(lineBuff, brightness == 0 ? " (AUTO)" : brightness == 1 ? " (OFF)" : brightness == 17 ? " (MAX)" : "");
        break;
      }
    }

    // print FIRST LINE from buffer and increment carousel, wrapping at 13
    lcd.setCursor(0, 0);
    lcd.print(lineBuff);
    carousel = (carousel+1) % 14;

    // print TEMPERATURE at second line with 0.1 degree precision
    char tempStr[6];
    sprintf(lineBuff, "TEMPERATURE: %s%cC", dtostrf(rtc.getTemp(), 0, 1, tempStr), 223); // 233 is character code for degree symbol
    lcd.setCursor(0, 1);
    lcd.print(lineBuff);

    // print LIGHT INTENSITY AND (BRIGHTNESS TO WRITE) at third line
    short light = analogRead(ldr);
    sprintf(lineBuff, "LIGHT: %d (%d)", light, brightCurve(light));
    lcd.setCursor(0, 2);
    lcd.print(lineBuff);
    
    // parse uptime by spliting milliseconds to days, hours, minutes, seconds
    unsigned long secs = millis() / 1000;
    byte days = secs / 86400;
    secs = secs % 86400;
    byte hours = secs / 3600;
    secs = secs % 3600;
    byte mins = secs / 60;
    secs = secs % 60;

    // print UPTIME in split form at fourth line
    sprintf(lineBuff, "UPTIME: %dd%dh%dm%lds", days, hours, mins, secs);
    lcd.setCursor(0, 3);
    lcd.print(lineBuff);

    // update last loop time to now
    prev = millis();
  }
}
