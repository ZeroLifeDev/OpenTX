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
        GFXcanvas16* sprite = display->getCanvas();

        // Background is already cleared by DisplayManager
        
        // 1. Draw Logo Text (Centered)
        sprite->setTextColor(COLOR_TEXT_MAIN);
        display->setTextCentered("OpenTX", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, 2);
        
        sprite->setTextColor(COLOR_ACCENT);
        display->setTextCentered("SYSTEMS", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 1);

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
        sprite->setTextColor(COLOR_TEXT_SUB);
        
        String status = "Ready.";
        if (progress < 40) status = "Init Hardware...";
        else if (progress < 80) status = "Loading Modules...";
        
        display->setTextCentered(status, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10, 1);
    }

    bool isFinished() {
        return isComplete;
    }
};

// Global Instance
ScreenIntro screenIntro;

#endif // SCREEN_INTRO_H
