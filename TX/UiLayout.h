#pragma once

#include <Arduino.h>

namespace UiLayout {
constexpr int16_t ScreenW = 128;
constexpr int16_t ScreenH = 160;
constexpr int16_t StatusH = 14;
constexpr int16_t TempH = 12;
constexpr int16_t ContentY = StatusH + TempH;
constexpr int16_t ContentH = ScreenH - ContentY;
constexpr int16_t HeaderH = 14;
constexpr int16_t ListY = ContentY + HeaderH;
constexpr int16_t ListH = ContentH - HeaderH;
constexpr int16_t ItemH = 18;
}  // namespace UiLayout
