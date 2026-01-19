#pragma once

#include <Arduino.h>
#include "HardwareConfig.h"

struct InputActions {
  bool menuShort = false;
  bool menuLong = false;
  bool setShort = false;
  bool setLong = false;
  bool setDouble = false;
  int8_t trimDelta = 0;
};

class InputManager {
public:
  void begin();
  InputActions update();

private:
  struct ButtonState {
    uint8_t pin = 0;
    bool activeLow = true;
    bool stableState = false;
    bool lastStable = false;
    uint32_t lastChangeMs = 0;
    uint32_t pressedAtMs = 0;
    uint32_t lastReleaseMs = 0;
    uint8_t clickCount = 0;
    bool longFired = false;
  };

  struct RepeatState {
    uint32_t pressedAtMs = 0;
    uint32_t lastRepeatMs = 0;
    bool pressed = false;
  };

  ButtonState menuBtn;
  ButtonState setBtn;
  ButtonState trimPlus;
  ButtonState trimMinus;

  RepeatState trimPlusRepeat;
  RepeatState trimMinusRepeat;

  void initButton(ButtonState &btn, uint8_t pin);
  bool readButton(const ButtonState &btn) const;
  void updateButton(ButtonState &btn);
  void handleClicks(ButtonState &btn, bool &shortPress, bool &longPress, bool &doublePress);
  int8_t handleTrimRepeat(ButtonState &btn, RepeatState &rep, int8_t direction);

  static const uint16_t kDebounceMs = 20;
  static const uint16_t kLongPressMs = 650;
  static const uint16_t kDoublePressMs = 350;
  static const uint16_t kRepeatDelayMs = 280;
  static const uint16_t kRepeatFastMs = 90;
  static const uint16_t kAccelMs = 800;
};
