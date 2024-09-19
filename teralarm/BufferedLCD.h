/* TERALARM (FIRMWARE 3) - The effective alarm clock

   BufferedLCD.h - The header file containing overrides for LCD class methods
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

#ifndef BUFFEREDLCD_H
#define BUFFEREDLCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class BufferedLCD : private LiquidCrystal_I2C {
public:
    BufferedLCD(uint8_t addr, uint8_t cols, uint8_t rows, uint8_t charsize = 0);
    ~BufferedLCD();
    using LiquidCrystal_I2C::begin;
    void clear();
    using LiquidCrystal_I2C::createChar;
    void setCursor(uint8_t x, uint8_t y);
    void print(const __FlashStringHelper *string);
    void print(const char *string);

private:
    char *buffer;
    uint8_t maxX;
    uint8_t maxY;
    size_t cursor;
};

#endif
