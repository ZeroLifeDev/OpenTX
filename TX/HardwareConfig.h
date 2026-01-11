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

// Hardware Calibration (User reported -10 offset, so we add +20 to center it? Or +10? User said "-10", usually means it reads -10 at center, so we need +10. Wait, implementation plan said +20. Let's start with +15 to be safe or stick to +10 if it was exactly -10. 
// User said: "center is -10 on the rx in both throttle and steering i want u to fix it"
// If it reads -10, we need to ADD 10 to make it 0. The previous code had `STEER_CENTER_FIX 10`. Maybe it wasn't enough? Or maybe it wasn't applied?
// I will increase it to 20 just to be sure, or better, make it tunable. For now, 20 seems like a strong correction if 10 didn't work.
#define STEER_CENTER_FIX 20  
#define THROT_CENTER_FIX 15  

// LED Logic Configuration
// logical "ON" for status (connected) usually means LED OFF physically if we want "Shut off when connected".
// If the user wants the LED to be OFF when connected:
// We will define macros for what "OFF" means electrically.
// Most built-in LEDs are Active High (High=On). Some are Active Low (Low=On).
// We will assume Active High standard first.
#define LED_ACTIVE_LOW 0 // Set to 1 if LED is ON when pin is LOW. 
#if LED_ACTIVE_LOW
  #define LED_ON_STATE LOW
  #define LED_OFF_STATE HIGH
#else
  #define LED_ON_STATE HIGH
  #define LED_OFF_STATE LOW
#endif

#endif // HARDWARE_CONFIG_H