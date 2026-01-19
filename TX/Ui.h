#pragma once

#include <Arduino.h>

#include "InputManager.h"
#include "Renderer.h"
#include "UiState.h"

enum ScreenId : uint8_t {
  SCREEN_BOOT = 0,
  SCREEN_DASHBOARD,
  SCREEN_TELEMETRY,
  SCREEN_ACCESSORY_CTRL,
  SCREEN_ACCESSORY_MAP,
  SCREEN_GYRO,
  SCREEN_STEERING,
  SCREEN_THROTTLE,
  SCREEN_SUSPENSION,
  SCREEN_INPUT_MON,
  SCREEN_TRIM,
  SCREEN_PROFILE_SELECT,
  SCREEN_PROFILE_EDIT,
  SCREEN_BATTERY,
  SCREEN_FAILSAFE,
  SCREEN_ALERTS,
  SCREEN_SYSTEM,
  SCREEN_PERF,
  SCREEN_LOGGING,
  SCREEN_CALIBRATION,
  SCREEN_WIRELESS,
  SCREEN_DIAGNOSTICS,
  SCREEN_DEVELOPER,
  SCREEN_SAFE_SHUTDOWN,
  SCREEN_COUNT
};

struct UiContext {
  ScreenId current = SCREEN_BOOT;
  uint8_t focus[SCREEN_COUNT] = {0};
  uint8_t scroll[SCREEN_COUNT] = {0};
  bool editMode[SCREEN_COUNT] = {false};
  bool contextActive = false;
  uint8_t contextIndex = 0;
  uint32_t bootStartMs = 0;
  uint32_t confirmUntilMs = 0;
};

class UiManager {
public:
  void begin();
  void handleInput(const InputActions &actions, UiState &state);
  void draw(Renderer &renderer, UiState &state);
  ScreenId currentScreen() const { return ctx.current; }

private:
  UiContext ctx;

  void gotoHome();
  void gotoNextScreen();
  void goBack();
  void handleContext(const InputActions &actions, UiState &state);
  void handleListInput(ScreenId id, int itemCount, const InputActions &actions, UiState &state, bool directAdjust);
  void adjustScroll(ScreenId id, int itemCount, int visibleCount);
  void adjustValue(ScreenId id, int index, int delta, UiState &state);
};
