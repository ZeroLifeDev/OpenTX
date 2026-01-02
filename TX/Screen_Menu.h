#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "DisplayManager.h"
#include "Theme.h"

// Premium Scrolling Menu V3
#define MENU_ITEMS 4

class ScreenMenu {
private:
    const char* items[MENU_ITEMS] = { "DASHBOARD", "TELEMETRY", "SETTINGS", "ABOUT" };
    int selected = 0;
    
public:
    void init() { selected = 0; }
    
    void next() {
        selected++;
        if (selected >= MENU_ITEMS) selected = 0;
    }
    
    void prev() {
        selected--;
        if (selected < 0) selected = MENU_ITEMS - 1;
    }
    
    int getSelection() { return selected; }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 25, COLOR_BG_HEADER);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_HEADER);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("MAIN MENU", SCREEN_WIDTH/2, 12, FONT_MED);
        
        // Dynamic List
        int startY = 35;
        int itemH = 30;
        
        for (int i=0; i<MENU_ITEMS; i++) {
            int y = startY + (i*itemH);
            
            if (i == selected) {
                // Glow Effect
                sprite->fillRoundRect(5, y, SCREEN_WIDTH-10, 24, 4, COLOR_ACCENT_PRI);
                sprite->setTextColor(COLOR_BG_MAIN, COLOR_ACCENT_PRI); // Inverted
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y+12, FONT_MED);
            } else {
                sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y+12, FONT_SMALL); // Smaller font for non-selected
            }
        }
    }
};
#endif // SCREEN_MENU_H
