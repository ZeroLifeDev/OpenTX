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

class InputManager {
private:
    unsigned long btnPressTime = 0;
    bool lastMenuState = true; 
    bool lastUpState = true;
    bool lastDownState = true;
    
public:
    // Navigation Events
    bool navUp = false;
    bool navDown = false;
    bool navSelect = false;
    bool navHome = false;

    void init() {
         pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
         pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
         pinMode(PIN_BTN_MENU, INPUT_PULLUP);
    }

    void update() {
        // Reset One-Shot Events
        navUp = false; navDown = false; navSelect = false; navHome = false;
        
        // 1. MENU BUTTON LOGIC
        bool currMenu = digitalRead(PIN_BTN_MENU);
        
        if (lastMenuState && !currMenu) { // Button Pressed (Falling Edge)
            btnPressTime = millis();
        }
        if (!lastMenuState && currMenu) { // Button Released (Rising Edge)
            unsigned long dur = millis() - btnPressTime;
            if (dur < 800) {
                navSelect = true; // Short Press = Select
                sound.playClick();
            } else {
                navHome = true;   // Long Press = Home
                sound.playBack();
            }
        }
        lastMenuState = currMenu;
        
        // 2. TRIM BUTTONS AS NAVIGATION
        bool currUp = digitalRead(PIN_BTN_TRIM_PLUS);
        bool currDown = digitalRead(PIN_BTN_TRIM_MINUS);
        
        if (lastUpState && !currUp) {
             navUp = true;
             sound.playClick();
        }
        if (lastDownState && !currDown) {
             navDown = true;
             sound.playClick();
        }
        
        lastUpState = currUp;
        lastDownState = currDown;
    }
};

static InputManager input; // Global instance

#endif
