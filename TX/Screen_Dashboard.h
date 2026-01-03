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
        GraphicsUtils::drawTechGrid(sprite);

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
        int val = mixer.getMsgThrottle();       
        int rawThr = (val > 0) ? map(val, 0, 100, 0, 120) : 0; 
        needleAnim.set((float)rawThr); 
        needleAnim.update();
        
        steerAnim.set((float)inputManager.getSteeringNormalized());
        steerAnim.update();
        
        batAnim.set(commsManager.getRxVoltage() > 1.0 ? commsManager.getRxVoltage() : 8.2); // Fallback
        batAnim.update();

        // 2. HEADER HUD (Refined)
        // Draw Solid Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 22, COLOR_BG_PANEL);
        sprite->drawFastHLine(0, 22, SCREEN_WIDTH, COLOR_ACCENT_PRI);
        
        // Pulsing "Connected" or "Scanning" indicator
        bool con = commsManager.isConnected();
        uint16_t conCol = con ? COLOR_ACCENT_TER : COLOR_ACCENT_SEC;
        
        if (con) {
             sprite->fillRect(0, 0, 3, 22, conCol); // Left strip
             // Data Flow animation
             int flowX = (millis() / 50) % (SCREEN_WIDTH - 60);
             sprite->drawPixel(flowX + 10, 21, COLOR_ACCENT_PRI);
        } else {
             // Blink strip
             if ((millis()/500)%2==0) sprite->fillRect(0, 0, 3, 22, conCol);
             sprite->setTextColor(COLOR_ACCENT_SEC, COLOR_BG_PANEL);
             sprite->setTextDatum(TC_DATUM);
             sprite->drawString("NO LINK", SCREEN_WIDTH/2, 2, FONT_SMALL);
        }

        // Model Name
        if (con) {
            sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
            sprite->setTextDatum(TL_DATUM);
            sprite->drawString(m->name, 8, 4, FONT_MED);
        }
        
        // TX Volts
        char vBuf[8];
        sprintf(vBuf, "%.1v", batAnim.val());
        sprite->setTextDatum(TR_DATUM);
        sprite->setTextColor(con ? COLOR_ACCENT_PRI : COLOR_TEXT_DIM, COLOR_BG_PANEL); 
        sprite->drawString(vBuf, SCREEN_WIDTH-4, 4, FONT_MED);

        // 3. SUSPENSION BAR (Left Side)
        // Vertical Bar
        int susW = 6;
        int susH = 80;
        int susX = 4;
        int susY = 40;
        
        sprite->drawRect(susX, susY, susW, susH, COLOR_BORDER);
        // Fill based on pot
        int susVal = inputManager.currentState.potSuspension; // 0-4095
        int fillH = map(susVal, 0, 4095, 0, susH-2);
        // Gradient fill logic (Green -> Yellow -> Red) (Inverse: Soft -> Stiff)
        uint16_t susCol = (fillH < susH/2) ? COLOR_ACCENT_TER : COLOR_ACCENT_SEC;
        
        sprite->fillRect(susX+1, susY + (susH-2) - fillH + 1, susW-2, fillH, susCol);
        
        // Label
        sprite->setTextDatum(TC_DATUM);
        sprite->setTextColor(COLOR_TEXT_DIM);
        sprite->drawString("S", susX+3, susY-10, FONT_SMALL);

        // 4. MAIN GAUGE (Center - Neon Arc)
        int cx = SCREEN_WIDTH / 2 + 5; // Shift right slightly due to Susp bar
        int cy = 70;
        int r = 48;
        
        // Draw Dynamic Arc (No ticks, solid flow)
        // We simulate a thick arc using multiple circles or lines
        float nVal = needleAnim.val();
        float nPct = nVal / 120.0f;
        
        // Draw Background Ring
        for(int i=0; i<360; i+=4) { // Dotted background
             if (i > 130 && i < 410) continue; // Skip active area inverse? 
             // Active area is 135 to 405 (270 deg)
             // Let's just draw the active range static grey
        }
        
        int startAng = 135;
        int sweep = 270;
        
        // Draw Segmented "Digital" Arc
        // 20 segments
        for(int i=0; i<20; i++) {
             float segPct = i / 20.0f;
             if (segPct > nPct) {
                 // Dim Segment
                 drawArcSegment(sprite, cx, cy, r, 6, startAng + (segPct*sweep), sweep/20 - 2, COLOR_BG_PANEL); // Empty slot style
             } else {
                 // Lit Segment
                 uint16_t col = COLOR_ACCENT_PRI;
                 if (segPct > 0.6) col = COLOR_ACCENT_SEC; // Redline
                 drawArcSegment(sprite, cx, cy, r, 6, startAng + (segPct*sweep), sweep/20 - 2, col);
             }
        }

        // --- HUB & SPEED ---
        sprite->fillCircle(cx, cy, 18, COLOR_BG_PANEL); 
        sprite->drawCircle(cx, cy, 18, COLOR_BORDER); 
        
        int speedVal = (int)commsManager.getSpeed();
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL); 
        sprite->setTextDatum(MC_DATUM);
        sprite->drawNumber(speedVal, cx, cy - 2, FONT_NUMS);
        
        sprite->setTextColor(COLOR_ACCENT_GOLD, COLOR_BG_PANEL);
        sprite->drawString("KMH", cx, cy + 24, FONT_SMALL);

        // 5. WIDGETS (Bottom)
        int wy = 125;
        
        // Trim Box (Flash on update)
        // uint16_t trimBorder = (millis() - flashTimer < 200 && lastTrim != inputManager.internalTrim) ? COLOR_TEXT_MAIN : COLOR_BORDER; // Removed flash logic
        
        GraphicsUtils::drawCornerWidget(sprite, 15, wy, 50, 30, COLOR_BORDER); // Changed to static border
        sprite->setTextDatum(TC_DATUM);
        sprite->setTextColor(COLOR_ACCENT_PRI);
        sprite->drawString("TRM", 40, wy+3, FONT_SMALL);
        sprite->drawNumber(inputManager.internalTrim, 40, wy+14, FONT_MED);
        
        // Gyro Box
        uint16_t gyroCol = inputManager.currentState.swGyro ? COLOR_ACCENT_TER : COLOR_TEXT_DIM;
        GraphicsUtils::drawCornerWidget(sprite, 70, wy, 50, 30, COLOR_BORDER);
        sprite->setTextDatum(TC_DATUM);
        sprite->setTextColor(COLOR_ACCENT_SEC);
        sprite->drawString("GYR", 95, wy+3, FONT_SMALL);
        sprite->setTextColor(gyroCol);
        sprite->drawString(inputManager.currentState.swGyro ? "ON" : "OFF", 95, wy+14, FONT_MED);
        
        // 6. STEERING footer
        // Split bar
        int sx = SCREEN_WIDTH/2;
        int sy = 158;
        float sVal = steerAnim.val(); 
        int sLen = map(abs((int)sVal), 0, 100, 0, 60);
        
        sprite->drawFastVLine(sx, 154, 6, COLOR_TEXT_DIM);
        if (sVal > 0) sprite->fillRect(sx+2, sy, sLen, 2, COLOR_ACCENT_PRI);
        else sprite->fillRect(sx-2-sLen, sy, sLen, 2, COLOR_ACCENT_PRI);
        
        // --- OVERLAY LAYER ---
        // Render Cutscene on top of everything if active
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
