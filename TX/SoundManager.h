#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "HardwareConfig.h"

// Sound Config
#define BUZZER_CHANNEL 0
#define BUZZER_RES 8

// Note Frequencies (Increased by ~50% for cleaner tone)
#define NOTE_C5 784  // Was C5 (523)
#define NOTE_D5 880
#define NOTE_E5 988
#define NOTE_F5 1046
#define NOTE_G5 1175
#define NOTE_A5 1318
#define NOTE_B5 1480
#define NOTE_C6 1568
#define NOTE_E6 1979
#define NOTE_G6 2352
#define NOTE_C7 3136

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

    // Gentle Chirp
    void playConnected() {
        if (!enabled) return;
        playTone(NOTE_C6, 60, 100);
        delay(30);
        playTone(NOTE_E6, 60, 100);
    }

    // Critical Alarm (Higher pitch)
    void playDisconnected() {
        if (!enabled) return;
        for(int k=0; k<2; k++) {
            playTone(2500, 100, 200);
            delay(50);
            playTone(2000, 100, 200);
        }
    }

    // Fast Sweep
    void beepStartup() {
        if (!enabled) return;
        for (int i=1000; i<3000; i+=200) {
            ledcAttach(PIN_BUZZER, i, BUZZER_RES);
            ledcWrite(PIN_BUZZER, 100);
            delay(3);
        }
        ledcWrite(PIN_BUZZER, 0);
        delay(50);
        playTone(NOTE_C7, 150, 150);
    }
    
    // UI Sounds - Short & Crisp
    void playClick() { playTone(2500, 10, 80); } 
    void playConfirm() { playTone(1500, 40, 100); delay(30); playTone(2200, 60, 100); }
    void playBack() { playTone(2200, 40, 100); delay(30); playTone(1500, 60, 100); }
    
    // Gyro
    void playGyroOn() {
        playTone(NOTE_E6, 40); delay(20); playTone(NOTE_G6, 80);
    }
    void playGyroOff() {
        playTone(NOTE_G6, 40); delay(20); playTone(NOTE_E6, 80);
    }
};

// Global Instance
SoundManager soundManager;

#endif // SOUND_MANAGER_H
