/*-------------------------------------------------------------------------
  Spark Core, Photon, P1 and Electron library to control WS2811/WS2812 based RGB
  LED devices such as Adafruit NeoPixel strips.
  Currently handles 800 KHz and 400kHz bitstream on Spark Core and Photon,
  WS2812, WS2812B and WS2811.

  Also supports:
  - Radio Shack Tri-Color Strip with TM1803 controller 400kHz bitstream.
  - TM1829 pixels

  PLEASE NOTE that the NeoPixels require 5V level inputs
  and the Spark Core, Photon, P1 and Electron only have 3.3V level outputs.
  Level shifting is necessary, but will require a fast device such as one
  of the following:

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

#ifndef SPARK_NEOPIXEL_H
#define SPARK_NEOPIXEL_H

#include "application.h"

// 'type' flags for LED pixels (third parameter to constructor):
#define WS2812   0x02 // 800 KHz datastream (NeoPixel)
#define WS2812B  0x02 // 800 KHz datastream (NeoPixel)
#define WS2811   0x00 // 400 KHz datastream (NeoPixel)
#define TM1803   0x03 // 400 KHz datastream (Radio Shack Tri-Color Strip)
#define TM1829   0x04 // 800 KHz datastream ()
#define WS2812B2 0x05 // 800 KHz datastream (NeoPixel)

class Adafruit_NeoPixel {

 public:

  // Constructor: number of LEDs, pin number, LED type
  Adafruit_NeoPixel(uint16_t n, uint8_t p=2, uint8_t t=WS2812B);
  ~Adafruit_NeoPixel();

  void
    begin(void),
    show(void) __attribute__((optimize("Ofast"))),
    setPin(uint8_t p),
    setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b),
    setPixelColor(uint16_t n, uint32_t c),
    setBrightness(uint8_t),
    setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue),
    setColorScaled(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aScaling),
    setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aBrightness),
    clear(void);
  uint8_t
   *getPixels() const,
    getBrightness(void) const;
  uint16_t
    numPixels(void) const,
    getNumLeds(void) const;
  static uint32_t
    Color(uint8_t r, uint8_t g, uint8_t b);
  uint32_t
    getPixelColor(uint16_t n) const;
  byte
    brightnessToPWM(byte aBrightness);

 private:

  const uint16_t
    numLEDs,       // Number of RGB LEDs in strip
    numBytes;      // Size of 'pixels' buffer below
  const uint8_t
    type;          // Pixel type flag (400 vs 800 KHz)
  uint8_t
    pin,           // Output pin number
    brightness,
   *pixels;        // Holds LED color values (3 bytes each)
  uint32_t
    endTime;       // Latch timing reference
};

#endif // ADAFRUIT_NEOPIXEL_H
