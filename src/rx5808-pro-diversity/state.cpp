#include <stddef.h>
#include <new> // PICO FIX: Use standard header for placement new

#include "state.h"

#include "state_screensaver.h"
#include "state_search.h"
#include "state_bandscan.h"
#include "state_menu.h"
#include "state_settings.h"
#include "state_settings_rssi.h"
#include "state_settings_custom.h"

#include "ui.h"
#include "buttons.h"

#include "timer.h"


// PICO FIX: Removed manual definition of 'operator new'.
// The ARM toolchain provides this via <new>.

#ifdef MAX
    #undef MAX
#endif
#define MAX(a, b) (a > b ? a : b)
#define STATE_BUFFER_SIZE \
    MAX(sizeof(ScreensaverStateHandler), \
    MAX(sizeof(SearchStateHandler), \
    MAX(sizeof(BandScanStateHandler), \
    MAX(sizeof(MenuStateHandler), \
    MAX(sizeof(SettingsStateHandler), \
    MAX(sizeof(SettingsCustomStateHandler), \
        sizeof(SettingsRssiStateHandler) \
    ))))))
;

namespace StateMachine {
    static void onButtonChange(Button button, Buttons::PressType pressType);
    static StateHandler *getStateHandler(State stateType);

    alignas(4) static uint8_t stateBuffer[STATE_BUFFER_SIZE];
    static StateHandler* currentHandler = nullptr;
    State currentState = State::BOOT;
    State lastState = currentState;

    void setup() {
        Buttons::registerChangeFunc(onButtonChange);
    }

    void update() {
        if (currentHandler) {
            currentHandler->onUpdate();

            // FIXME: This should probably be handled in the UI module but not
            // 100% on how to decouple them at this stage
            static Timer drawTimer = Timer(OLED_FRAMERATE);
            if (currentHandler
                && Ui::shouldDrawUpdate
                && drawTimer.hasTicked()
            ) {
                if (Ui::shouldFullRedraw) {
                    currentHandler->onInitialDraw();
                    Ui::shouldFullRedraw = false;
                }

                currentHandler->onUpdateDraw();
                Ui::shouldDrawUpdate = false;
                drawTimer.reset();
            }
        }
    }

    void switchState(State newState) {
        if (currentHandler != nullptr) {
            currentHandler->onExit();
        }

        lastState = currentState;
        currentState = newState;
        currentHandler = getStateHandler(newState);

        if (currentHandler != nullptr) {
            currentHandler->onEnter();
            currentHandler->onInitialDraw();
        }
    }

    static StateHandler *getStateHandler(State state) {
        #define STATE_FACTORY(s, c) \
            case s: \
                return new (&stateBuffer) c(); \
                break;

        switch (state) {
            STATE_FACTORY(State::SEARCH, SearchStateHandler);
            STATE_FACTORY(State::SCREENSAVER, ScreensaverStateHandler);
            STATE_FACTORY(State::BANDSCAN, BandScanStateHandler);
            STATE_FACTORY(State::MENU, MenuStateHandler);
            STATE_FACTORY(State::SETTINGS, SettingsStateHandler);
            STATE_FACTORY(State::SETTINGS_RSSI, SettingsRssiStateHandler);
            STATE_FACTORY(State::SETTINGS_CUSTOM, SettingsCustomStateHandler);

            default:
                return nullptr;
        }

        #undef STATE_FACTORY
    }

    static void onButtonChange(Button button, Buttons::PressType pressType) {
        if (currentHandler != nullptr) {
            currentHandler->onButtonChange(button, pressType);
        }
    }
}