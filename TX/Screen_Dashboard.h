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
    
    // Rotating Hexagon Gyro
    AnimFloat gyroAnim; // 0-100 expansion/opacity
    float gyroAngle = 0;
    bool lastGyroState;
    
    MicroJitter jitter; 
    
    DashboardState() : 
        throttleVal(0, 0.15f, 0.85f), 
        steerVal(0, 0.2f, 0.7f), 
        gyroAnim(0, 0.1f, 0.85f),
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
        
        // Updates
        state.throttleVal.target = inputManager.getThrottleNormalized();
        state.throttleVal.update();
        state.steerVal.target = inputManager.getSteeringNormalized();
        state.steerVal.update();
        
        // Gyro
        bool currentGyro = inputManager.currentState.swGyro;
        if (currentGyro != state.lastGyroState) {
            if (currentGyro) {
                 state.gyroAnim.target = 100.0f; 
                 // Play Tone via Manager (handled in UI loop or here, added here for user request of "More Feedback")
                 soundManager.playGyroOn();
            } else {
                 state.gyroAnim.target = 0.0f;
                 soundManager.playGyroOff();
            }
            state.lastGyroState = currentGyro;
        }
        state.gyroAnim.update();
        if (currentGyro) state.gyroAngle += 4.0f; // Spin
        if (state.gyroAngle > 360) state.gyroAngle = 0;

        // --- DRAWING ---
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // NO SCANLINE (User requested removal)
        // sprite->drawFastHLine(0, state.bgScan.y(), SCREEN_WIDTH, COLOR_BG_PANEL);

        // Grid (Subtle)
        for (int y=0; y<SCREEN_HEIGHT; y+=25) {
             sprite->drawFastHLine(0, y, SCREEN_WIDTH, COLOR_BG_SHADOW);
        }

        int cx = SCREEN_WIDTH / 2;
        int cy = 70;
        int r = 54;
        
        // 1. Center Tach (Tech Hexagon Ring)
        // Draw a static hexagon approximation or just circle for now to be safe/clean
        sprite->drawCircle(cx, cy, r, COLOR_BG_PANEL);
        sprite->drawCircle(cx, cy, r-2, COLOR_BG_PANEL);

        // RPM Arc (Dotted)
        int tVal = (int)state.throttleVal.val();
        int angle = map(abs(tVal), 0, 100, 0, 270);
        int start = 135;
        
        uint16_t col = COLOR_ACCENT_2; // Cyan
        if (abs(tVal) > 85) col = COLOR_DANGER;
        
        for (int i=0; i<angle; i+=5) {
             float rad = (start + i) * DEG_TO_RAD;
             int x = cx + cos(rad) * (r-4);
             int y = cy + sin(rad) * (r-4);
             sprite->fillCircle(x, y, 2, col);
        }
        
        // Speed
        int kph = map(abs(tVal), 0, 100, 0, 120);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawNumber(kph, cx, cy-5, FONT_DIGIT);
        sprite->drawString("KMH", cx, cy+22, FONT_MICRO);

        // 2. Gyro Animation (Cool Rotating Hexagon)
        float g = state.gyroAnim.val();
        if (g > 10.0f) { // If active/fading
             // Draw dynamic hexagon
             float rad = map(g, 0, 100, 10, r-10); // Expand
             float alpha = state.gyroAngle;
             
             // Vertex calculation
             for (int i=0; i<6; i++) {
                 float a1 = (alpha + (i*60)) * DEG_TO_RAD;
                 float a2 = (alpha + ((i+1)*60)) * DEG_TO_RAD;
                 
                 int x1 = cx + cos(a1) * rad;
                 int y1 = cy + sin(a1) * rad;
                 int x2 = cx + cos(a2) * rad;
                 int y2 = cy + sin(a2) * rad;
                 
                 sprite->drawLine(x1, y1, x2, y2, COLOR_ACCENT_4);
             }
             
             if (g > 80) {
                 sprite->setTextColor(COLOR_ACCENT_4, COLOR_BG_MAIN);
                 sprite->drawString("STABLE", cx, cy - 30, FONT_MICRO);
             }
        }

        // 3. Trim / Steering
        int steerY = 135;
        int sVal = (int)state.steerVal.val();
        int bW = 50;
        
        sprite->drawRect(cx - bW, steerY, bW*2, 6, COLOR_BG_PANEL);
        int barLen = map(abs(sVal), 0, 100, 0, bW);
        if (sVal > 0) sprite->fillRect(cx, steerY+1, barLen, 4, COLOR_ACCENT_2);
        else sprite->fillRect(cx-barLen, steerY+1, barLen, 4, COLOR_ACCENT_2);
        
        // 4. Suspension (Right Bar)
        int sus = inputManager.currentState.potSuspension;
        int susH = map(sus, 0, 4095, 0, 50);
        sprite->drawRect(SCREEN_WIDTH-8, 50, 4, 50, COLOR_BG_PANEL);
        sprite->fillRect(SCREEN_WIDTH-7, 100-susH, 2, susH, COLOR_WARNING); // Yellow fill

        // 5. Status
        bool linked = commsManager.isConnected();
        sprite->setTextDatum(TL_DATUM);
        sprite->setTextColor(linked ? COLOR_SUCCESS : COLOR_DANGER, COLOR_BG_MAIN);
        sprite->drawString(linked ? "CONN" : "DISC", 4, 4, FONT_LABEL);
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
