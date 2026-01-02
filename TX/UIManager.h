#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "InputManager.h"
#include "DisplayManager.h"
#include "Screen_Intro.h"
#include "Screen_Dashboard.h"
#include "Screen_Debug.h"
#include "Screen_Popup.h"
#include "Screen_Notification.h"
#include "Screen_Menu.h"

// Enum for Screen States
enum ScreenState {
    SCREEN_INTRO,
    SCREEN_DASHBOARD,
    SCREEN_MENU,
    SCREEN_DEBUG
};

class UIManager {
private:
    ScreenState currentState = SCREEN_INTRO;
    ScreenIntro screenIntro;
    ScreenDashboard screenDashboard;
    ScreenDebug screenDebug;
    ScreenPopup screenPopup;
    ScreenMenu screenMenu;
    ScreenNotification screenNotification; 
    
    // Input Handling (Debounce)
    unsigned long lastInputTime = 0;
    const unsigned long DEBOUNCE_DELAY = 150; // ms

public:
    void init() {
        // Ensure pins are set (Redundant safety)
        pinMode(PIN_BTN_MENU, INPUT_PULLUP);
        pinMode(PIN_BTN_SET, INPUT_PULLUP);
        pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
        pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
        
        screenIntro.init();
        screenDebug.init();
        screenMenu.init(); 
        screenNotification.init();
    } 
    
    void update() {
        // Overlay Priority
        screenNotification.update();
        if (screenNotification.isActive()) return;

        if (screenPopup.isVisible()) {
            if (screenPopup.isFinished()) screenPopup.hide();
            return; 
        }

        // --- Robust Input Handling ---
        // Read raw states
        bool btnMenu = digitalRead(PIN_BTN_MENU) == LOW; // Active Low
        bool btnSet  = digitalRead(PIN_BTN_SET) == LOW;
        bool trimP   = digitalRead(PIN_BTN_TRIM_PLUS) == LOW;
        bool trimM   = digitalRead(PIN_BTN_TRIM_MINUS) == LOW;
        
        // Simple Debounce / Cooldown
        if (millis() - lastInputTime > DEBOUNCE_DELAY) {
            
            // Global Menu Toggle
            if (btnMenu) {
                lastInputTime = millis();
                soundManager.playBack();
                if (currentState == SCREEN_MENU) currentState = SCREEN_DASHBOARD;
                else currentState = SCREEN_MENU;
                return; // Early exit to prevent double-processing
            }

            // State Machine Logic
            switch (currentState) {
                case SCREEN_DASHBOARD:
                    // Normal Driving
                    if (btnSet) {
                        lastInputTime = millis();
                        inputManager.resetTrim();
                        showPopup("TRIM RESET", "OK", COLOR_ACCENT_2);
                        soundManager.playConfirm();
                    }
                    break;
                    
                case SCREEN_MENU:
                    // Menu Navigation
                    if (trimP) { 
                        lastInputTime = millis();
                        screenMenu.next(); 
                        soundManager.playClick(); 
                    }
                    if (trimM) { 
                        lastInputTime = millis();
                        screenMenu.prev(); 
                        soundManager.playClick(); 
                    }
                    if (btnSet) {
                         lastInputTime = millis();
                         soundManager.playConfirm();
                         int sel = screenMenu.getSelection();
                         if (sel == 0) currentState = SCREEN_DASHBOARD;
                         else if (sel == 1) currentState = SCREEN_DEBUG;
                         else if (sel == 2) showPopup("TRIM SETTINGS", "USE BUTTONS", COLOR_ACCENT_4); 
                         else if (sel == 3) showPopup("OpenTX v2.1", "CYBER-NOIR", COLOR_TEXT_MAIN);
                    }
                    break;
                    
                case SCREEN_DEBUG:
                    if (btnSet) { // Exit debug
                        lastInputTime = millis();
                        currentState = SCREEN_MENU;
                    }
                    screenDebug.update();
                    break;
            }
        
        } // End Debounce check

        // Intro Logic (Independent)
        if (currentState == SCREEN_INTRO) {
            screenIntro.update();
            if (screenIntro.isFinished()) {
                currentState = SCREEN_DASHBOARD;
                soundManager.beepStartup(); 
            }
        }
    }

    void draw() {
        displayManager.beginFrame();

        switch (currentState) {
            case SCREEN_INTRO: screenIntro.draw(&displayManager); break;
            case SCREEN_DASHBOARD: screenDashboard.draw(&displayManager); break;
            case SCREEN_MENU: screenMenu.draw(&displayManager); break;
            case SCREEN_DEBUG: screenDebug.draw(&displayManager); break;
        }
        
        screenPopup.draw(&displayManager);
        screenNotification.draw(&displayManager);

        displayManager.endFrame();
    }
    
    void showPopup(const char* msg, const char* sub, uint16_t color) {
        screenPopup.show(msg, sub, color);
    }
    
    void showNotification(String msg, String sub, uint16_t color) {
        screenNotification.show(msg, sub, color);
    }
};

// Global Instance
UIManager uiManager;

#endif // UI_MANAGER_H
