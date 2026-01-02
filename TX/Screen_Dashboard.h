#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "CommsManager.h"
#include "InputManager.h"
#include "Theme.h"
#include "AnimationUtils.h"

// Complex Dashboard State
struct DashboardState {
    AnimFloat throttleVal;
    AnimFloat steerVal;
    
    // Gyro Stability Animation
    AnimFloat gyroStab; // 0-100 expansion of the "Shield"
    bool lastGyroState;
    
    ScanLine bgScan;
    
    DashboardState() : 
        throttleVal(0, 0.15f, 0.85f), 
        steerVal(0, 0.2f, 0.7f), 
        gyroStab(0, 0.1f, 0.85f),
        bgScan(0, SCREEN_HEIGHT, 0.8f),
        lastGyroState(false)
    {}
};

class ScreenDashboard {
private:
    DashboardState state;

public:
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // 1. Physics Update
        state.throttleVal.target = inputManager.getThrottleNormalized();
        state.throttleVal.update();
        state.steerVal.target = inputManager.getSteeringNormalized();
        state.steerVal.update();
        state.bgScan.update();
        
        bool currentGyro = inputManager.currentState.swGyro;
        if (currentGyro != state.lastGyroState) {
            if (currentGyro) {
                state.gyroStab.target = 100.0f; // Enable Shield
                soundManager.playGyroOn();
            } else {
                state.gyroStab.target = 0.0f; 
                soundManager.playGyroOff();
            }
            state.lastGyroState = currentGyro;
        }
        state.gyroStab.update();

        // 2. Background (Tech Blue)
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Digital Grid
        for (int y=0; y<SCREEN_HEIGHT; y+=20) {
             sprite->drawFastHLine(0, y, SCREEN_WIDTH, COLOR_BG_SHADOW);
        }
        // Scanline
        sprite->drawFastHLine(0, state.bgScan.y(), SCREEN_WIDTH, COLOR_BG_PANEL);

        // 3. Center Tach (Clean Tech)
        int cx = SCREEN_WIDTH / 2;
        int cy = 70;
        int r = 50;
        
        int tVal = (int)state.throttleVal.val();
        
        // Circular Track
        sprite->drawCircle(cx, cy, r, COLOR_BG_PANEL);
        sprite->drawCircle(cx, cy, r-5, COLOR_BG_PANEL);
        
        // Arc
        int angle = map(abs(tVal), 0, 100, 0, 260);
        int start = 140; // Bottom left
        
        uint16_t gaugeCol = COLOR_ACCENT_2; // Cyan
        if (abs(tVal)>80) gaugeCol = COLOR_ACCENT_1; // Red
        
        // Draw multiple arcs for "Tech" look
        for (int i=0; i<angle; i+=4) {
             float rad = (start + i) * DEG_TO_RAD;
             int x = cx + cos(rad) * (r-2);
             int y = cy + sin(rad) * (r-2);
             sprite->drawPixel(x, y, gaugeCol);
             
             // Inner ticks
             if (i%20 == 0) {
                 int x2 = cx + cos(rad) * (r-8);
                 int y2 = cy + sin(rad) * (r-8);
                 sprite->drawLine(x, y, x2, y2, gaugeCol);
             }
        }
        
        // Digital Speed
        int kph = map(abs(tVal), 0, 100, 0, 80);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawNumber(kph, cx, cy-5, FONT_DIGIT);
        
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_MAIN);
        sprite->drawString("KM/H", cx, cy+20, FONT_MICRO);

        // 4. Steering (Bottom)
        int sVal = (int)state.steerVal.val();
        int sy = 135;
        // Center mark
        sprite->drawFastVLine(cx, sy-5, 10, COLOR_TEXT_MUTED);
        
        // Bar
        int barLen = map(abs(sVal), 0, 100, 0, 50);
        if (sVal > 0) sprite->fillRect(cx, sy, barLen, 4, COLOR_ACCENT_2);
        else sprite->fillRect(cx-barLen, sy, barLen, 4, COLOR_ACCENT_2);

        // 5. Suspension Bar (Right Side)
        int sus = inputManager.currentState.potSuspension;
        int susH = map(sus, 0, 4095, 0, 60);
        sprite->drawRect(SCREEN_WIDTH-10, 40, 6, 60, COLOR_BG_PANEL);
        sprite->fillRect(SCREEN_WIDTH-9, 100-susH, 4, susH, COLOR_ACCENT_4); // Electric Blue
        sprite->drawString("S", SCREEN_WIDTH-7, 30, FONT_MICRO);

        // 6. GYRO STABILITY DISPLAY
        // "Protective Shield" / Brackets
        float g = state.gyroStab.val(); // 0-100
        if (g > 1.0f) {
            int gap = map((long)g, 0, 100, 60, 30); // Clamps IN
            int w = 10;
            int h = 40;
            // Left Bracket
            sprite->drawFastVLine(cx - gap, cy - h/2, h, COLOR_SUCCESS);
            sprite->drawFastHLine(cx - gap, cy - h/2, w, COLOR_SUCCESS); // Top cap
            sprite->drawFastHLine(cx - gap, cy + h/2, w, COLOR_SUCCESS); // Bot cap
            
            // Right Bracket
            sprite->drawFastVLine(cx + gap, cy - h/2, h, COLOR_SUCCESS);
            sprite->drawFastHLine(cx + gap - w, cy - h/2, w + 1, COLOR_SUCCESS);
            sprite->drawFastHLine(cx + gap - w, cy + h/2, w + 1, COLOR_SUCCESS);
            
            if (g > 90) {
                sprite->setTextColor(COLOR_SUCCESS, COLOR_BG_MAIN);
                sprite->drawString("STABILITY ON", cx, cy + 30, FONT_MICRO);
            }
        }
        
        // 7. Connectivity Status
        bool linked = commsManager.isConnected();
        sprite->fillCircle(10, 10, 4, linked ? COLOR_SUCCESS : COLOR_DANGER);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString(linked ? "TX-LINK" : "SEARCH", 20, 10, FONT_LABEL);
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
