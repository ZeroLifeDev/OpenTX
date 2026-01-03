#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// ==========================================
//             COLOR PALETTE
// ==========================================
// NOTE: To fix the "Red Bleeding/Ghosting" issue, the main background
// MUST be Pure Black (0x0000). Dark Blue (0x100B) causes artifacts.

// --- PRIMARY BACKGROUNDS ---
#define COL_BG_TOP      0x0000  // Pure Black (CRITICAL FOR VISUAL FIX)
#define COL_CARD_STD    0x2104  // Very Dark Grey for UI panels
#define COL_CARD_LIGHT  0x4208  // Lighter Grey for headers

// --- ACCENT COLORS ---
#define COL_ACCENT_PRI  0x07FF  // Cyan (Normal Operation)
#define COL_ACCENT_SEC  0xF800  // Red (Braking/Warning)
#define COL_ACCENT_TER  0x07E0  // Green (Success/Gyro)

// --- TEXT COLORS ---
#define COL_TEXT_PRI    0xFFFF  // White
#define COL_TEXT_SEC    0xBDF7  // Blue-Grey
#define COL_TEXT_DIS    0x5AEB  // Muted Grey

// ==========================================
//             LAYOUT CONSTANTS
// ==========================================
#define MC_DATUM 4
#define TC_DATUM 1

#endif