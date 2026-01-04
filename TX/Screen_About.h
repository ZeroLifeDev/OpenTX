#ifndef SCREEN_ABOUT_H
#define SCREEN_ABOUT_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Types.h"
#include "Theme.h"

extern TFT_eSprite sprite;

class ScreenAbout {
public:
    void draw() {
        sprite.fillSprite(TFT_BLACK);
        
        // Logo / Title
        sprite.setTextColor(0x07FF, TFT_BLACK); // Cyan
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("ANTIGRAVITY", 64, 40, 2);
        
        sprite.setTextColor(TFT_WHITE, TFT_BLACK);
        sprite.drawString("RC SYSTEM", 64, 60, 4);
        
        // Info
        sprite.setTextColor(0x7BEF, TFT_BLACK);
        sprite.drawString("FIRMWARE v2.5", 64, 100, 2);
        sprite.drawString("2026 EDITION", 64, 120, 1);
        
        // Footer
        sprite.fillRect(0, 140, 128, 20, 0x2104);
        sprite.setTextColor(TFT_WHITE, 0x2104);
        sprite.drawString("OK to BACK", 64, 150, 1);
    }
};

static ScreenAbout screenAbout;

#endif
