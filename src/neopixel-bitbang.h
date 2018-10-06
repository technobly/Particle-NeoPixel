/*-------------------------------------------------------------------------
  Driver to generate the Neopixel waveform by turning the pin on, executing a precise
  number of delay instructions then turning the pin off again. This is called bit banging.

  The downside of the bit bang driver is that it needs to disable interrupts and occupy
  the whole CPU while generating the waveform.
  --------------------------------------------------------------------*/

/* ======================= neopixel-bitbang.h ======================= */
/*--------------------------------------------------------------------
  This file is part of the Adafruit NeoPixel library.

  NeoPixel is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of
  the License, or (at your option) any later version.

  NeoPixel is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with NeoPixel.  If not, see
  <http://www.gnu.org/licenses/>.
  --------------------------------------------------------------------*/
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
