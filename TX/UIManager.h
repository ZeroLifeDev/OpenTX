#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "InputManager.h"
#include "DisplayManager.h"
#include "SoundManager.h"

// Screen Includes
#include "Screen_Intro.h"
#include "Screen_Dashboard.h"
#include "Screen_Menu.h"
#include "Screen_Telemetry.h"
#include "Screen_Settings.h"
#include "Screen_Popup.h"
#include "Screen_Notification.h"

// State Enumeration
enum ScreenState {
    SCREEN_INTRO,
    SCREEN_DASHBOARD,
    SCREEN_MENU,
    SCREEN_TELEMETRY,
    SCREEN_SETTINGS
};

class UIManager {
private:
    ScreenState currentState = SCREEN_INTRO;
    
    // Screens
    ScreenIntro screenIntro;
    ScreenDashboard screenDashboard;
    ScreenMenu screenMenu;
    ScreenTelemetry screenTelemetry;
    ScreenSettings screenSettings;
    
    // Overlays
    ScreenPopup screenPopup;
    ScreenNotification screenNotification;
    
    // Input Handling
    unsigned long lastInputTime = 0;
    const unsigned long DEBOUNCE = 180;

public:
    void init() {
        // Init Screens
        screenIntro.init();
        screenMenu.init();
        screenTelemetry.init();
        screenSettings.init();
        screenNotification.init();
        
        // Ensure Inputs
        pinMode(PIN_BTN_MENU, INPUT_PULLUP);
        pinMode(PIN_BTN_SET, INPUT_PULLUP);
        pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
        pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
    }
    
    void update() {
        // Priority Overlays
        screenNotification.update();
        if (screenNotification.isActive()) return;
        if (screenPopup.isVisible() && screenPopup.isFinished()) screenPopup.hide();
        
        // Input Handling
        unsigned long now = millis();
        bool btnMenu = !digitalRead(PIN_BTN_MENU);
        bool btnSet  = !digitalRead(PIN_BTN_SET);
        bool btnP    = !digitalRead(PIN_BTN_TRIM_PLUS); // Used as UP/RIGHT
        bool btnM    = !digitalRead(PIN_BTN_TRIM_MINUS); // Used as DOWN/LEFT

        if (now - lastInputTime > DEBOUNCE) {
            
            // Global: MENU Button toggles Dashboard/Menu
            if (btnMenu) {
                lastInputTime = now;
                soundManager.playBack();
                
                // If in deep sub-menu, go back to Menu first
                if (currentState == SCREEN_TELEMETRY || currentState == SCREEN_SETTINGS) {
                    currentState = SCREEN_MENU;
                } else if (currentState == SCREEN_MENU) {
                    // In Menu, Button Cycles option OR Exits?
                    // User requested cycling with Menu button previously.
                    // Let's support both: Cycle with Menu? No, standard is Menu back/toggle.
                    // User said "I WANT THE MENU BUTTON TO LOOP THROUGH THE OPTION".
                    screenMenu.next();
                    soundManager.playClick();
                } else {
                    currentState = SCREEN_MENU;
                }
                return; 
            }

            // State Machine
            switch(currentState) {
                case SCREEN_DASHBOARD:
                    // Set -> Quick Action?
                    if (btnSet) {
                         lastInputTime = now;
                         inputManager.resetTrim();
                         showPopup("TRIM RESET", "OK", COLOR_ACCENT_PRI);
                         soundManager.playConfirm();
                    }
                    break;
                    
                case SCREEN_MENU:
                    // Nav also supported via Trim buttons
                    if (btnP) { screenMenu.prev(); soundManager.playClick(); lastInputTime = now; } // Up
                    if (btnM) { screenMenu.next(); soundManager.playClick(); lastInputTime = now; } // Down
                    
                    if (btnSet) {
                        lastInputTime = now;
                        soundManager.playConfirm();
                        int s = screenMenu.getSelection();
                        if (s == 0) currentState = SCREEN_DASHBOARD;
                        if (s == 1) currentState = SCREEN_TELEMETRY;
                        if (s == 2) currentState = SCREEN_SETTINGS;
                        if (s == 3) showPopup("ABOUT", "OpenTX Pro v3", COLOR_TEXT_MAIN);
                    }
                    break;
                    
                case SCREEN_TELEMETRY:
                    screenTelemetry.update();
                    if (btnSet) { 
                        lastInputTime = now; 
                        currentState = SCREEN_MENU; 
                        soundManager.playBack(); 
                    }
                    break;
                    
                case SCREEN_SETTINGS:
                    if (btnP || btnM) { 
                         lastInputTime = now;
                         screenSettings.next(); 
                         soundManager.playClick(); 
                    }
                    if (btnSet) {
                         lastInputTime = now;
                         screenSettings.toggle();
                         soundManager.playConfirm();
                    }
                    break;
            }
        }
        
        // Intro
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
            case SCREEN_TELEMETRY: screenTelemetry.draw(&displayManager); break;
            case SCREEN_SETTINGS: screenSettings.draw(&displayManager); break;
        }
        
        screenPopup.draw(&displayManager);
        screenNotification.draw(&displayManager);
        
        displayManager.endFrame();
    }
    
    void showPopup(const char* m, const char* s, uint16_t c) {
        screenPopup.show(m, s, c);
    }
    
    void showNotification(String m, String s, uint16_t c) {
        screenNotification.show(m, s, c);
    }
};

// Global
UIManager uiManager;

#endif // UI_MANAGER_H
