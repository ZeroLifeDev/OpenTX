#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "HardwareConfig.h"
#include "Theme.h"

// Glass Menu System
#define MENU_ITEMS 4

class ScreenMenu {
private:
    const char* items[MENU_ITEMS] = { "DASHBOARD", "TELEMETRY", "SETTINGS", "ABOUT" };
    int itemCount = MENU_ITEMS;
    int selected = 0;
    
    // Animation
    int curY = 0;
    
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
        
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Glass Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_BG_PANEL);
        sprite->drawFastHLine(0, 30, SCREEN_WIDTH, COLOR_BG_SHADOW);
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("MAIN MENU", SCREEN_WIDTH/2, 15, FONT_HEADER);

        // List
        int startY = 40;
        int h = 28;

        for(int i=0; i<itemCount; i++) {
            int y = startY + (i * h);
            
            if (i == selected) {
                // Active Item: Floating Card
                sprite->fillRoundRect(10, y, SCREEN_WIDTH-20, h-4, 4, COLOR_BG_PANEL);
                sprite->drawRoundRect(10, y, SCREEN_WIDTH-20, h-4, 4, COLOR_ACCENT_2);
                
                // Glow text
                sprite->setTextColor(COLOR_ACCENT_2, COLOR_BG_PANEL);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_LABEL); // Bold
            } else {
                // Inactive
                sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_MAIN);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_BODY);
            }
        }
        
        // Footer (Keys)
        sprite->fillRect(0, SCREEN_HEIGHT-20, SCREEN_WIDTH, 20, COLOR_BG_PANEL);
        sprite->drawFastHLine(0, SCREEN_HEIGHT-20, SCREEN_WIDTH, COLOR_BG_SHADOW);
        
        sprite->setTextColor(COLOR_TEXT_MUTED, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("[SET] OK   [MENU] BACK", SCREEN_WIDTH/2, SCREEN_HEIGHT-10, FONT_MICRO);
    }
};

#endif // SCREEN_MENU_H
