#ifndef SCREEN_NOTIFICATION_H
#define SCREEN_NOTIFICATION_H

#include "DisplayManager.h"
#include "Theme.h"
#include "SoundManager.h"
#include "AnimationUtils.h"

// Complex Notification System
class ScreenNotification {
private:
    bool visible = false;
    String message;
    String subMessage;
    uint16_t color;
    unsigned long showTime;
    
    // Animations
    AnimFloat slideY;      // Slide in from top
    AnimFloat opacity;     // "Fade" simulation (via width/lines)
    MicroJitter glitch;    // Visual noise
    
public:
    ScreenNotification() : slideY(-60, 0.4f, 0.6f), opacity(0, 0.2f, 0.8f), glitch(5) {}

    void init() {}

    void show(String msg, String sub, uint16_t col) {
        message = msg;
        subMessage = sub;
        color = col;
        visible = true;
        showTime = millis();
        
        // Reset Animation
        slideY.snap(-80); // Start off-screen
        slideY.target = 10; // Target position
        
        opacity.snap(0);
        opacity.target = 100;

        // Play SFX
        if (msg.indexOf("DISCONN") >= 0 || msg.indexOf("EMERGENCY") >= 0) {
             soundManager.playDisconnected();
        } else {
             soundManager.beepStartup(); // Generic alert
        }
    }

    void update() {
        if (!visible) return;
        
        unsigned long elapsed = millis() - showTime;
        
        // Update Physics
        slideY.update();
        opacity.update();
        
        // Auto-dismiss logic with exit animation
        if (elapsed > 2500) {
            slideY.target = -80; // Slide out
            opacity.target = 0;
            
            if (slideY.val() < -70) {
                visible = false;
            }
        }
    }

    void draw(DisplayManager* display) {
        if (!visible) return;
        
        TFT_eSprite* sprite = display->getSprite();
        
        int y = (int)slideY.val();
        int h = 50;
        int w = SCREEN_WIDTH - 20;
        int x = 10;
        
        // 1. Shadow / Glitch Effect
        // Draw random "Ghost" boxes for "too much animation"
        if (random(0, 10) > 6) {
            int offX = glitch.get();
            int offY = glitch.get();
            sprite->drawRect(x + offX, y + offY, w, h, COLOR_ACCENT_4);
        }
        
        // 2. Main Box Background (White/Glass)
        sprite->fillRect(x, y, w, h, COLOR_BG_PANEL);
        sprite->drawRect(x, y, w, h, COLOR_TEXT_MAIN); // Border
        
        // 3. Header Bar
        sprite->fillRect(x, y, w, 16, color);
        
        // 4. Content
        // Title
        sprite->setTextDatum(TC_DATUM);
        sprite->setTextColor(COLOR_BG_PANEL, color); // Inverted on colored bar
        sprite->drawString(message, SCREEN_WIDTH/2, y + 1, FONT_LABEL); // Fixed font size use
        
        // Body
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString(subMessage, SCREEN_WIDTH/2, y + 30, FONT_SMALL);
        
        // 5. Progress/Timer Bar
        unsigned long elapsed = millis() - showTime;
        if (elapsed < 2500) {
            int barW = map(elapsed, 0, 2500, w-4, 0);
            sprite->fillRect(x+2, y+h-4, barW, 2, color);
        }
    }
    
    bool isActive() { return visible; }
};

#endif // SCREEN_NOTIFICATION_H
