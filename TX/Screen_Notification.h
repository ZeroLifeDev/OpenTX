#ifndef SCREEN_NOTIFICATION_H
#define SCREEN_NOTIFICATION_H

#include "DisplayManager.h"
#include "Theme.h"
#include "AnimationUtils.h"

class ScreenNotification {
private:
    bool active;
    unsigned long startTime;
    String message;
    String subMessage;
    uint16_t color;
    
    AnimFloat slideY; // Physics slide

public:
    ScreenNotification() : slideY(-40, 0.2f, 0.75f) {} // Start off-screen top

    void init() {
        active = false;
        slideY.reset(-40);
    }

    void show(String msg, String sub, uint16_t c) {
        message = msg;
        subMessage = sub;
        color = c;
        active = true;
        startTime = millis();
        slideY.reset(-40);
        slideY.target = 0;
        
        soundManager.playClick(); // Beep
    }

    void update() {
        if (!active) return;
        
        slideY.update();
        
        if (millis() - startTime > 3000) {
            slideY.target = -40; // Slide out
        }
        
        if (slideY.target == -40 && slideY.val() < -35) {
            active = false;
        }
    }

    bool isActive() { return active; }

    void draw(DisplayManager* display) {
        if (!active && slideY.val() <= -39) return;

        TFT_eSprite* sprite = display->getSprite();
        
        int y = (int)slideY.val();
        
        // Top Banner Style
        sprite->fillRect(0, y, SCREEN_WIDTH, 35, COLOR_BG_HEADER);
        sprite->drawFastHLine(0, y+34, SCREEN_WIDTH, color);
        
        // Icon / Bar
        sprite->fillRect(0, y, 6, 35, color);
        
        // Text
        sprite->setTextDatum(TL_DATUM);
        sprite->setTextColor(color, COLOR_BG_HEADER);
        sprite->drawString(message, 10, y + 4, FONT_MED); // Fixed
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_HEADER);
        sprite->drawString(subMessage, 10, y + 20, FONT_SMALL); // Fixed
    }
};

#endif // SCREEN_NOTIFICATION_H
