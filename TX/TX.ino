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

static const uint8_t CFG_VERSION = 2;
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
      uint16_t px = (bits & (1 << (7 - col))) ? color : bg;
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
  uint32_t lastMs = 0;
  bool valid = false;
};
static TelemetryState telemetry;

struct TelemetryPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t flags;
  uint16_t speedKmh;
  int8_t rssi;
  uint8_t reserved;
};
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
static const uint16_t C_BG_TOP = rgb565(4, 8, 16);
static const uint16_t C_BG_BOT = rgb565(18, 24, 40);
static const uint16_t C_PANEL = rgb565(14, 22, 36);
static const uint16_t C_PANEL2 = rgb565(24, 34, 56);
static const uint16_t C_LINE = rgb565(36, 52, 78);
static const uint16_t C_ACCENT = rgb565(255, 178, 72);
static const uint16_t C_ACCENT2 = rgb565(56, 196, 255);
static const uint16_t C_TEXT = rgb565(238, 242, 250);
static const uint16_t C_MUTED = rgb565(134, 154, 178);
static const uint16_t C_WARN = rgb565(255, 92, 92);
static const uint16_t C_SHADOW = rgb565(2, 4, 8);
static const uint16_t C_HILITE = rgb565(48, 70, 102);
static const uint16_t C_GLOW = rgb565(150, 220, 255);

static void advActionSave();
static void advActionReset();
static void advActionBeep();

static const char *optDriveMode[] = { "ECO", "NORM", "SPORT" };
static const char *optInvert[] = { "OFF", "ON" };
static const char *optGyro[] = { "OFF", "ON" };

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
  { "DRIVE MODE", MENU_ENUM, &advDriveMode, 0, 2, 1, optDriveMode, 3, nullptr },
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
static uint32_t buzzerUntil = 0;
static bool buzzerOn = false;

static void buzzerBegin() {
  if (PIN_BUZZER >= 0) {
    pinMode(PIN_BUZZER, OUTPUT);
    noTone(PIN_BUZZER);
  }
}

static void buzzerClick() { buzzerBeep(2200, 20); }
static void buzzerSelect() { buzzerBeep(2600, 35); }
static void buzzerBack() { buzzerBeep(1800, 25); }
static void buzzerAlert() { buzzerBeep(3200, 60); }

static void buzzerBeep(uint16_t freq, uint16_t ms) {
  if (PIN_BUZZER < 0) return;
  buzzerOn = true;
  buzzerUntil = millis() + ms;
  tone(PIN_BUZZER, freq);
}

static void buzzerUpdate() {
  if (!buzzerOn) return;
  if ((int32_t)(millis() - buzzerUntil) >= 0) {
    buzzerOn = false;
    noTone(PIN_BUZZER);
  }
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
  cfg.driveMode = 1;
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
  float phase = (float)now * 0.0012f;
  for (int y = 0; y < TFT_H; y++) {
    float t = (float)y / (float)(TFT_H - 1);
    uint8_t r = (uint8_t)lerp8(4, 20, (uint8_t)(t * 255));
    uint8_t g = (uint8_t)lerp8(8, 26, (uint8_t)(t * 255));
    uint8_t b = (uint8_t)lerp8(18, 44, (uint8_t)(t * 255));
    int wave = (int)(6.0f * sinf(phase + (float)y * 0.08f));
    int rr = r + wave;
    int gg = g + wave;
    int bb = b + wave;
    if (rr < 0) rr = 0; if (rr > 255) rr = 255;
    if (gg < 0) gg = 0; if (gg > 255) gg = 255;
    if (bb < 0) bb = 0; if (bb > 255) bb = 255;
    tft.drawFastHLine(0, y, TFT_W, rgb565((uint8_t)rr, (uint8_t)gg, (uint8_t)bb));
  }

  int sweep = (now / 9) % (TFT_W + 70) - 70;
  uint16_t glow = mix565(C_PANEL2, C_ACCENT2, pulse8(now, 2200));
  if (sweep < TFT_W) {
    tft.fillRect(sweep, 20, 70, TFT_H - 20, scale565(glow, 120));
    tft.fillRect(sweep + 18, 20, 28, TFT_H - 20, scale565(glow, 170));
  }

  fillGradientRect(0, TFT_H - 24, TFT_W, 24, scale565(C_PANEL2, 200), C_BG_BOT);
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
  uint8_t pulse = pulse8(now, 2000);
  uint16_t top = mix565(C_PANEL, C_PANEL2, pulse);
  fillGradientRect(0, 0, TFT_W, 20, top, C_PANEL2);
  tft.drawFastHLine(0, 19, TFT_W, C_LINE);

  int dotX = TFT_W - 10;
  int dotY = 6;
  uint16_t dotCol = connected ? mix565(C_ACCENT2, C_GLOW, pulse) : C_WARN;
  tft.fillRect(dotX, dotY, 6, 6, dotCol);
  tft.drawRect(dotX - 1, dotY - 1, 8, 8, C_LINE);

  int badgeX = dotX - 22;
  tft.fillRect(badgeX, 4, 18, 12, C_PANEL2);
  tft.drawRect(badgeX, 4, 18, 12, gyroOn ? C_ACCENT2 : C_LINE);
  tft.drawText(badgeX + 3, 6, gyroOn ? "GYR" : "OFF", gyroOn ? C_ACCENT2 : C_MUTED, 0xFFFF, 1);

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
  drawTitleClamped(6, 2, title, maxChars);
}

static inline void drawScreenHeader(const char *title, uint32_t now) {
  bool connected = (now - lastAckMs) < 1000;
  bool gyroOn = false;
  if (PIN_SW_GYRO >= 0) {
    gyroOn = SW_GYRO_ACTIVE_LOW ? (digitalRead(PIN_SW_GYRO) == LOW)
                                : (digitalRead(PIN_SW_GYRO) == HIGH);
  }
  drawTopBar(title, connected, gyroOn, now);
}

static void drawCard(int x, int y, int w, int h, const char *title, uint16_t accent) {
  tft.fillRect(x + 1, y + 2, w, h, C_SHADOW);
  fillGradientRect(x, y, w, h, C_PANEL, C_PANEL2);
  tft.drawRect(x, y, w, h, C_LINE);
  tft.drawFastHLine(x + 1, y + 1, w - 2, C_HILITE);
  if (accent) tft.drawFastHLine(x + 2, y + h - 2, w - 4, accent);
  if (title) tft.drawText(x + 6, y + 3, title, C_MUTED, 0xFFFF, 1);
}

static void drawListItem(int x, int y, int w, int h, bool sel, bool edit,
                         const char *label, const char *value, uint32_t now) {
  uint16_t top = C_PANEL;
  uint16_t bot = C_PANEL2;
  uint16_t fg = C_TEXT;
  if (sel) {
    uint8_t pulse = pulse8(now, 1600);
    uint16_t a = edit ? C_ACCENT2 : C_ACCENT;
    uint16_t b = edit ? C_ACCENT : C_ACCENT2;
    top = mix565(a, b, pulse);
    bot = mix565(top, C_PANEL2, 120);
    fg = C_BG_TOP;
  }
  tft.fillRect(x + 1, y + 2, w, h, C_SHADOW);
  fillGradientRect(x, y, w, h, top, bot);
  tft.drawRect(x, y, w, h, sel ? C_GLOW : C_LINE);
  if (sel) tft.fillRect(x, y, 3, h, C_GLOW);
  tft.drawText(x + 6, y + 3, label, fg, 0xFFFF, 1);
  if (value && value[0]) {
    int vx = x + w - 6 - (int)strlen(value) * 6;
    tft.drawText(vx, y + 3, value, fg, 0xFFFF, 1);
  }
}

static void drawSpeedCard(int x, int y, int w, int h, uint16_t kmh, uint16_t maxKmh, uint32_t now) {
  drawCard(x, y, w, h, "SPEED", C_ACCENT2);
  char buf[8];
  snprintf(buf, sizeof(buf), "%u", kmh);
  uint8_t scale = (strlen(buf) <= 2) ? 2 : 1;
  int tw = textWidth8x16(buf, scale);
  int tx = x + (w - tw) / 2;
  int ty = y + 12 - (scale == 2 ? 2 : 0);
  drawText8x16(tx, ty, buf, C_TEXT, 0xFFFF, scale);
  tft.drawText(x + w - 30, y + h - 16, "KM/H", C_MUTED, 0xFFFF, 1);

  int barX = x + 10;
  int barY = y + h - 8;
  int barW = w - 20;
  int barH = 4;
  tft.fillRect(barX, barY, barW, barH, C_LINE);
  int fill = (maxKmh > 0) ? (int)((uint32_t)kmh * (uint32_t)barW / maxKmh) : 0;
  if (fill > barW) fill = barW;
  uint16_t barCol = mix565(C_ACCENT2, C_GLOW, pulse8(now, 1800));
  tft.fillRect(barX, barY, fill, barH, barCol);
}

static void drawThrottleCard(int x, int y, int w, int h, int v, uint32_t now) {
  drawCard(x, y, w, h, "THROTTLE", C_ACCENT2);
  int top = y + 14;
  int hh = h - 22;
  int mid = top + (hh / 2);
  tft.drawFastHLine(x + 6, mid, w - 12, C_LINE);
  int bar = (v * (hh / 2 - 2)) / 1000;
  uint16_t colPos = mix565(C_ACCENT2, C_GLOW, pulse8(now, 1500));
  if (bar >= 0) tft.fillRect(x + 8, mid - bar, w - 16, bar, colPos);
  else tft.fillRect(x + 8, mid, w - 16, -bar, C_ACCENT);

  char buf[8];
  int pct = (v * 100) / 1000;
  snprintf(buf, sizeof(buf), "%d%%", pct);
  tft.drawText(x + 6, y + h - 10, buf, C_MUTED, 0xFFFF, 1);
}

static void drawSteerCard(int x, int y, int w, int h, int v, uint32_t now) {
  drawCard(x, y, w, h, "STEER", C_ACCENT);
  int barY = y + h / 2 - 3;
  int barH = 6;
  int cx = x + w / 2;
  tft.drawFastVLine(cx, y + 14, h - 24, C_LINE);
  int bar = (v * (w / 2 - 8)) / 1000;
  uint16_t colPos = mix565(C_ACCENT, C_GLOW, pulse8(now, 1400));
  if (bar >= 0) tft.fillRect(cx, barY, bar, barH, colPos);
  else tft.fillRect(cx + bar, barY, -bar, barH, C_ACCENT2);

  char buf[8];
  int pct = (v * 100) / 1000;
  snprintf(buf, sizeof(buf), "%d%%", pct);
  int vx = x + w - 6 - (int)strlen(buf) * 6;
  tft.drawText(vx, y + h - 10, buf, C_MUTED, 0xFFFF, 1);
}

static void drawModePill(int x, int y, const char *name, uint16_t col, uint32_t now) {
  uint16_t top = mix565(col, C_PANEL2, pulse8(now, 1200));
  uint16_t bot = mix565(col, C_PANEL, 100);
  fillGradientRect(x, y, 36, 12, top, bot);
  tft.drawRect(x, y, 36, 12, col);
  int tx = x + (36 - (int)strlen(name) * 6) / 2;
  tft.drawText(tx, y + 2, name, C_BG_TOP, 0xFFFF, 1);
}

static void drawStatusCard(int x, int y, int w, int h, int susp, int16_t trimS, int16_t trimT, uint8_t mode, uint32_t now) {
  drawCard(x, y, w, h, "STATUS", C_ACCENT);
  tft.drawText(x + 6, y + 14, "SUSP", C_MUTED, 0xFFFF, 1);
  int sliderX = x + 36;
  int sliderY = y + 14;
  int sliderW = w - 44;
  int sliderH = 6;
  tft.fillRect(sliderX, sliderY, sliderW, sliderH, C_LINE);
  int fill = (susp + 1000) * sliderW / 2000;
  if (fill < 0) fill = 0;
  if (fill > sliderW) fill = sliderW;
  tft.fillRect(sliderX, sliderY, fill, sliderH, C_ACCENT2);

  const char *modeName = (mode == 0) ? "ECO" : (mode == 1) ? "NORM" : "SPORT";
  drawModePill(x + 6, y + 20, modeName, (mode == 2) ? C_ACCENT2 : C_ACCENT, now);

  char buf[24];
  snprintf(buf, sizeof(buf), "S%+d T%+d", trimS, trimT);
  int vx = x + w - 6 - (int)strlen(buf) * 6;
  tft.drawText(vx, y + 22, buf, C_TEXT, 0xFFFF, 1);
}

static void renderHome() {
  uint32_t now = millis();
  bool connected = (now - lastAckMs) < 1000;
  bool gyroOn = false;
  if (PIN_SW_GYRO >= 0) {
    gyroOn = SW_GYRO_ACTIVE_LOW ? (digitalRead(PIN_SW_GYRO) == LOW)
                                : (digitalRead(PIN_SW_GYRO) == HIGH);
  }
  uint16_t speedSrc = 0;
  if (telemetry.valid && (now - telemetry.lastMs) < 800) {
    speedSrc = telemetry.speedKmh;
  } else {
    speedSrc = (uint16_t)((abs(throtAxis.value) * (uint32_t)cfg.maxKmh) / 1000);
  }
  uiSteer = lerpf(uiSteer, steerAxis.value, 0.2f);
  uiThrot = lerpf(uiThrot, throtAxis.value, 0.2f);
  uiSusp = lerpf(uiSusp, suspAxis.value, 0.2f);
  uiSpeed = lerpf(uiSpeed, speedSrc, 0.18f);

  drawBackground(now);
  drawTopBar(profiles[activeProfile].name, connected, gyroOn, now);

  drawSpeedCard(6, 24, 116, 44, (uint16_t)uiSpeed, cfg.maxKmh, now);
  drawThrottleCard(6, 72, 56, 46, (int)uiThrot, now);
  drawSteerCard(66, 72, 56, 46, (int)uiSteer, now);
  drawStatusCard(6, 122, 116, 32, (int)uiSusp, cfg.trimSteer, cfg.trimThrot, cfg.driveMode, now);

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
  "TRIM STEER", "TRIM THROT", "DRIVE MODE", "EXPO STEER", "EXPO THROT",
  "RATE STEER", "RATE THROT", "SEND HZ", "MAX KMH", "INV STEER", "INV THROT", "SAVE"
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
  if (mainMenuIndex > mainMenuTop + 7) mainMenuTop = mainMenuIndex - 7;

  for (uint8_t i = 0; i < 8; i++) {
    uint8_t idx = mainMenuTop + i;
    if (idx >= MAIN_MENU_COUNT) break;
    int y = 26 + i * 16;
    bool sel = (idx == mainMenuIndex);
    drawListItem(8, y, 112, 14, sel, false, mainMenuItems[idx], ">", now);
  }
  if (MAIN_MENU_COUNT > 8) {
    int barX = 122;
    int barY = 26;
    int barH = 16 * 8 - 2;
    tft.drawFastVLine(barX, barY, barH, C_LINE);
    int thumbH = (barH * 8) / MAIN_MENU_COUNT;
    if (thumbH < 10) thumbH = 10;
    int thumbY = barY + ((barH - thumbH) * mainMenuTop) / (MAIN_MENU_COUNT - 8);
    tft.fillRect(barX - 1, thumbY, 3, thumbH, C_ACCENT2);
  }
  tft.present();
}

static void renderSettings() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("SETTINGS", now);

  if (settingsIndex < settingsTop) settingsTop = settingsIndex;
  if (settingsIndex > settingsTop + 7) settingsTop = settingsIndex - 7;

  char buf[16];
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t idx = settingsTop + i;
    if (idx >= SETTINGS_COUNT) break;
    const char *val = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%d", cfg.trimSteer); val = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%d", cfg.trimThrot); val = buf; }
    else if (idx == 2) { val = (cfg.driveMode == 0) ? "ECO" : (cfg.driveMode == 1) ? "NORM" : "SPORT"; }
    else if (idx == 3) { snprintf(buf, sizeof(buf), "%u", cfg.expoSteer); val = buf; }
    else if (idx == 4) { snprintf(buf, sizeof(buf), "%u", cfg.expoThrot); val = buf; }
    else if (idx == 5) { snprintf(buf, sizeof(buf), "%u", cfg.rateSteer); val = buf; }
    else if (idx == 6) { snprintf(buf, sizeof(buf), "%u", cfg.rateThrot); val = buf; }
    else if (idx == 7) { snprintf(buf, sizeof(buf), "%u", cfg.sendHz); val = buf; }
    else if (idx == 8) { snprintf(buf, sizeof(buf), "%u", cfg.maxKmh); val = buf; }
    else if (idx == 9) { val = cfg.invertSteer ? "ON" : "OFF"; }
    else if (idx == 10) { val = cfg.invertThrot ? "ON" : "OFF"; }
    else if (idx == 11) { val = ">"; }
    int y = 26 + i * 16;
    bool sel = (idx == settingsIndex);
    drawListItem(8, y, 112, 14, sel, settingsEdit && sel, settingsItems[idx], val, now);
  }
  if (SETTINGS_COUNT > 8) {
    int barX = 122;
    int barY = 26;
    int barH = 16 * 8 - 2;
    tft.drawFastVLine(barX, barY, barH, C_LINE);
    int thumbH = (barH * 8) / SETTINGS_COUNT;
    if (thumbH < 10) thumbH = 10;
    int thumbY = barY + ((barH - thumbH) * settingsTop) / (SETTINGS_COUNT - 8);
    tft.fillRect(barX - 1, thumbY, 3, thumbH, C_ACCENT2);
  }
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
    int y = 28 + i * 20;
    bool sel = (i == pairIndex);
    drawListItem(8, y, 112, 16, sel, false, label, val, now);
  }

  if (pairingActive) {
    drawCard(8, 130, 112, 16, nullptr, 0);
    tft.drawText(12, 134, "LISTENING...", C_TEXT, 0xFFFF, 1);
  } else {
    drawCard(8, 130, 112, 16, nullptr, 0);
    tft.drawText(12, 134, "SET=PAIR  HOLD=DEL", C_MUTED, 0xFFFF, 1);
  }
  tft.present();
}

static void renderProfiles() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("PROFILES", now);
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
    int y = 28 + i * 18;
    bool sel = (i == profileIndex);
    const char *val = (i == activeProfile) ? "ACTIVE" : "";
    drawListItem(8, y, 112, 14, sel, false, profiles[i].name, val, now);
  }
  drawCard(8, 136, 112, 16, nullptr, 0);
  tft.drawText(12, 140, "SET=LOAD  HOLD=SAVE", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderTelemetry() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("TELEMETRY", now);
  char buf[16];
  uint32_t age = telemetry.valid ? (millis() - telemetry.lastMs) : 9999;

  drawCard(6, 28, 116, 40, "SPEED", C_ACCENT2);
  snprintf(buf, sizeof(buf), "%u", telemetry.speedKmh);
  uint8_t scale = (strlen(buf) <= 2) ? 2 : 1;
  int tw = textWidth8x16(buf, scale);
  int tx = 6 + (116 - tw) / 2;
  drawText8x16(tx, 40, buf, C_TEXT, 0xFFFF, scale);
  tft.drawText(88, 54, "KM/H", C_MUTED, 0xFFFF, 1);

  drawCard(6, 74, 116, 40, "RSSI", C_ACCENT);
  snprintf(buf, sizeof(buf), "%d", telemetry.rssi);
  tw = textWidth8x16(buf, 2);
  tx = 6 + (116 - tw) / 2;
  drawText8x16(tx, 84, buf, C_TEXT, 0xFFFF, 2);
  tft.drawText(92, 100, "DBM", C_MUTED, 0xFFFF, 1);

  drawCard(6, 120, 116, 28, "AGE", 0);
  snprintf(buf, sizeof(buf), "%lums", (unsigned long)age);
  tft.drawText(10, 132, buf, C_TEXT, 0xFFFF, 1);
  tft.present();
}

static void renderModelSetup() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("MODEL SETUP", now);
  drawCard(6, 28, 116, 36, "MODEL", C_ACCENT2);
  drawText8x16Shadow(12, 42, profiles[activeProfile].name, C_TEXT);

  drawCard(6, 68, 116, 36, "FAILSAFE", C_ACCENT);
  char buf[16];
  snprintf(buf, sizeof(buf), "S:%u  T:%u", model.failsafeSteer, model.failsafeThrot);
  tft.drawText(12, 84, buf, C_TEXT, 0xFFFF, 1);

  drawCard(6, 108, 116, 40, "ACTIONS", 0);
  tft.drawText(12, 122, "NAME=SET", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 134, "SAVE=HOLD", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderRatesExpo() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("RATES / EXPO", now);
  const char *items[] = { "STEER", "THROTTLE", "AUX" };
  for (int i = 0; i < 3; i++) {
    int y = 30 + i * 38;
    bool sel = (i == ratesIndex);
    RateExpo *re = (i == 0) ? &model.steer : (i == 1) ? &model.throttle : &model.aux;
    char buf[16];
    snprintf(buf, sizeof(buf), "R%u  E%u", re->rate, re->expo);
    drawListItem(8, y, 112, 28, sel, false, items[i], buf, now);
  }
  tft.present();
}

static void renderOutputs() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("OUTPUTS", now);
  const char *ch[] = { "CH1", "CH2", "CH3", "CH4" };
  for (int i = 0; i < 4; i++) {
    int y = 30 + i * 26;
    bool sel = (i == outputIndex);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", channels[i].value);
    drawListItem(8, y, 112, 18, sel, false, ch[i], buf, now);
  }
  tft.present();
}

static void renderCurves() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("CURVES", now);
  drawCard(6, 28, 116, 92, "CURVE", C_ACCENT2);
  int cx = 12, cy = 44, w = 104, h = 68;
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
  tft.drawText(10, 32, buf, C_MUTED, 0xFFFF, 1);
  drawCard(6, 124, 116, 24, "ACTION", 0);
  tft.drawText(12, 134, "SET=NEXT", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderTimers() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("TIMERS", now);
  uint32_t d = driveTimer.totalMs / 1000;
  uint32_t s = sessionTimer.totalMs / 1000;
  char buf[16];
  drawCard(6, 28, 116, 44, "DRIVE", C_ACCENT2);
  snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(d/60), (unsigned long)(d%60));
  drawText8x16(20, 44, buf, C_TEXT, 0xFFFF, 1);
  drawCard(6, 76, 116, 44, "SESSION", C_ACCENT);
  snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)(s/60), (unsigned long)(s%60));
  drawText8x16(20, 92, buf, C_TEXT, 0xFFFF, 1);
  drawCard(6, 124, 116, 24, "ACTION", 0);
  tft.drawText(12, 134, "SET=RESET", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderDiagnostics() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("DIAGNOSTICS", now);
  char buf[16];

  snprintf(buf, sizeof(buf), "%d", steerAxis.raw);
  drawListItem(8, 30, 112, 18, false, false, "STEER RAW", buf, now);
  snprintf(buf, sizeof(buf), "%d", throtAxis.raw);
  drawListItem(8, 54, 112, 18, false, false, "THROT RAW", buf, now);
  snprintf(buf, sizeof(buf), "%d", suspAxis.raw);
  drawListItem(8, 78, 112, 18, false, false, "SUSP RAW", buf, now);
  snprintf(buf, sizeof(buf), "%u", (unsigned)(1000 / UI_MENU_MS));
  drawListItem(8, 102, 112, 18, false, false, "FPS", buf, now);
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
    int y = 30 + i * 18;
    drawListItem(8, y, 112, 14, false, false, buf, "", now);
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
    int y = 34 + i * 28;
    bool sel = (i == mixerIndex);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)*vals[i]);
    drawListItem(8, y, 112, 20, sel, false, items[i], buf, now);
  }
  drawCard(8, 118, 112, 24, "ACTION", 0);
  tft.drawText(12, 130, "SET=SAVE", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static const char nameChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

static void renderNameEdit() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("NAME EDIT", now);
  drawCard(6, 28, 116, 42, "PROFILE", C_ACCENT2);
  int nameX = 12;
  int nameY = 46;
  tft.drawText(nameX, nameY, profiles[activeProfile].name, C_TEXT, 0xFFFF, 1);
  int x = nameX + nameCharIndex * 6;
  tft.drawFastHLine(x, nameY + 10, 6, nameEdit ? C_ACCENT2 : C_ACCENT);

  drawCard(6, 76, 116, 72, "CONTROLS", 0);
  tft.drawText(12, 92, nameEdit ? "EDIT CHAR" : "MOVE CURSOR", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 106, "SET=TOGGLE", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 120, "MENU=BACK", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void renderChMap() {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader("CHANNEL MAP", now);
  const char *src[] = { "STEER", "THROT", "AUX", "NONE" };
  for (int i = 0; i < 4; i++) {
    int y = 30 + i * 26;
    bool sel = (i == chMapIndex);
    char label[8];
    snprintf(label, sizeof(label), "CH%u", i + 1);
    uint8_t v = model.chMap[i];
    if (v > 3) v = 3;
    drawListItem(8, y, 112, 18, sel, chMapEdit && sel, label, src[v], now);
  }
  drawCard(8, 130, 112, 24, "ACTION", 0);
  tft.drawText(12, 140, "SET=SAVE", C_MUTED, 0xFFFF, 1);
  tft.present();
}

static void advDrawTitle(const char *title) {
  uint32_t now = millis();
  drawBackground(now);
  drawScreenHeader(title, now);
}

static void advDrawRow(int y, bool sel, const char *label, const char *value, bool edit) {
  uint32_t now = millis();
  drawListItem(8, y + 2, 112, 14, sel, edit, label, value, now);
}

static void advSyncFromCfg() {
  advDriveMode = cfg.driveMode;
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
  cfg.driveMode = (uint8_t)advDriveMode;
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
  drawCard(6, 28, 116, 60, "DEVICE", C_ACCENT2);
  tft.drawText(12, 46, "OpenTX ESP32", C_TEXT, 0xFFFF, 1);
  tft.drawText(12, 60, "ESP-NOW TX", C_MUTED, 0xFFFF, 1);
  tft.drawText(12, 74, "DMA TFT", C_MUTED, 0xFFFF, 1);
  drawCard(6, 92, 116, 40, "FIRMWARE", C_ACCENT);
  tft.drawText(12, 110, "v1.0", C_TEXT, 0xFFFF, 1);
  tft.present();
}

static void renderSplash() {
  for (int i = 0; i < 28; i++) {
    uint32_t now = millis();
    drawBackground(now);
    drawCard(10, 40, 108, 64, nullptr, C_ACCENT2);
    drawText8x16Shadow(26, 52, "OPEN", C_TEXT);
    drawText8x16Shadow(38, 68, "TX", C_ACCENT2);
    tft.drawText(32, 86, "ESP32 RADIO", C_MUTED, 0xFFFF, 1);
    int w = (i * 100) / 27;
    uint16_t barCol = mix565(C_ACCENT2, C_GLOW, pulse8(now, 1200));
    tft.fillRect(14, 110, 100, 6, C_LINE);
    tft.fillRect(14, 110, w, 6, barCol);
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
  drawCard(6, 34, 116, 60, "STEP", C_ACCENT2);
  tft.drawText(12, 54, line1, C_TEXT, 0xFFFF, 1);
  tft.drawText(12, 70, line2, C_TEXT, 0xFFFF, 1);
  drawCard(6, 98, 116, 40, "TIP", 0);
  tft.drawText(12, 114, "MOVE FULL RANGE", C_MUTED, 0xFFFF, 1);
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
    if (btnSet.pressed) cfg.driveMode = (cfg.driveMode + 1) % 3;
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
        else if (settingsIndex == 2) cfg.driveMode = (cfg.driveMode + 1) % 3;
        else if (settingsIndex == 3) cfg.expoSteer = clampi(cfg.expoSteer + 2, 0, 100);
        else if (settingsIndex == 4) cfg.expoThrot = clampi(cfg.expoThrot + 2, 0, 100);
        else if (settingsIndex == 5) cfg.rateSteer = clampi(cfg.rateSteer + 2, 50, 100);
        else if (settingsIndex == 6) cfg.rateThrot = clampi(cfg.rateThrot + 2, 50, 100);
        else if (settingsIndex == 7) cfg.sendHz = clampi(cfg.sendHz + 1, 20, 60);
        else if (settingsIndex == 8) cfg.maxKmh = clampi(cfg.maxKmh + 1, 1, 200);
        else if (settingsIndex == 9) cfg.invertSteer = !cfg.invertSteer;
        else if (settingsIndex == 10) cfg.invertThrot = !cfg.invertThrot;
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
        else if (settingsIndex == 2) cfg.driveMode = (cfg.driveMode == 0) ? 2 : (cfg.driveMode - 1);
        else if (settingsIndex == 3) cfg.expoSteer = clampi(cfg.expoSteer - 2, 0, 100);
        else if (settingsIndex == 4) cfg.expoThrot = clampi(cfg.expoThrot - 2, 0, 100);
        else if (settingsIndex == 5) cfg.rateSteer = clampi(cfg.rateSteer - 2, 50, 100);
        else if (settingsIndex == 6) cfg.rateThrot = clampi(cfg.rateThrot - 2, 50, 100);
        else if (settingsIndex == 7) cfg.sendHz = clampi(cfg.sendHz - 1, 20, 60);
        else if (settingsIndex == 8) cfg.maxKmh = clampi(cfg.maxKmh - 1, 1, 200);
        else if (settingsIndex == 9) cfg.invertSteer = !cfg.invertSteer;
        else if (settingsIndex == 10) cfg.invertThrot = !cfg.invertThrot;
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
    p.driveMode = cfg.driveMode;
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
  bool connected = (millis() - lastAckMs) < 1000;
  digitalWrite(PIN_LED_BUILTIN, connected ? LED_OFF_STATE : LED_ON_STATE);
}
