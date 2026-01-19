#pragma once

#include <Arduino.h>
#include "HardwareConfig.h"
#include "UiState.h"

class Buzzer {
public:
  void begin();
  void update(const UiState &state);

private:
  void setOutput(bool on);

  uint32_t lastToggleMs = 0;
  bool buzzerOn = false;
  bool lastAlertActive = false;
  uint8_t lastAlertType = 0;
  uint8_t pulsesRemaining = 0;
  uint16_t onMs = 0;
  uint16_t offMs = 0;
};
