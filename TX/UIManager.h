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
    SCREEN_TELEMETRY,
    SCREEN_SETTINGS,
    SCREEN_CALIBRATION,
    
    // New Apps
    SCREEN_APP_STOPWATCH,
    SCREEN_APP_TUNER
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
            
            // Global: MENU Button toggles or Exits
            if (btnMenu) {
                lastInputTime = now;
                soundManager.playBack();
                
                // If in App, exit to Menu
                if (currentState == SCREEN_APP_STOPWATCH || currentState == SCREEN_APP_TUNER) {
                    currentState = SCREEN_MENU;
                }
                // If in Screen, exit to Menu
                else if (currentState == SCREEN_TELEMETRY || currentState == SCREEN_SETTINGS) {
                    currentState = SCREEN_MENU;
                } 
                // If in Menu, Toggle Dashboard? Or Cycle? User seems to prefer Cycle now, but standard is toggle.
                else if (currentState == SCREEN_MENU) {
                    screenMenu.next(); // Cycle
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
                    // Pass inputs to the deep menu logic
                    if (btnP) { 
                        // If Editing, adjust value
                        // screenMenu.adjustValue(1); 
                        screenMenu.prev(); 
                        soundManager.playClick(); lastInputTime = now; 
                    }
                    if (btnM) { 
                        screenMenu.next(); 
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

                case SCREEN_APP_STOPWATCH:
                    appStopwatch.handleInput(btnSet, btnMenu);
                    appStopwatch.update();
                    if (btnSet) lastInputTime = now;
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
