#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ==========================================
//               GLOBAL STATE
// ==========================================

enum AppState {
    STATE_DASHBOARD,
    STATE_MENU_MAIN,
    STATE_MENU_SETTINGS,
    STATE_TELEMETRY,
    STATE_ABOUT
};

struct InputState {
    float steerPct = 0;
    float throttlePct = 0;
    int rawSuspension = 0;
    int steerTrim = 0;
    
    // Telemetry / Sensor Data
    float speedKmh = 0.0;
    float mpuPitch = 0; 
    bool rxConnected = false;
    bool swGyro = false;
};

// ==========================================
//           SHARED UTILS
// ==========================================
// ==========================================
//           SHARED UTILS
// ==========================================
class AnimFloat {
private:
    float _val, _target, _vel;
    float _stiffness; // 0.1 to 0.5 (Spring force)
    float _damping;   // 0.0 to 1.0 (Friction)
public:
    // Constructor: Val, Stiffness (Strength), Damping (Friction)
    // Good defaults: 0.2, 0.85
    AnimFloat(float val, float stiffness, float damping) 
        : _val(val), _target(val), _vel(0), _stiffness(stiffness), _damping(damping) {}
        
    void set(float t) { _target = t; }
    
    void update() {
        // Spring Physics: F = -kx
        float force = (_target - _val) * _stiffness;
        _vel += force;
        _vel *= _damping; // Apply friction
        _val += _vel;
    }
    
    float val() { return _val; }
};

#endif
