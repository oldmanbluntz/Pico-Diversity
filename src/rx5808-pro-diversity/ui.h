#ifndef UI_H
#define UI_H


//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <TFT_eSPI.h>
#include <stdint.h>

#include "settings.h"
#include "settings_internal.h"


#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 80

#define SCREEN_WIDTH_MID ((SCREEN_WIDTH / 2) - 1)
#define SCREEN_HEIGHT_MID ((SCREEN_HEIGHT / 2) - 1)

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7


namespace Ui {
    // OLED_CLASS is defined in settings.h (usually Adafruit_SSD1306)
    //extern OLED_CLASS display;
    extern TFT_eSPI display;
    
    extern bool shouldDrawUpdate;
    extern bool shouldDisplay;
    extern bool shouldFullRedraw;

    void setup();
    void update();

    void drawGraph(
        const uint8_t data[],
        const uint8_t dataSize,
        const uint8_t dataScale,
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h
    );

    void drawDiversityGraph(
        const uint8_t dataA[],
        const uint8_t dataB[],
        const uint8_t dataSize,
        const uint8_t dataScale,
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h
    );

    void drawRssiBars(
        const uint8_t rssiA,
        const uint8_t rssiB,
        const uint8_t rssiMin,
        const uint8_t rssiMax,
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h,
        const uint16_t colorA,
        const uint16_t colorB,
        bool forceRedraw = false
    );

    void drawStatusBar();

    void drawDashedHLine(const int x, const int y, const int w, const int step);
    void drawDashedVLine(const int x, const int y, const int w, const int step);

    void clear();
    void clearRect(const int x, const int y, const int w, const int h);

    void needUpdate();
    void needDisplay();
    void needFullRedraw();
}

#endif