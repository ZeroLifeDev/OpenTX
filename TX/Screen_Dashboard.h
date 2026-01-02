#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "CommsManager.h"
#include "InputManager.h"
#include "Theme.h"
#include "AnimationUtils.h"

// ==========================================
//          PREMIUM DASHBOARD V3
// ==========================================

class ScreenDashboard {
private:
    // Animation States
    AnimFloat needleVal;
    AnimFloat barSteer;
    float gyroAngle = 0;
    
public:
    ScreenDashboard() : 
        needleVal(0, 0.1f, 0.85f),
        barSteer(0, 0.2f, 0.8f)
    {}

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // 1. Inputs & Physics
        needleVal.target = abs(inputManager.getThrottleNormalized());
        needleVal.update();
        
        barSteer.target = inputManager.getSteeringNormalized();
        barSteer.update();
        
        // Gyro Spin
        if (inputManager.currentState.swGyro) {
            gyroAngle += 5.0f;
            if (gyroAngle >= 360) gyroAngle -= 360;
        }

        // 2. Background (Obsidian)
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_HEADER);
        bool linked = commsManager.isConnected();
        sprite->fillCircle(10, 10, 4, linked ? COLOR_OK : COLOR_ERR);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_HEADER);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString(linked ? "LINKED" : "SEARCH", 20, 11, FONT_SMALL);
        
        // Battery (Simulated)
        sprite->setTextDatum(MR_DATUM);
        sprite->drawString("4.2V", SCREEN_WIDTH-5, 11, FONT_SMALL);

        // 3. Main Gauge (Automotive Style)
        int cx = SCREEN_WIDTH / 2;
        int cy = 80;
        int r = 58;
        
        // Outer Bezel
        sprite->drawCircle(cx, cy, r, COLOR_BG_PANEL);
        sprite->drawCircle(cx, cy, r-1, COLOR_BG_PANEL);
        
        // Ticks
        for (int i=135; i<=405; i+=15) {
             float rad = i * DEG_TO_RAD;
             int x1 = cx + cos(rad) * (r-5);
             int y1 = cy + sin(rad) * (r-5);
             int x2 = cx + cos(rad) * (r-10);
             int y2 = cy + sin(rad) * (r-10);
             uint16_t col = (i > 360) ? COLOR_ACCENT_SEC : COLOR_TEXT_DIM;
             sprite->drawLine(x1, y1, x2, y2, col);
        }
        
        // Needle
        float val = needleVal.val();
        float angle = map(val, 0, 100, 135, 405);
        float rad = angle * DEG_TO_RAD;
        int nx = cx + cos(rad) * (r-12);
        int ny = cy + sin(rad) * (r-12);
        sprite->drawLine(cx, cy, nx, ny, COLOR_ACCENT_PRI);
        sprite->drawCircle(cx, cy, 4, COLOR_BG_PANEL); // Cap
        
        // Digital Speed
        int kph = map(val, 0, 100, 0, 120);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawNumber(kph, cx, cy+15, FONT_NUMS);
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->drawString("KMH", cx, cy+32, FONT_SMALL);
        
        // 4. Gyro Horizon (Top Gap)
        if (inputManager.currentState.swGyro) {
             int gy = 40;
             sprite->drawRect(cx-20, gy, 40, 2, COLOR_ACCENT_PRI);
             // Rotating element
             float gr = gyroAngle * DEG_TO_RAD;
             int gx = cx + cos(gr) * 10;
             int gy2 = gy + sin(gr) * 4; // Flat perspective
             sprite->fillCircle(gx, gy2, 2, COLOR_ACCENT_PRI);
        }

        // 5. Steering Bar (Bottom)
        int sy = 145;
        int sVal = (int)barSteer.val();
        int barW = 50;
        
        sprite->drawFastHLine(cx-barW, sy, barW*2, COLOR_BG_PANEL);
        sprite->drawFastVLine(cx, sy-3, 6, COLOR_TEXT_DIM);
        
        int len = map(abs(sVal), 0, 100, 0, barW);
        if (sVal > 0) sprite->fillRect(cx, sy-1, len, 3, COLOR_ACCENT_PRI);
        else sprite->fillRect(cx-len, sy-1, len, 3, COLOR_ACCENT_PRI);
        
        // 6. Suspension Bars (Vertical sides)
        int susp = inputManager.currentState.potSuspension; // 0-4095
        int sH = map(susp, 0, 4095, 0, 80);
        
        // Right Side
        sprite->drawRect(SCREEN_WIDTH-6, 40, 4, 80, COLOR_BG_PANEL);
        sprite->fillRect(SCREEN_WIDTH-5, 120-sH, 2, sH, COLOR_ACCENT_TER);
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
