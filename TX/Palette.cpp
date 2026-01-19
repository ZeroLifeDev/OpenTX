#include "Palette.h"

namespace {
struct HsvSpec {
  uint16_t h;
  uint8_t s;
  uint8_t v;
};

enum Hue : uint16_t {
  HUE_SLATE = 210,
  HUE_CYAN = 190,
  HUE_AMBER = 35,
  HUE_RED = 8,
  HUE_LIME = 110
};

enum Sat : uint8_t {
  SAT_OFF = 0,
  SAT_LOW = 18,
  SAT_MED = 38,
  SAT_HIGH = 70
};

enum Val : uint8_t {
  VAL_BLACK = 6,
  VAL_DARK = 12,
  VAL_LOW = 22,
  VAL_MID = 40,
  VAL_HIGH = 70,
  VAL_FULL = 96
};

uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

uint16_t hsvTo565(const HsvSpec &spec) {
  uint8_t s = static_cast<uint8_t>((static_cast<uint16_t>(spec.s) * 255) / 100);
  uint8_t v = static_cast<uint8_t>((static_cast<uint16_t>(spec.v) * 255) / 100);

  uint8_t region = spec.h / 43;
  uint8_t remainder = (spec.h - (region * 43)) * 6;
  uint8_t p = (v * (255 - s)) / 255;
  uint8_t q = (v * (255 - ((s * remainder) / 255))) / 255;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) / 255))) / 255;

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }

  return rgbTo565(r, g, b);
}

uint16_t gray565(uint8_t v) {
  return rgbTo565(v, v, v);
}
}  // namespace

void Palette::begin() {
  colors[BG_PRIMARY] = hsvTo565({HUE_SLATE, SAT_LOW, VAL_BLACK});
  colors[BG_PANEL] = hsvTo565({HUE_SLATE, SAT_LOW, VAL_DARK});
  colors[BG_PANEL_ALT] = hsvTo565({HUE_SLATE, SAT_LOW, VAL_LOW});
  colors[BG_STATUS] = hsvTo565({HUE_SLATE, SAT_MED, VAL_LOW});
  colors[BG_STRIP] = hsvTo565({HUE_SLATE, SAT_MED, VAL_LOW});

  colors[TEXT_PRIMARY] = gray565(88);
  colors[TEXT_MUTED] = gray565(62);
  colors[TEXT_INVERT] = gray565(8);

  colors[ACCENT_CYAN] = hsvTo565({HUE_CYAN, SAT_HIGH, VAL_HIGH});
  colors[ACCENT_AMBER] = hsvTo565({HUE_AMBER, SAT_HIGH, VAL_HIGH});
  colors[ACCENT_RED] = hsvTo565({HUE_RED, SAT_HIGH, VAL_HIGH});

  colors[STATE_OK] = hsvTo565({HUE_LIME, SAT_HIGH, VAL_HIGH});
  colors[STATE_WARN] = colors[ACCENT_AMBER];
  colors[STATE_CRIT] = colors[ACCENT_RED];

  colors[FOCUS_BG] = hsvTo565({HUE_SLATE, SAT_MED, VAL_MID});
  colors[FOCUS_BORDER] = colors[ACCENT_CYAN];

  colors[BAR_POS] = colors[ACCENT_CYAN];
  colors[BAR_NEG] = colors[ACCENT_RED];
  colors[BAR_NEUTRAL] = colors[TEXT_MUTED];

  colors[GRID_LINE] = gray565(24);
}

uint16_t Palette::color(ColorToken token) const {
  if (token >= COLOR_TOKEN_COUNT) {
    return colors[BG_PRIMARY];
  }
  return colors[token];
}
