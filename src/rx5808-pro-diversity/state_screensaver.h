#ifndef STATE_SCREENSAVER_H
#define STATE_SCREENSAVER_H


#include "state.h"
#include "timer.h"
#include "settings.h"


namespace StateMachine {
    class ScreensaverStateHandler : public StateMachine::StateHandler {
        private:
            // PICO FIX: Hardcoded 5000ms (5s) to avoid missing dependency errors
            // from settings_internal.h during the port.
            Timer displaySwapTimer = Timer(5000);
            bool showLogo = false;

        public:
            void onEnter();
            void onUpdate();

            void onInitialDraw();
            void onUpdateDraw();

            void onButtonChange(Button button, Buttons::PressType pressType);
    };
}


#endif