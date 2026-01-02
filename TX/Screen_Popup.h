#ifndef SCREEN_POPUP_H
#define SCREEN_POPUP_H

#include "DisplayManager.h"
#include "Theme.h"

class ScreenPopup {
private:
    bool visible = false;
    unsigned long showTime = 0;
    const char* msg1 = "";
    const char* msg2 = "";
    uint16_t color = COLOR_ACCENT_PRI;

public:
    void show(const char* m1, const char* m2, uint16_t c) {
        visible = true;
        showTime = millis();
        msg1 = m1;
        msg2 = m2;
        color = c;
    }

    void hide() {
        visible = false;
    }

    bool isVisible() { return visible; }
    
    bool isFinished() {
        return visible && (millis() - showTime > 2000);
    }

    void draw(DisplayManager* display) {
        if (!visible) return;

        TFT_eSprite* sprite = display->getSprite();
        
        int w = 110;
        int h = 70;
        int x = (SCREEN_WIDTH - w) / 2;
        int y = (SCREEN_HEIGHT - h) / 2;

        // Shadow
        sprite->fillRoundRect(x+4, y+4, w, h, 4, COLOR_BG_MAIN);
        
        // Panel Glow
        sprite->fillRoundRect(x, y, w, h, 4, COLOR_BG_PANEL);
        sprite->drawRoundRect(x, y, w, h, 4, color);
        
        // Header Strip
        sprite->fillRect(x+2, y+2, w-4, 20, color);
        
        // Header Text
        sprite->setTextColor(COLOR_BG_MAIN, color); // Inverted on strip
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("ALERT", SCREEN_WIDTH/2, y + 12, FONT_SMALL); // Fixed: FONT_MICRO -> SMALL

        // Body Text
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->drawString(msg1, SCREEN_WIDTH/2, y + 35, FONT_MED); // Fixed: FONT_LABEL -> MED
        
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_PANEL); // Fixed: COLOR_TEXT_SUB -> DIM
        sprite->drawString(msg2, SCREEN_WIDTH/2, y + 55, FONT_SMALL);
    }
};

#endif // SCREEN_POPUP_H
