#ifndef SCREEN_INTRO_H
#define SCREEN_INTRO_H

#include "DisplayManager.h"
#include "Theme.h"
#include "AnimationUtils.h"

class ScreenIntro {
private:
    unsigned long startTime;
    bool isComplete;
    
    // Animations
    AnimFloat logoY;
    AnimFloat lineW;
    
public:
    ScreenIntro() : logoY(SCREEN_HEIGHT, 0.1f, 0.7f), lineW(0, 0.2f, 0.9f) {}

    void init() {
        startTime = millis();
        isComplete = false;
        logoY.snap(SCREEN_HEIGHT + 20); // Start below
        logoY.target = SCREEN_HEIGHT/2 - 10; // Move to center
        
        lineW.snap(0);
        lineW.target = SCREEN_WIDTH - 60;
    }

    void update() {
        if (isComplete) return;
        unsigned long elapsed = millis() - startTime;
        
        logoY.update();
        lineW.update();
        
        if (elapsed > 3000) {
            isComplete = true;
        }
    }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        sprite->fillSprite(COLOR_BG_MAIN); // Light Theme
        
        // Grid Background (Subtle)
        for (int i=0; i<SCREEN_HEIGHT; i+=10) {
             if (i % 20 == 0) sprite->drawFastHLine(0, i, SCREEN_WIDTH, COLOR_BG_SHADOW);
        }

        int cy = (int)logoY.val();
        int cx = SCREEN_WIDTH/2;
        
        // Main Logo
        sprite->setTextDatum(MC_DATUM);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->drawString("OpenTX", cx, cy, FONT_HEADER);
        
        // Tagline (Flashing)
        if (millis() % 400 < 200) {
            sprite->setTextColor(COLOR_ACCENT_3, COLOR_BG_MAIN);
            sprite->drawString("SYSTEM READY", cx, cy + 25, FONT_MICRO);
        }
        
        // Loading Bars (Expanding)
        int w = (int)lineW.val();
        sprite->fillRect(cx - w/2, cy + 40, w, 4, COLOR_ACCENT_2);
    }

    bool isFinished() {
        return isComplete;
    }
};

extern ScreenIntro screenIntro;

#endif // SCREEN_INTRO_H
