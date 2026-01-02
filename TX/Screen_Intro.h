#ifndef SCREEN_INTRO_H
#define SCREEN_INTRO_H

#include "DisplayManager.h"
#include "Theme.h"

class ScreenIntro {
private:
    unsigned long startTime;
    bool isComplete;
    int progress;

public:
    void init() {
        startTime = millis();
        isComplete = false;
        progress = 0;
    }

    void update() {
        if (isComplete) return;

        unsigned long elapsed = millis() - startTime;

        // Simulate system check progress
        if (elapsed < 1000) {
            progress = map(elapsed, 0, 1000, 0, 40);
        } else if (elapsed < 2000) {
            progress = map(elapsed, 1000, 2000, 40, 80);
        } else if (elapsed < 2500) {
            progress = map(elapsed, 2000, 2500, 80, 100);
        } else {
            isComplete = true;
        }
    }

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();

        // Background is already cleared by DisplayManager
        
        // 1. Draw Logo Text (Centered)
        sprite->setTextDatum(MC_DATUM);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_DARK);
        sprite->drawString("OpenTX", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, FONT_HEADER);
        
        sprite->setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
        sprite->drawString("SYSTEMS", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, FONT_BODY);

        // 2. Draw Loading Bar
        int barWidth = 120;
        int barHeight = 6;
        int barX = (SCREEN_WIDTH - barWidth) / 2;
        int barY = SCREEN_HEIGHT - 30;

        // Bar Container
        sprite->drawRect(barX, barY, barWidth, barHeight, COLOR_BG_PANEL);
        
        // Bar Fill
        int fillWidth = map(progress, 0, 100, 0, barWidth - 2);
        if (fillWidth > 0) {
            sprite->fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, COLOR_ACCENT);
        }

        // 3. Draw Status Text
        sprite->setTextColor(COLOR_TEXT_SUB, COLOR_BG_DARK);
        sprite->setTextDatum(BC_DATUM); // Bottom Center
        
        if (progress < 40) {
            sprite->drawString("Initializing Hardware...", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 5, FONT_SMALL);
        } else if (progress < 80) {
            sprite->drawString("Loading Modules...", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 5, FONT_SMALL);
        } else {
            sprite->drawString("Ready.", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 5, FONT_SMALL);
        }
    }

    bool isFinished() {
        return isComplete;
    }
};

// Global Instance
ScreenIntro screenIntro;

#endif // SCREEN_INTRO_H
