#include <stdint.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "settings.h"
#include "settings_internal.h"
#include "ui.h"
#include "receiver.h"

// Define your SPI1 pins
#define TFT_MOSI 11
#define TFT_SCLK 10
#define TFT_CS   13
#define TFT_DC   12
#define TFT_RST  9

namespace Ui {
    // Instantiate Adafruit display using Hardware SPI1
    Adafruit_ST7789 display = Adafruit_ST7789(&SPI1, TFT_CS, TFT_DC, TFT_RST);
    
    bool shouldDrawUpdate = false;
    bool shouldDisplay = false;
    bool shouldFullRedraw = false;

    void setup() {
        // FORCE the RP2040 to route the SPI1 bus to your specific pins
        SPI1.setSCK(TFT_SCLK);
        SPI1.setTX(TFT_MOSI);
        SPI1.begin();

        // Initialize the screen using Adafruit's specific 135x240 command
        display.init(135, 240);
        display.setRotation(1); 

        // --- DIAGNOSTIC TEST ---
        display.fillScreen(TFT_RED); 
        delay(3000); 
        display.fillScreen(TFT_GREEN);
        delay(3000);
        // -----------------------

        display.setTextColor(TFT_WHITE, TFT_BLACK); 
        display.setTextSize(1);
        display.setTextWrap(false);
        display.fillScreen(TFT_BLACK); 
    }
    
    void update() {
        if (shouldDisplay) {
            shouldDisplay = false;
        }
    }

    void drawStatusBar() {
        const int startX = SCREEN_WIDTH - 30;
        const int startY = 2;
        
        display.setTextSize(1);
        display.setTextColor(TFT_WHITE, TFT_BLACK); 
        display.setCursor(startX, startY);

        if (Receiver::activeReceiver == Receiver::ReceiverId::A) {
            display.print("RX:A");
        } else {
            display.print("RX:B");
        }
    }

    static uint16_t last_yA[SCREEN_WIDTH] = {0};
    static uint16_t last_yB[SCREEN_WIDTH] = {0};
    static uint16_t last_ySingle[SCREEN_WIDTH] = {0};
    static bool firstDiversityDraw = true;
    static bool firstSingleDraw = true;

    void drawDiversityGraph(const uint8_t dataA[], const uint8_t dataB[], const uint8_t dataSize, const uint8_t dataScale, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) {
        uint8_t snapA[SCREEN_WIDTH]; 
        uint8_t snapB[SCREEN_WIDTH];
        for (uint16_t i = 0; i < dataSize; i++) {
            snapA[i] = dataA[i];
            snapB[i] = dataB[i];
        }

        if (firstDiversityDraw) {
            for(int i = 0; i < SCREEN_WIDTH; i++) {
                last_yA[i] = y + h - 2;
                last_yB[i] = y + h - 2;
            }
            display.fillRect(x, y, w, h, TFT_BLACK); 
            firstDiversityDraw = false;
        }

        for (uint16_t i = 0; i < dataSize - 1; i++) {
            uint16_t x1 = map(i, 0, dataSize - 2, x, x + w - 1);
            uint16_t x2 = map(i + 1, 0, dataSize - 2, x, x + w - 1);
            
            if (last_yA[i] == last_yA[i + 1]) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, last_yA[i], TFT_BLACK);
            } else {
                display.drawLine(x1, last_yA[i], x2, last_yA[i + 1], TFT_BLACK);
            }

            if (last_yB[i] == last_yB[i + 1]) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, last_yB[i], TFT_BLACK);
            } else {
                display.drawLine(x1, last_yB[i], x2, last_yB[i + 1], TFT_BLACK);
            }
        }

        for (uint16_t i = 0; i < dataSize - 1; i++) {
            uint16_t pA1 = constrain(snapA[i], 0, dataScale);
            uint16_t pA2 = constrain(snapA[i + 1], 0, dataScale);
            uint16_t pB1 = constrain(snapB[i], 0, dataScale);
            uint16_t pB2 = constrain(snapB[i + 1], 0, dataScale);

            uint16_t yA1 = map(pA1, 0, dataScale, y + h - 2, y + 2);
            uint16_t yA2 = map(pA2, 0, dataScale, y + h - 2, y + 2);
            uint16_t yB1 = map(pB1, 0, dataScale, y + h - 2, y + 2);
            uint16_t yB2 = map(pB2, 0, dataScale, y + h - 2, y + 2);

            uint16_t x1 = map(i, 0, dataSize - 2, x, x + w - 1);
            uint16_t x2 = map(i + 1, 0, dataSize - 2, x, x + w - 1);

            if (yA1 == yA2) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, yA1, TFT_RED);
            } else {
                display.drawLine(x1, yA1, x2, yA2, TFT_RED);
            }

            if (yB1 == yB2) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, yB1, TFT_CYAN);
            } else {
                display.drawLine(x1, yB1, x2, yB2, TFT_CYAN);
            }

            last_yA[i] = yA1;
            last_yB[i] = yB1;
            if (i == dataSize - 2) {
                last_yA[i + 1] = yA2;
                last_yB[i + 1] = yB2;
            }
        }
    }

    void drawGraph(const uint8_t data[], const uint8_t dataSize, const uint8_t dataScale, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) {
        if (firstSingleDraw) {
            for(int i = 0; i < SCREEN_WIDTH; i++) {
                last_ySingle[i] = y + h - 2;
            }
            display.fillRect(x, y, w, h, TFT_BLACK);
            firstSingleDraw = false;
        }

        for (uint16_t i = 0; i < dataSize - 1; i++) {
            uint16_t x1 = map(i, 0, dataSize - 2, x, x + w - 1);
            uint16_t x2 = map(i + 1, 0, dataSize - 2, x, x + w - 1);
            display.drawLine(x1, last_ySingle[i], x2, last_ySingle[i + 1], TFT_BLACK);
        }

        for (uint16_t i = 0; i < dataSize - 1; i++) {
            uint16_t p1 = constrain(data[i], 0, dataScale);
            uint16_t p2 = constrain(data[i + 1], 0, dataScale);

            uint16_t y1 = map(p1, 0, dataScale, y + h - 2, y + 2);
            uint16_t y2 = map(p2, 0, dataScale, y + h - 2, y + 2);

            uint16_t x1 = map(i, 0, dataSize - 2, x, x + w - 1);
            uint16_t x2 = map(i + 1, 0, dataSize - 2, x, x + w - 1);

            display.drawLine(x1, y1, x2, y2, TFT_WHITE);

            last_ySingle[i] = y1;
            if (i == dataSize - 2) {
                last_ySingle[i + 1] = y2;
            }
        }
    }

    static uint16_t last_barA_w = 0;
    static uint16_t last_barB_w = 0;

    void drawRssiBars(const uint8_t rssiA, const uint8_t rssiB, const uint8_t rssiMin, const uint8_t rssiMax, const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint16_t colorA, const uint16_t colorB, bool forceRedraw) {
        if (forceRedraw) {
            last_barA_w = 0;
            last_barB_w = 0;
            display.fillRect(x, y, w, h, TFT_BLACK); 
        }

        uint16_t barW_A = map(constrain(rssiA, rssiMin, rssiMax), rssiMin, rssiMax, 0, w);
        uint16_t barW_B = map(constrain(rssiB, rssiMin, rssiMax), rssiMin, rssiMax, 0, w);
        uint16_t halfH = h / 2;

        if (barW_A > last_barA_w) {
            display.fillRect(x + last_barA_w, y, barW_A - last_barA_w, halfH - 2, colorA); 
        } else if (barW_A < last_barA_w) {
            display.fillRect(x + barW_A, y, last_barA_w - barW_A, halfH - 2, TFT_BLACK); 
        }

        #ifdef USE_DIVERSITY
        if (barW_B > last_barB_w) {
            display.fillRect(x + last_barB_w, y + halfH, barW_B - last_barB_w, halfH - 2, colorB); 
        } else if (barW_B < last_barB_w) {
            display.fillRect(x + barW_B, y + halfH, last_barB_w - barW_B, halfH - 2, TFT_BLACK); 
        }
        #endif

        last_barA_w = barW_A;
        last_barB_w = barW_B;
    }

    void drawDashedHLine(const int x, const int y, const int w, const int step) {
        for (int i = 0; i <= w; i += step) {
            Ui::display.drawFastHLine(x + i, y, step / 2, TFT_LIGHTGREY);
        }
    }

    void drawDashedVLine(const int x, const int y, const int h, const int step) {
        for (int i = 0; i <= h; i += step) {
            Ui::display.drawFastVLine(x, y + i, step / 2, TFT_LIGHTGREY); 
        }
    }

    void clear() {
        display.fillScreen(TFT_BLACK);
    }

    void clearRect(const int x, const int y, const int w, const int h) {
        display.fillRect(x, y, w, h, TFT_BLACK);
    }

    void needUpdate() {
        shouldDrawUpdate = true;
    }

    void needDisplay() {
        shouldDisplay = true;
    }

    void needFullRedraw() {
        shouldFullRedraw = true;
    }
}