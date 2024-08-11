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
