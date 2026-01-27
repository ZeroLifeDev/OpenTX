// RC Car TX Firmware - ESP32 + ESP-NOW + 1.8" ILI9163 TFT (portrait)
// Custom TFT driver (no external TFT library).

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include "HardwareConfig.h"

// --------------------------
// Build-time configuration
// --------------------------
#define TFT_W 128
#define TFT_H 160
#define TFT_BGR 1           // 1 if your colors look swapped (BGR panels)
#define TFT_INVERT 0        // 1 to invert colors
#define TFT_X_OFFSET 0
#define TFT_Y_OFFSET 0

#define USE_HW_SPI 0        // 0 = bit-bang SPI (no SPI library), 1 = HW SPI (needs SPI.h)
#define SW_GYRO_ACTIVE_LOW 1

// ESP-NOW settings
static const uint8_t ESPNOW_CHANNEL = 1;
static const uint8_t ESPNOW_BROADCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// UI timing
static const uint32_t UI_FRAME_MS = 33;   // ~30 FPS
static const uint32_t SEND_MIN_MS = 20;   // 50 Hz base

// Buzzer
static const uint8_t BUZZER_CH = 0;

// --------------------------
// Helpers
// --------------------------
static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// --------------------------
// TFT Driver (ILI9163)
// --------------------------
class TFTILI9163 {
public:
  void begin() {
    pinMode(PIN_TFT_CS, OUTPUT);
    pinMode(PIN_TFT_DC, OUTPUT);
    pinMode(PIN_TFT_RST, OUTPUT);
    pinMode(PIN_TFT_SCLK, OUTPUT);
    pinMode(PIN_TFT_MOSI, OUTPUT);

    digitalWrite(PIN_TFT_CS, HIGH);
    digitalWrite(PIN_TFT_SCLK, LOW);
    digitalWrite(PIN_TFT_MOSI, LOW);

    // Reset
    digitalWrite(PIN_TFT_RST, LOW);
    delay(150);
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(150);

    // Init sequence based on Newhaven ILI9163 example, 16bpp
    writeCommand(0x11); // SLPOUT
    delay(100);
    writeCommand(0x28); // DISPOFF
    writeCommand(0x26); // GAMMASET
    writeData(0x04);
    writeCommand(0xB1); // FRMCTR1
    writeData(0x0A);
    writeData(0x14);
    writeCommand(0xC0); // PWCTR1
    writeData(0x0A);
    writeData(0x00);
    writeCommand(0xC1); // PWCTR2
    writeData(0x02);
    writeCommand(0xC5); // VMCTR1
    writeData(0x2F);
    writeData(0x3E);
    writeCommand(0xC7); // VMCTR2
    writeData(0x40);

    writeCommand(0x2A); // CASET
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0x7F);

    writeCommand(0x2B); // PASET
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0x9F);

    setRotation(0);
    writeCommand(0x3A); // COLMOD
    writeData(0x05);    // 16bpp

    if (TFT_INVERT) {
      writeCommand(0x21); // INVON
    } else {
      writeCommand(0x20); // INVOFF
    }

    writeCommand(0x29); // DISPON
    delay(10);
  }

  void setRotation(uint8_t r) {
    rotation = r & 3;
    uint8_t madctl = 0x00;
    switch (rotation) {
      case 0: madctl = 0xC0; break; // portrait
      case 1: madctl = 0xA0; break; // landscape
      case 2: madctl = 0x00; break; // portrait flipped
      case 3: madctl = 0x60; break; // landscape flipped
    }
    if (TFT_BGR) madctl |= 0x08;
    writeCommand(0x36);
    writeData(madctl);
  }

  void fillScreen(uint16_t c) {
    fillRect(0, 0, TFT_W, TFT_H, c);
  }

  void drawPixel(int x, int y, uint16_t c) {
    if (x < 0 || y < 0 || x >= TFT_W || y >= TFT_H) return;
    setAddrWindow(x, y, x, y);
    pushColor(c, 1);
  }

  void drawFastHLine(int x, int y, int w, uint16_t c) {
    if (y < 0 || y >= TFT_H || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > TFT_W) w = TFT_W - x;
    if (w <= 0) return;
    setAddrWindow(x, y, x + w - 1, y);
    pushColor(c, w);
  }

  void drawFastVLine(int x, int y, int h, uint16_t c) {
    if (x < 0 || x >= TFT_W || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > TFT_H) h = TFT_H - y;
    if (h <= 0) return;
    setAddrWindow(x, y, x, y + h - 1);
    pushColor(c, h);
  }

  void drawRect(int x, int y, int w, int h, uint16_t c) {
    drawFastHLine(x, y, w, c);
    drawFastHLine(x, y + h - 1, w, c);
    drawFastVLine(x, y, h, c);
    drawFastVLine(x + w - 1, y, h, c);
  }

  void fillRect(int x, int y, int w, int h, uint16_t c) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > TFT_W) w = TFT_W - x;
    if (y + h > TFT_H) h = TFT_H - y;
    if (w <= 0 || h <= 0) return;
    setAddrWindow(x, y, x + w - 1, y + h - 1);
    pushColor(c, (uint32_t)w * (uint32_t)h);
  }

  void drawLine(int x0, int y0, int x1, int y1, uint16_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
      drawPixel(x0, y0, c);
      if (x0 == x1 && y0 == y1) break;
      e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  void drawChar(int x, int y, char ch, uint16_t color, uint16_t bg, uint8_t size);
  void drawText(int x, int y, const char *s, uint16_t color, uint16_t bg, uint8_t size);

private:
  uint8_t rotation = 0;

  void csLow() { digitalWrite(PIN_TFT_CS, LOW); }
  void csHigh() { digitalWrite(PIN_TFT_CS, HIGH); }
  void dcLow() { digitalWrite(PIN_TFT_DC, LOW); }
  void dcHigh() { digitalWrite(PIN_TFT_DC, HIGH); }

  void writeCommand(uint8_t c) {
    csLow();
    dcLow();
    spiWrite(c);
    csHigh();
  }

  void writeData(uint8_t d) {
    csLow();
    dcHigh();
    spiWrite(d);
    csHigh();
  }

  void writeData16(uint16_t d) {
    csLow();
    dcHigh();
    spiWrite(d >> 8);
    spiWrite(d & 0xFF);
    csHigh();
  }

  void setAddrWindow(int x0, int y0, int x1, int y1) {
    x0 += TFT_X_OFFSET;
    x1 += TFT_X_OFFSET;
    y0 += TFT_Y_OFFSET;
    y1 += TFT_Y_OFFSET;
    writeCommand(0x2A);
    csLow(); dcHigh();
    spiWrite(x0 >> 8); spiWrite(x0 & 0xFF);
    spiWrite(x1 >> 8); spiWrite(x1 & 0xFF);
    csHigh();
    writeCommand(0x2B);
    csLow(); dcHigh();
    spiWrite(y0 >> 8); spiWrite(y0 & 0xFF);
    spiWrite(y1 >> 8); spiWrite(y1 & 0xFF);
    csHigh();
    writeCommand(0x2C);
  }

  void pushColor(uint16_t color, uint32_t count) {
    csLow(); dcHigh();
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    while (count--) {
      spiWrite(hi);
      spiWrite(lo);
    }
    csHigh();
  }

  void spiWrite(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
      digitalWrite(PIN_TFT_SCLK, LOW);
      digitalWrite(PIN_TFT_MOSI, (data & 0x80) ? HIGH : LOW);
      digitalWrite(PIN_TFT_SCLK, HIGH);
      data <<= 1;
    }
  }
};

TFTILI9163 tft;

// 5x7 ASCII font, 0x20..0x7F
static const uint8_t font5x7[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
  0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62, 0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00,
  0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00, 0x14,0x08,0x3E,0x08,0x14, 0x08,0x08,0x3E,0x08,0x08,
  0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x60,0x60,0x00,0x00, 0x20,0x10,0x08,0x04,0x02,
  0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31,
  0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03,
  0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E, 0x00,0x36,0x36,0x00,0x00, 0x00,0x56,0x36,0x00,0x00,
  0x08,0x14,0x22,0x41,0x00, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x51,0x09,0x06,
  0x32,0x49,0x79,0x41,0x3E, 0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
  0x7F,0x41,0x41,0x22,0x1C, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x49,0x49,0x7A,
  0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41,
  0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
  0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31,
  0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F,
  0x63,0x14,0x08,0x14,0x63, 0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43, 0x00,0x7F,0x41,0x41,0x00,
  0x02,0x04,0x08,0x10,0x20, 0x00,0x41,0x41,0x7F,0x00, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
  0x00,0x03,0x05,0x00,0x00, 0x20,0x54,0x54,0x54,0x78, 0x7F,0x48,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x20,
  0x38,0x44,0x44,0x48,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x08,0x7E,0x09,0x01,0x02, 0x0C,0x52,0x52,0x52,0x3E,
  0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x44,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00,
  0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x18,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
  0x7C,0x14,0x14,0x14,0x08, 0x08,0x14,0x14,0x18,0x7C, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x20,
  0x04,0x3F,0x44,0x40,0x20, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
  0x44,0x28,0x10,0x28,0x44, 0x0C,0x50,0x50,0x50,0x3C, 0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00,
  0x00,0x00,0x7F,0x00,0x00, 0x00,0x41,0x36,0x08,0x00, 0x02,0x01,0x02,0x04,0x02, 0x3C,0x26,0x23,0x26,0x3C
};

void TFTILI9163::drawChar(int x, int y, char ch, uint16_t color, uint16_t bg, uint8_t size) {
  if (ch < 0x20 || ch > 0x7F) ch = '?';
  uint16_t index = (ch - 0x20) * 5;
  for (uint8_t i = 0; i < 5; i++) {
    uint8_t line = pgm_read_byte(&font5x7[index + i]);
    for (uint8_t j = 0; j < 7; j++) {
      uint16_t px = (line & 0x01) ? color : bg;
      if (px != bg || bg != 0xFFFF) {
        if (size == 1) {
          drawPixel(x + i, y + j, px);
        } else {
          fillRect(x + i * size, y + j * size, size, size, px);
        }
      }
      line >>= 1;
    }
  }
}

void TFTILI9163::drawText(int x, int y, const char *s, uint16_t color, uint16_t bg, uint8_t size) {
  while (*s) {
    drawChar(x, y, *s, color, bg, size);
    x += (6 * size);
    s++;
  }
}

// --------------------------
// Inputs & Config
// --------------------------
struct AxisCal {
  int16_t min;
  int16_t center;
  int16_t max;
};

struct TxConfig {
  AxisCal steer;
  AxisCal throttle;
  AxisCal susp;
  int16_t trimSteer;
  int16_t trimThrot;
  uint8_t driveMode;    // 0..2
  uint8_t expoSteer;    // 0..100
  uint8_t expoThrot;    // 0..100
  uint8_t rateSteer;    // 50..100
  uint8_t rateThrot;    // 50..100
  uint8_t sendHz;       // 20..60
  uint8_t invertSteer;  // 0/1
  uint8_t invertThrot;  // 0/1
};

TxConfig cfg;
Preferences prefs;

struct AxisState {
  int raw = 0;
  float smooth = 0.0f;
  int value = 0;
};

AxisState steerAxis, throtAxis, suspAxis;

// --------------------------
// ESP-NOW Packet
// --------------------------
struct ControlPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t seq;
  uint32_t ms;
  int16_t steer;
  int16_t throttle;
  int16_t suspension;
  int16_t trimSteer;
  int16_t trimThrot;
  uint8_t driveMode;
  uint8_t flags;
  uint8_t checksum;
};

static uint8_t txSeq = 0;
static bool lastSendOk = false;
static uint32_t lastAckMs = 0;

// --------------------------
// Buttons
// --------------------------
struct Button {
  int pin = -1;
  bool activeLow = true;
  bool last = false;
  bool pressed = false;
  bool released = false;
  uint32_t lastChange = 0;
  uint32_t pressStart = 0;

  void begin(int p, bool actLow = true) {
    pin = p;
    activeLow = actLow;
    if (pin >= 0) {
      pinMode(pin, actLow ? INPUT_PULLUP : INPUT);
      last = read();
    }
  }

  bool read() const {
    if (pin < 0) return false;
    bool v = digitalRead(pin);
    return activeLow ? !v : v;
  }

  void update() {
    pressed = false;
    released = false;
    if (pin < 0) return;
    bool v = read();
    uint32_t now = millis();
    if (v != last && (now - lastChange) > 20) {
      lastChange = now;
      if (v) {
        pressed = true;
        pressStart = now;
      } else {
        released = true;
      }
      last = v;
    }
  }

  bool isDown() const { return last; }

  bool heldFor(uint32_t ms) const {
    return last && (millis() - pressStart >= ms);
  }
};

Button btnMenu, btnSet, btnTrimPlus, btnTrimMinus;

// --------------------------
// UI State
// --------------------------
enum ScreenId { SCREEN_HOME = 0, SCREEN_MENU = 1, SCREEN_CAL = 2 };
static ScreenId screen = SCREEN_HOME;
static uint32_t lastFrameMs = 0;
static bool uiDirty = true;

static int lastSteerUI = 0;
static int lastThrotUI = 0;
static int lastSuspUI = 0;
static bool lastConnUI = false;
static uint8_t lastModeUI = 255;

// Calibration state
enum CalStep { CAL_NONE, CAL_CENTER, CAL_STEER_SWEEP, CAL_THROT_SWEEP, CAL_SUSP_SWEEP, CAL_DONE };
static CalStep calStep = CAL_NONE;
static int calMin = 4095, calMax = 0;

// --------------------------
// Colors
// --------------------------
static const uint16_t C_BG = rgb565(10, 18, 28);
static const uint16_t C_PANEL = rgb565(20, 30, 46);
static const uint16_t C_ACCENT = rgb565(255, 140, 40);
static const uint16_t C_ACCENT2 = rgb565(60, 200, 160);
static const uint16_t C_TEXT = rgb565(230, 235, 240);
static const uint16_t C_DIM = rgb565(140, 150, 160);
static const uint16_t C_WARN = rgb565(255, 60, 60);

// --------------------------
// Buzzer
// --------------------------
static uint32_t buzzerUntil = 0;
static bool buzzerOn = false;

static void buzzerBegin() {
  if (PIN_BUZZER >= 0) {
    ledcSetup(BUZZER_CH, 2000, 8);
    ledcAttachPin(PIN_BUZZER, BUZZER_CH);
    ledcWriteTone(BUZZER_CH, 0);
  }
}

static void buzzerBeep(uint16_t freq, uint16_t ms) {
  if (PIN_BUZZER < 0) return;
  buzzerOn = true;
  buzzerUntil = millis() + ms;
  ledcWriteTone(BUZZER_CH, freq);
}

static void buzzerUpdate() {
  if (!buzzerOn) return;
  if ((int32_t)(millis() - buzzerUntil) >= 0) {
    buzzerOn = false;
    ledcWriteTone(BUZZER_CH, 0);
  }
}

// --------------------------
// Config storage
// --------------------------
static void loadDefaults() {
  cfg.steer = {500, JOY_CENTER, 3500};
  cfg.throttle = {500, JOY_CENTER, 3500};
  cfg.susp = {500, 2048, 3500};
  cfg.trimSteer = 0;
  cfg.trimThrot = 0;
  cfg.driveMode = 1;
  cfg.expoSteer = 35;
  cfg.expoThrot = 30;
  cfg.rateSteer = 90;
  cfg.rateThrot = 90;
  cfg.sendHz = 50;
  cfg.invertSteer = 0;
  cfg.invertThrot = 0;
}

static void loadConfig() {
  loadDefaults();
  prefs.begin("txcfg", true);
  if (prefs.getBool("ok", false)) {
    prefs.getBytes("cfg", &cfg, sizeof(cfg));
  }
  prefs.end();
}

static void saveConfig() {
  prefs.begin("txcfg", false);
  prefs.putBool("ok", true);
  prefs.putBytes("cfg", &cfg, sizeof(cfg));
  prefs.end();
}

// --------------------------
// Axis processing
// --------------------------
static int readAnalogSmooth(int pin) {
  if (pin < 0) return 0;
  int v = 0;
  for (int i = 0; i < 4; i++) v += analogRead(pin);
  return v / 4;
}

static int mapAxis(int raw, const AxisCal &cal) {
  if (raw >= cal.center) {
    if (cal.max <= cal.center + 10) return 0;
    return (int)((int32_t)(raw - cal.center) * 1000 / (cal.max - cal.center));
  }
  if (cal.center <= cal.min + 10) return 0;
  return (int)((int32_t)(raw - cal.center) * 1000 / (cal.center - cal.min));
}

static int applyExpo(int v, uint8_t expo) {
  float e = expo / 100.0f;
  float x = v / 1000.0f;
  float out = (1.0f - e) * x + e * x * x * x;
  return (int)(out * 1000.0f);
}

static int processAxis(AxisState &st, int raw, const AxisCal &cal, int16_t trim, uint8_t expo, uint8_t rate, bool invert) {
  st.raw = raw;
  st.smooth = st.smooth + 0.25f * (raw - st.smooth);
  int v = mapAxis((int)st.smooth, cal);
  if (abs(raw - cal.center) < JOY_DEADZONE) v = 0;
  if (invert) v = -v;
  v = applyExpo(v, expo);
  v = (int)((int32_t)v * rate / 100);
  v = clampi(v + trim * 5, -1000, 1000);
  st.value = v;
  return v;
}

// --------------------------
// ESP-NOW
// --------------------------
static uint8_t calcChecksum(const ControlPacket &p) {
  const uint8_t *b = (const uint8_t *)&p;
  uint8_t c = 0;
  for (size_t i = 0; i < sizeof(ControlPacket) - 1; i++) c ^= b[i];
  return c;
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendOk = (status == ESP_NOW_SEND_SUCCESS);
  if (lastSendOk) lastAckMs = millis();
}

static void espnowInit() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    buzzerBeep(200, 200);
    return;
  }
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, ESPNOW_BROADCAST, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

// --------------------------
// UI Rendering
// --------------------------
static void drawHeader() {
  tft.fillRect(0, 0, TFT_W, 22, rgb565(12, 22, 34));
  tft.drawText(6, 6, "RC TX", C_TEXT, 0xFFFF, 1);
  tft.drawText(70, 6, "DMI", C_DIM, 0xFFFF, 1);
}

static void drawPanels() {
  tft.fillRect(0, 22, TFT_W, TFT_H - 22, C_BG);
  tft.fillRect(6, 28, 30, 100, C_PANEL);   // Throttle
  tft.fillRect(42, 36, 80, 20, C_PANEL);   // Steering bar
  tft.fillRect(42, 66, 80, 42, C_PANEL);   // Mode / Status
  tft.fillRect(42, 114, 80, 14, C_PANEL);  // Suspension
  tft.fillRect(6, 132, 116, 22, C_PANEL);  // Footer

  tft.drawText(9, 30, "THR", C_DIM, 0xFFFF, 1);
  tft.drawText(45, 38, "STEER", C_DIM, 0xFFFF, 1);
  tft.drawText(45, 68, "MODE", C_DIM, 0xFFFF, 1);
  tft.drawText(45, 116, "SUSP", C_DIM, 0xFFFF, 1);
}

static void drawConnection(bool ok) {
  uint16_t col = ok ? C_ACCENT2 : C_WARN;
  tft.fillRect(108, 4, 16, 14, rgb565(12, 22, 34));
  tft.drawRect(108, 4, 16, 14, col);
  tft.fillRect(111, 7, ok ? 10 : 4, 8, col);
}

static void drawThrottle(int v) {
  int top = 38;
  int h = 92;
  int mid = top + (h / 2);
  tft.drawFastHLine(8, mid, 26, C_DIM);
  tft.fillRect(8, top, 26, h, C_PANEL);
  int bar = (v * (h / 2 - 2)) / 1000;
  if (bar >= 0) {
    tft.fillRect(8, mid - bar, 26, bar, C_ACCENT);
  } else {
    tft.fillRect(8, mid, 26, -bar, C_ACCENT2);
  }
  tft.drawRect(6, 28, 30, 100, C_DIM);
}

static void drawSteer(int v) {
  int x = 44, y = 50, w = 76, h = 8;
  tft.fillRect(x, y, w, h, C_PANEL);
  tft.drawRect(42, 36, 80, 20, C_DIM);
  int mid = x + w / 2;
  tft.drawFastVLine(mid, y, h, C_DIM);
  int dx = (v * (w / 2 - 2)) / 1000;
  if (dx >= 0) {
    tft.fillRect(mid, y + 1, dx, h - 2, C_ACCENT);
  } else {
    tft.fillRect(mid + dx, y + 1, -dx, h - 2, C_ACCENT2);
  }
}

static void drawSusp(int v) {
  int x = 44, y = 122, w = 76, h = 6;
  tft.fillRect(x, y, w, h, C_PANEL);
  int fill = (v + 1000) * w / 2000;
  tft.fillRect(x, y, fill, h, C_ACCENT2);
  tft.drawRect(42, 114, 80, 14, C_DIM);
}

static void drawMode(uint8_t mode) {
  tft.fillRect(44, 82, 76, 20, C_PANEL);
  const char *name = (mode == 0) ? "ECO" : (mode == 1) ? "NORM" : "SPORT";
  uint16_t col = (mode == 2) ? C_ACCENT : C_TEXT;
  tft.drawText(60, 86, name, col, 0xFFFF, 2);
}

static void drawFooter() {
  tft.fillRect(8, 136, 112, 14, C_PANEL);
  tft.drawText(10, 136, "TRIM", C_DIM, 0xFFFF, 1);
}

static void drawTrim(int16_t s, int16_t t) {
  char buf[32];
  tft.fillRect(44, 136, 76, 14, C_PANEL);
  snprintf(buf, sizeof(buf), "S%+d T%+d", s, t);
  tft.drawText(44, 136, buf, C_TEXT, 0xFFFF, 1);
}

static void drawAnimAccent() {
  uint32_t now = millis();
  int w = TFT_W - 12;
  int t = (now / 20) % (2 * w);
  int x = t < w ? t : (2 * w - t);
  tft.fillRect(6, 20, TFT_W - 12, 2, rgb565(12, 22, 34));
  tft.fillRect(6 + x, 20, 10, 2, C_ACCENT);
}

static void renderHome(bool force) {
  bool connected = (millis() - lastAckMs) < 1000;
  if (force || uiDirty) {
    drawHeader();
    drawPanels();
    drawFooter();
    uiDirty = false;
    lastSteerUI = 9999;
    lastThrotUI = 9999;
    lastSuspUI = 9999;
    lastModeUI = 255;
    lastConnUI = !connected;
  }
  if (force || connected != lastConnUI) {
    drawConnection(connected);
    lastConnUI = connected;
  }
  if (force || abs(throtAxis.value - lastThrotUI) > 10) {
    drawThrottle(throtAxis.value);
    lastThrotUI = throtAxis.value;
  }
  if (force || abs(steerAxis.value - lastSteerUI) > 10) {
    drawSteer(steerAxis.value);
    lastSteerUI = steerAxis.value;
  }
  if (force || abs(suspAxis.value - lastSuspUI) > 10) {
    drawSusp(suspAxis.value);
    lastSuspUI = suspAxis.value;
  }
  if (force || cfg.driveMode != lastModeUI) {
    drawMode(cfg.driveMode);
    lastModeUI = cfg.driveMode;
  }
  drawTrim(cfg.trimSteer, cfg.trimThrot);
  drawAnimAccent();
}

static const char *menuLabels[] = {
  "TRIM STEER", "TRIM THROT", "DRIVE MODE", "EXPO STEER", "EXPO THROT",
  "RATE STEER", "RATE THROT", "SEND HZ", "INV STEER", "INV THROT", "CALIBRATE", "SAVE"
};
static const uint8_t MENU_COUNT = sizeof(menuLabels) / sizeof(menuLabels[0]);
static uint8_t menuIndex = 0;
static uint8_t menuTop = 0;
static bool menuEdit = false;

static void renderMenu() {
  tft.fillScreen(C_BG);
  tft.drawText(8, 6, "SETTINGS", C_TEXT, 0xFFFF, 1);
  tft.drawRect(6, 20, 116, 128, C_DIM);

  if (menuIndex < menuTop) menuTop = menuIndex;
  if (menuIndex > menuTop + 7) menuTop = menuIndex - 7;

  char buf[16];
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t idx = menuTop + i;
    if (idx >= MENU_COUNT) break;
    int y = 24 + i * 14;
    bool sel = (idx == menuIndex);
    uint16_t rowBg = sel ? (menuEdit ? C_ACCENT2 : C_ACCENT) : C_PANEL;
    uint16_t rowFg = sel ? C_BG : C_TEXT;
    tft.fillRect(8, y, 112, 12, rowBg);
    tft.drawText(10, y + 2, menuLabels[idx], rowFg, 0xFFFF, 1);

    const char *val = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%d", cfg.trimSteer); val = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%d", cfg.trimThrot); val = buf; }
    else if (idx == 2) { val = (cfg.driveMode == 0) ? "ECO" : (cfg.driveMode == 1) ? "NORM" : "SPORT"; }
    else if (idx == 3) { snprintf(buf, sizeof(buf), "%u", cfg.expoSteer); val = buf; }
    else if (idx == 4) { snprintf(buf, sizeof(buf), "%u", cfg.expoThrot); val = buf; }
    else if (idx == 5) { snprintf(buf, sizeof(buf), "%u", cfg.rateSteer); val = buf; }
    else if (idx == 6) { snprintf(buf, sizeof(buf), "%u", cfg.rateThrot); val = buf; }
    else if (idx == 7) { snprintf(buf, sizeof(buf), "%u", cfg.sendHz); val = buf; }
    else if (idx == 8) { val = cfg.invertSteer ? "ON" : "OFF"; }
    else if (idx == 9) { val = cfg.invertThrot ? "ON" : "OFF"; }
    else if (idx == 10) { val = ">"; }
    else if (idx == 11) { val = ">"; }
    int vx = 8 + 112 - (strlen(val) * 6);
    tft.drawText(vx, y + 2, val, rowFg, 0xFFFF, 1);
  }
}

static void renderCalScreen() {
  tft.fillScreen(C_BG);
  tft.drawText(8, 6, "CALIBRATION", C_TEXT, 0xFFFF, 1);
  tft.drawRect(6, 20, 116, 120, C_DIM);
  const char *line1 = "";
  const char *line2 = "";
  switch (calStep) {
    case CAL_CENTER: line1 = "Center sticks"; line2 = "Press SET"; break;
    case CAL_STEER_SWEEP: line1 = "Sweep steering"; line2 = "Press SET"; break;
    case CAL_THROT_SWEEP: line1 = "Sweep throttle"; line2 = "Press SET"; break;
    case CAL_SUSP_SWEEP: line1 = "Sweep suspension"; line2 = "Press SET"; break;
    case CAL_DONE: line1 = "Saved!"; line2 = "Press MENU"; break;
    default: break;
  }
  tft.drawText(10, 40, line1, C_TEXT, 0xFFFF, 1);
  tft.drawText(10, 56, line2, C_TEXT, 0xFFFF, 1);
}

// --------------------------
// Input & UI Handling
// --------------------------
static void startCalibration() {
  calStep = CAL_CENTER;
  uiDirty = true;
  renderCalScreen();
}

static void updateCalibration() {
  if (calStep == CAL_CENTER) {
    if (btnSet.pressed) {
      cfg.steer.center = readAnalogSmooth(PIN_STEERING) + STEER_CENTER_FIX;
      cfg.throttle.center = readAnalogSmooth(PIN_THROTTLE) + THROT_CENTER_FIX;
      cfg.susp.center = readAnalogSmooth(PIN_POT_SUSPENSION);
      calStep = CAL_STEER_SWEEP;
      calMin = 4095; calMax = 0;
      renderCalScreen();
    }
  } else if (calStep == CAL_STEER_SWEEP) {
    int v = readAnalogSmooth(PIN_STEERING);
    if (v < calMin) calMin = v;
    if (v > calMax) calMax = v;
    if (btnSet.pressed) {
      cfg.steer.min = calMin;
      cfg.steer.max = calMax;
      calStep = CAL_THROT_SWEEP;
      calMin = 4095; calMax = 0;
      renderCalScreen();
    }
  } else if (calStep == CAL_THROT_SWEEP) {
    int v = readAnalogSmooth(PIN_THROTTLE);
    if (v < calMin) calMin = v;
    if (v > calMax) calMax = v;
    if (btnSet.pressed) {
      cfg.throttle.min = calMin;
      cfg.throttle.max = calMax;
      calStep = CAL_SUSP_SWEEP;
      calMin = 4095; calMax = 0;
      renderCalScreen();
    }
  } else if (calStep == CAL_SUSP_SWEEP) {
    int v = readAnalogSmooth(PIN_POT_SUSPENSION);
    if (v < calMin) calMin = v;
    if (v > calMax) calMax = v;
    if (btnSet.pressed) {
      cfg.susp.min = calMin;
      cfg.susp.max = calMax;
      calStep = CAL_DONE;
      saveConfig();
      renderCalScreen();
      buzzerBeep(1200, 120);
    }
  } else if (calStep == CAL_DONE) {
    if (btnMenu.pressed) {
      calStep = CAL_NONE;
      screen = SCREEN_HOME;
      uiDirty = true;
      tft.fillScreen(C_BG);
    }
  }
}

static void handleButtons() {
  btnMenu.update();
  btnSet.update();
  btnTrimPlus.update();
  btnTrimMinus.update();

  if (screen == SCREEN_HOME) {
    if (btnMenu.released) {
      uint32_t held = millis() - btnMenu.pressStart;
      if (held >= 1500) {
        screen = SCREEN_CAL;
        startCalibration();
      } else {
        screen = SCREEN_MENU;
        renderMenu();
      }
    }
    if (btnTrimPlus.pressed) cfg.trimSteer = clampi(cfg.trimSteer + 1, -50, 50);
    if (btnTrimMinus.pressed) cfg.trimSteer = clampi(cfg.trimSteer - 1, -50, 50);
    if (btnSet.pressed) cfg.driveMode = (cfg.driveMode + 1) % 3;
  } else if (screen == SCREEN_MENU) {
    if (btnMenu.pressed) {
      if (menuEdit) {
        menuEdit = false;
        renderMenu();
      } else {
        saveConfig();
        screen = SCREEN_HOME;
        uiDirty = true;
        tft.fillScreen(C_BG);
      }
    }
    if (btnSet.pressed) {
      if (menuIndex == 10) {
        screen = SCREEN_CAL;
        startCalibration();
      } else if (menuIndex == 11) {
        saveConfig();
        buzzerBeep(1000, 120);
        renderMenu();
      } else {
        menuEdit = !menuEdit;
        renderMenu();
      }
    }
    if (btnTrimPlus.pressed) {
      if (menuEdit) {
        // Adjust
        int delta = 1;
        if (menuIndex == 0) cfg.trimSteer = clampi(cfg.trimSteer + delta, -50, 50);
        else if (menuIndex == 1) cfg.trimThrot = clampi(cfg.trimThrot + delta, -50, 50);
        else if (menuIndex == 2) cfg.driveMode = (cfg.driveMode + 1) % 3;
        else if (menuIndex == 3) cfg.expoSteer = clampi(cfg.expoSteer + 2, 0, 100);
        else if (menuIndex == 4) cfg.expoThrot = clampi(cfg.expoThrot + 2, 0, 100);
        else if (menuIndex == 5) cfg.rateSteer = clampi(cfg.rateSteer + 2, 50, 100);
        else if (menuIndex == 6) cfg.rateThrot = clampi(cfg.rateThrot + 2, 50, 100);
        else if (menuIndex == 7) cfg.sendHz = clampi(cfg.sendHz + 1, 20, 60);
        else if (menuIndex == 8) cfg.invertSteer = !cfg.invertSteer;
        else if (menuIndex == 9) cfg.invertThrot = !cfg.invertThrot;
        renderMenu();
      } else {
        menuIndex = (menuIndex + 1) % MENU_COUNT;
        renderMenu();
      }
    }
    if (btnTrimMinus.pressed) {
      if (menuEdit) {
        int delta = -1;
        if (menuIndex == 0) cfg.trimSteer = clampi(cfg.trimSteer + delta, -50, 50);
        else if (menuIndex == 1) cfg.trimThrot = clampi(cfg.trimThrot + delta, -50, 50);
        else if (menuIndex == 2) cfg.driveMode = (cfg.driveMode == 0) ? 2 : (cfg.driveMode - 1);
        else if (menuIndex == 3) cfg.expoSteer = clampi(cfg.expoSteer - 2, 0, 100);
        else if (menuIndex == 4) cfg.expoThrot = clampi(cfg.expoThrot - 2, 0, 100);
        else if (menuIndex == 5) cfg.rateSteer = clampi(cfg.rateSteer - 2, 50, 100);
        else if (menuIndex == 6) cfg.rateThrot = clampi(cfg.rateThrot - 2, 50, 100);
        else if (menuIndex == 7) cfg.sendHz = clampi(cfg.sendHz - 1, 20, 60);
        else if (menuIndex == 8) cfg.invertSteer = !cfg.invertSteer;
        else if (menuIndex == 9) cfg.invertThrot = !cfg.invertThrot;
        renderMenu();
      } else {
        menuIndex = (menuIndex == 0) ? (MENU_COUNT - 1) : (menuIndex - 1);
        renderMenu();
      }
    }
  } else if (screen == SCREEN_CAL) {
    updateCalibration();
  }
}

// --------------------------
// Main
// --------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(PIN_LED_BUILTIN, OUTPUT);
  digitalWrite(PIN_LED_BUILTIN, LED_OFF_STATE);
  if (PIN_SW_GYRO >= 0) pinMode(PIN_SW_GYRO, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  btnMenu.begin(PIN_BTN_MENU, true);
  btnSet.begin(PIN_BTN_SET, true);
  btnTrimPlus.begin(PIN_BTN_TRIM_PLUS, true);
  btnTrimMinus.begin(PIN_BTN_TRIM_MINUS, true);

  buzzerBegin();
  loadConfig();
  tft.begin();
  tft.fillScreen(C_BG);
  drawHeader();
  drawPanels();
  drawFooter();
  buzzerBeep(1200, 80);

  espnowInit();
}

void loop() {
  buzzerUpdate();
  handleButtons();

  // Read and process inputs
  int rawSteer = readAnalogSmooth(PIN_STEERING) + STEER_CENTER_FIX;
  int rawThrot = readAnalogSmooth(PIN_THROTTLE) + THROT_CENTER_FIX;
  int rawSusp = readAnalogSmooth(PIN_POT_SUSPENSION);

  processAxis(steerAxis, rawSteer, cfg.steer, cfg.trimSteer, cfg.expoSteer, cfg.rateSteer, cfg.invertSteer);
  processAxis(throtAxis, rawThrot, cfg.throttle, cfg.trimThrot, cfg.expoThrot, cfg.rateThrot, cfg.invertThrot);
  processAxis(suspAxis, rawSusp, cfg.susp, 0, 0, 100, false);

  // Send packet
  static uint32_t lastSendMs = 0;
  uint32_t now = millis();
  uint32_t sendInterval = 1000 / clampi(cfg.sendHz, 20, 60);
  if (now - lastSendMs >= sendInterval) {
    lastSendMs = now;
    ControlPacket p = {};
    p.magic = 0xACED;
    p.version = 1;
    p.seq = txSeq++;
    p.ms = now;
    p.steer = steerAxis.value;
    p.throttle = throtAxis.value;
    p.suspension = suspAxis.value;
    p.trimSteer = cfg.trimSteer;
    p.trimThrot = cfg.trimThrot;
    p.driveMode = cfg.driveMode;
    uint8_t flags = 0;
    if (PIN_SW_GYRO >= 0) {
      bool gyroOn = SW_GYRO_ACTIVE_LOW ? (digitalRead(PIN_SW_GYRO) == LOW)
                                       : (digitalRead(PIN_SW_GYRO) == HIGH);
      if (gyroOn) flags |= 0x01;
    }
    p.flags = flags;
    p.checksum = calcChecksum(p);
    esp_now_send(ESPNOW_BROADCAST, (uint8_t *)&p, sizeof(p));
  }

  // UI
  if (screen == SCREEN_HOME) {
    if (now - lastFrameMs >= UI_FRAME_MS) {
      lastFrameMs = now;
      renderHome(false);
    }
  } else if (screen == SCREEN_MENU) {
    // static screen, refresh only on changes
  } else if (screen == SCREEN_CAL) {
    // calibration handled in updateCalibration
  }

  // LED status
  bool connected = (millis() - lastAckMs) < 1000;
  digitalWrite(PIN_LED_BUILTIN, connected ? LED_OFF_STATE : LED_ON_STATE);
}
