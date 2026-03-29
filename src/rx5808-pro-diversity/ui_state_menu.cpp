#include "ui.h"
#include "ui_state_menu.h"

using Ui::display;
using Ui::StateMenuHelper;

#define MENU_W 56
#define MENU_X 192 
#define MENU_H 135
#define MENU_ITEM_H 14

// The dedicated memory buffer JUST for the menu
TFT_eSprite menuSprite = TFT_eSprite(&display);
bool menuSpriteCreated = false;

void StateMenuHelper::addItem(const MenuText textFn, const MenuHandler handler) {
    if (this->activeItems < STATE_MENU_ITEMS_MAX) {
        this->menuItems[this->activeItems].textFn = textFn;
        this->menuItems[this->activeItems].handler = handler;
        this->activeItems++;
    }
}

bool StateMenuHelper::handleButtons(Button button, Buttons::PressType pressType) {
    if (button == Button::MODE && pressType == Buttons::PressType::LONG) {
        this->visible = !this->visible;
        Ui::needFullRedraw(); 
        return true;
    }

    if (!this->isVisible()) return false;
    if (pressType != Buttons::PressType::SHORT) return true; 

    switch (button) {
        case Button::UP:
            if (--this->selectedItem < 0) this->selectedItem = this->activeItems - 1;
            break;
        case Button::DOWN:
            if (++this->selectedItem >= this->activeItems) this->selectedItem = 0;
            break;
        case Button::MODE:
            this->menuItems[this->selectedItem].handler(this->state);
            break;
    }
    
    Ui::needUpdate(); 
    return true;
}

void StateMenuHelper::draw() {
    if (!this->isVisible()) return;

    // Draw directly to the screen instead of the sprite
    display.fillRect(MENU_X, 0, MENU_W, MENU_H, TFT_BLACK);
    display.drawFastVLine(MENU_X, 0, MENU_H, TFT_WHITE); 

    const uint8_t yOffset = (MENU_H / 2) - ((this->activeItems * MENU_ITEM_H) / 2);

    for (uint8_t i = 0; i < this->activeItems; i++) {
        uint16_t bgColor = (this->selectedItem == i) ? TFT_WHITE : TFT_BLACK;
        uint16_t fgColor = (this->selectedItem == i) ? TFT_BLACK : TFT_WHITE;

        // Add MENU_X to all X coordinates since we aren't using the sprite's local 0,0 anymore
        display.fillRect(MENU_X + 1, MENU_ITEM_H * i + yOffset, MENU_W - 1, MENU_ITEM_H, bgColor);

        const char* text = this->menuItems[i].textFn(this->state);
        display.setTextSize(1);
        display.setTextColor(fgColor, bgColor);
        
        display.setCursor(MENU_X + 3, MENU_ITEM_H * i + yOffset + 3);
        display.print(text);
    }
}