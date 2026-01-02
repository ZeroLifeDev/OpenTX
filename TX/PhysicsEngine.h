#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include <Arduino.h>

// ==========================================
//          HYPER-REAL PHYSICS CORE
// ==========================================
// Simulates vehicle dynamics on the TX side
// for predictive HUD and safety warnings.

class PhysicsEngine {
private:
    // --- SIMULATION CONSTANTS ---
    const float MASS = 2.5; // kg
    const float DRAG_COEFF = 0.35;
    const float ROLLING_RES = 0.02;
    const float MOTOR_KV = 3500;
    const float BATTERY_IR = 0.015; // Internal Resistance
    
    // --- STATE VARIABLES ---
    float speedKmh; // Virtual speed
    float batteryVoltage; // Virtual voltage at ESC
    float motorTemp; // Celsius
    float tireTemp; // Celsius
    float traction; // 0.0 - 1.0 (1.0 = Grip)
    
    // Vector Physics
    float gForceX;
    float gForceY;
    
    // Time tracking
    unsigned long lastUpdate;

public:
    void init() {
        speedKmh = 0;
        batteryVoltage = 8.4; // 2S lipo full
        motorTemp = 25.0; // Ambient
        tireTemp = 25.0;
        traction = 1.0;
        lastUpdate = millis();
    }

    void update(int throttleInput, int steeringInput) {
        unsigned long now = millis();
        float dt = (now - lastUpdate) / 1000.0f; // Delta time in seconds
        lastUpdate = now;
        
        if (dt > 1.0) dt = 0.01; // Clamp lags

        // 1. INPUT NORMALIZATION (-1.0 to 1.0)
        float throt = constrain(throttleInput, -100, 100) / 100.0f;
        float steer = constrain(steeringInput, -100, 100) / 100.0f;

        // 2. BATTERY SAG SIMULATION (V = V_nominal - I * R)
        // Assume I_max = 120A
        float currentDraw = abs(throt) * 120.0f; 
        // Recovery logic (chemical battery relaxation)
        float vNominal = 8.2; // Slowly dropping
        batteryVoltage = vNominal - (currentDraw * BATTERY_IR);
        
        // 3. THERMAL SIMULATION
        // Heat gain = I^2 * R * Time
        // Cooling = Airflow (speed dependent)
        float heatGen = (currentDraw * currentDraw * 0.001) * dt;
        float cooling = (speedKmh * 0.5) * dt; 
        motorTemp += heatGen - cooling;
        motorTemp = constrain(motorTemp, 25.0, 110.0); // Clamp ambient to overheat
        
        // 4. TRACTION & SLIP
        // Sharp steering at speed = Slip
        float lateralForce = abs(steer) * (speedKmh * 0.1);
        if (lateralForce > 5.0) {
            traction -= 0.1 * dt; // Losing grip
            tireTemp += 5.0 * dt; // Friction heat
        } else {
            traction += 0.2 * dt; // Regaining grip
            tireTemp -= 1.0 * dt; // Cooling
        }
        traction = constrain(traction, 0.2, 1.0);
        
        // 5. KINEMATICS (Virtual Speed)
        // F = ma
        float force = (throt * 50.0); // Motor force
        if (traction < 0.5) force *= traction; // Wheelspin loss
        
        float drag = (speedKmh * speedKmh) * 0.005; // Aero drag
        float netAccel = force - drag;
        
        speedKmh += netAccel * dt;
        if (throt == 0 && speedKmh > 0) speedKmh -= 20 * dt; // Braking/Coasting
        if (speedKmh < 0) speedKmh = 0; // No reverse physics for now simplicity
        
        // 6. G-FORCE VECTORS
        gForceY = netAccel * 0.1; // Forward/Brake G
        gForceX = steer * (speedKmh * 0.05); // Lateral G
    }
    
    // Getters
    float getVoltage() { return batteryVoltage; }
    float getMotorTemp() { return motorTemp; }
    float getTireTemp() { return tireTemp; }
    float getGForceX() { return gForceX; }
    float getGForceY() { return gForceY; }
    float getTraction() { return traction; }
    float getSimSpeed() { return speedKmh; }
};

// Global Instance
PhysicsEngine physicsEngine;

#endif // PHYSICS_ENGINE_H
