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
    // Volume: 0-255 (Keep low for "Quiet")
    void playTone(int freq, int duration, int volume = 10) {
        if (!enabled) return;
        
        // Update Frequency
        // In 3.0, we can re-attach or use ledcChangeFrequency if available, 
        // but simplest compatible way is often just re-attach or ledcWriteTone (if we didn't want volume).
        // To control volume (duty), we need manual PWM.
        
        ledcAttach(PIN_BUZZER, freq, BUZZER_RES); 
        ledcWrite(PIN_BUZZER, volume); // Low duty cycle = Quiet
        delay(duration);
        ledcWrite(PIN_BUZZER, 0);
    }

    void beep() {
        // Very short, quiet blip
        playTone(3000, 30, 5); // 3kHz, 30ms, very low volume
    }

    void beepClick() {
        // Like a button click
        playTone(2000, 10, 5); 
    }
    
    void beepConfirm() {
        // Ascending
        playTone(1500, 50, 5);
        delay(30);
        playTone(2500, 50, 5);
    }

    void beepStartup() {
        playTone(1000, 100, 10);
        delay(50);
        playTone(2000, 100, 10);
        delay(50);
        playTone(3000, 200, 10);
    }
    
    // Test sound
    void test() {
        beepStartup();
    }
};

// Global Instance
SoundManager soundManager;

#endif // SOUND_MANAGER_H
