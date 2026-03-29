#ifndef STATE_BANDSCAN_H
#define STATE_BANDSCAN_H

#include <stdint.h>
#include "channels.h"
#include "state.h"
#include "ui_state_menu.h" //

namespace StateMachine {
    class BandScanStateHandler : public StateMachine::StateHandler {
        private:
            uint8_t orderedChanelIndex = 0;
            uint8_t lastChannelIndex = 0;
            
            uint8_t rssiDataA[CHANNELS_SIZE] = { 0 };
            
            #ifdef USE_DIVERSITY
                uint8_t rssiDataB[CHANNELS_SIZE] = { 0 };
            #endif

            // CORRECTED: Use the actual class name from ui_state_menu.h
            Ui::StateMenuHelper menu; 

        public:
            // The constructor must initialize the StateMenuHelper with 'this'
            BandScanStateHandler() : menu(this) {}

            void onEnter();
            void onExit();
            void onUpdate();

            void onInitialDraw();
            void onUpdateDraw();
            
            void onButtonChange(Button button, Buttons::PressType pressType); 
    };
}

#endif