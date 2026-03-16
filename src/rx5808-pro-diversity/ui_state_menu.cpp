#include "ui.h"
#include "ui_state_menu.h"

using Ui::display;
using Ui::StateMenuHelper;

#define MENU_ITEM_H 14
#define MENU_W 52
#define MENU_TARGET_X (SCREEN_WIDTH - MENU_W) 
#define MENU_H (SCREEN_HEIGHT)

void StateMenuHelper::hide() {
    this->visible = false;
    Ui::needFullRedraw();
}

void StateMenuHelper::addItem(
    const MenuText textFn,
    const MenuHandler handler
) {
    if (this->activeItems < STATE_MENU_ITEMS_MAX) {
        this->menuItems[this->activeItems].textFn = textFn;
        this->menuItems[this->activeItems].handler = handler;
        this->activeItems++;
    }
}

bool StateMenuHelper::handleButtons(
    Button button,
    Buttons::PressType pressType
) {
    if (button == Button::MODE && pressType == Buttons::PressType::LONG) {
        this->visible = !this->visible;
        if (!this->visible)
            Ui::needFullRedraw();

        if (this->visible) {
            this->menuX = SCREEN_WIDTH; // Start entirely off-screen
        }
        return true;
    }

    if (!this->isVisible())
        return false;
        
    if (pressType != Buttons::PressType::SHORT) 
        return true; 

    switch (button) {
        case Button::UP:
            if (--this->selectedItem < 0)
                this->selectedItem = this->activeItems - 1;
            break;

        case Button::DOWN:
            if (++this->selectedItem >= this->activeItems)
                this->selectedItem = 0;
            break;

        case Button::MODE:
            this->menuItems[this->selectedItem].handler(this->state);
            break;
    }
    
    Ui::needUpdate(); 
    return true;
}

void StateMenuHelper::draw() {
    if (!this->isVisible())
        return;

    if (this->menuX > MENU_TARGET_X) {
        this->menuX -= 8; 
        if (this->menuX < MENU_TARGET_X) {
            this->menuX = MENU_TARGET_X; 
        }
    }

    // Fill from the menu's left edge all the way to the right screen edge.
    display.fillRect(this->menuX, 0, SCREEN_WIDTH - this->menuX, MENU_H, TFT_BLACK);
    
    // Draw the white border line
    display.drawFastVLine(this->menuX - 1, 0, MENU_H, TFT_WHITE);

    const uint8_t yOffset =
        SCREEN_HEIGHT_MID - ((this->activeItems * MENU_ITEM_H) / 2);

    // TURN OFF TEXT WRAPPING!
    display.setTextWrap(false);

    for (uint8_t i = 0; i < this->activeItems; i++) {
        uint16_t bgColor = (this->selectedItem == i) ? TFT_WHITE : TFT_BLACK;
        uint16_t fgColor = (this->selectedItem == i) ? TFT_BLACK : TFT_WHITE;

        display.fillRect(this->menuX, MENU_ITEM_H * i + yOffset, MENU_W, MENU_ITEM_H, bgColor);

        const char* text = this->menuItems[i].textFn(this->state);
        display.setTextSize(1);
        display.setTextColor(fgColor, bgColor);
        
        display.setCursor(this->menuX + 2, MENU_ITEM_H * i + yOffset + 3);
        display.print(text);
    }
    
    // Turn it back on so we don't break other screens
    display.setTextWrap(true);
}