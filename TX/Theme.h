#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// ==========================================
//          PREMIUM BLUE THEME
// ==========================================

// Colors (R5 G6 B5 - 16 bit)
// Fix for Red/Blue Swap: 
// 0x001F is Blue. 0xF800 is Red.
// If your screen is BGR, these might flip.
// We will use standard TFT_BLUE if defined, or hardcoded safe values.

#define COLOR_BG_MAIN     0x0010 // Dark Navy Blue
#define COLOR_BG_PANEL    0x0018 // Slightly lighter Navy
#define COLOR_BG_SHADOW   0x0008 

#define COLOR_TEXT_MAIN   0xFFFF // White
#define COLOR_TEXT_SUB    0x9EFE // Cyan-White
#define COLOR_TEXT_MUTED  0x4A69 // Muted

// Accents
#define COLOR_ACCENT_1    0xF800 // Red (Alarm)
#define COLOR_ACCENT_2    0x07FF // Cyan (Primary)
#define COLOR_ACCENT_3    0xFFE0 // Yellow
#define COLOR_ACCENT_4    0x04FF // Electric Blue

// Status
#define COLOR_SUCCESS     0x07E0 
#define COLOR_WARNING     0xFFE0 
#define COLOR_DANGER      0xF800 

// Fonts
#define FONT_MICRO  1
#define FONT_LABEL  2
#define FONT_HEADER 4
#define FONT_DIGIT  6
#define FONT_BIG    7

// ==========================================
//              UI CONSTANTS
// ==========================================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// GRID SYSTEM
#define GRID_COLS 12
#define GRID_ROWS 16
#define CELL_W (SCREEN_WIDTH / GRID_COLS)
#define CELL_H (SCREEN_HEIGHT / GRID_ROWS)

// ==========================================
//          BACKWARD COMPATIBILITY
// ==========================================
#define COLOR_BG_DARK    COLOR_BG_MAIN
#define COLOR_ACCENT     COLOR_ACCENT_2
#define COLOR_HIGHLIGHT  COLOR_ACCENT_4

#define FONT_SMALL       FONT_LABEL
#define FONT_BODY        FONT_LABEL

#endif // THEME_H
