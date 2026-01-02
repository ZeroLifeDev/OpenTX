#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// ==========================================
//              HYPER-GLASS THEME
// ==========================================
// "Complexly Overengineered" Palette
// Light, Glossy, High-Tech, NOT DARK.

// Colors (R5 G6 B5 - 16 bit)
// https://rgbcolorpicker.com/565

#define COLOR_BG_MAIN     0xE79F // Light Cool Grey (Base)
#define COLOR_BG_PANEL    0xFFFF // Pure White (Panels)
#define COLOR_BG_SHADOW   0xBDF7 // Shadow Grey

#define COLOR_TEXT_MAIN   0x0000 // Black (Sharp)
#define COLOR_TEXT_SUB    0x52AA // Dark Grey
#define COLOR_TEXT_MUTED  0x9CD3 // Light Grey (Details)

// Intense Accents
#define COLOR_ACCENT_1    0xF800 // Deep Red (Power/Critical)
#define COLOR_ACCENT_2    0x04BF // Electric Cyan (Data/Link)
#define COLOR_ACCENT_3    0xFD20 // Solar Orange (Active/Warning)
#define COLOR_ACCENT_4    0x901F // Tech Purple (Decor)

// Status
#define COLOR_SUCCESS     0x05E0 // Emerald Green
#define COLOR_WARNING     0xFDE0 // Bright Yellow
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
#define COLOR_ACCENT     COLOR_ACCENT_1
#define COLOR_HIGHLIGHT  COLOR_ACCENT_2

#define FONT_SMALL       FONT_LABEL
#define FONT_BODY        FONT_LABEL

#endif // THEME_H
