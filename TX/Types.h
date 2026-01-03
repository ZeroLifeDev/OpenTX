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
class AnimFloat {
private:
    float _val, _target, _smooth;
public:
    AnimFloat(float val, float smooth) : _val(val), _target(val), _smooth(smooth) {}
    void set(float t) { _target = t; }
    void update() { _val += (_target - _val) * _smooth; }
    float val() { return _val; }
};

#endif
