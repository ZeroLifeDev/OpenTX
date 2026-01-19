#pragma once

#include <Arduino.h>

enum ColorToken : uint8_t {
  BG_PRIMARY = 0,
  BG_PANEL,
  BG_PANEL_ALT,
  BG_STATUS,
  BG_STRIP,
  TEXT_PRIMARY,
  TEXT_MUTED,
  TEXT_INVERT,
  ACCENT_CYAN,
  ACCENT_AMBER,
  ACCENT_RED,
  STATE_OK,
  STATE_WARN,
  STATE_CRIT,
  FOCUS_BG,
  FOCUS_BORDER,
  BAR_POS,
  BAR_NEG,
  BAR_NEUTRAL,
  GRID_LINE,
  COLOR_TOKEN_COUNT
};

class Palette {
public:
  void begin();
  uint16_t color(ColorToken token) const;

private:
  uint16_t colors[COLOR_TOKEN_COUNT];
};
