/*-------------------------------------------------------------------------
  Spark Core, Particle Photon, P1, Electron and RedBear Duo library to control
  WS2811/WS2812/WS2813 based RGB LED devices such as Adafruit NeoPixel strips.

  Supports:
  - 800 KHz WS2812, WS2812B, WS2813 and 400kHz bitstream and WS2811
  - 800 KHz bitstream SK6812RGBW (NeoPixel RGBW pixel strips)
    (use 'SK6812RGBW' as PIXEL_TYPE)

  Also supports:
  - Radio Shack Tri-Color Strip with TM1803 controller 400kHz bitstream.
  - TM1829 pixels

  PLEASE NOTE that the NeoPixels require 5V level inputs
  and the Spark Core, Particle Photon, P1, Electron and RedBear Duo only
  have 3.3V level outputs. Level shifting is necessary, but will require
  a fast device such as one of the following:

  [SN74HCT125N]
  http://www.digikey.com/product-detail/en/SN74HCT125N/296-8386-5-ND/376860

  [SN74HCT245N]
  http://www.digikey.com/product-detail/en/SN74HCT245N/296-1612-5-ND/277258

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
  Modified to work with Particle devices by Technobly.
  Contributions by PJRC and other members of the open source community.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!
  --------------------------------------------------------------------*/

/* ======================= Adafruit_NeoPixel.h ======================= */
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

// FIXME: remove before release
#ifndef PLATFORM_ID
#define PLATFORM_ID 14
#endif

#if (PLATFORM_ID == 0) || (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88)
  #define USE_DRIVER_BLOCKING
#elif (PLATFORM_ID == 12) || (PLATFORM_ID == 13) || (PLATFORM_ID == 14) // Argon (12), Boron (13), Xenon (14)
  // These platforms use hardware PWM for generating the Neopixel waveform
  #define USE_DRIVER_NRFPWM
#else
  #error "*** PLATFORM_ID not supported by this library. PLATFORM should be Core, Photon, P1, Electron, Argon, Boron or Xenon or RedBear Duo ***"
#endif

#include "Particle.h"

#ifdef USE_DRIVER_BLOCKING
#include "neopixel-driver-blocking.h"
#endif

#ifdef USE_DRIVER_NRFPWM
#include "neopixel-driver-nrfpwm.h"
#endif

// 'type' flags for LED pixels (third parameter to constructor):
#define WS2811         0x00 // 400 KHz datastream (NeoPixel)
#define WS2812         0x02 // 800 KHz datastream (NeoPixel)
#define WS2812B        0x02 // 800 KHz datastream (NeoPixel)
#define WS2813         0x02 // 800 KHz datastream (NeoPixel)
#define TM1803         0x03 // 400 KHz datastream (Radio Shack Tri-Color Strip)
#define TM1829         0x04 // 800 KHz datastream ()
#define WS2812B2       0x05 // 800 KHz datastream (NeoPixel)
#define SK6812RGBW     0x06 // 800 KHz datastream (NeoPixel RGBW)
#define WS2812B_FAST   0x07 // 800 KHz datastream (NeoPixel)
#define WS2812B2_FAST  0x08 // 800 KHz datastream (NeoPixel)

class Adafruit_NeoPixel {

 public:

  // Constructor: number of LEDs, pin number, LED type
  Adafruit_NeoPixel(uint16_t n, uint8_t p, uint8_t t = WS2812B);
  ~Adafruit_NeoPixel();

  void begin();
  void end();
  void show();
  void setPin(uint8_t p);
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  void setPixelColor(uint16_t n, uint32_t c);
  void setBrightness(uint8_t);
  void setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue);
  void setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aWhite);
  void setColorScaled(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aScaling);
  void setColorScaled(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aWhite, byte aScaling);
  void setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aBrightness);
  void setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aWhite, byte aBrightness);
  void updateLength(uint16_t n);
  void clear();
  uint8_t* getPixels() const;
  uint8_t getBrightness() const;
  uint16_t numPixels() const;
  uint16_t getNumLeds() const;
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  uint32_t getPixelColor(uint16_t n) const;
  byte brightnessToPWM(byte aBrightness);

  byte bytesPerPixel() const {
    return (type == SK6812RGBW) ? 4 : 3;
  }

  uint16_t freqkHz() const {
    return (type == WS2811 || type == TM1803) ? 400 : 800;
  }

 private:
  bool begun;         // true if begin() previously called
  
  uint16_t numLEDs;       // Number of RGB LEDs in strip
  uint16_t numBytes;      // Size of 'pixels' buffer below
  const uint8_t type;          // Pixel type flag (400 vs 800 KHz)
  uint8_t brightness;
  uint8_t* pixels;        // Holds LED color values (3 bytes each)

  DECLARE_DRIVER;
};
