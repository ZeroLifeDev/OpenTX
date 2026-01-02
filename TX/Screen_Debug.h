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
        GFXcanvas16* sprite = display->getCanvas();
        
        // Header
        display->drawHeader("DEBUG / TELEMETRY");

        // Content
        int y = 25;
        int lh = 12; // Line Height

        sprite->setTextColor(COLOR_TEXT_MAIN);
        sprite->setTextSize(1);

        // 1. Inputs (Raw)
        display->setTextLeft("RAW INPUTS:", 5, y, 1); y += lh;
        
        char buf[32];
        sprintf(buf, " Steer: %d", inputManager.currentState.steering);
        display->setTextLeft(buf, 5, y, 1); y += lh;
        
        sprintf(buf, " Thr: %d", inputManager.currentState.throttle);
        display->setTextLeft(buf, 5, y, 1); y += lh;
        
        sprintf(buf, " Susp: %d", inputManager.currentState.potSuspension);
        display->setTextLeft(buf, 5, y, 1); y += lh;
        
        sprintf(buf, " Trim: %d", inputManager.currentState.trimLevel);
        display->setTextLeft(buf, 5, y, 1); y += lh;

        // 2. System Info
        y += 5;
        display->setTextLeft("SYSTEM INFO:", 5, y, 1); y += lh;
        
        float txTemp = temperatureRead(); 
        if (isnan(txTemp)) txTemp = 0.0;
        
        sprintf(buf, " TX Temp: %.1f C", txTemp);
        display->setTextLeft(buf, 5, y, 1); y += lh;
        
        sprintf(buf, " RX Temp: --.-- C"); 
        display->setTextLeft(buf, 5, y, 1); y += lh;
        
        sprintf(buf, " Gyro: %s", inputManager.currentState.swGyro ? "ON" : "OFF");
        display->setTextLeft(buf, 5, y, 1); y += lh;

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
            if (i >= 160) break;
            
            int val = signalHistory[idx]; // 40-90
            int h = map(val, 40, 90, 0, graphH);
            
            sprite->drawPixel(i, graphY + graphH - h, COLOR_ACCENT);
        }
        
        display->setTextLeft("RX SIG", 2, graphY + 2, 1);
    }
};

// Global Instance
ScreenDebug screenDebug;

#endif // SCREEN_DEBUG_H
