#ifndef SCREEN_INTRO_H
#define SCREEN_INTRO_H

#include "DisplayManager.h"
#include "Theme.h"
#include "AnimationUtils.h"

class ScreenIntro {
private:
    AnimFloat logoY;
    AnimFloat lineW;
    unsigned long startTime;
    bool complete;

public:
    ScreenIntro() : logoY(0, 0.1f, 0.8f), lineW(0, 0.15f, 0.8f) {}

    void init() {
        startTime = millis();
        complete = false;
        logoY.reset(0);
        logoY.target = 70;
        lineW.reset(0);
        lineW.target = 0;
    }

    void update() {
        if (complete) return;
        
        logoY.update();
        lineW.update();
        
        unsigned long elapsed = millis() - startTime;
        
        if (elapsed > 500) lineW.target = 100;
        if (elapsed > 2000) complete = true;
    }

    bool isFinished() { return complete; }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Grid (Subtle)
        for(int i=0; i<SCREEN_HEIGHT; i+=20) {
             sprite->drawFastHLine(0, i, SCREEN_WIDTH, COLOR_BG_PANEL); // Fixed: COLOR_BG_SHADOW -> COLOR_BG_PANEL
        }
        
        int cy = (int)logoY.val();
        int cx = SCREEN_WIDTH / 2;
        
        // Logo Text
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("OpenTX", cx, cy, FONT_LARGE); // Fixed: FONT_HEADER -> FONT_LARGE
        
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->drawString("PRO V3", cx, cy + 20, FONT_SMALL); // Fixed

        // Loading Bar
        int w = (int)lineW.val();
        if (w > 0) {
            sprite->fillRect(cx - w/2, cy + 40, w, 4, COLOR_ACCENT_PRI); // Fixed: COLOR_ACCENT_2 -> PRI
        }
        
        // Tagline
        if (millis() % 1000 < 500) {
            sprite->setTextColor(COLOR_ACCENT_TER, COLOR_BG_MAIN); // Fixed: ACCENT_3 -> TER
            sprite->drawString("SYSTEM READY", cx, cy + 55, FONT_SMALL); // Fixed: FONT_MICRO -> SMALL
        }
    }
};

#endif // SCREEN_INTRO_H
