#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// ==========================================
//          CYBER-NOIR THEME (DARK)
// ==========================================
// "Complexly Overengineered" Dark Palette
// Deep Black, Neon Accents, High Tech

// Colors (R5 G6 B5 - 16 bit)

#define COLOR_BG_MAIN     0x0000 // Deepest Black
#define COLOR_BG_PANEL    0x10A2 // Dark Charcoal / Gunmetal
#define COLOR_BG_SHADOW   0x0841 // Very Dark Grey

#define COLOR_TEXT_MAIN   0xFFFF // White
#define COLOR_TEXT_SUB    0x9CC7 // Cyan-Grey
#define COLOR_TEXT_MUTED  0x632C // Dark Grey

// Neon Accents
#define COLOR_ACCENT_1    0xF800 // Cyber Red (Critical)
#define COLOR_ACCENT_2    0x07E0 // Matrix Green (Data)
#define COLOR_ACCENT_3    0x04FF // Neon Cyan (Active)
#define COLOR_ACCENT_4    0xF81F // Hot Pink (Decor)

// Status
#define COLOR_SUCCESS     0x07E0 // Green
#define COLOR_WARNING     0xFFE0 // Yellow
#define COLOR_DANGER      0xF800 // Red

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
#define COLOR_ACCENT     COLOR_ACCENT_3
#define COLOR_HIGHLIGHT  COLOR_ACCENT_2

#define FONT_SMALL       FONT_LABEL
#define FONT_BODY        FONT_LABEL

#endif // THEME_H
