#include "InputManager.h"

void InputManager::begin() {
  initButton(menuBtn, PIN_BTN_MENU);
  initButton(setBtn, PIN_BTN_SET);
  initButton(trimPlus, PIN_BTN_TRIM_PLUS);
  initButton(trimMinus, PIN_BTN_TRIM_MINUS);

  pinMode(PIN_SW_GYRO, INPUT_PULLUP);
}

void InputManager::initButton(ButtonState &btn, uint8_t pin) {
  btn.pin = pin;
  btn.activeLow = true;
  btn.stableState = false;
  btn.lastStable = false;
  btn.lastChangeMs = 0;
  btn.pressedAtMs = 0;
  btn.lastReleaseMs = 0;
  btn.clickCount = 0;
  btn.longFired = false;
  pinMode(pin, INPUT_PULLUP);
}

bool InputManager::readButton(const ButtonState &btn) const {
  bool raw = digitalRead(btn.pin) == LOW;
  return btn.activeLow ? raw : !raw;
}

void InputManager::updateButton(ButtonState &btn) {
  bool raw = readButton(btn);
  uint32_t now = millis();
  if (raw != btn.stableState && (now - btn.lastChangeMs) >= kDebounceMs) {
    btn.lastStable = btn.stableState;
    btn.stableState = raw;
    btn.lastChangeMs = now;

    if (btn.stableState) {
      btn.pressedAtMs = now;
      btn.longFired = false;
    } else {
      btn.lastReleaseMs = now;
      if (!btn.longFired) {
        btn.clickCount++;
      }
    }
  }
}

void InputManager::handleClicks(ButtonState &btn, bool &shortPress, bool &longPress, bool &doublePress) {
  uint32_t now = millis();
  if (btn.stableState && !btn.longFired && (now - btn.pressedAtMs) >= kLongPressMs) {
    btn.longFired = true;
    btn.clickCount = 0;
    longPress = true;
  }

  if (!btn.stableState && btn.clickCount > 0 && (now - btn.lastReleaseMs) > kDoublePressMs) {
    if (btn.clickCount == 1) {
      shortPress = true;
    } else if (btn.clickCount >= 2) {
      doublePress = true;
    }
    btn.clickCount = 0;
  }
}

int8_t InputManager::handleTrimRepeat(ButtonState &btn, RepeatState &rep, int8_t direction) {
  uint32_t now = millis();
  if (btn.stableState && !rep.pressed) {
    rep.pressed = true;
    rep.pressedAtMs = now;
    rep.lastRepeatMs = now;
    return direction;
  }

  if (!btn.stableState && rep.pressed) {
    rep.pressed = false;
    return 0;
  }

  if (rep.pressed) {
    uint16_t interval = (now - rep.pressedAtMs) > kAccelMs ? kRepeatFastMs : kRepeatDelayMs;
    if ((now - rep.lastRepeatMs) >= interval) {
      rep.lastRepeatMs = now;
      return direction;
    }
  }

  return 0;
}

InputActions InputManager::update() {
  InputActions actions;

  updateButton(menuBtn);
  updateButton(setBtn);
  updateButton(trimPlus);
  updateButton(trimMinus);

  bool menuDouble = false;
  handleClicks(menuBtn, actions.menuShort, actions.menuLong, menuDouble);
  handleClicks(setBtn, actions.setShort, actions.setLong, actions.setDouble);

  int8_t trim = 0;
  trim += handleTrimRepeat(trimPlus, trimPlusRepeat, 1);
  trim += handleTrimRepeat(trimMinus, trimMinusRepeat, -1);
  actions.trimDelta = trim;

  return actions;
}
