#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include <Preferences.h>

// --- CONFIG & CONSTANTS ---
#if __has_include("HardwareConfig.h")
  #include "HardwareConfig.h"
#endif

// Fallback Pin Config
#ifndef PIN_STEERING
  #define PIN_STEERING 34
#endif
#ifndef PIN_THROTTLE
  #define PIN_THROTTLE 35
#endif
#ifndef PIN_POT_SUSPENSION
  #define PIN_POT_SUSPENSION 32
#endif
#ifndef PIN_SW_GYRO
  #define PIN_SW_GYRO 22
#endif

// Calibration Offsets
#ifndef STEER_CENTER_FIX
  #define STEER_CENTER_FIX 0
#endif
#ifndef THROT_CENTER_FIX
  #define THROT_CENTER_FIX 0
#endif

// --- MODULAR INCLUDES ---
#include "Types.h"
#include "Theme.h"
#include "Sound.h"
#include "InputManager.h"
#include "Dashboard.h"
#include "Menu.h"

// --- GLOBALS ---
Preferences prefs;
TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite sprite = TFT_eSprite(&tft); // Global sprite for screens

// State
AppState currentState = STATE_DASHBOARD;
InputState state;

// ==========================================
//            MAIN LOGIC & LOOP
// ==========================================
void handleNavigation() {
    input.update();
    
    // Global Home Short-circuit
    if (input.navHome) {
        if (currentState != STATE_DASHBOARD) sound.playBack();
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
            if (input.navUp) { 
                state.steerTrim++; 
                prefs.putInt("trim_s", state.steerTrim); 
            }
            if (input.navDown) { 
                state.steerTrim--; 
                prefs.putInt("trim_s", state.steerTrim); 
            }
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
            if (input.navSelect) {
                sound.playBack();
                currentState = STATE_MENU_MAIN; 
            }
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
    
    // 1. Hardware Init
    input.init();
    sound.init();

    // 2. Preferences
    prefs.begin("tx-cfg", false);
    state.steerTrim = prefs.getInt("trim_s", 0);

    // 3. Pin Setup (Analog/Direct)
    pinMode(PIN_STEERING, INPUT);
    pinMode(PIN_THROTTLE, INPUT);
    pinMode(PIN_POT_SUSPENSION, INPUT);
    pinMode(PIN_SW_GYRO, INPUT_PULLUP);

    // 4. Display Setup
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true); 
    
    if (!sprite.createSprite(128, 160)) {
        Serial.println("Memory Allocation Failed!");
        while(1);
    }
    
    tft.fillScreen(TFT_BLACK);
    
    // 5. Ready Sound
    sound.playStartup();
}

void loop() {
    // 1. Read Inputs
    int sRaw = analogRead(PIN_STEERING) + STEER_CENTER_FIX;
    int tRaw = analogRead(PIN_THROTTLE) + THROT_CENTER_FIX;
    state.rawSuspension = analogRead(PIN_POT_SUSPENSION);
    
    // 2. Normalize
    float tPct = (tRaw - 2048) / 20.48f;
    float sPct = (sRaw - 2048) / 20.48f + state.steerTrim;
    
    // Deadzone
    if (abs(tPct) < 5.0) tPct = 0;
    if (abs(sPct) < 2.0) sPct = 0;

    state.steerPct = constrain(sPct, -100, 100);
    state.throttlePct = constrain(tPct, -100, 100);
    state.swGyro = (digitalRead(PIN_SW_GYRO) == LOW);
    
    // Simulate Speed
    if (state.throttlePct > 0) state.speedKmh += 0.5;
    else state.speedKmh -= 0.5;
    if (state.throttlePct == 0) state.speedKmh *= 0.95;
    state.speedKmh = constrain(state.speedKmh, 0, 120);
    
    // 3. Logic & Draw
    handleNavigation();
    sound.update(); // Update Audio Loop
    drawScreens();
    
    // 4. Push to Screen
    sprite.pushSprite(0, 0);
    delay(5); 
}