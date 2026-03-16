#include "state_search.h"
#include "receiver.h"
#include "channels.h"
#include "ui.h"

#define FREQUENCY_TEXT_SIZE 1
#define FREQUENCY_TEXT_X 68 
#define FREQUENCY_TEXT_Y 2

#define CHANNEL_TEXT_SIZE 5
#define CHANNEL_TEXT_X 46   
#define CHANENL_TEXT_Y 14

#define BARS_Y 58
#define BARS_H 20
#define BARS_X 28
#define BARS_W (160 - BARS_X - 4)

#define COLOR_RXA TFT_YELLOW
#define COLOR_RXB TFT_CYAN

using Ui::display;

void StateMachine::SearchStateHandler::onInitialDraw() {
    Ui::clear();

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

void StateMachine::SearchStateHandler::drawBorders() { }

void StateMachine::SearchStateHandler::drawChannelText() {
    const char* name = Channels::getName(Receiver::activeChannel);
    char letter = name[0];
    const char* number = &name[1]; 

    uint16_t letterColor = TFT_WHITE;
    switch(letter) {
        case 'A': letterColor = TFT_RED; break;    
        case 'B': letterColor = TFT_ORANGE; break; 
        case 'E': letterColor = TFT_YELLOW; break; 
        case 'F': letterColor = TFT_GREEN; break;  
        case 'R': letterColor = TFT_BLUE; break;   
        case 'L': letterColor = TFT_PURPLE; break; 
        case 'U': letterColor = TFT_MAGENTA; break;
        default:  letterColor = TFT_WHITE; break;
    }

    display.setTextSize(CHANNEL_TEXT_SIZE);
    display.setCursor(CHANNEL_TEXT_X, CHANENL_TEXT_Y);
    
    display.setTextColor(letterColor, TFT_BLACK);
    display.print(letter);
    
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.print(number);
}

void StateMachine::SearchStateHandler::drawFrequencyText() {
    display.setTextSize(FREQUENCY_TEXT_SIZE);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setCursor(FREQUENCY_TEXT_X, FREQUENCY_TEXT_Y);
    display.print(Channels::getFrequency(Receiver::activeChannel));
}

void StateMachine::SearchStateHandler::drawScanBar() { }

void StateMachine::SearchStateHandler::drawRssiGraph() {
    // Dynamically shrink the bars if the menu is open to prevent overlap
    uint16_t activeBarsW = this->menu.isVisible() ? (108 - BARS_X - 2) : BARS_W;

    #ifdef USE_DIVERSITY
        Ui::drawRssiBars(Receiver::rssiA, Receiver::rssiB, 0, 100, BARS_X, BARS_Y, activeBarsW, BARS_H, COLOR_RXA, COLOR_RXB);
    #else
        Ui::drawRssiBars(Receiver::rssiA, 0, 0, 100, BARS_X, BARS_Y, activeBarsW, BARS_H, COLOR_RXA, COLOR_RXB);
    #endif
}

void StateMachine::SearchStateHandler::drawMenu() {
    this->menu.draw();
}