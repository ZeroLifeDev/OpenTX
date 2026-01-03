#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "InputManager.h"
#include "DisplayManager.h"
#include "SoundManager.h"
#include "ModelManager.h"
#include "Mixer.h"
#include "CommsManager.h" // Added for Dashboard

// Screen Includes
#include "Screen_Intro.h"
#include "Screen_Dashboard.h"
#include "Screen_Menu.h"
#include "Screen_Calibration.h"
// Keeping basic overlays
#include "Screen_Popup.h"
#include "Screen_Notification.h"

// State Enumeration
enum ScreenState {
    SCREEN_INTRO,
    SCREEN_DASHBOARD,
    SCREEN_MENU,
    SCREEN_CALIBRATION
};

class UIManager {
private:
    ScreenState currentState = SCREEN_INTRO;
    
    // Screens
    ScreenIntro screenIntro;
    ScreenDashboard screenDashboard;
    ScreenMenu screenMenu;
    ScreenCalibration screenCalibration;
    
    // Overlays
    ScreenPopup screenPopup;
    ScreenNotification screenNotification;
    
    // Input Handling
    unsigned long lastInputTime = 0;
    const unsigned long DEBOUNCE = 180;

public:
    void init() {
        // Init Cores
        modelManager.init();
        // mixer.init(); // Mixer just needs updates
        
        // Init Screens
        screenIntro.init();
        screenMenu.init();
        screenCalibration.init();
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
        
        // --- CORE SYSTEM UPDATES ---
        // 1. Process Input -> Mixer
        mixer.update();
        
        // 2. Send Data (Real Mixer Output)
        // CommsManager should read from Mixer, not Input directly
        // comms.send(mixer.getMsgThrottle(), mixer.getMsgSteering()); // (Concept)

        // --- INPUT HANDLING ---
        unsigned long now = millis();
        bool btnMenu = !digitalRead(PIN_BTN_MENU);
        bool btnSet  = !digitalRead(PIN_BTN_SET);
        bool btnP    = !digitalRead(PIN_BTN_TRIM_PLUS); 
        bool btnM    = !digitalRead(PIN_BTN_TRIM_MINUS); 

        if (now - lastInputTime > DEBOUNCE) {
            
            // Combo: MENU + TRIM+ -> Calibration
            if (btnMenu && btnP) {
                currentState = SCREEN_CALIBRATION;
                screenCalibration.init();
                lastInputTime = now;
                return;
            }

            // Global: MENU Button toggles or Exits
            if (btnMenu) {
                lastInputTime = now;
                soundManager.playBack();
                
                // If in Calibration, handled by state machine or exit?
                if (currentState == SCREEN_CALIBRATION) {
                    currentState = SCREEN_MENU;
                    return;
                }
                
                // Default handling: Go to Menu or Toggle Dashboard
                if (currentState == SCREEN_MENU) {
                    // If in Menu root, maybe exit to Dashboard?
                    // For now, let's keep it simple: Menu Button Cycles or Enters Menu
                    // User preference seems to be "Menu button opens Menu"
                    currentState = SCREEN_DASHBOARD;
                } else {
                    currentState = SCREEN_MENU;
                }
                return; 
            }

            // State Machine
            switch(currentState) {
                case SCREEN_DASHBOARD:
                    if (btnMenu) { currentState = SCREEN_MENU; lastInputTime = now; }
                    break;
                    
                case SCREEN_MENU:
                    // MENU LOGIC
                    if (btnP) { 
                        if (screenMenu.isEditing()) screenMenu.adjustValue(1); 
                        else screenMenu.prev(); 
                        soundManager.playClick(); lastInputTime = now; 
                    }
                    if (btnM) { 
                        if (screenMenu.isEditing()) screenMenu.adjustValue(-1); 
                        else screenMenu.next(); 
                        soundManager.playClick(); lastInputTime = now; 
                    }
                    
                    if (btnSet) {
                        screenMenu.select();
                        soundManager.playConfirm();
                        lastInputTime = now; 
                    }
                    
                    if (btnMenu) {
                        screenMenu.back(); // Back navigation
                        lastInputTime = now;
                        // If at root and press back? Maybe exit to dash?
                        // Implemented in back() or here?
                    }
                    break;
                    
                case SCREEN_CALIBRATION:
                    screenCalibration.handleInput(btnSet);
                    if (screenCalibration.getStage() == 3) {
                         // Done
                         delay(1000);
                         currentState = SCREEN_MENU;
                    }
                    if (btnMenu) currentState = SCREEN_MENU; // Abort
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
            case SCREEN_CALIBRATION: screenCalibration.draw(&displayManager); break;
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
