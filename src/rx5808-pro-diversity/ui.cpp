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

        // Draw a small black background box to erase any old text
        display.fillRect(startX, startY, 28, 10, TFT_BLACK);
        
        display.setTextSize(1);
        display.setTextColor(TFT_WHITE, TFT_BLACK);
        display.setCursor(startX, startY);

        if (Receiver::activeReceiver == Receiver::ReceiverId::A) {
            display.print("RX:A");
        } else {
            display.print("RX:B");
        }
    }

    void drawGraph(
        const uint8_t data[],
        const uint8_t dataSize,
        const uint8_t dataScale,
        const uint8_t x,
        const uint8_t y,
        const uint8_t w,
        const uint8_t h
    ) {
        #define SCALE_DATAPOINT(p) (p * h / dataScale)
        #define CLAMP_DATAPOINT(p) \
            (p > dataScale) ? dataScale : ((p < 0) ? 0 : p);

        Ui::clearRect(x, y, w - 1, h + 1);

        const uint8_t xScaler = w / (dataSize - 1);
        const uint8_t xScalarMissing = w - (xScaler * (dataSize - 1));

        uint8_t xNext = x;

        for (uint8_t i = 0; i < dataSize - 1; i++) {
            const uint8_t dataPoint = CLAMP_DATAPOINT(data[i]);
            const uint8_t dataPointNext = CLAMP_DATAPOINT(data[i + 1]);

            const uint8_t dataPointHeight = h - SCALE_DATAPOINT(dataPoint);
            const uint8_t dataPointNextHeight =
                h - SCALE_DATAPOINT(dataPointNext);

            const uint8_t xEnd = xNext + xScaler
                    + (i == 0 || i == dataSize - 2 ? (xScalarMissing + 1) / 2 : 0);

            Ui::display.drawLine(
                xNext,
                y + dataPointHeight,
                xEnd,
                y + dataPointNextHeight,
                TFT_WHITE
            );

            xNext = xEnd;
        }

        #undef SCALE_DATAPOINT
        #undef CLAMP_DATAPOINT
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