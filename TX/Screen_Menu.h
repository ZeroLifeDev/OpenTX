#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "HardwareConfig.h"
#include "Theme.h"

// Clean Functional Menu
#define MENU_ITEMS 4

class ScreenMenu {
private:
    const char* items[MENU_ITEMS] = { "DASHBOARD", "TELEMETRY", "SETTINGS", "ABOUT" };
    int itemCount = MENU_ITEMS;
    int selected = 0;
    
public:
    void init() {
         selected = 0;
    }

    void update() {}
    
    void next() {
        selected++;
        if (selected >= MENU_ITEMS) selected = 0;
        // Animation trigger could go here
    }
    
    void prev() {
        selected--;
        if (selected < 0) selected = MENU_ITEMS - 1;
    }
    
    int getSelection() { return selected; }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // 1. Full Blue Background
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // 2. Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_PANEL);
        sprite->drawFastHLine(0, 20, SCREEN_WIDTH, COLOR_TEXT_MAIN); // White line separator
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("OPTIONS", SCREEN_WIDTH/2, 11, FONT_LABEL);

        // 3. Items Loop
        int startY = 35;
        int h = 26;

        for(int i=0; i<itemCount; i++) {
            int y = startY + (i * h);
            
            if (i == selected) {
                // FIXED: Active Item = Solid White Box with Blue Text
                // "No Black Box"
                sprite->fillRoundRect(10, y, SCREEN_WIDTH-20, h-4, 4, COLOR_TEXT_MAIN);
                
                sprite->setTextColor(COLOR_BG_MAIN, COLOR_TEXT_MAIN); // Inverted text
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_LABEL);
            } else {
                // Inactive = Transparent/Blue with White Text
                sprite->drawRoundRect(10, y, SCREEN_WIDTH-20, h-4, 4, COLOR_TEXT_MUTED); // Subtle border
                
                sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_MAIN);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_BODY);
            }
        }
        
        // 4. Footer
        sprite->drawFastHLine(0, SCREEN_HEIGHT-18, SCREEN_WIDTH, COLOR_TEXT_MUTED);
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("[SET] SELECT    [MENU] EXIT", SCREEN_WIDTH/2, SCREEN_HEIGHT-8, FONT_MICRO);
    }
};

#endif // SCREEN_MENU_H
