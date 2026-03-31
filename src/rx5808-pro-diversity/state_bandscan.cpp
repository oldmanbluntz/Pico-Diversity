#include <Arduino.h>
#include "state_bandscan.h"
#include "settings.h"
#include "settings_internal.h"
#include "settings_eeprom.h"
#include "receiver.h"
#include "channels.h"
#include "buttons.h"
#include "ui.h"
#include "ui_state_menu.h"

static GFXcanvas16* bsCanvas = nullptr;

// Menu text and handlers
static const char* menuSearchText(void* state) { return "Search"; }
static const char* menuSettingsText(void* state) { return "Settings"; }

static void menuSearchHandler(void* state) {
    StateMachine::switchState(StateMachine::State::SEARCH);
}
static void menuSettingsHandler(void* state) {
    StateMachine::switchState(StateMachine::State::SETTINGS);
}

void StateMachine::BandScanStateHandler::onEnter() {
    orderedChanelIndex = 0;
    lastChannelIndex = Receiver::activeChannel;
    
    // Register the items to the menu object using the StateMenuHelper API
    menu.addItem(menuSearchText, menuSearchHandler);
    menu.addItem(menuSettingsText, menuSettingsHandler);

    if (!bsCanvas) {
        bsCanvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void StateMachine::BandScanStateHandler::onExit() {
    Receiver::setChannel(lastChannelIndex);
    if (bsCanvas) {
        delete bsCanvas;
        bsCanvas = nullptr;
    }
}

void StateMachine::BandScanStateHandler::onUpdate() {
    if (!Receiver::isRssiStable())
        return;

    rssiDataA[orderedChanelIndex] = Receiver::rssiA;
    #ifdef USE_DIVERSITY
        rssiDataB[orderedChanelIndex] = Receiver::rssiB;
    #endif

    orderedChanelIndex = (orderedChanelIndex + 1) % (CHANNELS_SIZE);
    Receiver::setChannel(Channels::getOrderedIndex(orderedChanelIndex));

    Ui::needUpdate();
}

void StateMachine::BandScanStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    // Correctly passing buttons to the StateMenuHelper instance
    if (this->menu.handleButtons(button, pressType))
        return;
}

// Layout constants for 240x135
#define TEXT_SIZE 2
#define TEXT_H 16
#define TEXT_W 48
#define BORDER_BOTTOM_Y (SCREEN_HEIGHT - TEXT_H - 4)
#define PROGRESS_X (4 + TEXT_W + 8)
#define PROGRESS_Y (SCREEN_HEIGHT - TEXT_H + 2)
#define PROGRESS_W (SCREEN_WIDTH - TEXT_W - 4 - PROGRESS_X - 8)
#define PROGRESS_H (TEXT_H - 4)
#define GRAPH_X 1
#define GRAPH_Y 0
#define GRAPH_W (SCREEN_WIDTH - 2)
#define GRAPH_H BORDER_BOTTOM_Y

void StateMachine::BandScanStateHandler::onInitialDraw() {
    Ui::needDisplay();
}

void StateMachine::BandScanStateHandler::onUpdateDraw() {
    if (!bsCanvas) return;

    bsCanvas->fillScreen(TFT_BLACK);
    bsCanvas->drawFastHLine(0, BORDER_BOTTOM_Y, SCREEN_WIDTH, TFT_LIGHTGREY);

    bsCanvas->setTextSize(TEXT_SIZE);
    bsCanvas->setTextColor(TFT_LIGHTGREY);
    bsCanvas->setCursor(4, SCREEN_HEIGHT - TEXT_H);
    bsCanvas->print(Channels::getFrequency(Channels::getOrderedIndex(0)));

    bsCanvas->setCursor(SCREEN_WIDTH - TEXT_W - 4, SCREEN_HEIGHT - TEXT_H);
    bsCanvas->print(Channels::getFrequency(Channels::getOrderedIndex(CHANNELS_SIZE - 1)));

    bsCanvas->drawRect(PROGRESS_X - 1, PROGRESS_Y - 1, PROGRESS_W + 2, PROGRESS_H + 2, TFT_WHITE);
    uint16_t progressW = (orderedChanelIndex * PROGRESS_W) / CHANNELS_SIZE;
    if (progressW > 0) bsCanvas->fillRect(PROGRESS_X, PROGRESS_Y, progressW, PROGRESS_H, TFT_RED);
    if (progressW < PROGRESS_W) bsCanvas->fillRect(PROGRESS_X + progressW, PROGRESS_Y, PROGRESS_W - progressW, PROGRESS_H, TFT_DARKGREY);

    for (uint16_t i = 0; i < CHANNELS_SIZE - 1; i++) {
        uint16_t x1 = map(i, 0, CHANNELS_SIZE - 2, GRAPH_X, GRAPH_X + GRAPH_W - 1);
        uint16_t x2 = map(i + 1, 0, CHANNELS_SIZE - 2, GRAPH_X, GRAPH_X + GRAPH_W - 1);
        uint16_t yA1 = map(rssiDataA[i], 0, 100, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
        uint16_t yA2 = map(rssiDataA[i + 1], 0, 100, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
        
        #ifdef USE_DIVERSITY
            bsCanvas->drawLine(x1, yA1, x2, yA2, TFT_RED);
            uint16_t yB1 = map(rssiDataB[i], 0, 100, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
            uint16_t yB2 = map(rssiDataB[i + 1], 0, 100, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
            bsCanvas->drawLine(x1, yB1, x2, yB2, getSchemeColorB());
        #else
            bsCanvas->drawLine(x1, yA1, x2, yA2, getSchemeColorA());
        #endif
    }

    // Render the menu INTO the buffer so it's included when we push the bitmap to the display
    this->menu.draw(bsCanvas);

    Ui::display.drawRGBBitmap(0, 0, bsCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);

    Ui::needDisplay();
}