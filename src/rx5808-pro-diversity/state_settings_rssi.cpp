#include <stdint.h>
#include <Arduino.h> 

#include "state_settings_rssi.h"

#include "receiver.h"
#include "channels.h"

#include "settings.h"
#include "settings_internal.h"
#include "settings_eeprom.h"
#include "buttons.h"

#include "ui.h"

void StateMachine::SettingsRssiStateHandler::onEnter() {
    internalState = InternalState::WAIT_FOR_LOW;
}

void StateMachine::SettingsRssiStateHandler::onUpdate() {
    if (!Receiver::isRssiStable())
        return;

    switch (internalState) {
        case InternalState::SCANNING_LOW:
            if (Receiver::rssiARaw < EepromSettings.rssiAMin)
                EepromSettings.rssiAMin = Receiver::rssiARaw;

            #ifdef USE_DIVERSITY
                if (Receiver::rssiBRaw < EepromSettings.rssiBMin)
                    EepromSettings.rssiBMin = Receiver::rssiBRaw;
            #endif
        break;

        case InternalState::SCANNING_HIGH:
            if (Receiver::rssiARaw > EepromSettings.rssiAMax)
                EepromSettings.rssiAMax = Receiver::rssiARaw;

            #ifdef USE_DIVERSITY
                if (Receiver::rssiBRaw > EepromSettings.rssiBMax)
                    EepromSettings.rssiBMax = Receiver::rssiBRaw;
            #endif
        break;
    }

    Receiver::setChannel((Receiver::activeChannel + 1) % CHANNELS_SIZE);
    if (Receiver::activeChannel == 0) {
        currentSweep++;

        if (currentSweep == RSSI_SETUP_RUN) {
            switch (internalState) {
                case InternalState::SCANNING_LOW:
                    internalState = InternalState::WAIT_FOR_HIGH;
                break;

                case InternalState::SCANNING_HIGH:
                    internalState = InternalState::DONE;
                break;
            }

            Ui::needUpdate();
        }
    }
}

void StateMachine::SettingsRssiStateHandler::onButtonChange(
    Button button,
    Buttons::PressType pressType
) {
    if (button != Button::MODE || pressType != Buttons::PressType::SHORT)
        return;

    switch (internalState) {
        case InternalState::WAIT_FOR_LOW:
            internalState = InternalState::SCANNING_LOW;
            currentSweep = 0;
            Receiver::setChannel(0);

            EepromSettings.rssiAMin = UINT16_MAX;
            #ifdef USE_DIVERSITY
                EepromSettings.rssiBMin = UINT16_MAX;
            #endif
        break;

        case InternalState::WAIT_FOR_HIGH:
            internalState = InternalState::SCANNING_HIGH;
            currentSweep = 0;
            Receiver::setChannel(0);

            EepromSettings.rssiAMax = 0;
            #ifdef USE_DIVERSITY
                EepromSettings.rssiBMax = 0;
            #endif
        break;

        case InternalState::DONE:
            EepromSettings.models[EepromSettings.activeModel].rssiAMin = EepromSettings.rssiAMin;
            EepromSettings.models[EepromSettings.activeModel].rssiAMax = EepromSettings.rssiAMax;
            #ifdef USE_DIVERSITY
                EepromSettings.models[EepromSettings.activeModel].rssiBMin = EepromSettings.rssiBMin;
                EepromSettings.models[EepromSettings.activeModel].rssiBMax = EepromSettings.rssiBMax;
            #endif

            EepromSettings.save();
            StateMachine::switchState(StateMachine::State::SETTINGS);
        break;
    }

    Ui::needUpdate();
}

void StateMachine::SettingsRssiStateHandler::onInitialDraw() {
    Ui::needUpdate(); 
}

void StateMachine::SettingsRssiStateHandler::onUpdateDraw() {
    Ui::clear();

    Ui::display.setTextSize(2);

    switch (internalState) {
        case InternalState::WAIT_FOR_LOW:
            Ui::display.setTextColor(getSchemeColorMenu(), TFT_BLACK);
            Ui::display.setCursor(0, 0);
            Ui::display.print("1/4");
            Ui::display.setTextColor(TFT_WHITE, TFT_BLACK);
            Ui::display.print("\nTurn off all\nVTXs.");
            
            Ui::display.setCursor(0, 60);
            Ui::display.print("Remove RX\nantennas.");

            Ui::display.setCursor(0, SCREEN_HEIGHT - 36);
            Ui::display.print("Press MODE\nwhen ready.");
        break;

        case InternalState::SCANNING_LOW:
            Ui::display.setTextColor(getSchemeColorMenu(), TFT_BLACK);
            Ui::display.setCursor(0, 0);
            Ui::display.print("2/4");
            Ui::display.setTextColor(TFT_WHITE, TFT_BLACK);
            Ui::display.print("\nScanning for\nlowest RSSI...");
        break;

        case InternalState::WAIT_FOR_HIGH:
            Ui::display.setTextColor(getSchemeColorMenu(), TFT_BLACK);
            Ui::display.setCursor(0, 0);
            Ui::display.print("3/4");
            Ui::display.setTextColor(TFT_WHITE, TFT_BLACK);
            Ui::display.print("\nTurn on your\nVTX.");

            Ui::display.setCursor(0, SCREEN_HEIGHT - 36);
            Ui::display.print("Press MODE\nwhen ready.");
        break;

        case InternalState::SCANNING_HIGH:
            Ui::display.setTextColor(getSchemeColorMenu(), TFT_BLACK);
            Ui::display.setCursor(0, 0);
            Ui::display.print("4/4");
            Ui::display.setTextColor(TFT_WHITE, TFT_BLACK);
            Ui::display.print("\nScanning for\nhighest RSSI...");
        break;

        case InternalState::DONE:
            Ui::display.setTextColor(getSchemeColorMenu(), TFT_BLACK);
            Ui::display.setCursor(0, 0);
            Ui::display.print("All done!");

            Ui::display.setTextColor(TFT_WHITE, TFT_BLACK);
            Ui::display.setCursor(0, 40);
            Ui::display.print("Min: ");
            Ui::display.setTextColor(getSchemeColorMenu(), TFT_BLACK);
            Ui::display.setCursor(60, 40);
            Ui::display.print(EepromSettings.rssiAMin);
            #ifdef USE_DIVERSITY
                Ui::display.setCursor(140, 40);
                Ui::display.print(EepromSettings.rssiBMin);
            #endif

            Ui::display.setTextColor(TFT_WHITE, TFT_BLACK);
            Ui::display.setCursor(0, 64);
            Ui::display.print("Max: ");
            Ui::display.setTextColor(getSchemeColorMenu(), TFT_BLACK);
            Ui::display.setCursor(60, 64);
            Ui::display.print(EepromSettings.rssiAMax);
            #ifdef USE_DIVERSITY
                Ui::display.setCursor(140, 64);
                Ui::display.print(EepromSettings.rssiBMax);
            #endif

            Ui::display.setTextColor(TFT_WHITE, TFT_BLACK);
            Ui::display.setCursor(0, SCREEN_HEIGHT - 20);
            Ui::display.print("Press MODE to save");
        break;
    }

    Ui::needDisplay();
}