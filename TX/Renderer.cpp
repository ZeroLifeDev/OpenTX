#include "Renderer.h"

#include "Font5x7.h"

Renderer::Renderer() : buffer(nullptr), width(0), height(0), palette(nullptr) {}

void Renderer::setBuffer(uint16_t *buf, int16_t w, int16_t h, const Palette *pal) {
  buffer = buf;
  width = w;
  height = h;
  palette = pal;
}

void Renderer::clear(ColorToken color) {
  if (!buffer || !palette) return;
  uint16_t c = palette->color(color);
  for (int32_t i = 0; i < width * height; ++i) {
    buffer[i] = c;
  }
}

void Renderer::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, ColorToken color) {
  if (!buffer || !palette) return;
  if (w <= 0 || h <= 0) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > width) w = width - x;
  if (y + h > height) h = height - y;
  if (w <= 0 || h <= 0) return;

  uint16_t c = palette->color(color);
  for (int16_t yy = y; yy < y + h; ++yy) {
    uint32_t idx = static_cast<uint32_t>(yy) * width + x;
    for (int16_t xx = 0; xx < w; ++xx) {
      buffer[idx + xx] = c;
    }
  }
}

void Renderer::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, ColorToken color) {
  drawHLine(x, y, w, color);
  drawHLine(x, y + h - 1, w, color);
  drawVLine(x, y, h, color);
  drawVLine(x + w - 1, y, h, color);
}

void Renderer::drawHLine(int16_t x, int16_t y, int16_t w, ColorToken color) {
  if (!buffer || !palette) return;
  if (y < 0 || y >= height || w <= 0) return;
  if (x < 0) { w += x; x = 0; }
  if (x + w > width) w = width - x;
  if (w <= 0) return;

  uint16_t c = palette->color(color);
  uint32_t idx = static_cast<uint32_t>(y) * width + x;
  for (int16_t i = 0; i < w; ++i) {
    buffer[idx + i] = c;
  }
}

void Renderer::drawVLine(int16_t x, int16_t y, int16_t h, ColorToken color) {
  if (!buffer || !palette) return;
  if (x < 0 || x >= width || h <= 0) return;
  if (y < 0) { h += y; y = 0; }
  if (y + h > height) h = height - y;
  if (h <= 0) return;

  uint16_t c = palette->color(color);
  uint32_t idx = static_cast<uint32_t>(y) * width + x;
  for (int16_t i = 0; i < h; ++i) {
    buffer[idx + i * width] = c;
  }
}

void Renderer::drawText(int16_t x, int16_t y, const char *text, uint8_t scale, ColorToken color) {
  if (!text || scale == 0) return;
  while (*text) {
    drawChar(x, y, *text, scale, color);
    x += (6 * scale);
    ++text;
  }
}

void Renderer::drawTextRight(int16_t right, int16_t y, const char *text, uint8_t scale, ColorToken color) {
  int16_t w = textWidth(text, scale);
  drawText(right - w, y, text, scale, color);
}

void Renderer::drawTextCentered(int16_t y, const char *text, uint8_t scale, ColorToken color) {
  int16_t w = textWidth(text, scale);
  int16_t x = (width - w) / 2;
  drawText(x, y, text, scale, color);
}

int16_t Renderer::textWidth(const char *text, uint8_t scale) const {
  if (!text || scale == 0) return 0;
  return static_cast<int16_t>(strlen(text) * 6 * scale);
}

int16_t Renderer::textHeight(uint8_t scale) const {
  return static_cast<int16_t>(8 * scale);
}

void Renderer::drawVBar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t pct, ColorToken fill, ColorToken track) {
  if (pct < -100) pct = -100;
  if (pct > 100) pct = 100;
  fillRect(x, y, w, h, track);
  int16_t mid = y + h / 2;
  drawHLine(x, mid, w, GRID_LINE);
  if (pct == 0) return;
  if (pct > 0) {
    int16_t fh = (h / 2) * pct / 100;
    fillRect(x + 1, mid - fh, w - 2, fh, fill);
  } else {
    int16_t fh = (h / 2) * (-pct) / 100;
    fillRect(x + 1, mid, w - 2, fh, BAR_NEG);
  }
}

void Renderer::drawHBar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t pct, ColorToken fill, ColorToken track) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  fillRect(x, y, w, h, track);
  int16_t fw = (w * pct) / 100;
  if (fw > 0) {
    fillRect(x, y, fw, h, fill);
  }
}

void Renderer::drawValueBox(int16_t x, int16_t y, int16_t w, int16_t h, const char *text, bool focused) {
  fillRect(x, y, w, h, focused ? FOCUS_BG : BG_PANEL);
  drawRect(x, y, w, h, focused ? FOCUS_BORDER : GRID_LINE);
  int16_t ty = y + (h - textHeight(1)) / 2;
  drawText(x + 4, ty, text, 1, focused ? TEXT_INVERT : TEXT_PRIMARY);
}

void Renderer::drawChar(int16_t x, int16_t y, char c, uint8_t scale, ColorToken color) {
  if (!buffer || !palette) return;
  if (c < 32 || c > 126) c = '?';
  uint16_t idx = (c - 32) * 5;
  for (uint8_t i = 0; i < 5; ++i) {
    uint8_t line = pgm_read_byte(kFont5x7 + idx + i);
    for (uint8_t j = 0; j < 7; ++j) {
      if (line & 0x01) {
        int16_t px = x + (i * scale);
        int16_t py = y + (j * scale);
        fillRect(px, py, scale, scale, color);
      }
      line >>= 1;
    }
  }
}
