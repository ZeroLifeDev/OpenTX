#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "DisplayManager.h"
#include "ModelManager.h"
#include "Mixer.h"
#include "Theme.h"
#include "CommsManager.h"

// ==========================================
//          SANWA-STYLE PRO DASHBOARD
// ==========================================
// No toys. Just data.
// - Model Name (Big)
// - Lap Timer
// - Output Bars (Real Mixer Output)
// - TX Voltage

class ScreenDashboard {
private:
    float vTx = 8.1; // Simulated for now
    
public:
    void init() {}

    void draw(DisplayManager* display) {
        TFT_eSprite* sprite = display->getSprite();
        ModelData* m = modelManager.getModel();
        
        sprite->fillSprite(COLOR_BG_MAIN);
        
        // 1. TOP HEADER (Model Name & ID)
        sprite->fillRect(0, 0, SCREEN_WIDTH, 24, COLOR_BG_HEADER);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_HEADER);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString(m->name, 4, 12, FONT_MED); // "MODEL 01"
        
        // Slot ID Badge
        int id = modelManager.getSlot() + 1;
        sprite->fillCircle(SCREEN_WIDTH-12, 12, 8, COLOR_BG_PANEL);
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_PANEL);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawNumber(id, SCREEN_WIDTH-12, 12, FONT_SMALL);

        // 2. MAIN TIMER (Center)
        // Standard "Lap Time" look
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString("LAP TIMER", SCREEN_WIDTH/2, 40, FONT_SMALL);
        
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        // Using large nums
        sprite->drawString("00:00.00", SCREEN_WIDTH/2, 60, FONT_LARGE);

        // 3. CHANNEL OUTPUT BARS (The "Pro" Visual)
        // Shows exactly what the mixer is sending (after expo/epa)
        int barY = 90;
        int barH = 10;
        int barW = 100;
        int bx = (SCREEN_WIDTH - barW)/2;
        
        // Steering (CH1)
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString("ST", 4, barY+5, FONT_SMALL);
        
        sprite->drawRect(bx, barY, barW, barH, COLOR_BG_PANEL); // Frame
        sprite->drawFastVLine(bx + barW/2, barY-1, barH+2, COLOR_TEXT_DIM); // Center
        
        int sv = mixer.getMsgSteering(); // -120 to 120
        int sl = map(abs(sv), 0, 120, 0, barW/2);
        if (sv > 0) sprite->fillRect(bx + barW/2, barY+1, sl, barH-2, COLOR_ACCENT_PRI);
        else        sprite->fillRect(bx + barW/2 - sl, barY+1, sl, barH-2, COLOR_ACCENT_PRI);

        // Throttle (CH2)
        barY += 20;
        sprite->drawString("TH", 4, barY+5, FONT_SMALL);
        
        sprite->drawRect(bx, barY, barW, barH, COLOR_BG_PANEL);
        sprite->drawFastVLine(bx + barW/2, barY-1, barH+2, COLOR_TEXT_DIM);
        
        int tv = mixer.getMsgThrottle();
        int tl = map(abs(tv), 0, 120, 0, barW/2);
        uint16_t tCol = (tv < 0) ? COLOR_ACCENT_SEC : COLOR_ACCENT_TER; // Red for Brake, Green for Gas
        if (tv > 0) sprite->fillRect(bx + barW/2, barY+1, tl, barH-2, tCol);
        else        sprite->fillRect(bx + barW/2 - tl, barY+1, tl, barH-2, tCol);
        
        // 4. BOTTOM STATUS STRIP
        int fy = 140;
        sprite->drawFastHLine(0, fy, SCREEN_WIDTH, COLOR_TEXT_DIM);
        
        // Voltage
        sprite->setTextDatum(BL_DATUM);
        sprite->setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite->drawString("TX: 8.1v", 4, SCREEN_HEIGHT-4, FONT_SMALL);
        
        // Trim Indicators
        char trimBuf[10];
        sprintf(trimBuf, "TRM:%d", inputManager.internalTrim);
        sprite->setTextDatum(BC_DATUM);
        sprite->drawString(trimBuf, SCREEN_WIDTH/2, SCREEN_HEIGHT-4, FONT_SMALL);
        
        // Mode
        sprite->setTextDatum(BR_DATUM);
        sprite->setTextColor(COLOR_ACCENT_PRI, COLOR_BG_MAIN);
        sprite->drawString("FHSS-4", SCREEN_WIDTH-4, SCREEN_HEIGHT-4, FONT_SMALL);
    }
};

extern ScreenDashboard screenDashboard;

#endif // SCREEN_DASHBOARD_H
