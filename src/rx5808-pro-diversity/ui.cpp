#include <stdint.h>
#include <TFT_eSPI.h>

#include "settings.h"
#include "settings_internal.h"
#include "ui.h"

namespace Ui {
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSprite display = TFT_eSprite(&tft); // Framebuffer for flicker-free drawing
    
    bool shouldDrawUpdate = false;
    bool shouldDisplay = false;
    bool shouldFullRedraw = false;

    void setup() {
        tft.init();
        tft.setRotation(1); // 1 = Landscape
        tft.fillScreen(TFT_BLACK);

        if(!display.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT)) {
            // Memory allocation failed (shouldn't happen on Pico)
            while(true) {
                digitalWrite(PIN_LED, HIGH); delay(100);
                digitalWrite(PIN_LED, LOW); delay(100);
            }
        }

        display.setTextSize(1);
        display.setTextColor(TFT_WHITE, TFT_BLACK);
        display.fillSprite(TFT_BLACK);
        display.pushSprite(0, 0); 
    }

    void update() {
        if (shouldDisplay) {
            display.pushSprite(0, 0); // Push the buffered sprite to the screen
            shouldDisplay = false;
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

    void drawDashedHLine(const int x, const int y, const int w, const int step) {
        for (int i = 0; i <= w; i += step) {
            Ui::display.drawFastHLine(x + i, y, step / 2, TFT_WHITE);
        }
    }

    void drawDashedVLine(const int x, const int y, const int h, const int step) {
        for (int i = 0; i <= h; i += step) {
            // Using a dark grey instead of INVERSE to simulate the dashed look clearly
            Ui::display.drawFastVLine(x, y + i, step / 2, TFT_DARKGREY);
        }
    }

    void clear() {
        display.fillSprite(TFT_BLACK);
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