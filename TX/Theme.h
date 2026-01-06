#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// ==========================================
//             COLOR PALETTE
// ==========================================
// NOTE: To fix the "Red Bleeding/Ghosting" issue, the main background
// MUST be Pure Black (0x0000). Dark Blue (0x100B) causes artifacts.

// --- PRIMARY BACKGROUNDS ---
#define COL_BG_TOP      0x0000  // Pure Black
#define COL_CARD_STD    0x2104  // Very Dark Grey
#define COL_CARD_LIGHT  0x4208  // Lighter Grey

// --- ACCENT COLORS ---
#define COL_ACCENT_PRI  0x07FF  // Cyan
#define COL_ACCENT_SEC  0xF800  // Red
#define COL_ACCENT_TER  0x07E0  // Green

// --- TEXT COLORS ---
#define COL_TEXT_PRI    0xFFFF  // White
#define COL_TEXT_SEC    0xBDF7  // Blue-Grey
#define COL_TEXT_DIS    0x5AEB  // Muted Grey

#define MC_DATUM 4
#define TC_DATUM 1
 
// --- GLOBAL THEME MAPPING ---
#define COLOR_BG_MAIN    COL_BG_TOP
#define COLOR_BG_PANEL   COL_CARD_STD
#define COLOR_ACCENT_PRI COL_ACCENT_PRI
#define COLOR_ACCENT_SEC COL_ACCENT_SEC
#define COLOR_ACCENT_TER COL_ACCENT_TER
#define COLOR_TEXT_MAIN  COL_TEXT_PRI
#define COLOR_TEXT_DIM   COL_TEXT_SEC
#define COLOR_TEXT_DIS   COL_TEXT_DIS 

#endif