#ifndef MIXER_H
#define MIXER_H

#include "InputManager.h"
#include "ModelManager.h"
#include <Arduino.h>

// ==========================================
//          PROFESSIONAL MIXER ENGINE
// ==========================================
// Pipeline: Input -> Norm -> Reverse -> Subtrim -> Expo -> EPA -> Speed -> Output

class Mixer {
private:
    float steerVal = 0;
    float throtVal = 0;
    
    // ABS State
    unsigned long lastAbsTime = 0;
    bool absState = false;

public:
    int outputSteering; // -100 to 100 (Final)
    int outputThrottle; // -100 to 100 (Final)
    
    void update() {
        ModelData* m = modelManager.getModel();
        
        // 1. RAW INPUT (-100 to 100)
        float rawS = inputManager.getSteeringNormalized();
        float rawT = inputManager.getThrottleNormalized();
        
        // 2. REVERSE
        if (m->steering.reverse) rawS *= -1;
        if (m->throttle.reverse) rawT *= -1;
        
        // 3. SUBTRIM (Mechanical Center Offset)
        // Add before expo
        rawS += m->steering.subTrim;
        rawT += m->throttle.subTrim;
        
        // 4. MAIN TRIM (Digital) - Added from InputManager
        rawS += inputManager.currentState.trimLevel * 2; // Scale trim effect
        
        // 5. EXPO (Cubic Curve)
        // Y = X + (X^3 - X) * k
        // Makes center less sensitive (if k > 0)
        rawS = applyExpo(rawS, m->steering.expo);
        rawT = applyExpo(rawT, m->throttle.expo);
        
        // 6. END POINT ADJUSTMENT (EPA)
        // Asymmetrical Limits
        if (rawS > 0) rawS = (rawS * m->steering.epaMax) / 100.0f;
        else          rawS = (rawS * m->steering.epaMin) / 100.0f;
        
        if (rawT > 0) rawT = (rawT * m->throttle.epaMax) / 100.0f;
        else          rawT = (rawT * m->throttle.epaMin) / 100.0f;
        
        // 7. ABS (Anti-lock Brakes) - Only on negative throttle
        if (m->absLevel > 0 && rawT < -20) {
             long cycle = map(m->absLevel, 0, 100, 300, 50); // Speed of pulse
             if (millis() - lastAbsTime > cycle) {
                 absState = !absState;
                 lastAbsTime = millis();
             }
             if (absState) rawT *= 0.7; // Release brake slightly
        }
        
        // 8. SERVO SPEED SIMULATION (Slew Rate)
        // Not implemented yet to keep latency low, but placeholder
        
        // 9. CLAMP & OUTPUT
        outputSteering = constrain((int)rawS, -120, 120); // Allow over-drive if EPA > 100
        outputThrottle = constrain((int)rawT, -120, 120);
    }
    
    float applyExpo(float input, int expoVal) {
        if (expoVal == 0) return input;
        
        float val = input / 100.0f; // -1.0 to 1.0
        float k = expoVal / 100.0f;
        
        // Cubic formula
        float y = val + (val * val * val - val) * k;
        
        return y * 100.0f;
    }
    
    int getMsgSteering() { return outputSteering; }
    int getMsgThrottle() { return outputThrottle; }
};

Mixer mixer;

#endif // MIXER_H
