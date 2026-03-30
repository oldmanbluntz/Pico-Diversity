#include <Arduino.h>
#include <string.h>
#include "state_settings_models.h"
#include "state.h"
#include "buttons.h"
#include "ui.h"
#include "settings_eeprom.h"

static GFXcanvas16* modelsCanvas = nullptr;

void StateMachine::SettingsModelsStateHandler::cycleChar(char& c, int dir) {
    const char chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-";
    int len = strlen(chars);
    int pos = 0;
    for (int i = 0; i < len; i++) { if (chars[i] == c) { pos = i; break; } }
    pos += dir;
    if (pos < 0) pos = len - 1;
    if (pos >= len) pos = 0;
    c = chars[pos];
}

void StateMachine::SettingsModelsStateHandler::onEnter() {
    currentSubMenu = SubMenu::LIST;
    selectedListIdx = 0;
    if (!modelsCanvas) {
        modelsCanvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void StateMachine::SettingsModelsStateHandler::onExit() {
    if (modelsCanvas) {
        delete modelsCanvas;
        modelsCanvas = nullptr;
    }
}

void StateMachine::SettingsModelsStateHandler::onUpdate() {}

void StateMachine::SettingsModelsStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    if (currentSubMenu == SubMenu::LIST) {
        if (pressType == Buttons::PressType::SHORT) {
            if (button == Button::UP) {
                selectedListIdx--;
                if (selectedListIdx < 0) selectedListIdx = 8; 
                Ui::needUpdate();
            } else if (button == Button::DOWN) {
                selectedListIdx++;
                if (selectedListIdx > 8) selectedListIdx = 0;
                Ui::needUpdate();
            } else if (button == Button::MODE) {
                if (selectedListIdx == 8) { // "Back" option
                    StateMachine::switchState(StateMachine::State::SETTINGS);
                } else {
                    currentSubMenu = SubMenu::POPUP;
                    selectedPopupIdx = 0;
                    Ui::needUpdate();
                }
            }
        }
    } else if (currentSubMenu == SubMenu::POPUP) {
        if (pressType == Buttons::PressType::SHORT) {
            if (button == Button::UP) {
                selectedPopupIdx--;
                if (selectedPopupIdx < 0) selectedPopupIdx = 3;
                Ui::needUpdate();
            } else if (button == Button::DOWN) {
                selectedPopupIdx++;
                if (selectedPopupIdx > 3) selectedPopupIdx = 0;
                Ui::needUpdate();
            } else if (button == Button::MODE) {
                if (selectedPopupIdx == 0) { // Select
                    EepromSettings.activeModel = selectedListIdx;
                    
                    EepromSettings.rssiAMin = EepromSettings.models[selectedListIdx].rssiAMin;
                    EepromSettings.rssiAMax = EepromSettings.models[selectedListIdx].rssiAMax;
                    #ifdef USE_DIVERSITY
                        EepromSettings.rssiBMin = EepromSettings.models[selectedListIdx].rssiBMin;
                        EepromSettings.rssiBMax = EepromSettings.models[selectedListIdx].rssiBMax;
                    #endif
                    
                    EepromSettings.markDirty();
                    currentSubMenu = SubMenu::LIST;

                } else if (selectedPopupIdx == 1) { // Edit / Add
                    currentSubMenu = SubMenu::EDIT;
                    editCursorIdx = 0;
                    strncpy(editBuffer, EepromSettings.models[selectedListIdx].name, 8);
                    editBuffer[8] = '\0';
                    
                    for (int i = 0; i < 8; i++) {
                        if (editBuffer[i] == '\0') {
                            for (int j = i; j < 8; j++) editBuffer[j] = ' ';
                            break;
                        }
                    }

                } else if (selectedPopupIdx == 2) { // Delete
                    memset(EepromSettings.models[selectedListIdx].name, 0, 9);
                    EepromSettings.models[selectedListIdx].rssiAMin = RSSI_MIN_VAL;
                    EepromSettings.models[selectedListIdx].rssiAMax = RSSI_MAX_VAL;
                    #ifdef USE_DIVERSITY
                        EepromSettings.models[selectedListIdx].rssiBMin = RSSI_MIN_VAL;
                        EepromSettings.models[selectedListIdx].rssiBMax = RSSI_MAX_VAL;
                    #endif
                    
                    if (EepromSettings.activeModel == selectedListIdx) {
                        EepromSettings.rssiAMin = RSSI_MIN_VAL;
                        EepromSettings.rssiAMax = RSSI_MAX_VAL;
                        #ifdef USE_DIVERSITY
                            EepromSettings.rssiBMin = RSSI_MIN_VAL;
                            EepromSettings.rssiBMax = RSSI_MAX_VAL;
                        #endif
                    }
                    
                    EepromSettings.markDirty();
                    currentSubMenu = SubMenu::LIST;

                } else if (selectedPopupIdx == 3) { // Back
                    currentSubMenu = SubMenu::LIST;
                }
                Ui::needUpdate();
            }
        }
    } else if (currentSubMenu == SubMenu::EDIT) {
        if (pressType == Buttons::PressType::SHORT) {
            if (button == Button::UP) {
                if (editCursorIdx < 8) cycleChar(editBuffer[editCursorIdx], 1);
                Ui::needUpdate();
            } else if (button == Button::DOWN) {
                if (editCursorIdx < 8) cycleChar(editBuffer[editCursorIdx], -1);
                Ui::needUpdate();
            } else if (button == Button::MODE) {
                if (editCursorIdx == 8) {
                    // SAVE EXECUTED!
                    int lastChar = 7;
                    while (lastChar >= 0 && editBuffer[lastChar] == ' ') {
                        editBuffer[lastChar] = '\0';
                        lastChar--;
                    }
                    strncpy(EepromSettings.models[selectedListIdx].name, editBuffer, 8);
                    EepromSettings.models[selectedListIdx].name[8] = '\0';
                    EepromSettings.markDirty();
                    
                    currentSubMenu = SubMenu::LIST;
                } else {
                    editCursorIdx++;
                }
                Ui::needUpdate();
            }
        }
    }
}

void StateMachine::SettingsModelsStateHandler::onInitialDraw() {
    Ui::needUpdate();
}

void StateMachine::SettingsModelsStateHandler::onUpdateDraw() {
    if (!modelsCanvas) return;

    modelsCanvas->fillScreen(TFT_BLACK);
    
    modelsCanvas->setTextSize(2);
    modelsCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
    modelsCanvas->setCursor(4, 4);
    modelsCanvas->print("Models");
    modelsCanvas->drawFastHLine(0, 24, SCREEN_WIDTH, TFT_LIGHTGREY);

    if (currentSubMenu == SubMenu::LIST || currentSubMenu == SubMenu::EDIT) {
        int startIdx = selectedListIdx < 4 ? 0 : selectedListIdx - 3;
        if (startIdx > 4) startIdx = 4; 

        for (int i = 0; i < 5; i++) { 
            int idx = startIdx + i;
            if (idx > 8) break;

            int y = 32 + (i * 20);
            
            if (idx == selectedListIdx && currentSubMenu == SubMenu::LIST) {
                modelsCanvas->fillRect(0, y - 2, SCREEN_WIDTH, 18, TFT_WHITE);
                modelsCanvas->setTextColor(TFT_BLACK, TFT_WHITE);
            } else {
                modelsCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
            }

            modelsCanvas->setCursor(8, y);
            
            if (idx == 8) {
                modelsCanvas->print("  Back");
            } else {
                if (EepromSettings.activeModel == idx) modelsCanvas->print("* ");
                else modelsCanvas->print("  ");

                if (currentSubMenu == SubMenu::EDIT && idx == selectedListIdx) {
                    // Draw the 8 editable characters
                    for (int c = 0; c < 8; c++) {
                        if (c == editCursorIdx) {
                            modelsCanvas->setTextColor(TFT_BLACK, TFT_WHITE);
                            modelsCanvas->print(editBuffer[c]);
                            modelsCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
                        } else {
                            modelsCanvas->print(editBuffer[c]);
                        }
                    }
                    
                    // Draw the 9th slot for saving
                    if (editCursorIdx == 8) {
                        modelsCanvas->setTextColor(TFT_BLACK, TFT_WHITE);
                        modelsCanvas->print(" [SAVE]");
                        modelsCanvas->setTextColor(TFT_WHITE, TFT_BLACK);
                    } else {
                        modelsCanvas->print(" [SAVE]");
                    }
                } else {
                    char name[9];
                    strncpy(name, EepromSettings.models[idx].name, 8);
                    name[8] = '\0';
                    if (strlen(name) == 0) modelsCanvas->print("Empty");
                    else modelsCanvas->print(name);
                }
            }
        }
    } 
    else if (currentSubMenu == SubMenu::POPUP) {
        modelsCanvas->setTextColor(TFT_DARKGREY, TFT_BLACK);
        modelsCanvas->setCursor(8, 32);
        char name[9];
        strncpy(name, EepromSettings.models[selectedListIdx].name, 8);
        name[8] = '\0';
        modelsCanvas->print("Slot: ");
        modelsCanvas->print(strlen(name) == 0 ? "Empty" : name);

        modelsCanvas->fillRect(40, 45, 160, 80, 0x2104);
        modelsCanvas->drawRect(40, 45, 160, 80, TFT_WHITE);
        
        const char* popupItems[] = {"Select", "Edit", "Delete", "Back"};
        for (int i = 0; i < 4; i++) {
            int y = 50 + (i * 18);
            if (i == selectedPopupIdx) {
                modelsCanvas->fillRect(42, y - 2, 156, 18, TFT_WHITE);
                modelsCanvas->setTextColor(TFT_BLACK, TFT_WHITE);
            } else {
                modelsCanvas->setTextColor(TFT_WHITE, 0x2104);
            }
            modelsCanvas->setCursor(50, y);
            modelsCanvas->print(popupItems[i]);
        }
    }

    Ui::display.drawRGBBitmap(0, 0, modelsCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    Ui::needDisplay();
}