#pragma once

#include <Arduino.h>
#include <driver/spi_master.h>

#include "HardwareConfig.h"

namespace PanelIO {
static const int16_t kWidth = 128;
static const int16_t kHeight = 160;

bool begin();
void pushRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *buffer, int16_t stride);
}  // namespace PanelIO
