#include "state_search.h"

#include "receiver.h"
#include "channels.h"
#include "ui.h"

// Top 2/3 Layout (Y: 0 to 53)
#define FREQUENCY_TEXT_SIZE 1
#define FREQUENCY_TEXT_X 68 // Centered: (160 - (4 chars * 6px * size 1)) / 2
#define FREQUENCY_TEXT_Y 2

#define CHANNEL_TEXT_SIZE 5
#define CHANNEL_TEXT_X 46   // Centered: (160 - (2 chars * 6px * size 5)) / 2
#define CHANENL_TEXT_Y 14

// Bottom 1/3 Layout (Y: 54 to 80)
#define BARS_Y 58
#define BARS_H 20
#define BARS_X 28
#define BARS_W (SCREEN_WIDTH - BARS_X - 4)

// Sync colors for Labels and Bars
#define COLOR_RXA TFT_YELLOW
#define COLOR_RXB TFT_CYAN

using Ui::display;

void StateMachine::SearchStateHandler::onInitialDraw() {
    Ui::clear();

    // Draw static labels for RXA and RXB
    display.setTextSize(1);
    #ifdef USE_DIVERSITY
        display.setTextColor(COLOR_RXA, TFT_BLACK);
        display.setCursor(2, BARS_Y);
        display.print("RXA");
        
        display.setTextColor(COLOR_RXB, TFT_BLACK);
        display.setCursor(2, BARS_Y + (BARS_H / 2));
        display.print("RXB");
    #else
        display.setTextColor(COLOR_RXA, TFT_BLACK);
        display.setCursor(2, BARS_Y + (BARS_H / 4));
        display.print("RX");
    #endif

    drawChannelText();
    drawFrequencyText();
    
    // Force a full redraw of the bars on screen load to prevent holes
    #ifdef USE_DIVERSITY
        Ui::drawRssiBars(Receiver::rssiA, Receiver::rssiB, 0, 100, BARS_X, BARS_Y, BARS_W, BARS_H, COLOR_RXA, COLOR_RXB, true);
    #else
        Ui::drawRssiBars(Receiver::rssiA, 0, 0, 100, BARS_X, BARS_Y, BARS_W, BARS_H, COLOR_RXA, COLOR_RXB, true);
    #endif

    Ui::needDisplay();
}

void StateMachine::SearchStateHandler::onUpdateDraw() {
    drawChannelText();
    drawFrequencyText();
    drawRssiGraph();
    menu.draw();
    Ui::needDisplay();
}

void StateMachine::SearchStateHandler::drawBorders() {
    // Intentionally left empty to satisfy the header definition
}

void StateMachine::SearchStateHandler::drawChannelText() {
    const char* name = Channels::getName(Receiver::activeChannel);
    char letter = name[0];
    const char* number = &name[1]; // Grabs the numeric remainder

    uint16_t letterColor = TFT_WHITE;
    
    // Rainbow color ordering for standard FPV bands
    switch(letter) {
        case 'A': letterColor = TFT_RED; break;     // Boscam A
        case 'B': letterColor = TFT_ORANGE; break;  // Boscam B
        case 'E': letterColor = TFT_YELLOW; break;  // Boscam E
        case 'F': letterColor = TFT_GREEN; break;   // Fatshark
        case 'R': letterColor = TFT_BLUE; break;    // Raceband
        case 'L': letterColor = TFT_PURPLE; break;  // Lowband
        case 'U': letterColor = TFT_MAGENTA; break; // User/Custom
        default:  letterColor = TFT_WHITE; break;
    }

    display.setTextSize(CHANNEL_TEXT_SIZE);
    display.setCursor(CHANNEL_TEXT_X, CHANENL_TEXT_Y);
    
    // Draw the band letter in color
    display.setTextColor(letterColor, TFT_BLACK);
    display.print(letter);
    
    // Draw the channel number in white
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.print(number);
}

void StateMachine::SearchStateHandler::drawFrequencyText() {
    display.setTextSize(FREQUENCY_TEXT_SIZE);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setCursor(FREQUENCY_TEXT_X, FREQUENCY_TEXT_Y);

    display.print(Channels::getFrequency(Receiver::activeChannel));
}

void StateMachine::SearchStateHandler::drawScanBar() {
    // Intentionally left empty to satisfy the header definition
}

void StateMachine::SearchStateHandler::drawRssiGraph() {
    // Regular update drawing loop. forceRedraw flag is naturally omitted/false.
    #ifdef USE_DIVERSITY
        Ui::drawRssiBars(Receiver::rssiA, Receiver::rssiB, 0, 100, BARS_X, BARS_Y, BARS_W, BARS_H, COLOR_RXA, COLOR_RXB);
    #else
        Ui::drawRssiBars(Receiver::rssiA, 0, 0, 100, BARS_X, BARS_Y, BARS_W, BARS_H, COLOR_RXA, COLOR_RXB);
    #endif
}

void StateMachine::SearchStateHandler::drawMenu() {
    this->menu.draw();
}