#ifndef STATE_SETTINGS_MODELS_H
#define STATE_SETTINGS_MODELS_H

#include "state.h"

namespace StateMachine {
    class SettingsModelsStateHandler : public StateHandler {
    private:
        enum class SubMenu { LIST, POPUP, EDIT };
        SubMenu currentSubMenu = SubMenu::LIST;
        int selectedListIdx = 0;
        int selectedPopupIdx = 0;
        int editCursorIdx = 0;
        char editBuffer[9];
        
        void cycleChar(char& c, int dir);

    public:
        void onEnter() override;
        void onExit() override;
        void onUpdate() override;
        void onButtonChange(Button button, Buttons::PressType pressType) override;
        void onInitialDraw() override;
        void onUpdateDraw() override;
    };
}

#endif