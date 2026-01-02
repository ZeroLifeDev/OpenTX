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
    void show(String msg, String subMsg, int ms, uint16_t c = COLOR_ACCENT) {
        message = msg;
        subMessage = subMsg;
        duration = ms;
        color = c;
        startTime = millis();
        isActive = true;
    }

    void update() {
        if (!isActive) return;
        if (millis() - startTime > duration) {
            isActive = false;
        }
    }

    void draw(DisplayManager* display) {
        if (!isActive) return;
        
        GFXcanvas16* sprite = display->getCanvas();
        
        // Compute overlay rect (centered)
        int w = SCREEN_WIDTH - 20;
        int h = 60;
        int x = 10;
        int y = (SCREEN_HEIGHT - h) / 2;
        
        // Draw Box with Animation (Pop in effect simulated by just drawing for now)
        sprite->fillRoundRect(x, y, w, h, 6, COLOR_BG_PANEL);
        sprite->drawRoundRect(x, y, w, h, 6, color);
        
        // Text
        sprite->setTextColor(color);
        display->setTextCentered(message, SCREEN_WIDTH/2, y + 20, 1);
        
        sprite->setTextColor(COLOR_TEXT_MAIN);
        display->setTextCentered(subMessage, SCREEN_WIDTH/2, y + 40, 1);
        
        // Progress bar for timeout?
        int timeParams = millis() - startTime;
        int barW = map(timeParams, 0, duration, 0, w - 10);
        sprite->fillRect(x+5, y+h-4, w-10, 2, COLOR_BG_DARK);
        sprite->fillRect(x+5, y+h-4, barW, 2, color);
    }
    
    bool isVisible() {
        return isActive;
    }
};

// Global Instance
ScreenPopup screenPopup;

#endif // SCREEN_POPUP_H
