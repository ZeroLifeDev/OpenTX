#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "HardwareConfig.h" 
#include <TFT_eSPI.h> 
#include "Theme.h"

// ==========================================
//          ILI9163 DRIVER CORE
// ==========================================

class DisplayManager {
private:
    TFT_eSPI tft;
    TFT_eSprite sprite;

public:
    DisplayManager() : tft(), sprite(&tft) {}

    void init() {
        tft.init();
        
        // --- ILI9163 SPECIFIC FIXES ---
        // 1. Rotation and Offsets
        // ILI9163 128x160 often needs Rotation 0 for Portrait.
        tft.setRotation(0); 
        
        // 2. Color Correction (Red/Blue Swap)
        // Most generic 1.8" displays need SwapBytes enabled for 16-bit color.
        tft.setSwapBytes(true); 
        
        // 3. Black/Red Tab Offset Fix
        // Sometimes needed: tft.writecommand(0x21); // Invert off
        tft.fillScreen(COLOR_BG_MAIN);

        // --- SPRITE BUFFER ---
        sprite.setColorDepth(16);
        sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
        sprite.setSwapBytes(true); // Critical for Sprites too
    }

    TFT_eSprite* getSprite() {
        return &sprite;
    }

    void beginFrame() {
        sprite.fillSprite(COLOR_BG_MAIN);
    }

    void endFrame() {
        sprite.pushSprite(0, 0);
    }
};

// Global Instance
DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
