#ifndef SCREEN_POPUP_H
#define SCREEN_POPUP_H

#include "DisplayManager.h"
#include "Theme.h"

// Generic Pop-up Screen
class ScreenPopup {
private:
    unsigned long startTime;
    String message;
    String subMessage;
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
    
    bool isFinished() { return (millis() - startTime > 1500); }
    
    void draw(DisplayManager* display) {
        if (!isActive) return;
        
        TFT_eSprite* sprite = display->getSprite();
        
        // Glass Overlay Panel
        int w = SCREEN_WIDTH - 20;
        int h = 60;
        int x = 10;
        int y = (SCREEN_HEIGHT - h) / 2;
        
        // Drop Shadow
        sprite->fillRect(x+2, y+2, w, h, COLOR_TEXT_SUB);
        // Main Panel
        sprite->fillRect(x, y, w, h, COLOR_BG_PANEL);
        sprite->drawRect(x, y, w, h, color); // Color Border
        
        // Header Strip
        sprite->fillRect(x, y, w, 15, color);
        
        // Text
        sprite->setTextColor(COLOR_BG_PANEL, color);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("ALERT", SCREEN_WIDTH/2, y + 8, FONT_MICRO);
        
        sprite->setTextColor(color, COLOR_BG_PANEL);
        sprite->drawString(message, SCREEN_WIDTH/2, y + 28, FONT_LABEL);
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->drawString(subMessage, SCREEN_WIDTH/2, y + 48, FONT_SMALL);
    }
    
    bool isVisible() { return isActive; }
};

extern ScreenPopup screenPopup;

#endif // SCREEN_POPUP_H
