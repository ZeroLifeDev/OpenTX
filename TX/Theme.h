#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// ==========================================
//          TITANIUM PRO UI (V4)
// ==========================================
// Commercial Style: High Contrast, Functional, No Neon.
// Inspiration: Sanwa M17 / Futaba 7PX

// Safe Color Definitions (Standard RGB565)
// If colors are still swapped, DisplayManager::setSwapBytes(true) fixes it.

// Backgrounds
#define COLOR_BG_MAIN     0x2124 // Dark Titanium Grey (Approx 33,33,33)
#define COLOR_BG_PANEL    0x0000 // Black for data panels
#define COLOR_BG_HEADER   0xF800 // Pro Orange (Sanwa-ish) or 0x01E8 (Deep Blue)
// Let's go with a High-Vis Orange for Header to look "Race"
// Actually user said "Pick different palette". Let's go Clean Blue/White/Grey.
#undef COLOR_BG_HEADER
#define COLOR_BG_HEADER   0x18C3 // Slate Blue

// Text
#define COLOR_TEXT_MAIN   0xFFFF // White
#define COLOR_TEXT_DIM    0xBDF7 // Light Grey
#define COLOR_TEXT_GLOW   0xFFFF // No Glow, just White
#define COLOR_TEXT_HI     0xFAAA // Gold/Orange highlight

// Accents
#define COLOR_ACCENT_PRI  0xFAAA // Gold/Orange (Active)
#define COLOR_ACCENT_SEC  0xF800 // Red (Critical)
#define COLOR_ACCENT_TER  0x07E0 // Green (Safe)

// Status
#define COLOR_OK          0x07E0 // Green
#define COLOR_WARN        0xFFE0 // Yellow
#define COLOR_ERR         0xF800 // Red

// Layout
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// Fonts
#define FONT_SMALL  1
#define FONT_MED    2
#define FONT_LARGE  4
#define FONT_NUMS   6

#endif // THEME_H
