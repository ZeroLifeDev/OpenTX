// RC Car TX Firmware - ESP32 + ESP-NOW + 1.8" ILI9163 TFT (portrait)
// Custom TFT driver (no external TFT library).

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <driver/spi_master.h>
#include <esp_idf_version.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <string.h>
#include "HardwareConfig.h"
#include "TxTypes.h"
#include "OpenTX_Font8x16.h"
#include "OpenTX_AdvMenu.h"

struct RateExpo;

// --------------------------
// Build-time configuration
// --------------------------
#define TFT_W 128
#define TFT_H 160
#define TFT_BGR 1           // 1 if your colors look swapped (BGR panels)
#define TFT_INVERT 0        // 1 to invert colors
#define TFT_X_OFFSET 0
#define TFT_Y_OFFSET 0

#define SW_GYRO_ACTIVE_LOW 1

// ESP-NOW settings
static const uint8_t ESPNOW_CHANNEL = 1;
static const uint8_t ESPNOW_BROADCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// UI timing
static const uint32_t UI_FRAME_MS = 16;   // ~60 FPS
static const uint32_t UI_MENU_MS = 32;    // ~30 FPS for animated menus
static const uint32_t SEND_MIN_MS = 20;   // 50 Hz base

// Buzzer
static const uint8_t BUZZER_CH = 0;

static const uint8_t CFG_VERSION = 3;
static const uint8_t MAX_PEERS = 5;
static const uint32_t PAIR_LISTEN_MS = 12000;
static const uint8_t MAX_PROFILES = 6;

// --------------------------
// Helpers
// --------------------------
static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  return (uint16_t)((c << 8) | (c >> 8)); // swap for DMA byte order
}

static inline float lerpf(float a, float b, float t) {
  return a + (b - a) * t;
}

static inline uint8_t lerp8(uint8_t a, uint8_t b, uint8_t t) {
  return (uint8_t)(a + ((int)(b - a) * t) / 255);
}

static inline uint16_t mix565(uint16_t a, uint16_t b, uint8_t t) {
  uint16_t a0 = (uint16_t)((a << 8) | (a >> 8));
  uint16_t b0 = (uint16_t)((b << 8) | (b >> 8));
  uint8_t ar = (uint8_t)((a0 >> 11) & 0x1F);
  uint8_t ag = (uint8_t)((a0 >> 5) & 0x3F);
  uint8_t ab = (uint8_t)(a0 & 0x1F);
  uint8_t br = (uint8_t)((b0 >> 11) & 0x1F);
  uint8_t bg = (uint8_t)((b0 >> 5) & 0x3F);
  uint8_t bb = (uint8_t)(b0 & 0x1F);
  uint8_t rr = (uint8_t)(ar + ((int)(br - ar) * t) / 255);
  uint8_t rg = (uint8_t)(ag + ((int)(bg - ag) * t) / 255);
  uint8_t rb = (uint8_t)(ab + ((int)(bb - ab) * t) / 255);
  uint16_t c = (uint16_t)((rr << 11) | (rg << 5) | rb);
  return (uint16_t)((c << 8) | (c >> 8));
}

static int applyExpo(int v, uint8_t expo);
static inline bool macEqual(const uint8_t *a, const uint8_t *b) {
  for (int i = 0; i < 6; i++) if (a[i] != b[i]) return false;
  return true;
}

// --------------------------
// TFT Driver (ILI9163)
// --------------------------
class TFTILI9163 {
public:
  void begin() {
    pinMode(PIN_TFT_DC, OUTPUT);
    pinMode(PIN_TFT_RST, OUTPUT);

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = PIN_TFT_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = PIN_TFT_SCLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4096 + 8;

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 40000000;
    devcfg.mode = 0;
    devcfg.spics_io_num = PIN_TFT_CS;
    devcfg.queue_size = 1;
    devcfg.flags = SPI_DEVICE_HALFDUPLEX;

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

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
    writeData(0x77);

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

    fb = (uint16_t *)heap_caps_malloc(TFT_W * TFT_H * 2, MALLOC_CAP_DMA);
    if (!fb) {
      fb = (uint16_t *)malloc(TFT_W * TFT_H * 2);
    }
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
    if (!fb) return;
    fillRect(0, 0, TFT_W, TFT_H, c);
  }

  void drawPixel(int x, int y, uint16_t c) {
    if (!fb) return;
    if (x < 0 || y < 0 || x >= TFT_W || y >= TFT_H) return;
    fb[y * TFT_W + x] = c;
  }

  void drawFastHLine(int x, int y, int w, uint16_t c) {
    if (!fb) return;
    if (y < 0 || y >= TFT_H || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > TFT_W) w = TFT_W - x;
    if (w <= 0) return;
    uint16_t *p = fb + y * TFT_W + x;
    for (int i = 0; i < w; i++) p[i] = c;
  }

  void drawFastVLine(int x, int y, int h, uint16_t c) {
    if (!fb) return;
    if (x < 0 || x >= TFT_W || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > TFT_H) h = TFT_H - y;
    if (h <= 0) return;
    uint16_t *p = fb + y * TFT_W + x;
    for (int i = 0; i < h; i++) {
      *p = c;
      p += TFT_W;
    }
  }

  void drawRect(int x, int y, int w, int h, uint16_t c) {
    drawFastHLine(x, y, w, c);
    drawFastHLine(x, y + h - 1, w, c);
    drawFastVLine(x, y, h, c);
    drawFastVLine(x + w - 1, y, h, c);
  }

  void fillRect(int x, int y, int w, int h, uint16_t c) {
    if (!fb) return;
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > TFT_W) w = TFT_W - x;
    if (y + h > TFT_H) h = TFT_H - y;
    if (w <= 0 || h <= 0) return;
    for (int yy = 0; yy < h; yy++) {
      uint16_t *p = fb + (y + yy) * TFT_W + x;
      for (int xx = 0; xx < w; xx++) p[xx] = c;
    }
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
  void present();

private:
  spi_device_handle_t spi = nullptr;
  uint16_t *fb = nullptr;
  uint8_t rotation = 0;

  void dcLow() { digitalWrite(PIN_TFT_DC, LOW); }
  void dcHigh() { digitalWrite(PIN_TFT_DC, HIGH); }

  void writeCommand(uint8_t c) {
    dcLow();
    spiWriteBytes(&c, 1);
  }

  void writeData(uint8_t d) {
    dcHigh();
    spiWriteBytes(&d, 1);
  }

  void writeData16(uint16_t d) {
    uint8_t b[2] = { (uint8_t)(d >> 8), (uint8_t)(d & 0xFF) };
    dcHigh();
    spiWriteBytes(b, 2);
  }

  void setAddrWindow(int x0, int y0, int x1, int y1) {
    x0 += TFT_X_OFFSET;
    x1 += TFT_X_OFFSET;
    y0 += TFT_Y_OFFSET;
    y1 += TFT_Y_OFFSET;
    writeCommand(0x2A);
    uint8_t colData[4] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
                           (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    dcHigh();
    spiWriteBytes(colData, 4);
    writeCommand(0x2B);
    uint8_t rowData[4] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
                           (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };
    dcHigh();
    spiWriteBytes(rowData, 4);
    writeCommand(0x2C);
  }

  void spiWriteBytes(const uint8_t *data, size_t len) {
    if (!spi || !data || len == 0) return;
    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(spi, &t);
  }

  void pushPixelsDMA(const uint8_t *data, size_t len) {
    if (!spi || !data || len == 0) return;
    const size_t chunk = 4096;
    while (len) {
      size_t n = (len > chunk) ? chunk : len;
      spi_transaction_t t = {};
      t.length = n * 8;
      t.tx_buffer = data;
      dcHigh();
      spi_device_polling_transmit(spi, &t);
      data += n;
      len -= n;
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

static void drawChar8x16(int x, int y, char ch, uint16_t color, uint16_t bg, uint8_t scale) {
  uint8_t c = (uint8_t)ch;
  for (int row = 0; row < 16; row++) {
    uint8_t bits = pgm_read_byte(&font8x16[c][row]);
    for (int col = 0; col < 8; col++) {
      bool on = (bits & (1 << (7 - col))) != 0;
      if (!on && bg == 0xFFFF) continue;
      uint16_t px = on ? color : bg;
      if (scale == 1) {
        tft.drawPixel(x + col, y + row, px);
      } else {
        tft.fillRect(x + col * scale, y + row * scale, scale, scale, px);
      }
    }
  }
}

static void drawText8x16(int x, int y, const char *s, uint16_t color, uint16_t bg, uint8_t scale) {
  while (*s) {
    drawChar8x16(x, y, *s, color, bg, scale);
    x += (8 * scale);
    s++;
  }
}

void TFTILI9163::present() {
  if (!fb) return;
  setAddrWindow(0, 0, TFT_W - 1, TFT_H - 1);
  pushPixelsDMA((const uint8_t *)fb, TFT_W * TFT_H * 2);
}

// --------------------------
// Inputs & Config
// --------------------------
TxConfig cfg;
Preferences prefs;
AxisState steerAxis, throtAxis, suspAxis;

struct PeerEntry {
  uint8_t mac[6];
  uint8_t valid;
};
static PeerEntry peers[MAX_PEERS];
static uint8_t activePeer = 0;
static bool pairingActive = false;
static uint32_t pairingUntil = 0;
static uint8_t lastPairMac[6] = {0};

static int findPeer(const uint8_t *mac) {
  for (int i = 0; i < MAX_PEERS; i++) {
    if (peers[i].valid && macEqual(peers[i].mac, mac)) return i;
  }
  return -1;
}

static int firstFreePeer() {
  for (int i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].valid) return i;
  }
  return -1;
}

struct TelemetryState {
  uint16_t speedKmh = 0;
  int8_t rssi = -127;
  int16_t tempCx10 = 0;
  uint32_t lastMs = 0;
  bool valid = false;
};
static TelemetryState telemetry;

static const uint16_t TEL_MAGIC = 0xBEEF;

struct ModelProfile {
  char name[12];
  TxConfig cfg;
};
static ModelProfile profiles[MAX_PROFILES];
static uint8_t activeProfile = 0;

static ModelConfig model;

struct ChannelOutput {
  int16_t value;
};
static ChannelOutput channels[4];

struct TimerState {
  uint32_t totalMs = 0;
  uint32_t lastUpdate = 0;
  bool running = false;
};
static TimerState driveTimer;
static TimerState sessionTimer;

struct EventLogItem {
  uint32_t ms;
  uint8_t code;
  int16_t data;
};
static EventLogItem eventLog[20];
static uint8_t eventHead = 0;

static inline int16_t applyRateExpo(int16_t v, const RateExpo &re) {
  int16_t out = v;
  if (re.reverse) out = -out;
  out = applyExpo(out, re.expo);
  out = (int16_t)((int32_t)out * re.rate / 100);
  out = clampi(out + re.subtrim, -1000, 1000);
  int16_t minv = re.min ? re.min : -1000;
  int16_t maxv = re.max ? re.max : 1000;
  return clampi(out, minv, maxv);
}

static void initModelDefaults() {
  model.steer = { 90, 35, 0, -1000, 1000, 0 };
  model.throttle = { 90, 30, 0, -1000, 1000, 0 };
  model.aux = { 80, 20, 0, -1000, 1000, 0 };
  for (int i = 0; i < 3; i++) {
    for (int p = 0; p < 9; p++) model.curves[i].points[p] = (int8_t)(-100 + p * 25);
  }
  model.failsafeSteer = 128;
  model.failsafeThrot = 128;
  model.mixSteerToAux = 0;
  model.mixThrotToAux = 0;
  model.chMap[0] = 0;
  model.chMap[1] = 1;
  model.chMap[2] = 2;
  model.chMap[3] = 3;
}

static void logEvent(uint8_t code, int16_t data) {
  eventLog[eventHead].ms = millis();
  eventLog[eventHead].code = code;
  eventLog[eventHead].data = data;
  eventHead = (eventHead + 1) % 20;
}

// --------------------------
// ESP-NOW Packet
// --------------------------
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
enum ScreenId {
  SCREEN_HOME = 0,
  SCREEN_MENU = 1,
  SCREEN_SETTINGS = 2,
  SCREEN_PAIR = 3,
  SCREEN_PROFILES = 4,
  SCREEN_TELEMETRY = 5,
  SCREEN_MODEL = 6,
  SCREEN_RATES = 7,
  SCREEN_OUTPUTS = 8,
  SCREEN_CURVES = 9,
  SCREEN_TIMERS = 10,
  SCREEN_DIAG = 11,
  SCREEN_LOG = 12,
  SCREEN_ADV = 13,
  SCREEN_MIXER = 14,
  SCREEN_NAME = 15,
  SCREEN_CHMAP = 16,
  SCREEN_CAL = 17,
  SCREEN_INFO = 18
};
static ScreenId screen = SCREEN_HOME;
static uint32_t lastFrameMs = 0;
static uint32_t lastMenuFrameMs = 0;
static bool uiDirty = true;
static const uint8_t MENU_ROWS = 7;
static const int UI_X = 6;
static const int UI_W = TFT_W - 12;
static const int UI_SCROLL_X = TFT_W - 4;

static int lastSteerUI = 0;
static int lastThrotUI = 0;
static int lastSuspUI = 0;
static bool lastConnUI = false;
static uint8_t lastModeUI = 255;
static float uiSteer = 0.0f;
static float uiThrot = 0.0f;
static float uiSusp = 0.0f;
static float uiSpeed = 0.0f;

// Calibration state
enum CalStep { CAL_NONE, CAL_CENTER, CAL_STEER_SWEEP, CAL_THROT_SWEEP, CAL_SUSP_SWEEP, CAL_DONE };
static CalStep calStep = CAL_NONE;
static int calMin = 4095, calMax = 0;

// --------------------------
// Colors
// --------------------------
static const uint16_t C_BG_TOP = rgb565(10, 12, 16);
static const uint16_t C_BG_BOT = rgb565(18, 20, 28);
static const uint16_t C_PANEL = rgb565(20, 22, 30);
static const uint16_t C_PANEL2 = rgb565(28, 32, 42);
static const uint16_t C_LINE = rgb565(52, 58, 72);
static const uint16_t C_ACCENT = rgb565(0, 190, 255);
static const uint16_t C_ACCENT2 = rgb565(255, 196, 64);
static const uint16_t C_TEXT = rgb565(232, 236, 242);
static const uint16_t C_MUTED = rgb565(140, 150, 165);
static const uint16_t C_WARN = rgb565(255, 90, 90);
static const uint16_t C_SHADOW = rgb565(6, 7, 10);
static const uint16_t C_HILITE = rgb565(72, 80, 96);
static const uint16_t C_GLOW = rgb565(120, 220, 255);

static void advActionSave();
static void advActionReset();
static void advActionBeep();

static const char *optDriveMode[] = { "RAW" };
static const char *optInvert[] = { "OFF", "ON" };
static const char *optGyro[] = { "OFF", "ON" };
static const char *optLightMode[] = { "OFF", "LOW", "HIGH" };
static const char *optIndMode[] = { "OFF", "AUTO", "MAN" };
static const char *optIndManual[] = { "OFF", "LEFT", "RIGHT", "HAZ" };

static int16_t advDriveMode;
static int16_t advInvertSteer;
static int16_t advInvertThrot;
static int16_t advSendHz;
static int16_t advMaxKmh;
static int16_t advSteerRate;
static int16_t advThrotRate;
static int16_t advSteerExpo;
static int16_t advThrotExpo;
static int16_t advSteerSub;
static int16_t advThrotSub;

static MenuItem advItems[] = {
  { "RAW MODE", MENU_ENUM, &advDriveMode, 0, 0, 1, optDriveMode, 1, nullptr },
  { "INV STEER", MENU_ENUM, &advInvertSteer, 0, 1, 1, optInvert, 2, nullptr },
  { "INV THROT", MENU_ENUM, &advInvertThrot, 0, 1, 1, optInvert, 2, nullptr },
  { "SEND HZ", MENU_INT, &advSendHz, 20, 60, 1, nullptr, 0, nullptr },
  { "MAX KMH", MENU_INT, &advMaxKmh, 1, 200, 1, nullptr, 0, nullptr },
  { "STEER RATE", MENU_INT, &advSteerRate, 50, 120, 2, nullptr, 0, nullptr },
  { "THROT RATE", MENU_INT, &advThrotRate, 50, 120, 2, nullptr, 0, nullptr },
  { "STEER EXPO", MENU_INT, &advSteerExpo, 0, 100, 2, nullptr, 0, nullptr },
  { "THROT EXPO", MENU_INT, &advThrotExpo, 0, 100, 2, nullptr, 0, nullptr },
  { "STEER SUB", MENU_INT, &advSteerSub, -200, 200, 2, nullptr, 0, nullptr },
  { "THROT SUB", MENU_INT, &advThrotSub, -200, 200, 2, nullptr, 0, nullptr },
  { "SAVE", MENU_ACTION, nullptr, 0, 0, 0, nullptr, 0, advActionSave },
  { "RESET", MENU_ACTION, nullptr, 0, 0, 0, nullptr, 0, advActionReset },
  { "BEEP TEST", MENU_ACTION, nullptr, 0, 0, 0, nullptr, 0, advActionBeep },
};
static MenuPage advPage = { "ADVANCED", advItems, (uint8_t)(sizeof(advItems) / sizeof(advItems[0])) };

// --------------------------
// Buzzer
// --------------------------
static const uint16_t buzzClickFreq[] = { 1900 };
static const uint16_t buzzClickDur[] = { 18 };
static const uint16_t buzzSelectFreq[] = { 2200, 0, 2600 };
static const uint16_t buzzSelectDur[] = { 20, 20, 30 };
static const uint16_t buzzBackFreq[] = { 2100, 0, 1700 };
static const uint16_t buzzBackDur[] = { 16, 18, 22 };
static const uint16_t buzzAlertFreq[] = { 2800, 0, 2400, 0, 2800 };
static const uint16_t buzzAlertDur[] = { 25, 20, 25, 20, 40 };

static const uint16_t *buzzFreq = nullptr;
static const uint16_t *buzzDur = nullptr;
static uint8_t buzzLen = 0;
static uint8_t buzzIdx = 0;
static uint32_t buzzNextMs = 0;

static void buzzerBegin() {
  if (PIN_BUZZER >= 0) {
    pinMode(PIN_BUZZER, OUTPUT);
    noTone(PIN_BUZZER);
  }
}

static void buzzerSequence(const uint16_t *freqs, const uint16_t *durs, uint8_t len) {
  if (PIN_BUZZER < 0) return;
  buzzFreq = freqs;
  buzzDur = durs;
  buzzLen = len;
  buzzIdx = 0;
  buzzNextMs = 0;
}

static void buzzerClick() { buzzerSequence(buzzClickFreq, buzzClickDur, 1); }
static void buzzerSelect() { buzzerSequence(buzzSelectFreq, buzzSelectDur, 3); }
static void buzzerBack() { buzzerSequence(buzzBackFreq, buzzBackDur, 3); }
static void buzzerAlert() { buzzerSequence(buzzAlertFreq, buzzAlertDur, 5); }

static void buzzerBeep(uint16_t freq, uint16_t ms) {
  static uint16_t singleFreq[1];
  static uint16_t singleDur[1];
  singleFreq[0] = freq;
  singleDur[0] = ms;
  buzzerSequence(singleFreq, singleDur, 1);
}

static void buzzerUpdate() {
  if (buzzLen == 0) return;
  uint32_t now = millis();
  if ((int32_t)(now - buzzNextMs) < 0) return;
  if (buzzIdx >= buzzLen) {
    buzzLen = 0;
    noTone(PIN_BUZZER);
    return;
  }
  uint16_t f = buzzFreq[buzzIdx];
  uint16_t d = buzzDur[buzzIdx];
  buzzIdx++;
  if (f == 0) {
    noTone(PIN_BUZZER);
  } else {
    tone(PIN_BUZZER, f);
  }
  buzzNextMs = now + d;
}

// --------------------------
// Config storage
// --------------------------
static void loadDefaults() {
  cfg.ver = CFG_VERSION;
  cfg.steer = {500, JOY_CENTER, 3500};
  cfg.throttle = {500, JOY_CENTER, 3500};
  cfg.susp = {500, 2048, 3500};
  cfg.trimSteer = 0;
  cfg.trimThrot = 0;
  cfg.driveMode = 0;
  cfg.headlight = 1;
  cfg.taillight = 1;
  cfg.indMode = 1;
  cfg.indManual = 0;
  cfg.expoSteer = 35;
  cfg.expoThrot = 30;
  cfg.rateSteer = 90;
  cfg.rateThrot = 90;
  cfg.sendHz = 50;
  cfg.invertSteer = 0;
  cfg.invertThrot = 0;
  cfg.maxKmh = 45;
}

static void loadConfig() {
  loadDefaults();
  prefs.begin("txcfg", true);
  uint8_t ver = prefs.getUChar("ver", 0);
  if (ver == CFG_VERSION) {
    prefs.getBytes("cfg", &cfg, sizeof(cfg));
  }
  prefs.end();

  prefs.begin("txpeer", true);
  size_t got = prefs.getBytesLength("peers");
  if (got == sizeof(peers)) {
    prefs.getBytes("peers", peers, sizeof(peers));
  } else {
    memset(peers, 0, sizeof(peers));
  }
  activePeer = prefs.getUChar("active", 0);
  if (activePeer >= MAX_PEERS) activePeer = 0;
  prefs.end();

  // Force RAW mode
  cfg.driveMode = 0;

  prefs.begin("txprof", true);
  size_t gotp = prefs.getBytesLength("profiles");
  if (gotp == sizeof(profiles)) {
    prefs.getBytes("profiles", profiles, sizeof(profiles));
  } else {
    const char *names[MAX_PROFILES] = { "Street", "Trail", "Race", "Crawl", "Bash", "Spare" };
    for (int i = 0; i < MAX_PROFILES; i++) {
      memset(&profiles[i], 0, sizeof(ModelProfile));
      strncpy(profiles[i].name, names[i], sizeof(profiles[i].name) - 1);
      profiles[i].cfg = cfg;
    }
  }
  activeProfile = prefs.getUChar("active", 0);
  if (activeProfile >= MAX_PROFILES) activeProfile = 0;
  prefs.end();

  prefs.begin("txmodel", true);
  size_t gotm = prefs.getBytesLength("model");
  if (gotm == sizeof(model)) {
    prefs.getBytes("model", &model, sizeof(model));
  } else {
    initModelDefaults();
  }
  prefs.end();
}

static void saveConfig() {
  prefs.begin("txcfg", false);
  prefs.putUChar("ver", CFG_VERSION);
  prefs.putBytes("cfg", &cfg, sizeof(cfg));
  prefs.end();
}

static void savePeers() {
  prefs.begin("txpeer", false);
  prefs.putBytes("peers", peers, sizeof(peers));
  prefs.putUChar("active", activePeer);
  prefs.end();
}

static void saveProfiles() {
  prefs.begin("txprof", false);
  prefs.putBytes("profiles", profiles, sizeof(profiles));
  prefs.putUChar("active", activeProfile);
  prefs.end();
}

static void saveModel() {
  prefs.begin("txmodel", false);
  prefs.putBytes("model", &model, sizeof(model));
  prefs.end();
}

static void loadProfile(uint8_t idx) {
  if (idx >= MAX_PROFILES) return;
  cfg = profiles[idx].cfg;
  activeProfile = idx;
  saveProfiles();
  saveConfig();
}

static void saveProfile(uint8_t idx) {
  if (idx >= MAX_PROFILES) return;
  profiles[idx].cfg = cfg;
  saveProfiles();
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

static int readAnalogAvg(int pin, int samples) {
  if (pin < 0) return 0;
  if (samples < 1) samples = 1;
  int v = 0;
  for (int i = 0; i < samples; i++) v += analogRead(pin);
  return v / samples;
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

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac_addr = info ? info->src_addr : nullptr;
#else
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
#endif
  if (data && len >= (int)sizeof(TelemetryPacket)) {
    TelemetryPacket pkt;
    memcpy(&pkt, data, sizeof(TelemetryPacket));
    if (pkt.magic == TEL_MAGIC) {
      telemetry.speedKmh = pkt.speedKmh;
      telemetry.rssi = pkt.rssi;
      telemetry.tempCx10 = pkt.tempCx10;
      telemetry.lastMs = millis();
      telemetry.valid = true;
      logEvent(2, pkt.rssi);
    }
  }

  if (!pairingActive || !mac_addr) return;
  if (findPeer(mac_addr) >= 0) {
    pairingActive = false;
    return;
  }
  int slot = firstFreePeer();
  if (slot < 0) return;
  memcpy(peers[slot].mac, mac_addr, 6);
  peers[slot].valid = 1;
  activePeer = slot;
  memcpy(lastPairMac, mac_addr, 6);
  savePeers();
  pairingActive = false;
  buzzerSelect();
  if (screen == SCREEN_PAIR) renderPairMenu();
  logEvent(1, slot);
}

static void addEspNowPeer(const uint8_t *mac) {
  if (!mac) return;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

static void espnowInit() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    buzzerAlert();
    return;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  addEspNowPeer(ESPNOW_BROADCAST);
  for (int i = 0; i < MAX_PEERS; i++) {
    if (peers[i].valid) addEspNowPeer(peers[i].mac);
  }
}

// --------------------------
// UI Rendering
// --------------------------
static inline uint16_t scale565(uint16_t c, uint8_t scale) {
  uint16_t c0 = (uint16_t)((c << 8) | (c >> 8));
  uint8_t r = (uint8_t)(((c0 >> 11) & 0x1F) * 255 / 31);
  uint8_t g = (uint8_t)(((c0 >> 5) & 0x3F) * 255 / 63);
  uint8_t b = (uint8_t)((c0 & 0x1F) * 255 / 31);
  r = (uint8_t)((r * scale) / 255);
  g = (uint8_t)((g * scale) / 255);
  b = (uint8_t)((b * scale) / 255);
  return rgb565(r, g, b);
}

static inline uint8_t pulse8(uint32_t now, uint16_t periodMs) {
  if (periodMs == 0) return 0;
  float phase = (float)(now % periodMs) / (float)periodMs;
  float s = 0.5f + 0.5f * sinf(phase * 6.283185f);
  int v = (int)(s * 255.0f);
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

static void fillGradientRect(int x, int y, int w, int h, uint16_t top, uint16_t bottom) {
  if (w <= 0 || h <= 0) return;
  for (int i = 0; i < h; i++) {
    uint8_t t = (h == 1) ? 0 : (uint8_t)((i * 255) / (h - 1));
    tft.drawFastHLine(x, y + i, w, mix565(top, bottom, t));
  }
}

static inline int textWidth5x7(const char *s, uint8_t size) {
  return (int)strlen(s) * 6 * size;
}

static inline int textWidth8x16(const char *s, uint8_t scale) {
  return (int)strlen(s) * 8 * scale;
}

static void drawText8x16Shadow(int x, int y, const char *s, uint16_t col) {
  drawText8x16(x + 1, y + 1, s, C_SHADOW, 0xFFFF, 1);
  drawText8x16(x, y, s, col, 0xFFFF, 1);
}

static void drawText5x7Shadow(int x, int y, const char *s, uint16_t col) {
  tft.drawText(x + 1, y + 1, s, C_SHADOW, 0xFFFF, 1);
  tft.drawText(x, y, s, col, 0xFFFF, 1);
}
static void drawBackground(uint32_t now) {
  for (int y = 0; y < TFT_H; y++) {
    uint8_t t = (uint8_t)((y * 255) / (TFT_H - 1));
    tft.drawFastHLine(0, y, TFT_W, mix565(C_BG_TOP, C_BG_BOT, t));
  }

  int y = 18 + ((now / 30) % (TFT_H - 22));
  tft.drawFastHLine(0, y, TFT_W, scale565(C_HILITE, 120));
  tft.drawFastHLine(0, y + 1, TFT_W, scale565(C_HILITE, 60));

  fillGradientRect(0, TFT_H - 14, TFT_W, 14, C_PANEL2, C_BG_BOT);
}

static void drawSignalBars(int x, int y, int bars, uint16_t col) {
  for (int i = 0; i < 4; i++) {
    int h = 2 + i * 2;
    uint16_t c = (i < bars) ? col : C_LINE;
    tft.fillRect(x + i * 4, y + (8 - h), 3, h, c);
  }
}

static void drawTitleClamped(int x, int y, const char *title, int maxChars) {
  if (maxChars <= 0 || !title) return;
  char buf[20];
  int safeMax = maxChars;
  if (safeMax > (int)sizeof(buf) - 1) safeMax = (int)sizeof(buf) - 1;
  int len = (int)strlen(title);
  if (len > safeMax) {
    int cut = safeMax - 1;
    if (cut < 1) cut = 1;
    strncpy(buf, title, (size_t)cut);
    buf[cut] = '\0';
    if (cut > 1) buf[cut - 1] = '.';
  } else {
    strncpy(buf, title, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
  }
  drawText8x16Shadow(x, y, buf, C_TEXT);
}

static void drawTopBar(const char *title, bool connected, bool gyroOn, uint32_t now) {
  uint8_t pulse = pulse8(now, 1600);
  tft.fillRect(0, 0, TFT_W, 18, C_PANEL2);
  tft.drawFastHLine(0, 17, TFT_W, C_LINE);

  int dotX = TFT_W - 10;
  int dotY = 5;
  uint16_t dotCol = connected ? mix565(C_ACCENT, C_GLOW, pulse) : C_WARN;
  tft.fillRect(dotX, dotY, 6, 6, dotCol);
  tft.drawRect(dotX - 1, dotY - 1, 8, 8, C_LINE);

  int badgeX = dotX - 22;
  tft.drawRect(badgeX, 3, 18, 12, gyroOn ? C_ACCENT : C_LINE);
  tft.drawText(badgeX + 3, 5, gyroOn ? "GYR" : "OFF", gyroOn ? C_ACCENT : C_MUTED, 0xFFFF, 1);

  int bars = 0;
  if (telemetry.valid && (millis() - telemetry.lastMs) < 800) {
    int r = telemetry.rssi;
    if (r > -60) bars = 4;
    else if (r > -70) bars = 3;
    else if (r > -80) bars = 2;
    else if (r > -90) bars = 1;
  }
  drawSignalBars(badgeX - 20, 4, bars, C_ACCENT2);

  int maxChars = (badgeX - 10) / 8;
  drawTitleClamped(6, 1, title, maxChars);
}

static inline void drawScreenHeader(const char *title, uint32_t now) {
  bool connected = telemetry.valid && ((now - telemetry.lastMs) < 1000);
  bool gyroOn = false;
  if (PIN_SW_GYRO >= 0) {
    gyroOn = SW_GYRO_ACTIVE_LOW ? (digitalRead(PIN_SW_GYRO) == LOW)
                                : (digitalRead(PIN_SW_GYRO) == HIGH);
  }
  drawTopBar(title, connected, gyroOn, now);
}

static void drawCard(int x, int y, int w, int h, const char *title, uint16_t accent) {
  tft.fillRect(x, y, w, h, C_PANEL);
  tft.drawRect(x, y, w, h, C_LINE);
  if (accent) tft.fillRect(x, y, 3, h, accent);
  if (title) tft.drawText(x + 6, y + 2, title, C_MUTED, 0xFFFF, 1);
}

static void drawListItem(int x, int y, int w, int h, bool sel, bool edit,
                         const char *label, const char *value, uint32_t now) {
  uint16_t bg = C_PANEL;
  uint16_t fg = C_TEXT;
  if (sel) {
    uint8_t pulse = pulse8(now, 1600);
    uint16_t a = edit ? C_ACCENT2 : C_ACCENT;
    uint16_t b = edit ? C_ACCENT : C_ACCENT2;
    bg = mix565(a, b, pulse);
    fg = C_BG_TOP;
  }
  tft.fillRect(x, y, w, h, bg);
  tft.drawRect(x, y, w, h, sel ? bg : C_LINE);
  tft.drawText(x + 6, y + 2, label, fg, 0xFFFF, 1);
  if (value && value[0]) {
    int vx = x + w - 6 - (int)strlen(value) * 6;
    tft.drawText(vx, y + 2, value, fg, 0xFFFF, 1);
  }
}

static void drawCenterBar(int x, int y, int w, int h, int v, uint16_t colPos, uint16_t colNeg) {
  tft.fillRect(x, y, w, h, C_LINE);
  int mid = x + w / 2;
  tft.drawFastVLine(mid, y, h, C_PANEL2);
  int half = w / 2;
  int fill = (abs(v) * half) / 1000;
  if (fill > half) fill = half;
  if (v >= 0) tft.fillRect(mid, y, fill, h, colPos);
  else tft.fillRect(mid - fill, y, fill, h, colNeg);
}

static void drawProgressBar(int x, int y, int w, int h, int v, int vmax, uint16_t col) {
  tft.fillRect(x, y, w, h, C_LINE);
  int fill = (vmax > 0) ? (v * w) / vmax : 0;
  if (fill < 0) fill = 0;
  if (fill > w) fill = w;
  tft.fillRect(x, y, fill, h, col);
}

static void renderHome() {
  uint32_t now = millis();
  bool connected = telemetry.valid && ((now - telemetry.lastMs) < 1000);
  bool gyroOn = false;
  if (PIN_SW_GYRO >= 0) {
    gyroOn = SW_GYRO_ACTIVE_LOW ? (digitalRead(PIN_SW_GYRO) == LOW)
                                : (digitalRead(PIN_SW_GYRO) == HIGH);
  }

  uint16_t speedSrc = 0;
  if (telemetry.valid && (now - telemetry.lastMs) < 800) {
    speedSrc = telemetry.speedKmh;
  } else {
    speedSrc = 0;
  }

  uiSteer = lerpf(uiSteer, steerAxis.value, 0.2f);
  uiThrot = lerpf(uiThrot, throtAxis.value, 0.2f);
  uiSusp = lerpf(uiSusp, suspAxis.value, 0.2f);
  uiSpeed = lerpf(uiSpeed, speedSrc, 0.18f);

  drawBackground(now);
  drawTopBar(profiles[activeProfile].name, connected, gyroOn, now);

  char buf[24];
  tft.drawText(UI_X, 22, "SPEED", C_MUTED, 0xFFFF, 1);
  snprintf(buf, sizeof(buf), "%u", (uint16_t)uiSpeed);
  int tw = textWidth8x16(buf, 1);
  int tx = UI_X + (UI_W - tw) / 2;
  drawText8x16(tx, 34, buf, C_TEXT, 0xFFFF, 1);
  tft.drawText(UI_X + UI_W - 30, 38, "KM/H", C_MUTED, 0xFFFF, 1);
  drawProgressBar(UI_X, 52, UI_W, 4, (int)uiSpeed, cfg.maxKmh, C_ACCENT2);

  tft.drawText(UI_X, 60, "THR", C_MUTED, 0xFFFF, 1);
  snprintf(buf, sizeof(buf), "%d%%", (int)((uiThrot * 100) / 1000));
  tft.drawText(UI_X + UI_W - (int)strlen(buf) * 6, 60, buf, C_MUTED, 0xFFFF, 1);
  drawCenterBar(UI_X, 70, UI_W, 4, (int)uiThrot, C_ACCENT, C_ACCENT2);

  tft.drawText(UI_X, 78, "STR", C_MUTED, 0xFFFF, 1);
  snprintf(buf, sizeof(buf), "%d%%", (int)((uiSteer * 100) / 1000));
  tft.drawText(UI_X + UI_W - (int)strlen(buf) * 6, 78, buf, C_MUTED, 0xFFFF, 1);
  drawCenterBar(UI_X, 88, UI_W, 4, (int)uiSteer, C_ACCENT, C_ACCENT2);

  if (telemetry.valid && (now - telemetry.lastMs) < 800) {
    int16_t tc = telemetry.tempCx10;
    snprintf(buf, sizeof(buf), "TEMP %d.%dC  RSSI %ddBm", tc / 10, abs(tc % 10), telemetry.rssi);
  } else {
    snprintf(buf, sizeof(buf), "TEMP --.-C  NO LINK");
  }
  tft.drawText(UI_X, 98, buf, C_TEXT, 0xFFFF, 1);

  uint32_t d = driveTimer.totalMs / 1000;
  uint32_t s = sessionTimer.totalMs / 1000;
  char tbuf[20];
  snprintf(tbuf, sizeof(tbuf), "D %02lu:%02lu  S %02lu:%02lu",
           (unsigned long)(d / 60), (unsigned long)(d % 60),
           (unsigned long)(s / 60), (unsigned long)(s % 60));
  tft.drawText(UI_X, 110, tbuf, C_MUTED, 0xFFFF, 1);

  tft.drawText(UI_X, 122, "SUSP", C_MUTED, 0xFFFF, 1);
  drawCenterBar(UI_X + 24, 124, UI_W - 24, 4, (int)uiSusp, C_ACCENT2, C_ACCENT);
  snprintf(buf, sizeof(buf), "TRIM S%+d T%+d", cfg.trimSteer, cfg.trimThrot);
  tft.drawText(UI_X, 134, buf, C_TEXT, 0xFFFF, 1);

  tft.present();
}
static const char *mainMenuItems[] = {
  "DASHBOARD", "SETTINGS", "PAIR RX", "PROFILES", "MODEL SETUP",
  "RATES/EXPO", "OUTPUTS", "CURVES", "MIXER", "NAME EDIT",
  "CH MAP", "TIMERS", "TELEMETRY", "DIAGNOSTICS", "EVENT LOG",
  "ADVANCED", "CALIBRATE", "INFO"
};
static const uint8_t MAIN_MENU_COUNT = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
static uint8_t mainMenuIndex = 0;
static uint8_t mainMenuTop = 0;

static const char *settingsItems[] = {
  "TRIM STEER", "TRIM THROT", "RAW MODE", "HEADLIGHTS", "TAILLIGHTS",
  "IND MODE", "IND MANUAL", "EXPO STEER", "EXPO THROT",
  "RATE STEER", "RATE THROT", "SEND HZ", "MAX KMH",
  "INV STEER", "INV THROT", "SAVE"
};
static const uint8_t SETTINGS_COUNT = sizeof(settingsItems) / sizeof(settingsItems[0]);
static uint8_t settingsIndex = 0;
static uint8_t settingsTop = 0;
static bool settingsEdit = false;

static uint8_t pairIndex = 0;
static uint8_t profileIndex = 0;
static uint8_t mainMenuPage = 0;
static uint8_t ratesIndex = 0;
static uint8_t outputIndex = 0;
static uint8_t curveIndex = 0;
static uint8_t timerIndex = 0;
static bool advEdit = false;
static uint8_t advIndex = 0;
static uint8_t advTop = 0;
static uint8_t nameCharIndex = 0;
static bool nameEdit = false;
static uint8_t chMapIndex = 0;
static uint8_t mixerIndex = 0;
static bool chMapEdit = false;

static void renderMainMenu() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("MAIN MENU", now);

  if (mainMenuIndex < mainMenuTop) mainMenuTop = mainMenuIndex;
  if (mainMenuIndex > mainMenuTop + (MENU_ROWS - 1)) mainMenuTop = mainMenuIndex - (MENU_ROWS - 1);

  for (uint8_t i = 0; i < MENU_ROWS; i++) {
    uint8_t idx = mainMenuTop + i;
    if (idx >= MAIN_MENU_COUNT) break;
    int y = 24 + i * 16;
    bool sel = (idx == mainMenuIndex);
    drawListItem(UI_X, y, UI_W, 14, sel, false, mainMenuItems[idx], ">", now);
  }
  if (MAIN_MENU_COUNT > MENU_ROWS) {
    int barX = UI_SCROLL_X;
    int barY = 24;
    int barH = 16 * MENU_ROWS - 2;
    tft.drawFastVLine(barX, barY, barH, C_LINE);
    int thumbH = (barH * MENU_ROWS) / MAIN_MENU_COUNT;
    if (thumbH < 10) thumbH = 10;
    int thumbY = barY + ((barH - thumbH) * mainMenuTop) / (MAIN_MENU_COUNT - MENU_ROWS);
    tft.fillRect(barX - 1, thumbY, 3, thumbH, C_ACCENT2);
  }
  drawCard(UI_X, 138, UI_W, 18, nullptr, 0);
  tft.drawText(10, 142, "SET=ENTER MENU=BACK", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderSettings() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("SETTINGS", now);

  if (settingsIndex < settingsTop) settingsTop = settingsIndex;
  if (settingsIndex > settingsTop + (MENU_ROWS - 1)) settingsTop = settingsIndex - (MENU_ROWS - 1);

  char buf[16];
  for (uint8_t i = 0; i < MENU_ROWS; i++) {
    uint8_t idx = settingsTop + i;
    if (idx >= SETTINGS_COUNT) break;
    const char *val = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%d", cfg.trimSteer); val = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%d", cfg.trimThrot); val = buf; }
    else if (idx == 2) { val = "RAW"; }
    else if (idx == 3) { val = optLightMode[cfg.headlight % 3]; }
    else if (idx == 4) { val = optLightMode[cfg.taillight % 3]; }
    else if (idx == 5) { val = optIndMode[cfg.indMode % 3]; }
    else if (idx == 6) { val = optIndManual[cfg.indManual % 4]; }
    else if (idx == 7) { snprintf(buf, sizeof(buf), "%u", cfg.expoSteer); val = buf; }
    else if (idx == 8) { snprintf(buf, sizeof(buf), "%u", cfg.expoThrot); val = buf; }
    else if (idx == 9) { snprintf(buf, sizeof(buf), "%u", cfg.rateSteer); val = buf; }
    else if (idx == 10) { snprintf(buf, sizeof(buf), "%u", cfg.rateThrot); val = buf; }
    else if (idx == 11) { snprintf(buf, sizeof(buf), "%u", cfg.sendHz); val = buf; }
    else if (idx == 12) { snprintf(buf, sizeof(buf), "%u", cfg.maxKmh); val = buf; }
    else if (idx == 13) { val = cfg.invertSteer ? "ON" : "OFF"; }
    else if (idx == 14) { val = cfg.invertThrot ? "ON" : "OFF"; }
    else if (idx == 15) { val = ">"; }
    int y = 24 + i * 16;
    bool sel = (idx == settingsIndex);
    drawListItem(UI_X, y, UI_W, 14, sel, settingsEdit && sel, settingsItems[idx], val, now);
  }
  if (SETTINGS_COUNT > MENU_ROWS) {
    int barX = UI_SCROLL_X;
    int barY = 24;
    int barH = 16 * MENU_ROWS - 2;
    tft.drawFastVLine(barX, barY, barH, C_LINE);
    int thumbH = (barH * MENU_ROWS) / SETTINGS_COUNT;
    if (thumbH < 10) thumbH = 10;
    int thumbY = barY + ((barH - thumbH) * settingsTop) / (SETTINGS_COUNT - MENU_ROWS);
    tft.fillRect(barX - 1, thumbY, 3, thumbH, C_ACCENT2);
  }
  drawCard(UI_X, 138, UI_W, 18, nullptr, 0);
  tft.drawText(10, 142, settingsEdit ? "TRIM=ADJ MENU=BACK" : "SET=EDIT MENU=BACK", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderPairMenu() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("PAIR RX", now);

  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    char label[16];
    snprintf(label, sizeof(label), "RX %u", i + 1);

    const char *val = peers[i].valid ? (i == activePeer ? "ACTIVE" : "PAIRED") : "EMPTY";
    int y = 24 + i * 18;
    bool sel = (i == pairIndex);
    drawListItem(UI_X, y, UI_W, 14, sel, false, label, val, now);
  }

  if (pairingActive) {
    drawCard(UI_X, 128, UI_W, 16, nullptr, 0);
    tft.drawText(10, 132, "LISTENING...", C_TEXT, 0xFFFF, 1);
  } else {
    drawCard(UI_X, 128, UI_W, 16, nullptr, 0);
    tft.drawText(10, 132, "SET=PAIR HOLD=DEL", C_MUTED, 0xFFFF, 1);
  }
  tft.present();
}

static void renderProfiles() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("PROFILES", now);
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
    int y = 24 + i * 18;
    bool sel = (i == profileIndex);
    const char *val = (i == activeProfile) ? "ACTIVE" : "";
    drawListItem(UI_X, y, UI_W, 14, sel, false, profiles[i].name, val, now);
  }
  drawCard(UI_X, 140, UI_W, 16, nullptr, 0);
  tft.drawText(10, 144, "SET=LOAD HOLD=SAVE", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderTelemetry() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("TELEMETRY", now);
  char buf[16];
  uint32_t age = telemetry.valid ? (millis() - telemetry.lastMs) : 9999;

  drawCard(UI_X, 24, UI_W, 34, "SPEED", C_ACCENT2);
  snprintf(buf, sizeof(buf), "%u", telemetry.speedKmh);
  int tw = textWidth8x16(buf, 1);
  int tx = UI_X + (UI_W - tw) / 2;
  drawText8x16(tx, 38, buf, C_TEXT, 0xFFFF, 1);
  tft.drawText(UI_X + UI_W - 30, 44, "KM/H", C_MUTED, 0xFFFF, 1);

  drawCard(UI_X, 62, UI_W, 34, "RSSI", C_ACCENT);
  if (telemetry.valid && (millis() - telemetry.lastMs) < 800) {
    snprintf(buf, sizeof(buf), "%d", telemetry.rssi);
  } else {
    strncpy(buf, "--", sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
  }
  tw = textWidth8x16(buf, 1);
  tx = UI_X + (UI_W - tw) / 2;
  drawText8x16(tx, 76, buf, C_TEXT, 0xFFFF, 1);
  tft.drawText(UI_X + UI_W - 26, 82, "DBM", C_MUTED, 0xFFFF, 1);

  drawCard(UI_X, 100, UI_W, 28, "STATUS", 0);
  snprintf(buf, sizeof(buf), "AGE %lums", (unsigned long)age);
  tft.drawText(UI_X + 6, 112, buf, C_TEXT, 0xFFFF, 1);
  int16_t tc = telemetry.tempCx10;
  snprintf(buf, sizeof(buf), "TEMP %d.%dC", tc / 10, abs(tc % 10));
  tft.drawText(UI_X + 6, 124, buf, C_MUTED, 0xFFFF, 1);

  drawCard(UI_X, 132, UI_W, 22, "LINK", 0);
  int bars = 0;
  if (telemetry.valid && (millis() - telemetry.lastMs) < 800) {
    int r = telemetry.rssi;
    if (r > -60) bars = 4;
    else if (r > -70) bars = 3;
    else if (r > -80) bars = 2;
    else if (r > -90) bars = 1;
  }
  drawSignalBars(UI_X + 6, 140, bars, C_ACCENT2);
  snprintf(buf, sizeof(buf), "%uhz", (unsigned)cfg.sendHz);
  tft.drawText(UI_X + 48, 140, buf, C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderModelSetup() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("MODEL SETUP", now);
  drawCard(UI_X, 24, UI_W, 28, "MODEL", C_ACCENT2);
  drawText8x16Shadow(12, 38, profiles[activeProfile].name, C_TEXT);

  drawCard(UI_X, 58, UI_W, 28, "FAILSAFE", C_ACCENT);
  char buf[16];
  snprintf(buf, sizeof(buf), "S:%u  T:%u", model.failsafeSteer, model.failsafeThrot);
  tft.drawText(12, 70, buf, C_TEXT, 0xFFFF, 1);

  drawCard(UI_X, 92, UI_W, 36, "ACTIONS", 0);
  tft.drawText(12, 108, "NAME=SET", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 122, "SAVE=HOLD", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderRatesExpo() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("RATES / EXPO", now);
  const char *items[] = { "STEER", "THROTTLE", "AUX" };
  for (int i = 0; i < 3; i++) {
    int y = 28 + i * 36;
    bool sel = (i == ratesIndex);
    RateExpo *re = (i == 0) ? &model.steer : (i == 1) ? &model.throttle : &model.aux;
    char buf[16];
    snprintf(buf, sizeof(buf), "R%u  E%u", re->rate, re->expo);
    drawListItem(UI_X, y, UI_W, 26, sel, false, items[i], buf, now);
  }
  tft.present();
}

static void renderOutputs() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("OUTPUTS", now);
  const char *ch[] = { "CH1", "CH2", "CH3", "CH4" };
  for (int i = 0; i < 4; i++) {
    int y = 26 + i * 24;
    bool sel = (i == outputIndex);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", channels[i].value);
    drawListItem(UI_X, y, UI_W, 18, sel, false, ch[i], buf, now);
  }
  tft.present();
}

static void renderCurves() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("CURVES", now);
  drawCard(UI_X, 24, UI_W, 70, "CURVE", C_ACCENT2);
  int cx = 12, cy = 38, w = 96, h = 50;
  tft.drawRect(cx, cy, w, h, C_LINE);
  Curve &c = model.curves[curveIndex];
  for (int i = 0; i < 8; i++) {
    int x0 = cx + (i * (w - 2)) / 8;
    int y0 = cy + h/2 - (c.points[i] * (h/2-2)) / 100;
    int x1 = cx + ((i+1) * (w - 2)) / 8;
    int y1 = cy + h/2 - (c.points[i+1] * (h/2-2)) / 100;
    tft.drawLine(x0, y0, x1, y1, C_ACCENT);
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "C%u", curveIndex + 1);
  tft.drawText(12, 30, buf, C_MUTED, 0xFFFF, 1);
  drawCard(UI_X, 98, UI_W, 28, "ACTION", 0);
  tft.drawText(12, 110, "SET=NEXT", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderTimers() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("TIMERS", now);
  uint32_t d = driveTimer.totalMs / 1000;
  uint32_t s = sessionTimer.totalMs / 1000;
  char buf[16];
  drawCard(UI_X, 24, UI_W, 32, "DRIVE", C_ACCENT2);
  snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(d/60), (unsigned long)(d%60));
  drawText8x16(20, 38, buf, C_TEXT, 0xFFFF, 1);
  drawCard(UI_X, 60, UI_W, 32, "SESSION", C_ACCENT);
  snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(s/60), (unsigned long)(s%60));
  drawText8x16(20, 74, buf, C_TEXT, 0xFFFF, 1);
  drawCard(UI_X, 98, UI_W, 28, "ACTION", 0);
  tft.drawText(12, 110, "SET=RESET", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderDiagnostics() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("DIAGNOSTICS", now);
  char buf[16];

  snprintf(buf, sizeof(buf), "%d", steerAxis.raw);
  drawListItem(UI_X, 24, UI_W, 16, false, false, "STEER RAW", buf, now);
  snprintf(buf, sizeof(buf), "%d", throtAxis.raw);
  drawListItem(UI_X, 44, UI_W, 16, false, false, "THROT RAW", buf, now);
  snprintf(buf, sizeof(buf), "%d", suspAxis.raw);
  drawListItem(UI_X, 64, UI_W, 16, false, false, "SUSP RAW", buf, now);
  snprintf(buf, sizeof(buf), "%u", (unsigned)(1000 / UI_MENU_MS));
  drawListItem(UI_X, 84, UI_W, 16, false, false, "FPS", buf, now);
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)esp_get_free_heap_size());
  drawListItem(UI_X, 104, UI_W, 16, false, false, "HEAP", buf, now);
  tft.present();
}

static void renderEventLog() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("EVENT LOG", now);
  for (int i = 0; i < 6; i++) {
    int idx = (eventHead + 19 - i) % 20;
    EventLogItem &e = eventLog[idx];
    char buf[18];
    snprintf(buf, sizeof(buf), "%lu C%u", (unsigned long)e.ms, e.code);
    int y = 24 + i * 18;
    drawListItem(UI_X, y, UI_W, 14, false, false, buf, "", now);
  }
  tft.present();
}

static void renderMixer() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("MIXER", now);
  const char *items[] = { "STEER -> AUX", "THROT -> AUX" };
  int8_t *vals[] = { &model.mixSteerToAux, &model.mixThrotToAux };
  for (int i = 0; i < 2; i++) {
    int y = 34 + i * 26;
    bool sel = (i == mixerIndex);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)*vals[i]);
    drawListItem(UI_X, y, UI_W, 18, sel, false, items[i], buf, now);
  }
  drawCard(UI_X, 98, UI_W, 28, "ACTION", 0);
  tft.drawText(12, 110, "SET=SAVE", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static const char nameChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

static void renderNameEdit() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("NAME EDIT", now);
  drawCard(UI_X, 24, UI_W, 28, "PROFILE", C_ACCENT2);
  int nameX = 12;
  int nameY = 38;
  tft.drawText(nameX, nameY, profiles[activeProfile].name, C_TEXT, 0xFFFF, 1);
  int x = nameX + nameCharIndex * 6;
  tft.drawFastHLine(x, nameY + 10, 6, nameEdit ? C_ACCENT2 : C_ACCENT);

  drawCard(UI_X, 58, UI_W, 84, "CONTROLS", 0);
  tft.drawText(12, 76, nameEdit ? "EDIT CHAR" : "MOVE CURSOR", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 92, "SET=TOGGLE", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 108, "MENU=BACK", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderChMap() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("CHANNEL MAP", now);
  const char *src[] = { "STEER", "THROT", "AUX", "NONE" };
  for (int i = 0; i < 4; i++) {
    int y = 28 + i * 24;
    bool sel = (i == chMapIndex);
    char label[8];
    snprintf(label, sizeof(label), "CH%u", i + 1);
    uint8_t v = model.chMap[i];
    if (v > 3) v = 3;
    drawListItem(UI_X, y, UI_W, 18, sel, chMapEdit && sel, label, src[v], now);
  }
  drawCard(UI_X, 124, UI_W, 28, "ACTION", 0);
  tft.drawText(12, 136, "SET=SAVE", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void advDrawTitle(const char *title) {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader(title, now);
}

static void advDrawRow(int y, bool sel, const char *label, const char *value, bool edit) {
  uint32_t now = millis();
  drawListItem(UI_X, y + 2, UI_W, 14, sel, edit, label, value, now);
}

static void advSyncFromCfg() {
  advDriveMode = 0;
  advInvertSteer = cfg.invertSteer;
  advInvertThrot = cfg.invertThrot;
  advSendHz = cfg.sendHz;
  advMaxKmh = cfg.maxKmh;
  advSteerRate = cfg.rateSteer;
  advThrotRate = cfg.rateThrot;
  advSteerExpo = cfg.expoSteer;
  advThrotExpo = cfg.expoThrot;
  advSteerSub = model.steer.subtrim;
  advThrotSub = model.throttle.subtrim;
}

static void advApplyToCfg() {
  cfg.driveMode = 0;
  cfg.invertSteer = (uint8_t)advInvertSteer;
  cfg.invertThrot = (uint8_t)advInvertThrot;
  cfg.sendHz = (uint8_t)advSendHz;
  cfg.maxKmh = (uint16_t)advMaxKmh;
  cfg.rateSteer = (uint8_t)advSteerRate;
  cfg.rateThrot = (uint8_t)advThrotRate;
  cfg.expoSteer = (uint8_t)advSteerExpo;
  cfg.expoThrot = (uint8_t)advThrotExpo;
  model.steer.subtrim = advSteerSub;
  model.throttle.subtrim = advThrotSub;
}

static void advActionSave() {
  advApplyToCfg();
  saveConfig();
  saveModel();
  buzzerSelect();
}

static void advActionReset() {
  loadDefaults();
  initModelDefaults();
  advSyncFromCfg();
  buzzerAlert();
}

static void advActionBeep() {
  buzzerBeep(2800, 30);
  delay(35);
  buzzerBeep(3000, 40);
}

static void renderAdvanced() {
  menuRender(advPage, advIndex, advTop, advEdit, advDrawTitle, advDrawRow);
  tft.present();
}
static void renderInfo() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("ABOUT", now);
  drawCard(UI_X, 24, UI_W, 54, "DEVICE", C_ACCENT2);
  tft.drawText(12, 42, "OpenTX ESP32", C_TEXT, 0xFFFF, 1);
  tft.drawText(12, 58, "ESP-NOW TX", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 72, "DMA TFT", C_MUTED, 0xFFFF, 1);
  drawCard(UI_X, 82, UI_W, 38, "FIRMWARE", C_ACCENT);
  tft.drawText(12, 98, "v1.0", C_TEXT, 0xFFFF, 1);
  tft.present();
}

static void renderSplash() {
  for (int i = 0; i < 28; i++) {
    uint32_t now = millis();
    drawBackground(now);
    drawCard(UI_X + 2, 38, 104, 60, nullptr, C_ACCENT2);
    drawText8x16Shadow(30, 50, "OPEN", C_TEXT);
    drawText8x16Shadow(46, 66, "TX", C_ACCENT2);
    tft.drawText(26, 84, "ESP32 RADIO", C_MUTED, 0xFFFF, 1);
    int w = (i * 88) / 27;
    uint16_t barCol = mix565(C_ACCENT2, C_GLOW, pulse8(now, 1200));
    tft.fillRect(16, 106, 88, 6, C_LINE);
    tft.fillRect(16, 106, w, 6, barCol);
    tft.present();
    delay(14);
  }
}

static void renderCalScreen() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("CALIBRATION", now);
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
  drawCard(UI_X, 30, UI_W, 44, "STEP", C_ACCENT2);
  tft.drawText(12, 48, line1, C_TEXT, 0xFFFF, 1);
  tft.drawText(12, 62, line2, C_TEXT, 0xFFFF, 1);
  drawCard(UI_X, 78, UI_W, 34, "TIP", 0);
  tft.drawText(12, 92, "MOVE FULL RANGE", C_MUTED, 0xFFFF, 1);
  char buf[20];
  int rs = readAnalogAvg(PIN_STEERING, 4);
  int rt = readAnalogAvg(PIN_THROTTLE, 4);
  int rp = readAnalogAvg(PIN_POT_SUSPENSION, 4);
  snprintf(buf, sizeof(buf), "S%4d T%4d", rs, rt);
  tft.drawText(12, 108, buf, C_MUTED, 0xFFFF, 1);
  snprintf(buf, sizeof(buf), "P%4d", rp);
  tft.drawText(12, 120, buf, C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderActiveScreen() {
  switch (screen) {
    case SCREEN_MENU: renderMainMenu(); break;
    case SCREEN_SETTINGS: renderSettings(); break;
    case SCREEN_PAIR: renderPairMenu(); break;
    case SCREEN_PROFILES: renderProfiles(); break;
    case SCREEN_TELEMETRY: renderTelemetry(); break;
    case SCREEN_MODEL: renderModelSetup(); break;
    case SCREEN_RATES: renderRatesExpo(); break;
    case SCREEN_OUTPUTS: renderOutputs(); break;
    case SCREEN_CURVES: renderCurves(); break;
    case SCREEN_TIMERS: renderTimers(); break;
    case SCREEN_DIAG: renderDiagnostics(); break;
    case SCREEN_LOG: renderEventLog(); break;
    case SCREEN_ADV: renderAdvanced(); break;
    case SCREEN_MIXER: renderMixer(); break;
    case SCREEN_NAME: renderNameEdit(); break;
    case SCREEN_CHMAP: renderChMap(); break;
    case SCREEN_CAL: renderCalScreen(); break;
    case SCREEN_INFO: renderInfo(); break;
    case SCREEN_HOME:
    default:
      renderHome();
      break;
  }
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
      cfg.steer.center = readAnalogAvg(PIN_STEERING, 16) + STEER_CENTER_FIX;
      cfg.throttle.center = readAnalogAvg(PIN_THROTTLE, 16) + THROT_CENTER_FIX;
      cfg.susp.center = readAnalogAvg(PIN_POT_SUSPENSION, 16);
      calStep = CAL_STEER_SWEEP;
      calMin = 4095; calMax = 0;
      renderCalScreen();
    }
  } else if (calStep == CAL_STEER_SWEEP) {
    int v = readAnalogAvg(PIN_STEERING, 8);
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
    int v = readAnalogAvg(PIN_THROTTLE, 8);
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
    int v = readAnalogAvg(PIN_POT_SUSPENSION, 8);
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
      renderHome();
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
        renderMainMenu();
        buzzerBeep(1000, 40);
      }
    }
    if (btnTrimPlus.pressed) cfg.trimSteer = clampi(cfg.trimSteer + 1, -50, 50);
    if (btnTrimMinus.pressed) cfg.trimSteer = clampi(cfg.trimSteer - 1, -50, 50);
    if (btnSet.pressed) cfg.driveMode = 0;
  } else if (screen == SCREEN_MENU) {
    if (btnMenu.pressed) {
      screen = SCREEN_HOME;
      renderHome();
      buzzerBack();
    }
    if (btnTrimPlus.pressed) {
      mainMenuIndex = (mainMenuIndex + 1) % MAIN_MENU_COUNT;
      renderMainMenu();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      mainMenuIndex = (mainMenuIndex == 0) ? (MAIN_MENU_COUNT - 1) : (mainMenuIndex - 1);
      renderMainMenu();
      buzzerClick();
    }
    if (btnSet.pressed) {
      buzzerSelect();
      if (mainMenuIndex == 0) {
        screen = SCREEN_HOME;
        renderHome();
      } else if (mainMenuIndex == 1) {
        screen = SCREEN_SETTINGS;
        renderSettings();
      } else if (mainMenuIndex == 2) {
        screen = SCREEN_PAIR;
        renderPairMenu();
      } else if (mainMenuIndex == 3) {
        screen = SCREEN_PROFILES;
        renderProfiles();
      } else if (mainMenuIndex == 4) {
        screen = SCREEN_MODEL;
        renderModelSetup();
      } else if (mainMenuIndex == 5) {
        screen = SCREEN_RATES;
        renderRatesExpo();
      } else if (mainMenuIndex == 6) {
        screen = SCREEN_OUTPUTS;
        renderOutputs();
      } else if (mainMenuIndex == 7) {
        screen = SCREEN_CURVES;
        renderCurves();
      } else if (mainMenuIndex == 8) {
        screen = SCREEN_MIXER;
        renderMixer();
      } else if (mainMenuIndex == 9) {
        screen = SCREEN_NAME;
        renderNameEdit();
      } else if (mainMenuIndex == 10) {
        screen = SCREEN_CHMAP;
        renderChMap();
      } else if (mainMenuIndex == 11) {
        screen = SCREEN_TIMERS;
        renderTimers();
      } else if (mainMenuIndex == 12) {
        screen = SCREEN_TELEMETRY;
        renderTelemetry();
      } else if (mainMenuIndex == 13) {
        screen = SCREEN_DIAG;
        renderDiagnostics();
      } else if (mainMenuIndex == 14) {
        screen = SCREEN_LOG;
        renderEventLog();
      } else if (mainMenuIndex == 15) {
        screen = SCREEN_ADV;
        renderAdvanced();
      } else if (mainMenuIndex == 16) {
        screen = SCREEN_CAL;
        startCalibration();
      } else if (mainMenuIndex == 17) {
        screen = SCREEN_INFO;
        renderInfo();
      }
    }
  } else if (screen == SCREEN_SETTINGS) {
    if (btnMenu.pressed) {
      if (settingsEdit) {
        settingsEdit = false;
        renderSettings();
      } else {
        screen = SCREEN_MENU;
        renderMainMenu();
      }
      buzzerBack();
    }
    if (btnSet.pressed) {
      if (settingsIndex == SETTINGS_COUNT - 1) {
        saveConfig();
        buzzerSelect();
        renderSettings();
      } else {
        settingsEdit = !settingsEdit;
        renderSettings();
        buzzerSelect();
      }
    }
    if (btnTrimPlus.pressed) {
      if (settingsEdit) {
        // Adjust
        int delta = 1;
        if (settingsIndex == 0) cfg.trimSteer = clampi(cfg.trimSteer + delta, -50, 50);
        else if (settingsIndex == 1) cfg.trimThrot = clampi(cfg.trimThrot + delta, -50, 50);
        else if (settingsIndex == 2) cfg.driveMode = 0;
        else if (settingsIndex == 3) cfg.headlight = (cfg.headlight + 1) % 3;
        else if (settingsIndex == 4) cfg.taillight = (cfg.taillight + 1) % 3;
        else if (settingsIndex == 5) cfg.indMode = (cfg.indMode + 1) % 3;
        else if (settingsIndex == 6) cfg.indManual = (cfg.indManual + 1) % 4;
        else if (settingsIndex == 7) cfg.expoSteer = clampi(cfg.expoSteer + 2, 0, 100);
        else if (settingsIndex == 8) cfg.expoThrot = clampi(cfg.expoThrot + 2, 0, 100);
        else if (settingsIndex == 9) cfg.rateSteer = clampi(cfg.rateSteer + 2, 50, 100);
        else if (settingsIndex == 10) cfg.rateThrot = clampi(cfg.rateThrot + 2, 50, 100);
        else if (settingsIndex == 11) cfg.sendHz = clampi(cfg.sendHz + 1, 20, 60);
        else if (settingsIndex == 12) cfg.maxKmh = clampi(cfg.maxKmh + 1, 1, 200);
        else if (settingsIndex == 13) cfg.invertSteer = !cfg.invertSteer;
        else if (settingsIndex == 14) cfg.invertThrot = !cfg.invertThrot;
        renderSettings();
      } else {
        settingsIndex = (settingsIndex + 1) % SETTINGS_COUNT;
        renderSettings();
      }
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      if (settingsEdit) {
        int delta = -1;
        if (settingsIndex == 0) cfg.trimSteer = clampi(cfg.trimSteer + delta, -50, 50);
        else if (settingsIndex == 1) cfg.trimThrot = clampi(cfg.trimThrot + delta, -50, 50);
        else if (settingsIndex == 2) cfg.driveMode = 0;
        else if (settingsIndex == 3) cfg.headlight = (cfg.headlight == 0) ? 2 : (cfg.headlight - 1);
        else if (settingsIndex == 4) cfg.taillight = (cfg.taillight == 0) ? 2 : (cfg.taillight - 1);
        else if (settingsIndex == 5) cfg.indMode = (cfg.indMode == 0) ? 2 : (cfg.indMode - 1);
        else if (settingsIndex == 6) cfg.indManual = (cfg.indManual == 0) ? 3 : (cfg.indManual - 1);
        else if (settingsIndex == 7) cfg.expoSteer = clampi(cfg.expoSteer - 2, 0, 100);
        else if (settingsIndex == 8) cfg.expoThrot = clampi(cfg.expoThrot - 2, 0, 100);
        else if (settingsIndex == 9) cfg.rateSteer = clampi(cfg.rateSteer - 2, 50, 100);
        else if (settingsIndex == 10) cfg.rateThrot = clampi(cfg.rateThrot - 2, 50, 100);
        else if (settingsIndex == 11) cfg.sendHz = clampi(cfg.sendHz - 1, 20, 60);
        else if (settingsIndex == 12) cfg.maxKmh = clampi(cfg.maxKmh - 1, 1, 200);
        else if (settingsIndex == 13) cfg.invertSteer = !cfg.invertSteer;
        else if (settingsIndex == 14) cfg.invertThrot = !cfg.invertThrot;
        renderSettings();
      } else {
        settingsIndex = (settingsIndex == 0) ? (SETTINGS_COUNT - 1) : (settingsIndex - 1);
        renderSettings();
      }
      buzzerClick();
    }
  } else if (screen == SCREEN_PAIR) {
    if (btnMenu.pressed) {
      pairingActive = false;
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnTrimPlus.pressed) {
      pairIndex = (pairIndex + 1) % MAX_PEERS;
      renderPairMenu();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      pairIndex = (pairIndex == 0) ? (MAX_PEERS - 1) : (pairIndex - 1);
      renderPairMenu();
      buzzerClick();
    }
    if (btnSet.released) {
      uint32_t held = millis() - btnSet.pressStart;
      if (held >= 1500) {
        if (peers[pairIndex].valid) {
          peers[pairIndex].valid = 0;
          savePeers();
          buzzerAlert();
        }
        renderPairMenu();
      } else {
        if (peers[pairIndex].valid) {
          activePeer = pairIndex;
          savePeers();
          buzzerSelect();
          renderPairMenu();
        } else {
          pairingActive = true;
          pairingUntil = millis() + PAIR_LISTEN_MS;
          buzzerSelect();
          renderPairMenu();
        }
      }
    }
  } else if (screen == SCREEN_PROFILES) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnTrimPlus.pressed) {
      profileIndex = (profileIndex + 1) % MAX_PROFILES;
      renderProfiles();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      profileIndex = (profileIndex == 0) ? (MAX_PROFILES - 1) : (profileIndex - 1);
      renderProfiles();
      buzzerClick();
    }
    if (btnSet.released) {
      uint32_t held = millis() - btnSet.pressStart;
      if (held >= 1500) {
        saveProfile(profileIndex);
        buzzerSelect();
        renderProfiles();
      } else {
        loadProfile(profileIndex);
        buzzerSelect();
        renderProfiles();
      }
    }
  } else if (screen == SCREEN_TELEMETRY) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.pressed) {
      renderTelemetry();
      buzzerClick();
    }
  } else if (screen == SCREEN_MODEL) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.released) {
      uint32_t held = millis() - btnSet.pressStart;
      if (held >= 1500) {
        saveModel();
        buzzerSelect();
        renderModelSetup();
      } else {
        screen = SCREEN_NAME;
        renderNameEdit();
        buzzerSelect();
      }
    }
  } else if (screen == SCREEN_RATES) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnTrimPlus.pressed) {
      ratesIndex = (ratesIndex + 1) % 3;
      renderRatesExpo();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      ratesIndex = (ratesIndex == 0) ? 2 : (ratesIndex - 1);
      renderRatesExpo();
      buzzerClick();
    }
    if (btnSet.pressed) {
      RateExpo *re = (ratesIndex == 0) ? &model.steer : (ratesIndex == 1) ? &model.throttle : &model.aux;
      re->rate = clampi(re->rate + 5, 50, 120);
      re->expo = clampi(re->expo + 5, 0, 100);
      renderRatesExpo();
      buzzerSelect();
    }
  } else if (screen == SCREEN_OUTPUTS) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnTrimPlus.pressed) {
      outputIndex = (outputIndex + 1) % 4;
      renderOutputs();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      outputIndex = (outputIndex == 0) ? 3 : (outputIndex - 1);
      renderOutputs();
      buzzerClick();
    }
  } else if (screen == SCREEN_CURVES) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.pressed) {
      curveIndex = (curveIndex + 1) % 3;
      renderCurves();
      buzzerClick();
    }
  } else if (screen == SCREEN_TIMERS) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.pressed) {
      driveTimer.totalMs = 0;
      sessionTimer.totalMs = 0;
      renderTimers();
      buzzerSelect();
    }
  } else if (screen == SCREEN_DIAG) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.pressed) {
      renderDiagnostics();
      buzzerClick();
    }
  } else if (screen == SCREEN_LOG) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
  } else if (screen == SCREEN_ADV) {
    if (btnMenu.pressed) {
      advEdit = false;
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.pressed) {
      MenuItem &it = advItems[advIndex];
      if (it.type == MENU_ACTION && it.action) it.action();
      else advEdit = !advEdit;
      renderAdvanced();
      buzzerSelect();
    }
    if (btnTrimPlus.pressed) {
      if (advEdit) menuAdjust(advItems[advIndex], 1);
      else advIndex = (advIndex + 1) % advPage.count;
      renderAdvanced();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      if (advEdit) menuAdjust(advItems[advIndex], -1);
      else advIndex = (advIndex == 0) ? (advPage.count - 1) : (advIndex - 1);
      renderAdvanced();
      buzzerClick();
    }
  } else if (screen == SCREEN_CAL) {
    updateCalibration();
  } else if (screen == SCREEN_MIXER) {
    if (btnMenu.pressed) {
      saveModel();
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnTrimPlus.pressed) {
      if (mixerIndex == 0) model.mixSteerToAux = clampi(model.mixSteerToAux + 5, -100, 100);
      else model.mixThrotToAux = clampi(model.mixThrotToAux + 5, -100, 100);
      renderMixer();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      if (mixerIndex == 0) model.mixSteerToAux = clampi(model.mixSteerToAux - 5, -100, 100);
      else model.mixThrotToAux = clampi(model.mixThrotToAux - 5, -100, 100);
      renderMixer();
      buzzerClick();
    }
    if (btnSet.pressed) {
      mixerIndex = (mixerIndex + 1) % 2;
      renderMixer();
      buzzerSelect();
    }
  } else if (screen == SCREEN_NAME) {
    if (btnMenu.pressed) {
      saveProfiles();
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.pressed) {
      nameEdit = !nameEdit;
      renderNameEdit();
      buzzerSelect();
    }
    if (btnTrimPlus.pressed) {
      if (nameEdit) {
        char *name = profiles[activeProfile].name;
        const char *pos = strchr(nameChars, name[nameCharIndex]);
        int idx = pos ? (int)(pos - nameChars) : 0;
        idx = (idx + 1) % (int)(sizeof(nameChars) - 1);
        name[nameCharIndex] = nameChars[idx];
      } else {
        nameCharIndex = (nameCharIndex + 1) % 11;
      }
      renderNameEdit();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      if (nameEdit) {
        char *name = profiles[activeProfile].name;
        const char *pos = strchr(nameChars, name[nameCharIndex]);
        int idx = pos ? (int)(pos - nameChars) : 0;
        idx = (idx == 0) ? (int)(sizeof(nameChars) - 2) : (idx - 1);
        name[nameCharIndex] = nameChars[idx];
      } else {
        nameCharIndex = (nameCharIndex == 0) ? 10 : (nameCharIndex - 1);
      }
      renderNameEdit();
      buzzerClick();
    }
  } else if (screen == SCREEN_CHMAP) {
    if (btnMenu.pressed) {
      saveModel();
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
    if (btnSet.pressed) {
      chMapEdit = !chMapEdit;
      renderChMap();
      buzzerSelect();
    }
    if (btnTrimPlus.pressed) {
      if (chMapEdit) {
        model.chMap[chMapIndex] = (model.chMap[chMapIndex] + 1) % 4;
      } else {
        chMapIndex = (chMapIndex + 1) % 4;
      }
      renderChMap();
      buzzerClick();
    }
    if (btnTrimMinus.pressed) {
      if (chMapEdit) {
        model.chMap[chMapIndex] = (model.chMap[chMapIndex] == 0) ? 3 : (model.chMap[chMapIndex] - 1);
      } else {
        chMapIndex = (chMapIndex == 0) ? 3 : (chMapIndex - 1);
      }
      renderChMap();
      buzzerClick();
    }
  } else if (screen == SCREEN_INFO) {
    if (btnMenu.pressed) {
      screen = SCREEN_MENU;
      renderMainMenu();
      buzzerBack();
    }
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
  renderSplash();
  renderHome();
  buzzerSelect();
  advSyncFromCfg();

  espnowInit();
}

void loop() {
  buzzerUpdate();
  handleButtons();
  if (pairingActive && (int32_t)(millis() - pairingUntil) >= 0) {
    pairingActive = false;
    if (screen == SCREEN_PAIR) renderPairMenu();
  }
  if (telemetry.valid && (millis() - telemetry.lastMs) > 1200) {
    telemetry.valid = false;
  }
  uint32_t nowMs = millis();
  if (!sessionTimer.running) { sessionTimer.running = true; sessionTimer.lastUpdate = nowMs; }
  uint32_t dt = nowMs - sessionTimer.lastUpdate;
  sessionTimer.totalMs += dt;
  sessionTimer.lastUpdate = nowMs;
  bool driving = abs(throtAxis.value) > 80;
  if (driving) {
    if (!driveTimer.running) { driveTimer.running = true; driveTimer.lastUpdate = nowMs; }
    uint32_t dtd = nowMs - driveTimer.lastUpdate;
    driveTimer.totalMs += dtd;
    driveTimer.lastUpdate = nowMs;
  } else {
    driveTimer.running = false;
  }

  // Read and process inputs
  int rawSteer = readAnalogSmooth(PIN_STEERING) + STEER_CENTER_FIX;
  int rawThrot = readAnalogSmooth(PIN_THROTTLE) + THROT_CENTER_FIX;
  int rawSusp = readAnalogSmooth(PIN_POT_SUSPENSION);

  processAxis(steerAxis, rawSteer, cfg.steer, cfg.trimSteer, cfg.expoSteer, cfg.rateSteer, cfg.invertSteer);
  processAxis(throtAxis, rawThrot, cfg.throttle, cfg.trimThrot, cfg.expoThrot, cfg.rateThrot, cfg.invertThrot);
  processAxis(suspAxis, rawSusp, cfg.susp, 0, 0, 100, false);

  int16_t steerOut = applyRateExpo(steerAxis.value, model.steer);
  int16_t throtOut = applyRateExpo(throtAxis.value, model.throttle);
  int16_t auxOut = applyRateExpo(suspAxis.value, model.aux);
  auxOut = clampi(auxOut + (steerOut * model.mixSteerToAux) / 100 + (throtOut * model.mixThrotToAux) / 100, -1000, 1000);
  int16_t src[4] = { steerOut, throtOut, auxOut, 0 };
  for (int i = 0; i < 4; i++) {
    uint8_t m = model.chMap[i];
    channels[i].value = (m < 4) ? src[m] : 0;
  }

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
    p.driveMode = 0;
    p.headlight = cfg.headlight;
    p.taillight = cfg.taillight;
    p.indMode = cfg.indMode;
    p.indManual = cfg.indManual;
    uint8_t flags = 0;
    if (PIN_SW_GYRO >= 0) {
      bool gyroOn = SW_GYRO_ACTIVE_LOW ? (digitalRead(PIN_SW_GYRO) == LOW)
                                       : (digitalRead(PIN_SW_GYRO) == HIGH);
      if (gyroOn) flags |= 0x01;
    }
    p.flags = flags;
    p.checksum = calcChecksum(p);
    const uint8_t *dest = ESPNOW_BROADCAST;
    if (activePeer < MAX_PEERS && peers[activePeer].valid) dest = peers[activePeer].mac;
    esp_now_send(dest, (uint8_t *)&p, sizeof(p));
  }

  // UI
  if (screen == SCREEN_HOME) {
    if (now - lastFrameMs >= UI_FRAME_MS) {
      lastFrameMs = now;
      renderHome();
    }
  } else {
    if (now - lastMenuFrameMs >= UI_MENU_MS) {
      lastMenuFrameMs = now;
      renderActiveScreen();
    }
  }

  // LED status
  bool connected = telemetry.valid && ((millis() - telemetry.lastMs) < 1000);
  digitalWrite(PIN_LED_BUILTIN, connected ? LED_OFF_STATE : LED_ON_STATE);
}
