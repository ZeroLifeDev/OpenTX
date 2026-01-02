#ifndef SCREEN_INTRO_H
#define SCREEN_INTRO_H

#include "DisplayManager.h"
#include "Theme.h"
#include "SoundManager.h"

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
        
        // Stage Logic
        if (stage == 0 && t > 1200) stage = 1;
        if (stage == 1 && t > 2500) stage = 2;
        if (stage == 2 && t > 4000) complete = true;
    }

    bool isFinished() { return complete; }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        sprite->fillSprite(COLOR_BG_MAIN);
        
        unsigned long t = millis() - startTime;

        // Stage 0: Matrix Text Scroll
        if (stage == 0) {
            sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
            sprite->setTextDatum(TL_DATUM);
            int lines = (t / 100); // New line every 100ms
            
            int y = 5;
            if (lines > 0) sprite->drawString("> BIOS CHECK...", 5, y, FONT_SMALL);
            if (lines > 1) sprite->drawString("> MEMORY OK", 5, y+10, FONT_SMALL);
            if (lines > 2) sprite->drawString("> IO PROTOCOL...", 5, y+20, FONT_SMALL);
            if (lines > 3) sprite->drawString("> LINKING...", 5, y+30, FONT_SMALL);
            if (lines > 4) sprite->drawString("> DRIVERS LOADED", 5, y+40, FONT_SMALL);
            if (lines > 5) sprite->drawString("> SYSTEM READY", 5, y+50, FONT_SMALL);
            
            // Blink cursor
            if ((t % 200) < 100) sprite->fillRect(5, y + (lines > 5 ? 60 : (lines+1)*10), 6, 8, COLOR_ACCENT_PRI);
        }
        
        // Stage 1: Loading Bars
        if (stage == 1) {
            sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
            sprite->setTextDatum(MC_DATUM);
            sprite->drawString("LOADING MODULES", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 20, FONT_SMALL);
            
            int progress = map(t, 1200, 2500, 0, 100);
            int barW = 100;
            int barH = 6;
            
            // Frame
            sprite->drawRect((SCREEN_WIDTH-barW)/2, SCREEN_HEIGHT/2, barW, barH, COLOR_BG_PANEL);
            // Fill
            int fill = map(progress, 0, 100, 0, barW-2);
            sprite->fillRect((SCREEN_WIDTH-barW)/2 + 1, SCREEN_HEIGHT/2 + 1, fill, barH-2, COLOR_ACCENT_PRI);
            
            // Rapid random numbers
            sprite->drawString(String(random(1000,9999)), SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 20, FONT_SMALL);
        }
        
        // Stage 2: Logo Success
        if (stage == 2) {
            sprite->setTextColor(COLOR_TEXT_GLOW, COLOR_BG_MAIN);
            sprite->setTextDatum(MC_DATUM);
            sprite->drawString("OpenTX", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 10, FONT_LARGE);
            
            sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
            sprite->drawString("PRO V4", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 15, FONT_MED);
        }
    }
};

#endif // SCREEN_INTRO_H
