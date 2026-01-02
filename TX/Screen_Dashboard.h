#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "CommsManager.h"
#include "InputManager.h"
#include "Theme.h"
#include "Widgets.h"
#include "AnimationUtils.h"

// ==========================================
//          MAXIMALIST V4 COCKPIT
// ==========================================

class ScreenDashboard {
private:
    // Animation States
    AnimFloat needleVal;
    AnimFloat barSteer;
    float gyroAngle = 0;
    
    // Widgets
    WidgetGauge gaugeTemp;
    WidgetGauge gaugeVolt;
    WidgetRadar radarG;
    WidgetGraph graphRSS;
    
    // Fake Data Generators
    int fakeTemp = 45;
    
public:
    ScreenDashboard() : 
        needleVal(0, 0.15f, 0.8f),
        barSteer(0, 0.2f, 0.8f),
        gaugeTemp("TMP", 14, COLOR_WARN),
        gaugeVolt("BAT", 14, COLOR_OK),
        radarG(18),
        graphRSS(30, 15, COLOR_ACCENT_TER)
    {}

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // --- UPDATES ---
        needleVal.target = abs(inputManager.getThrottleNormalized());
        needleVal.update();
        barSteer.target = inputManager.getSteeringNormalized();
        barSteer.update();
        
        // Gyro
        if (inputManager.currentState.swGyro) {
            gyroAngle += 8.0f;
            if (gyroAngle >= 360) gyroAngle -= 360;
        }
        
        // Simulation
        fakeTemp = 45 + (int)(needleVal.val() * 0.2); // Temp rises with Throttle
        gaugeTemp.update(fakeTemp);
        gaugeVolt.update(90); // 90% Battery
        
        // G-Force Calc (Rough approx)
        int gY = (int)(needleVal.val()); // accel
        int gX = (int)(barSteer.val()); // turn
        radarG.update(gX, gY);
        
        // RSSI Fake
        if (millis() % 100 == 0) graphRSS.push(random(70, 100));

        // --- DRAWING (DENSE) ---
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // 1. TOP BAR (Status)
        sprite->fillRect(0, 0, SCREEN_WIDTH, 14, COLOR_BG_HEADER);
        bool linked = commsManager.isConnected();
        sprite->setTextColor(linked ? COLOR_OK : COLOR_ERR, COLOR_BG_HEADER);
        sprite->drawString(linked ? "LINK: OK" : "LINK: NC", 5, 2, FONT_SMALL);
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_HEADER);
        sprite->setTextDatum(TR_DATUM);
        sprite->drawString("USER: ADM", SCREEN_WIDTH-5, 2, FONT_SMALL);

        // 2. MAIN TACHOMETER (Center)
        int cx = SCREEN_WIDTH / 2;
        int cy = 72;
        int r = 52;
        
        // Tech Ring
        sprite->drawCircle(cx, cy, r, COLOR_BG_PANEL);
        sprite->drawCircle(cx, cy, r-3, COLOR_BG_PANEL);
        
        // Redline Zone
        int startRL = 360; 
        int endRL = 405;
        // manually draw redline arc or just colored ticks
        
        // Ticks
        for (int i=135; i<=405; i+=10) {
             float rad = i * DEG_TO_RAD;
             int len = (i%45==0) ? 8 : 4;
             int x1 = cx + cos(rad) * (r-len);
             int y1 = cy + sin(rad) * (r-len);
             int x2 = cx + cos(rad) * r;
             int y2 = cy + sin(rad) * r;
             
             uint16_t col = (i >= 360) ? COLOR_ACCENT_SEC : COLOR_TEXT_DIM;
             sprite->drawLine(x1, y1, x2, y2, col);
        }
        
        // Needle
        float val = needleVal.val(); // 0-100
        float angle = map(val, 0, 100, 135, 405);
        float rad = angle * DEG_TO_RAD;
        int nx = cx + cos(rad) * (r-10);
        int ny = cy + sin(rad) * (r-10);
        sprite->drawLine(cx, cy, nx, ny, COLOR_ACCENT_PRI);
        sprite->fillCircle(cx, cy, 5, COLOR_BG_PANEL);

        // Digital Readout
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        int kph = map(val, 0, 100, 0, 120);
        sprite->drawNumber(kph, cx, cy+18, FONT_NUMS);
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->drawString("KPH", cx, cy+34, FONT_SMALL);

        // 3. WIDGETS (Filling Space)
        
        // Left: Temp & Batt
        gaugeTemp.draw(sprite, 20, 40);
        gaugeVolt.draw(sprite, 20, 80);
        
        // Right: RSSI Graph & System Load
        graphRSS.draw(sprite, SCREEN_WIDTH-35, 30);
        sprite->drawString("RSSI", SCREEN_WIDTH-35, 46, FONT_SMALL);
        
        // Right: Sys Load
        sprite->drawRect(SCREEN_WIDTH-35, 60, 30, 10, COLOR_BG_PANEL);
        sprite->fillRect(SCREEN_WIDTH-34, 61, 20, 8, COLOR_ACCENT_PRI); // Fake 66%
        sprite->drawString("CPU", SCREEN_WIDTH-35, 72, FONT_SMALL);

        // 4. BOTTOM AREA (Control & G-Force)
        int by = 120;
        
        // G-Force Radar (Center Bottom) (Moved slightly up to fit steering)
        // Actually, let's put it Bottom Center overlaid or just below tach
        radarG.draw(sprite, cx, 125);
        
        // Steering Bar (Very Bottom)
        int sy = 152;
        int barW = 50;
        sprite->drawFastHLine(cx-barW, sy, barW*2, COLOR_BG_PANEL);
        sprite->drawFastVLine(cx, sy-4, 8, COLOR_TEXT_MAIN); // Center
        
        int sVal = (int)barSteer.val();
        int len = map(abs(sVal), 0, 100, 0, barW);
        if (sVal > 0) sprite->fillRect(cx, sy-2, len, 5, COLOR_ACCENT_PRI);
        else sprite->fillRect(cx-len, sy-2, len, 5, COLOR_ACCENT_PRI);

        // 5. GYRO OVERLAY (Hexagon Forcefield)
        if (inputManager.currentState.swGyro) {
            float gRad = gyroAngle * DEG_TO_RAD;
            // Draw 3 spinning brackets around tach
            for(int k=0; k<3; k++) {
                float a = gRad + (k * 120 * DEG_TO_RAD);
                int gx = cx + cos(a) * (r+4);
                int gy = cy + sin(a) * (r+4);
                sprite->fillCircle(gx, gy, 2, COLOR_ACCENT_SEC);
            }
            sprite->setTextColor(COLOR_ACCENT_SEC, COLOR_BG_MAIN);
            sprite->drawString("STAB-ON", cx, cy-20, FONT_SMALL);
        }
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
