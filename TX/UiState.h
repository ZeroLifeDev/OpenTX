#pragma once

#include <Arduino.h>

enum AccessoryTriState : uint8_t { ACC_OFF = 0, ACC_LOW, ACC_HIGH };
enum AccessoryOnOff : uint8_t { ACC_OFF_ON = 0, ACC_ON = 1 };
enum TurnSignalState : uint8_t { TURN_OFF = 0, TURN_LEFT, TURN_RIGHT, TURN_HAZARD };
enum BrakeState : uint8_t { BRAKE_AUTO = 0, BRAKE_FORCE };
enum GyroMode : uint8_t { GYRO_DRIFT = 0, GYRO_GRIP };
enum SuspensionMode : uint8_t { SUSP_MANUAL = 0, SUSP_AUTO };
enum SuspensionPreset : uint8_t { SUSP_LOW = 0, SUSP_MID, SUSP_HIGH };
enum FailsafeMode : uint8_t { FAILSAFE_HOLD = 0, FAILSAFE_CENTER, FAILSAFE_CUT };
enum BootMode : uint8_t { BOOT_FAST = 0, BOOT_SAFE };
enum LedMode : uint8_t { LED_OFF = 0, LED_STATUS, LED_ALWAYS };

struct UiState {
  float steerPct = 0.0f;
  float throttlePct = 0.0f;
  float suspensionPct = 0.0f;
  float speedKmh = 0.0f;
  bool gyroOn = false;
  bool gyroOverride = false;
  bool rxConnected = true;

  uint16_t rawSteer = 0;
  uint16_t rawThrottle = 0;
  uint16_t rawSuspension = 0;
  bool btnMenu = false;
  bool btnSet = false;
  bool btnTrimPlus = false;
  bool btnTrimMinus = false;

  float txVoltage = 8.2f;
  float rxVoltage = 7.4f;
  bool rxVoltageAvailable = true;

  float tempMotor = 38.0f;
  float tempEsc = 34.0f;
  float tempBoard = 30.0f;

  float rpmEstimate = 0.0f;
  float currentA = 0.0f;
  float signalStrength = 100.0f;
  float latencyMs = 12.0f;

  int16_t steerTrim = 0;
  int16_t throttleTrim = 0;
  bool trimPerProfile = true;

  AccessoryTriState headlights = ACC_LOW;
  AccessoryOnOff taillights = ACC_ON;
  TurnSignalState turnSignals = TURN_OFF;
  BrakeState brakeLights = BRAKE_AUTO;
  uint8_t auxOutput = 0;
  uint8_t accessoryMap[5] = {0, 0, 0, 0, 0};

  uint8_t gyroGain = 45;
  GyroMode gyroMode = GYRO_GRIP;
  uint8_t gyroResponse = 60;

  uint8_t steerDeadzone = 4;
  int8_t steerCenter = 0;
  uint8_t steerExpo = 12;
  uint8_t steerEndpoint = 100;

  uint8_t throttleDeadzone = 5;
  int8_t throttleCenter = 0;
  uint8_t throttleCurve = 10;
  uint8_t brakeStrength = 65;
  bool reverseLogic = false;

  SuspensionMode suspensionMode = SUSP_AUTO;
  uint8_t suspensionPotMap = 50;
  uint8_t suspensionSpeedLogic = 40;
  SuspensionPreset suspensionPreset = SUSP_MID;

  uint8_t activeProfile = 0;
  bool autoLoadProfile = true;

  float txVoltageWarn = 7.0f;
  float rxVoltageWarn = 6.6f;
  bool voltageSagDetect = true;

  FailsafeMode failsafeMode = FAILSAFE_HOLD;
  bool throttleCut = true;
  bool steeringCenter = true;
  bool alertSignal = true;

  bool alertTemp = true;
  bool alertVoltage = true;
  uint8_t buzzerVolume = 70;
  bool buzzerMute = false;

  uint8_t displayBrightness = 80;
  uint16_t sleepTimeoutSec = 300;
  LedMode ledMode = LED_STATUS;
  BootMode bootMode = BOOT_FAST;

  uint32_t loopTimeUs = 0;
  uint16_t fps = 0;
  uint8_t cpuLoad = 30;
  uint32_t memFree = 0;

  uint16_t peakTemp = 72;
  uint16_t maxSpeed = 88;
  uint32_t driveTimeSec = 0;
  uint8_t errorCount = 0;

  uint8_t wizardStep = 0;

  uint8_t linkQuality = 95;
  uint8_t packetLoss = 1;
  uint16_t updateRate = 150;
  bool reconnectEnabled = true;

  bool sensorsHealthy = true;
  bool gyroValid = true;
  uint16_t adcSanity = 100;

  bool expertMode = false;
  bool experimental = false;
  uint8_t perfOverride = 0;
};
