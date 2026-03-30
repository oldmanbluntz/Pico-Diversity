#include <Arduino.h>
#include "state_settings.h"
#include "state.h"
#include "buttons.h"
#include "ui.h"

#define NUM_SETTINGS_ITEMS 4
static const char* menuItems[NUM_SETTINGS_ITEMS] = {
    "Customization",
    "Models",
    "RSSI Calibration",
    "Back"
};

static GFXcanvas16* settingsCanvas = nullptr;

void StateMachine::SettingsStateHandler::onEnter() {
    selectedItem = 0;
    if (!settingsCanvas) {
        settingsCanvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void StateMachine::SettingsStateHandler::onExit() {
    if (settingsCanvas) {
        delete settingsCanvas;
        settingsCanvas = nullptr;
    }
}

void StateMachine::SettingsStateHandler::onUpdate() {}

void StateMachine::SettingsStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    if (pressType != Buttons::PressType::SHORT) return;

    switch (button) {
        case Button::UP:
            selectedItem--;
            if (selectedItem < 0) selectedItem = NUM_SETTINGS_ITEMS - 1;
            Ui::needUpdate();
            break;
        case Button::DOWN:
            selectedItem++;
            if (selectedItem >= NUM_SETTINGS_ITEMS) selectedItem = 0;
            Ui::needUpdate();
            break;
        case Button::MODE:
            if (selectedItem == 0) {
                StateMachine::switchState(StateMachine::State::SETTINGS_CUSTOM);
            } else if (selectedItem == 1) {
                StateMachine::switchState(StateMachine::State::SETTINGS_MODELS); // NEW ROUTE
            } else if (selectedItem == 2) {
                StateMachine::switchState(StateMachine::State::SETTINGS_RSSI);
            } else if (selectedItem == 3) {
                StateMachine::switchState(StateMachine::State::SEARCH); 
            }
            break;
    }
}

void StateMachine::SettingsStateHandler::onInitialDraw() {
    Ui::needUpdate(); 
}

void StateMachine::SettingsStateHandler::onUpdateDraw() {
    if (!settingsCanvas) return;

    settingsCanvas->fillScreen(TFT_BLACK);
    
    settingsCanvas->setTextSize(2);
    settingsCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
    settingsCanvas->setCursor(4, 4);
    settingsCanvas->print("Settings");
    settingsCanvas->drawFastHLine(0, 24, SCREEN_WIDTH, TFT_LIGHTGREY);

    settingsCanvas->setTextSize(2);
    for (int i = 0; i < NUM_SETTINGS_ITEMS; i++) {
        int y = 32 + (i * 22); 
        
        if (i == selectedItem) {
            settingsCanvas->fillRect(0, y - 2, SCREEN_WIDTH, 20, TFT_WHITE);
            settingsCanvas->setTextColor(TFT_BLACK, TFT_WHITE);
        } else {
            settingsCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        
        settingsCanvas->setCursor(8, y);
        settingsCanvas->print(menuItems[i]);
    }

    Ui::display.drawRGBBitmap(0, 0, settingsCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    Ui::needDisplay();
}