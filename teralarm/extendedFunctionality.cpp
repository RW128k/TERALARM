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

  // once a button has been pressed, absorb and prepare to start the countdown
  consumePress();
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
    }

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

  // absorb presses from caller and clear LCD
  consumePress();
  lcd.clear();

  // print constant values to LCD
  lcd.setCursor(0, 1);
  lcd.print(F("TEMPERATURE: "));
  lcd.setCursor(0, 2);
  lcd.print(F("LIGHT: "));
  lcd.setCursor(0, 3);
  lcd.print(F("UPTIME:   d  h  m  s"));

  // loop (drawing debug UI) until a button is pressed
  while(getPressed() == 0) {
    // buffer for holding the string of the currently processing line
    char lineBuff[21];
    int length;

    // print raw light intensity value followed by newline over serial
    sprintf(lineBuff, "%d", analogRead(ldr));
    Serial.println(lineBuff);

    // redraw only every 0.2 seconds
    if (millis() - prev < 200) {
      continue;
    }

    // fill buffer for TOP LINE based on carousel position
    switch (carousel / 10) {
      case 0: {
        // static debug mode title
        strcpy_P(lineBuff, reinterpret_cast<const char *>(F("\1\1\1\1\1DEBUG MODE\1\1\1\1\1")));
        length = 20;
        break;
      } case 1: {
        // unix time from RTC
        length = sprintf(lineBuff, "UNIX: %ld", rtc.getUnixTime(rtc.getTime()));
        break;
      } case 2: {
        // numerical day of week from RTC and (textual version)
        byte numDow = rtc.getTime().dow;
        length = sprintf(lineBuff, "DAY: %d (%s)", numDow, dows[numDow - 1]);
        break;
      } case 3: {
        // alarm time from RAM (synced with EEPROM)
        length = sprintf(lineBuff, "ALARM TIME: %02d:%02d", alarmHrs, alarmMins);
        break;
      } case 4: {
        // alarm challenge from RAM (synced with EEPROM)
        length = sprintf(lineBuff, "ALARM CHALLENGE: %d", alarmChallenge);
        break;
      } case 5: {
        // alarm state from RAM (synced with EEPROM) numerical and textual form
        if (alarmState) {
          strcpy_P(lineBuff, reinterpret_cast<const char *>(F("ALARM STATE: 1 (ON)")));
          length = 19;
        } else {
          strcpy_P(lineBuff, reinterpret_cast<const char *>(F("ALARM STATE: 0 (OFF)")));
          length = 20;
        }
        break;
      } case 6: {
        // internal numerical brightness from RAM (synced with EEPROM)
        length = sprintf(lineBuff, "BRIGHTNESS: %d", brightness);
        if (brightness == 0) {
          strcat(lineBuff, " (AUTO)");
          length += 7;
        } else if (brightness == 1) {
          strcat(lineBuff, " (OFF)");
          length += 6;
        } else if (brightness == 17) {
          strcat(lineBuff, " (MAX)");
          length += 6;
        }
        break;
      }
    }

    // print FIRST LINE from buffer
    memset(lineBuff + length, ' ', 20 - length); // 20 being the total characters in the line
    lineBuff[20] = 0;
    lcd.setCursor(0, 0);
    lcd.print(lineBuff);

    // print TEMPERATURE at second line with 0.1 degree precision
    char tempStr[6];
    length = sprintf(lineBuff, "%s%cC", dtostrf(rtc.getTemp(), 0, 1, tempStr), 223); // 233 is character code for degree symbol
    memset(lineBuff + length, ' ', 7 - length); // 7 being the remaining characters in the line following "TEMPERATURE: " -->
    lineBuff[7] = 0;
    lcd.setCursor(13, 1);
    lcd.print(lineBuff);

    // print LIGHT INTENSITY AND (BRIGHTNESS TO WRITE) at third line
    short light = analogRead(ldr);
    length = sprintf(lineBuff, "%d (%d)", light, brightCurve(light));
    memset(lineBuff + length, ' ', 13 - length); // 13 being the remaining characters in the line following "LIGHT: " -->
    lineBuff[13] = 0;
    lcd.setCursor(7, 2);
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
    sprintf(lineBuff, "%02d %02d %02d %02ld", days, hours, mins, secs);

    // insert terminators between values (use single buffer for 4 strings)
    lineBuff[2] = 0;
    lineBuff[5] = 0;
    lineBuff[8] = 0;

    // print the values individually
    lcd.setCursor(8, 3);
    lcd.print(lineBuff);
    lcd.setCursor(11, 3);
    lcd.print(lineBuff + 3);
    lcd.setCursor(14, 3);
    lcd.print(lineBuff + 6);
    lcd.setCursor(17, 3);
    lcd.print(lineBuff + 9);

    // increment carousel and reset timer
    prev = millis();
    carousel = (carousel+1) % 70;
  }
}
