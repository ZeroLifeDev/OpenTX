#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Types.h"
#include "Theme.h"

extern TFT_eSprite sprite;

const char* menuMainItems[] = {"SETTINGS", "TELEMETRY", "ABOUT", "EXIT"};
const int menuMainCount = 4;

const char* menuSettingsItems[] = {"CALIBRATE", "TRIMS", "EXPO", "EPA", "BACK"};
const int menuSettingsCount = 5;

class ScreenMenu {
private:
    int selectIdx = 0;
public:
    void reset() { selectIdx = 0; }
    
    void draw(const char* title, const char* items[], int count) {
        sprite.fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite.fillRect(0, 0, 128, 25, COLOR_BG_PANEL);
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(title, 64, 12, 2);
        sprite.drawFastHLine(0, 25, 128, COLOR_TEXT_DIM); // Underline
        
        // List
        int startY = 35;
        int rowH = 22;
        
        for (int i=0; i<count; i++) {
            int y = startY + (i * rowH);
            
            if (i == selectIdx) {
                // Selected Item: Cyan Box, Black Text
                sprite.fillRoundRect(5, y, 118, 20, 4, COLOR_ACCENT_PRI);
                sprite.setTextColor(COLOR_BG_MAIN, COLOR_ACCENT_PRI); 
            } else {
                // Normal Item: White Text
                sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
            }
            
            sprite.setTextDatum(MC_DATUM);
            sprite.drawString(items[i], 64, y + 10, 2);
        }
    }
    
    void nav(bool up, bool down, int count) {
        if (up) selectIdx = (selectIdx - 1 + count) % count;
        if (down) selectIdx = (selectIdx + 1) % count;
    }
    
    int getSelection() { return selectIdx; }
};

static ScreenMenu menu;

#endif
