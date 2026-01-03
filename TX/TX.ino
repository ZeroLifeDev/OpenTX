#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include <Preferences.h>
#include "HardwareConfig.h"
#include "Theme.h"

// ==========================================
//               THEME MAPPING
// ==========================================
#define COLOR_BG_MAIN    COL_BG_TOP
// COLOR_BG_DIM Removed
#define COLOR_BG_PANEL   COL_CARD_STD
#define COLOR_ACCENT_PRI COL_ACCENT_PRI
#define COLOR_ACCENT_SEC COL_ACCENT_SEC
#define COLOR_ACCENT_TER COL_ACCENT_TER
#define COLOR_TEXT_MAIN  COL_TEXT_PRI
#define COLOR_TEXT_DIM   COL_TEXT_SEC
#define COLOR_TEXT_DIS   COL_TEXT_DIS // Mapped to Theme.h

// Text Datums
#define TL_DATUM 0
#define TC_DATUM 1
#define TR_DATUM 2
#define ML_DATUM 3
#define MC_DATUM 4
#define MR_DATUM 5

// ==========================================
//               ANIMATION UTILS
// ==========================================
class AnimFloat {
private:
    float _val;
    float _target;
    float _smooth;
public:
    AnimFloat(float val, float smooth) {
        _val = val; 
        _target = val; 
        _smooth = smooth; 
    }
    void set(float t) { _target = t; }
    void snap(float v) { _val = v; _target = v; }
    void update() { _val += (_target - _val) * _smooth; }
    float val() { return _val; }
};

// ==========================================
//               GLOBAL STATE
// ==========================================
struct InputState {
    // Raw
    int rawSteer;
    int rawThrottle;
    int rawSuspension; 
    
    // Processed
    float steerPct; // -100 to 100
    float throttlePct; // -100 to 100
    int steerTrim;
    
    // Status
    bool swGyro;
    bool motorEnabled;
};

InputState state;
Preferences prefs;
TFT_eSPI tft = TFT_eSPI(); 
// Sprite specific to Full Screen for DMA push if screen small enough?
// 128x160 RGB565 is ~40KB. ESP32 has plenty of RAM.
TFT_eSprite sprite = TFT_eSprite(&tft);

// ==========================================
//          PORTRAIT DASHBOARD (NO FAKE)
// ==========================================
class ScreenDashboard {
private:
    AnimFloat needleAnim;
    AnimFloat steerAnim;
    bool lastGyro = false;
    int lastTrim = 0;
    
public:
    ScreenDashboard() : 
        needleAnim(0, 0.2), // Smooth
        steerAnim(0, 0.3)
    {}

    void init() {
        needleAnim.snap(0);
        steerAnim.snap(0);
        lastGyro = state.swGyro;
        lastTrim = state.steerTrim;
    }

    void draw() {
        // 1. CLEAR / BG
        sprite.fillSprite(COLOR_BG_MAIN);

        // --- UPDATE ANIMS ---
        // Throttle Mapping (Abs value for ring)
        float tAbs = abs(state.throttlePct);
        needleAnim.set(tAbs);
        needleAnim.update();
        
        steerAnim.set(state.steerPct);
        steerAnim.update();
        
        // --- LAYOUT: PORTRAIT ---
        // Width: 128, Height: 160
        
        int cx = SCREEN_WIDTH / 2;
        int cy = 60; // Upper Center
        
        // 2. MAIN RING (Throttle)
        int r = 50;
        int thickness = 6;
        // Draw Track
        sprite.drawSmoothArc(cx, cy, r, r-thickness, 135, 405, COLOR_BG_PANEL, COLOR_BG_MAIN, true);
        
        // Draw Fill
        // Angle: 135 to 405 (270 deg span)
        // 0% = 135, 100% = 405
        float nVal = needleAnim.val(); // 0-100
        if(nVal > 1) {
            int endAng = 135 + (int)(nVal / 100.0f * 270.0f);
            uint16_t col = (state.throttlePct >= 0) ? COLOR_ACCENT_PRI : COLOR_ACCENT_SEC; // Cyan fwd, Orange rev
            sprite.drawSmoothArc(cx, cy, r, r-thickness, 135, endAng, col, COLOR_BG_MAIN, true);
        }
        
        // Center Text (Real Throttle %)
        sprite.setTextFont(4); // Large Font
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN); // transparent bg doesn't work well with sprite overwrite? Use BG color
        // Actually sprite overwrite works if we cleared.
        sprite.setTextDatum(MC_DATUM);
        sprite.drawNumber((int)state.throttlePct, cx, cy);
        
        sprite.setTextFont(1);
        sprite.setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite.drawString(state.throttlePct >= 0 ? "THR" : "BRK", cx, cy + 20);

        // 3. STEERING BAR (Horizontal below ring)
        int barY = 120;
        int barW = 100;
        int barH = 12;
        int barX = (SCREEN_WIDTH - barW) / 2;
        
        // Track
        sprite.fillRoundRect(barX, barY, barW, barH, 4, COLOR_BG_PANEL);
        
        // Indicator
        float sVal = steerAnim.val(); // -100 to 100
        int sCenter = barX + barW/2;
        int sPx = (int)(sVal / 100.0f * (barW/2 - 2));
        
        if (abs(sPx) > 1) {
            if (sPx > 0) sprite.fillRoundRect(sCenter, barY+2, sPx, barH-4, 2, COLOR_ACCENT_PRI);
            else sprite.fillRoundRect(sCenter + sPx, barY+2, -sPx, barH-4, 2, COLOR_ACCENT_PRI);
        }
        // Center Line
        sprite.drawFastVLine(sCenter, barY, barH, COLOR_TEXT_DIS);
        
        sprite.setTextDatum(TC_DATUM);
        sprite.drawString("STR", sCenter, barY + 14);

        // 4. BOTTOM STATUS (Suspension Pot + Trims)
        // No fake voltage. Real data only.
        int footerY = 145;
        
        // Gyro Status
        if (state.swGyro) {
            sprite.setTextColor(COLOR_ACCENT_TER, COLOR_BG_MAIN);
            sprite.drawString("GYRO ON", 30, footerY); // Left
        } else {
            sprite.setTextColor(COLOR_TEXT_DIS, COLOR_BG_MAIN);
            sprite.drawString("GYRO OFF", 30, footerY);
        }
        
        // Trim Value
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite.drawString("TRIM:", 90, footerY);
        sprite.drawNumber(state.steerTrim, 115, footerY);
        
        // Suspension Indicator (Small vertical bar on edge?)
        // Let's put it on the far right edge overlay
        int suspW = 4;
        int suspH = 100;
        int suspX = SCREEN_WIDTH - 6;
        int suspY = 30;
        // Track
        sprite.fillRect(suspX, suspY, suspW, suspH, COLOR_BG_PANEL);
        // Fill
        int sFill = map(state.rawSuspension, 0, 4095, 0, suspH);
        sprite.fillRect(suspX, suspY + (suspH - sFill), suspW, sFill, COLOR_ACCENT_TER);
    }
};

ScreenDashboard dashboard;

// ==========================================
//               INPUT & SETUP
// ==========================================
void readInputs() {
    state.rawSteer = analogRead(PIN_STEERING) + STEER_CENTER_FIX;
    state.rawThrottle = analogRead(PIN_THROTTLE) + THROT_CENTER_FIX;
    state.rawSuspension = analogRead(PIN_POT_SUSPENSION);
    
    // Joystick Normalization
    auto norm = [](int val, int center) {
        float v = (val - center) / (float)(4095-center) * 100.0f;
        if(v > 100) v=100; if(v<-100) v=-100;
        return v;
    };
    
    state.steerPct = norm(state.rawSteer, JOY_CENTER) + state.steerTrim;
    state.throttlePct = norm(state.rawThrottle, JOY_CENTER);

    state.swGyro = (digitalRead(PIN_SW_GYRO) == LOW); // Active Low
    
    // Trim Buttons
    static unsigned long lastTrim = 0;
    if (millis() - lastTrim > 150) { // Repeat delay
        if (digitalRead(PIN_BTN_TRIM_PLUS) == LOW) { state.steerTrim++; lastTrim = millis(); }
        if (digitalRead(PIN_BTN_TRIM_MINUS) == LOW) { state.steerTrim--; lastTrim = millis(); }
    }
}

void setup() {
    Serial.begin(115200);
    
    prefs.begin("tx-cfg", false);
    state.steerTrim = prefs.getInt("trim_s", 0);
    
    pinMode(PIN_STEERING, INPUT);
    pinMode(PIN_THROTTLE, INPUT);
    pinMode(PIN_POT_SUSPENSION, INPUT);
    pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
    pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
    pinMode(PIN_SW_GYRO, INPUT_PULLUP);
    
    // DMA Init
    tft.init();
    tft.initDMA(); 
    tft.setRotation(0); // PORTRAIT (0 or 2)
    
    // Create Sprite (Full Screen Buffer)
    // 128 x 160
    if (!sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT)) {
        Serial.println("Sprite Alloc Fail!");
    } else {
        Serial.println("Sprite Alloc OK");
    }
    
    dashboard.init();
}

void loop() {
    readInputs();
    
    dashboard.draw();
    
    // DMA Push if supported, otherwise standard push
    // pushSprite is optimized in library usually.
    sprite.pushSprite(0, 0); 
}
