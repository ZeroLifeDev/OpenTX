#include "Buzzer.h"

namespace {
static const uint16_t kMinOnMs = 20;
}

void Buzzer::begin() {
  pinMode(PIN_BUZZER, OUTPUT);
  setOutput(false);
}

void Buzzer::setOutput(bool on) {
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

void Buzzer::update(const UiState &state) {
  if (state.buzzerMute || state.buzzerVolume == 0) {
    setOutput(false);
    buzzerOn = false;
    lastAlertActive = false;
    lastAlertType = 0;
    pulsesRemaining = 0;
    return;
  }

  bool tempAlert = state.alertTemp && (state.tempMotor > 85.0f || state.tempEsc > 85.0f || state.tempBoard > 70.0f);
  bool signalAlert = state.alertSignal && !state.rxConnected;
  bool voltageAlert = state.alertVoltage && (state.txVoltage < state.txVoltageWarn || (state.rxVoltageAvailable && state.rxVoltage < state.rxVoltageWarn));

  uint16_t baseOn = 0;
  uint16_t baseOff = 0;
  uint8_t alertType = 0;
  uint8_t pulseCount = 0;

  if (tempAlert) {
    baseOn = 80;
    baseOff = 80;
    alertType = 1;
    pulseCount = 4;
  } else if (signalAlert) {
    baseOn = 50;
    baseOff = 50;
    alertType = 2;
    pulseCount = 3;
  } else if (voltageAlert) {
    baseOn = 150;
    baseOff = 150;
    alertType = 3;
    pulseCount = 2;
  }

  if (alertType == 0) {
    setOutput(false);
    buzzerOn = false;
    lastAlertActive = false;
    lastAlertType = 0;
    pulsesRemaining = 0;
    return;
  }

  uint32_t now = millis();
  if (!lastAlertActive || alertType != lastAlertType) {
    lastAlertActive = true;
    lastAlertType = alertType;
    uint16_t scaledOn = static_cast<uint16_t>((static_cast<uint32_t>(baseOn) * state.buzzerVolume) / 100);
    if (scaledOn < kMinOnMs) scaledOn = kMinOnMs;
    onMs = scaledOn;
    offMs = baseOff;
    pulsesRemaining = static_cast<uint8_t>(pulseCount * 2);
    buzzerOn = true;
    setOutput(true);
    lastToggleMs = now;
    pulsesRemaining--;
  }

  if (pulsesRemaining == 0) {
    setOutput(false);
    buzzerOn = false;
    return;
  }

  if (buzzerOn) {
    if (now - lastToggleMs >= onMs) {
      buzzerOn = false;
      lastToggleMs = now;
      setOutput(false);
      if (pulsesRemaining > 0) pulsesRemaining--;
    }
  } else {
    if (now - lastToggleMs >= offMs) {
      buzzerOn = true;
      lastToggleMs = now;
      setOutput(true);
      if (pulsesRemaining > 0) pulsesRemaining--;
    }
  }
}
