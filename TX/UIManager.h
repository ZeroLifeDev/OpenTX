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
            screenPopup.update();
            // Don't block background updates entirely if we want live bg, 
            // but for inputs we might want to block? 
            // Let's allow background updates but block input parsing for screen switching.
        }

        // Sound Feedback for Trim (Check Edge)
        if (inputManager.currentState.btnTrimPlus && !inputManager.lastState.btnTrimPlus) {
             soundManager.beepClick();
        }
        if (inputManager.currentState.btnTrimMinus && !inputManager.lastState.btnTrimMinus) {
             soundManager.beepClick();
        }
        
        // Sound Feedback & Cutscene for Gyro
        if (inputManager.currentState.swGyro != inputManager.lastState.swGyro) {
             if (inputManager.currentState.swGyro) {
                 soundManager.playGyroOn();
                 screenPopup.show("GYRO SYSTEM", "ENGAGED", 1500, COLOR_ACCENT);
             } else {
                 soundManager.playGyroOff();
                 screenPopup.show("GYRO SYSTEM", "DISABLED", 1500, COLOR_ACCENT_ALT);
             }
        }

        switch (currentScreen) {
            case SCREEN_INTRO:
                screenIntro.update();
                if (screenIntro.isFinished()) {
                    currentScreen = SCREEN_MAIN;
                }
                break;
                
            case SCREEN_MAIN:
                if (!screenPopup.isVisible()) {
                    // Switch to Debug on Menu Button Press
                    if (inputManager.isMenuPressed()) {
                        soundManager.beepConfirm();
                        currentScreen = SCREEN_DEBUG;
                    }
                    // Use SET button to Reset Trim
                    if (inputManager.isSetPressed()) {
                        soundManager.beep();
                        inputManager.internalTrim = 0;
                        screenPopup.show("TRIM RESET", "CENTERED", 1000, COLOR_HIGHLIGHT);
                    }
                }
                break;
                
            case SCREEN_DEBUG:
                screenDebug.update();
                if (!screenPopup.isVisible()) {
                    // Exit Debug on Menu Button Press
                    if (inputManager.isMenuPressed()) {
                        soundManager.beepConfirm();
                        currentScreen = SCREEN_MAIN;
                    }
                }
                break;
        }
    }

    void draw() {
        displayManager.beginFrame();
        
        switch (currentScreen) {
            case SCREEN_INTRO:
                screenIntro.draw(&displayManager);
                break;
                
            case SCREEN_MAIN:
                screenDashboard.draw(&displayManager);
                break;
                
            case SCREEN_DEBUG:
                screenDebug.draw(&displayManager);
                break;
        }

        // Draw Overlay Layer
        if (screenPopup.isVisible()) {
            screenPopup.draw(&displayManager);
        }

        displayManager.endFrame();
    }
};

// Global Instance
UIManager uiManager;

#endif // UI_MANAGER_H
