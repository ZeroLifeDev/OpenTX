#ifndef SCREEN_DEBUG_H
#define SCREEN_DEBUG_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

class ScreenDebug {
private:
    int signalHistory[128]; // Matches screen width
    int historyIndex = 0;

public:
    void init() {
        for(int i=0; i<128; i++) signalHistory[i] = 40;
    }

    void update() {
        // Simulate Signal Data
        signalHistory[historyIndex] = random(40, 90);
        historyIndex = (historyIndex + 1) % 128;
    }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        sprite->fillSprite(COLOR_BG_MAIN); // Light
        
        // Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_ACCENT_4);
        sprite->setTextColor(COLOR_BG_PANEL, COLOR_ACCENT_4);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString(" DIAGNOSTICS", 2, 10, FONT_LABEL);
        
        // System Data Block
        int y = 30;
        int x = 5;
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(TL_DATUM);
        
        char buf[32];
        
        float txTemp = temperatureRead(); 
        if (isnan(txTemp)) txTemp = 0.0;
        
        sprintf(buf, "CPU T: %.1f C", txTemp);
        sprite->drawString(buf, x, y, FONT_SMALL); y += 12;
        
        sprintf(buf, "V_BAT: 4.20 V"); // Placeholder
        sprite->drawString(buf, x, y, FONT_SMALL); y += 12;

        sprintf(buf, "GYRO: %s", inputManager.currentState.swGyro ? "ACT" : "STBY");
        sprite->drawString(buf, x, y, FONT_SMALL); y += 12;
        
        // Signal Graph (Bottom)
        int graphH = 50;
        int graphY = SCREEN_HEIGHT - graphH;
        
        // Background Grid
        sprite->fillRect(0, graphY, SCREEN_WIDTH, graphH, COLOR_BG_PANEL);
        sprite->drawRect(0, graphY, SCREEN_WIDTH, graphH, COLOR_TEXT_MUTED);
        sprite->drawFastHLine(0, graphY + graphH/2, SCREEN_WIDTH, COLOR_BG_SHADOW);
        
        // Plot
        for (int i=0; i<SCREEN_WIDTH; i++) {
            int idx = (historyIndex + i) % 128;
            int val = signalHistory[idx];
            int h = map(val, 0, 100, 0, graphH);
            
            sprite->drawPixel(i, graphY + graphH - h, COLOR_ACCENT_2);
        }
        
        sprite->setTextColor(COLOR_ACCENT_2, COLOR_BG_PANEL);
        sprite->drawString("RF NOISE FLOOR", 2, graphY + 2, FONT_MICRO);
    }
};

extern ScreenDebug screenDebug;

#endif // SCREEN_DEBUG_H
