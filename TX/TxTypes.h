#ifndef TX_TYPES_H
#define TX_TYPES_H

#include <stdint.h>

struct AxisCal {
  int16_t min;
  int16_t center;
  int16_t max;
};

struct AxisState {
  int raw = 0;
  float smooth = 0.0f;
  int value = 0;
};

struct TxConfig {
  uint8_t ver;
  AxisCal steer;
  AxisCal throttle;
  AxisCal susp;
  int16_t trimSteer;
  int16_t trimThrot;
  uint8_t driveMode;    // 0..2
  uint8_t headlight;    // 0..2 off/low/high
  uint8_t taillight;    // 0..2 off/low/high
  uint8_t indMode;      // 0 off,1 auto,2 manual
  uint8_t indManual;    // 0 off,1 left,2 right,3 hazard
  uint8_t expoSteer;    // 0..100
  uint8_t expoThrot;    // 0..100
  uint8_t rateSteer;    // 50..100
  uint8_t rateThrot;    // 50..100
  uint8_t sendHz;       // 20..60
  uint8_t invertSteer;  // 0/1
  uint8_t invertThrot;  // 0/1
  uint16_t maxKmh;      // speed scale
};

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
  uint8_t headlight;
  uint8_t taillight;
  uint8_t indMode;
  uint8_t indManual;
  uint8_t flags;
  uint8_t checksum;
};

struct TelemetryPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t flags;
  uint16_t speedKmh;
  int8_t rssi;
  int16_t tempCx10;
};

struct RateExpo {
  uint8_t rate;
  uint8_t expo;
  int16_t subtrim;
  int16_t min;
  int16_t max;
  uint8_t reverse;
};

struct Curve {
  int8_t points[9];
};

struct ModelConfig {
  RateExpo steer;
  RateExpo throttle;
  RateExpo aux;
  Curve curves[3];
  uint8_t failsafeSteer;
  uint8_t failsafeThrot;
  int8_t mixSteerToAux;
  int8_t mixThrotToAux;
  uint8_t chMap[4];
};

#endif // TX_TYPES_H
