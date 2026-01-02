#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "CommsManager.h"
#include "InputManager.h"
#include "Theme.h"
#include "Widgets.h"
#include "AnimationUtils.h"

// ==========================================
//          CLEAN V4.1 DASHBOARD
// ==========================================

class ScreenDashboard {
private:
    float smoothT = 0;
    float smoothS = 0;
    
    // Widgets
    WidgetGauge gaugeVolt;
    WidgetRadar radarG;
    WidgetGraph graphRSS;
    
public:
    ScreenDashboard() : 
        gaugeVolt("BAT", 13, COLOR_OK), // Smaller radius
        radarG(20), // Slightly larger radar
        graphRSS(28, 14, COLOR_ACCENT_PRI) // Clean graph
    {}

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        sprite->fillSprite(COLOR_BG_MAIN);

        // Data smoothing
        float rawT = abs(inputManager.getThrottleNormalized());
        float rawS = inputManager.getSteeringNormalized();
        smoothT += (rawT - smoothT) * 0.2;
        smoothS += (rawS - smoothS) * 0.2;

        // Widget Updates
        gaugeVolt.update(92); // Fake 92%
        radarG.update((int)smoothS, (int)smoothT);
        if (millis() % 200 == 0) graphRSS.push(random(85, 99));

        // --- LAYOUT: STRICT GRID ---
        
        // 1. HEADER (H: 16px)
        // Background
        sprite->fillRect(0, 0, SCREEN_WIDTH, 16, COLOR_BG_HEADER);
        // Link Status
        bool linked = commsManager.isConnected();
        sprite->setTextColor(linked ? COLOR_OK : COLOR_ERR, COLOR_BG_HEADER);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString(linked ? "LNK" : "NC", 4, 8, FONT_SMALL);
        // Mode
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_HEADER);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("SPORT", SCREEN_WIDTH/2, 8, FONT_SMALL);
        // Batt/User
        sprite->setTextDatum(MR_DATUM);
        sprite->drawString("ADM", SCREEN_WIDTH-4, 8, FONT_SMALL);

        // 2. MAIN TACH (Center: 64, 70)
        int cx = SCREEN_WIDTH / 2;
        int cy = 70;
        int r = 48; // Smaller than before for breathing room
        
        // Outer Ring
        sprite->drawCircle(cx, cy, r, COLOR_BG_PANEL);
        sprite->drawCircle(cx, cy, r+3, COLOR_BG_PANEL);
        
        // Speed Arc (270 deg)
        int angle = map((int)smoothT, 0, 100, 135, 405);
        // Draw Arc Segments (Cleaner than pixel loop)
        // TFT_eSPI doesn't have drawArc easily, using vector lines
        for (int i=135; i<=405; i+=6) {
             if (i > angle) break;
             float rad = i * DEG_TO_RAD;
             int x1 = cx + cos(rad) * (r-4);
             int y1 = cy + sin(rad) * (r-4);
             int x2 = cx + cos(rad) * r;
             int y2 = cy + sin(rad) * r;
             
             // Color Gradient logic
             uint16_t col = COLOR_ACCENT_PRI;
             if (i > 360) col = COLOR_ACCENT_SEC; // Redline
             
             sprite->drawLine(x1, y1, x2, y2, col);
        }

        // Digital Speed (Big & Clean)
        sprite->setTextColor(COLOR_TEXT_GLOW, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        int dispSpeed = map((int)smoothT, 0, 100, 0, 99);
        sprite->drawNumber(dispSpeed, cx, cy+5, FONT_LARGE);
        
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->drawString("KMH", cx, cy + 22, FONT_SMALL);

        // 3. SIDE PANELS
        // Left: Battery Gauge
        gaugeVolt.draw(sprite, 20, 50);
        
        // Right: Signal Graph
        graphRSS.draw(sprite, SCREEN_WIDTH - 32, 42);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("SIG", SCREEN_WIDTH - 18, 62, FONT_SMALL);
        
        // 4. BOTTOM CLUSTER (Y: 120+)
        // G-Force Radar (moved to left bottom)
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->drawString("G-METER", 25, 120, FONT_SMALL);
        radarG.draw(sprite, 25, 140);
        
        // Steering Bar (Right bottom)
        sprite->drawString("STEER", SCREEN_WIDTH - 25, 120, FONT_SMALL);
        int barW = 40;
        int bx = SCREEN_WIDTH - 45;
        int by = 140;
        
        sprite->drawRect(bx, by-3, barW, 6, COLOR_BG_PANEL);
        sprite->drawFastVLine(bx + barW/2, by-5, 10, COLOR_TEXT_DIM); // Center Mark
        
        int sLen = map(abs((int)smoothS), 0, 100, 0, barW/2);
        if (smoothS > 0) 
            sprite->fillRect(bx + barW/2, by-2, sLen, 4, COLOR_ACCENT_PRI);
        else 
            sprite->fillRect(bx + barW/2 - sLen, by-2, sLen, 4, COLOR_ACCENT_PRI);

        // 5. GYRO STATUS (Top Left overlay)
        if (inputManager.currentState.swGyro) {
            sprite->setTextColor(COLOR_ACCENT_3, COLOR_BG_MAIN);
            sprite->setTextDatum(TL_DATUM);
            sprite->drawString("GYRO ON", 4, 20, FONT_SMALL);
        }
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
