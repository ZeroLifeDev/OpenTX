#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "HardwareConfig.h" 
#include <TFT_eSPI.h> 
#include <FS.h>
#include <LittleFS.h>
#include "Theme.h"

// ==========================================
//          ILI9163 DRIVER CORE
// ==========================================

class DisplayManager {
private:
    TFT_eSPI tft;
    TFT_eSprite sprite;
    bool fsOk;

public:
    DisplayManager() : tft(), sprite(&tft), fsOk(false) {}

    void init() {
        // 1. Init File System for Fonts
        if (LittleFS.begin()) {
            fsOk = true;
            Serial.println("LittleFS Mounted");
        } else {
            Serial.println("LittleFS Failed");
        }

        tft.init();
        tft.setRotation(0); 
        tft.setSwapBytes(true); 
        tft.fillScreen(COLOR_BG_MAIN);

        // --- SPRITE BUFFER ---
        sprite.setColorDepth(16);
        sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
        sprite.setSwapBytes(true); 
    }

    TFT_eSprite* getSprite() {
        return &sprite;
    }
    
    // Custom Font Helpers
    void loadFont(String name) {
        if (fsOk) sprite.loadFont(name);
    }
    
    void unloadFont() {
        if (fsOk) sprite.unloadFont();
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
