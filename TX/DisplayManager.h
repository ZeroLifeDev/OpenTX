#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "HardwareConfig.h" 
#include <TFT_eSPI.h> // Make sure to edit User_Setup.h to select ILI9163_DRIVER
#include "Theme.h"

class DisplayManager {
private:
    TFT_eSPI tft;
    TFT_eSprite sprite;

public:
    DisplayManager() : tft(), sprite(&tft) {}

    void init() {
        tft.init();
        tft.setRotation(0); 
        tft.fillScreen(COLOR_BG_DARK);
        
        // ILI9163 usually matches ST7735 BGR/RGB needs
        tft.setSwapBytes(true); 

        sprite.setColorDepth(16);
        sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
        sprite.setSwapBytes(true);
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
    
    // Draw Header Helper
    void drawHeader(const char* title) {
        sprite.fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_PANEL);
        sprite.drawFastHLine(0, 20, SCREEN_WIDTH, COLOR_ACCENT);
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(title, SCREEN_WIDTH / 2, 11, FONT_SMALL);
    }
    
    // Legacy Helpers not needed as we updated screens to use Sprite directly,
    // but in case any GFX calls remain:
    void setTextCentered(String text, int x, int y, int size) {
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(text, x, y, size);
    }
    
    void setTextLeft(String text, int x, int y, int size) {
        sprite.setTextDatum(TL_DATUM);
        sprite.drawString(text, x, y, size);
    }
    
    void drawArcSegment(int x, int y, int r, int startAngle, int endAngle, uint16_t color) {
         // rough implementation for sprite
         for (int i=startAngle; i<=endAngle; i+=5) {
             float rad = i * DEG_TO_RAD;
             int px = x + cos(rad) * r;
             int py = y + sin(rad) * r;
             sprite.drawPixel(px, py, color);
        }
    }
};

// Global Instance
DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
