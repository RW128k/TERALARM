/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   clockAlarmInterface.cpp - The source file containing the functions which
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
       alarmSnoozeSecs - The seconds value of the time period to snooze for
         (synchronised with EEPROM).
       alarmSnoozeMins - The minutes value of the time period to snooze for
         (synchronised with EEPROM).
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

#include <Arduino.h>

#include "backgroundTasks.h"
#include "clockAlarmInterface.h"

#define button1 2
#define button2 3
#define button3 4
#define button4 5
#define buzzer 8
#define lcdLED 10
#define redLED 11
#define blueLED 12
#define ldr A0

void updateTime() {
  /* updateTime - Function which draws the clockface to the LCD. Shows alarm
       time, temperature, RTC time and full date. Completely clears and redraws
       the UI which can cause flickering if called frequently. Could be fixed
       by buffering LCD contents.
       Parameters: N/A
       Returns: N/A
   */

  // buffer for entire line on clockface
  char lineBuff[21];
  
  // print ALARM TIME at top left or 'OFF' if alarm disabled
  char alarmStr[6];
  if (alarmState) {
    sprintf(alarmStr, "%02d:%02d", alarmHrs, alarmMins);
  } else {
    strcpy_P(alarmStr, reinterpret_cast<const char *>(F("OFF")));
  }
  lcd.setCursor(0, 0);
  lcd.print(alarmStr);
  
  // print TEMPERATURE at top right
  char tempStr[6];
  int tempLength = sprintf(tempStr, "%d%cC", int(rtc.getTemp()), 223); // 233 is character code for degree symbol
  memset(lineBuff, ' ', 15 - tempLength);
  memcpy(lineBuff + 15 - tempLength, tempStr, tempLength + 1);
  lcd.setCursor(5, 0);
  lcd.print(lineBuff);
  
  // print RTC TIME at upper centre
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeObj.hour, timeObj.min, timeObj.sec);
  lcd.setCursor(6, 1);
  lcd.print(timeStr);
  
  // print DATE spread out over lower centre and bottom by loading the entire
  // date string into buffer and storing pointer to begining of second line
  char dateUpperStr[28];
  int dateUpperLength = sprintf(dateUpperStr, "%s %d ", dows[timeObj.dow - 1], timeObj.date);
  int dateLowerLength = sprintf(dateUpperStr + dateUpperLength, "%s %d", months[timeObj.mon - 1], timeObj.year);
  // set the pointer to the start of the second line to the year if the month
  // can fit on first line or the month if it can't
  char *dateLowerStr = dateUpperLength + dateLowerLength > 25 ? dateUpperStr + dateUpperLength : dateUpperStr + dateUpperLength + dateLowerLength - 4;

  // (re)calculate upper / lower date lengths and positions
  dateLowerLength = dateUpperStr + dateUpperLength + dateLowerLength - dateLowerStr;
  dateUpperLength = dateLowerStr - dateUpperStr - 1;
  int dateLowerPos = (20 - dateLowerLength) / 2;
  int dateUpperPos = (20 - dateUpperLength) / 2;

  // place null terminator at end of first line (before start second line)
  dateUpperStr[dateUpperLength] = 0;

  // print UPPER DATE line centrally, padding remaining characters in line
  memset(lineBuff, ' ', dateUpperPos);
  memset(lineBuff + dateUpperPos + dateUpperLength, ' ', 20 - (dateUpperPos + dateUpperLength));
  memcpy(lineBuff + dateUpperPos, dateUpperStr, dateUpperLength);
  lineBuff[20] = 0;
  lcd.setCursor(0, 2);
  lcd.print(lineBuff);

  // print LOWER DATE line centrally, padding remaining characters in line
  memset(lineBuff, ' ', dateLowerPos);
  memset(lineBuff + dateLowerPos + dateLowerLength, ' ', 20 - (dateLowerPos + dateLowerLength));
  memcpy(lineBuff + dateLowerPos, dateLowerStr, dateLowerLength);
  lineBuff[20] = 0;
  lcd.setCursor(0, 3);
  lcd.print(lineBuff);
}

void soundAlarm() {
  /* soundAlarm - Function called to trigger the alarm and to draw the UI which
       provides a means to disarm. Instructs user to push a sequence of buttons
       of length provided by the global alarmChallenge variable. Displays the
       time and instruction on the LCD while sounding the buzzer and flashing
       the LEDs. Handles button presses as answer to instruction and adds/ 
       subtracts points given correct/incorrect answers until the challenge
       number is reached and the system is disarmed.
       Parameters: N/A
       Returns: N/A
  */

  // initialise variable for storing number of correct answers
  byte points = 0;
  randomSeed(rtc.getUnixTime(rtc.getTime()));
  // set brightness to maximum while alarm is sounding
  analogWrite(lcdLED, 255);
  // halt automatic brightness if enabled, reverts when alarm disabled
  if (brightness == 0) {brightness = 255;}

  // loop for each question. ends when enough points gained 
  do {
    // initialise timer variables and flags for sounding the buzzer / flashing 
    // LEDs and blinking the LCD text. generate random number for question 
    byte num = random(1, 5);
    unsigned long prev1 = 0;
    unsigned long prev2 = 0;
    bool blinkText = false;
    bool buzz = false;

    // initial buzzer state off
    noTone(buzzer);
    digitalWrite(buzzer, HIGH);

    // main infinite loop that runs until question is answered correctly
    while (true) {
      // every 1 second redraw UI and toggle alarm text visibility
      if (millis() - prev1 > 1000) {
        timeObj = rtc.getTime();

        // print ALARM TEXT at top left depending on flag and then update flag
        lcd.setCursor(0, 0);
        lcd.print(blinkText ? "ALARM!" : "      ");
        blinkText = !blinkText;

        // print RTC TIME at upper centre
        char timeStr[9];
        sprintf(timeStr, "%02d:%02d:%02d", timeObj.hour, timeObj.min, timeObj.sec);
        lcd.setCursor(6, 1);
        lcd.print(timeStr);

        // show question and progress if challenge is not 0
        if (alarmChallenge > 0) {
          // print QUESTION at bottom
          char instructionStr[9];
          sprintf(instructionStr, "ENTER: %d", num);
          lcd.setCursor(6, 3);
          lcd.print(instructionStr);

          // print PROGRESS (number of points) at top right
          char pointsStr[6];
          sprintf(pointsStr, "%d/%d", points + 1, alarmChallenge); // +1 as internally 0 indexed
          lcd.setCursor(20 - strlen(pointsStr), 0);
          lcd.print(pointsStr);
        } else {
          // print NO CHALLENGE INSTRUCTION at bottom
          lcd.setCursor(2, 3);
          lcd.print(F("PRESS ANY BUTTON"));
        }
        
        prev1 = millis();
      }

      // every 0.1 seconds change buzzer tone and toggle LEDs
      if (millis() - prev2 > 100) {
        // change tone and LED depending on flag and then update flag
        if (buzz) {
          noTone(buzzer);
          digitalWrite(buzzer, HIGH);
          digitalWrite(buzzer, LOW);
          buzz = false;
          digitalWrite(redLED, HIGH);
          digitalWrite(blueLED, LOW);
        } else {
          digitalWrite(buzzer, HIGH);
          tone(buzzer, 2000);
          buzz = true;
          digitalWrite(redLED, LOW);
          digitalWrite(blueLED, HIGH);
        }
        prev2 = millis();
      }

      // get currently pressed button to handle as answer and run background
      byte pressed = getPressed();

      // if no button is pressed skip answer handling and loop again
      if (pressed == 0) {continue;}

      // if challenge is set to none break from loop on any button press
      if (alarmChallenge <= 0) {
        // disable buzzer
        noTone(buzzer);
        digitalWrite(buzzer, HIGH);
        consumePress();
        lcd.clear();
        break;
      // if challenge is > 0 and answer is correct increment points and break
      } else if (pressed == num) {
        points++;
        // show feedback: blue LED flash, buzzer tone and LCD message
        lcd.clear();
        lcd.setCursor(6, 1);
        digitalWrite(buzzer, HIGH);
        digitalWrite(redLED, LOW);
        digitalWrite(blueLED, HIGH);
        lcd.print(F("CORRECT!"));
        tone(buzzer, 2000);
        delay(500);
        tone(buzzer, 1000);
        delay(500);
        noTone(buzzer);
        digitalWrite(buzzer, HIGH);
        digitalWrite(blueLED, LOW);
        consumePress();
        lcd.clear();
        // break from loop shows next question or disables alarm
        break;
      // if challenge is > 0 and answer is incorrect decrement points and break
      } else {
        // do not decrement if there are no points (minimum points: 0)
        if (points != 0) {
          points--;
        }
        // show feedback: red LED flash, buzzer tone and LCD message
        lcd.clear();
        lcd.setCursor(5, 1);
        digitalWrite(buzzer, HIGH);
        digitalWrite(redLED, HIGH);
        digitalWrite(blueLED, LOW);
        lcd.print(F("INCORRECT!"));
        tone(buzzer, 1000);
        delay(500);
        tone(buzzer, 2000);
        delay(500);
        noTone(buzzer);
        digitalWrite(buzzer, HIGH);
        digitalWrite(redLED, LOW);
        consumePress();
        lcd.clear();
        // do not break, loop again for the same question but with less points
      }
    }
  } while (points < alarmChallenge);

  // outer loop ended so alarm disabled. print DISABLED MESSAGE at upper centre
  lcd.setCursor(3, 1);
  lcd.print(F("ALARM DISABLED!"));

  // turn off both LEDs
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);

  // sound buzzer 3 times (400ms off, 400ms on) to signify alarm disabled
  for (byte i = 0; i < 6; i++) {
    delay(400);
    digitalWrite(buzzer, i % 2 == 0 ? LOW : HIGH);
    digitalWrite(blueLED, i % 2 == 0 ? HIGH : LOW);   
  }

  // resume automatic brightness if enabled and previously halted
  if (brightness == 255) {
    brightness = 0;
    analogWrite(lcdLED, brightCurve(analogRead(ldr)));
  // if brightness was manually selected, revert to value before override
  } else {
    analogWrite(lcdLED, brightness == 1 ? 0 : brightCurve(41.3 * (brightness - 2) + 110));
  }

  // return and do not snooze if snooze is set to NONE (00:00)
  if (alarmSnoozeMins == 0 && alarmSnoozeSecs == 0) {return;}

  // draw static parts of snooze countdown UI (title and progres bar bounds)
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print(F("SNOOZING"));
  lcd.setCursor(0, 3);
  lcd.print(F("\2"));
  lcd.setCursor(19, 3);
  lcd.print(F("\4"));

  // initialise timing variables for snooze and flags for flashing the LED
  unsigned long snoozeTimer = millis();
  unsigned long snoozeMillis = ((alarmSnoozeMins * 60) + alarmSnoozeSecs) * 1000UL;
  bool flash = true;

  // loop until snooze period has elapsed, tracking elapsed time each iteration
  for (unsigned long elapsed; (elapsed = millis() - snoozeTimer) < snoozeMillis;) {
    // calculate remaining snooze time in minutes and seconds and progress
    // scaled to 0 -> 18 (number of units on progress bar)
    byte remainingMins = (snoozeMillis - elapsed) / 60000;
    byte remainingSecs = ((snoozeMillis - elapsed) % 60000) / 1000;
    byte progress = ((float) elapsed / snoozeMillis) * 18;

    // run background tasks
    getPressed();

    // print REMAINING TIME at lower center
    char remainingStr[6];
    sprintf(remainingStr, "%02d:%02d", remainingMins, remainingSecs);
    lcd.setCursor(7, 2);
    lcd.print(remainingStr);

    // character string for progrss bar
    char bar[19];
    bar[18] = 0;

    // populate the progress bar with calculated number of block characters and
    // remaining whitespace
    if ((elapsed - ((progress * snoozeMillis) / 18)) % 1000 >= 500) {
      // show additional block for 500ms every 1000ms after last block added
      memset(bar, 3, progress + 1);
      memset(bar + progress + 1, ' ', 17 - progress);
    } else {
      // otherwise show only correct number of blocks in progress bar
      memset(bar, 3, progress);
      memset(bar + progress, ' ', 18 - progress);
    }

    // print PROGRESS BAR at bottom
    lcd.setCursor(1, 3);
    lcd.print(bar);

    // flash the blue LED for 200ms every 5000ms, tracking state with flag
    if (elapsed % 5000 >= 4800 && flash) {
      digitalWrite(blueLED, HIGH);
      flash = false;
    } else if (elapsed % 5000 < 4800 && !flash) {
      digitalWrite(blueLED, LOW);
      flash = true;
    }

    // skip snooze if all buttons are held during countdown
    if (digitalRead(button1) == LOW && digitalRead(button2) == LOW && digitalRead(button3) == LOW && digitalRead(button4) == LOW) {
      // absorb button presses and clear LCD from countdown
      consumePress();
      lcd.clear();

      // print SKIPPED MESSAGE at upper centre
      lcd.setCursor(2, 1);
      lcd.print(F("SNOOZE SKIPPED!"));

      // sound buzzer 3 times (200ms off, 200ms on) to signify snooze skipped
      for (byte i = 0; i < 6; i++) {
        background(200);
        digitalWrite(buzzer, i % 2 == 0 ? LOW : HIGH);
        digitalWrite(blueLED, i % 2 == 0 ? HIGH : LOW);
      }

      // show message for additional 800ms after last buzzer before returning
      background(800);

      return;
    }
  }

  // set brightness to maximum during snooze alert
  analogWrite(lcdLED, 255);
  // halt automatic brightness if enabled, reverts when alert dismissed
  if (brightness == 0) {brightness = 255;}

  // absorb button presses, turn off blue LED and clear LCD from countdown
  consumePress();
  digitalWrite(blueLED, LOW);
  lcd.clear();

  // print SNOOZE ALERT TITLE at top
  lcd.setCursor(3, 0);
  lcd.print(F("SNOOZE ELAPSED"));

  // reset timing variable and LED flash flag for use in alert loop
  snoozeTimer = millis();
  flash = true;

  // loop until any button is pressed, tracking elapsed time and running
  // background tasks each iteration
  for (unsigned long elapsed = millis(); getPressed() == 0; elapsed = millis() - snoozeTimer) {
    // blink DISMISS INSTRUCTION on / off at lower center / bottom every 750ms
    if (elapsed % 1500 >= 750) {
        lcd.setCursor(2, 2);
        lcd.print(F("                "));
        lcd.setCursor(5, 3);
        lcd.print(F("          "));
    } else {
        lcd.setCursor(2, 2);
        lcd.print(F("PRESS ANY BUTTON"));
        lcd.setCursor(5, 3);
        lcd.print(F("TO DISMISS"));
    }

    // flash the red LED and sound buzzer for 200ms every 5000ms, tracking
    // state with flag
    if (elapsed % 5000 < 200 && flash) {
      digitalWrite(buzzer, LOW);
      digitalWrite(redLED, HIGH);
      flash = false;
    } else if (elapsed % 5000 >= 200 && !flash) {
      digitalWrite(buzzer, HIGH);
      digitalWrite(redLED, LOW);
      flash = true;
    }
  }

  // absorb button presses, turn off red LED and clear LCD from alert
  consumePress();
  digitalWrite(redLED, LOW);
  lcd.clear();

  // print ALERT DISMISSED MESSAGE at upper centre
  lcd.setCursor(5, 1);
  lcd.print(F("DISMISSED!"));

  // sound buzzer 3 times (400ms off, 400ms on) to signify alert dismissed
  for (byte i = 0; i < 6; i++) {
    delay(400);
    digitalWrite(buzzer, i % 2 == 0 ? LOW : HIGH);
    digitalWrite(blueLED, i % 2 == 0 ? HIGH : LOW);
  }

  // resume automatic brightness if enabled and previously halted
  if (brightness == 255) {
    brightness = 0;
    analogWrite(lcdLED, brightCurve(analogRead(ldr)));
  // if brightness was manually selected, revert to value before override
  } else {
    analogWrite(lcdLED, brightness == 1 ? 0 : brightCurve(41.3 * (brightness - 2) + 110));
  }
}
