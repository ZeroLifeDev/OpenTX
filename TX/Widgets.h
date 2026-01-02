#ifndef WIDGETS_H
#define WIDGETS_H

#include <TFT_eSPI.h>
#include "Theme.h"

// ==========================================
//          MODULAR WIDGET SYSTEM
// ==========================================

class Widget {
public:
    virtual void draw(TFT_eSprite* sprite, int x, int y) = 0;
};

// 1. Circular Gauge (Mini)
// Used for: Temp, Voltage, RPM
class WidgetGauge : public Widget {
public:
    const char* label;
    int value; // 0-100
    int r;
    uint16_t color;
    
    WidgetGauge(const char* lbl, int rad, uint16_t col) : label(lbl), r(rad), color(col), value(0) {}
    
    void update(int v) { value = v; }
    
    void draw(TFT_eSprite* sprite, int x, int y) override {
        // bg
        sprite->drawCircle(x, y, r, COLOR_BG_PANEL);
        sprite->drawCircle(x, y, r-2, COLOR_BG_PANEL);
        
        // Arc
        int angle = map(value, 0, 100, 0, 270);
        int start = 135;
        
        for(int i=0; i<angle; i+=10) {
            float rad = (start + i) * DEG_TO_RAD;
            int x1 = x + cos(rad) * (r-2);
            int y1 = y + sin(rad) * (r-2);
            sprite->fillCircle(x1, y1, 1, color);
        }
        
        // Label
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString(label, x, y+2, FONT_SMALL);
        
        // Value Text
        // sprite->drawString(String(value), x, y-5, FONT_SMALL);
    }
};

// 2. Line Graph (Compact)
// Used for: Signal, Throttle Hist
class WidgetGraph : public Widget {
public:
    int history[20];
    int head;
    int w, h;
    uint16_t color;
    
    WidgetGraph(int w, int h, uint16_t col) : w(w), h(h), color(col), head(0) {
        for(int i=0; i<20; i++) history[i] = 0;
    }
    
    void push(int val) {
        history[head] = val;
        head = (head + 1) % 20;
    }
    
    void draw(TFT_eSprite* sprite, int x, int y) override {
        // Frame
        sprite->drawRect(x, y, w, h, COLOR_BG_PANEL);
        
        // Plot
        for(int i=0; i<w; i++) {
             // Map i to index
             int idx = (head - (w - i)/ (w/20)); 
             if (idx < 0) idx += 20;
             idx %= 20;
             
             int val = history[idx]; // 0-100
             int py = map(val, 0, 100, h-2, 2);
             sprite->drawPixel(x+i, y+py, color);
        }
    }
};

// 3. Radar Chart (Simulated G-Force)
// A diamond shape that stretches based on Input
class WidgetRadar : public Widget {
public:
    int xVal; // -100 to 100 (Lat)
    int yVal; // -100 to 100 (Long)
    int size;
    
    WidgetRadar(int s) : size(s), xVal(0), yVal(0) {}
    
    void update(int x, int y) { xVal = x; yVal = y; }
    
    void draw(TFT_eSprite* sprite, int cx, int cy) override {
        // Grid
        sprite->drawRect(cx-size, cy-size, size*2, size*2, COLOR_BG_PANEL);
        sprite->drawLine(cx, cy-size, cx, cy+size, COLOR_BG_PANEL);
        sprite->drawLine(cx-size, cy, cx+size, cy, COLOR_BG_PANEL);
        
        // Dot
        int dx = map(xVal, -100, 100, -size, size);
        int dy = map(yVal, -100, 100, size, -size); // Inverted Y for screen
        
        // Trail / Dot
        sprite->fillCircle(cx+dx, cy+dy, 3, COLOR_ACCENT_SEC);
        
        // Connect to center (Vector)
        sprite->drawLine(cx, cy, cx+dx, cy+dy, COLOR_ACCENT_SEC);
        
        // Label
        sprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite->drawString("G-FORCE", cx + size - 10, cy - size - 5, FONT_SMALL);
    }
};

#endif // WIDGETS_H
