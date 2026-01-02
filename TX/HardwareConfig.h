#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// ==========================================
//              PIN DEFINITIONS
// ==========================================

// --- Inputs ---
// --- Inputs ---
// Control Stick
#define PIN_STEERING 33
#define PIN_THROTTLE 35
#define PIN_JOY_BTN -1 

// Potentiometers
#define PIN_POT_SUSPENSION 32 // ADC1

// Buttons (Avoid TFT Pins: 4, 5, 13, 14, 25)
// and Strapping Pins (0, 2, 12, 15) where possible
#define PIN_BTN_MENU 22 // Moved from 13 (TFT)
#define PIN_BTN_SET  19 // Moved from 12 (Strap)
#define PIN_BTN_TRIM_PLUS 26 
#define PIN_BTN_TRIM_MINUS 21 // Moved from 14 (TFT)

// Switches
#define PIN_SW_GYRO 27 

// Outputs
#define PIN_BUZZER 18 // Moved from 25 (TFT)

// --- System ---
#define SERIAL_BAUD 115200

// --- Configuration ---
#define TEST_MODE 0       // Set to 0 to use real inputs
#define JOY_DEADZONE 200  // Deadzone for joystick center (ADC values)
#define JOY_CENTER 1850   // Approx center

#endif // HARDWARE_CONFIG_H
