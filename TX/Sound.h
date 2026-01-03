#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>

// Define Buzzer Pin if not globally defined
#ifndef PIN_BUZZER
  #define PIN_BUZZER 4 // Default to IO 4 if undefined
#endif

class SoundManager {
private:
    unsigned long toneEndTime = 0;
    bool tonesActive = false;

public:
    void init() {
        pinMode(PIN_BUZZER, OUTPUT);
        digitalWrite(PIN_BUZZER, LOW);
    }

    void update() {
        if (tonesActive && millis() > toneEndTime) {
            noTone(PIN_BUZZER);
            tonesActive = false;
            digitalWrite(PIN_BUZZER, LOW); // Ensure off
        }
    }

    // Soft Haptic Chime: High freq, short duration
    void playClick() {
        if (!tonesActive) { 
            tone(PIN_BUZZER, 2400); // 2.4kHz "Click"
            toneEndTime = millis() + 15; 
            tonesActive = true;
        }
    }
    
    void playScroll() {
        if (!tonesActive) {
            tone(PIN_BUZZER, 2800); 
            toneEndTime = millis() + 10; // Very short tick
            tonesActive = true;
        }
    }

    void playSelect() {
        tone(PIN_BUZZER, 3500); 
        toneEndTime = millis() + 50;
        tonesActive = true;
    }
    
    void playBack() {
        tone(PIN_BUZZER, 1000); 
        toneEndTime = millis() + 30;
        tonesActive = true;
    }
    
    void playError() {
        tone(PIN_BUZZER, 150); 
        toneEndTime = millis() + 150;
        tonesActive = true;
    }

    void playStartup() {
        // Simple blocking startup sequence
        tone(PIN_BUZZER, 2000, 80); delay(100);
        tone(PIN_BUZZER, 3000, 80); delay(100);
        tone(PIN_BUZZER, 4000, 100); delay(150);
        noTone(PIN_BUZZER);
    }
};

static SoundManager sound; // Global instance

#endif
