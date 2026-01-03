#ifndef SCREEN_NOTIFICATION_H
#define SCREEN_NOTIFICATION_H

#include "DisplayManager.h"
#include "Theme.h"
#include "AnimationUtils.h"
#include "GraphicsUtils.h"

// ==========================================
//          CINEMATIC OVERLAY SYSTEM
// ==========================================
// Handles "Cutscene" style popups for major events.
// Pauses dashboard interaction visually.

class ScreenNotification {
private:
    bool active;
    unsigned long startTime;
    String mainText;
    String subText;
    uint16_t accentColor;
    
    // Animations
    AnimFloat openAnim; // 0.0 to 1.0 (Opening shutter)
    
public:
    ScreenNotification() : openAnim(0, 0.2f, 0.6f) {} 

    void init() {
        active = false;
        openAnim.reset(0);
    }

    void show(String main, String sub, uint16_t color) {
        mainText = main;
        subText = sub;
        accentColor = color;
        active = true;
        startTime = millis();
        openAnim.reset(0);
        openAnim.target = 100.0f; // Expand to 100%
        
        // Play sound based on context (handled by caller mostly, but we can default)
    }

    void update() {
        if (!active) return;
        
        openAnim.update();
        
        // Auto close after 1.5s
        if (openAnim.target == 100.0f && millis() - startTime > 1500) {
            openAnim.target = 0.0f; // Close
        }
        
        if (openAnim.target == 0.0f && openAnim.val() < 1.0f) {
            active = false;
        }
    }

    bool isActive() { return active; }

    void draw(DisplayManager* display) {
        if (!active && openAnim.val() < 1.0f) return;

        TFT_eSprite* sprite = display->getSprite();
        
        float val = openAnim.val(); // 0 to 100
        int h = (int)map(val, 0, 100, 0, 60); // Height of bandwidth (center out)
        int center = SCREEN_HEIGHT / 2;
        int topY = center - (h/2);
        int botY = center + (h/2);
        
        // 1. DIM BACKGROUND (Overlay)
        // Only if fully open, otherwise maybe too expensive? 
        // Just draw the strap for speed.
        
        // 2. ANIMATED STRAP
        // Draw Main Box
        if (h > 2) {
            sprite->fillRect(0, topY, SCREEN_WIDTH, h, COLOR_BG_PANEL);
            sprite->drawFastHLine(0, topY, SCREEN_WIDTH, accentColor);
            sprite->drawFastHLine(0, botY, SCREEN_WIDTH, accentColor);
            
            // Tech Lines
            if (h > 40) {
                // Decor
                sprite->drawRect(0, topY+2, 10, h-4, accentColor); // Left bracket
                sprite->fillRect(0, topY+2, 4, h-4, accentColor);
                
                sprite->drawRect(SCREEN_WIDTH-10, topY+2, 10, h-4, accentColor); // Right bracket
                sprite->fillRect(SCREEN_WIDTH-4, topY+2, 4, h-4, accentColor);
            }
        }
        
        // 3. TEXT (Only when open enough)
        if (val > 80.0f) {
            sprite->setTextDatum(MC_DATUM);
            
            // Main Text (Big)
            sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
            sprite->drawString(mainText, SCREEN_WIDTH/2, center - 8, FONT_LARGE);
            
            // Sub Text (Accented)
            sprite->setTextColor(accentColor, COLOR_BG_PANEL);
            sprite->drawString(subText, SCREEN_WIDTH/2, center + 15, FONT_MED);
        }
    }
};

// Global instance needed for Dashboard to access
// ScreenNotification notificationSystem; // user UIManager's instance or separate?
// Actually simpler:
extern ScreenNotification notificationSystem;

#endif // SCREEN_NOTIFICATION_H
