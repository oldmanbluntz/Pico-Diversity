#include <Arduino.h>
#include <string.h>
#include "state_settings_custom.h"
#include "state.h"
#include "buttons.h"
#include "ui.h"
#include "settings_eeprom.h"

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

void StateMachine::SettingsCustomStateHandler::onUpdate() {}

void StateMachine::SettingsCustomStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    if (pressType != Buttons::PressType::SHORT) return;

    switch (button) {
        case Button::UP:
            selectedItem--;
            if (selectedItem < 0) selectedItem = 3;
            Ui::needUpdate();
            break;
        case Button::DOWN:
            selectedItem++;
            if (selectedItem > 3) selectedItem = 0;
            Ui::needUpdate();
            break;
        case Button::MODE:
            if (selectedItem == 0) {
                // Layout Toggle (Bars vs Graphs)
                EepromSettings.uiLayout = (EepromSettings.uiLayout == 0) ? 1 : 0;
                EepromSettings.save();
            } else if (selectedItem == 1) {
                // Scheme Toggle (Easter vs Night)
                EepromSettings.uiScheme = (EepromSettings.uiScheme == 0) ? 1 : 0;
                EepromSettings.save();
            } else if (selectedItem == 2) {
                // Screensaver Toggle (Cube vs Tubes)
                EepromSettings.screensaverStyle = (EepromSettings.screensaverStyle == 0) ? 1 : 0;
                EepromSettings.save();
            } else if (selectedItem == 3) {
                StateMachine::switchState(StateMachine::State::SETTINGS);
            }
            Ui::needUpdate();
            break;
    }
}

void StateMachine::SettingsCustomStateHandler::onInitialDraw() {
    Ui::needUpdate(); // <--- FIXED: Now correctly triggers the draw cycle!
}

void StateMachine::SettingsCustomStateHandler::onUpdateDraw() {
    if (!customCanvas) return;

    customCanvas->fillScreen(TFT_BLACK);
    
    customCanvas->setTextSize(2);
    customCanvas->setTextColor(getSchemeColorMenu(), TFT_BLACK);
    customCanvas->setCursor(4, 4);
    customCanvas->print("Customization");
    customCanvas->drawFastHLine(0, 24, SCREEN_WIDTH, getSchemeColorMenu());

    char items[4][32];
    sprintf(items[0], "Layout: %s", EepromSettings.uiLayout == 0 ? "Bars" : "Graphs");
    sprintf(items[1], "Scheme: %s", EepromSettings.uiScheme == 0 ? "Easter" : "Night");
    sprintf(items[2], "Saver:  %s", EepromSettings.screensaverStyle == 0 ? "Cube" : "Tubes");
    strcpy(items[3], "Back");

    for (int i = 0; i < 4; i++) {
        int y = 32 + (i * 22); 
        
        if (i == selectedItem) {
            customCanvas->fillRect(0, y - 2, SCREEN_WIDTH, 20, getSchemeColorMenu());
            customCanvas->setTextColor(TFT_BLACK, getSchemeColorMenu());
        } else {
            customCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        
        customCanvas->setCursor(8, y);
        customCanvas->print(items[i]);
    }

    Ui::display.drawRGBBitmap(0, 0, customCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    Ui::needDisplay();
}