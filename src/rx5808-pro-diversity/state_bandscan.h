#ifndef STATE_BANDSCAN_H
#define STATE_BANDSCAN_H

#include <stdint.h>

#include "channels.h"
#include "state.h"
#include "ui_state_menu.h" // Added to support the slide-out menu

namespace StateMachine {
    class BandScanStateHandler : public StateMachine::StateHandler {
        private:
            uint8_t orderedChanelIndex = 0;
            uint8_t lastChannelIndex = 0;
            
            // Track Receiver A and B separately for the color graph
            uint8_t rssiDataA[CHANNELS_SIZE] = { 0 };
            
            #ifdef USE_DIVERSITY
                uint8_t rssiDataB[CHANNELS_SIZE] = { 0 };
            #endif

            // The slide-out menu object
            Ui::StateMenu menu;

        public:
            void onEnter();
            void onExit();
            void onUpdate();

            void onInitialDraw();
            void onUpdateDraw();
            
            // Added to intercept menu button presses
            void onButtonChange(Button button, Buttons::PressType pressType); 
    };
}

#endif