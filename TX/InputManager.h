#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "Sound.h" // Access to sound for clicks

// --- PIN DEFINITIONS (Fallbacks) ---
#ifndef PIN_BTN_TRIM_PLUS
  #define PIN_BTN_TRIM_PLUS 18
#endif
#ifndef PIN_BTN_TRIM_MINUS
  #define PIN_BTN_TRIM_MINUS 19
#endif
#ifndef PIN_BTN_MENU
  #define PIN_BTN_MENU 23
#endif
#ifndef PIN_BTN_SET
  #define PIN_BTN_SET 25 // Default Set Pin
#endif

class InputManager {
private:
    unsigned long btnPressTime = 0;
    bool lastMenuState = true; 
    bool lastSetState = true;
    bool lastUpState = true;
    bool lastDownState = true;
    
public:
    // Navigation Events
    bool navUp = false;
    bool navDown = false;
    bool navSet = false; // "Select" / Enter
    bool navBack = false; // "Back" / Exit
    bool navHome = false; // Long Press Menu

    void init() {
         pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
         pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
         pinMode(PIN_BTN_MENU, INPUT_PULLUP);
         pinMode(PIN_BTN_SET, INPUT_PULLUP);
    }

    void update() {
        // Reset One-Shot Events
        navUp = false; navDown = false; navSet = false; navBack = false; navHome = false;
        
        // 1. MENU BUTTON (BACK / HOME)
        bool currMenu = digitalRead(PIN_BTN_MENU);
        if (lastMenuState && !currMenu) { // Press
            btnPressTime = millis();
        }
        if (!lastMenuState && currMenu) { // Release
            unsigned long dur = millis() - btnPressTime;
            if (dur < 800) {
                navBack = true; // Short = Back
                sound.playBack();
            } else {
                navHome = true; // Long = Home
                sound.playBack();
            }
        }
        lastMenuState = currMenu;

        // 2. SET BUTTON (SELECT)
        bool currSet = digitalRead(PIN_BTN_SET);
        if (lastSetState && !currSet) {
             navSet = true;
             sound.playSelect();
        }
        lastSetState = currSet;
        
        // 3. TRIM TRANSITIONS (SCROLL)
        bool currUp = digitalRead(PIN_BTN_TRIM_PLUS);
        bool currDown = digitalRead(PIN_BTN_TRIM_MINUS);
        
        if (lastUpState && !currUp) {
             navUp = true;
             sound.playScroll();
        }
        if (lastDownState && !currDown) {
             navDown = true;
             sound.playScroll();
        }
        
        lastUpState = currUp;
        lastDownState = currDown;
    }
};

static InputManager input; // Global instance

#endif
