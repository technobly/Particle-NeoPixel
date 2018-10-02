/*-------------------------------------------------------------------------
  Spark Core, Particle Photon, P1, Electron, Argon, Boron, Xenon and
  RedBear Duo library to control WS2811/WS2812/WS2813 based RGB LED
  devices such as Adafruit NeoPixel strips.

  Supports:
  - 800 KHz WS2812, WS2812B, WS2813 and 400kHz bitstream and WS2811
  - 800 KHz bitstream SK6812RGBW (NeoPixel RGBW pixel strips)
    (use 'SK6812RGBW' as PIXEL_TYPE)

  Also supports:
  - Radio Shack Tri-Color Strip with TM1803 controller 400kHz bitstream.
  - TM1829 pixels

  PLEASE NOTE that the NeoPixels require 5V level inputs
  and the supported microcontrollers only have 3.3V level outputs. Level
  shifting is necessary, but will require a fast device such as one of
  the following:

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

/* ======================= Adafruit_NeoPixel.cpp ======================= */
/*-------------------------------------------------------------------------
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
  -------------------------------------------------------------------------*/

// FIXME: remove before release
#ifndef PLATFORM_ID
#define PLATFORM_ID 14
#define USE_NRF_PWM
#endif

#include "neopixel.h"

Adafruit_NeoPixel::Adafruit_NeoPixel(uint16_t n, uint8_t p, uint8_t t) :
  begun(false), type(t), brightness(0), pixels(NULL), driver(p, t)
{
  updateLength(n);
}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
  updateLength(0);
  if (begun) end();
}

void Adafruit_NeoPixel::updateLength(uint16_t n) {
  numBytes = n * bytesPerPixel();

  driver.updateLength(numBytes);

  if (pixels) {
    free(pixels); // Free existing data (if any)
    pixels = NULL;
  }

  if (n == 0) {
    return;
  }

  // Allocate new data -- note: ALL PIXELS ARE CLEARED
  pixels = (uint8_t*)malloc(numBytes);
  if (pixels) {
    memset(pixels, 0, numBytes);
    numLEDs = n;
  } else {
    numLEDs = numBytes = 0;
  }
}

void Adafruit_NeoPixel::begin() {
  driver.begin();
  begun = true;
}

void Adafruit_NeoPixel::end() {
  driver.end();
  begun = false;
}

// Set the output pin number
void Adafruit_NeoPixel::setPin(uint8_t p) {
  bool wasBegun = begun;
  if (wasBegun) {
      end();
  }
  driver.setPin(p);
  if (wasBegun) {
      begin();
  }
}

void Adafruit_NeoPixel::show() {
  // Data latch = 24 or 50 microsecond pause in the output stream.  Rather than
  // put a delay at the end of the function, the ending time is noted and
  // the function will simply hold off (if needed) on issuing the
  // subsequent round of data until the latch time has elapsed.  This
  // allows the mainline code to start generating the next frame of data
  // rather than stalling for the latch.
  uint32_t waitTime; // wait time in microseconds.
  switch(type) {
    case TM1803: { // TM1803 = 24us reset pulse
        waitTime = 24L;
      } break;
    case SK6812RGBW: { // SK6812RGBW = 80us reset pulse
        waitTime = 80L;
      } break;
    case TM1829: { // TM1829 = 500us reset pulse
        waitTime = 500L;
      } break;
    case WS2812B: // WS2812, WS2812B & WS2813 = 300us reset pulse
    case WS2812B2: {
        waitTime = 300L;
      } break;
    case WS2811: // WS2811, WS2812B_FAST & WS2812B2_FAST = 50us reset pulse
    case WS2812B_FAST:
    case WS2812B2_FAST:
    default: {   // default = 50us reset pulse
        waitTime = 50L;
      } break;
  }
  driver.show(pixels, numBytes, waitTime);
}

// Set pixel color from separate R,G,B components:
void Adafruit_NeoPixel::setPixelColor(
  uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if(n < numLEDs) {
    if(brightness) { // See notes in setBrightness()
      r = (r * brightness) >> 8;
      g = (g * brightness) >> 8;
      b = (b * brightness) >> 8;
    }
    uint8_t *p = &pixels[n * 3];
    switch(type) {
      case WS2812B: // WS2812, WS2812B & WS2813 is GRB order.
      case WS2812B_FAST:
      case WS2812B2:
      case WS2812B2_FAST: {
          *p++ = g;
          *p++ = r;
          *p = b;
        } break;
      case TM1829: { // TM1829 is special RBG order
          if(r == 255) r = 254; // 255 on RED channel causes display to be in a special mode.
          *p++ = r;
          *p++ = b;
          *p = g;
        } break;
      case WS2811: // WS2811 is RGB order
      case TM1803: // TM1803 is RGB order
      default: {   // default is RGB order
          *p++ = r;
          *p++ = g;
          *p = b;
        } break;
    }
  }
}

// Set pixel color from separate R,G,B,W components:
void Adafruit_NeoPixel::setPixelColor(
  uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if(n < numLEDs) {
    if(brightness) { // See notes in setBrightness()
      r = (r * brightness) >> 8;
      g = (g * brightness) >> 8;
      b = (b * brightness) >> 8;
      w = (w * brightness) >> 8;
    }
    uint8_t *p = &pixels[n * bytesPerPixel()];
    switch(type) {
      case WS2812B: // WS2812, WS2812B & WS2813 is GRB order.
      case WS2812B_FAST:
      case WS2812B2:
      case WS2812B2_FAST: {
          *p++ = g;
          *p++ = r;
          *p = b;
        } break;
      case TM1829: { // TM1829 is special RBG order
          if(r == 255) r = 254; // 255 on RED channel causes display to be in a special mode.
          *p++ = r;
          *p++ = b;
          *p = g;
        } break;
      case SK6812RGBW: { // SK6812RGBW is RGBW order
          *p++ = r;
          *p++ = g;
          *p++ = b;
          *p = w;
        } break;
      case WS2811: // WS2811 is RGB order
      case TM1803: // TM1803 is RGB order
      default: {   // default is RGB order
          *p++ = r;
          *p++ = g;
          *p = b;
        } break;
    }
  }
}

// Set pixel color from 'packed' 32-bit RGB color:
// If RGB+W color, order of bytes is WRGB in packed 32-bit form
void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint32_t c) {
  if(n < numLEDs) {
    uint8_t
      r = (uint8_t)(c >> 16),
      g = (uint8_t)(c >>  8),
      b = (uint8_t)c;
    if(brightness) { // See notes in setBrightness()
      r = (r * brightness) >> 8;
      g = (g * brightness) >> 8;
      b = (b * brightness) >> 8;
    }
    uint8_t *p = &pixels[n * bytesPerPixel()];
    switch(type) {
      case WS2812B: // WS2812, WS2812B & WS2813 is GRB order.
      case WS2812B_FAST:
      case WS2812B2:
      case WS2812B2_FAST: {
          *p++ = g;
          *p++ = r;
          *p = b;
        } break;
      case TM1829: { // TM1829 is special RBG order
          if(r == 255) r = 254; // 255 on RED channel causes display to be in a special mode.
          *p++ = r;
          *p++ = b;
          *p = g;
        } break;
      case SK6812RGBW: { // SK6812RGBW is RGBW order
          uint8_t w = (uint8_t)(c >> 24);
          *p++ = r;
          *p++ = g;
          *p++ = b;
          *p = brightness ? ((w * brightness) >> 8) : w;
        } break;
      case WS2811: // WS2811 is RGB order
      case TM1803: // TM1803 is RGB order
      default: {   // default is RGB order
          *p++ = r;
          *p++ = g;
          *p = b;
        } break;
    }
  }
}

void Adafruit_NeoPixel::setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue) {
  return setPixelColor(aLedNumber, (uint8_t) aRed, (uint8_t) aGreen, (uint8_t) aBlue);
}

void Adafruit_NeoPixel::setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aWhite) {
  return setPixelColor(aLedNumber, (uint8_t) aRed, (uint8_t) aGreen, (uint8_t) aBlue, (uint8_t) aWhite);
}

void Adafruit_NeoPixel::setColorScaled(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aScaling) {
  // scale RGB with a common brightness parameter
  setColor(aLedNumber, (aRed*aScaling)>>8, (aGreen*aScaling)>>8, (aBlue*aScaling)>>8);
}

void Adafruit_NeoPixel::setColorScaled(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aWhite, byte aScaling) {
  // scale RGB with a common brightness parameter
  setColor(aLedNumber, (aRed*aScaling)>>8, (aGreen*aScaling)>>8, (aBlue*aScaling)>>8, (aWhite*aScaling)>>8);
}

void Adafruit_NeoPixel::setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aBrightness) {
  setColorScaled(aLedNumber, aRed, aGreen, aBlue, brightnessToPWM(aBrightness));
}

void Adafruit_NeoPixel::setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aWhite, byte aBrightness) {
  setColorScaled(aLedNumber, aRed, aGreen, aBlue, aWhite, brightnessToPWM(aBrightness));
}

byte Adafruit_NeoPixel::brightnessToPWM(byte aBrightness) {
  static const byte pwmLevels[16] = { 0, 1, 2, 3, 4, 6, 8, 12, 23, 36, 48, 70, 95, 135, 190, 255 };
  return pwmLevels[aBrightness>>4];
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

// Convert separate R,G,B,W into packed 32-bit WRGB color.
// Packed format is always WRGB, regardless of LED strand color order.
uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

// Query color from previously-set pixel (returns packed 32-bit RGB value)
uint32_t Adafruit_NeoPixel::getPixelColor(uint16_t n) const {
  if(n >= numLEDs) {
    // Out of bounds, return no color.
    return 0;
  }

  uint8_t *p = &pixels[n * bytesPerPixel()];
  uint32_t c;

  switch(type) {
    case WS2812B: // WS2812, WS2812B & WS2813 is GRB order.
    case WS2812B_FAST:
    case WS2812B2:
    case WS2812B2_FAST: {
        c = ((uint32_t)p[1] << 16) | ((uint32_t)p[0] <<  8) | (uint32_t)p[2];
      } break;
    case TM1829: { // TM1829 is special RBG order
        c = ((uint32_t)p[0] << 16) | ((uint32_t)p[2] <<  8) | (uint32_t)p[1];
      } break;
    case SK6812RGBW: { // SK6812RGBW is RGBW order, but returns packed WRGB color
        c = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] <<  8) | (uint32_t)p[3];
      } break;
    case WS2811: // WS2811 is RGB order
    case TM1803: // TM1803 is RGB order
    default: {   // default is RGB order
        c = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] <<  8) | (uint32_t)p[2];
      } break;
  }

  // Adjust this back up to the true color, as setting a pixel color will
  // scale it back down again.
  if(brightness) { // See notes in setBrightness()
    //Cast the color to a byte array
    uint8_t * c_ptr =reinterpret_cast<uint8_t*>(&c);
    if (bytesPerPixel() == 4) {
      c_ptr[3] = (c_ptr[3] << 8)/brightness;
    }
    c_ptr[0] = (c_ptr[0] << 8)/brightness;
    c_ptr[1] = (c_ptr[1] << 8)/brightness;
    c_ptr[2] = (c_ptr[2] << 8)/brightness;
  }
  return c; // Pixel # is out of bounds
}

uint8_t *Adafruit_NeoPixel::getPixels() const {
  return pixels;
}

uint16_t Adafruit_NeoPixel::numPixels() const {
  return numLEDs;
}

uint16_t Adafruit_NeoPixel::getNumLeds() const {
  return numPixels();
}

// Adjust output brightness; 0=darkest (off), 255=brightest.  This does
// NOT immediately affect what's currently displayed on the LEDs.  The
// next call to show() will refresh the LEDs at this level.  However,
// this process is potentially "lossy," especially when increasing
// brightness.  The tight timing in the WS2811/WS2812 code means there
// aren't enough free cycles to perform this scaling on the fly as data
// is issued.  So we make a pass through the existing color data in RAM
// and scale it (subsequent graphics commands also work at this
// brightness level).  If there's a significant step up in brightness,
// the limited number of steps (quantization) in the old data will be
// quite visible in the re-scaled version.  For a non-destructive
// change, you'll need to re-render the full strip data.  C'est la vie.
void Adafruit_NeoPixel::setBrightness(uint8_t b) {
  // Stored brightness value is different than what's passed.
  // This simplifies the actual scaling math later, allowing a fast
  // 8x8-bit multiply and taking the MSB.  'brightness' is a uint8_t,
  // adding 1 here may (intentionally) roll over...so 0 = max brightness
  // (color values are interpreted literally; no scaling), 1 = min
  // brightness (off), 255 = just below max brightness.
  uint8_t newBrightness = b + 1;
  if(newBrightness != brightness) { // Compare against prior value
    // Brightness has changed -- re-scale existing data in RAM
    uint8_t  c,
            *ptr           = pixels,
             oldBrightness = brightness - 1; // De-wrap old brightness value
    uint16_t scale;
    if(oldBrightness == 0) scale = 0; // Avoid /0
    else if(b == 255) scale = 65535 / oldBrightness;
    else scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
    for(uint16_t i=0; i<numBytes; i++) {
      c      = *ptr;
      *ptr++ = (c * scale) >> 8;
    }
    brightness = newBrightness;
  }
}

//Return the brightness value
uint8_t Adafruit_NeoPixel::getBrightness() const {
  return brightness - 1;
}

void Adafruit_NeoPixel::clear() {
  memset(pixels, 0, numBytes);
}
