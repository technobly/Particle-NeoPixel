/*-------------------------------------------------------------------------
  Driver to generate the Neopixel waveform on the nRF52840 using the PWM peripheral and DMA.

  On the Argon, Boron and Xenon, only these pins can be used for Neopixel:
  - D2, D3, A4, A5
  - D4, D6, D7, D8
  - A0, A1, A2, A3
  
  In addition, only one pin per group can be used at a time. So it's OK to have one Adafruit_NeoPixel
  instance on pin D2 and another one on pin A2, but it's not possible to have one on pin A0 and another
  one on pin A1.

  For each Neopixel LED the driver outputs 3 or 4 bytes of data (1 byte per red/green/blue
  color + optional white) so it outputs 24 or 32 bits per LED. For each bit, the waveform
  lasts 1.25 microseconds. If the waveform is high for 300 nanoseconds, then low for 950 ns
  it is a 0 bit. If it is high for 850 ns and low for 400 ns, it is a 1 bit.

  This driver uses the ability of the nRF microcontroller to play back an array of PWM
  duty cycles without CPU intervention. It converts the pixel array into an array of PWM duty
  cycles so it uses 16 times more memory than the bit bang driver (since each pixel has 8 bits
  per pixel and the duty cycle register has 2 bytes).

  For the original idea of using PWM and DMA for driving Neopixel / WS2812 LEDs, see the
  OctoWS2811 library by Paul Stoffregen
  https://www.pjrc.com/teensy/td_libs_OctoWS2811.html#tech

  --------------------------------------------------------------------*/

/* ======================= neopixel-nrfpwm.h ======================= */
#pragma once

#include "neopixel.h"

#ifdef USE_DRIVER_NRFPWM

#include "nrfx_pwm.h"
#define NRF5X_PWM_COUNT 4

#define DECLARE_DRIVER NeoPixel_NrfPwm driver

class NeoPixel_NrfPwm {

 public:

  NeoPixel_NrfPwm(uint8_t p, uint8_t t);
  ~NeoPixel_NrfPwm();

  void begin();
  void end();
  void setPin(uint8_t p);
  void updateLength(uint16_t numBytes);
  void show(uint8_t* pixels, uint16_t numBytes, uint32_t waitTime);

 private:
  void waveformDone();
  void pixelsToBits(uint8_t* pixels, uint16_t numBytes);
  void waitForInactive();
  uint16_t freqkHz() const;

  // nrfx doesn't allow passing state to handler so we need one static function per PWM instance
  static void stopHandler0(nrfx_pwm_evt_type_t eventType);
  static void stopHandler1(nrfx_pwm_evt_type_t eventType);
  static void stopHandler2(nrfx_pwm_evt_type_t eventType);
  static void stopHandler3(nrfx_pwm_evt_type_t eventType);
  static NeoPixel_NrfPwm* instances[NRF_PWM_CHANNEL_COUNT];

  static const nrfx_pwm_t pwms[NRF5X_PWM_COUNT];
  
  const uint8_t type;
  uint8_t pin;
  uint32_t endTime;
  bool active;
  uint16_t* bits;
};

#endif // USE_DRIVER_NRFPWM
