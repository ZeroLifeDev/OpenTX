#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "HardwareConfig.h"

class InputManager {
public:
    // Input States
    struct State {
        int steering; // was joyX
        int throttle; // was joyY
        bool joyBtn;
        
        int potSuspension; // 0-4095
        
        // Trim is now a value controlled by buttons
        int trimLevel; // Range -20 to +20
        
        bool btnMenu;
        bool btnSet;
        bool btnTrimPlus;
        bool btnTrimMinus;
        
        bool swGyro; // Toggle Switch
    };

    State currentState;
    State lastState; // Track previous state for edge detection

    // Internal state for logic
    int internalTrim = 0;

    void init() {
        // Digital Inputs
        if (PIN_JOY_BTN != -1) {
            pinMode(PIN_JOY_BTN, INPUT_PULLUP);
        }
        pinMode(PIN_BTN_MENU, INPUT_PULLUP);
        pinMode(PIN_BTN_SET, INPUT_PULLUP);
        
        // TRIM + is now 26 (Digital)
        pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
        pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
        pinMode(PIN_SW_GYRO, INPUT_PULLUP);
    }

    // Helper function for mapping joystick values
    // Assumes 0-4095 range for raw input
    int mapJoystick(int rawValue) {
        // Map from 0-4095 to -100-100
        return map(rawValue, 0, 4095, -100, 100);
    }

    void update() {
        // Save last state
        lastState = currentState;

        #if TEST_MODE
            // Force missing inputs to "Safe" values (Center/Zero)
            currentState.steering = 0; // Center
            currentState.throttle = 0; // Center
            currentState.potSuspension = 2048; // Middle
            currentState.swGyro = false;
            // Trim controlled by buttons below even in test mode if buttons mapped or simulated
        #else
            // Read Joysticks (ADC)
            // Adjust for hardware center offset
            int rawSteer = analogRead(PIN_STEERING);
            int rawThrot = analogRead(PIN_THROTTLE);
            
            // Map to -100 to 100
            // We use a safe mapping that ignores the deadzone in the middle
            int mapSteer = mapJoystick(rawSteer) + STEER_CENTER_FIX;
            int mapThrot = mapJoystick(rawThrot) + THROT_CENTER_FIX;
            
            // Constrain
            currentState.steering = constrain(mapSteer, -100, 100);
            currentState.throttle = constrain(mapThrot, -100, 100);
            currentState.potSuspension = analogRead(PIN_POT_SUSPENSION);
            currentState.swGyro = !digitalRead(PIN_SW_GYRO);
        #endif

        // Read Digital 
        if (PIN_JOY_BTN != -1) {
            currentState.joyBtn = !digitalRead(PIN_JOY_BTN);
        } else {
            currentState.joyBtn = false;
        }

        currentState.btnMenu = !digitalRead(PIN_BTN_MENU);
        currentState.btnSet = !digitalRead(PIN_BTN_SET);
        
        // Read Trim Buttons 
        currentState.btnTrimPlus = !digitalRead(PIN_BTN_TRIM_PLUS);
        currentState.btnTrimMinus = !digitalRead(PIN_BTN_TRIM_MINUS);

        // Trim Logic (State Machine)
        // Increment on Rising Edge of Plus
        if (currentState.btnTrimPlus && !lastState.btnTrimPlus) {
            internalTrim++;
            if (internalTrim > 20) internalTrim = 20;
        }
        // Decrement on Rising Edge of Minus
        if (currentState.btnTrimMinus && !lastState.btnTrimMinus) {
            internalTrim--;
            if (internalTrim < -20) internalTrim = -20;
        }
        
        currentState.trimLevel = internalTrim;
    }

    // Edge Detection Helpers
    bool isMenuPressed() { return currentState.btnMenu && !lastState.btnMenu; }
    bool isSetPressed() { return currentState.btnSet && !lastState.btnSet; }
    
    void resetTrim() {
        internalTrim = 0;
    }
    
    // Helper to get normalized values (-100 to 100)
    int getSteeringNormalized() {
        return currentState.steering; // Already mapped in update()
    }
    
    int getThrottleNormalized() {
        return currentState.throttle; // Already mapped in update()
    }
};

// Global Instance
InputManager inputManager;

#endif // INPUT_MANAGER_H
