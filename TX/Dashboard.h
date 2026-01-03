#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Types.h"
#include "Theme.h"

extern TFT_eSprite sprite; // Defined in TX.ino

class ScreenDashboard {
private:
    AnimFloat needleAnim;
    AnimFloat steerAnim;
    float gyroAngle = 0; // For animation

public:
    ScreenDashboard() : needleAnim(0, 0.2), steerAnim(0, 0.3) {}

    void draw(InputState &s) {
        // Clear BG
        sprite.fillSprite(COLOR_BG_MAIN);

        // Update Physics
        needleAnim.set(abs(s.throttlePct));
        needleAnim.update();
        steerAnim.set(s.steerPct);
        steerAnim.update();
        
        // Gyro Animation Logic
        if (s.swGyro) {
            gyroAngle += 5;
            if (gyroAngle > 360) gyroAngle = 0;
        }

        int cx = 64, cy = 78; // Center
        
        // --- 1. TECH DECOR ---
        // Top Header Bar
        sprite.fillRect(0, 0, 128, 18, COLOR_BG_PANEL);
        sprite.drawFastHLine(0, 18, 128, COLOR_TEXT_DIM);
        
        // Corner Brackets
        sprite.drawFastHLine(5, 30, 15, COLOR_TEXT_DIS);
        sprite.drawFastVLine(5, 30, 15, COLOR_TEXT_DIS);
        sprite.drawFastHLine(108, 30, 15, COLOR_TEXT_DIS);
        sprite.drawFastVLine(123, 30, 15, COLOR_TEXT_DIS);

        // --- 2. HEADER INFO ---
        sprite.setTextColor(COLOR_TEXT_DIM, COLOR_BG_PANEL);
        sprite.setTextDatum(MC_DATUM);
        
        // GYRO ANIMATION (Replaces Text)
        int gx = 64, gy = 9;
        if (s.swGyro) {
            // Active Spinning Icon
            sprite.drawCircle(gx, gy, 6, COLOR_ACCENT_PRI);
            float rad = gyroAngle * DEG_TO_RAD;
            int x1 = gx + cos(rad) * 6;
            int y1 = gy + sin(rad) * 6;
            int x2 = gx - cos(rad) * 6;
            int y2 = gy - sin(rad) * 6;
            sprite.drawLine(x1, y1, x2, y2, COLOR_ACCENT_PRI);
        } else {
            // Static "OFF" Icon
            sprite.drawCircle(gx, gy, 6, COLOR_TEXT_DIS);
            sprite.drawLine(gx-4, gy, gx+4, gy, COLOR_TEXT_DIS);
        }
        
        // Connection Dot (Top Right)
        sprite.fillCircle(118, 9, 4, s.rxConnected ? COLOR_ACCENT_TER : COLOR_ACCENT_SEC);

        // --- 3. SUSPENSION BAR (Left Side) ---
        // Vertical bar showing potentiometer value (0-4095 mapped to height)
        int suspH = map(s.rawSuspension, 0, 4095, 0, 60);
        int suspY = 100; 
        int suspX = 5;
        // Background
        sprite.drawRect(suspX, suspY - 60, 6, 62, COLOR_TEXT_DIS);
        // Fill
        sprite.fillRect(suspX + 1, suspY - suspH, 4, suspH, COLOR_ACCENT_TER);
        // Label
        // sprite.drawString("SUSP", suspX + 3, suspY + 8, 1);

        // --- 4. MAIN THROTTLE ARC ---
        // Base Grey Arc
        sprite.drawSmoothArc(cx, cy, 42, 38, 135, 405, COLOR_BG_PANEL, COLOR_BG_MAIN, true);
        
        float nVal = needleAnim.val();
        if(nVal > 1) {
            int endAng = 135 + (int)(nVal / 100.0f * 270.0f);
            uint16_t col = (s.throttlePct >= 0) ? COLOR_ACCENT_PRI : COLOR_ACCENT_SEC;
            sprite.drawSmoothArc(cx, cy, 42, 38, 135, endAng, col, COLOR_BG_MAIN, true);
        }
        
        // --- 5. CENTRAL SPEEDOMETER ---
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite.setTextDatum(MC_DATUM);
        
        // Big Speed Number
        sprite.drawNumber((int)s.speedKmh, cx, cy, 6); 
        
        // Label
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(COLOR_TEXT_DIS, COLOR_BG_MAIN);
        sprite.drawString("KM/H", cx, cy + 18, 1); 
        
        // --- 6. STEERING STRIP ---
        int barY = 135;
        // Center Marker
        sprite.drawFastVLine(64, barY - 4, 12, COLOR_TEXT_DIS);
        
        // Background Strip
        sprite.fillRect(14, barY, 100, 6, COLOR_BG_PANEL);
        
        // Active Steering Indicator
        int sPx = (int)(steerAnim.val() / 100.0f * 50); 
        int startX = 64;
        int width = abs(sPx);
        if (sPx < 0) startX += sPx; // Shift left for negative
        
        if (width > 1) {
            sprite.fillRect(startX, barY, width, 6, COLOR_ACCENT_PRI);
        }
        
        // Trim Indicator Text
        sprite.setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("TRIM: " + String(s.steerTrim), 64, 153, 1);
    }
};

static ScreenDashboard dashboard;

#endif
