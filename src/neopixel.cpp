/*-------------------------------------------------------------------------
  Spark Core, Particle Photon, P1, Electron and RedBear Duo library to control
  WS2811/WS2812 based RGB LED devices such as Adafruit NeoPixel strips.

  Supports:
  - 800 KHz and 400kHz bitstream WS2812, WS2812B and WS2811
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

#include "neopixel.h"

#if PLATFORM_ID == 0 // Core (0)
  #define pinLO(_pin) (PIN_MAP[_pin].gpio_peripheral->BRR = PIN_MAP[_pin].gpio_pin)
  #define pinHI(_pin) (PIN_MAP[_pin].gpio_peripheral->BSRR = PIN_MAP[_pin].gpio_pin)
#elif (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
  STM32_Pin_Info* PIN_MAP2 = HAL_Pin_Map(); // Pointer required for highest access speed
  #define pinLO(_pin) (PIN_MAP2[_pin].gpio_peripheral->BSRRH = PIN_MAP2[_pin].gpio_pin)
  #define pinHI(_pin) (PIN_MAP2[_pin].gpio_peripheral->BSRRL = PIN_MAP2[_pin].gpio_pin)
#else
  #error "*** PLATFORM_ID not supported by this library. PLATFORM should be Core, Photon, P1, Electron or RedBear Duo ***"
#endif
// fast pin access
#define pinSet(_pin, _hilo) (_hilo ? pinHI(_pin) : pinLO(_pin))

Adafruit_NeoPixel::Adafruit_NeoPixel(uint16_t n, uint8_t p, uint8_t t) :
  begun(false), type(t), brightness(0), pixels(NULL), endTime(0)
{
  updateLength(n);
  setPin(p);
}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
  if (pixels) free(pixels);
  if (begun) pinMode(pin, INPUT);
}

void Adafruit_NeoPixel::updateLength(uint16_t n) {
  if (pixels) free(pixels); // Free existing data (if any)

  // Allocate new data -- note: ALL PIXELS ARE CLEARED
  numBytes = n * ((type == SK6812RGBW) ? 4 : 3);
  if ((pixels = (uint8_t *)malloc(numBytes))) {
    memset(pixels, 0, numBytes);
    numLEDs = n;
  } else {
    numLEDs = numBytes = 0;
  }
}

void Adafruit_NeoPixel::begin(void) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  begun = true;
}

// Set the output pin number
void Adafruit_NeoPixel::setPin(uint8_t p) {
    if (begun) {
        pinMode(pin, INPUT);
    }
    pin = p;
    if (begun) {
        pinMode(p, OUTPUT);
        digitalWrite(p, LOW);
    }
}

void Adafruit_NeoPixel::show(void) {
  if(!pixels) return;

  // Data latch = 24 or 50 microsecond pause in the output stream.  Rather than
  // put a delay at the end of the function, the ending time is noted and
  // the function will simply hold off (if needed) on issuing the
  // subsequent round of data until the latch time has elapsed.  This
  // allows the mainline code to start generating the next frame of data
  // rather than stalling for the latch.
  uint32_t wait_time; // wait time in microseconds.
  switch(type) {
    case TM1803: { // TM1803 = 24us reset pulse
        wait_time = 24L;
      } break;
    case SK6812RGBW: { // SK6812RGBW = 80us reset pulse
        wait_time = 80L;
      } break;
    case TM1829: { // TM1829 = 500us reset pulse
        wait_time = 500L;
      } break;
    case WS2812B: // WS2812 & WS2812B = 50us reset pulse
    case WS2812B2:
    case WS2811: // WS2811 = 50us reset pulse
    default: {   // default = 50us reset pulse
        wait_time = 50L;
      } break;
  }
  while((micros() - endTime) < wait_time);
  // endTime is a private member (rather than global var) so that multiple
  // instances on different pins can be quickly issued in succession (each
  // instance doesn't delay the next).

  __disable_irq(); // Need 100% focus on instruction timing

  volatile uint32_t
    c,    // 24-bit/32-bit pixel color
    mask; // 1-bit mask
  volatile uint16_t i = numBytes; // Output loop counter
  volatile uint8_t
    j,              // 8-bit inner loop counter
   *ptr = pixels,   // Pointer to next byte
    g,              // Current green byte value
    r,              // Current red byte value
    b,              // Current blue byte value
    w;              // Current white byte value

  if(type == WS2812B) { // same as WS2812, 800 KHz bitstream
    while(i) { // While bytes left... (3 bytes = 1 pixel)
      mask = 0x800000; // reset the mask
      i = i-3;      // decrement bytes remaining
      g = *ptr++;   // Next green byte value
      r = *ptr++;   // Next red byte value
      b = *ptr++;   // Next blue byte value
      c = ((uint32_t)g << 16) | ((uint32_t)r <<  8) | b; // Pack the next 3 bytes to keep timing tight
      j = 0;        // reset the 24-bit counter
      do {
        pinSet(pin, HIGH); // HIGH
        if (c & mask) { // if masked bit is high
          // WS2812 spec             700ns HIGH
          // Adafruit on Arduino    (meas. 812ns)
          // This lib on Spark Core (meas. 804ns)
          // This lib on Photon     (meas. 792ns)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          // WS2812 spec             600ns LOW
          // Adafruit on Arduino    (meas. 436ns)
          // This lib on Spark Core (meas. 446ns)
          // This lib on Photon     (meas. 434ns)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        else { // else masked bit is low
          // WS2812 spec             350ns HIGH
          // Adafruit on Arduino    (meas. 312ns)
          // This lib on Spark Core (meas. 318ns)
          // This lib on Photon     (meas. 308ns)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          // WS2812 spec             800ns LOW
          // Adafruit on Arduino    (meas. 938ns)
          // This lib on Spark Core (meas. 944ns)
          // This lib on Photon     (meas. 936ns)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        mask >>= 1;
      } while ( ++j < 24 ); // ... pixel done
    } // end while(i) ... no more pixels
  }
  else if(type == SK6812RGBW) { // similar to WS2812, 800 KHz bitstream but with RGB+W components
    while(i) { // While bytes left... (4 bytes = 1 pixel)
      mask = 0x80000000; // reset the mask
      i = i-4;      // decrement bytes remaining
      r = *ptr++;   // Next red byte value
      g = *ptr++;   // Next green byte value
      b = *ptr++;   // Next blue byte value
      w = *ptr++;   // Next white byte value
      c = ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b <<  8) | w; // Pack the next 4 bytes to keep timing tight
      j = 0;        // reset the 32-bit counter
      do {
        pinSet(pin, HIGH); // HIGH
        if (c & mask) { // if masked bit is high
          // SK6812RGBW spec         600ns HIGH
          // WS2812 spec             700ns HIGH
          // Adafruit on Arduino    (meas. 812ns)
          // This lib on Spark Core (meas. 610ns)
          // This lib on Photon     (meas. 608ns)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          // SK6812RGBW spec         600ns LOW
          // WS2812 spec             600ns LOW
          // Adafruit on Arduino    (meas. 436ns)
          // This lib on Spark Core (meas. 598ns)
          // This lib on Photon     (meas. 600ns)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        else { // else masked bit is low
          // SK6812RGBW spec         300ns HIGH
          // WS2812 spec             350ns HIGH
          // Adafruit on Arduino    (meas. 312ns)
          // This lib on Spark Core (meas. 305ns)
          // This lib on Photon     (meas. 308ns)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          // SK6812RGBW spec         900ns LOW
          // WS2812 spec             800ns LOW
          // Adafruit on Arduino    (meas. 938ns)
          // This lib on Spark Core (meas. 904ns)
          // This lib on Photon     (meas. 900ns)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        mask >>= 1;
      } while ( ++j < 32 ); // ... pixel done
    } // end while(i) ... no more pixels
  }
  else if(type == WS2812B2) { // WS2812B with DWT timer
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
    #define CYCLES_800_T0H  25 // 312ns (meas. 300ns)
    #define CYCLES_800_T0L  70 // 938ns (meas. 940ns)
    #define CYCLES_800_T1H  80 // 812ns (meas. 792ns)
    #define CYCLES_800_T1L  8  // 436ns (meas. 425ns)

    volatile uint32_t cyc;

    while(i) { // While bytes left... (3 bytes = 1 pixel)
      mask = 0x800000; // reset the mask
      i = i-3;      // decrement bytes remaining
      g = *ptr++;   // Next green byte value
      r = *ptr++;   // Next red byte value
      b = *ptr++;   // Next blue byte value
      c = ((uint32_t)g << 16) | ((uint32_t)r <<  8) | b; // Pack the next 3 bytes to keep timing tight
      j = 0;        // reset the 24-bit counter
      do {
        cyc = DWT->CYCCNT;
        pinSet(pin, HIGH); // HIGH
        if (c & mask) { // if masked bit is high
          while(DWT->CYCCNT - cyc < CYCLES_800_T1H);
          pinSet(pin, LOW);
          cyc = DWT->CYCCNT;
          while(DWT->CYCCNT - cyc < CYCLES_800_T1L);
        }
        else { // else masked bit is low
          while(DWT->CYCCNT - cyc < CYCLES_800_T0H);
          pinSet(pin, LOW);
          cyc = DWT->CYCCNT;
          while(DWT->CYCCNT - cyc < CYCLES_800_T0L);
        }
        mask >>= 1;
      } while ( ++j < 24 ); // ... pixel done
    } // end while(i) ... no more pixels
#endif
  }
  else if(type == WS2811) { // WS2811, 400 KHz bitstream
    while(i) { // While bytes left... (3 bytes = 1 pixel)
      mask = 0x800000; // reset the mask
      i = i-3;      // decrement bytes remaining
      r = *ptr++;   // Next red byte value
      g = *ptr++;   // Next green byte value
      b = *ptr++;   // Next blue byte value
      c = ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b; // Pack the next 3 bytes to keep timing tight
      j = 0;        // reset the 24-bit counter
      do {
        pinSet(pin, HIGH); // HIGH
        if (c & mask) { // if masked bit is high
          // WS2811 spec             1.20us HIGH
          // Adafruit on Arduino    (meas. 1.25us)
          // This lib on Spark Core (meas. 1.25us)
          // This lib on Photon     (meas. 1.25us)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          // WS2811 spec             1.30us LOW
          // Adafruit on Arduino    (meas. 1.25us)
          // This lib on Spark Core (meas. 1.24us)
          // This lib on Photon     (meas. 1.24us)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        else { // else masked bit is low
          // WS2811 spec             500ns HIGH
          // Adafruit on Arduino    (meas. 500ns)
          // This lib on Spark Core (meas. 500ns)
          // This lib on Photon     (meas. 500ns)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#endif
            "nop" "\n\t" "nop" "\n\t"
            ::: "r0", "cc", "memory");
          // WS2811 spec             2.000us LOW
          // Adafruit on Arduino    (meas. 2.000us)
          // This lib on Spark Core (meas. 2.000us)
          // This lib on Photon     (meas. 2.000us)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        mask >>= 1;
      } while ( ++j < 24 ); // ... pixel done
    } // end while(i) ... no more pixels
  }
  else if(type == TM1803) { // TM1803 (Radio Shack Tri-Color Strip), 400 KHz bitstream
    while(i) { // While bytes left... (3 bytes = 1 pixel)
      mask = 0x800000; // reset the mask
      i = i-3;      // decrement bytes remaining
      r = *ptr++;   // Next red byte value
      g = *ptr++;   // Next blue byte value
      b = *ptr++;   // Next green byte value
      c = ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b; // Pack the next 3 bytes to keep timing tight
      j = 0;        // reset the 24-bit counter
      do {
        pinSet(pin, HIGH); // HIGH
        if (c & mask) { // if masked bit is high
          // TM1803 spec             1.36us HIGH
          // Pololu on Arduino      (meas. 1.31us)
          // This lib on Spark Core (meas. 1.36us)
          // This lib on Photon     (meas. 1.36us)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          // TM1803 spec             680ns LOW
          // Pololu on Arduino      (meas. 1.024us)
          // This lib on Spark Core (meas. 680ns)
          // This lib on Photon     (meas. 684ns)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        else { // else masked bit is low
          // TM1803 spec             680ns HIGH
          // Pololu on Arduino      (meas. 374ns)
          // This lib on Spark Core (meas. 680ns)
          // This lib on Photon     (meas. 684ns)
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          // TM1803 spec             1.36us LOW
          // Pololu on Arduino      (meas. 2.00us)
          // This lib on Spark Core (meas. 1.36us)
          // This lib on Photon     (meas. 1.36us)
          pinSet(pin, LOW); // LOW
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
        }
        mask >>= 1;
      } while ( ++j < 24 ); // ... pixel done
    } // end while(i) ... no more pixels
  }
  else { // must be only other option TM1829, 800 KHz bitstream
    while(i) { // While bytes left... (3 bytes = 1 pixel)
      mask = 0x800000; // reset the mask
      i = i-3;      // decrement bytes remaining
      r = *ptr++;   // Next red byte value
      b = *ptr++;   // Next blue byte value
      g = *ptr++;   // Next green byte value
      c = ((uint32_t)r << 16) | ((uint32_t)b <<  8) | g; // Pack the next 3 bytes to keep timing tight
      j = 0;        // reset the 24-bit counter
      pinSet(pin, LOW); // LOW
      for( ;; ) {   // ... pixel done
        if (c & mask) { // if masked bit is high
          // TM1829 spec             800ns LOW
          // This lib on Spark Core (meas. 806ns)
          // This lib on Photon     (meas. 792ns)
          mask >>= 1; // Do this task during the long delay of this bit
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          j++;
          // TM1829 spec             300ns HIGH
          // This lib on Spark Core (meas. 305ns)
          // This lib on Photon     (meas. 300ns)
          pinSet(pin, HIGH); // HIGH
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          if(j==24) break;
          pinSet(pin, LOW); // LOW
        }
        else { // else masked bit is low
          // TM1829 spec             300ns LOW
          // This lib on Spark Core (meas. 390ns)
          // This lib on Photon     (meas. 300ns)
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
          asm volatile(
            "mov r0, r0" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t"
            ::: "r0", "cc", "memory");
#endif
          // TM1829 spec             800ns HIGH
          // This lib on Spark Core (meas. 792ns)
          // This lib on Photon     (meas. 800ns)
          pinSet(pin, HIGH); // HIGH
          j++;
          mask >>= 1; // Do this task during the long delay of this bit
          asm volatile(
            "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#if (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
            "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
#endif
            ::: "r0", "cc", "memory");
          if(j==24) break;
          pinSet(pin, LOW); // LOW
        }
      }
    } // end while(i) ... no more pixels
  }

  __enable_irq();
  endTime = micros(); // Save EOD time for latch on next call
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
      case WS2812B: // WS2812 & WS2812B is GRB order.
      case WS2812B2:
        *p++ = g;
        *p++ = r;
        *p = b;
        break;
      case TM1829: // TM1829 is special RBG order
        if(r == 255) r = 254; // 255 on RED channel causes display to be in a special mode.
        *p++ = r;
        *p++ = b;
        *p = g;
        break;
      case WS2811: // WS2811 is RGB order
      case TM1803: // TM1803 is RGB order
      default:     // default is RGB order
        *p++ = r;
        *p++ = g;
        *p = b;
        break;
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
    uint8_t *p = &pixels[n * (type==SK6812RGBW?4:3)];
    switch(type) {
      case WS2812B: // WS2812 & WS2812B is GRB order.
      case WS2812B2:
        *p++ = g;
        *p++ = r;
        *p = b;
        break;
      case TM1829: // TM1829 is special RBG order
        if(r == 255) r = 254; // 255 on RED channel causes display to be in a special mode.
        *p++ = r;
        *p++ = b;
        *p = g;
        break;
      case SK6812RGBW: // SK6812RGBW is RGBW order
        *p++ = r;
        *p++ = g;
        *p++ = b;
        *p = w;
        break;
      case WS2811: // WS2811 is RGB order
      case TM1803: // TM1803 is RGB order
      default:     // default is RGB order
        *p++ = r;
        *p++ = g;
        *p = b;
        break;
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
    uint8_t *p = &pixels[n * (type==SK6812RGBW?4:3)];
    switch(type) {
      case WS2812B: // WS2812 & WS2812B is GRB order.
      case WS2812B2: {
          *p++ = g;
          *p++ = r;
          *p = b;
        }
        break;
      case TM1829: { // TM1829 is special RBG order
          if(r == 255) r = 254; // 255 on RED channel causes display to be in a special mode.
          *p++ = r;
          *p++ = b;
          *p = g;
        }
        break;
      case SK6812RGBW: { // SK6812RGBW is RGBW order
          uint8_t w = (uint8_t)(c >> 24);
          *p++ = r;
          *p++ = g;
          *p++ = b;
          *p = brightness ? ((w * brightness) >> 8) : w;
        }
        break;
      case WS2811: // WS2811 is RGB order
      case TM1803: // TM1803 is RGB order
      default: {   // default is RGB order
          *p++ = r;
          *p++ = g;
          *p = b;
        }
        break;
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

  uint8_t *p = &pixels[n * (type==SK6812RGBW?4:3)];
  uint32_t c;

  switch(type) {
    case WS2812B: // WS2812 & WS2812B is GRB order.
    case WS2812B2: {
        c = ((uint32_t)p[1] << 16) | ((uint32_t)p[0] <<  8) | (uint32_t)p[2];
      }
      break;
    case TM1829: { // TM1829 is special RBG order
        c = ((uint32_t)p[0] << 16) | ((uint32_t)p[2] <<  8) | (uint32_t)p[1];
      }
      break;
    case SK6812RGBW: { // SK6812RGBW is RGBW order, but returns packed WRGB color
        c = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] <<  8) | (uint32_t)p[3];
      }
      break;
    case WS2811: // WS2811 is RGB order
    case TM1803: // TM1803 is RGB order
    default: {   // default is RGB order
        c = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] <<  8) | (uint32_t)p[2];
      }
      break;
  }

  // Adjust this back up to the true color, as setting a pixel color will
  // scale it back down again.
  if(brightness) { // See notes in setBrightness()
    //Cast the color to a byte array
    uint8_t * c_ptr =reinterpret_cast<uint8_t*>(&c);
    if (type == SK6812RGBW) {
      c_ptr[3] = (c_ptr[3] << 8)/brightness;
    }
    c_ptr[0] = (c_ptr[0] << 8)/brightness;
    c_ptr[1] = (c_ptr[1] << 8)/brightness;
    c_ptr[2] = (c_ptr[2] << 8)/brightness;
  }
  return c; // Pixel # is out of bounds
}

uint8_t *Adafruit_NeoPixel::getPixels(void) const {
  return pixels;
}

uint16_t Adafruit_NeoPixel::numPixels(void) const {
  return numLEDs;
}

uint16_t Adafruit_NeoPixel::getNumLeds(void) const {
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
uint8_t Adafruit_NeoPixel::getBrightness(void) const {
  return brightness - 1;
}

void Adafruit_NeoPixel::clear(void) {
  memset(pixels, 0, numBytes);
}
