#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// ==========================================
//           VISUAL DESIGN SYSTEM
// ==========================================

// Helper to convert Hex to RGB565
#define COLOR_RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))


#define COL_BG_TOP       COLOR_RGB565(0x00, 0x31, 0x63) // #003163

// Card Surface (Lighter than BG)
#define COL_CARD_STD     COLOR_RGB565(0x34, 0x49, 0x5E) // #34495E

// Accents (High Contrast)
#define COL_ACCENT_PRI   COLOR_RGB565(0x00, 0xE5, 0xC8) // #00E5C8 (Cyan)
#define COL_ACCENT_SEC   COLOR_RGB565(0xFF, 0xA0, 0x40) // Bright Orange
#define COL_ACCENT_TER   COLOR_RGB565(0x00, 0xD0, 0xB0) // Teal

// Text
#define COL_TEXT_PRI     COLOR_RGB565(0xEC, 0xF0, 0xF1) // White/Cloud
#define COL_TEXT_SEC     COLOR_RGB565(0xBD, 0xC3, 0xC7) // Silver
#define COL_TEXT_DIS     COLOR_RGB565(0x7F, 0x8C, 0x8D) // Concrete

// Status
#define COL_STATUS_OK    COLOR_RGB565(0x2E, 0xCC, 0x71) // Emerald
#define COL_STATUS_DANGER COLOR_RGB565(0xE7, 0x4C, 0x3C) // Alizarin

// Dimensions (Portrait Target)
// Assuming ILI9163 native usage
#define SCREEN_WIDTH  128 
#define SCREEN_HEIGHT 160
#define RADIUS_CORNER 6

#endif // THEME_H
