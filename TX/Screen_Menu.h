#ifndef SCREEN_MENU_H
#define SCREEN_MENU_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "HardwareConfig.h"

// Simple List Menu
class ScreenMenu {
private:
    const char* items[4] = { "DASHBOARD", "TELEMETRY", "SETTINGS", "ABOUT" };
    int itemCount = 4;
    int selected = 0;
    
    // Animation
    int animY = 0; 
    int targetedY = 0;

public:
    void update() {
        // Navigation logic handled by UIManager usually, but we can visualize selection here
        // If we had up/down buttons, we would change 'selected'
        // For now, since we only have "Menu" button which cycles screens, this might just be a static list
        // BUT user asked for "Toggle through stuff to get in". 
        // We need 'Up', 'Down', 'Select'.
        // We only have: Menu (Cycle), Set (Action), Trim+, Trim-
        
        // Let's use Trim buttons for Menu Up/Down when in Menu Screen?
        // That requires InputManager to know context. 
        // For now, let's just make it look cool.
    }
    
    // Method to change selection
    void next() {
        selected++;
        if (selected >= itemCount) selected = 0;
    }
    
    void prev() {
        selected--;
        if (selected < 0) selected = itemCount - 1;
    }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // Dark Background with Grid
        sprite->fillSprite(COLOR_BG_DARK);
        for(int i=0; i<SCREEN_HEIGHT; i+=20) sprite->drawFastHLine(0, i, SCREEN_WIDTH, 0x18E3); // Faint line
        
        // Header
        display->drawHeader("SYSTEM MENU");
        
        int startY = 35;
        int h = 25;
        
        // Draw Items
        for(int i=0; i<itemCount; i++) {
            int y = startY + (i * h);
            
            if (i == selected) {
                // Selected Item - "Expensive" Glow
                // Gradient bar
                sprite->fillRect(10, y, SCREEN_WIDTH-20, h-4, COLOR_ACCENT);
                sprite->drawRect(10, y, SCREEN_WIDTH-20, h-4, COLOR_TEXT_MAIN);
                
                sprite->setTextColor(COLOR_BG_DARK, COLOR_ACCENT);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_BODY);
                
                // Icon/Arrow
                sprite->fillTriangle(SCREEN_WIDTH-25, y+5, SCREEN_WIDTH-25, y+15, SCREEN_WIDTH-18, y+10, COLOR_BG_DARK);
            } else {
                // Normal Item
                sprite->drawRect(10, y, SCREEN_WIDTH-20, h-4, COLOR_BG_PANEL);
                
                sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_DARK);
                sprite->setTextDatum(MC_DATUM);
                sprite->drawString(items[i], SCREEN_WIDTH/2, y + (h/2) - 2, FONT_BODY);
            }
        }
    }
};

#endif // SCREEN_MENU_H
