#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "HardwareConfig.h"

// ==========================================
//          CINEMATIC SOUND ENGINE
// ==========================================

#define BUZZER_CHANNEL 0
#define BUZZER_RES 8

// High-Fidelity Notes
#define NOTE_C5 523
#define NOTE_E5 659
#define NOTE_G5 784
#define NOTE_C6 1047
#define NOTE_E6 1319
#define NOTE_G6 1568
#define NOTE_C7 2093
#define NOTE_E7 2637

class SoundManager {
private:
    bool enabled;

public:
    void init() {
        if (ledcAttach(PIN_BUZZER, 2000, BUZZER_RES)) {
            ledcWrite(PIN_BUZZER, 0); 
            enabled = true;
        } else {
            enabled = false;
        }
    }

    void playTone(int freq, int duration, int volume = 127) {
        if (!enabled) return;
        ledcAttach(PIN_BUZZER, freq, BUZZER_RES); 
        ledcWrite(PIN_BUZZER, volume); 
        delay(duration);
        ledcWrite(PIN_BUZZER, 0);
    }
    
    void playChirp(int startFreq, int endFreq, int duration) {
        if (!enabled) return;
        int steps = 10;
        int stepDelay = duration / steps;
        int freqStep = (endFreq - startFreq) / steps;
        
        for (int i=0; i<steps; i++) {
             ledcAttach(PIN_BUZZER, startFreq + (i*freqStep), BUZZER_RES);
             ledcWrite(PIN_BUZZER, 100);
             delay(stepDelay);
        }
        ledcWrite(PIN_BUZZER, 0);
    }

    // --- CINEMATIC SEQUENCES ---

    // "Turbine Spool-up" Startup
    // Low rumble rising to high pitch scream + confirmation ping
    void playStartupPro() {
        if (!enabled) return;
        
        // 1. Rumble (Low Freq Sweep)
        for(int f=100; f<800; f+=20) {
            ledcAttach(PIN_BUZZER, f, BUZZER_RES);
            ledcWrite(PIN_BUZZER, 80);
            delay(5);
        }
        
        // 2. Turbine Whine (High Freq Sweep)
        for(int f=800; f<3000; f+=100) {
            ledcAttach(PIN_BUZZER, f, BUZZER_RES);
            ledcWrite(PIN_BUZZER, 120);
            delay(2);
        }
        
        ledcWrite(PIN_BUZZER, 0);
        delay(50);
        
        // 3. System Online (Double Ping)
        playTone(NOTE_C7, 80, 150);
        delay(40);
        playTone(NOTE_E7, 150, 200);
    }

    // Gyro: Shield Charge (Rising Power)
    void playGyroEffect(bool active) {
        if (!enabled) return;
        if (active) {
            // Charge Up
            playChirp(1000, 3000, 150);
        } else {
            // Power Down
            playChirp(3000, 500, 150);
        }
    }
    
    // UI: High Tech Ticks
    void playClick() { playTone(2500, 5, 50); } // Tiny Tick
    
    void playConfirm() { 
        playTone(1500, 40, 100); 
        delay(20); 
        playTone(2500, 60, 100); 
    }
    
    void playBack() { 
        playTone(2500, 40, 100); 
        delay(20); 
        playTone(1500, 60, 100); 
    }
    
    // Connect/Disconnect
    void playConnected() {
        playTone(NOTE_C6, 50); delay(50); playTone(NOTE_G6, 100);
    }
    
    void playDisconnected() {
        for(int i=0; i<3; i++) {
            playTone(3000, 50); delay(50); playTone(2000, 50);
        }
    }
    
    // Legacy support wrappers
    void beepStartup() { playStartupPro(); }
    void playGyroOn() { playGyroEffect(true); }
    void playGyroOff() { playGyroEffect(false); }
};

// Global Instance
SoundManager soundManager;

#endif // SOUND_MANAGER_H
