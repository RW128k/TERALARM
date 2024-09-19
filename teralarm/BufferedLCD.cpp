/* TERALARM (FIRMWARE 3) - The effective alarm clock

   BufferedLCD.cpp - The source file containing overrides for LCD class methods
     which buffer the on-screen contents before sending to the hardware. Print
     calls containing identical data to that in the buffer will not be resent
     to the hardware, saving on overhead and reducing LCD flickers.
     External Variables / Constants: N/A
     Third Party Includes:
       Arduino.h - The main Arduino library containing all platform specific
         functions and constants.
       LiquidCrystal_I2C.h - Library used to interface with the LCD over the
         I2C bus.
     Local Includes:
       BufferedLCD.h - Own header file.

   (C) RW128k 2024
*/

#include "BufferedLCD.h"

BufferedLCD::BufferedLCD(uint8_t addr, uint8_t cols, uint8_t rows, uint8_t charsize)
: LiquidCrystal_I2C(addr, cols, rows, charsize) {
  /* Constructor which first calls base LCD class constructor with passed
       arguments and initialises instance variables. A character buffer is
       dynamically allocated with the size matching the total number of
       characters present on the hardware LCD. The buffer is filled with
       whitespace characters, as the LCD will be initially empty. The size of
       the LCD is recorded and the cursor is set to the first character.
       Parameters:
         addr - A byte representing the I2C address of the LCD.
         cols - A byte representing the number of characters the LCD device has
           horizontally.
         rows - A byte representing the number of characters the LCD device has
           vertically.
         charsize - A byte representing the pixel dimensions of each character
            on the LCD. See symbolic constants defined in LCD library for
            accepted values.
  */

  buffer = new char[cols * rows];
  memset(buffer, ' ', cols*rows);
  maxX = cols;
  maxY = rows;
  cursor = 0;
}

BufferedLCD::~BufferedLCD() {
  /* Destructor which simply frees the character buffer allocated within the
       constructor.
  */

  delete[] buffer;
}

void BufferedLCD::clear() {
  /* clear - Method which first calls the base method to trigger the LCD clear
       on the hardware, then fills the buffer with whitespace to match the now
       empty LCD.
       Parameters: N/A
       Returns: N/A
  */

  LiquidCrystal_I2C::clear();
  memset(buffer, ' ', maxX*maxY);
}

void BufferedLCD::setCursor(uint8_t x, uint8_t y) {
  /* setCursor - Method which sets the position on the LCD where characters are
       to be printed. Checks that the passed coordinates are within range of
       the LCD size specified in the constructor, then calls the base setCursor
       method to set the cursor on the hardware and updates the cursor position
       instance variable to the corresponding buffer index.
       Parameters:
         x - A byte representing the column number of the character to position
           the cursor at. Should be in range 0 -> maxX - 1.
         y - A byte representing the row number of the character to position
           the cursor at. Should be in range 0 -> maxY - 1.
       Returns: N/A
  */

  if (x < maxX && y < maxY) {
    LiquidCrystal_I2C::setCursor(x, y);
    cursor = (y * maxX) + x;
  }
}

void BufferedLCD::print(const __FlashStringHelper *string) {
  /* print - Method which prints the passed string on the LCD at the current
       cursor position if it differs from the existing contents. First checks
       that there is sufficient space for the string at the current position
       and that the string differs to the current contents of the buffer at
       that position. The base print method is then called to send the string
       to the hardware and the buffer is updated to reflect the new LCD
       contents. If the passed string is identical to the buffer contents at
       the current position, or if it will exceed the size of the LCD if
       printed at that position, the hardware print will not be invoked and the
       buffer will remain unchanged. As the string exists in program memory,
       the appropriate AVR _P functions are used and the pointer cast to the
       required type for use with these functions.
       Parameters:
         string - A pointer to the character string stored in program memory to
           be printed on the LCD at the current cursor position.
       Returns: N/A
  */

  const char *flashString = reinterpret_cast<const char *>(string);
  size_t length = strlen_P(flashString);
  if (cursor + length <= maxX * maxY && memcmp_P(buffer + cursor, flashString, length)) {
    LiquidCrystal_I2C::print(string);
    memcpy_P(buffer + cursor, flashString, length);
  }
}

void BufferedLCD::print(const char *string) {
  /* print - Method which prints the passed string on the LCD at the current
       cursor position if it differs from the existing contents. First checks
       that there is sufficient space for the string at the current position
       and that the string differs to the current contents of the buffer at
       that position. The base print method is then called to send the string
       to the hardware and the buffer is updated to reflect the new LCD
       contents. If the passed string is identical to the buffer contents at
       the current position, or if it will exceed the size of the LCD if
       printed at that position, the hardware print will not be invoked and the
       buffer will remain unchanged.
       Parameters:
         string - A pointer to the character string to be printed on the LCD at
           the current cursor position.
       Returns: N/A
  */

  size_t length = strlen(string);
  if (cursor + length <= maxX * maxY && memcmp(buffer + cursor, string, length)) {
    LiquidCrystal_I2C::print(string);
    memcpy(buffer + cursor, string, length);
  }
}
