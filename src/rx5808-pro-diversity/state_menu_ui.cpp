#include "state.h"
#include "state_menu.h"
#include "ui.h"
#include "ui_menu.h"

// Layout for 3 vertical text boxes
#define BOX_W 120
#define BOX_H 20
#define BOX_X ((SCREEN_WIDTH - BOX_W) / 2)
#define BOX_SPACING 24
#define MENU_START_Y 6 // Centers the 3 items vertically on the 80px screen

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
        // Grab the item using the new const getter
        const Ui::MenuItem* item = this->menu.getItem(i);
        
        int y = MENU_START_Y + (i * BOX_SPACING);

        // Highlight the selected item with Cyan text and a Green border. 
        // Dim the unselected items to Dark Grey.
        uint16_t borderColor = (i == selected) ? TFT_GREEN : TFT_DARKGREY;
        uint16_t textColor = (i == selected) ? TFT_CYAN : TFT_DARKGREY;
        
        // 1. Draw the black background to erase any previous artifacts
        Ui::display.fillRect(BOX_X, y, BOX_W, BOX_H, TFT_BLACK);
        
        // 2. Draw the border
        Ui::display.drawRect(BOX_X, y, BOX_W, BOX_H, borderColor);
        
        // 3. Center the text inside the box
        const uint8_t charLen = strlen(item->text);
        
        // Text size 2 characters are roughly 12 pixels wide (10px char + 2px space)
        int textPixelWidth = charLen * 12; 
        int textX = BOX_X + ((BOX_W - textPixelWidth) / 2);
        int textY = y + 3; // Center the 14px tall text inside the 20px tall box

        Ui::display.setTextSize(2);
        Ui::display.setTextColor(textColor, TFT_BLACK);
        Ui::display.setCursor(textX, textY);
        Ui::display.print(item->text);
    }
}