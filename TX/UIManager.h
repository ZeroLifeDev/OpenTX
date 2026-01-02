#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "InputManager.h"
#include "DisplayManager.h"
#include "Screen_Intro.h"
#include "Screen_Dashboard.h"
#include "Screen_Debug.h"
#include "Screen_Popup.h"
#include "SoundManager.h"

// Enum for Screen States
enum ScreenState {
    SCREEN_INTRO,
    SCREEN_MAIN,
    SCREEN_DEBUG
};

class UIManager {
private:
    ScreenState currentScreen;
    
public:
    void init() {
        currentScreen = SCREEN_INTRO;
        screenIntro.init();
        screenDebug.init();
    }

// Update Trim Sound Logic needs to be in loop or linked to state change
// Since InputManager handles state, we can detect edge there or check here if we had access to "changed" flags.
// InputManager stores lastState. We can check here or in InputManager.
// Better to check here in UIManager update or just let InputManager handle sound?
// UIManager is better for UI feedback.

    void update() {
        // Priority: Popup Update
        if (screenPopup.isVisible()) {
        // Handle Popup Animations
        if (screenPopup.isActive) {
            if (screenPopup.isFinished()) {
                 screenPopup.hide();
            }
            return; // Block input while popup active
        }

        bool btnMenu = digitalRead(PIN_BTN_MENU) == LOW;
        bool btnSet  = digitalRead(PIN_BTN_SET) == LOW;
        bool trimP   = digitalRead(PIN_BTN_TRIM_PLUS) == LOW;
        bool trimM   = digitalRead(PIN_BTN_TRIM_MINUS) == LOW;

        // Intro Logic
        if (currentState == SCREEN_INTRO) {
            screenIntro.update();
            if (screenIntro.isFinished()) {
                currentState = SCREEN_DASHBOARD;
                displayManager.beginFrame(); // Clear
                displayManager.endFrame();
            }
            return;
        }

        // Global: Menu Button Logic
        if (btnMenu && !lastBtnMenu) {
            // Toggle Main -> Menu -> Debug -> Main
            soundManager.playClick();
            if (currentState == SCREEN_DASHBOARD) currentState = SCREEN_MENU;
            else if (currentState == SCREEN_MENU) currentState = SCREEN_DEBUG;
            else currentState = SCREEN_DASHBOARD;
        }

        // Screen Specific Logic
        if (currentState == SCREEN_DASHBOARD) {
            // Set Button -> Reset Trim
            if (btnSet && !lastBtnSet) {
                 inputManager.resetTrim();
                 showPopup("TRIM RESET", "CENTERED", COLOR_ACCENT);
                 soundManager.playConfirm();
            }
            // Gyro Switch is handled in InputManager but we can show popup
            static bool lastGyro = false;
            if (inputManager.currentState.swGyro != lastGyro) {
                lastGyro = inputManager.currentState.swGyro;
                if (lastGyro) {
                     showPopup("GYRO SYSTEM", "ENABLED", COLOR_ACCENT);
                     soundManager.playGyroOn();
                } else {
                     showPopup("GYRO SYSTEM", "DISABLED", COLOR_ACCENT_ALT);
                     soundManager.playGyroOff();
                }
            }
        }
        else if (currentState == SCREEN_MENU) {
            // Use Trim Buttons to Navigate
            if (trimP && !lastTrimPlus) {
                screenMenu.next();
                soundManager.playClick();
            }
            if (trimM && !lastTrimMinus) {
                screenMenu.prev();
                soundManager.playClick();
            }
            if (btnSet && !lastBtnSet) {
                // Select Action
                soundManager.playConfirm();
                // For now, simplify: just go back to dash or debug based on selection?
                // Or just placeholder
            }
        }
        else if (currentState == SCREEN_DEBUG) {
            screenDebug.update();
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
        
        // Draw Popup Overlay on top
        screenPopup.draw(&displayManager);

        displayManager.endFrame();
    }
    
    void showPopup(const char* msg, const char* sub, uint16_t color) {
        screenPopup.show(msg, sub, color);
    }
};

// Global Instance
UIManager uiManager;

#endif // UI_MANAGER_H
