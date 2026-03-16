#include <stdint.h>
#include <TFT_eSPI.h>

#include "settings.h"
#include "settings_internal.h"
#include "ui.h"
#include "receiver.h"

namespace Ui {
    // Initialize the TFT_eSPI object
    TFT_eSPI display = TFT_eSPI();
    
    bool shouldDrawUpdate = false;
    bool shouldDisplay = false;
    bool shouldFullRedraw = false;

    void setup() {
        display.init();
        display.setRotation(3); // Set to 1 or 3 for landscape depending on your board mount

        // TFT_eSPI text background rendering
        display.setTextColor(TFT_WHITE, TFT_BLACK); 
        display.setTextSize(1);
        display.setTextWrap(false);

        display.fillScreen(TFT_BLACK); 
    }

    void update() {
        if (shouldDisplay) {
            // TFT_eSPI draws directly to the screen by default.
            // No display.display() buffer push is required here.
            shouldDisplay = false;
        }
    }

    void drawStatusBar() {
        // Position it in the top right corner
        const int startX = SCREEN_WIDTH - 30;
        const int startY = 2;
        
        display.setTextSize(1);

        // The second parameter (TFT_BLACK) acts as a flicker-free eraser!
        // It overwrites the graph lines behind the text cleanly in one pass.
        display.setTextColor(TFT_WHITE, TFT_BLACK); 
        display.setCursor(startX, startY);

        if (Receiver::activeReceiver == Receiver::ReceiverId::A) {
            display.print("RX:A");
        } else {
            display.print("RX:B");
        }
    }

    static uint16_t last_yA[160] = {0};
    static uint16_t last_yB[160] = {0};
    static uint16_t last_ySingle[160] = {0};
    static bool firstDiversityDraw = true;
    static bool firstSingleDraw = true;

    void drawDiversityGraph(
        const uint8_t dataA[],
        const uint8_t dataB[],
        const uint8_t dataSize,
        const uint8_t dataScale,
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h
    ) {
        // --- THE SNAPSHOT FIX ---
        // Freeze the data so asynchronous receiver updates can't tear the line mid-draw
        uint8_t snapA[160]; 
        uint8_t snapB[160];
        for (uint16_t i = 0; i < dataSize; i++) {
            snapA[i] = dataA[i];
            snapB[i] = dataB[i];
        }

        if (firstDiversityDraw) {
            for(int i = 0; i < 160; i++) {
                last_yA[i] = y + h - 2;
                last_yB[i] = y + h - 2;
            }
            display.fillRect(x, y, w, h, TFT_BLACK); 
            firstDiversityDraw = false;
        }

        // 1. ERASE the old lines (Now mirroring the pixel-by-pixel bypass to prevent dotted residues)
        for (uint16_t i = 0; i < dataSize - 1; i++) {
            uint16_t x1 = map(i, 0, dataSize - 2, x, x + w - 1);
            uint16_t x2 = map(i + 1, 0, dataSize - 2, x, x + w - 1);
            
            // Erase RX A
            if (last_yA[i] == last_yA[i + 1]) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, last_yA[i], TFT_BLACK);
            } else {
                display.drawLine(x1, last_yA[i], x2, last_yA[i + 1], TFT_BLACK);
            }

            // Erase RX B
            if (last_yB[i] == last_yB[i + 1]) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, last_yB[i], TFT_BLACK);
            } else {
                display.drawLine(x1, last_yB[i], x2, last_yB[i + 1], TFT_BLACK);
            }
        }

        // 2. DRAW the new lines using the FROZEN snapshot data
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

            // --- THE FLAT LINE BYPASS ---
            // Draw RX A (Red)
            if (yA1 == yA2) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, yA1, TFT_RED);
            } else {
                display.drawLine(x1, yA1, x2, yA2, TFT_RED);
            }

            // Draw RX B (Cyan)
            if (yB1 == yB2) {
                for (int px = x1; px <= x2; px++) display.drawPixel(px, yB1, TFT_CYAN);
            } else {
                display.drawLine(x1, yB1, x2, yB2, TFT_CYAN);
            }

            // Memorize coordinates for the next erasure
            last_yA[i] = yA1;
            last_yB[i] = yB1;
            if (i == dataSize - 2) {
                last_yA[i + 1] = yA2;
                last_yB[i + 1] = yB2;
            }
        }
    }
    void drawGraph(
        const uint8_t data[],
        const uint8_t dataSize,
        const uint8_t dataScale,
        const uint16_t x,
        const uint16_t y,
        const uint16_t w,
        const uint16_t h
    ) {
        if (firstSingleDraw) {
            for(int i = 0; i < 160; i++) {
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

    void drawRssiBars(
        const uint8_t rssiA, const uint8_t rssiB, const uint8_t rssiMin, const uint8_t rssiMax,
        const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
        const uint16_t colorA, const uint16_t colorB, bool forceRedraw
    ) {
        if (forceRedraw) {
            last_barA_w = 0;
            last_barB_w = 0;
            display.fillRect(x, y, w, h, TFT_BLACK); // Changed to canvas
        }

        uint16_t barW_A = map(constrain(rssiA, rssiMin, rssiMax), rssiMin, rssiMax, 0, w);
        uint16_t barW_B = map(constrain(rssiB, rssiMin, rssiMax), rssiMin, rssiMax, 0, w);
        uint16_t halfH = h / 2;

        if (barW_A > last_barA_w) {
            display.fillRect(x + last_barA_w, y, barW_A - last_barA_w, halfH - 2, colorA); // Changed to canvas
        } else if (barW_A < last_barA_w) {
            display.fillRect(x + barW_A, y, last_barA_w - barW_A, halfH - 2, TFT_BLACK); // Changed to canvas
        }

        #ifdef USE_DIVERSITY
        if (barW_B > last_barB_w) {
            display.fillRect(x + last_barB_w, y + halfH, barW_B - last_barB_w, halfH - 2, colorB); // Changed to canvas
        } else if (barW_B < last_barB_w) {
            display.fillRect(x + barW_B, y + halfH, last_barB_w - barW_B, halfH - 2, TFT_BLACK); // Changed to canvas
        }
        #endif

        last_barA_w = barW_A;
        last_barB_w = barW_B;
    }

    void drawDashedHLine(
        const int x,
        const int y,
        const int w,
        const int step
    ) {
        for (int i = 0; i <= w; i += step) {
            Ui::display.drawFastHLine(x + i, y, step / 2, TFT_WHITE);
        }
    }

    void drawDashedVLine(
        const int x,
        const int y,
        const int h,
        const int step
    ) {
        for (int i = 0; i <= h; i += step) {
            // Replaced INVERSE with TFT_LIGHTGREY
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