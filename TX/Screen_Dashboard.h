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
    AnimFloat gyroAnim; // 0.0 to 1.0 expansion
    bool lastGyroState;
    
    ScanLine bgScan;
    
    MicroJitter jitter; // For "Realism" noise
    
    DashboardState() : 
        throttleVal(0, 0.15f, 0.85f), 
        steerVal(0, 0.2f, 0.7f), 
        gyroAnim(0, 0.1f, 0.9f),
        bgScan(0, SCREEN_HEIGHT, 1.5f),
        jitter(2),
        lastGyroState(false)
    {}
};

class ScreenDashboard {
private:
    DashboardState state;
    bool needsFullRedraw = true;

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
                state.gyroAnim.snap(0); // Reset
                state.gyroAnim.target = 100.0f; // Expand
            } else {
                state.gyroAnim.target = 0.0f; // Collapse
            }
            state.lastGyroState = currentGyro;
        }
        state.gyroAnim.update();

        // --- 2. Background Rendering (Complex) ---
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Draw Grid
        // sprite->setColor(COLOR_BG_SHADOW); // Deleted, not supported
        for (int x=0; x<SCREEN_WIDTH; x+=CELL_W) sprite->drawFastVLine(x, 0, SCREEN_HEIGHT, COLOR_BG_SHADOW);
        for (int y=0; y<SCREEN_HEIGHT; y+=CELL_H) sprite->drawFastHLine(0, y, SCREEN_WIDTH, COLOR_BG_SHADOW);
        
        // Draw Scanning Line (Cyber Effect)
        sprite->drawFastHLine(0, state.bgScan.y(), SCREEN_WIDTH, COLOR_ACCENT_2);
        
        // --- 3. Main Tachometer (Overengineered) ---
        int cx = SCREEN_WIDTH / 2;
        int cy = 70;
        int r = 52;
        
        // Draw "Tech" Ring
        // Dashed Circle
        for (int angle = 0; angle < 360; angle += 15) {
             float rad = angle * DEG_TO_RAD;
             int x1 = cx + cos(rad) * r;
             int y1 = cy + sin(rad) * r;
             int x2 = cx + cos(rad) * (r+4);
             int y2 = cy + sin(rad) * (r+4);
             sprite->drawLine(x1, y1, x2, y2, COLOR_TEXT_SUB);
        }
        
        // Throttle Arc
        // Map -100..100 to Angle 135..405
        float tVal = state.throttleVal.val(); // Smoothed
        int startAngle = 135;
        int sweep = map((long)abs(tVal), 0, 100, 0, 270);
        
        // If reversing, change color
        uint16_t barColor = (tVal < 0) ? COLOR_ACCENT_3 : COLOR_ACCENT_2;
        if (abs(tVal) > 90) barColor = COLOR_ACCENT_1; // Critical Red at high speed
        
        // Draw Arc (manually for thickness)
        for (int i=0; i<sweep; i+=4) {
             float rad = (startAngle + i) * DEG_TO_RAD;
             // Add jitter to radius for "Power Surge" effect
             int jitterR = (abs(tVal) > 50) ? state.jitter.get() : 0;
             int x = cx + cos(rad) * (r - 2 + jitterR);
             int y = cy + sin(rad) * (r - 2 + jitterR);
             sprite->fillCircle(x, y, 3, barColor);
        }
        
        // Digital Readout (Rolling Counter Simulation)
        int dispSpeed = map((long)abs(tVal), 0, 100, 0, 999);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawNumber(dispSpeed, cx, cy, FONT_DIGIT);
        
        // Decoration Text
        sprite->setTextColor(COLOR_TEXT_MUTED, COLOR_BG_MAIN);
        sprite->setTextDatum(TC_DATUM);
        sprite->drawString("VELOCITY", cx, cy + 30, FONT_MICRO);

        // --- 4. Steering Bar (Bottom) ---
        int steerY = 130;
        float sVal = state.steerVal.val();
        int center = SCREEN_WIDTH/2;
        int maxW = (SCREEN_WIDTH-20)/2;
        
        // Frame
        sprite->drawRect(8, steerY-2, SCREEN_WIDTH-16, 8, COLOR_TEXT_SUB);
        
        // Bar
        if (sVal > 0) {
            int w = map((long)sVal, 0, 100, 0, maxW);
            sprite->fillRect(center, steerY, w, 4, COLOR_ACCENT_4);
        } else {
             int w = map((long)abs(sVal), 0, 100, 0, maxW);
             sprite->fillRect(center-w, steerY, w, 4, COLOR_ACCENT_4);
        }
        
        // --- 5. Connection Status (Top Left) ---
        bool linked = commsManager.isConnected();
        uint16_t linkColor = linked ? COLOR_SUCCESS : COLOR_DANGER;
        if (!linked && (millis()/200)%2==0) linkColor = COLOR_BG_MAIN; // Fast Blink
        
        sprite->setTextDatum(TL_DATUM);
        sprite->setTextColor(linkColor, COLOR_BG_MAIN);
        sprite->drawString(linked ? "LINK ACTIVE" : "NO CARRIER", 5, 5, FONT_LABEL);
        
        // --- 6. GYRO ACTIVATION SEQUENCE (The "Cool" Animation) ---
        // If Gyro is ON, draw the HUD elements
        float gAnim = state.gyroAnim.val(); // 0 to 100
        
        if (gAnim > 1.0f) {
            // Draw Target Reticle expanding
            int size = map((long)gAnim, 0, 100, 0, 60);
            int alpha = map((long)gAnim, 0, 100, 255, 100); // Fade? No alpha support in basic lib, simulate with line skipping?
            
            // Draw Crosshairs
            sprite->drawCircle(cx, cy, size, COLOR_ACCENT_3);
            sprite->drawCircle(cx, cy, size/2, COLOR_ACCENT_3);
            
            sprite->drawLine(cx-size-10, cy, cx+size+10, cy, COLOR_ACCENT_3);
            sprite->drawLine(cx, cy-size-10, cx, cy+size+10, COLOR_ACCENT_3);
            
            // "Lock On" Text
            if (gAnim > 80) {
                 sprite->setTextColor(COLOR_ACCENT_3, COLOR_BG_MAIN); // No bg fill to overlay
                 sprite->drawString("GYRO STABILIZED", cx, cy - size - 15, FONT_MICRO);
            }
        }
        
        // --- 7. Footer Info ---
        sprite->setTextDatum(BC_DATUM);
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_MAIN);
        String trimStr = "AXIS TRIM: " + String(inputManager.currentState.trimLevel);
        sprite->drawString(trimStr, cx, SCREEN_HEIGHT-2, FONT_MICRO);
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
