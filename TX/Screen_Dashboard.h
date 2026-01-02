#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

class ScreenDashboard {
private:
    float animPhase = 0; // For pulsing effects

    // Helper to draw a partial arc/ring
    void drawArc(TFT_eSprite* sprite, int x, int y, int r, int startAngle, int endAngle, uint16_t color) {
        // Simple segmented approximation
        for (int i=startAngle; i<=endAngle; i+=3) {
             float rad = i * DEG_TO_RAD;
             int px = x + cos(rad) * r;
             int py = y + sin(rad) * r;
             sprite->drawPixel(px, py, color);
             // Make it thicker
             sprite->drawPixel(px+1, py, color);
             sprite->drawPixel(px, py+1, color);
        }
    }

    void drawHorizontalBar(TFT_eSprite* sprite, int x, int y, int w, int h, int val, int minVal, int maxVal, const char* label, uint16_t color) {
        // Background
        sprite->drawRect(x, y, w, h, COLOR_BG_PANEL);
        
        // Fill
        int valPx = map(val, minVal, maxVal, 0, w-2);
        valPx = constrain(valPx, 0, w-2);
        
        sprite->fillRect(x + 1, y + 1, valPx, h - 2, color);
        
        // Label
        sprite->setTextDatum(BL_DATUM);
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_DARK);
        sprite->drawString(label, x, y - 2, FONT_SMALL);
    }
    
    // New fancy bar for center steering
    void drawCenterBar(TFT_eSprite* sprite, int x, int y, int w, int h, int val, int minVal, int maxVal, const char* label) {
         // Center Line
        int center = x + w/2;
        sprite->drawFastVLine(center, y-2, h+4, COLOR_TEXT_SUB); // Marker
        
        // Bar Container
        sprite->drawRect(x, y, w, h, COLOR_BG_PANEL);
        
        int range = maxVal - minVal;
        int centerVal = (maxVal + minVal) / 2;
        
        int pxOffset = map(val, minVal, maxVal, 0, w);
        int centerPx = w/2;
        
        if (pxOffset > centerPx) {
             sprite->fillRect(x + centerPx, y + 1, pxOffset - centerPx, h - 2, COLOR_ACCENT);
        } else {
             sprite->fillRect(x + pxOffset, y + 1, centerPx - pxOffset, h - 2, COLOR_ACCENT_ALT); 
        }
        
        // Label
        sprite->setTextDatum(TC_DATUM);
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_DARK);
        sprite->drawString(label, center, y + h + 2, FONT_SMALL);
    }

public:
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        // Update Animation Phase
        animPhase += 0.1;
        if (animPhase > 6.28) animPhase = 0;
        
        // 1. Top Status Bar (Tech Style)
        sprite->fillRect(0, 0, SCREEN_WIDTH, 18, COLOR_BG_PANEL);
        sprite->drawRect(0, 0, SCREEN_WIDTH, 18, COLOR_BG_PANEL); // Outline
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString("OpenTX", 4, 9, FONT_SMALL);
        
        // Gyro Indicator in header
        int gyroX = SCREEN_WIDTH / 2;
        bool gyroOn = inputManager.currentState.swGyro;
        sprite->setTextDatum(MC_DATUM);
        if (gyroOn) {
             sprite->setTextColor(COLOR_ACCENT, COLOR_BG_PANEL);
             sprite->drawString("GYRO", gyroX, 9, FONT_SMALL);
        } else {
             sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_PANEL);
             sprite->drawString("GYRO", gyroX, 9, FONT_SMALL);
        }

        // Battery Icon (Animated Pulse if low)
        int batX = SCREEN_WIDTH - 25;
        sprite->drawRect(batX, 4, 18, 10, COLOR_TEXT_MAIN);
        sprite->fillRect(batX + 18, 6, 2, 6, COLOR_TEXT_MAIN);
        
        // Fill
        sprite->fillRect(batX + 2, 6, 12, 6, COLOR_ACCENT); 
        
        // 2. Main Throttle Gauge (Center)
        // Using Throttle Input
        int throttle = inputManager.getThrottleNormalized(); // Renamed from joyY
        int speed = map(abs(throttle), 0, 100, 0, 99);
        
        int centerX = SCREEN_WIDTH / 2;
        int centerY = 70;
        int radius = 45; // reduced slightly
        
        // Rotating Outer Ring Animation
        int rotOffset = (int)(animPhase * 5); // slow rotation
        
        // Draw Ticks
        for (int i = 0; i < 12; i++) {
            float ang = (i * 30 + rotOffset) * DEG_TO_RAD;
            int x1 = centerX + cos(ang) * (radius + 2);
            int y1 = centerY + sin(ang) * (radius + 2);
            int x2 = centerX + cos(ang) * (radius + 6);
            int y2 = centerY + sin(ang) * (radius + 6);
            sprite->drawLine(x1, y1, x2, y2, COLOR_BG_PANEL);
        }
        
        // Main Circle
        sprite->drawCircle(centerX, centerY, radius, COLOR_BG_PANEL);
        
        // Active Throttle Arc
        int segments = 20; // Reduced
        float angleStep = 270.0 / segments;
        int startAngle = 135; // Bottom Left-ish
        
        int activeSegs = map(abs(throttle), 0, 100, 0, segments);
        
        for (int i = 0; i < segments; i++) {
             float angle = startAngle + (i * angleStep);
             float rad = angle * DEG_TO_RAD;
             
             // Dynamic color based on intensity
             uint16_t segColor = COLOR_ACCENT;
             if (i > segments * 0.8) segColor = COLOR_ACCENT_ALT; // Redline
             
             // Pulse effect on max throttle
             if (abs(throttle) > 90 && (millis() / 100) % 2 == 0) segColor = COLOR_TEXT_MAIN;

             if (i < activeSegs) {
                 // Draw thick wedge line using simpler geometry
                 int x1 = centerX + cos(rad) * (radius - 8);
                 int y1 = centerY + sin(rad) * (radius - 8);
                 int x2 = centerX + cos(rad) * radius;
                 int y2 = centerY + sin(rad) * radius;
                 sprite->drawLine(x1, y1, x2, y2, segColor);
                 sprite->drawLine(x1+1, y1, x2+1, y2, segColor);
                 sprite->drawLine(x1, y1+1, x2, y2+1, segColor);
             }
        }
        
        // Inner Speed Value
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_DARK);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString(String(speed), centerX, centerY - 8, FONT_HEADER); // Large Font
        
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_DARK);
        sprite->drawString(throttle >= 0 ? "FWD" : "REV", centerX, centerY + 20, FONT_SMALL);

        // 3. Steering Bar (Bottom)
        int steering = inputManager.getSteeringNormalized(); // Renamed from joyX
        drawCenterBar(sprite, 20, 130, SCREEN_WIDTH - 40, 8, steering, -100, 100, "STEER");

        // 4. Suspension & Trim (Bottom Corners)
        
        // Suspension (Left) 
        int susp = map(inputManager.currentState.potSuspension, 0, 4095, 0, 100);
        sprite->drawRect(5, 40, 10, 80, COLOR_BG_PANEL); // Vertical Bar container
        int fillH = map(susp, 0, 100, 0, 78);
        sprite->fillRect(6, 40 + 80 - fillH - 1, 8, fillH, COLOR_HIGHLIGHT);
        sprite->setTextDatum(TC_DATUM);
        sprite->drawString("SUS", 10, 30, FONT_SMALL);

        // Trim (Right) - Digital Value
        int trim = inputManager.currentState.trimLevel; // -20 to 20
        sprite->setTextColor(COLOR_HIGHLIGHT, COLOR_BG_DARK);
        sprite->setTextDatum(TC_DATUM);
        
        // Draw a "Container" for trim
        sprite->drawRect(SCREEN_WIDTH - 35, 40, 30, 30, COLOR_BG_PANEL);
        sprite->drawString("TRIM", SCREEN_WIDTH - 20, 30, FONT_SMALL);
        
        String trimStr = (trim > 0 ? "+" : "") + String(trim);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString(trimStr, SCREEN_WIDTH - 20, 55, FONT_BODY);
        
        // 5. Tech Decor
        // Corner brackets / decoration
        sprite->drawFastVLine(0, SCREEN_HEIGHT-10, 10, COLOR_ACCENT);
        sprite->drawFastHLine(0, SCREEN_HEIGHT-1, 10, COLOR_ACCENT);
        
        sprite->drawFastVLine(SCREEN_WIDTH-1, SCREEN_HEIGHT-10, 10, COLOR_ACCENT);
        sprite->drawFastHLine(SCREEN_WIDTH-11, SCREEN_HEIGHT-1, 10, COLOR_ACCENT);
    }
};

// Global Instance
ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
