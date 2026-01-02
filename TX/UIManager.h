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
    
    unsigned long lastInputTime = 0;
    const unsigned long DEBOUNCE = 250; // Slower debounce

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

        bool btnMenu = !digitalRead(PIN_BTN_MENU);
        bool btnSet  = !digitalRead(PIN_BTN_SET);
        bool btnPlus = !digitalRead(PIN_BTN_TRIM_PLUS);
        bool btnMinus = !digitalRead(PIN_BTN_TRIM_MINUS);
        
        unsigned long now = millis();
        if (now - lastInputTime > DEBOUNCE) {
            
            // --- STATE MACHINE ---
            switch (currentState) {
                
                // 1. DASHBOARD Mode
                // MENU -> Go to Menu
                // SET  -> Reset Trim
                // TRIM -> Adjust Trim (Handled by InputManager, but we assume we want simple feedback here?)
                case SCREEN_DASHBOARD:
                    if (btnMenu) {
                        lastInputTime = now;
                        currentState = SCREEN_MENU;
                        soundManager.playBack();
                    }
                    if (btnSet) {
                         lastInputTime = now;
                         inputManager.resetTrim();
                         showPopup("TRIM RESET", "CENTERED", COLOR_ACCENT_2);
                         soundManager.playConfirm();
                    }
                    // Trim buttons work natively in InputManager, no logic needed here except maybe sound
                    if (btnPlus || btnMinus) {
                        lastInputTime = now;
                        soundManager.playClick();
                    }
                    break;

                // 2. MENU Mode
                // MENU -> Cycle Option (User Request)
                // SET  -> Select Option
                // TRIM -> Nothing (Or Scroll?)
                case SCREEN_MENU:
                    if (btnMenu) {
                        // User wants MENU to loop through options
                        lastInputTime = now;
                        screenMenu.next();
                        soundManager.playClick(); 
                    }
                    
                    if (btnSet) {
                        lastInputTime = now;
                        soundManager.playConfirm();
                        int sel = screenMenu.getSelection();
                        
                        // Action
                        if (sel == 0) currentState = SCREEN_DASHBOARD; // "DASHBOARD"
                        else if (sel == 1) currentState = SCREEN_DEBUG; // "TELEMETRY"
                        else if (sel == 2) showPopup("SETTINGS", "LOCKED", COLOR_ACCENT_3);
                        else if (sel == 3) showPopup("FIRMWARE", "v2.5 PRO", COLOR_TEXT_MAIN);
                    }
                    break;
                    
                // 3. DEBUG Mode
                // MENU/SET -> Exit
                case SCREEN_DEBUG:
                    screenDebug.update();
                    if (btnMenu || btnSet) {
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
