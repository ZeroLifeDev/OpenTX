#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "ModelManager.h"
#include "Mixer.h"
#include "Theme.h"
#include "CommsManager.h"
#include "AnimationUtils.h"
#include "GraphicsUtils.h"
#include "InputManager.h"
#include "SoundManager.h"
#include "Screen_Notification.h"

// Access global notification system (defined in Screen_Notification.h)
extern ScreenNotification notificationSystem;

// ==========================================
//          OPENTX PRO DASHBOARD
// ==========================================
// Layout: Audi Virtual Cockpit Style
// Center: Performance Tachometer
// Header: Status Bar
// Footer: Telemetry Strip

class ScreenDashboard {
private:
    // Animations
    AnimFloat needleAnim;
    AnimFloat steerAnim;
    AnimFloat batAnim;
    
    // State Tracking for FX
    bool lastGyro = false;
    int lastTrim = 0;
    unsigned long flashTimer = 0;
    int flashColor = 0; // 0=None, 1=Pri, 2=Sec
    
public:
    ScreenDashboard() : 
        needleAnim(0, 0.25, 0.65), // Springier
        steerAnim(0, 0.35, 0.60),
        batAnim(8.0, 0.05, 0.9)
    {}

    void init() {
        needleAnim.snap(0);
        steerAnim.snap(0);
        // Sync initial state to avoid boom on startup
        lastGyro = inputManager.currentState.swGyro;
        lastTrim = inputManager.internalTrim;
    }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        ModelData* m = modelManager.getModel();
        
        // 1. BG & Grid
        GraphicsUtils::fillGradientRect(sprite, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG_MAIN, COLOR_BG_DIM);
        // GraphicsUtils::drawTechGrid(sprite); // Deprecated

        // --- INTERACTION LOGIC (Cinematic) ---
        // Gyro Toggle
        if (inputManager.currentState.swGyro != lastGyro) {
            lastGyro = inputManager.currentState.swGyro;
            soundManager.playGyroEffect(lastGyro);
            
            // Trigger Cutscene
            if (lastGyro) {
                notificationSystem.show("GYRO", "SYSTEM ARMED", COLOR_ACCENT_TER);
            } else {
                notificationSystem.show("MANUAL", "STABILITY OFF", COLOR_ACCENT_SEC);
            }
        }
        
        // Trim Change (Keep subtle click, maybe small popup?)
        if (inputManager.internalTrim != lastTrim) {
            lastTrim = inputManager.internalTrim;
            soundManager.playClick();
        }
        
        // Data Updates

        // --- PRE-CALC ANIMATIONS ---
        int val = mixer.getMsgThrottle();       
        int rawThr = (val > 0) ? map(val, 0, 100, 0, 120) : 0; 
        needleAnim.set((float)rawThr); 
        needleAnim.update();
        steerAnim.set((float)inputManager.getSteeringNormalized());
        steerAnim.update();
        
        bool con = commsManager.isConnected();
        batAnim.set(commsManager.getRxVoltage() > 1.0 ? commsManager.getRxVoltage() : 8.2);
        batAnim.update();
        
        // --- PASS 1: GRAPHICS & SHAPES (No Fonts) ---
        
        // HEADER BAR (Card)
        sprite->fillRoundRect(2, 2, SCREEN_WIDTH-4, 24, 4, COLOR_BG_PANEL);
        if (con) { 
             // Active Link Indicator (Small dot)
             sprite->fillCircle(10, 14, 3, (millis()/500)%2==0 ? COLOR_ACCENT_TER : COLOR_BG_MAIN);
        } else {
             sprite->fillCircle(10, 14, 3, COLOR_ACCENT_SEC);
        }

        // SUSPENSION BAR (Clean Pill)
        int susH = 70;
        int susW = 6;
        int susX = 5;
        int susY = 45;
        // Background track
        sprite->fillRoundRect(susX, susY, susW, susH, 3, COLOR_BG_PANEL);
        // Active Fill
        int susVal = inputManager.currentState.potSuspension;
        int fillH = map(susVal, 0, 4095, 0, susH);
        uint16_t susCol = (fillH < susH/2) ? COLOR_ACCENT_TER : COLOR_ACCENT_SEC;
        // Draw from bottom
        sprite->fillRoundRect(susX, susY + (susH - fillH), susW, fillH, 3, susCol);

        // MAIN CENTER GAUGE (Modern Ring)
        int cx = SCREEN_WIDTH / 2 + 5;
        int cy = 80; // Lower center
        int r = 48; 
        
        // Background Ring (Dark)
        sprite->drawSmoothArc(cx, cy, r, r-5, 45, 315, COLOR_BG_PANEL, COLOR_BG_MAIN, true);
        
        // Active Arc (Neon)
        float nPct = needleAnim.val() / 120.0f; // 0.0 to 1.0
        if (nPct > 0.01) {
            int endAng = 45 + (int)(nPct * 270);
            uint16_t arcCol = COLOR_ACCENT_PRI;
            if (nPct > 0.7) arcCol = COLOR_ACCENT_SEC; // Redline
            sprite->drawSmoothArc(cx, cy, r, r-5, 45, endAng, arcCol, COLOR_BG_MAIN, true);
        }

        // Center Hub (Glassy)
        sprite->fillCircle(cx, cy, 22, COLOR_BG_PANEL);
        sprite->drawCircle(cx, cy, 22, COLOR_BORDER);

        // WIDGET CARDS (Clean Rounded Rects)
        int wy = 125;
        // Trim
        sprite->fillRoundRect(15, wy, 50, 30, 4, COLOR_BG_PANEL);
        // Gyro
        sprite->fillRoundRect(70, wy, 50, 30, 4, COLOR_BG_PANEL);


        // --- PASS 2: TEXT (Custom Fonts) ---
        
        // 1. HEADERS (Bold)
        display->loadFont(FONT_PATH_BOLD);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->setTextDatum(TL_DATUM);
        sprite->drawString(m->name, 20, 5); // Beside dot
        display->unloadFont();

        // 2. DATA VALUES (Regular)
        display->loadFont(FONT_PATH_REG);
        
        // TX Volts
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_PANEL);
        char vBuf[8]; sprintf(vBuf, "%.1v", batAnim.val());
        sprite->setTextDatum(TR_DATUM);
        sprite->drawString(vBuf, SCREEN_WIDTH-8, 7);

        // Labels in Widgets
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_PANEL);
        sprite->setTextDatum(TC_DATUM);
        sprite->drawString("TRIM", 40, wy+2); 
        sprite->drawString("GYRO", 95, wy+2);
        
        // Widget Values
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_PANEL);
        sprite->drawNumber(inputManager.internalTrim, 40, wy+14);
        
        // Gyro Value
        if (inputManager.currentState.swGyro) {
             sprite->setTextColor(COLOR_ACCENT_TER, COLOR_BG_PANEL);
             sprite->drawString("ON", 95, wy+14);
        } else {
             sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_PANEL);
             sprite->drawString("OFF", 95, wy+14);
        }
        
        // Speed Label
        sprite->setTextColor(COLOR_ACCENT_GOLD, COLOR_BG_PANEL);
        sprite->drawString("KMH", cx, cy + 12);
        
        display->unloadFont();

        // 3. DIGITAL SPEED (Huge DSEG)
        display->loadFont(FONT_PATH_DIGIT);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        int spd = (int)commsManager.getSpeed();
        // Adjust Y for font baseline
        sprite->drawNumber(spd, cx, cy - 5); 
        display->unloadFont();

        // --- LOGIC HANDLING ---
        if (inputManager.currentState.swGyro != lastGyro) {
            lastGyro = inputManager.currentState.swGyro;
            soundManager.playGyroEffect(lastGyro);
            if (lastGyro) notificationSystem.show("GYRO", "ARMED", COLOR_ACCENT_TER);
            else notificationSystem.show("GYRO", "OFF", COLOR_ACCENT_SEC);
        }
        if (inputManager.internalTrim != lastTrim) {
            lastTrim = inputManager.internalTrim;
            soundManager.playClick();
        }

        // START/STOP Overlay handled by UIManager? 
        if (notificationSystem.isActive()) {
            notificationSystem.update();
            notificationSystem.draw(display);
        }
    }
    
    // Helper for thick arc segments
    void drawArcSegment(TFT_eSprite* s, int cx, int cy, int r, int w, float startDeg, float sweepDeg, uint16_t col) {
        // Draw multiple lines to simulate thickness
        // Center arc
        float midR = r - (w/2.0f);
        float startRad = startDeg * DEG_TO_RAD;
        float endRad = (startDeg + sweepDeg) * DEG_TO_RAD;
        
        // We can use a simple line loop for filling
        int steps = 2; 
        for(int i=0; i<w; i++) {
             int currR = r - i;
             int x1 = cx + cos(startRad) * currR;
             int y1 = cy + sin(startRad) * currR;
             int x2 = cx + cos(endRad) * currR;
             int y2 = cy + sin(endRad) * currR;
             s->drawLine(x1, y1, x2, y2, col);
        }
    }
};

#endif // SCREEN_DASHBOARD_H
