#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include "buttons.h"

namespace StateMachine {
    #define STATE_COUNT 8
    enum class State : uint8_t {
        BOOT,
        SEARCH,
        BANDSCAN,
        SCREENSAVER,
        MENU,
        SETTINGS,
        SETTINGS_RSSI,
        SETTINGS_CUSTOM
    };

    class StateHandler {
        public:
            virtual void onInitialDraw() {};
            virtual void onUpdateDraw() {};

            virtual void onEnter() {};
            virtual void onUpdate() {};
            virtual void onExit() {};

            virtual void onButtonChange(
                Button button,
                Buttons::PressType pressType) {};
    };

    extern State currentState;
    extern State lastState;

    void setup();
    void update();

    void switchState(State newState);
}

#endif