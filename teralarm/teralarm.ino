/* TERALARM (PROTOTYPE 2) - The effective alarm clock

   teralarm.ino - The main Arduino source file containing the setup and loop
     functions which run the startup procedure and clockface backend
     respectively. Global variables and constants are declared here, some of
     which are used in other source files.
     External Variables / Constants: N/A
     Third Party Includes:
       EEPROM.h - Adruino library used to read and write settings from EEPROM.
       LiquidCrystal_I2C.h - Library used to interface with the LCD over the
         I2C bus.
       DS3231.h - Library used to interface with the RTC over the I2C bus.
     Local Includes:
       setInterface.h - Used to create and handle frontend for altering
         settings.
       backgroundTasks.h - Handles brightness related requirements and reads
         user input from buttons in a non-blocking way.
       extendedFunctionality.h - Provides function for the secret countdown
         timer.
       clockAlarmInterface.h - Handles the drawing of the clockface and the
         entire alarm procedure.

   (C) RW128k 2022
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <DS3231.h>

#include "BufferedLCD.h"
#include "setInterface.h"
#include "backgroundTasks.h"
#include "extendedFunctionality.h"
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

// hardware objects
BufferedLCD lcd(0x27, 20, 4);
DS3231 rtc(SDA, SCL);

// synchronised EEPROM values in RAM
byte alarmMins;
byte alarmHrs;
byte alarmChallenge;
byte alarmSnoozeSecs;
byte alarmSnoozeMins;
bool alarmState;
byte brightness;

// current time struct
Time timeObj;

// string constants
const char *dows[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
const char *months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char *stateStrs[2] = {"OFF", "ON"};
const char titleStr[13] = "PROTOTYPE 02";

void setup() {
  /* setup - Standard Arduino setup function. Called once on microcontroller
       start up. Sets up hardware, reads saved values from EEPROM into memory,
       displays boot animation, allows access to hidden countdown timer and
       prints serial messages. Should not be called manually.
       Parameters: N/A
       Returns: N/A
  */

  // set up hardware objects
  lcd.begin();
  rtc.begin();
  Serial.begin(9600);
  
  // set pin modes for IO
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(button4, INPUT);
  pinMode(buzzer, OUTPUT); 
  pinMode(lcdLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(ldr, INPUT);

  // create custom LCD characters for blinking cursor and brightness bar
  byte blinkChar[8] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
  byte brightBoundL[8] = {0b00011, 0b00011, 0b00011, 0b00011, 0b00011, 0b00011, 0b00011, 0b00011};
  byte brightFill[8] = {0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000};
  byte brightBoundR[8] = {0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000};

  // add custom LCD characters to LCD object to be used in printing
  lcd.createChar(1, blinkChar);
  lcd.createChar(2, brightBoundL);
  lcd.createChar(3, brightFill);
  lcd.createChar(4, brightBoundR);

  // set buzzer to off (as it is active low) and LCD to maximum brightness
  digitalWrite(buzzer, HIGH);
  analogWrite(lcdLED, 255);

  // synchronise EEPROM values with RAM, using defaults if out of bounds
  alarmHrs = EEPROM.read(0) >= 0 && EEPROM.read(0) <= 23 ? EEPROM.read(0) : 0;
  alarmMins = EEPROM.read(1) >= 0 && EEPROM.read(1) <= 59 ? EEPROM.read(1) : 0;
  alarmChallenge = EEPROM.read(2) >= 0 && EEPROM.read(2) <= 99 ? EEPROM.read(2) : 10;
  alarmSnoozeMins = EEPROM.read(3) >= 0 && EEPROM.read(3) <= 59 ? EEPROM.read(3) : 0;
  alarmSnoozeSecs = EEPROM.read(4) >= 0 && EEPROM.read(4) <= 59 ? EEPROM.read(4) : 0;
  alarmState = EEPROM.read(5) == 1;
  brightness = EEPROM.read(6) >= 0 && EEPROM.read(6) <= 17 ? EEPROM.read(6) : 0;

  // set the seed for generating random numbers based on RTC time
  randomSeed(rtc.getUnixTime(rtc.getTime()));

  // array of 80 y and x coordinates representing characters on the LCD to be
  // randomly filled, creating boot animation
  byte coords[80][2];
  
  // populate the array with coordinates in order left to right, top to bottom
  for (byte i=0; i<20; i++) {
     for (byte j=0; j<4; j++) {
      coords[i*4 + j][0] = j; // y coordinate
      coords[i*4 + j][1] = i; // x coordinate
     }
  } 

  // loop over set of 80 unfilled coordinates, reducing set size each iteration
  for (byte maxi = 80; maxi > 0; maxi--) {
    // select coordinate pair at random from remaining set and fill it on LCD
    byte pos = random(0, maxi);
    lcd.setCursor(coords[pos][1], coords[pos][0]);
    lcd.print(F("\1"));
    delay(20);

    // replace recently filled coordinate with rightmost unfilled
    coords[pos][0] = coords[maxi-1][0];
    coords[pos][1] = coords[maxi-1][1];
  }

  // iterate over each character of title and print it to the LCD every 100ms
  for (byte i = 0; i < 12; i++) {
    char titleChar[2] = {titleStr[i], 0}; // convert single character to null terminated string
    lcd.setCursor(4 + i, 1);
    lcd.print(titleChar);
    delay(100);
  }

  // fill in previously printed title to make completely filled screen
  lcd.setCursor(4, 1);
  lcd.print(F("\1\1\1\1\1\1\1\1\1\1\1\1"));
  delay(150);
  
  // if all buttons are held down at this point, halt automatic brightness,
  // absorb presses, clear LCD and enter secret timer function
  if (digitalRead(button1) == LOW && digitalRead(button2) == LOW && digitalRead(button3) == LOW && digitalRead(button4) == LOW) {
    if (brightness == 0) {brightness = 255;}
    consumePress();
    lcd.clear();
    secretTimer();
    // revert automatic brightness: never reached but kept for portability
    if (brightness == 255) {brightness = 0;}
  }
  
  // print entire title again
  delay(250);
  lcd.setCursor(4, 1);
  lcd.print(titleStr);

  // only play buzzer sound for 0.5s if no buttons are held
  if (digitalRead(button1) == HIGH && digitalRead(button2) == HIGH && digitalRead(button3) == HIGH && digitalRead(button4) == HIGH) {
    digitalWrite(buzzer, LOW);
  }

  // blink both LEDs for 0.5s then disable buzzer regardless of if muted above
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, HIGH);
  delay(500);
  digitalWrite(buzzer, HIGH);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);

  // print credits line on LCD
  lcd.setCursor(4, 2);
  lcd.print(F("-RWGUNN '22-"));

  // print credits art, title, time and date over serial
  Serial.println(F(" ____          ____"));
  Serial.println(F("|    | |    | |      |    | |\\   | |\\   |"));
  Serial.println(F("|____| |    | |  __  |    | | \\  | | \\  |"));
  Serial.println(F("|  \\   | /\\ | |    | |    | |  \\ | |  \\ |"));
  Serial.println(F("|   \\  |/  \\| |____| |____| |   \\| |   \\| (C) RWGUNN 2022\n"));
  Serial.println(F("WELCOME TO PROTOTYPE 02."));
  Serial.print(F("THE CURRENT TIME IS "));
  Serial.print(rtc.getTimeStr(FORMAT_SHORT));
  Serial.print(F(" ON "));
  Serial.print(rtc.getDateStr(FORMAT_LONG, FORMAT_LITTLEENDIAN, '/'));
  Serial.println(F(".\n"));
    
  // give the user time (2.5s) to read the static LCD then clear for drawing
  // the clockface
  delay(2500);
  lcd.clear();

  // send final serial message informing setup has finished
  Serial.println(F("THE SYSTEM IS NOW OPERATIONAL AND NO FURTHER SERIAL COMMUNICATION WILL BE PROVIDED."));

  // set the LCD brightness to the users preference: light intensity from LDR
  // if automatic or scaled value if manual all passed through reciprocal
  // brightness equation or 0 if off
  analogWrite(lcdLED, brightness == 0 ? brightCurve(analogRead(ldr)) : brightness == 1 ? 0 : brightCurve(41.3 * (brightness - 2) + 110));

  // absorb button presses before entering main loop to force clockface
  consumePress();
}

void loop() {
  /* loop - Standard Arduino main loop function. Called after setup terminates
       and every time it terminates itself. Checks if the clockface needs to be
       redrawn, checks if the alarm needs to be triggered and handles all user
       input / button presses on clockface. Most backend logic for editing time
       settings, alarm settings and brightness is contained here, while the
       front end (UI) is invoked in this function but its logic is handled by
       source files setInterface and backgroundTasks. Should not be called
       manually.
       Parameters: N/A
       Returns: N/A
  */

  // variable which stores whether alarm has been disabled in current minute to
  // stop it triggering directly after being disabled if time is the same
  static bool alarmDisabled = false;
  
  // get RTC time and paint / update the clockface every iteration of the loop
  timeObj = rtc.getTime();
  updateTime();

  // sound alarm if the current time equals the alarm time and it has not been
  // disabled already in the current minute
  if (timeObj.hour == alarmHrs && timeObj.min == alarmMins && !alarmDisabled && alarmState) {
    consumePress();
    lcd.clear();
    soundAlarm();
    // mark alarm as disabled for current minute
    alarmDisabled = true;
    consumePress();
    lcd.clear();
  }

  // mark alarm as not already disabled if the current time is not the alarm
  // time and alarm is marked (incorrectly) as disabled for current minute
  if ((timeObj.hour != alarmHrs || timeObj.min != alarmMins) && alarmDisabled) {alarmDisabled = false;}

  // handle user input and run background tasks on every iteration of loop
  switch (getPressed()) {
    // BUTTON 1: alter time and date
    case 1: {
      // load current time object into memory to copy from for manipulation
      timeObj = rtc.getTime();

      /* SET TIME */

      // set up UI background on LCD (clear and print title)
      consumePress();
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print(F("SET TIME:"));

      // load current hours and minutes into memory for manipulation
      byte setHrs = timeObj.hour;
      byte setMins = timeObj.min;

      // create UI for altering above time values and run its loop
      if (chTime(setHrs, setMins)) {
        // update time to altered values on RTC if confirmed
        rtc.setTime(setHrs, setMins, 0);

        // paint confirmation UI with new time and play buzzer sound
        lcd.clear();
        lcd.setCursor(4, 1);
        lcd.print(F("TIME SET TO:"));
        lcd.setCursor(7, 2);
        lcd.print(rtc.getTimeStr(FORMAT_SHORT));
        confirm();    
      } else {
        // paint cancellation UI and play buzzer sound if cancelled
        cancel();
        // do not proceed with altering other settings, run main loop again
        return;
      }
      
      /* SET DATE */

      // print title on LCD
      lcd.setCursor(5, 0);
      lcd.print(F("SET DATE:"));

      // load current day, month and year into memory for manipulation
      byte setDay = timeObj.date;
      byte setMonth = timeObj.mon;
      short setYear = timeObj.year;

      // create UI for altering above date values and run its loop
      if (chDate(setDay, setMonth, setYear)) {
        // update date to altered values on RTC if confirmed
        rtc.setDate(setDay, setMonth, setYear);

        // paint confirmation UI with new date and play buzzer sound
        lcd.clear();
        lcd.setCursor(4, 1);
        lcd.print(F("DATE SET TO:"));
        lcd.setCursor(5, 2);
        lcd.print(rtc.getDateStr(FORMAT_LONG, FORMAT_LITTLEENDIAN, '/'));
        confirm();    
      } else {
        // paint cancellation UI and play buzzer sound if cancelled
        cancel();
        // do not proceed with altering other settings, run main loop again
        return;
      }

      /* SET WEEKDAY */

      // print title on LCD
      lcd.setCursor(4, 0);
      lcd.print(F("SET WEEKDAY:"));

      // load current weekday into memory for manipulation
      byte setDow = timeObj.dow;

      // create UI for altering above weekday value and run its loop
      if (chArray(dows, 7, setDow)) {
        // update weekday to altered value on RTC if confirmed
        rtc.setDOW(setDow);

        // paint confirmation UI with new weekday and play buzzer sound
        lcd.clear();
        lcd.setCursor(2, 1);
        lcd.print(F("WEEKDAY SET TO:"));
        // time object must be recreated to read new weekday from RTC
        timeObj = rtc.getTime();
        // numerical to textual weekday: calculate LCD position and print
        lcd.setCursor(byte((20 - strlen(dows[timeObj.dow - 1])) / 2), 2);
        lcd.print(dows[timeObj.dow - 1]);
        confirm();    
      } else {
        // paint cancellation UI and play buzzer sound if cancelled
        cancel();
      }
      
      break;

    // BUTTON 2: alter alarm settings
    } case 2: {
      /* SET ALARM TIME */

      // set up UI background on LCD (clear and print title)
      consumePress();
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print(F("SET ALARM:"));

      // copy current alarm hours and minutes to new memory for manipulation
      byte setHrs = alarmHrs;
      byte setMins = alarmMins;

      // create UI for altering above time values and run its loop
      if (chTime(setHrs, setMins)) {
        // update alarm time to altered values in RAM and EEPROM if confirmed
        alarmHrs = setHrs;
        alarmMins = setMins;
        EEPROM.update(0, alarmHrs);
        EEPROM.update(1, alarmMins);

        // paint confirmation UI with new alarm time and play buzzer sound
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print(F("ALARM SET TO:"));
        lcd.setCursor(7, 2);
        // create buffer and format time string into buffer then print to LCD
        char confStr[6];
        sprintf(confStr, "%02d:%02d", alarmHrs, alarmMins);
        lcd.print(confStr);
        confirm();    
      } else {
        // paint cancellation UI and play buzzer sound if cancelled
        cancel();
        // do not proceed with altering other settings, run main loop again
        return;        
      }

      /* SET CHALLENGE */

      // print title on LCD
      lcd.setCursor(3, 0);
      lcd.print(F("SET CHALLENGE:"));

      // copy current challenge to new memory for manipulation
      byte setNum = alarmChallenge;

      // create UI for altering above challenge value and run its loop
      if (chChallenge(setNum)) {
        // update challenge to altered value in RAM and EEPROM if confirmed
        alarmChallenge = setNum;
        EEPROM.update(2, alarmChallenge);

        // paint confirmation UI with new challenge and play buzzer sound
        lcd.clear();
        lcd.setCursor(1, 1);
        lcd.print(F("CHALLENGE SET TO:"));
        // create buffer and format challenge into buffer then print to LCD
        char confStr[5];
        if (alarmChallenge == 0) {
          lcd.setCursor(8, 2);
          strcpy_P(confStr, reinterpret_cast<const char *>(F("NONE")));
        } else {
          lcd.setCursor(9, 2);
          sprintf(confStr, "%d", alarmChallenge);
        }
        lcd.print(confStr);
        confirm();
      } else {
        // paint cancellation UI and play buzzer sound if cancelled
        cancel();
        // do not proceed with altering other settings, run main loop again
        return;
      }

      /* SET SNOOZE */

      // print title on LCD
      lcd.setCursor(4, 0);
      lcd.print(F("SET SNOOZE:"));

      // copy current snooze minutes and seconds to new memory for manipulation
      byte setSnoozeMins = alarmSnoozeMins;
      byte setSnoozeSecs = alarmSnoozeSecs;

      // create UI for altering above snooze value and run its loop
      if (chMinsSecs(setSnoozeMins, setSnoozeSecs)) {
        // update snooze period to altered value in RAM and EEPROM if confirmed
        alarmSnoozeMins = setSnoozeMins;
        alarmSnoozeSecs = setSnoozeSecs;
        EEPROM.update(3, alarmSnoozeMins);
        EEPROM.update(4, alarmSnoozeSecs);

        // paint confirmation UI with new snooze period and play buzzer sound
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print(F("SNOOZE SET TO:"));
        // create buffer and format snooze string into buffer then print to LCD
        char confStr[7];
        if (alarmSnoozeMins == 0 && alarmSnoozeSecs == 0) {
          lcd.setCursor(8, 2);
          strcpy_P(confStr, reinterpret_cast<const char *>(F("NONE")));
        } else {
          lcd.setCursor(7, 2);
          sprintf(confStr, "%02dm%02ds", alarmSnoozeMins, alarmSnoozeSecs);
        }
        lcd.print(confStr);
        confirm();
      } else {
        // paint cancellation UI and play buzzer sound if cancelled
        cancel();
        // do not proceed with altering other settings, run main loop again
        return;
      }

      /* SET STATE */

      // print title on LCD
      lcd.setCursor(5, 0);
      lcd.print(F("SET STATE:"));

      // copy current state to new memory for manipulation (boolean to integer)
      byte setState = alarmState ? 2 : 1;

      // create UI for altering above state value and run its loop
      if (chArray(stateStrs, 2, setState)) {
        // update state to altered value in RAM and EEPROM if confirmed
        alarmState = setState == 2; // integer to boolean (RAM)
        EEPROM.update(5, alarmState ? 1 : 0); // boolean to integer (EEPROM)

        // paint confirmation UI with new state and play buzzer sound
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print(F("STATE SET TO:"));
        // numerical to textual state: calculate LCD position and print
        lcd.setCursor(alarmState ? 9 : 8, 2);
        lcd.print(alarmState ? stateStrs[1] : stateStrs[0]);
        confirm();    
      } else {
        // paint cancellation UI and play buzzer sound if cancelled
        cancel();
      }

      break;

    // BUTTON 3: increment brightness and draw brightness UI
    } case 3: {
      // increase brightness in range 0, 1 -> 17 (AUTO, OFF -> MAX)
      brightness = (brightness + 1) % 18;
      // if UI function returns true, brightness changed further so call again
      lcd.clear();
      while (updateBrightness());
      consumePress();
      lcd.clear();
      break;

    // BUTTON 4: decrement brightness and draw brightness UI
    } case 4:{
      // decrease brightness in range 0, 1 -> 17 (AUTO, OFF -> MAX)
      brightness = (brightness + 17) % 18; // rollunder 0 -> 17
      // if UI function returns true, brightness changed further so call again
      lcd.clear();
      while (updateBrightness());
      consumePress();
      lcd.clear();
      break;
    }
  }
}
