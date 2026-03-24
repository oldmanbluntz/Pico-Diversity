#include <stdint.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #include <avr/pgmspace.h> // PICO FIX: Removed AVR specific header

#include "settings.h"
#include "settings_internal.h"
#include "ui.h"


namespace Ui {
    OLED_CLASS display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    bool shouldDrawUpdate = false;
    bool shouldDisplay = false;
    bool shouldFullRedraw = false;


    void setup() {
        // PICO NOTE: Wire.begin() and pin remapping moved to main.cpp setup() 
        // to prevent hardware conflicts with buttons on GP4/GP5.
        
        if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
            // If we get trapped here, the screen is still failing to init
            while(true) {
                digitalWrite(PIN_LED, HIGH); delay(100);
                digitalWrite(PIN_LED, LOW); delay(100);
            }
        }
        //Wire.setClock(100000);

        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setTextWrap(false);

        display.clearDisplay();
        
        // --- FORCE PRINT TEST ---
        display.setTextSize(2);      // Make the text big
        display.setCursor(10, 20);   // Move cursor to the middle
        display.println("WORKING");  // Draw the text
        display.display();           // Push it to the physical screen
        
        delay(5000);

        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setTextWrap(false);

        display.clearDisplay();
        display.display(); // Force an initial clear
    }

    void update() {
        if (shouldDisplay) {
            display.display();
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

            // Need to invert the heights so it shows the right way on the
            // screen.
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
                WHITE
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
            Ui::display.drawFastHLine(x + i, y, step / 2, WHITE);
        }
    }

    void drawDashedVLine(
        const int x,
        const int y,
        const int h,
        const int step
    ) {
        for (int i = 0; i <= h; i += step) {
            Ui::display.drawFastVLine(x, y + i, step / 2, INVERSE);
        }
    }

    void clear() {
        display.clearDisplay();
    }

    void clearRect(const int x, const int y, const int w, const int h) {
        display.fillRect(x, y, w, h, BLACK);
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