#ifndef SCREEN_INTRO_H
#define SCREEN_INTRO_H

#include "DisplayManager.h"
#include "Theme.h"
#include "SoundManager.h"
#include "GraphicsUtils.h"

// Cinematic Boot Sequence
class ScreenIntro {
private:
    unsigned long startTime;
    bool complete;
    int stage; // 0=Text, 1=Bars, 2=Logo

public:
    void init() {
        startTime = millis();
        complete = false;
        stage = 0;
    }

    void update() {
        if (complete) return;
        unsigned long t = millis() - startTime;
        
        // Single fluid stage
        if (t > 2500) complete = true;
    }

    bool isFinished() { return complete; }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // Cyber Gradient & Grid
        GraphicsUtils::fillGradientRect(sprite, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG_MAIN, COLOR_BG_DIM);
        GraphicsUtils::drawTechGrid(sprite);
        
        unsigned long t = millis() - startTime;

        // 1. LOGO ANIMATION (0 - 2000ms)
        // Center huge
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN); 
        sprite->setTextDatum(MC_DATUM);
        
        sprite->drawString("OpenTX", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 20, FONT_LARGE);
        
        sprite->setTextColor(COLOR_ACCENT_PRI); // Cyan Pro
        sprite->drawString("PRO", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 10, FONT_MED);

        // 2. PROGRESS BAR (Bottom)
        int barW = 80;
        int barH = 6;
        int bx = (SCREEN_WIDTH - barW) / 2;
        int by = SCREEN_HEIGHT - 40;
        
        float progress = (float)t / 2000.0f;
        if (progress > 1.0f) progress = 1.0f;
        
        // Detailed Tech Bar
        sprite->drawRect(bx-2, by-2, barW+4, barH+4, COLOR_BORDER);
        GraphicsUtils::drawProgressBar(sprite, bx, by, barW, barH, progress, COLOR_ACCENT_TER, COLOR_BG_PANEL); // Green bar
        
        // Version
        sprite->setTextColor(COLOR_TEXT_DIM);
        sprite->setTextDatum(BC_DATUM);
        sprite->drawString("v5.0 Commercial", SCREEN_WIDTH/2, SCREEN_HEIGHT - 10, FONT_SMALL);
    }
};

#endif // SCREEN_INTRO_H
