#ifndef UI_MENU_H
#define UI_MENU_H

#include "ui.h"

#define MAIN_MENU_ITEMS_MAX 4

namespace Ui {
    typedef void(*MenuHandler)();

    struct MenuItem {
        const char* text = nullptr;
        Ui::MenuHandler handler = nullptr;
        const unsigned char* icon = nullptr;
    };

    class MenuHelper {
        public:
            void reset();
            void addItem(
                const char* text,
                const unsigned char* icon,
                const MenuHandler handler
            );

            MenuItem* getCurrentItem();

            // --- NEW GETTERS FOR TFT REDESIGN ---
            int getActiveItems() const { return activeItems; }
            int getSelectedItem() const { return selectedItem; }
            const MenuItem* getItem(int index) const { return &menuItems[index]; }

            void selectNextItem();
            void selectPreviousItem();
            void activateItem();

        private:
            Ui::MenuItem menuItems[MAIN_MENU_ITEMS_MAX];

            int activeItems = 0;
            int selectedItem = 0;
    };
}

#endif