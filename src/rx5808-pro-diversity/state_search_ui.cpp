#include "state_search.h"
#include "receiver.h"
#include "channels.h"
#include "ui.h"
#include "settings_eeprom.h"

#ifndef TFT_ORANGE
#define TFT_ORANGE  0xFDA0
#endif
#ifndef TFT_PURPLE
#define TFT_PURPLE  0x780F
#endif
#ifndef TFT_MAGENTA 
#define TFT_MAGENTA 0xF81F
#endif

#define FREQUENCY_TEXT_SIZE 2 
#define FREQUENCY_TEXT_X 96   
#define FREQUENCY_TEXT_Y 6    

#define CHANNEL_TEXT_SIZE 8   
#define CHANNEL_TEXT_X 72     
#define CHANENL_TEXT_Y 24     

#define BARS_Y 90             
#define BARS_H 40             
#define BARS_X 44             
#define BARS_W (SCREEN_WIDTH - BARS_X - 4)

#define COLOR_RXA TFT_YELLOW
#define COLOR_RXB TFT_CYAN

using Ui::display;

// Create a static canvas for the search UI to prevent flickering
static GFXcanvas16* searchCanvas = nullptr;

void StateMachine::SearchStateHandler::onInitialDraw() {
    if (!searchCanvas) {
        searchCanvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    Ui::needDisplay();
}

void StateMachine::SearchStateHandler::onUpdateDraw() {
    if (!searchCanvas) return;

    // 1. Clear the buffer
    searchCanvas->fillScreen(TFT_BLACK);

    // 2. Draw static labels
    searchCanvas->setTextSize(2);
    #ifdef USE_DIVERSITY
        searchCanvas->setTextColor(COLOR_RXA, TFT_BLACK);
        searchCanvas->setCursor(2, BARS_Y);
        searchCanvas->print("RXA");
        
        searchCanvas->setTextColor(COLOR_RXB, TFT_BLACK);
        searchCanvas->setCursor(2, BARS_Y + (BARS_H / 2));
        searchCanvas->print("RXB");
    #else
        searchCanvas->setTextColor(COLOR_RXA, TFT_BLACK);
        searchCanvas->setCursor(2, BARS_Y + (BARS_H / 4));
        searchCanvas->print("RX");
    #endif

    // 3. Draw dynamic text into the canvas
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

    searchCanvas->setTextSize(CHANNEL_TEXT_SIZE);
    searchCanvas->setCursor(CHANNEL_TEXT_X, CHANENL_TEXT_Y);
    searchCanvas->setTextColor(letterColor, TFT_BLACK);
    searchCanvas->print(String(letter));
    
    // Check if Night Mode is active for the numbers
    uint16_t numColor = (EepromSettings.uiScheme == 1) ? TFT_RED : TFT_WHITE;
    
    searchCanvas->setTextColor(numColor, TFT_BLACK);
    searchCanvas->print(String(number));

    searchCanvas->setTextSize(FREQUENCY_TEXT_SIZE);
    searchCanvas->setTextColor(numColor, TFT_BLACK);
    searchCanvas->setCursor(FREQUENCY_TEXT_X, FREQUENCY_TEXT_Y);
    searchCanvas->print(Channels::getFrequency(Receiver::activeChannel));

    // 4. Draw RSSI indicators
    // Calculate width available based on menu visibility
    // If menu is 64 wide, available width ends at 240 - 64 = 176
    uint16_t activeBarsW = this->menu.isVisible() ? (176 - BARS_X - 4) : BARS_W;
    uint16_t barW_A = map(constrain(Receiver::rssiA, 0, 100), 0, 100, 0, activeBarsW);
    searchCanvas->fillRect(BARS_X, BARS_Y, barW_A, (BARS_H / 2) - 2, COLOR_RXA);

    #ifdef USE_DIVERSITY
        uint16_t barW_B = map(constrain(Receiver::rssiB, 0, 100), 0, 100, 0, activeBarsW);
        searchCanvas->fillRect(BARS_X, BARS_Y + (BARS_H / 2), barW_B, (BARS_H / 2) - 2, COLOR_RXB);
    #endif

    // 5. Draw the menu into the canvas buffer
    if (this->menu.isVisible()) {
        this->menu.draw(searchCanvas);
    }

    // 6. Push the complete flicker-free frame to the hardware display
    display.drawRGBBitmap(0, 0, searchCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    
    Ui::needDisplay();
}

void StateMachine::SearchStateHandler::drawBorders() { }
void StateMachine::SearchStateHandler::drawChannelText() { }
void StateMachine::SearchStateHandler::drawFrequencyText() { }
void StateMachine::SearchStateHandler::drawScanBar() { }
void StateMachine::SearchStateHandler::drawRssiGraph() { }
void StateMachine::SearchStateHandler::drawMenu() { }