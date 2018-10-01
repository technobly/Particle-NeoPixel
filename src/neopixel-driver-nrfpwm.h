#pragma once

#include "neopixel.h"

#ifdef USE_DRIVER_NRFPWM

#include "nrfx_pwm.h"

#define DECLARE_DRIVER NeoPixel_Driver_NrfPwm driver

class NeoPixel_Driver_NrfPwm {

 public:

  NeoPixel_Driver_NrfPwm(uint8_t p, uint8_t t);
  ~NeoPixel_Driver_NrfPwm();

  void begin();
  void end();
  void setPin(uint8_t p);
  void updateLength(uint16_t numBytes);
  void show(uint8_t* pixels, uint16_t numBytes);

 private:
  void waveformDone();
  void pixelsToBits(uint8_t* pixels, uint16_t numBytes);
  void waitForInactive();
  uint16_t freqkHz() const;

  // FIXME: currently nrfx doesn't allow passing state to handler so we need a static function and global instance
  static void stopHandler(nrfx_pwm_evt_type_t eventType);
  static NeoPixel_Driver_NrfPwm* instance;

  const uint8_t type;
  uint8_t pin;
  uint32_t endTime;
  bool active;
  uint16_t* bits;
};

#endif // USE_DRIVER_NRFPWM