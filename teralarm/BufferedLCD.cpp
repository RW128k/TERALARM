#include "BufferedLCD.h"
//#include <cstdio>

BufferedLCD::BufferedLCD(uint8_t addr, uint8_t cols, uint8_t rows, uint8_t charsize)
: LiquidCrystal_I2C(addr, cols, rows, charsize) {
    buffer = new char[cols * rows];
    memset(buffer, ' ', cols*rows);
    maxX = cols;
    maxY = rows;
    cursor = 0;
}

BufferedLCD::~BufferedLCD() {
    delete[] buffer;
}

void BufferedLCD::clear() {
    LiquidCrystal_I2C::clear();
    memset(buffer, ' ', maxX*maxY);
    //printf("CLEARED!\n");
}

void BufferedLCD::setCursor(uint8_t x, uint8_t y) {
    if (x < maxX && y < maxY) {
        LiquidCrystal_I2C::setCursor(x, y);
        cursor = (y * maxX) + x;
    }
}

void BufferedLCD::print(const __FlashStringHelper *string) {
    const char *flashString = reinterpret_cast<const char *>(string);
    size_t length = strlen_P(flashString);
    if (cursor + length <= maxX * maxY && memcmp_P(buffer + cursor, flashString, length)) {
        LiquidCrystal_I2C::print(string);
        memcpy_P(buffer + cursor, flashString, length);
    }
}

void BufferedLCD::print(const char *string) {
    size_t length = strlen(string);
    if (cursor + length <= maxX * maxY && memcmp(buffer + cursor, string, length)) {
        LiquidCrystal_I2C::print(string);
        memcpy(buffer + cursor, string, length);
    }
    //else printf("IDENTICAL: [%s]\n", string);

    /*
    //debug
    char tmp[21];
    tmp[20] = 0;

    printf("#====================#\n");
    memcpy(tmp, buffer + 0, 20);
    printf("|%s|\n", tmp);
    memcpy(tmp, buffer + 20, 20);
    printf("|%s|\n", tmp);
    memcpy(tmp, buffer + 40, 20);
    printf("|%s|\n", tmp);
    memcpy(tmp, buffer + 60, 20);
    printf("|%s|\n", tmp);
    printf("#====================#\n\n");
    */
}
