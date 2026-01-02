#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "CommsManager.h"
#include "InputManager.h"
#include "Theme.h"
#include "AnimationUtils.h"

// Complex Dashboard State
struct DashboardState {
    // Physics-based values
    AnimFloat throttleVal;
    AnimFloat steerVal;
    
    // Animation States
    AnimFloat gyroAnim; // 0.0 to 100.0 (Expansion)
    AnimFloat gyroRot;  // Rotation for "Target"
    bool lastGyroState;
    
    ScanLine bgScan;
    MicroJitter jitter; 
    
    DashboardState() : 
        throttleVal(0, 0.15f, 0.85f), 
        steerVal(0, 0.2f, 0.7f), 
        gyroAnim(0, 0.1f, 0.85f),
        gyroRot(0, 0.05f, 0.95f),
        bgScan(0, SCREEN_HEIGHT, 1.0f),
        jitter(2),
        lastGyroState(false)
    {}
};

class ScreenDashboard {
private:
    DashboardState state;

public:
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // --- 1. Update Physics ---
        state.throttleVal.target = inputManager.getThrottleNormalized();
        state.throttleVal.update();
        
        state.steerVal.target = inputManager.getSteeringNormalized();
        state.steerVal.update();
        
        state.bgScan.update();
        
        // Gyro Logic
        bool currentGyro = inputManager.currentState.swGyro;
        if (currentGyro != state.lastGyroState) {
            if (currentGyro) {
                state.gyroAnim.snap(0); 
                state.gyroAnim.target = 100.0f; 
                state.gyroRot.velocity = 20.0f; // Spin up
            } else {
                state.gyroAnim.target = 0.0f; 
            }
            state.lastGyroState = currentGyro;
        }
        state.gyroAnim.update();
        if (currentGyro) state.gyroRot.target += 2.0f; // Constant rotation
        state.gyroRot.update();

        // --- 2. Background Rendering (Cyber-Noir) ---
        sprite->fillSprite(COLOR_BG_MAIN); // Deep Black
        
        // Subtle Grid (Darker)
        for (int x=0; x<SCREEN_WIDTH; x+=CELL_W) sprite->drawFastVLine(x, 0, SCREEN_HEIGHT, COLOR_BG_SHADOW);
        for (int y=0; y<SCREEN_HEIGHT; y+=CELL_H) sprite->drawFastHLine(0, y, SCREEN_WIDTH, COLOR_BG_SHADOW);
        
        // Vignette / Scanline
        int scanY = state.bgScan.y();
        if (scanY < SCREEN_HEIGHT) sprite->drawFastHLine(0, scanY, SCREEN_WIDTH, COLOR_BG_PANEL);
        
        // --- 3. Main Tachometer (Center) ---
        int cx = SCREEN_WIDTH / 2;
        int cy = 75;
        int r = 50;
        
        // Outer Tech Ring
        sprite->drawCircle(cx, cy, r, COLOR_BG_PANEL);
        
        int tVal = (int)state.throttleVal.val();
        
        // RPM Arcs
        int startAngle = 135;
        int sweep = map(abs(tVal), 0, 100, 0, 270);
        
        // Dynamic Color based on speed
        uint16_t arcColor = COLOR_ACCENT_3; // Cyan
        if (abs(tVal) > 50) arcColor = COLOR_WARNING;
        if (abs(tVal) > 85) arcColor = COLOR_DANGER;
        
        // Draw Arc
        for (int i=0; i<sweep; i+=3) {
             float rad = (startAngle + i) * DEG_TO_RAD;
             int rx = cx + cos(rad) * (r-2);
             int ry = cy + sin(rad) * (r-2);
             sprite->drawPixel(rx, ry, arcColor);
             // Thick
             rx = cx + cos(rad) * (r-3);
             ry = cy + sin(rad) * (r-3);
             sprite->drawPixel(rx, ry, arcColor);
        }
        
        // Center Speedo (Rolling Counter)
        int dispSpeed = map(abs(tVal), 0, 100, 0, 999);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawNumber(dispSpeed, cx, cy - 5, FONT_DIGIT);
        
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_MAIN);
        sprite->drawString("KPH", cx, cy + 20, FONT_MICRO);

        // --- 4. Steering (Bottom) ---
        int steerY = 135;
        int sVal = (int)state.steerVal.val();
        int bW = (SCREEN_WIDTH - 30) / 2;
        
        sprite->drawFastHLine(15, steerY, SCREEN_WIDTH-30, COLOR_BG_PANEL);
        sprite->drawFastVLine(cx, steerY-3, 6, COLOR_TEXT_MUTED); // Center mark
        
        if (sVal != 0) {
            int len = map(abs(sVal), 0, 100, 0, bW);
            if (sVal > 0) sprite->fillRect(cx, steerY-1, len, 3, COLOR_ACCENT_3);
            else sprite->fillRect(cx-len, steerY-1, len, 3, COLOR_ACCENT_3);
        }

        // --- 5. Suspension Gauge (New Feature) ---
        // Vertical Bar on Right
        int suspVal = inputManager.currentState.potSuspension; // 0-4095
        int sH = map(suspVal, 0, 4095, 0, 60);
        int barX = SCREEN_WIDTH - 8;
        int barY = 40;
        int barMax = 60;
        
        // Container
        sprite->drawRect(barX, barY, 4, barMax, COLOR_BG_PANEL);
        // Fill
        sprite->fillRect(barX+1, barY + (barMax - sH), 2, sH, COLOR_ACCENT_2); // Green fill
        
        // Label
        sprite->setTextDatum(TC_DATUM);
        sprite->drawString("S", barX+2, barY - 10, FONT_MICRO);


        // --- 6. GYRO TARGET SYSTEM (The "Target" Request) ---
        float gAnim = state.gyroAnim.val();
        
        if (gAnim > 1.0f) {
            // "Sniper" Scope Effect
            int size = map((long)gAnim, 0, 100, 0, 60);
            float rotation = state.gyroRot.val();
            
            // 4 Corner Brackets expanding in
            // Use polar coordinates for rotation
            for (int i=0; i<4; i++) {
                float ang = (rotation + (i * 90)) * DEG_TO_RAD;
                int x = cx + cos(ang) * size;
                int y = cy + sin(ang) * size;
                sprite->drawCircle(x, y, 2, COLOR_ACCENT_1);
            }
            
            // Inner Reticle
            if (gAnim > 80) {
                 sprite->drawCircle(cx, cy, 15, COLOR_ACCENT_1);
                 sprite->drawLine(cx-20, cy, cx+20, cy, COLOR_ACCENT_1);
                 sprite->drawLine(cx, cy-20, cx, cy+20, COLOR_ACCENT_1);
                 
                 sprite->setTextColor(COLOR_ACCENT_1, COLOR_BG_MAIN);
                 sprite->drawString("TARGET LOCKED", cx, cy - 40, FONT_MICRO);
            }
        }

        // --- 7. Status Header ---
        bool linked = commsManager.isConnected();
        sprite->setTextColor(linked ? COLOR_ACCENT_2 : COLOR_ACCENT_1, COLOR_BG_MAIN);
        sprite->setTextDatum(TL_DATUM);
        sprite->drawString(linked ? "[LINK]" : "[VOID]", 4, 4, FONT_MICRO);
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
