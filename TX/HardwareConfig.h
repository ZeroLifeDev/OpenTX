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
#define PIN_BUZZER 18 

// TFT Display (ILI9163)
#define PIN_TFT_MOSI 13
#define PIN_TFT_SCLK 14
#define PIN_TFT_CS    5
#define PIN_TFT_DC   25
#define PIN_TFT_RST   4

// Status LED
#define PIN_LED_BUILTIN 2

// --- System ---
#define SERIAL_BAUD 115200

// --- Configuration ---
#define TEST_MODE 0       // Set to 0 to use real inputs
#define JOY_DEADZONE 250  // Increased Deadzone
#define JOY_CENTER 1850   // Approx center

// Hardware Calibration (User reported -10 offset)
#define STEER_CENTER_FIX 10  // Add to correct the -10 reading
#define THROT_CENTER_FIX 10  // Add to correct the -10 reading

// (Configuration Removed for Adafruit ST7735 Compatibility)

#endif // HARDWARE_CONFIG_H
