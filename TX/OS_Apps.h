#ifndef OS_APPS_H
#define OS_APPS_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"
#include "GXKernel.h"
#include "SoundManager.h"

// ==========================================
//          OS APP ECOSYSTEM
// ==========================================
// Defines discrete "Applications" that run on the OS.

// 1. APP: STOPWATCH
class App_Stopwatch {
private:
    bool running;
    unsigned long startTime;
    unsigned long elapsedTime;
    
public:
    void init() { running = false; elapsedTime = 0; }
    
    void toggle() {
        if (running) {
             running = false;
        } else {
             running = true;
             startTime = millis() - elapsedTime;
        }
    }
    
    void reset() {
        running = false;
        elapsedTime = 0;
    }
    
    void update() {
        if (running) elapsedTime = millis() - startTime;
    }
    
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        sprite->fillSprite(COLOR_BG_MAIN);
        
        gxKernel.drawHoloRect(sprite, 10, 40, SCREEN_WIDTH-20, 50, COLOR_ACCENT_PRI);
        
        // Time Format MM:SS:ms
        int mins = (elapsedTime / 60000);
        int secs = (elapsedTime % 60000) / 1000;
        int ms   = (elapsedTime % 1000) / 10;
        
        String timeStr = "";
        if (mins < 10) timeStr += "0";
        timeStr += String(mins) + ":";
        if (secs < 10) timeStr += "0";
        timeStr += String(secs) + ":";
        if (ms < 10) timeStr += "0";
        timeStr += String(ms);
        
        sprite->setTextDatum(MC_DATUM);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->drawString(timeStr, SCREEN_WIDTH/2, 65, FONT_NUMS);
        
        sprite->drawString("CHRONO", SCREEN_WIDTH/2, 20, FONT_MED);
        
        sprite->setTextDatum(BC_DATUM);
        sprite->drawString("[SET] START/STOP", SCREEN_WIDTH/2, SCREEN_HEIGHT-5, FONT_SMALL);
    }
    
    void handleInput(bool btnSet, bool btnBack) {
        if (btnSet) { toggle(); soundManager.playClick(); }
    }
};

// 2. APP: TUNER (Curve Editor)
class App_Tuner {
private:
    int expoVal = 0; // -100 to 100
    
public:
    void init() {}
    
    void adjust(int delta) {
        expoVal += delta;
        if (expoVal > 100) expoVal = 100;
        if (expoVal < -100) expoVal = -100;
    }
    
    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        sprite->fillSprite(COLOR_BG_MAIN);
        
        sprite->drawString("CURVE TUNER", 10, 10, FONT_SMALL);
        
        // Draw Curve
        int cx = 20; 
        int cy = 100;
        int w = 80; 
        int h = 80;
        
        // Axis
        sprite->drawRect(cx, cy-h, w, h, COLOR_BG_PANEL);
        sprite->drawLine(cx, cy, cx+w, cy-h, COLOR_TEXT_DIM); // Linear Ref
        
        // Plot Expo Curve
        for(int i=0; i<w; i+=2) {
            float x = i / (float)w; // 0.0 - 1.0
            // Simple cubic bezier approx for expo
            // y = x + (x-x^3)*k
            float k = expoVal / 100.0f;
            float y = x + (x - (x*x*x)) * k;
            
            int py = cy - (int)(y * h);
            sprite->drawPixel(cx+i, py, COLOR_ACCENT_SEC);
        }
        
        sprite->setTextColor(COLOR_ACCENT_SEC, COLOR_BG_MAIN);
        sprite->drawString("EXPO: " + String(expoVal) + "%", SCREEN_WIDTH/2, 120, FONT_MED);
    }
    
    void handleInput(bool up, bool down) {
        if (up) adjust(5);
        if (down) adjust(-5);
    }
};

// Global Apps
App_Stopwatch appStopwatch;
App_Tuner appTuner;

#endif // OS_APPS_H
