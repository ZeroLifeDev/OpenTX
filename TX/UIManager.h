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
    
    // Better Input Handling
    unsigned long lastInputTime = 0;
    const unsigned long DEBOUNCE = 200; // Slower debounce to prevent double skips
    
    // Previous states for edge detection to allow "Hold to scroll" or just "Click"
    bool lastTrimP = false;
    bool lastTrimM = false;

public:
    void init() {
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
        screenNotification.update();
        if (screenNotification.isActive()) return;

        if (screenPopup.isVisible()) {
            if (screenPopup.isFinished()) screenPopup.hide();
            return; 
        }

        // Inputs (Active Low)
        bool btnMenu = !digitalRead(PIN_BTN_MENU);
        bool btnSet  = !digitalRead(PIN_BTN_SET);
        bool btnPlus = !digitalRead(PIN_BTN_TRIM_PLUS);
        bool btnMinus = !digitalRead(PIN_BTN_TRIM_MINUS);
        
        unsigned long now = millis();
        if (now - lastInputTime > DEBOUNCE) {
            
            // MENU TOGGLE
            if (btnMenu) {
                lastInputTime = now;
                soundManager.playBack();
                if (currentState == SCREEN_MENU) currentState = SCREEN_DASHBOARD;
                else currentState = SCREEN_MENU;
                return;
            }

            // STATE LOGIC
            switch (currentState) {
                // DASHBOARD MODE
                case SCREEN_DASHBOARD:
                    if (btnSet) {
                         lastInputTime = now;
                         inputManager.resetTrim();
                         showPopup("TRIM ZERO", "AXIS RESET", COLOR_ACCENT_2);
                         soundManager.playConfirm();
                    }
                    // Dashboard uses Trim buttons for actual trimming in InputManager::update(), 
                    // so we don't handle them here unless we want UI feedback.
                    // But InputManager handles the trim counters.
                    break;

                // MENU MODE
                case SCREEN_MENU:
                    // Navigation
                    if (btnPlus) {
                        lastInputTime = now; // consume input
                        screenMenu.next();
                        soundManager.playClick();
                    }
                    if (btnMinus) {
                        lastInputTime = now;
                        screenMenu.prev();
                        soundManager.playClick();
                    }
                    
                    // Select
                    if (btnSet) {
                        lastInputTime = now;
                        soundManager.playConfirm();
                        int sel = screenMenu.getSelection();
                        if (sel == 0) currentState = SCREEN_DASHBOARD;
                        else if (sel == 1) currentState = SCREEN_DEBUG;
                        else if (sel == 2) showPopup("SETTINGS", "NOT AVAILABLE", COLOR_ACCENT_3);
                        else if (sel == 3) showPopup("FIRMWARE", "v2.2 STABLE", COLOR_TEXT_MAIN);
                    }
                    break;
                    
                // DEBUG MODE
                case SCREEN_DEBUG:
                    screenDebug.update();
                    if (btnSet) {
                        lastInputTime = now;
                        currentState = SCREEN_MENU;
                        soundManager.playBack();
                    }
                    break;
            }
        }
        
        // Intro Logic
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
