#include "state.h"
#include "state_menu.h"
#include "ui.h"
#include "ui_menu.h"

// Map the missing TFT_eSPI color
#ifndef TFT_DARKGREY
#define TFT_DARKGREY 0x7BEF
#endif

// Layout for 3 vertical text boxes
#define BOX_W 120
#define BOX_H 20
#define BOX_X ((SCREEN_WIDTH - BOX_W) / 2)
#define BOX_SPACING 24
#define MENU_START_Y 6 

void StateMachine::MenuStateHandler::onInitialDraw() {
    Ui::clear();
    drawMenuEntry();
    Ui::needDisplay();
}

void StateMachine::MenuStateHandler::onUpdateDraw() {
    drawMenuEntry();
    Ui::needDisplay();
}

void StateMachine::MenuStateHandler::drawMenuEntry() {
    int active = this->menu.getActiveItems();
    int selected = this->menu.getSelectedItem();

    for (int i = 0; i < active; i++) {
        const Ui::MenuItem* item = this->menu.getItem(i);
        
        int y = MENU_START_Y + (i * BOX_SPACING);

        uint16_t borderColor = (i == selected) ? TFT_GREEN : TFT_DARKGREY;
        uint16_t textColor = (i == selected) ? TFT_CYAN : TFT_DARKGREY;
        
        Ui::display.fillRect(BOX_X, y, BOX_W, BOX_H, TFT_BLACK);
        Ui::display.drawRect(BOX_X, y, BOX_W, BOX_H, borderColor);
        
        const uint8_t charLen = strlen(item->text);
        int textPixelWidth = charLen * 12; 
        int textX = BOX_X + ((BOX_W - textPixelWidth) / 2);
        int textY = y + 3; 

        Ui::display.setTextSize(2);
        Ui::display.setTextColor(textColor, TFT_BLACK);
        Ui::display.setCursor(textX, textY);
        Ui::display.print(item->text);
    }
}