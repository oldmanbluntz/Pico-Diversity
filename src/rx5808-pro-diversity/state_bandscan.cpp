#include <Arduino.h>

#include "state_bandscan.h"

#include "settings.h"
#include "settings_internal.h"
#include "settings_eeprom.h"
#include "receiver.h"
#include "channels.h"
#include "buttons.h"

#include "ui.h"
#include "ui_menu.h"

// Allocate our 64KB double buffer specifically for the bandscan
static GFXcanvas16* bsCanvas = nullptr;

// --- DYNAMIC TEXT LABELS FOR MENU ---
static const char* menuSearchText(void*) { return "Search"; }
static const char* menuSettingsText(void*) { return "Settings"; }

// --- MENU HANDLERS ---
static void menuSearchHandler(void* state) {
    StateMachine::switchState(StateMachine::State::SEARCH);
}
static void menuSettingsHandler(void* state) {
    StateMachine::switchState(StateMachine::State::SETTINGS);
}

void StateMachine::BandScanStateHandler::onEnter() {
    orderedChanelIndex = 0;
    lastChannelIndex = Receiver::activeChannel;
    
    // Register the slide-out menu items
    menu.addItem(menuSearchText, menuSearchHandler);
    menu.addItem(menuSettingsText, menuSettingsHandler);

    // Dynamically allocate the canvas when we enter the scanner
    if (!bsCanvas) {
        bsCanvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void StateMachine::BandScanStateHandler::onExit() {
    Receiver::setChannel(lastChannelIndex);
    
    // Safely free the 64KB of RAM when we leave so the screensaver can use it later!
    if (bsCanvas) {
        delete bsCanvas;
        bsCanvas = nullptr;
    }
}

void StateMachine::BandScanStateHandler::onUpdate() {
    if (!Receiver::isRssiStable())
        return;

    // Record data for true diversity color plotting
    rssiDataA[orderedChanelIndex] = Receiver::rssiA;
    
    #ifdef USE_DIVERSITY
        rssiDataB[orderedChanelIndex] = Receiver::rssiB;
    #endif

    orderedChanelIndex = (orderedChanelIndex + 1) % (CHANNELS_SIZE);
    Receiver::setChannel(Channels::getOrderedIndex(orderedChanelIndex));

    Ui::needUpdate();
}

void StateMachine::BandScanStateHandler::onButtonChange(Button button, Buttons::PressType pressType) {
    // Pass button presses to the slide-out menu.
    // If the menu is active, it intercepts the inputs and returns true.
    if (this->menu.handleButtons(button, pressType))
        return;
}

// --- UI LAYOUT CONSTANTS FOR 240x135 TFT ---
#define TEXT_SIZE 2
#define TEXT_H 16
#define TEXT_W 48

#define BORDER_LEFT_X 0
#define BORDER_LEFT_Y 0
#define BORDER_LEFT_H (SCREEN_HEIGHT - TEXT_H - 4)

#define BORDER_RIGHT_X (SCREEN_WIDTH - 1)
#define BORDER_RIGHT_Y 0
#define BORDER_RIGHT_H BORDER_LEFT_H

#define BORDER_BOTTOM_X 0
#define BORDER_BOTTOM_Y (SCREEN_HEIGHT - TEXT_H - 4)
#define BORDER_BOTTOM_W SCREEN_WIDTH

#define CHANNEL_TEXT_LOW_X 4
#define CHANNEL_TEXT_LOW_Y (SCREEN_HEIGHT - TEXT_H)

#define CHANNEL_TEXT_HIGH_X (SCREEN_WIDTH - TEXT_W - 4)
#define CHANNEL_TEXT_HIGH_Y CHANNEL_TEXT_LOW_Y

#define PROGRESS_X (CHANNEL_TEXT_LOW_X + TEXT_W + 8)
#define PROGRESS_Y (SCREEN_HEIGHT - TEXT_H + 2)
#define PROGRESS_W (CHANNEL_TEXT_HIGH_X - PROGRESS_X - 8)
#define PROGRESS_H (TEXT_H - 4)

#define GRAPH_X 1
#define GRAPH_Y 0
#define GRAPH_W (SCREEN_WIDTH - 2)
#define GRAPH_H BORDER_BOTTOM_Y


void StateMachine::BandScanStateHandler::onInitialDraw() {
    Ui::needDisplay();
}

void StateMachine::BandScanStateHandler::onUpdateDraw() {
    if (!bsCanvas) return; // Safety check

    // 1. Wipe the invisible canvas clean
    bsCanvas->fillScreen(TFT_BLACK);

    // 2. Draw graph boundaries
    bsCanvas->drawFastVLine(BORDER_LEFT_X, BORDER_LEFT_Y, BORDER_LEFT_H, TFT_LIGHTGREY);
    bsCanvas->drawFastVLine(BORDER_RIGHT_X, BORDER_RIGHT_Y, BORDER_RIGHT_H, TFT_LIGHTGREY);
    bsCanvas->drawFastHLine(BORDER_BOTTOM_X, BORDER_BOTTOM_Y, BORDER_BOTTOM_W, TFT_LIGHTGREY);

    // 3. Draw frequency labels
    bsCanvas->setTextSize(TEXT_SIZE);
    bsCanvas->setTextColor(TFT_LIGHTGREY);
    
    bsCanvas->setCursor(CHANNEL_TEXT_LOW_X, CHANNEL_TEXT_LOW_Y);
    bsCanvas->print(Channels::getFrequency(Channels::getOrderedIndex(0)));

    bsCanvas->setCursor(CHANNEL_TEXT_HIGH_X, CHANNEL_TEXT_HIGH_Y);
    bsCanvas->print(Channels::getFrequency(Channels::getOrderedIndex(CHANNELS_SIZE - 1)));

    // 4. Draw progress bar outline and fill
    bsCanvas->drawRect(PROGRESS_X - 1, PROGRESS_Y - 1, PROGRESS_W + 2, PROGRESS_H + 2, TFT_WHITE);
    
    uint16_t progressW = (orderedChanelIndex * PROGRESS_W) / CHANNELS_SIZE;
    if (progressW > 0) {
        bsCanvas->fillRect(PROGRESS_X, PROGRESS_Y, progressW, PROGRESS_H, TFT_RED);
    }
    if (progressW < PROGRESS_W) {
        bsCanvas->fillRect(PROGRESS_X + progressW, PROGRESS_Y, PROGRESS_W - progressW, PROGRESS_H, TFT_DARKGREY);
    }

    // 5. Draw the graph lines directly onto the canvas
    int dataSize = CHANNELS_SIZE;
    int scale = 100;
    
    for (uint16_t i = 0; i < dataSize - 1; i++) {
        uint16_t x1 = map(i, 0, dataSize - 2, GRAPH_X, GRAPH_X + GRAPH_W - 1);
        uint16_t x2 = map(i + 1, 0, dataSize - 2, GRAPH_X, GRAPH_X + GRAPH_W - 1);

        uint16_t pA1 = constrain(rssiDataA[i], 0, scale);
        uint16_t pA2 = constrain(rssiDataA[i + 1], 0, scale);
        uint16_t yA1 = map(pA1, 0, scale, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
        uint16_t yA2 = map(pA2, 0, scale, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
        
        #ifdef USE_DIVERSITY
            bsCanvas->drawLine(x1, yA1, x2, yA2, TFT_RED);
            
            uint16_t pB1 = constrain(rssiDataB[i], 0, scale);
            uint16_t pB2 = constrain(rssiDataB[i + 1], 0, scale);
            uint16_t yB1 = map(pB1, 0, scale, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
            uint16_t yB2 = map(pB2, 0, scale, GRAPH_Y + GRAPH_H - 2, GRAPH_Y + 2);
            
            bsCanvas->drawLine(x1, yB1, x2, yB2, TFT_CYAN);
        #else
            bsCanvas->drawLine(x1, yA1, x2, yA2, TFT_YELLOW);
        #endif
    }

    // 6. Blast the fully rendered canvas to the physical screen in one shot!
    Ui::display.drawRGBBitmap(0, 0, bsCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);

    // 7. Draw the slide-out menu directly over top of the canvas background
    this->menu.draw();

    Ui::needDisplay();
}