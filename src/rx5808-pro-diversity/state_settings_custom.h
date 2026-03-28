#ifndef STATE_SETTINGS_CUSTOM_H
#define STATE_SETTINGS_CUSTOM_H

#include "state.h"

namespace StateMachine {
    class SettingsCustomStateHandler : public StateMachine::StateHandler {
        public:
            void onEnter();
            void onExit();
            void onUpdate();

            void onInitialDraw();
            void onUpdateDraw();

            void onButtonChange(Button button, Buttons::PressType pressType);
            
        private:
            int8_t selectedItem = 0;
    };
}

#endif