#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <Arduino.h>
#include "HardwareConfig.h"

// Sound Config
#define BUZZER_CHANNEL 0
#define BUZZER_RES 8

class SoundManager {
private:
    bool enabled;

public:
    void init() {
        // ESP32 3.0+ API
        // ledcAttach(pin, freq, resolution)
        // Returns true on success
        if (ledcAttach(PIN_BUZZER, 2000, BUZZER_RES)) {
            ledcWrite(PIN_BUZZER, 0); // Off
            enabled = true;
        } else {
            enabled = false;
        }
    }

    // Play a gentle tone
    // Freq: Hz
    // Duration: ms
    // Volume: 0-255 (Duty Cycle). For max loudness on passive buzzer, use 127 (50%).
    void playTone(int freq, int duration, int volume = 127) {
        if (!enabled) return;
        
        ledcAttach(PIN_BUZZER, freq, BUZZER_RES); 
        ledcWrite(PIN_BUZZER, volume); 
        delay(duration);
        ledcWrite(PIN_BUZZER, 0);
    }

    void beep() {
        playTone(3000, 50, 127); 
    }

    void beepClick() {
        playTone(2000, 20, 127); 
    }
    
    void beepConfirm() {
        playTone(1500, 80, 127);
        delay(30);
        playTone(2500, 80, 127);
    }
    
    void playGyroOn() {
        // Sci-fi "Power Up" sound
        for (int i=1000; i<3000; i+=200) {
            ledcAttach(PIN_BUZZER, i, BUZZER_RES);
            ledcWrite(PIN_BUZZER, 127);
            delay(10);
        }
        ledcWrite(PIN_BUZZER, 0);
    }

    void playGyroOff() {
        // Sci-fi "Power Down" sound
        for (int i=3000; i>1000; i-=200) {
            ledcAttach(PIN_BUZZER, i, BUZZER_RES);
            ledcWrite(PIN_BUZZER, 127);
            delay(10);
        }
        ledcWrite(PIN_BUZZER, 0);
    }

    void beepStartup() {
        // Melody: C5, E5, G5, C6
        int melody[] = {523, 659, 784, 1047};
        int durations[] = {100, 100, 100, 300};
        
        for (int i = 0; i < 4; i++) {
             playTone(melody[i], durations[i], 127);
             delay(30);
        }
    }
    
    // Test sound
    void test() {
        beepStartup();
    }
};

// Global Instance
SoundManager soundManager;

#endif // SOUND_MANAGER_H
