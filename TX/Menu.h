#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Types.h"
#include "Theme.h"

extern TFT_eSprite sprite;

const char* menuMainItems[] = {"SETTINGS", "TELEMETRY", "ABOUT", "EXIT"};
const int menuMainCount = 4;

const char* menuSettingsItems[] = {"CALIBRATE", "TRIMS", "EXPO", "EPA", "REVERSE", "SOUND", "BACK"};
const int menuSettingsCount = 7;

class ScreenMenu {
private:
    int selectIdx = 0;
    AnimFloat cursorAnim;
    
public:
    // Snappy spring
    ScreenMenu() : cursorAnim(0, 0.2, 0.6) {} 

    void reset() { 
        selectIdx = 0; 
        cursorAnim.set(0); 
    }
    
    void draw(const char* title, const char* items[], int count) {
        sprite.fillSprite(TFT_BLACK); 
        
        // Anim logic
        cursorAnim.set((float)selectIdx);
        cursorAnim.update();
        
        // Header (Blue bar)
        sprite.fillRect(0, 0, 128, 30, 0x001F); // Blue
        sprite.setTextColor(TFT_WHITE, 0x001F);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(title, 64, 15, 2); 
        
        // Cursor (White Block)
        int startY = 35;
        int rowH = 24;
        int curY = startY + (cursorAnim.val() * rowH);
        sprite.fillRoundRect(5, curY, 118, 20, 6, TFT_WHITE); // White Highlight
        
        // List Items
        for (int i=0; i<count; i++) {
            int y = startY + (i * rowH);
            
            if (abs((float)i - cursorAnim.val()) < 0.5) {
                 // Selected: Black Text (on White block)
                 sprite.setTextColor(TFT_BLACK, TFT_WHITE);
            } else {
                 // Normal: White Text (on Black BG)
                 sprite.setTextColor(TFT_WHITE, TFT_BLACK);
            }
            
            sprite.setTextDatum(ML_DATUM); // Left Align for list
            sprite.drawString(items[i], 20, y + 10, 2); // Font 2 is standard
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
