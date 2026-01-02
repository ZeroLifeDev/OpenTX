#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

class ScreenDashboard {
private:
    float animPhase = 0; // For pulsing effects

public:
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // Update Animation Phase
        animPhase += 0.2; // faster
        if (animPhase > 6.28) animPhase = 0;
        
        bool connected = commsManager.isConnected();

        // 1. Header (Dynamic)
        sprite->fillRect(0, 0, SCREEN_WIDTH, 18, COLOR_BG_PANEL);
        sprite->drawRect(0, 0, SCREEN_WIDTH, 18, COLOR_BG_PANEL); 
        
        // Connection Icon (Pulse green if connected, red if not)
        uint16_t connColor = connected ? COLOR_HIGHLIGHT : COLOR_ACCENT_ALT;
        if (!connected && (millis()/500)%2==0) connColor = COLOR_BG_DARK; // Blink
        
        sprite->fillCircle(10, 9, 3, connColor);
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("OpenTX", SCREEN_WIDTH/2, 9, FONT_SMALL);

        // Gyro Status (Right)
        if (inputManager.currentState.swGyro) {
             sprite->setTextColor(COLOR_ACCENT, COLOR_BG_PANEL);
             sprite->drawString("GYRO", SCREEN_WIDTH - 20, 9, FONT_SMALL);
        }

        // 2. Main Gauge (Hexagonal Tech Style)
        int cx = SCREEN_WIDTH / 2;
        int cy = 75;
        int r = 45;
        
        // Dynamic color ring
        int throttle = inputManager.getThrottleNormalized();
        int absThrot = abs(throttle);
        int speed = map(absThrot, 0, 100, 0, 99);
        
        // Draw Hex Outline
        // (Simplified to circle for performance but styled)
        sprite->drawCircle(cx, cy, r, COLOR_BG_PANEL);
        sprite->drawCircle(cx, cy, r+2, COLOR_BG_PANEL);
        
        // Active RPM Bar
        int arcLen = map(absThrot, 0, 100, 0, 270);
        // Draw Arc manually since pushSprite is used
        // Use a series of lines or dots for "Tech" look
        int startAng = 135;
        for(int i=0; i<30; i++) {
             int ang = startAng + (i * 9); // 270 deg total
             if (ang >= startAng + arcLen) break;
             
             float rad = ang * DEG_TO_RAD;
             int x1 = cx + cos(rad) * (r - 2);
             int y1 = cy + sin(rad) * (r - 2);
             int x2 = cx + cos(rad) * (r + 4);
             int y2 = cy + sin(rad) * (r + 4);
             
             uint16_t tickCol = (i > 24) ? COLOR_ACCENT_ALT : COLOR_ACCENT;
             sprite->drawLine(x1, y1, x2, y2, tickCol);
        }
        
        // Digital Speed
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_DARK);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString(String(speed), cx, cy, FONT_HEADER); // Large Font 6 IS HUGE, maybe 4? default is small. 4 is good.
        
        sprite->setTextDatum(TC_DATUM);
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_DARK);
        sprite->drawString(throttle >= 0 ? "FWD" : "REV", cx, cy + 25, FONT_SMALL);

        // 3. Lower Infos (Steering & Trim)
        // Steering Bar (Horizontal)
        int steer = inputManager.getSteeringNormalized();
        int barW = SCREEN_WIDTH - 20;
        int barX = 10;
        int barY = 130;
        
        sprite->drawRect(barX, barY, barW, 6, COLOR_BG_PANEL);
        int center = barX + barW/2;
        
        if (steer > 0) {
            int w = map(steer, 0, 100, 0, barW/2);
            sprite->fillRect(center, barY+1, w, 4, COLOR_ACCENT);
        } else {
            int w = map(abs(steer), 0, 100, 0, barW/2);
            sprite->fillRect(center-w, barY+1, w, 4, COLOR_ACCENT);
        }
        sprite->drawFastVLine(center, barY-2, 10, COLOR_TEXT_MAIN); // Center Marker

        // Text
        sprite->setTextDatum(BC_DATUM);
        sprite->drawString("STEERING", cx, barY - 4, FONT_SMALL);
        
        // Trim Value
        sprite->setTextDatum(BC_DATUM);
        sprite->setTextColor(COLOR_HIGHLIGHT, COLOR_BG_DARK);
        sprite->drawString("TRIM: " + String(inputManager.currentState.trimLevel), cx, SCREEN_HEIGHT - 2, FONT_SMALL);
    }
};

// Global Instance
ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
