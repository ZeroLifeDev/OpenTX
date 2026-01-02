#include <Arduino.h>

// Modular Architecture Includes
#include "HardwareConfig.h"
#include "Theme.h"
#include "InputManager.h"
#include "DisplayManager.h"
#include "UIManager.h"
#include "SoundManager.h"

// ==========================================
//              SETUP
// ==========================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("--- OpenTX OS STARTING ---");

    // 1. Initialize Hardware Inputs
    Serial.println("Initializing Inputs...");
    inputManager.init();

    // 2. Initialize Sound
    soundManager.init();
    soundManager.beepStartup();

    // 3. Initialize Display
    Serial.println("Initializing Display...");
    displayManager.init();

    // 4. Initialize UI
    Serial.println("Initializing UI...");
    uiManager.init();

    Serial.println("System Ready.");
}

// ==========================================
//              LOOP
// ==========================================
unsigned long lastLog = 0;

void loop() {
    // 1. Update Inputs
    inputManager.update();

    // 2. Update UI Logic
    uiManager.update();

    // 3. Draw UI
    uiManager.draw();
    
    // 4. Debug Logging (Every 500ms)
    if (millis() - lastLog > 500) {
        lastLog = millis();
        Serial.print("Steer: "); Serial.print(inputManager.currentState.steering);
        Serial.print(" | Thr: "); Serial.print(inputManager.currentState.throttle);
        Serial.print(" | Susp: "); Serial.print(inputManager.currentState.potSuspension);
        Serial.print(" | Trim: "); Serial.print(inputManager.currentState.trimLevel);
        Serial.print(" | Gyro: "); Serial.print(inputManager.currentState.swGyro);
        Serial.println();
    }
}
