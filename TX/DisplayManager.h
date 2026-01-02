#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "HardwareConfig.h"
#include "Theme.h"

// Canvas size for double buffering (128x160 fits in ESP32 RAM: ~40KB)
class DisplayManager {
private:
    Adafruit_ST7735 tft;
    GFXcanvas16* canvas; // Double buffer

public:
    // Initialize with explicit pins
    DisplayManager() : tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_MOSI, PIN_TFT_SCLK, PIN_TFT_RST) {
        canvas = new GFXcanvas16(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    void init() {
        tft.initR(INITR_BLACKTAB); // Init ST7735S chip, black tab
        tft.setRotation(0); // Portrait
        tft.fillScreen(ST7735_BLACK);
        
        // Clear canvas
        canvas->fillScreen(COLOR_BG_DARK);
    }

    GFXcanvas16* getCanvas() {
        return canvas;
    }

    void beginFrame() {
        canvas->fillScreen(COLOR_BG_DARK);
    }

    void endFrame() {
        // Push canvas to display
        // Optimized drawing not possible with standard GFX drawRGBBitmap to ST7735 without modification
        // but drawRGBBitmap is available.
        tft.drawRGBBitmap(0, 0, canvas->getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    
    // Legacy helper compatibility
    void drawHeader(const char* title) {
        canvas->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_PANEL);
        canvas->drawFastHLine(0, 20, SCREEN_WIDTH, COLOR_ACCENT);
        
        canvas->setTextColor(COLOR_TEXT_MAIN);
        setTextCentered(title, SCREEN_WIDTH / 2, 6, 1);
    }

    // Helper to simulate text datum (centering)
    void setTextCentered(String text, int x, int y, int size) {
        int w = text.length() * 6 * size; // Approx width (default font 6x8)
        int h = 8 * size;
        canvas->setCursor(x - w/2, y);
        canvas->setTextSize(size);
        canvas->print(text);
    }
    
    void setTextLeft(String text, int x, int y, int size) {
        canvas->setCursor(x, y);
        canvas->setTextSize(size);
        canvas->print(text);
    }
    
    // GFX doesn't have drawSmoothArc, draw helper
    void drawArcSegment(int x, int y, int r, int startAngle, int endAngle, uint16_t color) {
        // Very basic approximations
        for (int i=startAngle; i<=endAngle; i+=5) {
             float rad = i * DEG_TO_RAD;
             int px = x + cos(rad) * r;
             int py = y + sin(rad) * r;
             canvas->drawPixel(px, py, color);
        }
    }
};

// Global Instance
DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
