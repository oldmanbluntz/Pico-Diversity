#include "ui.h"
#include "ui_state_menu.h"

using Ui::display;
using Ui::StateMenuHelper;

#define MENU_W 52
#define MENU_X 108 
#define MENU_H 80
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

    // 1. Allocate the tiny RAM buffer on the first open
    if (!menuSpriteCreated) {
        menuSprite.setColorDepth(16); // <--- ADD THIS LINE HERE
        menuSprite.createSprite(MENU_W, MENU_H);
        menuSpriteCreated = true;
    }

    // 2. Build the menu inside the invisible memory buffer
    menuSprite.fillSprite(TFT_BLACK);
    menuSprite.drawFastVLine(0, 0, MENU_H, TFT_WHITE);

    const uint8_t yOffset = (MENU_H / 2) - ((this->activeItems * MENU_ITEM_H) / 2);

    for (uint8_t i = 0; i < this->activeItems; i++) {
        uint16_t bgColor = (this->selectedItem == i) ? TFT_WHITE : TFT_BLACK;
        uint16_t fgColor = (this->selectedItem == i) ? TFT_BLACK : TFT_WHITE;

        menuSprite.fillRect(1, MENU_ITEM_H * i + yOffset, MENU_W - 1, MENU_ITEM_H, bgColor);

        const char* text = this->menuItems[i].textFn(this->state);
        menuSprite.setTextSize(1);
        menuSprite.setTextColor(fgColor, bgColor);
        
        menuSprite.setCursor(3, MENU_ITEM_H * i + yOffset + 3);
        menuSprite.print(text);
    }
    
    // 3. Blast the fully completed image to the TFT hardware instantly
    menuSprite.pushSprite(MENU_X, 0);
}