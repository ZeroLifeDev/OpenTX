#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "HardwareConfig.h"
#include "Theme.h"

// Cyber-Noir Menu System
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

    void update() {
        // Controlled by UIManager
    }
    
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
        
        sprite->fillSprite(COLOR_BG_MAIN); // Deep Black
        
        // Cyber Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 22, COLOR_BG_PANEL);
        sprite->drawFastHLine(0, 22, SCREEN_WIDTH, COLOR_ACCENT_4); // Hot Pink Line
        
        sprite->setTextColor(COLOR_ACCENT_3, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("SYSTEM MENU", SCREEN_WIDTH/2, 11, FONT_LABEL);

        // List
        int startY = 32;
        int h = 28;

        for(int i=0; i<itemCount; i++) {
            int y = startY + (i * h);
            
            if (i == selected) {
                // Active Item: Glowing Box
                sprite->fillRect(5, y, SCREEN_WIDTH-10, h-4, COLOR_BG_PANEL);
                sprite->drawRect(5, y, SCREEN_WIDTH-10, h-4, COLOR_ACCENT_4);
                
                // Tech decor
                sprite->fillRect(5, y, 4, h-4, COLOR_ACCENT_4);
                
                // Glow text
                sprite->setTextColor(COLOR_ACCENT_4, COLOR_BG_PANEL);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_LABEL);
            } else {
                // Inactive
                sprite->drawRect(5, y, SCREEN_WIDTH-10, h-4, COLOR_BG_SHADOW);
                
                sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_MAIN);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_BODY);
            }
        }
        
        // Footer (Keys)
        sprite->setTextColor(COLOR_TEXT_MUTED, COLOR_BG_MAIN);
        sprite->setTextDatum(BC_DATUM);
        sprite->drawString("[SET] SELECT   [MENU] EXIT", SCREEN_WIDTH/2, SCREEN_HEIGHT-4, FONT_MICRO);
    }
};

#endif // SCREEN_MENU_H
