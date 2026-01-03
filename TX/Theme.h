#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// ==========================================
//          OBSIDIAN GLASS THEME (V7)
// ==========================================
// Aesthetic: Clean, Premium, Automotive, Digital
// Goal: High Contrast, Smooth Gradients, No "Gamer" clutter.

// Swap Bytes Helper
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// Backgrounds - "Apple Watch" Dark
#define COLOR_BG_MAIN     0x0000               // Pure Black (Standard for OLED/LCD to hide bleed if capable, else deep grey)
#define COLOR_BG_PANEL    RGB565(20, 20, 24)   // Soft Slate Grey (Card Backgrounds)
#define COLOR_BG_HEADER   RGB565(10, 10, 10)   // Almost Black
#define COLOR_BG_DIM      RGB565(5, 5, 5)      // Deep Gradient End

// Accents - Focused & Professional
#define COLOR_ACCENT_PRI  RGB565(0, 190, 255)  // Electric Blue (Primary Data)
#define COLOR_ACCENT_SEC  RGB565(255, 60, 60)  // Sport Red (Alert/Limit)
#define COLOR_ACCENT_TER  RGB565(50, 220, 100) // Success Green
#define COLOR_ACCENT_GOLD RGB565(255, 180, 0)  // Warning/Highlight

// Status Aliases
#define COLOR_OK          COLOR_ACCENT_TER
#define COLOR_WARN        COLOR_ACCENT_GOLD
#define COLOR_ERR         COLOR_ACCENT_SEC

// UI Elements - Clean, Thin, Elegant
#define COLOR_BORDER      RGB565(60, 70, 80)   // Subtle bezel
#define COLOR_WIDGET_BG   RGB565(18, 18, 20)   // Widget fills

// Text - MAXIMUM CONTRAST
#define COLOR_TEXT_MAIN   0xFFFF               // Pure White
#define COLOR_TEXT_DIM    RGB565(140, 145, 150)// Silver
#define COLOR_TEXT_HI     0xFFFF               // White (Keep highlights simple)

// Custom Fonts (SPIFFS/LittleFS Paths)
#define FONT_PATH_LIGHT   "Montserrat_Light_10"
#define FONT_PATH_REG     "Montserrat_Regular_12"
#define FONT_PATH_BOLD    "Montserrat_Bold_18"
#define FONT_PATH_DIGIT   "DSEG7_Classic_Bold_48"

// Font Aliases (for code compatibility, mapped to 1 for now, logic in DisplayManager handled later)
// Note: We will load these as GFXFonts or VLW. 
// For now, we will use the string paths in loadFont calls.
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// Legacy Font Aliases (Fallback if custom not loaded)
#define FONT_SMALL  1
#define FONT_MED    2
#define FONT_LARGE  4
#define FONT_NUMS   6

#endif // THEME_H
