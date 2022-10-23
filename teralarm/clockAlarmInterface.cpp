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
       alarmState - Boolean state value determining whether the alarm is
         enabled or disabled (synchronised with EEPROM).
       brightness - Brightness setting value (synchronised with EEPROM).
       timeObj - Shared current Date / Time object across sources.
       dows - Constant array of strings holding the days of the week.
       timeStr - Shared string of colon delimited hours, minutes and seconds
         across sources.
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
  timeObj = rtc.getTime();
  
  lcd.clear();
  
  // print ALARM TIME at top left or 'OFF' if alarm disabled
  char alarmStr[6];
  sprintf(alarmStr , alarmState ? "%02d:%02d" : "OFF", alarmHrs, alarmMins);
  lcd.setCursor(0, 0);
  lcd.print(alarmStr);
  
  // print TEMPERATURE at top right
  char tempStr[6];
  sprintf(tempStr, "%d%cC", int(rtc.getTemp()), 223); // 233 is character code for degree symbol
  lcd.setCursor(int(20 - strlen(tempStr)), 0);
  lcd.print(tempStr);
  
  // print RTC TIME at upper centre
  strcpy(timeStr, rtc.getTimeStr());
  lcd.setCursor(6, 1);
  lcd.print(timeStr);
  
  // print DATE spread out over lower centre and bottom by loading the entire
  // date string into buffer and storing pointer to begining of second line
  char dateStr[23];
  char *yearStr;
  sprintf(dateStr, "%s %d %s %d", dows[timeObj.dow - 1], timeObj.date, rtc.getMonthStr(), timeObj.year);

  // set the pointer to the start of the second line to the year if the month
  // can fit on first line or the month if it can't
  yearStr = dateStr + (strlen(dateStr) - (strlen(dateStr) > 25 ? strlen(rtc.getMonthStr()) + 1 : 0) - 4);
  // place null terminator at end of first line (before start second line)
  *(yearStr - 1) = 0;

  // print first and second lines of date
  lcd.setCursor(int((20 - strlen(dateStr)) / 2), 2);
  lcd.print(dateStr);
  lcd.setCursor(int((20 - strlen(yearStr)) / 2), 3);
  lcd.print(yearStr);
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
        lcd.clear();

        // print ALARM TEXT at top left depending on flag and then update flag
        if (blinkText) {
          lcd.setCursor(0, 0);
          lcd.print("ALARM!");
          blinkText = false;
        } else {
          blinkText = true;
        }

        // print RTC TIME at upper centre
        strcpy(timeStr, rtc.getTimeStr());
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
          lcd.setCursor(3, 3);
          lcd.print("PRESS ANY KEY");
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
        lcd.print("CORRECT!");
        tone(buzzer, 2000);
        delay(500);
        tone(buzzer, 1000);
        delay(500);
        noTone(buzzer);
        digitalWrite(buzzer, HIGH);
        digitalWrite(blueLED, LOW);
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
        lcd.print("INCORRECT!");
        tone(buzzer, 1000);
        delay(500);
        tone(buzzer, 2000);
        delay(500);
        noTone(buzzer);
        digitalWrite(buzzer, HIGH);
        digitalWrite(redLED, LOW);
        // do not break, loop again for the same question but with less points
      }
    }
  } while (points < alarmChallenge);

  // outer loop ended so alarm disabled. print DISABLED MESSAGE at upper centre
  lcd.clear();
  lcd.setCursor(3, 1);
  lcd.print("ALARM DISABLED!");

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
}
