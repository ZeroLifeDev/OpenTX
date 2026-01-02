#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "InputManager.h"
#include "DisplayManager.h"
#include "SoundManager.h"
#include "PhysicsEngine.h"
#include "DataLogger.h"
#include "GXKernel.h"
#include "OS_Apps.h"

// Screen Includes
#include "Screen_Intro.h"
#include "Screen_Dashboard.h"
#include "Screen_Menu.h"
#include "Screen_Telemetry.h"
#include "Screen_Settings.h"
#include "Screen_Popup.h"
#include "Screen_Notification.h"
#include "Screen_Calibration.h"

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
        gxKernel.init();
        physicsEngine.init();
        dataLogger.init();
        
        // Init Screens
        screenIntro.init();
        screenMenu.init();
        screenTelemetry.init();
        screenSettings.init();
        screenCalibration.init();
        screenNotification.init();
        
        // Init Apps
        appStopwatch.init();
        appTuner.init();
        
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
        // Run Physics every frame
        physicsEngine.update(inputManager.currentState.throttle, inputManager.currentState.steering);
        
        // Log Data (Throttle, Steering, FakeSignal)
        static unsigned long lastLog = 0;
        if (millis() - lastLog > 50) { // 20Hz logging
            dataLogger.log(inputManager.currentState.throttle, inputManager.currentState.steering, 90);
            lastLog = millis();
        }
        
        // Update GX
        gxKernel.updateParticles();

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
                    if (btnSet) {
                         lastInputTime = now;
                         inputManager.resetTrim();
                         showPopup("TRIM RESET", "OK", COLOR_ACCENT_PRI);
                         soundManager.playConfirm();
                    }
                    if (btnP || btnM) {
                        // Pass input to InputManager trim logic directly? 
                        // It's handled in InputManager::update(), but we can add UI feedback here
                    }
                    break;
                    
                case SCREEN_MENU:
                    if (btnP) { screenMenu.prev(); soundManager.playClick(); lastInputTime = now; }
                    if (btnM) { screenMenu.next(); soundManager.playClick(); lastInputTime = now; }
                    
                    if (btnSet) {
                        lastInputTime = now;
                        soundManager.playConfirm();
                        int s = screenMenu.getSelection();
                        // 0=Dash, 1=Telem, 2=Settings, 3=About -> We need to expand Menu items for Apps
                        // Hardcoded for now, let's inject Apps into index 1 and 2
                        if (s == 0) currentState = SCREEN_DASHBOARD;
                        if (s == 1) currentState = SCREEN_TELEMETRY;
                        if (s == 2) currentState = SCREEN_SETTINGS;
                        if (s == 3) currentState = SCREEN_APP_STOPWATCH; // Replaced About for now or added
                    }
                    break;
                    
                case SCREEN_TELEMETRY:
                    screenTelemetry.update();
                    if (btnSet) { currentState = SCREEN_MENU; lastInputTime = now; soundManager.playBack(); }
                    break;
                    
                case SCREEN_SETTINGS:
                    if (btnP || btnM) { screenSettings.next(); soundManager.playClick(); lastInputTime = now; }
                    if (btnSet) { 
                        // Long press SET in settings to enter calibration? 
                        // Or just toggle normal settings. 
                        // Let's make it so if you scroll past last item it goes to Calib?
                        // For now, let's just make it a hidden combo: MENU + UP
                        screenSettings.toggle(); soundManager.playConfirm(); lastInputTime = now; 
                    }
                    if (btnMenu && btnP) { currentState = SCREEN_CALIBRATION; screenCalibration.init(); }
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
            case SCREEN_TELEMETRY: screenTelemetry.draw(&displayManager); break;
            case SCREEN_SETTINGS: screenSettings.draw(&displayManager); break;
            case SCREEN_CALIBRATION: screenCalibration.draw(&displayManager); break;
            case SCREEN_APP_STOPWATCH: appStopwatch.draw(&displayManager); break;
            case SCREEN_APP_TUNER: appTuner.draw(&displayManager); break;
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
