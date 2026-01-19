#include "PanelIO.h"

#include <driver/gpio.h>
#include <esp_heap_caps.h>

namespace {
static const int kSpiClockHz = 26000000;

spi_device_handle_t spi;
uint16_t *dmaLineBuffers[2];

uint16_t swap565(uint16_t v) {
  return static_cast<uint16_t>((v << 8) | (v >> 8));
}

void writeCommand(uint8_t cmd) {
  spi_transaction_t t = {};
  t.length = 8;
  t.tx_buffer = &cmd;
  digitalWrite(PIN_TFT_DC, LOW);
  spi_device_transmit(spi, &t);
}

void writeData(const uint8_t *data, size_t len) {
  if (!data || len == 0) return;
  spi_transaction_t t = {};
  t.length = len * 8;
  t.tx_buffer = data;
  digitalWrite(PIN_TFT_DC, HIGH);
  spi_device_transmit(spi, &t);
}

void setAddrWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  uint8_t data[4];

  writeCommand(0x2A);
  data[0] = static_cast<uint8_t>(x0 >> 8);
  data[1] = static_cast<uint8_t>(x0 & 0xFF);
  data[2] = static_cast<uint8_t>(x1 >> 8);
  data[3] = static_cast<uint8_t>(x1 & 0xFF);
  writeData(data, 4);

  writeCommand(0x2B);
  data[0] = static_cast<uint8_t>(y0 >> 8);
  data[1] = static_cast<uint8_t>(y0 & 0xFF);
  data[2] = static_cast<uint8_t>(y1 >> 8);
  data[3] = static_cast<uint8_t>(y1 & 0xFF);
  writeData(data, 4);

  writeCommand(0x2C);
}

void resetPanel() {
  digitalWrite(PIN_TFT_RST, LOW);
  delay(5);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(120);
}

void initPanel() {
  writeCommand(0x01);
  delay(5);
  writeCommand(0x11);
  delay(120);

  uint8_t data = 0x05;
  writeCommand(0x3A);
  writeData(&data, 1);

  data = TFT_MADCTL;
  writeCommand(0x36);
  writeData(&data, 1);

  writeCommand(0x29);
  delay(20);
}
}  // namespace

namespace PanelIO {
bool begin() {
  pinMode(PIN_TFT_DC, OUTPUT);
  pinMode(PIN_TFT_RST, OUTPUT);
  pinMode(PIN_TFT_CS, OUTPUT);

  digitalWrite(PIN_TFT_CS, HIGH);
  resetPanel();

  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = PIN_TFT_MOSI;
  buscfg.miso_io_num = -1;
  buscfg.sclk_io_num = PIN_TFT_SCLK;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = kWidth * 2 * 16;

  spi_device_interface_config_t devcfg = {};
  devcfg.clock_speed_hz = kSpiClockHz;
  devcfg.mode = 0;
  devcfg.spics_io_num = PIN_TFT_CS;
  devcfg.queue_size = 4;

  esp_err_t err = spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (err != ESP_OK) {
    return false;
  }
  err = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
  if (err != ESP_OK) {
    return false;
  }

  dmaLineBuffers[0] = static_cast<uint16_t *>(heap_caps_malloc(kWidth * sizeof(uint16_t), MALLOC_CAP_DMA));
  dmaLineBuffers[1] = static_cast<uint16_t *>(heap_caps_malloc(kWidth * sizeof(uint16_t), MALLOC_CAP_DMA));
  if (!dmaLineBuffers[0] || !dmaLineBuffers[1]) {
    return false;
  }

  initPanel();
  return true;
}

void pushRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *buffer, int16_t stride) {
  if (!buffer || w <= 0 || h <= 0) return;
  if (x < 0 || y < 0) return;
  if (x + w > kWidth || y + h > kHeight) return;

  setAddrWindow(x, y, x + w - 1, y + h - 1);
  digitalWrite(PIN_TFT_DC, HIGH);

  spi_transaction_t trans[2];
  memset(&trans, 0, sizeof(trans));
  int bufIndex = 0;
  const uint16_t *row = buffer + (y * stride) + x;

  for (int16_t yy = 0; yy < h; ++yy) {
    uint16_t *dmaBuf = dmaLineBuffers[bufIndex];
    for (int16_t xx = 0; xx < w; ++xx) {
      dmaBuf[xx] = swap565(row[xx]);
    }

    spi_transaction_t *t = &trans[bufIndex];
    t->length = w * 16;
    t->tx_buffer = dmaBuf;

    spi_device_queue_trans(spi, t, portMAX_DELAY);

    if (yy > 0) {
      spi_transaction_t *ret;
      spi_device_get_trans_result(spi, &ret, portMAX_DELAY);
    }

    bufIndex ^= 1;
    row += stride;
  }

  spi_transaction_t *ret;
  spi_device_get_trans_result(spi, &ret, portMAX_DELAY);
}
}  // namespace PanelIO
