#ifndef SCREEN_POPUP_H
#define SCREEN_POPUP_H

#include "DisplayManager.h"
#include "Theme.h"

// Generic Pop-up Screen for notifications / cutscenes
class ScreenPopup {
private:
    unsigned long startTime;
    String message;
    String subMessage;
    int duration;
    bool isActive;
    uint16_t color;

public:
    void show(const char* msg, const char* sub, uint16_t col) {
        message = msg;
        subMessage = sub;
        color = col;
        startTime = millis();
        isActive = true;
    }
    
    void hide() { isActive = false; }
    
    bool isFinished() {
        return (millis() - startTime > duration);
    }
    
    void draw(DisplayManager* display) {
        if (!isActive) return;
        
        TFT_eSprite* sprite = display->getSprite();
        
        long elapsed = millis() - startTime;
        
        // "Among Us" Style:
        // 1. Full Screen Tint/Overlay? No, just heavy bars.
        // 2. Animate Bars entering from sides? 
        // Let's do a simple expanding center bar.
        
        int h = 50;
        int y = (SCREEN_HEIGHT - h) / 2;
        
         // Animation: Expand Height
        if (elapsed < 100) h = map(elapsed, 0, 100, 0, 50);
        
        // Black Bar Background
        sprite->fillRect(0, y, SCREEN_WIDTH, h, COLOR_BG_DARK);
        
        // Borders (Top/Bottom)
        sprite->drawFastHLine(0, y, SCREEN_WIDTH, color);
        sprite->drawFastHLine(0, y+h-1, SCREEN_WIDTH, color);
        
        // Strobe Effect for Text
        bool showText = true;
        if (elapsed < 300 && (elapsed / 50) % 2 == 0) showText = false;
        
        if (showText) {
            sprite->setTextColor(color, COLOR_BG_DARK);
            sprite->setTextDatum(MC_DATUM);
            sprite->drawString(message, SCREEN_WIDTH/2, y + 15, FONT_BODY);
            
            sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_DARK);
            sprite->drawString(subMessage, SCREEN_WIDTH/2, y + 35, FONT_SMALL);
        }
    }
    
    bool isVisible() {
        return isActive;
    }
};

// Global Instance
ScreenPopup screenPopup;

#endif // SCREEN_POPUP_H
