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
    ScreenNotification screenNotification; // The Among Us Notifier
    
    // Previous button states for edge detection in UI
    bool lastBtnMenu = false;
    bool lastBtnSet = false;
    bool lastTrimPlus = false;
    bool lastTrimMinus = false;

public:
    void init() {
        screenIntro.init();
        screenDebug.init();
        screenMenu.init(); 
        screenNotification.init();
    } 
    
    void update() {
        // Overlay Management (Priority)
        // Check Notification first (Highest Priority - Emergency)
        screenNotification.update();
        if (screenNotification.isActive()) return; // Block input

        if (screenPopup.isVisible()) {
            if (screenPopup.isFinished()) screenPopup.hide();
            return; 
        }

        bool btnMenu = digitalRead(PIN_BTN_MENU) == LOW;
        bool btnSet  = digitalRead(PIN_BTN_SET) == LOW;
        bool trimP   = digitalRead(PIN_BTN_TRIM_PLUS) == LOW;
        bool trimM   = digitalRead(PIN_BTN_TRIM_MINUS) == LOW;
        
        // Edge Detect
        bool pMenu = btnMenu && !lastBtnMenu;
        bool pSet = btnSet && !lastBtnSet;
        bool pPlus = trimP && !lastTrimPlus;
        bool pMinus = trimM && !lastTrimMinus;

        // Intro Logic
        if (currentState == SCREEN_INTRO) {
            screenIntro.update();
            if (screenIntro.isFinished()) {
                currentState = SCREEN_DASHBOARD;
                soundManager.beepStartup(); 
            }
            return;
        }

        // Global Menu Toggle
        if (pMenu) {
            soundManager.playBack();
            if (currentState == SCREEN_MENU) currentState = SCREEN_DASHBOARD;
            else currentState = SCREEN_MENU;
        }

        // State Machine
        switch (currentState) {
            case SCREEN_DASHBOARD:
                // Normal Driving
                if (pSet) {
                    inputManager.resetTrim();
                    showPopup("TRIM RESET", "OK", COLOR_ACCENT);
                    soundManager.playConfirm();
                }
                break;
                
            case SCREEN_MENU:
                // Menu Navigation
                if (pPlus) { screenMenu.next(); soundManager.playClick(); }
                if (pMinus) { screenMenu.prev(); soundManager.playClick(); }
                if (pSet) {
                     soundManager.playConfirm();
                     int sel = screenMenu.getSelection();
                     if (sel == 0) currentState = SCREEN_DASHBOARD;
                     else if (sel == 1) currentState = SCREEN_DEBUG;
                     else if (sel == 2) showPopup("TRIM SETTINGS", "USE BUTTONS", COLOR_HIGHLIGHT); 
                     else if (sel == 3) showPopup("OpenTX v2.0", "PROFESSIONAL", COLOR_TEXT_MAIN);
                }
                break;
                
            case SCREEN_DEBUG:
                screenDebug.update();
                break;
        }
        
        lastBtnMenu = btnMenu;
        lastBtnSet = btnSet;
        lastTrimPlus = trimP;
        lastTrimMinus = trimM;
    }

    void draw() {
        displayManager.beginFrame();

        switch (currentState) {
            case SCREEN_INTRO: screenIntro.draw(&displayManager); break;
            case SCREEN_DASHBOARD: screenDashboard.draw(&displayManager); break;
            case SCREEN_MENU: screenMenu.draw(&displayManager); break;
            case SCREEN_DEBUG: screenDebug.draw(&displayManager); break;
        }
        
        // Draw Popup Overlay 
        screenPopup.draw(&displayManager);
        
        // Draw Critical Notifications (Top)
        screenNotification.draw(&displayManager);

        displayManager.endFrame();
    }
    
    void showPopup(const char* msg, const char* sub, uint16_t color) {
        screenPopup.show(msg, sub, color);
    }
    
    // Call this from CommsManager or when needed
    void showNotification(String msg, String sub, uint16_t color) {
        screenNotification.show(msg, sub, color);
    }
};

// Global Instance
UIManager uiManager;

#endif // UI_MANAGER_H
