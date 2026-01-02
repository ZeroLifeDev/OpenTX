#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "HardwareConfig.h"

class InputManager {
private:
    Preferences prefs;
    
    // Calibration Data
    struct AxisCal {
        int minVal;
        int centerVal;
        int maxVal;
        int deadzone;
    };
    
    AxisCal calSteer;
    AxisCal calThrot;
    
public:
    // Input States
    struct State {
        int steering; // -100 to 100
        int throttle; // -100 to 100
        bool joyBtn;
        int potSuspension; // 0-4095
        int trimLevel; // -20 to +20
        bool btnMenu;
        bool btnSet;
        bool btnTrimPlus;
        bool btnTrimMinus;
        bool swGyro; 
    };

    State currentState;
    State lastState; 

    int internalTrim = 0;

    void init() {
        // Pin Setup
        if (PIN_JOY_BTN != -1) pinMode(PIN_JOY_BTN, INPUT_PULLUP);
        pinMode(PIN_BTN_MENU, INPUT_PULLUP);
        pinMode(PIN_BTN_SET, INPUT_PULLUP);
        pinMode(PIN_BTN_TRIM_PLUS, INPUT_PULLUP);
        pinMode(PIN_BTN_TRIM_MINUS, INPUT_PULLUP);
        pinMode(PIN_SW_GYRO, INPUT_PULLUP);
        
        // Load Calibration
        prefs.begin("tx_cal", false);
        
        // Defaults if empty (ESP32 approx ADC)
        calSteer.minVal = prefs.getInt("s_min", 0);
        calSteer.centerVal = prefs.getInt("s_mid", 1850);
        calSteer.maxVal = prefs.getInt("s_max", 4095);
        calSteer.deadzone = prefs.getInt("s_dz", 150);
        
        calThrot.minVal = prefs.getInt("t_min", 0);
        calThrot.centerVal = prefs.getInt("t_mid", 1850);
        calThrot.maxVal = prefs.getInt("t_max", 4095);
        calThrot.deadzone = prefs.getInt("t_dz", 150);
        
        prefs.end();
    }
    
    // Runtime Calibration Setters
    void saveCalibration(int sMin, int sMid, int sMax, int tMin, int tMid, int tMax) {
        prefs.begin("tx_cal", false);
        
        calSteer.minVal = sMin;
        calSteer.centerVal = sMid;
        calSteer.maxVal = sMax;
        
        calThrot.minVal = tMin;
        calThrot.centerVal = tMid;
        calThrot.maxVal = tMax;
        
        prefs.putInt("s_min", sMin);
        prefs.putInt("s_mid", sMid);
        prefs.putInt("s_max", sMax);
        prefs.putInt("t_min", tMin);
        prefs.putInt("t_mid", tMid);
        prefs.putInt("t_max", tMax);
        
        prefs.end();
    }

    int processAxis(int raw, AxisCal* cal) {
        // 1. Center Deadzone
        if (abs(raw - cal->centerVal) < cal->deadzone) {
            return 0;
        }
        
        // 2. Map halves separately for symmetry
        int val = 0;
        if (raw < cal->centerVal) {
            // Lower half
            val = map(raw, cal->minVal, cal->centerVal - cal->deadzone, -100, 0);
        } else {
            // Upper half
            val = map(raw, cal->centerVal + cal->deadzone, cal->maxVal, 0, 100);
        }
        
        return constrain(val, -100, 100);
    }

    void update() {
        lastState = currentState;

        #if TEST_MODE
            currentState.steering = 0;
            currentState.throttle = 0;
            currentState.potSuspension = 2048;
            currentState.swGyro = false;
        #else
            // Read Raw
            int rawSteer = analogRead(PIN_STEERING);
            int rawThrot = analogRead(PIN_THROTTLE);
            
            // Process with Configured Calibration
            currentState.steering = processAxis(rawSteer, &calSteer);
            currentState.throttle = processAxis(rawThrot, &calThrot);
            currentState.potSuspension = analogRead(PIN_POT_SUSPENSION);
            currentState.swGyro = !digitalRead(PIN_SW_GYRO);
        #endif

        // Buttons
        currentState.joyBtn = (PIN_JOY_BTN != -1) ? !digitalRead(PIN_JOY_BTN) : false;
        currentState.btnMenu = !digitalRead(PIN_BTN_MENU);
        currentState.btnSet = !digitalRead(PIN_BTN_SET);
        currentState.btnTrimPlus = !digitalRead(PIN_BTN_TRIM_PLUS);
        currentState.btnTrimMinus = !digitalRead(PIN_BTN_TRIM_MINUS);

        // Logic
        if (currentState.btnTrimPlus && !lastState.btnTrimPlus) {
            if (internalTrim < 20) internalTrim++;
        }
        if (currentState.btnTrimMinus && !lastState.btnTrimMinus) {
            if (internalTrim > -20) internalTrim--;
        }
        currentState.trimLevel = internalTrim;
    }
    
    // Helpers
    int getThrottleNormalized() { return currentState.throttle; }
    int getSteeringNormalized() { return currentState.steering; }
    void resetTrim() { internalTrim = 0; }
    
    // For Calibration Screen
    int getRawSteer() { return analogRead(PIN_STEERING); }
    int getRawThrot() { return analogRead(PIN_THROTTLE); }
};

InputManager inputManager;

#endif // INPUT_MANAGER_H
