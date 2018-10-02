#pragma once

#include "neopixel.h"

#ifdef USE_DRIVER_BITBANG

#define DECLARE_DRIVER NeoPixel_BitBang driver

class NeoPixel_BitBang {

 public:

  NeoPixel_BitBang(uint8_t p, uint8_t t);

  void begin();
  void end();
  void setPin(uint8_t p);
  void updateLength(uint16_t numBytes);
  void show(uint8_t* pixels, uint16_t numBytes, uint32_t waitTime) __attribute__((optimize("Ofast")));

 private:
  const uint8_t type;
  uint8_t pin;
  uint32_t endTime;
};

#endif // USE_DRIVER_BITBANG
