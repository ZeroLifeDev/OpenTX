#pragma once

#include "UiState.h"

namespace UiStrings {
inline const char *triStateLabel(AccessoryTriState state) {
  switch (state) {
    case ACC_OFF: return "OFF";
    case ACC_LOW: return "LOW";
    default: return "HIGH";
  }
}

inline const char *onOffLabel(bool on) {
  return on ? "ON" : "OFF";
}

inline const char *turnLabel(TurnSignalState state) {
  switch (state) {
    case TURN_LEFT: return "LEFT";
    case TURN_RIGHT: return "RIGHT";
    case TURN_HAZARD: return "HAZARD";
    default: return "OFF";
  }
}

inline const char *brakeLabel(BrakeState state) {
  return state == BRAKE_FORCE ? "FORCE" : "AUTO";
}

inline const char *gyroModeLabel(GyroMode mode) {
  return mode == GYRO_DRIFT ? "DRIFT" : "GRIP";
}

inline const char *suspModeLabel(SuspensionMode mode) {
  return mode == SUSP_AUTO ? "AUTO" : "MANUAL";
}

inline const char *suspPresetLabel(SuspensionPreset preset) {
  switch (preset) {
    case SUSP_LOW: return "LOW";
    case SUSP_MID: return "MID";
    default: return "HIGH";
  }
}

inline const char *failsafeLabel(FailsafeMode mode) {
  switch (mode) {
    case FAILSAFE_CENTER: return "CENTER";
    case FAILSAFE_CUT: return "CUT";
    default: return "HOLD";
  }
}

inline const char *ledModeLabel(LedMode mode) {
  switch (mode) {
    case LED_OFF: return "OFF";
    case LED_ALWAYS: return "ALWAYS";
    default: return "STATUS";
  }
}

inline const char *bootModeLabel(BootMode mode) {
  return mode == BOOT_SAFE ? "SAFE" : "FAST";
}

inline const char *mapLabel(uint8_t map) {
  switch (map) {
    case 0: return "MENU";
    case 1: return "SET";
    case 2: return "TRIM+";
    case 3: return "TRIM-";
    case 4: return "GYRO";
    default: return "NONE";
  }
}
}  // namespace UiStrings
