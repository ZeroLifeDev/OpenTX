#ifndef THEME_H
#define THEME_H

#include <TFT_eSPI.h>

// ==========================================
//              COLOR PALETTE
// ==========================================
// "OpenTX" Slate/Navy Aesthetic
#define COLOR_BG_DARK     0x1086 // Dark Slate Blue
#define COLOR_BG_PANEL    0x2989 // Lighter Slate Blue for panels
#define COLOR_ACCENT      0x07E0 // Bright Green for healthy status
#define COLOR_ACCENT_ALT  0xF800 // Red for warnings
#define COLOR_TEXT_MAIN   0xFFFF // White
#define COLOR_TEXT_SUB    0xBDF7 // Light Blue-Grey
#define COLOR_HIGHLIGHT   0xFFE0 // Yellow for selection

// ==========================================
//              FONTS
// ==========================================
// Using built-in TFT_eSPI fonts for now. 
// Can be replaced with custom VLW fonts later.
#define FONT_HEADER  4 // Large serif
#define FONT_BODY    2 // Small sans-serif
#define FONT_SMALL   1 // Tiny

// ==========================================
//              UI CONSTANTS
// ==========================================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160
#define UI_MARGIN     5
#define UI_RADIUS     4

#endif // THEME_H
