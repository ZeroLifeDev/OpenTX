#ifndef SCREEN_DEBUG_H
#define SCREEN_DEBUG_H

#include "DisplayManager.h"
#include "InputManager.h"
#include "Theme.h"

// System Internals View
class ScreenDebug {
public:
    void init() {}
    
    void update() {}

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite->fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_BG_HEADER);
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_HEADER);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("DEBUG CORE", SCREEN_WIDTH/2, 10, FONT_SMALL);
        
        // Data Columns
        int y = 30;
        
        // Throttle Raw
        sprite->setTextDatum(ML_DATUM);
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
        sprite->drawString("THR:", 10, y, FONT_SMALL);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->drawNumber(inputManager.currentState.throttle, 60, y, FONT_SMALL);
        
        y += 15;
        // Steering Raw
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
        sprite->drawString("STR:", 10, y, FONT_SMALL);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->drawNumber(inputManager.currentState.steering, 60, y, FONT_SMALL);

        y += 15;
        // Pot
        sprite->setTextColor(COLOR_ACCENT_SEC, COLOR_BG_MAIN);
        sprite->drawString("POT:", 10, y, FONT_SMALL);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->drawNumber(inputManager.currentState.potSuspension, 60, y, FONT_SMALL);
        
        y += 15;
        // Gyro Digital
        sprite->setTextColor(COLOR_ACCENT_TER, COLOR_BG_MAIN);
        sprite->drawString("GYRO:", 10, y, FONT_SMALL);
        sprite->drawString(inputManager.currentState.swGyro ? "ON" : "OFF", 60, y, FONT_SMALL);
    }
};

#endif // SCREEN_DEBUG_H
