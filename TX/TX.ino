#include <Arduino.h>

// Modular Architecture Includes
#include "HardwareConfig.h"
#include "Theme.h"
#include "InputManager.h"
#include "DisplayManager.h"
#include "UIManager.h"
#include "SoundManager.h"
#include "CommsManager.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ==========================================
//              SETUP
// ==========================================
void setup() {
    // Disable Brownout Detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
    
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("--- OpenTX OS STARTING ---");

    // 1. Initialize Hardware Inputs
    Serial.println("Initializing Inputs...");
    inputManager.init();
    Serial.println("Inputs Init Done");

    // 2. Initialize Sound
    soundManager.init();
    // soundManager.beepStartup(); // Disabled to prevent power spike/delay during fragile boot
    Serial.println("Sound Init Done (Startup Beep Disabled for stability)");

    // 3. Initialize Comms (ESP-NOW)
    commsManager.init();
    Serial.println("Comms Init Done");

    // 4. Initialize Display
    Serial.println("Initializing Display...");
    displayManager.init();
    Serial.println("Display Init Done");

    // 5. Initialize UI
    Serial.println("Initializing UI...");
    uiManager.init();
    Serial.println("UI Init Done");

    Serial.println("System Ready.");
    
    // Play startup sound NOW, after everything is stable
    soundManager.beepStartup();
}

// ==========================================
//              LOOP
// ==========================================
unsigned long lastLog = 0;
unsigned long lastTx = 0;
const int TX_INTERVAL = 20; // 50Hz Data Rate

void loop() {
    // 1. Update Inputs (Fast as possible for debounce)
    inputManager.update();

    // 2. Transmit Data (Rate Limited)
    if (millis() - lastTx >= TX_INTERVAL) {
        lastTx = millis();
        commsManager.sendData(
            inputManager.getSteeringNormalized(),
            inputManager.getThrottleNormalized(),
            inputManager.currentState.potSuspension,
            inputManager.currentState.trimLevel,
            inputManager.currentState.swGyro
        );
        commsManager.update();
    }

    // 3. Update UI Logic
    uiManager.update();

    // 4. Draw UI
    uiManager.draw();
    
    // 5. Debug Logging (Every 500ms)
    // ...
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
