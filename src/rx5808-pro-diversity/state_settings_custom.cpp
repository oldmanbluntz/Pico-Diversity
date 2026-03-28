#include <Arduino.h>
#include "state_settings_custom.h"
#include "state.h"
#include "buttons.h"
#include "ui.h"
#include "settings_eeprom.h"

#define NUM_CUSTOM_ITEMS 4
static const char* customItems[NUM_CUSTOM_ITEMS] = {
    "Layout",
    "Scheme",
    "Screensaver",
    "Back"
};

static GFXcanvas16* customCanvas = nullptr;

void StateMachine::SettingsCustomStateHandler::onEnter() {
    selectedItem = 0;
    if (!customCanvas) {
        customCanvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void StateMachine::SettingsCustomStateHandler::onExit() {
    if (customCanvas) {
        delete customCanvas;
        customCanvas = nullptr;
    }
}

void StateMachine::SettingsCustomStateHandler::onUpdate() {
}

void StateMachine::SettingsCustomStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    if (pressType != Buttons::PressType::SHORT) return;

    switch (button) {
        case Button::UP:
            selectedItem--;
            if (selectedItem < 0) selectedItem = NUM_CUSTOM_ITEMS - 1;
            Ui::needUpdate();
            break;
        case Button::DOWN:
            selectedItem++;
            if (selectedItem >= NUM_CUSTOM_ITEMS) selectedItem = 0;
            Ui::needUpdate();
            break;
        case Button::MODE:
            if (selectedItem == 0) {
                // Future Layout toggle
            } else if (selectedItem == 1) {
                // Future Scheme toggle
            } else if (selectedItem == 2) {
                // Toggle between 0 (Cube) and 1 (Bars)
                EepromSettings.screensaverStyle = (EepromSettings.screensaverStyle == 0) ? 1 : 0;
                EepromSettings.markDirty();
                Ui::needUpdate();
            } else if (selectedItem == 3) { 
                StateMachine::switchState(StateMachine::State::SETTINGS);
            }
            break;
    }
}

void StateMachine::SettingsCustomStateHandler::onInitialDraw() {
    Ui::needUpdate();
}

void StateMachine::SettingsCustomStateHandler::onUpdateDraw() {
    if (!customCanvas) return;

    customCanvas->fillScreen(TFT_BLACK);
    
    // Draw Header
    customCanvas->setTextSize(2);
    customCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
    customCanvas->setCursor(4, 4);
    customCanvas->print("Customization");
    customCanvas->drawFastHLine(0, 24, SCREEN_WIDTH, TFT_LIGHTGREY);

    // Draw Menu Items
    customCanvas->setTextSize(2);
    for (int i = 0; i < NUM_CUSTOM_ITEMS; i++) {
        int y = 32 + (i * 22); 
        
        if (i == selectedItem) {
            customCanvas->fillRect(0, y - 2, SCREEN_WIDTH, 20, TFT_WHITE);
            customCanvas->setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            customCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        
        customCanvas->setCursor(8, y);
        
        // Dynamically insert the current selection into the text
        if (i == 2) {
            if (EepromSettings.screensaverStyle == 0) {
                customCanvas->print("Saver: Cube");
            } else {
                customCanvas->print("Saver: Bars");
            }
        } else {
            customCanvas->print(customItems[i]);
        }
    }

    Ui::display.drawRGBBitmap(0, 0, customCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    Ui::needDisplay();
}