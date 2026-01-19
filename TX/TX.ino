#include <Arduino.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <math.h>

#include "HardwareConfig.h"
#include "PanelIO.h"
#include "Palette.h"
#include "Renderer.h"
#include "InputManager.h"
#include "UiState.h"
#include "Ui.h"
#include "Buzzer.h"

static Palette palette;
static Renderer renderer;
static InputManager input;
static UiManager ui;
static Buzzer buzzer;
static Preferences prefs;

static UiState state;

static uint16_t *frameA = nullptr;
static uint16_t *frameB = nullptr;
static uint16_t *frontBuffer = nullptr;
static uint16_t *backBuffer = nullptr;

static const int16_t kWidth = PanelIO::kWidth;
static const int16_t kHeight = PanelIO::kHeight;
static const int16_t kTileW = 16;
static const int16_t kTileH = 16;

static uint32_t lastFrameMs = 0;
static uint32_t lastFpsMs = 0;
static uint16_t frameCount = 0;
static uint32_t lastSimMs = 0;
static uint32_t lastDriveMs = 0;

static void allocateBuffers() {
  size_t bufSize = static_cast<size_t>(kWidth) * kHeight * sizeof(uint16_t);
  frameA = static_cast<uint16_t *>(heap_caps_malloc(bufSize, MALLOC_CAP_DMA));
  frameB = static_cast<uint16_t *>(heap_caps_malloc(bufSize, MALLOC_CAP_DMA));
  if (!frameA || !frameB) {
    Serial.println("Frame buffer allocation failed");
    while (true) { delay(100); }
  }
  frontBuffer = frameA;
  backBuffer = frameB;
}

static void flushDirtyTiles() {
  const int tilesX = (kWidth + kTileW - 1) / kTileW;
  const int tilesY = (kHeight + kTileH - 1) / kTileH;

  for (int ty = 0; ty < tilesY; ++ty) {
    for (int tx = 0; tx < tilesX; ++tx) {
      int x = tx * kTileW;
      int y = ty * kTileH;
      int w = (x + kTileW <= kWidth) ? kTileW : (kWidth - x);
      int h = (y + kTileH <= kHeight) ? kTileH : (kHeight - y);

      bool dirty = false;
      for (int yy = 0; yy < h && !dirty; ++yy) {
        int idx = (y + yy) * kWidth + x;
        for (int xx = 0; xx < w; ++xx) {
          if (backBuffer[idx + xx] != frontBuffer[idx + xx]) {
            dirty = true;
            break;
          }
        }
      }

      if (dirty) {
        PanelIO::pushRectDMA(x, y, w, h, backBuffer, kWidth);
      }
    }
  }
}

static void updateSensors() {
  state.rawSteer = analogRead(PIN_STEERING);
  state.rawThrottle = analogRead(PIN_THROTTLE);
  state.rawSuspension = analogRead(PIN_POT_SUSPENSION);

  state.btnMenu = (digitalRead(PIN_BTN_MENU) == LOW);
  state.btnSet = (digitalRead(PIN_BTN_SET) == LOW);
  state.btnTrimPlus = (digitalRead(PIN_BTN_TRIM_PLUS) == LOW);
  state.btnTrimMinus = (digitalRead(PIN_BTN_TRIM_MINUS) == LOW);

  state.gyroOn = (digitalRead(PIN_SW_GYRO) == LOW) || state.gyroOverride;

  float steerPct = (static_cast<int>(state.rawSteer) - 2048 + STEER_CENTER_FIX) / 20.48f;
  float throttlePct = (static_cast<int>(state.rawThrottle) - 2048 + THROT_CENTER_FIX) / 20.48f;
  steerPct += state.steerTrim;
  throttlePct += state.throttleTrim;

  if (fabsf(steerPct) < state.steerDeadzone) steerPct = 0.0f;
  if (fabsf(throttlePct) < state.throttleDeadzone) throttlePct = 0.0f;

  state.steerPct = constrain(steerPct, -100.0f, 100.0f);
  state.throttlePct = constrain(throttlePct, -100.0f, 100.0f);
  state.suspensionPct = constrain(state.rawSuspension / 40.95f, 0.0f, 100.0f);
}

static void updateTelemetry(uint32_t nowMs) {
  float dt = (nowMs - lastSimMs) / 1000.0f;
  if (dt <= 0.0f) return;
  lastSimMs = nowMs;

  float throttleAbs = fabsf(state.throttlePct);
  float targetSpeed = (state.throttlePct > 0.0f) ? (state.throttlePct * 1.2f) : 0.0f;
  state.speedKmh += (targetSpeed - state.speedKmh) * (dt * 2.0f);
  state.speedKmh = constrain(state.speedKmh, 0.0f, 120.0f);

  state.rpmEstimate = throttleAbs * 50.0f;
  state.currentA = throttleAbs * 0.6f;

  float tempTarget = 30.0f + throttleAbs * 0.6f;
  state.tempMotor += (tempTarget - state.tempMotor) * (dt * 0.8f);
  state.tempEsc += (tempTarget - 4.0f - state.tempEsc) * (dt * 0.6f);
  state.tempBoard += (28.0f - state.tempBoard) * (dt * 0.2f) + throttleAbs * 0.01f;

  float wave = (sinf(nowMs / 4000.0f) + 1.0f) * 0.5f;
  state.signalStrength = 88.0f + (wave * 12.0f);
  state.rxConnected = state.signalStrength > 20.0f;

  state.txVoltage = 8.2f - (throttleAbs / 100.0f) * 0.3f;
  state.rxVoltage = 7.4f - (throttleAbs / 100.0f) * 0.2f;

  if (nowMs - lastDriveMs >= 1000) {
    lastDriveMs = nowMs;
    state.driveTimeSec += 1;
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  analogReadResolution(12);
  pinMode(PIN_STEERING, INPUT);
  pinMode(PIN_THROTTLE, INPUT);
  pinMode(PIN_POT_SUSPENSION, INPUT);
  pinMode(PIN_SW_GYRO, INPUT_PULLUP);

  palette.begin();
  input.begin();
  buzzer.begin();
  ui.begin();

  prefs.begin("tx-cfg", false);
  state.steerTrim = prefs.getInt("trim_s", 0);
  state.throttleTrim = prefs.getInt("trim_t", 0);

  if (!PanelIO::begin()) {
    Serial.println("Display init failed");
    while (true) { delay(100); }
  }

  allocateBuffers();
  renderer.setBuffer(backBuffer, kWidth, kHeight, &palette);

  lastFrameMs = millis();
  lastFpsMs = millis();
  lastSimMs = millis();
  lastDriveMs = millis();
}

void loop() {
  uint32_t nowMs = millis();
  uint32_t frameStartUs = micros();

  InputActions actions = input.update();
  updateSensors();
  updateTelemetry(nowMs);
  ui.handleInput(actions, state);

  if (nowMs - lastFrameMs >= 33) {
    lastFrameMs = nowMs;

    renderer.setBuffer(backBuffer, kWidth, kHeight, &palette);
    ui.draw(renderer, state);
    flushDirtyTiles();

    uint16_t *tmp = frontBuffer;
    frontBuffer = backBuffer;
    backBuffer = tmp;

    frameCount++;
    if (nowMs - lastFpsMs >= 1000) {
      state.fps = frameCount;
      frameCount = 0;
      lastFpsMs = nowMs;
    }
  }

  state.loopTimeUs = micros() - frameStartUs;
  state.memFree = ESP.getFreeHeap();
  state.cpuLoad = static_cast<uint8_t>(constrain(state.loopTimeUs / 1000, 0, 100));

  buzzer.update(state);
}
