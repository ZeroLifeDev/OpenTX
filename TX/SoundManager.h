#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "HardwareConfig.h"

// Sound Config
#define BUZZER_CHANNEL 0
#define BUZZER_RES 8


// Note Frequencies
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_E6 1319
#define NOTE_G6 1568
#define NOTE_C7 2093

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

    // Play a gentle rising arpeggio (Connected)
    void playConnected() {
        if (!enabled) return;
        int notes[] = {NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6, NOTE_E6};
        for (int i=0; i<5; i++) {
            playTone(notes[i], 80, 100);
            delay(10);
        }
    }

    // Critical Alarm / Disconnect sound
    // Sharp, alarming siren pattern
    void playDisconnected() {
        if (!enabled) return;
        // Siren: High Low High Low
        for(int k=0; k<3; k++) {
            playTone(1500, 150, 200);
            playTone(1000, 150, 200);
        }
    }

    // System Boot Sweep
    void beepStartup() {
        if (!enabled) return;
        // rapid rise
        for (int i=500; i<2500; i+=100) {
            ledcAttach(PIN_BUZZER, i, BUZZER_RES);
            ledcWrite(PIN_BUZZER, 100);
            delay(5);
        }
        ledcWrite(PIN_BUZZER, 0);
        delay(100);
        // Final "Ping"
        playTone(NOTE_C7, 300, 150);
    }
    
    // UI Sounds
    void playClick() { playTone(2000, 15, 80); } // Short, crisp
    void playConfirm() { playTone(1200, 50, 100); delay(50); playTone(1800, 80, 100); }
    void playBack() { playTone(1800, 50, 100); delay(50); playTone(1200, 80, 100); }
    
    // Gyro
    void playGyroOn() {
        playTone(NOTE_C5, 50); playTone(NOTE_E5, 50); playTone(NOTE_G5, 100);
    }
    void playGyroOff() {
        playTone(NOTE_G5, 50); playTone(NOTE_E5, 50); playTone(NOTE_C5, 100);
    }
};

// Global Instance
SoundManager soundManager;

#endif // SOUND_MANAGER_H

