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
#ifndef TFT_DARKGREY
#define TFT_DARKGREY 0x7BEF
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

using Ui::display;

static GFXcanvas16* searchCanvas = nullptr;

void StateMachine::SearchStateHandler::onInitialDraw() {
    if (!searchCanvas) {
        searchCanvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    Ui::needDisplay();
}

void StateMachine::SearchStateHandler::onUpdateDraw() {
    if (!searchCanvas) return;

    searchCanvas->fillScreen(TFT_BLACK);

    // Shared dynamic text data
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

    // Night Mode numbers toggle
    uint16_t numColor = (EepromSettings.uiScheme == 1) ? TFT_RED : TFT_WHITE;

    if (EepromSettings.uiLayout == 0) {
        // ==========================================
        // LAYOUT 0: BARS 
        // ==========================================
        
        searchCanvas->setTextSize(2);
        #ifdef USE_DIVERSITY
            searchCanvas->setTextColor(getSchemeColorA(), TFT_BLACK);
            searchCanvas->setCursor(2, BARS_Y);
            searchCanvas->print("RXA");
            
            searchCanvas->setTextColor(getSchemeColorB(), TFT_BLACK);
            searchCanvas->setCursor(2, BARS_Y + (BARS_H / 2));
            searchCanvas->print("RXB");
        #else
            searchCanvas->setTextColor(getSchemeColorA(), TFT_BLACK);
            searchCanvas->setCursor(2, BARS_Y + (BARS_H / 4));
            searchCanvas->print("RX");
        #endif

        searchCanvas->setTextSize(CHANNEL_TEXT_SIZE);
        searchCanvas->setCursor(CHANNEL_TEXT_X, CHANENL_TEXT_Y);
        searchCanvas->setTextColor(letterColor, TFT_BLACK);
        searchCanvas->print(String(letter));
        
        searchCanvas->setTextColor(numColor, TFT_BLACK);
        searchCanvas->print(String(number));

        searchCanvas->setTextSize(FREQUENCY_TEXT_SIZE);
        searchCanvas->setTextColor(numColor, TFT_BLACK);
        searchCanvas->setCursor(FREQUENCY_TEXT_X, FREQUENCY_TEXT_Y);
        searchCanvas->print(Channels::getFrequency(Receiver::activeChannel));

        uint16_t activeBarsW = this->menu.isVisible() ? (176 - BARS_X - 4) : BARS_W;
        uint16_t barW_A = map(constrain(Receiver::rssiA, 0, 100), 0, 100, 0, activeBarsW);
        searchCanvas->fillRect(BARS_X, BARS_Y, barW_A, (BARS_H / 2) - 2, getSchemeColorA());

        #ifdef USE_DIVERSITY
            uint16_t barW_B = map(constrain(Receiver::rssiB, 0, 100), 0, 100, 0, activeBarsW);
            searchCanvas->fillRect(BARS_X, BARS_Y + (BARS_H / 2), barW_B, (BARS_H / 2) - 2, getSchemeColorB());
        #endif

    } else {
        // ==========================================
        // LAYOUT 1: GRAPHS 
        // ==========================================
        
        // Channel Name (Top Left)
        searchCanvas->setTextSize(5); 
        searchCanvas->setCursor(4, 20);
        searchCanvas->setTextColor(letterColor, TFT_BLACK);
        searchCanvas->print(String(letter));
        searchCanvas->setTextColor(numColor, TFT_BLACK);
        searchCanvas->print(String(number));

        // Frequency (Below Channel Name)
        searchCanvas->setTextSize(2);
        searchCanvas->setTextColor(numColor, TFT_BLACK);
        searchCanvas->setCursor(4, 64);
        searchCanvas->print(Channels::getFrequency(Receiver::activeChannel));

        // Graph Dimensions & Layout
        uint16_t graphX = 80;
        uint16_t graphW = this->menu.isVisible() ? (176 - graphX - 4) : (SCREEN_WIDTH - graphX - 4);
        uint16_t graphH = 46;
        uint16_t gYa = 12;
        uint16_t gYb = 76;

        #ifndef USE_DIVERSITY
            gYa = 44; 
        #endif

        // RXA Graph
        searchCanvas->drawRect(graphX - 1, gYa - 1, graphW + 2, graphH + 2, TFT_LIGHTGREY);
        searchCanvas->setTextSize(1);
        searchCanvas->setTextColor(getSchemeColorA(), TFT_BLACK);
        searchCanvas->setCursor(graphX + 2, gYa + 2);
        searchCanvas->print("RXA");

        int lastX = graphX;
        int lastYa = gYa + graphH - map(constrain(Receiver::rssiALast[0], 0, 100), 0, 100, 0, graphH);
        
        for (int i = 1; i < RECEIVER_LAST_DATA_SIZE; i++) {
            int x = graphX + (i * graphW) / (RECEIVER_LAST_DATA_SIZE - 1);
            int ya = gYa + graphH - map(constrain(Receiver::rssiALast[i], 0, 100), 0, 100, 0, graphH);
            searchCanvas->drawLine(lastX, lastYa, x, ya, getSchemeColorA());
            lastX = x;
            lastYa = ya;
        }
        searchCanvas->fillCircle(lastX, lastYa, 2, getSchemeColorA());

        #ifdef USE_DIVERSITY
        // RXB Graph
        searchCanvas->drawRect(graphX - 1, gYb - 1, graphW + 2, graphH + 2, TFT_LIGHTGREY);
        searchCanvas->setTextSize(1);
        searchCanvas->setTextColor(getSchemeColorB(), TFT_BLACK);
        searchCanvas->setCursor(graphX + 2, gYb + 2);
        searchCanvas->print("RXB");

        lastX = graphX;
        int lastYb = gYb + graphH - map(constrain(Receiver::rssiBLast[0], 0, 100), 0, 100, 0, graphH);
        
        for (int i = 1; i < RECEIVER_LAST_DATA_SIZE; i++) {
            int x = graphX + (i * graphW) / (RECEIVER_LAST_DATA_SIZE - 1);
            int yb = gYb + graphH - map(constrain(Receiver::rssiBLast[i], 0, 100), 0, 100, 0, graphH);
            searchCanvas->drawLine(lastX, lastYb, x, yb, getSchemeColorB());
            lastX = x;
            lastYb = yb;
        }
        searchCanvas->fillCircle(lastX, lastYb, 2, getSchemeColorB());
        #endif
    }

    if (this->menu.isVisible()) {
        this->menu.draw(searchCanvas);
    }

    display.drawRGBBitmap(0, 0, searchCanvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    
    Ui::needDisplay();
}

void StateMachine::SearchStateHandler::drawBorders() { }
void StateMachine::SearchStateHandler::drawChannelText() { }
void StateMachine::SearchStateHandler::drawFrequencyText() { }
void StateMachine::SearchStateHandler::drawScanBar() { }
void StateMachine::SearchStateHandler::drawRssiGraph() { }
void StateMachine::SearchStateHandler::drawMenu() { }