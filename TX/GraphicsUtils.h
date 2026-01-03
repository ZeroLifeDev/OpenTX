#ifndef GRAPHICS_UTILS_H
#define GRAPHICS_UTILS_H

#include <TFT_eSPI.h>
#include "Theme.h"

class GraphicsUtils {
public:
    // Draw a vertical gradient rect
    static void fillGradientRect(TFT_eSprite* sprite, int x, int y, int w, int h, uint16_t colorTop, uint16_t colorBot) {
        float rTop = (colorTop >> 11) & 0x1F;
        float gTop = (colorTop >> 5) & 0x3F;
        float bTop = (colorTop & 0x1F);
        
        float rBot = (colorBot >> 11) & 0x1F;
        float gBot = (colorBot >> 5) & 0x3F;
        float bBot = (colorBot & 0x1F);
        
        for (int i = 0; i < h; i++) {
            float ratio = (float)i / (float)h;
            uint16_t r = rTop + (rBot - rTop) * ratio;
            uint16_t g = gTop + (gBot - gTop) * ratio;
            uint16_t b = bTop + (bBot - bTop) * ratio;
            
            uint16_t color = (r << 11) | (g << 5) | b;
            sprite->drawFastHLine(x, y + i, w, color);
        }
    }

    // Draw a "Blade" needle (Triangle)
    static void drawNeedle(TFT_eSprite* sprite, int cx, int cy, int r, float angleDeg, uint16_t color) {
        float rad = angleDeg * DEG_TO_RAD;
        
        // Tip
        int tx = cx + cos(rad) * r;
        int ty = cy + sin(rad) * r;
        
        // Base (perpendicular)
        float baseRad1 = (angleDeg - 90) * DEG_TO_RAD;
        float baseRad2 = (angleDeg + 90) * DEG_TO_RAD;
        
        int w = 4; // Width at base
        int bx1 = cx + cos(baseRad1) * w;
        int by1 = cy + sin(baseRad1) * w;
        
        int bx2 = cx + cos(baseRad2) * w;
        int by2 = cy + sin(baseRad2) * w;
        
        sprite->fillTriangle(tx, ty, bx1, by1, bx2, by2, color);
    }
    
    // Draw a progress bar with segmented look
    static void drawProgressBar(TFT_eSprite* sprite, int x, int y, int w, int h, float val01, uint16_t activeColor, uint16_t bgCol) {
        sprite->drawRect(x, y, w, h, COLOR_BORDER);
        int innerW = w - 2;
        int innerH = h - 2;
        int fillW = (int)(innerW * val01);
        
        if (fillW > innerW) fillW = innerW;
        if (fillW < 0) fillW = 0;
        
        // Background
        sprite->fillRect(x+1, y+1, innerW, innerH, bgCol);
        
        // Active Fill (Gradient-ish)
        if (fillW > 0) {
            sprite->fillRect(x+1, y+1, fillW, innerH, activeColor);
            
            // Shine effect (top half lighter)
            // We can't easily alpha blend, but we can draw a lighter line
            sprite->drawFastHLine(x+1, y+1, fillW, 0xFFFF); // simple highlight
        }
    }
    
    // Draw a thick arc (simulated with concentric circles)
    static void drawThickArc(TFT_eSprite* sprite, int cx, int cy, int r, int thickness, uint16_t color, int startAngle, int endAngle) {
        // This is slow if not optimized, but for < 10 circles it's fine
        // tft_eSPI doesn't have drawArc everywhere.
        // Better to use drawArc if available. Assuming standard library without extensions.
        // Fallback: Just draw 3 circles for thickness
        for (int i = 0; i < thickness; i++) {
            sprite->drawArc(cx, cy, r-i, r-i-1, startAngle, endAngle, color, 0, false); 
            // Note: drawArc(x, y, r, inner_r, start, end, fg_col, bg_col, arc_end)
            // If drawArc isn't available, we might fail compilation. 
            // Most tft_espi versions have it now.
        }
    }

    // Draw "Cyber Corner" brackets for widget containers
    static void drawCornerWidget(TFT_eSprite* sprite, int x, int y, int w, int h, uint16_t color) {
        int len = 8;
        // TL
        sprite->drawFastHLine(x, y, len, color);
        sprite->drawFastVLine(x, y, len, color);
        // TR
        sprite->drawFastHLine(x+w-len, y, len, color);
        sprite->drawFastVLine(x+w-1, y, len, color);
        // BL
        sprite->drawFastHLine(x, y+h-1, len, color);
        sprite->drawFastVLine(x, y+h-len, len, color);
        // BR
        sprite->drawFastHLine(x+w-len, y+h-1, len, color);
        sprite->drawFastVLine(x+w-1, y+h-len, len, color);
    }
};

#endif // GRAPHICS_UTILS_H
