#ifndef SCREEN_CALIBRATION_H
#define SCREEN_CALIBRATION_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

class ScreenCalibration {
private:
    int stage = 0; // 0=Info, 1=Center, 2=Limits, 3=Save
    
    // Captured Values
    int sMid, tMid;
    int sMin, sMax, tMin, tMax;
    
public:
    void init() {
        stage = 0;
        // Reset capture limit vars to inverse extremes
        sMin = 4096; sMax = 0;
        tMin = 4096; tMax = 0;
    }

    // Returns true if calibration process exited
    bool update() {
        // Logic handled by buttons in Main Loop usually, but we need local processing here
        return false;
    }
    
    void nextStage() {
        stage++;
        if (stage > 3) stage = 0;
    }

    void handleInput(bool btnSet) {
        if (!btnSet) return;
        
        soundManager.playConfirm();
        
        if (stage == 0) {
            // Start
            stage = 1;
        } else if (stage == 1) {
            // Capture Centers
            sMid = inputManager.getRawSteer();
            tMid = inputManager.getRawThrot();
            stage = 2;
        } else if (stage == 2) {
            // Finish Limits (Capture happens in draw/update continuously)
            stage = 3;
        } else if (stage == 3) {
            // Save
            inputManager.saveCalibration(sMin, sMid, sMax, tMin, tMid, tMax);
            // Done -> The caller (UIManager) should switch screen back
        }
    }

    int getStage() { return stage; }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_HEADER);
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_HEADER);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("CALIBRATION", SCREEN_WIDTH/2, 10, FONT_SMALL);
        
        int rawS = inputManager.getRawSteer();
        int rawT = inputManager.getRawThrot();

        int cy = SCREEN_HEIGHT / 2;
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);

        if (stage == 0) {
            sprite->drawString("PRESS [SET] TO", SCREEN_WIDTH/2, cy-10, FONT_SMALL);
            sprite->drawString("START CALIBRATION", SCREEN_WIDTH/2, cy+5, FONT_SMALL);
            sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
            sprite->drawString("CENTER STICKS FIRST", SCREEN_WIDTH/2, cy+25, FONT_SMALL);
        }
        else if (stage == 1) {
            sprite->drawString("STEP 1: CENTER", SCREEN_WIDTH/2, 40, FONT_SMALL);
            sprite->setTextColor(COLOR_ACCENT_TER, COLOR_BG_MAIN);
            sprite->drawString("HOLD STICKS AT CENTER", SCREEN_WIDTH/2, 60, FONT_SMALL);
            sprite->drawString("THEN PRESS [SET]", SCREEN_WIDTH/2, 80, FONT_SMALL);
            
            char buf[32];
            sprintf(buf, "S:%d  T:%d", rawS, rawT);
            sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
            sprite->drawString(buf, SCREEN_WIDTH/2, 110, FONT_SMALL);
        }
        else if (stage == 2) {
            // Capture Min/Max continously
            if (rawS < sMin) sMin = rawS;
            if (rawS > sMax) sMax = rawS;
            if (rawT < tMin) tMin = rawT;
            if (rawT > tMax) tMax = rawT;
            
            sprite->drawString("STEP 2: LIMITS", SCREEN_WIDTH/2, 35, FONT_SMALL);
            sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
            sprite->drawString("MOVE EXTENTS", SCREEN_WIDTH/2, 55, FONT_MED);
            
            // Visual Bars
            int barW = 100;
            // Draw S Range
            sprite->drawRect((SCREEN_WIDTH-barW)/2, 80, barW, 6, COLOR_BG_PANEL);
            int sx = map(rawS, 0, 4095, 0, barW);
            sprite->fillRect((SCREEN_WIDTH-barW)/2 + sx, 80, 2, 6, COLOR_ACCENT_SEC);
            
            // Draw T Range
            sprite->drawRect((SCREEN_WIDTH-barW)/2, 100, barW, 6, COLOR_BG_PANEL);
            int tx = map(rawT, 0, 4095, 0, barW);
            sprite->fillRect((SCREEN_WIDTH-barW)/2 + tx, 100, 2, 6, COLOR_ACCENT_SEC);
            
            sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
            sprite->drawString("PRESS [SET] WHEN DONE", SCREEN_WIDTH/2, 130, FONT_SMALL);
        }
        else if (stage == 3) {
            sprite->setTextColor(COLOR_OK, COLOR_BG_MAIN);
            sprite->drawString("SAVING...", SCREEN_WIDTH/2, cy, FONT_MED);
        }
    }
};

#endif // SCREEN_CALIBRATION_H
