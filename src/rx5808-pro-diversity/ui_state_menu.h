#ifndef UI_STATE_MENU_H
#define UI_STATE_MENU_H

#include <stdint.h>
#include "buttons.h"
#include "ui.h"

#define STATE_MENU_ITEMS_MAX 6

namespace Ui {
    class StateMenuHelper {
        public:
            typedef const char* (*MenuText)(void* state);
            typedef void (*MenuHandler)(void* state);

            struct StateMenuItem {
                MenuText textFn = nullptr;
                MenuHandler handler = nullptr;
            };

            StateMenuHelper(void* state) { this->state = state; }
            void draw();
            void draw(GFXcanvas16* canvas);
            bool handleButtons(Button button, Buttons::PressType pressType);
            bool isVisible() { return this->visible; }
            
            void hide() { 
                this->visible = false; 
                Ui::needFullRedraw(); 
            }

            void addItem(
                const MenuText textFn,
                const MenuHandler handler
            );

        private:
            StateMenuItem menuItems[STATE_MENU_ITEMS_MAX];

            void *state = nullptr;
            int activeItems = 0;
            int selectedItem = 0;
            bool visible = false;
            
            // NO SLIDING VARIABLES ALLOWED HERE
    };
}

#endif