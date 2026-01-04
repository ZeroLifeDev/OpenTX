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

    // --- HIGH-TECH UI SOUNDS ---
    
    void playClick() { // Cursor Move
        if (!tonesActive) { 
            tone(PIN_BUZZER, 4000); // 4kHz crisp tick
            toneEndTime = millis() + 5; // Extremely short (5ms)
            tonesActive = true;
        }
    }
    
    void playScroll() { // Wheel/Trim
        if (!tonesActive) {
            tone(PIN_BUZZER, 4500); 
            toneEndTime = millis() + 5; 
            tonesActive = true;
        }
    }

    void playSelect() { // Enter / Set
        // Two-tone rising "Chirp" simulated
        // Note: rapid tone calls might not work perfectly non-blocking, so we use a high 'ding'
        tone(PIN_BUZZER, 5500); 
        toneEndTime = millis() + 30; // 30ms "Ding"
        tonesActive = true;
    }
    
    void playBack() { // Cancel / Back
        // Lower pitch "descend" feel
        tone(PIN_BUZZER, 2500); 
        toneEndTime = millis() + 20;
        tonesActive = true;
    }
    
    void playError() {
        tone(PIN_BUZZER, 1000); 
        toneEndTime = millis() + 100;
        tonesActive = true;
    }

    void playStartup() {
        // Sci-Fi Power Up
        for(int i=2000; i<6000; i+=500) {
            tone(PIN_BUZZER, i);
            delay(30);
        }
        noTone(PIN_BUZZER);
    }
};

static SoundManager sound; // Global instance

#endif
