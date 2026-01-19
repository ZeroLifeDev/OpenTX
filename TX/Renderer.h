#pragma once

#include <Arduino.h>
#include "Palette.h"

class Renderer {
public:
  Renderer();

  void setBuffer(uint16_t *buffer, int16_t width, int16_t height, const Palette *palette);

  void clear(ColorToken color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, ColorToken color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, ColorToken color);
  void drawHLine(int16_t x, int16_t y, int16_t w, ColorToken color);
  void drawVLine(int16_t x, int16_t y, int16_t h, ColorToken color);
  void drawText(int16_t x, int16_t y, const char *text, uint8_t scale, ColorToken color);
  void drawTextRight(int16_t right, int16_t y, const char *text, uint8_t scale, ColorToken color);
  void drawTextCentered(int16_t y, const char *text, uint8_t scale, ColorToken color);
  int16_t textWidth(const char *text, uint8_t scale) const;
  int16_t textHeight(uint8_t scale) const;

  void drawVBar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t pct, ColorToken fill, ColorToken track);
  void drawHBar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t pct, ColorToken fill, ColorToken track);
  void drawValueBox(int16_t x, int16_t y, int16_t w, int16_t h, const char *text, bool focused);

private:
  void drawChar(int16_t x, int16_t y, char c, uint8_t scale, ColorToken color);

  uint16_t *buffer;
  int16_t width;
  int16_t height;
  const Palette *palette;
};
