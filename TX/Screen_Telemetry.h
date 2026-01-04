#ifndef SCREEN_TELEMETRY_H
#define SCREEN_TELEMETRY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Types.h"
#include "Theme.h"

extern TFT_eSprite sprite;

class ScreenTelemetry {
public:
    void draw(InputState& s) {
        sprite.fillSprite(TFT_BLACK);
        
        // Header
        sprite.fillRect(0, 0, 128, 25, 0x2104);
        sprite.setTextColor(TFT_WHITE, 0x2104);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("TELEMETRY", 64, 12, 2);
        
        // Grid
        sprite.drawFastHLine(0, 60, 128, 0x4208);
        sprite.drawFastHLine(0, 100, 128, 0x4208);
        sprite.drawFastVLine(64, 25, 135, 0x4208);
        
        // Data 1: RX Volt
        sprite.setTextColor(0x07FF, TFT_BLACK); // Cyan
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("RX BATT", 32, 35, 1);
        sprite.drawFloat(4.2, 1, 32, 50, 2);
        
        // Data 2: SIGNAL
        sprite.drawString("SIGNAL", 96, 35, 1);
        sprite.drawNumber(-60, 96, 50, 2);

        // Data 3: TX Volt
        sprite.setTextColor(TFT_WHITE, TFT_BLACK);
        sprite.drawString("TX BATT", 32, 75, 1);
        sprite.drawFloat(3.9, 1, 32, 90, 2);
        
        // Data 4: TEMP
        sprite.drawString("TEMP", 96, 75, 1);
        sprite.drawNumber(45, 96, 90, 2);
    }
};

static ScreenTelemetry screenTelemetry;

#endif
