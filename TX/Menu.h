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
    // Stiffness 0.2, Damping 0.75 -> Nice bounce
    ScreenMenu() : cursorAnim(0, 0.2, 0.75) {} 

    void reset() { 
        selectIdx = 0; 
        cursorAnim.set(0); 
    }
    
    void draw(const char* title, const char* items[], int count) {
        sprite.fillSprite(0x0000); // BLACK
        
        // Update Animation
        cursorAnim.set((float)selectIdx);
        cursorAnim.update();
        
        // Header
        sprite.fillRect(0, 0, 128, 28, 0x18E3); // Dark Grey
        sprite.setTextColor(0xFFFF, 0x18E3);    // WHITE on Grey
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(title, 64, 14, 2);    // Font 2 for Title (Usually safe)
        sprite.drawFastHLine(0, 28, 128, 0xFFFF); 
        
        // List
        int startY = 40;
        int rowH = 26; // Taller rows for Font 4
        
        // Draw Cursor (Springy)
        float curY = startY + (cursorAnim.val() * rowH);
        sprite.fillRoundRect(8, (int)curY, 112, 22, 4, 0x07FF); // Cyan Highlight
        
        for (int i=0; i<count; i++) {
            int y = startY + (i * rowH);
            
            // Text color logic
            if (i == selectIdx) {
                // If it's selected, it overlaps the Cyan box -> Black Text
                sprite.setTextColor(0x0000, 0x07FF); 
            } else {
                // Unselected -> White Text on Black
                sprite.setTextColor(0xFFFF, 0x0000); 
            }
            
            sprite.setTextDatum(MC_DATUM);
            // Using Font 4 (Larger, bold) to ensure visibility
            sprite.drawString(items[i], 64, y + 12, 4);
        }
    }
    
    void nav(bool up, bool down, int count) {
        if (up) selectIdx = (selectIdx - 1 + count) % count;
        if (down) selectIdx = (selectIdx + 1) % count;
    }
    
    int getSelection() { return selectIdx; }
    
    // --- SUB SCREENS ---
    void drawHeader(const char* title) {
        sprite.fillSprite(TFT_BLACK);
        sprite.fillRect(0, 0, 128, 25, 0x18E3);
        sprite.setTextColor(TFT_WHITE, 0x18E3);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(title, 64, 12, 2);
    }

    void drawTelemetry(InputState& s) {
        drawHeader("TELEMETRY");
        
        sprite.setTextColor(TFT_WHITE, TFT_BLACK);
        
        int y = 45;
        // Mock Data
        sprite.setTextDatum(ML_DATUM); sprite.drawString("RX BATT:", 10, y, 2);
        sprite.setTextDatum(MR_DATUM); sprite.drawFloat(4.1, 2, 118, y, 2); y+=20;
        
        sprite.setTextDatum(ML_DATUM); sprite.drawString("TX BATT:", 10, y, 2);
        sprite.setTextDatum(MR_DATUM); sprite.drawFloat(3.9, 2, 118, y, 2); y+=20;
        
        sprite.setTextDatum(ML_DATUM); sprite.drawString("SIGNAL:", 10, y, 2);
        sprite.setTextDatum(MR_DATUM); sprite.drawNumber(-65, 118, y, 2); y+=20;
        
        sprite.setTextDatum(ML_DATUM); sprite.drawString("TEMP:", 10, y, 2);
        sprite.setTextDatum(MR_DATUM); sprite.drawNumber(42, 118, y, 2);

        sprite.setTextDatum(MC_DATUM);
        sprite.setTextColor(0x7BEF, TFT_BLACK); // Grey
        sprite.drawString("[BACK TO EXIT]", 64, 145, 1);
    }
    
    void drawAbout() {
        drawHeader("ABOUT");
        
        sprite.setTextColor(TFT_WHITE, TFT_BLACK);
        sprite.setTextDatum(MC_DATUM);
        
        sprite.drawString("RC OS v2.1", 64, 60, 4);
        sprite.setTextColor(0x07FF, TFT_BLACK);
        sprite.drawString("ANTIGRAVITY", 64, 90, 2);
        
        sprite.setTextColor(0x7BEF, TFT_BLACK);
        sprite.drawString("DESIGNED FOR", 64, 110, 1);
        sprite.drawString("ESP32 / TFT", 64, 120, 1);
    }
    
    void drawMessage(const char* title, const char* msg) {
        drawHeader(title);
        sprite.setTextColor(TFT_WHITE, TFT_BLACK);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(msg, 64, 80, 2);
    }
};

static ScreenMenu menu;

#endif
