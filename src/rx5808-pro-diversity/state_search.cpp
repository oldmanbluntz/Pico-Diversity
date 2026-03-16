#include <Arduino.h>

#include "state_search.h"

#include "settings.h"
#include "settings_internal.h"
#include "settings_eeprom.h"
#include "receiver.h"
#include "channels.h"
#include "buttons.h"
#include "ui.h"

using StateMachine::SearchStateHandler;

// --- DYNAMIC TEXT LABELS ---
static const char* menuModeText(void* state) {
    SearchStateHandler* search = static_cast<SearchStateHandler*>(state);
    return search->manual ? "Manual" : "Auto"; 
}

static const char* menuOrderText(void* state) {
    SearchStateHandler* search = static_cast<SearchStateHandler*>(state);
    return search->order == SearchStateHandler::ScanOrder::FREQUENCY ? "By Freq" : "By Chan"; 
}

static const char* bandscanText(void*) { return "Bandscan"; }  // Restored full word
static const char* settingsText(void*) { return "Settings"; }  // Restored full word
static const char* backText(void*) { return "Back"; }


// --- HANDLERS ---
static void menuModeHandler(void* state) {
    SearchStateHandler* search = static_cast<SearchStateHandler*>(state);
    search->manual = !search->manual;

    EepromSettings.searchManual = search->manual;
    EepromSettings.markDirty();
}

static void menuOrderHandler(void* state) {
    SearchStateHandler* search = static_cast<SearchStateHandler*>(state);
    if (search->order == SearchStateHandler::ScanOrder::FREQUENCY) {
        search->order = SearchStateHandler::ScanOrder::CHANNEL;
        search->orderedChanelIndex = Channels::getOrderedIndex(search->orderedChanelIndex);
        EepromSettings.searchOrderByChannel = true;
    } else {
        search->order = SearchStateHandler::ScanOrder::FREQUENCY;
        search->orderedChanelIndex = Channels::getOrderedIndexFromIndex(search->orderedChanelIndex);
        EepromSettings.searchOrderByChannel = false;
    }
    EepromSettings.markDirty();
}

static void bandscanHandler(void* state) {
    StateMachine::switchState(StateMachine::State::BANDSCAN);
}

static void settingsHandler(void* state) {
    StateMachine::switchState(StateMachine::State::SETTINGS);
}

static void backHandler(void* state) {
    SearchStateHandler* search = static_cast<SearchStateHandler*>(state);
    search->hideMenu(); 
}

// --- STATE LOGIC ---
void SearchStateHandler::onEnter() {
    // Register all 5 menu items
    menu.addItem(menuModeText, menuModeHandler);
    menu.addItem(menuOrderText, menuOrderHandler);
    menu.addItem(bandscanText, bandscanHandler);
    menu.addItem(settingsText, settingsHandler);
    menu.addItem(backText, backHandler);

    this->manual = EepromSettings.searchManual;
    this->order = EepromSettings.searchOrderByChannel ?
        ScanOrder::CHANNEL :
        ScanOrder::FREQUENCY;

    switch (this->order) {
        case ScanOrder::CHANNEL:
            this->orderedChanelIndex = EepromSettings.startChannel;
            break;

        case ScanOrder::FREQUENCY:
            this->orderedChanelIndex =
                Channels::getOrderedIndexFromIndex(EepromSettings.startChannel);
            break;
    }
}

void SearchStateHandler::onUpdate() {
    if (!manual) {
        onUpdateAuto();
    }

    Ui::needUpdate();
}

void SearchStateHandler::onUpdateAuto() {
    if (scanningPeak) {
        uint8_t peaksIndex = peakChannelIndex - orderedChanelIndex;
        peaks[peaksIndex] = Receiver::rssiA;
        peakChannelIndex++;

        if (peaksIndex >= PEAK_LOOKAHEAD || peakChannelIndex >= CHANNELS_SIZE) {
            uint8_t largestPeak = 0;
            uint8_t largestPeakIndex = 0;
            for (uint8_t i = 0; i < PEAK_LOOKAHEAD; i++) {
                uint8_t peak = peaks[i];
                if (peak > largestPeak) {
                    largestPeak = peak;
                    largestPeakIndex = i;
                }
            }

            uint8_t peakChannel = orderedChanelIndex + largestPeakIndex;
            orderedChanelIndex = peakChannel;
            Receiver::setChannel(Channels::getOrderedIndex(peakChannel));

            EepromSettings.startChannel = Channels::getOrderedIndex(peakChannel);
            EepromSettings.markDirty();

            scanningPeak = false;
        } else {
            Receiver::setChannel(Channels::getOrderedIndex(peakChannelIndex));
        }
    } else {
        if (scanning) {
            if (!forceNext && Receiver::rssiA >= RSSI_SEEK_TRESHOLD) {
                scanning = false;
                scanningPeak = true;
                peakChannelIndex = orderedChanelIndex;

                for (uint8_t i = 0; i < PEAK_LOOKAHEAD; i++)
                    peaks[i] = 0;
            } else {
                orderedChanelIndex += static_cast<int8_t>(direction);
                if (orderedChanelIndex == 255)
                    orderedChanelIndex = CHANNELS_SIZE - 1;
                else if (orderedChanelIndex >= CHANNELS_SIZE)
                    orderedChanelIndex = 0;

                Receiver::setChannel(Channels::getOrderedIndex(orderedChanelIndex));

                if (forceNext)
                    forceNext = false;
            }
        }
    }
}

void SearchStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    if (this->menu.handleButtons(button, pressType))
        return;

    if (!this->manual) {
        if (pressType != Buttons::PressType::SHORT || button == Button::MODE) {
            return;
        }

        scanning = true;
        forceNext = true;
        direction = button == Button::UP ? ScanDirection::UP : ScanDirection::DOWN;
    } else {
        if (pressType != Buttons::PressType::SHORT && pressType != Buttons::PressType::HOLDING) {
            return;
        }

        if (button == Button::UP) {
            orderedChanelIndex += 1;
        } else if (button == Button::DOWN) {
            orderedChanelIndex -= 1;
        }

        if (orderedChanelIndex == 255)
            orderedChanelIndex = CHANNELS_SIZE - 1;
        else if (orderedChanelIndex >= CHANNELS_SIZE)
            orderedChanelIndex = 0;

        this->setChannel();
    }
}

void SearchStateHandler::setChannel() {
    uint8_t actualChannelIndex;
    if (this->order == ScanOrder::FREQUENCY) {
        actualChannelIndex = Channels::getOrderedIndex(orderedChanelIndex);
    } else {
        actualChannelIndex = orderedChanelIndex;
    }

    Receiver::setChannel(actualChannelIndex);
    EepromSettings.startChannel = actualChannelIndex;
    EepromSettings.markDirty();
}