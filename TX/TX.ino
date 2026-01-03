#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include <Preferences.h>
#include "HardwareConfig.h"
#include "Theme.h"

// ==========================================
//             THEME & CONFIG
// ==========================================
#define COLOR_BG_MAIN    COL_BG_TOP
#define COLOR_BG_PANEL   COL_CARD_STD
#define COLOR_ACCENT_PRI COL_ACCENT_PRI
#define COLOR_ACCENT_SEC COL_ACCENT_SEC
#define COLOR_ACCENT_TER COL_ACCENT_TER
#define COLOR_TEXT_MAIN  COL_TEXT_PRI
#define COLOR_TEXT_DIM   COL_TEXT_SEC
#define COLOR_TEXT_DIS   COL_TEXT_DIS 

#define MC_DATUM 4

// ==========================================
//               GLOBAL STATE
// ==========================================
enum AppState {
    STATE_DASHBOARD,
    STATE_MENU_MAIN,
    STATE_MENU_SETTINGS,
    STATE_MENU_CALIBRATION,
    STATE_TELEMETRY,
    STATE_ABOUT
};

struct InputState {
    float steerPct = 0;
    float throttlePct = 0;
    int rawSuspension = 0;
    int steerTrim = 0;
    
    // Telemetry / Sensor Data
    float txVolts = 0;
    float rxVolts = 0;
    float mpuPitch = 0;
    float mpuRoll = 0;
    bool rxConnected = false;
    
    bool swGyro = false;
};

// ==========================================
//           INPUT MANAGER
// ==========================================
class InputManager {
private:
    unsigned long btnPressTime = 0;
    bool btnState = true; 
    
public:
    // Navigation Events
    bool navUp = false;
    bool navDown = false;
    bool navSelect = false;
    bool navBack = false;
    bool navHome = false;

    void update(InputState &state) {
        // Reset One-Shot Events
        navUp = false; navDown = false; navSelect = false; navBack = false; navHome = false;
        
        // 1. MENU BUTTON (Pin 7 in Schematic usually, here assumed PIN_BTN_MENU if defined, 
        // otherwise using SET as primary nav for now based on available pins in older code?)
        // WAIT. Previous code only had TRIM+, TRIM-, GYRO SW. 
        // HardwareConfig.h usually has PIN_BTN_MENU, PIN_BTN_SET.
        // I will assume defaults:
        
        static bool lastMenu = true;
        bool currMenu = digitalRead(PIN_BTN_MENU);
        
        if (lastMenu && !currMenu) { // Press Start
            btnPressTime = millis();
        }
        if (!lastMenu && currMenu) { // Release
            unsigned long dur = millis() - btnPressTime;
            if (dur < 800) {
                navSelect = true; // Short Press = Select/Enter
            } else {
                navHome = true;   // Long Press = Home
            }
        }
        lastMenu = currMenu;
        
        // Trims as Up/Down in Menu
        // Debounced in main loop usually, but let's do simple edge here
        static bool lastUp = true;
        static bool lastDown = true;
        bool currUp = digitalRead(PIN_BTN_TRIM_PLUS);
        bool currDown = digitalRead(PIN_BTN_TRIM_MINUS);
        
        if (lastUp && !currUp) navUp = true;
        if (lastDown && !currDown) navDown = true;
        
        lastUp = currUp;
        lastDown = currDown;
    }
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
//               GLOBALS
// ==========================================
AppState currentState = STATE_DASHBOARD;
InputState state;
InputManager input;
Preferences prefs;
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite sprite = TFT_eSprite(&tft);

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
        sprite.fillSprite(COLOR_BG_MAIN);

        // Anims
        needleAnim.set(abs(s.throttlePct));
        needleAnim.update();
        steerAnim.set(s.steerPct);
        steerAnim.update();

        int cx = 64, cy = 60;
        
        // 1. CENTRAL MPU DATA (New Feature)
        // Draw this first so needle goes over it? Or under? Under looks cooler.
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
             sprite.fillRoundRect(64 + (sPx > 0 ? 0 : sPx), barY+2, abs(sPx), barH-4, 2, COLOR_ACCENT_PRI);
        }
        sprite.drawFastVLine(64, barY, barH, COLOR_TEXT_DIS);

        // 4. FOOTER
        sprite.setTextColor(s.swGyro ? COLOR_ACCENT_TER : COLOR_TEXT_DIS);
        sprite.drawString(s.swGyro ? "GYRO" : "MANUAL", 20, 145, 1);
        
        sprite.setTextColor(COLOR_TEXT_MAIN);
        sprite.drawString("TRIM", 85, 145, 1);
        sprite.drawNumber(s.steerTrim, 115, 145, 1);
        
        // 5. RX STATUS (Top Right)
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
                // Highlight
                sprite.fillRoundRect(5, y, 118, 20, 4, COLOR_ACCENT_PRI);
                sprite.setTextColor(COLOR_BG_MAIN, COLOR_ACCENT_PRI); // Black text on Cyan
            } else {
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
//               LOGIC
// ==========================================
void handleNavigation() {
    input.update(state);
    
    // Global Home
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
            // Trim Logic in Dashboard
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
            
        // Placeholder Screens
        case STATE_TELEMETRY:
        case STATE_ABOUT:
            if (input.navSelect || input.navBack) currentState = STATE_MENU_MAIN;
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
            sprite.setTextColor(COLOR_TEXT_MAIN);
            sprite.drawString("TELEMETRY", 64, 20, 2);
            sprite.drawString("RX: --.--v", 64, 60, 2);
            sprite.drawString("TX: --.--v", 64, 90, 2);
            break;
        case STATE_ABOUT:
            sprite.fillSprite(COLOR_BG_MAIN);
            sprite.drawString("OPEN TX", 64, 40, 4);
            sprite.drawString("v2.0 Indigo", 64, 80, 2);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    prefs.begin("tx-cfg", false);
    state.steerTrim = prefs.getInt("trim_s", 0);

    // Hardcoded Pins based on user schematic provided earlier
    // Assuming standard hardware config includes
    pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
    pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
    pinMode(PIN_BTN_MENU, INPUT_PULLUP); // Assuming PIN_BTN_MENU matches HardwareConfig
    pinMode(PIN_SW_GYRO, INPUT_PULLUP);

    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true); 
    
    if (!sprite.createSprite(128, 160)) {
        Serial.println("Memory Allocation Failed!");
        while(1);
    }
    tft.fillScreen(TFT_BLACK);
}

void loop() {
    // Read Analog
    int sRaw = analogRead(PIN_STEERING) + STEER_CENTER_FIX;
    int tRaw = analogRead(PIN_THROTTLE) + THROT_CENTER_FIX;
    // Simple Norm
    state.steerPct = constrain((sRaw - 2048) / 20.48f + state.steerTrim, -100, 100);
    state.throttlePct = constrain((tRaw - 2048) / 20.48f, -100, 100);
    state.swGyro = (digitalRead(PIN_SW_GYRO) == LOW);
    
    // Simulate MPU for now
    state.mpuPitch = sin(millis()/1000.0) * 10.0;
    
    handleNavigation();
    drawScreens();
    
    sprite.pushSprite(0, 0);
    delay(5); 
}