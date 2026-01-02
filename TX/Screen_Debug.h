#ifndef SCREEN_DEBUG_H
#define SCREEN_DEBUG_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

class ScreenDebug {
private:
    // For "Radio" animation or graph
    int signalHistory[160]; // Store last signal strengths (simulated)
    int historyIndex = 0;

public:
    void init() {
        for(int i=0; i<160; i++) signalHistory[i] = 0;
    }

    void update() {
        // Update simulated signal for graph
        signalHistory[historyIndex] = random(40, 90); // -40 to -90 dBm simulated
        historyIndex = (historyIndex + 1) % 160;
    }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_PANEL);
        sprite->setTextColor(COLOR_ACCENT, COLOR_BG_PANEL);
        sprite->setTextDatum(TC_DATUM);
        sprite->drawString("DEBUG / TELEMETRY", SCREEN_WIDTH/2, 2, FONT_SMALL);
        sprite->drawFastHLine(0, 20, SCREEN_WIDTH, COLOR_ACCENT);

        // Content
        int y = 25;
        int lh = 12; // Line Height

        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_DARK);
        sprite->setTextDatum(TL_DATUM);

        // 1. Inputs (Raw)
        sprite->drawString("RAW INPUTS:", 5, y, FONT_SMALL); y += lh;
        
        char buf[32];
        sprintf(buf, " Steer: %d", inputManager.currentState.steering);
        sprite->drawString(buf, 5, y, FONT_SMALL); y += lh;
        
        sprintf(buf, " Thr: %d", inputManager.currentState.throttle);
        sprite->drawString(buf, 5, y, FONT_SMALL); y += lh;
        
        sprintf(buf, " Susp: %d", inputManager.currentState.potSuspension);
        sprite->drawString(buf, 5, y, FONT_SMALL); y += lh;
        
        sprintf(buf, " Trim: %d", inputManager.currentState.trimLevel);
        sprite->drawString(buf, 5, y, FONT_SMALL); y += lh;

        // 2. System Info
        y += 5;
        sprite->drawString("SYSTEM INFO:", 5, y, FONT_SMALL); y += lh;
        
        float txTemp = temperatureRead(); // Built-in ESP32 temp (may be inaccurate but real)
        if (isnan(txTemp)) txTemp = 0.0;
        
        sprintf(buf, " TX Temp: %.1f C", txTemp);
        sprite->drawString(buf, 5, y, FONT_SMALL); y += lh;
        
        // Mock RX Temp (since we don't have telemetry link code here yet)
        // User asked for it, imply it's coming or random
        sprintf(buf, " RX Temp: --.-- C"); 
        sprite->drawString(buf, 5, y, FONT_SMALL); y += lh;
        
        sprintf(buf, " Gyro: %s", inputManager.currentState.swGyro ? "ON" : "OFF");
        sprite->drawString(buf, 5, y, FONT_SMALL); y += lh;

        // 3. Mini Signal Graph at Bottom
        int graphY = SCREEN_HEIGHT - 30;
        int graphH = 30;
        
        // Draw Grid
        sprite->drawRect(0, graphY, SCREEN_WIDTH, graphH, COLOR_BG_PANEL);
        sprite->drawFastHLine(0, graphY + graphH/2, SCREEN_WIDTH, COLOR_BG_PANEL);
        
        // Draw Waveform
        for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
            // Get value from history buffer
            int idx = (historyIndex + i) % 160; 
            // Only draw as much as we have screen width
            if (i >= 160) break;
            
            int val = signalHistory[idx]; // 40-90
            // Map 40-90 to graph height (0-30)
            int h = map(val, 40, 90, 0, graphH);
            
            sprite->drawPixel(i, graphY + graphH - h, COLOR_ACCENT);
        }
        
        sprite->drawString("RX SIG", 2, graphY + 2, FONT_SMALL);
    }
};

// Global Instance
ScreenDebug screenDebug;

#endif // SCREEN_DEBUG_H
