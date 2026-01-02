#ifndef SCREEN_TELEMETRY_H
#define SCREEN_TELEMETRY_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

// Real-Time Graphing
class ScreenTelemetry {
private:
    int historyThr[128];
    int historyStr[128];
    int idx = 0;
    
public:
    void init() {
        for(int i=0; i<128; i++) { historyThr[i] = 0; historyStr[i] = 0; }
    }
    
    void update() {
        historyThr[idx] = inputManager.getThrottleNormalized();
        historyStr[idx] = inputManager.getSteeringNormalized();
        idx = (idx + 1) % 128;
    }
    
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite->setTextColor(COLOR_TEXT_GLOW, COLOR_BG_MAIN);
        sprite->setTextDatum(TL_DATUM);
        sprite->drawString("LIVE DATA", 5, 5, FONT_SMALL);
        
        int mid = SCREEN_HEIGHT/2;
        
        // Draw Axis
        sprite->drawFastHLine(0, mid, SCREEN_WIDTH, COLOR_BG_PANEL);
        
        // Draw Graphs
        for (int i=0; i<SCREEN_WIDTH; i++) {
            int pIdx = (idx + i) % 128;
            
            // Throttle (Cyan)
            int tVal = historyThr[pIdx]; // -100 to 100
            int tY = map(tVal, -100, 100, mid + 30, mid - 30);
            sprite->drawPixel(i, tY, COLOR_ACCENT_PRI);
            
            // Steering (Red, Offset)
            int sVal = historyStr[pIdx];
            int sY = map(sVal, -100, 100, mid + 30, mid - 30);
            sprite->drawPixel(i, sY, COLOR_ACCENT_SEC);
        }
        
        // Labels
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
        sprite->drawString("THR", 5, mid-35, FONT_SMALL);
        
        sprite->setTextColor(COLOR_ACCENT_SEC, COLOR_BG_MAIN);
        sprite->drawString("STR", 5, mid+25, FONT_SMALL);
    }
};
#endif // SCREEN_TELEMETRY_H
