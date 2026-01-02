#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include "HardwareConfig.h"
#include "Theme.h"

class DisplayManager {
private:
    TFT_eSPI tft;
    TFT_eSprite sprite;

public:
    DisplayManager() : tft(), sprite(&tft) {}

    void init() {
        tft.init();
        tft.setRotation(0); // Portrait
        tft.fillScreen(COLOR_BG_DARK);
        tft.setSwapBytes(true); // Important for correct colors with sprites

        // Create a full-screen sprite for double buffering
        // 160x128 might be too large for RAM on some ESP32s with heavy wifi usage, 
        // but usually okay. If it crashes, we can use partial sprites.
        sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    TFT_eSprite* getSprite() {
        return &sprite;
    }

    void beginFrame() {
        sprite.fillSprite(COLOR_BG_DARK);
    }

    void endFrame() {
        sprite.pushSprite(0, 0);
    }
    
    // Helper: Draw a "Commercial" style header
    void drawHeader(const char* title) {
        sprite.fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_PANEL);
        sprite.drawFastHLine(0, 20, SCREEN_WIDTH, COLOR_ACCENT);
        
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite.setTextDatum(MC_DATUM); // Middle Center
        sprite.drawString(title, SCREEN_WIDTH / 2, 10, FONT_BODY);
    }
};

// Global Instance
DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
