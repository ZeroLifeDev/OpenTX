#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include <Preferences.h>
#include "Theme.h"

// ==========================================
//          SAFETY CONFIGURATION
// ==========================================
// If you have a HardwareConfig.h, these will be ignored.
// If not, these defaults ensure the code compiles.

#if __has_include("HardwareConfig.h")
  #include "HardwareConfig.h"
#endif

// Fallback Definitions (Adjust pins to match your wiring!)
#ifndef PIN_STEERING
  #define PIN_STEERING 34
#endif
#ifndef PIN_THROTTLE
  #define PIN_THROTTLE 35
#endif
#ifndef PIN_POT_SUSPENSION
  #define PIN_POT_SUSPENSION 32
#endif
#ifndef PIN_BTN_TRIM_PLUS
  #define PIN_BTN_TRIM_PLUS 18 // Example Pin
#endif
#ifndef PIN_BTN_TRIM_MINUS
  #define PIN_BTN_TRIM_MINUS 19 // Example Pin
#endif
#ifndef PIN_BTN_MENU
  #define PIN_BTN_MENU 23      // Example Pin for Menu Button
#endif
#ifndef PIN_SW_GYRO
  #define PIN_SW_GYRO 22       // Example Pin
#endif

// Calibration Offsets (Set to 0 if your pots are perfect)
#ifndef STEER_CENTER_FIX
  #define STEER_CENTER_FIX 0
#endif
#ifndef THROT_CENTER_FIX
  #define THROT_CENTER_FIX 0
#endif

// ==========================================
//               GLOBAL STATE
// ==========================================
// Theme Mapping
#define COLOR_BG_MAIN    COL_BG_TOP
#define COLOR_BG_PANEL   COL_CARD_STD
#define COLOR_ACCENT_PRI COL_ACCENT_PRI
#define COLOR_ACCENT_SEC COL_ACCENT_SEC
#define COLOR_ACCENT_TER COL_ACCENT_TER
#define COLOR_TEXT_MAIN  COL_TEXT_PRI
#define COLOR_TEXT_DIM   COL_TEXT_SEC
#define COLOR_TEXT_DIS   COL_TEXT_DIS 

enum AppState {
    STATE_DASHBOARD,
    STATE_MENU_MAIN,
    STATE_MENU_SETTINGS,
    STATE_TELEMETRY,
    STATE_ABOUT
};

struct InputState {
    float steerPct = 0;
    float throttlePct = 0;
    int rawSuspension = 0;
    int steerTrim = 0;
    
    // Telemetry / Sensor Data
    float mpuPitch = 0;
    bool rxConnected = false;
    bool swGyro = false;
};

// ==========================================
//               ANIMATION UTILS
// ==========================================
class AnimFloat {
private:
    float _val, _target, _smooth;
public:
    AnimFloat(float val, float smooth) : _val(val), _target(val), _smooth(smooth) {}
    void set(float t) { _target = t; }
    void update() { _val += (_target - _val) * _smooth; }
    float val() { return _val; }
};

// ==========================================
//           GLOBALS & INSTANCES
// ==========================================
AppState currentState = STATE_DASHBOARD;
InputState state;
Preferences prefs;
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite sprite = TFT_eSprite(&tft);

// ==========================================
//           INPUT MANAGER
// ==========================================
class InputManager {
private:
    unsigned long btnPressTime = 0;
    bool lastMenuState = true; 
    bool lastUpState = true;
    bool lastDownState = true;
    
public:
    // Navigation Events
    bool navUp = false;
    bool navDown = false;
    bool navSelect = false;
    bool navHome = false;

    void update() {
        // Reset One-Shot Events
        navUp = false; navDown = false; navSelect = false; navHome = false;
        
        // 1. MENU BUTTON LOGIC
        bool currMenu = digitalRead(PIN_BTN_MENU);
        
        if (lastMenuState && !currMenu) { // Button Pressed (Falling Edge)
            btnPressTime = millis();
        }
        if (!lastMenuState && currMenu) { // Button Released (Rising Edge)
            unsigned long dur = millis() - btnPressTime;
            if (dur < 800) {
                navSelect = true; // Short Press = Select
            } else {
                navHome = true;   // Long Press = Home
            }
        }
        lastMenuState = currMenu;
        
        // 2. TRIM BUTTONS AS NAVIGATION
        bool currUp = digitalRead(PIN_BTN_TRIM_PLUS);
        bool currDown = digitalRead(PIN_BTN_TRIM_MINUS);
        
        if (lastUpState && !currUp) navUp = true;
        if (lastDownState && !currDown) navDown = true;
        
        lastUpState = currUp;
        lastDownState = currDown;
    }
};

InputManager input;

// ==========================================
//               DASHBOARD
// ==========================================
class ScreenDashboard {
private:
    AnimFloat needleAnim;
    AnimFloat steerAnim;
public:
    ScreenDashboard() : needleAnim(0, 0.2), steerAnim(0, 0.3) {}

    void draw(InputState &s) {
        // Clear BG
        sprite.fillSprite(COLOR_BG_MAIN);

        // Update Physics
        needleAnim.set(abs(s.throttlePct));
        needleAnim.update();
        steerAnim.set(s.steerPct);
        steerAnim.update();

        int cx = 64, cy = 60;
        
        // 1. CENTRAL PITCH DATA
        sprite.setTextColor(COLOR_TEXT_DIM, COLOR_BG_MAIN);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("PITCH", cx, cy - 15, 1);
        sprite.drawFloat(s.mpuPitch, 1, cx, cy - 5, 2); 
        
        // 2. THROTTLE ARC
        sprite.drawSmoothArc(cx, cy, 50, 44, 135, 405, COLOR_BG_PANEL, COLOR_BG_MAIN, true);
        float nVal = needleAnim.val();
        if(nVal > 1) {
            int endAng = 135 + (int)(nVal / 100.0f * 270.0f);
            uint16_t col = (s.throttlePct >= 0) ? COLOR_ACCENT_PRI : COLOR_ACCENT_SEC;
            sprite.drawSmoothArc(cx, cy, 50, 44, 135, endAng, col, COLOR_BG_MAIN, true);
        }
        
        // Throttle Text
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite.drawNumber((int)s.throttlePct, cx, cy + 25, 4);
        
        // 3. STEERING BAR
        int barY = 120, barW = 100, barH = 12, barX = 14;
        sprite.fillRoundRect(barX, barY, barW, barH, 4, COLOR_BG_PANEL);
        int sPx = (int)(steerAnim.val() / 100.0f * 48); 
        if (abs(sPx) > 1) {
             // Logic to draw bar from center
             int startX = (sPx > 0) ? 64 : 64 + sPx;
             sprite.fillRoundRect(startX, barY+2, abs(sPx), barH-4, 2, COLOR_ACCENT_PRI);
        }
        sprite.drawFastVLine(64, barY, barH, COLOR_TEXT_DIS);

        // 4. FOOTER
        sprite.setTextColor(s.swGyro ? COLOR_ACCENT_TER : COLOR_TEXT_DIS, COLOR_BG_MAIN);
        sprite.drawString(s.swGyro ? "GYRO" : "MANUAL", 20, 145, 1);
        
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
        sprite.drawString("TRIM", 85, 145, 1);
        sprite.drawNumber(s.steerTrim, 115, 145, 1);
        
        // 5. RX STATUS DOT
        sprite.fillCircle(120, 8, 3, s.rxConnected ? COLOR_ACCENT_TER : COLOR_ACCENT_SEC);
    }
};

// ==========================================
//               MENU SYSTEM
// ==========================================
const char* menuMainItems[] = {"SETTINGS", "TELEMETRY", "ABOUT", "EXIT"};
const int menuMainCount = 4;

const char* menuSettingsItems[] = {"CALIBRATE", "TRIMS", "EXPO", "EPA", "BACK"};
const int menuSettingsCount = 5;

class ScreenMenu {
private:
    int selectIdx = 0;
public:
    void reset() { selectIdx = 0; }
    
    void draw(const char* title, const char* items[], int count) {
        sprite.fillSprite(COLOR_BG_MAIN);
        
        // Header
        sprite.fillRect(0, 0, 128, 25, COLOR_BG_PANEL);
        sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_PANEL);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(title, 64, 12, 2);
        
        // List
        int startY = 35;
        int rowH = 22;
        
        for (int i=0; i<count; i++) {
            int y = startY + (i * rowH);
            
            if (i == selectIdx) {
                // Selected Item: Cyan Box, Black Text
                sprite.fillRoundRect(5, y, 118, 20, 4, COLOR_ACCENT_PRI);
                sprite.setTextColor(COLOR_BG_MAIN, COLOR_ACCENT_PRI); 
            } else {
                // Normal Item: White Text
                sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
            }
            
            sprite.setTextDatum(MC_DATUM);
            sprite.drawString(items[i], 64, y + 10, 2);
        }
    }
    
    void nav(bool up, bool down, int count) {
        if (up) selectIdx = (selectIdx - 1 + count) % count;
        if (down) selectIdx = (selectIdx + 1) % count;
    }
    
    int getSelection() { return selectIdx; }
};

ScreenDashboard dashboard;
ScreenMenu menu;

// ==========================================
//            MAIN LOGIC & LOOP
// ==========================================
void handleNavigation() {
    input.update();
    
    // Global Home Short-circuit
    if (input.navHome) {
        currentState = STATE_DASHBOARD;
        return;
    }

    switch (currentState) {
        case STATE_DASHBOARD:
            if (input.navSelect) {
                currentState = STATE_MENU_MAIN;
                menu.reset();
            }
            // Quick Trim Adjust on Dashboard
            if (input.navUp) { state.steerTrim++; prefs.putInt("trim_s", state.steerTrim); }
            if (input.navDown) { state.steerTrim--; prefs.putInt("trim_s", state.steerTrim); }
            break;
            
        case STATE_MENU_MAIN:
            menu.nav(input.navUp, input.navDown, menuMainCount);
            if (input.navSelect) {
                int sel = menu.getSelection();
                if (sel == 0) { currentState = STATE_MENU_SETTINGS; menu.reset(); }
                if (sel == 1) currentState = STATE_TELEMETRY;
                if (sel == 2) currentState = STATE_ABOUT;
                if (sel == 3) currentState = STATE_DASHBOARD;
            }
            break;
            
        case STATE_MENU_SETTINGS:
            menu.nav(input.navUp, input.navDown, menuSettingsCount);
            if (input.navSelect) {
                int sel = menu.getSelection();
                if (sel == 4) { currentState = STATE_MENU_MAIN; menu.reset(); } // Back
            }
            break;
            
        case STATE_TELEMETRY:
        case STATE_ABOUT:
            if (input.navSelect) currentState = STATE_MENU_MAIN; // Any click to go back
            break;
    }
}

void drawScreens() {
    switch (currentState) {
        case STATE_DASHBOARD:
            dashboard.draw(state);
            break;
        case STATE_MENU_MAIN:
            menu.draw("MAIN MENU", menuMainItems, menuMainCount);
            break;
        case STATE_MENU_SETTINGS:
            menu.draw("SETTINGS", menuSettingsItems, menuSettingsCount);
            break;
        case STATE_TELEMETRY:
            sprite.fillSprite(COLOR_BG_MAIN);
            sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
            sprite.setTextDatum(MC_DATUM);
            sprite.drawString("TELEMETRY", 64, 20, 2);
            sprite.drawString("RX: 0.00v", 64, 60, 2);
            sprite.drawString("TX: 0.00v", 64, 90, 2);
            sprite.drawString("[CLICK TO EXIT]", 64, 140, 1);
            break;
        case STATE_ABOUT:
            sprite.fillSprite(COLOR_BG_MAIN);
            sprite.setTextColor(COLOR_TEXT_MAIN, COLOR_BG_MAIN);
            sprite.setTextDatum(MC_DATUM);
            sprite.drawString("OPEN TX", 64, 40, 4);
            sprite.drawString("v2.0 Fixed", 64, 80, 2);
            sprite.drawString("[CLICK TO EXIT]", 64, 140, 1);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    
    // 1. Preferences
    prefs.begin("tx-cfg", false);
    state.steerTrim = prefs.getInt("trim_s", 0);

    // 2. Pin Setup
    pinMode(PIN_STEERING, INPUT);
    pinMode(PIN_THROTTLE, INPUT);
    pinMode(PIN_POT_SUSPENSION, INPUT);
    pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
    pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
    pinMode(PIN_BTN_MENU, INPUT_PULLUP);
    pinMode(PIN_SW_GYRO, INPUT_PULLUP);

    // 3. Display Setup
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true); // REQUIRED for correct colors
    
    if (!sprite.createSprite(128, 160)) {
        Serial.println("Memory Allocation Failed!");
        while(1);
    }
    
    tft.fillScreen(TFT_BLACK);
}

void loop() {
    // 1. Read Raw Inputs
    int sRaw = analogRead(PIN_STEERING) + STEER_CENTER_FIX;
    int tRaw = analogRead(PIN_THROTTLE) + THROT_CENTER_FIX;
    state.rawSuspension = analogRead(PIN_POT_SUSPENSION);
    
    // 2. Normalize (-100 to +100)
    // Assuming 12-bit ADC (0-4095), Center approx 2048
    float tPct = (tRaw - 2048) / 20.48f;
    float sPct = (sRaw - 2048) / 20.48f + state.steerTrim;
    
    // Deadzone
    if (abs(tPct) < 5.0) tPct = 0;
    if (abs(sPct) < 2.0) sPct = 0;

    state.steerPct = constrain(sPct, -100, 100);
    state.throttlePct = constrain(tPct, -100, 100);
    state.swGyro = (digitalRead(PIN_SW_GYRO) == LOW);
    
    // Simulate Pitch for UI demo
    state.mpuPitch = sin(millis()/1000.0) * 10.0;
    
    // 3. Logic & Draw
    handleNavigation();
    drawScreens();
    
    // 4. Push to Screen
    sprite.pushSprite(0, 0);
    
    // 5. Stability Delay
    delay(5); 
}