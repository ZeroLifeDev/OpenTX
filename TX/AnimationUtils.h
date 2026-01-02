#ifndef ANIMATION_UTILS_H
#define ANIMATION_UTILS_H

#include <Arduino.h>

// ==========================================
//          OVERENGINEERED ANIMATION
// ==========================================

struct AnimFloat {
    float current;
    float target;
    float velocity;
    float k; // Spring constant (stiffness)
    float d; // Damping ratio
    
    AnimFloat(float val = 0.0f, float stiffness = 0.2f, float damping = 0.8f) {
        current = val;
        target = val;
        velocity = 0;
        k = stiffness;
        d = damping;
    }
    
    void set(float t) {
        target = t;
    }

    void snap(float t) {
        target = t;
        current = t;
        velocity = 0;
    }
    
    // Alias for snap
    void reset(float t) { snap(t); }
    
    void update() {
        float force = (target - current) * k;
        velocity += force;
        velocity *= d;
        current += velocity;
        
        // Anti-jitter for very small values
        if (abs(current - target) < 0.01 && abs(velocity) < 0.01) {
            current = target;
            velocity = 0;
        }
    }
    
    float val() const { return current; }
    int iVal() const { return (int)round(current); }
};

class MicroJitter {
private:
    unsigned long lastUpdate;
    int range;
    int currentVal;
public:
    MicroJitter(int r = 1) : range(r), lastUpdate(0), currentVal(0) {}
    
    int get() {
        if (millis() - lastUpdate > 30) { // 30Hz Jitter
             lastUpdate = millis();
             currentVal = random(-range, range + 1);
        }
        return currentVal;
    }
};

// Helper for "Scanning" lines
struct ScanLine {
    float pos;
    float speed;
    int minVal;
    int maxVal;
    
    ScanLine(int min, int max, float spd) : minVal(min), maxVal(max), speed(spd) {
        pos = min;
    }
    
    void update() {
        pos += speed;
        if (pos > maxVal) pos = minVal;
        if (pos < minVal) pos = maxVal;
    }
    
    int y() const { return (int)pos; }
};

#endif // ANIMATION_UTILS_H
