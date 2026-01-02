#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// ==========================================
//          OBSIDIAN GLASS UI (V3)
// ==========================================
// Palette: Deep/Abyss Blue, Platinum, Electric Cyan

// Safe Color Definitions (Standard RGB565)
// If colors are still swapped, DisplayManager::setSwapBytes(true) fixes it.

// Backgrounds
#define COLOR_BG_MAIN     0x0000 // Pure Black (Abyss)
#define COLOR_BG_PANEL    0x1084 // Deep Slate Blue
#define COLOR_BG_HEADER   0x0010 // Navy for Headers

// Text
#define COLOR_TEXT_MAIN   0xFFFF // White
#define COLOR_TEXT_DIM    0x9CDF // Light Blue Grey
#define COLOR_TEXT_GLOW   0x07FF // Cyan Glow

// Accents
#define COLOR_ACCENT_PRI  0x07FF // Electric Cyan (Primary)
#define COLOR_ACCENT_SEC  0xF800 // Deep Red (Critical/Sport)
#define COLOR_ACCENT_TER  0xFFE0 // Yellow (Warning)

// Status
#define COLOR_OK          0x07E0 // Green
#define COLOR_WARN        0xFFE0 
#define COLOR_ERR         0xF800 

// Layout
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// Fonts
#define FONT_SMALL  1
#define FONT_MED    2
#define FONT_LARGE  4
#define FONT_NUMS   6

#endif // THEME_H
