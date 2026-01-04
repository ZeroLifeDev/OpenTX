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
    float gyroAngle = 0;
    
    // Cutscene State
    bool lastGyroState = false;
    unsigned long gyroAnimStart = 0;
    bool isAnimating = false;

public:
    ScreenDashboard() : needleAnim(0, 0.1, 0.85), steerAnim(0, 0.15, 0.8) {}

    void draw(InputState &s) {
        // Gyro Edge Detection for Animation
        if (s.swGyro != lastGyroState) {
            isAnimating = true;
            gyroAnimStart = millis();
            if(s.swGyro) sound.playGyroOn();
            else sound.playGyroOff();
            lastGyroState = s.swGyro;
        }

        // --- CUTSCENE OVERLAY ---
        if (isAnimating) {
            unsigned long progress = millis() - gyroAnimStart;
            if (progress > 800) {
                isAnimating = false; // End animation
            } else {
                // Draw Cutscene
                sprite.fillSprite(TFT_BLACK);
                uint16_t color = s.swGyro ? 0x07E0 : 0xF800; // Green vs Red
                const char* txt = s.swGyro ? "GYRO ACTIVE" : "GYRO OFF";
                
                // Blink Logic
                if ((progress / 100) % 2 == 0) {
                    sprite.fillRect(0, 60, 128, 40, color);
                    sprite.setTextColor(TFT_BLACK, color);
                } else {
                    sprite.drawRect(0, 60, 128, 40, color);
                    sprite.setTextColor(color, TFT_BLACK);
                }
                
                sprite.setTextDatum(MC_DATUM);
                sprite.drawString(txt, 64, 80, 4);
                return; // STOP DRAWING DASHBOARD
            }
        }

        // --- NORMAL DASHBOARD ---
        sprite.fillSprite(COLOR_BG_MAIN);

        // Update Physics
        needleAnim.set(abs(s.throttlePct));
        needleAnim.update();
        steerAnim.set(s.steerPct);
        steerAnim.update();
        
        if (s.swGyro) {
            gyroAngle += 5;
            if (gyroAngle > 360) gyroAngle = 0;
        }

        int cx = 64, cy = 78;
        
        // Header Bar
        sprite.fillRect(0, 0, 128, 20, COLOR_BG_PANEL);
        sprite.drawFastHLine(0, 20, 128, COLOR_TEXT_DIM);

        // GYRO STATUS (Top Left)
        int gx = 20, gy = 10;
        if (s.swGyro) {
             // Rotating Radar
             float rad = gyroAngle * DEG_TO_RAD;
             sprite.drawCircle(gx, gy, 6, COLOR_ACCENT_PRI);
             sprite.drawLine(gx, gy, gx + cos(rad)*6, gy + sin(rad)*6, COLOR_ACCENT_PRI);
        } else {
             sprite.drawCircle(gx, gy, 6, COLOR_TEXT_DIS);
        }
        
        // BATT (Top Right)
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite.setTextDatum(MR_DATUM);
        sprite.drawString("TX 4.2v", 120, 10, 1);

        // Suspension Bar
        int suspH = map(s.rawSuspension, 0, 4095, 0, 50);
        sprite.fillRect(4, 150 - suspH, 4, suspH, COLOR_ACCENT_TER);
        sprite.drawRect(3, 99, 6, 52, COLOR_TEXT_DIS);

        // Main Arc
        sprite.drawSmoothArc(cx, cy, 42, 38, 135, 405, COLOR_BG_PANEL, COLOR_BG_MAIN, true);
        float nVal = needleAnim.val();
        if(nVal > 1) {
            int endAng = 135 + (int)(nVal / 100.0f * 270.0f);
            uint16_t col = (s.throttlePct >= 0) ? COLOR_ACCENT_PRI : COLOR_ACCENT_SEC;
            sprite.drawSmoothArc(cx, cy, 42, 38, 135, endAng, col, COLOR_BG_MAIN, true);
        }
        
        // Value
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawNumber((int)s.speedKmh, cx, cy, 6);
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextColor(COLOR_TEXT_DIS, COLOR_BG_MAIN);
        sprite.drawString("KM/H", cx, cy + 18, 1);

        // Steering
        int barY = 135;
        sprite.fillRect(15, barY, 98, 4, COLOR_BG_PANEL);
        int sPx = (int)(steerAnim.val() / 100.0f * 49); 
        int startX = 64;
        if (sPx < 0) startX += sPx;
        if (abs(sPx) > 1) sprite.fillRect(startX, barY, abs(sPx), 4, COLOR_ACCENT_PRI);
        sprite.drawFastVLine(64, barY-2, 8, COLOR_TEXT_DIS);
    }
};

static ScreenDashboard dashboard;

#endif
