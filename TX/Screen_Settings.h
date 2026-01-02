#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

// Adjustable Settings System
class ScreenSettings {
private:
    const char* options[3] = { "SOUND", "BACKLIGHT", "INVERT GYRO" };
    bool values[3] = { true, true, false }; // Simulated constraints
    int selected = 0;
    
public:
    void init() { selected = 0; }
    
    void next() {
        selected++;
        if (selected > 2) selected = 0;
    }
    
    void toggle() {
        values[selected] = !values[selected];
    }
    
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 25, COLOR_BG_HEADER);
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_HEADER);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("SYS CONFIG", SCREEN_WIDTH/2, 12, FONT_MED);
        
        // List
        int startY = 35;
        for(int i=0; i<3; i++) {
            int y = startY + (i*35);
            
            // Box
            if (i==selected) {
                 sprite->drawRect(5, y, SCREEN_WIDTH-10, 30, COLOR_ACCENT_PRI);
                 sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
            } else {
                 sprite->drawRect(5, y, SCREEN_WIDTH-10, 30, COLOR_BG_PANEL);
                 sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
            }
            
            // Label
            sprite->setTextDatum(ML_DATUM);
            sprite->drawString(options[i], 12, y + 15, FONT_SMALL);
            
            // Value / Toggle
            sprite->setTextDatum(MR_DATUM);
            uint16_t valCol = values[i] ? COLOR_OK : COLOR_WARN;
            sprite->setTextColor(valCol, COLOR_BG_MAIN);
            sprite->drawString(values[i] ? "ON" : "OFF", SCREEN_WIDTH-12, y + 15, FONT_MED);
        }
        
        // Footer
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->setTextDatum(BC_DATUM);
        sprite->drawString("[SET] TOGGLE", SCREEN_WIDTH/2, SCREEN_HEIGHT-5, FONT_SMALL);
    }
};
#endif // SCREEN_SETTINGS_H
