#include "Ui.h"

#include <Arduino.h>

#include "ScreenAccessoryControl.h"
#include "ScreenAccessoryMapping.h"
#include "ScreenAlerts.h"
#include "ScreenBattery.h"
#include "ScreenBoot.h"
#include "ScreenCalibration.h"
#include "ScreenDashboard.h"
#include "ScreenDeveloper.h"
#include "ScreenDiagnostics.h"
#include "ScreenFailsafe.h"
#include "ScreenGyro.h"
#include "ScreenInputMonitor.h"
#include "ScreenLogging.h"
#include "ScreenPerformance.h"
#include "ScreenProfileEdit.h"
#include "ScreenProfileSelect.h"
#include "ScreenSafeShutdown.h"
#include "ScreenSteering.h"
#include "ScreenSuspension.h"
#include "ScreenSystem.h"
#include "ScreenTelemetry.h"
#include "ScreenThrottle.h"
#include "ScreenTrim.h"
#include "ScreenWireless.h"
#include "UiDraw.h"
#include "UiLayout.h"

namespace {
static const ScreenId kScreenOrder[] = {
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
  SCREEN_SAFE_SHUTDOWN
};

int clampInt(int value, int minVal, int maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

float clampFloat(float value, float minVal, float maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}
}  // namespace

void UiManager::begin() {
  ctx.current = SCREEN_BOOT;
  ctx.bootStartMs = millis();
}

void UiManager::handleInput(const InputActions &actions, UiState &state) {
  if (ctx.current == SCREEN_BOOT) {
    return;
  }

  if (ctx.contextActive) {
    handleContext(actions, state);
    return;
  }

  if (actions.menuLong) {
    gotoHome();
    return;
  }

  if (actions.setDouble) {
    goBack();
    return;
  }

  if (actions.menuShort) {
    if (ctx.editMode[ctx.current]) {
      goBack();
    } else {
      gotoNextScreen();
    }
    return;
  }

  if (actions.setLong) {
    ctx.contextActive = true;
    ctx.contextIndex = 0;
    return;
  }

  switch (ctx.current) {
    case SCREEN_ACCESSORY_CTRL:
      handleListInput(ctx.current, 5, actions, state, true);
      break;
    case SCREEN_TELEMETRY:
      handleListInput(ctx.current, 7, actions, state, false);
      break;
    case SCREEN_ACCESSORY_MAP:
      handleListInput(ctx.current, 5, actions, state, false);
      break;
    case SCREEN_GYRO:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_STEERING:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_THROTTLE:
      handleListInput(ctx.current, 5, actions, state, false);
      break;
    case SCREEN_SUSPENSION:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_INPUT_MON:
      handleListInput(ctx.current, 6, actions, state, false);
      break;
    case SCREEN_TRIM:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_PROFILE_SELECT:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_PROFILE_EDIT:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_BATTERY:
      handleListInput(ctx.current, 3, actions, state, false);
      break;
    case SCREEN_FAILSAFE:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_ALERTS:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_SYSTEM:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_PERF:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_LOGGING:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_CALIBRATION:
      ScreenCalibration_HandleInput(actions, state, ctx);
      break;
    case SCREEN_WIRELESS:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    case SCREEN_DIAGNOSTICS:
      handleListInput(ctx.current, 3, actions, state, false);
      break;
    case SCREEN_DEVELOPER:
      handleListInput(ctx.current, 3, actions, state, false);
      break;
    case SCREEN_SAFE_SHUTDOWN:
      handleListInput(ctx.current, 4, actions, state, false);
      break;
    default:
      if (actions.setShort) {
        ctx.editMode[ctx.current] = !ctx.editMode[ctx.current];
      }
      break;
  }
}

void UiManager::gotoHome() {
  ctx.current = SCREEN_DASHBOARD;
  ctx.contextActive = false;
}

void UiManager::gotoNextScreen() {
  for (uint8_t i = 0; i < sizeof(kScreenOrder) / sizeof(kScreenOrder[0]); ++i) {
    if (kScreenOrder[i] == ctx.current) {
      uint8_t next = (i + 1) % (sizeof(kScreenOrder) / sizeof(kScreenOrder[0]));
      ctx.current = kScreenOrder[next];
      return;
    }
  }
  ctx.current = SCREEN_DASHBOARD;
}

void UiManager::goBack() {
  ctx.contextActive = false;
  ctx.editMode[ctx.current] = false;
}

void UiManager::handleContext(const InputActions &actions, UiState &state) {
  if (actions.menuShort || actions.setDouble) {
    ctx.contextActive = false;
    return;
  }

  if (actions.trimDelta != 0) {
    int next = static_cast<int>(ctx.contextIndex) + actions.trimDelta;
    if (next < 0) next = 2;
    if (next >= 3) next = 0;
    ctx.contextIndex = static_cast<uint8_t>(next);
  }

  if (actions.setShort) {
    if (ctx.contextIndex == 0) {
      state.expertMode = !state.expertMode;
    } else if (ctx.contextIndex == 1) {
      ctx.focus[ctx.current] = 0;
      ctx.editMode[ctx.current] = false;
    }
    ctx.contextActive = false;
  }
}

void UiManager::handleListInput(ScreenId id, int itemCount, const InputActions &actions, UiState &state, bool directAdjust) {
  if (actions.setShort) {
    if (id == SCREEN_TRIM && ctx.focus[id] == 2) {
      state.steerTrim = 0;
      state.throttleTrim = 0;
    } else if (id == SCREEN_PROFILE_SELECT) {
      state.activeProfile = ctx.focus[id];
    } else if (id == SCREEN_SAFE_SHUTDOWN) {
      // Placeholder for safe actions.
    } else if (directAdjust) {
      ctx.confirmUntilMs = millis() + 400;
    } else if (!directAdjust) {
      ctx.editMode[id] = !ctx.editMode[id];
    }
  }

  if (actions.trimDelta != 0) {
    if (directAdjust || ctx.editMode[id]) {
      adjustValue(id, ctx.focus[id], actions.trimDelta, state);
    } else {
      int next = static_cast<int>(ctx.focus[id]) + actions.trimDelta;
      ctx.focus[id] = static_cast<uint8_t>(clampInt(next, 0, itemCount - 1));
    }
  }

  int visible = UiLayout::ListH / UiLayout::ItemH;
  adjustScroll(id, itemCount, visible);
}

void UiManager::adjustScroll(ScreenId id, int itemCount, int visibleCount) {
  if (itemCount <= visibleCount) {
    ctx.scroll[id] = 0;
    return;
  }
  uint8_t focus = ctx.focus[id];
  uint8_t scroll = ctx.scroll[id];
  if (focus < scroll) {
    scroll = focus;
  } else if (focus >= scroll + visibleCount) {
    scroll = focus - visibleCount + 1;
  }
  ctx.scroll[id] = scroll;
}

void UiManager::adjustValue(ScreenId id, int index, int delta, UiState &state) {
  switch (id) {
    case SCREEN_ACCESSORY_CTRL:
      if (index == 0) {
        state.headlights = static_cast<AccessoryTriState>((state.headlights + 3 + delta) % 3);
      } else if (index == 1) {
        state.taillights = static_cast<AccessoryOnOff>((state.taillights + 2 + delta) % 2);
      } else if (index == 2) {
        state.turnSignals = static_cast<TurnSignalState>((state.turnSignals + 4 + delta) % 4);
      } else if (index == 3) {
        state.brakeLights = static_cast<BrakeState>((state.brakeLights + 2 + delta) % 2);
      } else if (index == 4) {
        state.auxOutput = static_cast<uint8_t>((state.auxOutput + 4 + delta) % 4);
      }
      break;
    case SCREEN_ACCESSORY_MAP:
      if (index < 5) {
        state.accessoryMap[index] = static_cast<uint8_t>((state.accessoryMap[index] + 6 + delta) % 6);
      }
      break;
    case SCREEN_GYRO:
      if (index == 0) {
        state.gyroOverride = !state.gyroOverride;
      } else if (index == 1) {
        state.gyroGain = static_cast<uint8_t>(clampInt(state.gyroGain + delta * 2, 0, 100));
      } else if (index == 2) {
        state.gyroMode = static_cast<GyroMode>((state.gyroMode + 2 + delta) % 2);
      } else if (index == 3) {
        state.gyroResponse = static_cast<uint8_t>(clampInt(state.gyroResponse + delta * 2, 0, 100));
      }
      break;
    case SCREEN_STEERING:
      if (index == 0) {
        state.steerDeadzone = static_cast<uint8_t>(clampInt(state.steerDeadzone + delta, 0, 20));
      } else if (index == 1) {
        state.steerCenter = static_cast<int8_t>(clampInt(state.steerCenter + delta, -20, 20));
      } else if (index == 2) {
        state.steerExpo = static_cast<uint8_t>(clampInt(state.steerExpo + delta, 0, 50));
      } else if (index == 3) {
        state.steerEndpoint = static_cast<uint8_t>(clampInt(state.steerEndpoint + delta, 80, 120));
      }
      break;
    case SCREEN_THROTTLE:
      if (index == 0) {
        state.throttleDeadzone = static_cast<uint8_t>(clampInt(state.throttleDeadzone + delta, 0, 20));
      } else if (index == 1) {
        state.throttleCenter = static_cast<int8_t>(clampInt(state.throttleCenter + delta, -20, 20));
      } else if (index == 2) {
        state.throttleCurve = static_cast<uint8_t>(clampInt(state.throttleCurve + delta, 0, 50));
      } else if (index == 3) {
        state.brakeStrength = static_cast<uint8_t>(clampInt(state.brakeStrength + delta * 2, 0, 100));
      } else if (index == 4) {
        state.reverseLogic = !state.reverseLogic;
      }
      break;
    case SCREEN_SUSPENSION:
      if (index == 0) {
        state.suspensionMode = static_cast<SuspensionMode>((state.suspensionMode + 2 + delta) % 2);
      } else if (index == 1) {
        state.suspensionPotMap = static_cast<uint8_t>(clampInt(state.suspensionPotMap + delta * 2, 0, 100));
      } else if (index == 2) {
        state.suspensionSpeedLogic = static_cast<uint8_t>(clampInt(state.suspensionSpeedLogic + delta * 2, 0, 100));
      } else if (index == 3) {
        state.suspensionPreset = static_cast<SuspensionPreset>((state.suspensionPreset + 3 + delta) % 3);
      }
      break;
    case SCREEN_TRIM:
      if (index == 0) {
        state.steerTrim = static_cast<int16_t>(clampInt(state.steerTrim + delta, -20, 20));
      } else if (index == 1) {
        state.throttleTrim = static_cast<int16_t>(clampInt(state.throttleTrim + delta, -20, 20));
      } else if (index == 3) {
        state.trimPerProfile = !state.trimPerProfile;
      }
      break;
    case SCREEN_BATTERY:
      if (index == 0) {
        state.txVoltageWarn = clampFloat(state.txVoltageWarn + (delta * 0.1f), 6.0f, 8.4f);
      } else if (index == 1) {
        state.rxVoltageWarn = clampFloat(state.rxVoltageWarn + (delta * 0.1f), 5.8f, 7.6f);
      } else if (index == 2) {
        state.voltageSagDetect = !state.voltageSagDetect;
      }
      break;
    case SCREEN_FAILSAFE:
      if (index == 0) {
        state.failsafeMode = static_cast<FailsafeMode>((state.failsafeMode + 3 + delta) % 3);
      } else if (index == 1) {
        state.throttleCut = !state.throttleCut;
      } else if (index == 2) {
        state.steeringCenter = !state.steeringCenter;
      } else if (index == 3) {
        state.alertSignal = !state.alertSignal;
      }
      break;
    case SCREEN_ALERTS:
      if (index == 0) {
        state.alertTemp = !state.alertTemp;
      } else if (index == 1) {
        state.alertVoltage = !state.alertVoltage;
      } else if (index == 2) {
        state.buzzerVolume = static_cast<uint8_t>(clampInt(state.buzzerVolume + delta * 5, 0, 100));
      } else if (index == 3) {
        state.buzzerMute = !state.buzzerMute;
      }
      break;
    case SCREEN_SYSTEM:
      if (index == 0) {
        state.displayBrightness = static_cast<uint8_t>(clampInt(state.displayBrightness + delta * 5, 10, 100));
      } else if (index == 1) {
        state.sleepTimeoutSec = static_cast<uint16_t>(clampInt(state.sleepTimeoutSec + delta * 30, 60, 900));
      } else if (index == 2) {
        state.ledMode = static_cast<LedMode>((state.ledMode + 3 + delta) % 3);
      } else if (index == 3) {
        state.bootMode = static_cast<BootMode>((state.bootMode + 2 + delta) % 2);
      }
      break;
    case SCREEN_DEVELOPER:
      if (index == 0) {
        state.expertMode = !state.expertMode;
      } else if (index == 1) {
        state.experimental = !state.experimental;
      } else if (index == 2) {
        state.perfOverride = static_cast<uint8_t>(clampInt(state.perfOverride + delta, 0, 3));
      }
      break;
    case SCREEN_WIRELESS:
      if (index == 3) {
        state.reconnectEnabled = !state.reconnectEnabled;
      }
      break;
    default:
      break;
  }
}

void UiManager::draw(Renderer &renderer, UiState &state) {
  if (ctx.current == SCREEN_BOOT) {
    ScreenBoot_Draw(renderer);
    if (millis() - ctx.bootStartMs > 350) {
      ctx.current = SCREEN_DASHBOARD;
    }
    return;
  }

  renderer.clear(BG_PRIMARY);
  UiDrawOverlay(renderer, state);

  switch (ctx.current) {
    case SCREEN_DASHBOARD:
      ScreenDashboard_Draw(renderer, state, ctx);
      break;
    case SCREEN_TELEMETRY:
      ScreenTelemetry_Draw(renderer, state, ctx);
      break;
    case SCREEN_ACCESSORY_CTRL:
      ScreenAccessoryControl_Draw(renderer, state, ctx);
      break;
    case SCREEN_ACCESSORY_MAP:
      ScreenAccessoryMapping_Draw(renderer, state, ctx);
      break;
    case SCREEN_GYRO:
      ScreenGyro_Draw(renderer, state, ctx);
      break;
    case SCREEN_STEERING:
      ScreenSteering_Draw(renderer, state, ctx);
      break;
    case SCREEN_THROTTLE:
      ScreenThrottle_Draw(renderer, state, ctx);
      break;
    case SCREEN_SUSPENSION:
      ScreenSuspension_Draw(renderer, state, ctx);
      break;
    case SCREEN_INPUT_MON:
      ScreenInputMonitor_Draw(renderer, state, ctx);
      break;
    case SCREEN_TRIM:
      ScreenTrim_Draw(renderer, state, ctx);
      break;
    case SCREEN_PROFILE_SELECT:
      ScreenProfileSelect_Draw(renderer, state, ctx);
      break;
    case SCREEN_PROFILE_EDIT:
      ScreenProfileEdit_Draw(renderer, state, ctx);
      break;
    case SCREEN_BATTERY:
      ScreenBattery_Draw(renderer, state, ctx);
      break;
    case SCREEN_FAILSAFE:
      ScreenFailsafe_Draw(renderer, state, ctx);
      break;
    case SCREEN_ALERTS:
      ScreenAlerts_Draw(renderer, state, ctx);
      break;
    case SCREEN_SYSTEM:
      ScreenSystem_Draw(renderer, state, ctx);
      break;
    case SCREEN_PERF:
      ScreenPerformance_Draw(renderer, state, ctx);
      break;
    case SCREEN_LOGGING:
      ScreenLogging_Draw(renderer, state, ctx);
      break;
    case SCREEN_CALIBRATION:
      ScreenCalibration_Draw(renderer, state, ctx);
      break;
    case SCREEN_WIRELESS:
      ScreenWireless_Draw(renderer, state, ctx);
      break;
    case SCREEN_DIAGNOSTICS:
      ScreenDiagnostics_Draw(renderer, state, ctx);
      break;
    case SCREEN_DEVELOPER:
      ScreenDeveloper_Draw(renderer, state, ctx);
      break;
    case SCREEN_SAFE_SHUTDOWN:
      ScreenSafeShutdown_Draw(renderer, state, ctx);
      break;
    default:
      break;
  }

  if (ctx.contextActive) {
    UiDrawContextMenu(renderer, ctx);
  }
}
