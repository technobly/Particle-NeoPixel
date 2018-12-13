/*-------------------------------------------------------------------------
  Particle Core, Particle Photon, P1, Electron, Argon, Boron, Xenon and
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

/* ======================= neopixel.cpp ======================= */
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
#elif (PLATFORM_ID == 12) || (PLATFORM_ID == 13) || (PLATFORM_ID == 14) // Argon (12), Boron (13), Xenon (14)
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "pinmap_impl.h"
  NRF5x_Pin_Info* PIN_MAP2 = HAL_Pin_Map();
  #define pinLO(_pin) (nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(PIN_MAP2[_pin].gpio_port, PIN_MAP2[_pin].gpio_pin)))
  #define pinHI(_pin) (nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(PIN_MAP2[_pin].gpio_port, PIN_MAP2[_pin].gpio_pin)))
#else
  #error "*** PLATFORM_ID not supported by this library. PLATFORM should be Particle Core, Photon, Electron, Argon, Boron, Xenon and RedBear Duo ***"
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
    case WS2812B: // WS2812, WS2812B & WS2813 = 300us reset pulse
    case WS2812B2: {
        wait_time = 300L;
      } break;
    case WS2811: // WS2811, WS2812B_FAST & WS2812B2_FAST = 50us reset pulse
    case WS2812B_FAST:
    case WS2812B2_FAST:
    default: {   // default = 50us reset pulse
        wait_time = 50L;
      } break;
  }
  while((micros() - endTime) < wait_time);
  // endTime is a private member (rather than global var) so that multiple
  // instances on different pins can be quickly issued in succession (each
  // instance doesn't delay the next).

#if (PLATFORM_ID == 0) || (PLATFORM_ID == 6) || (PLATFORM_ID == 8) || (PLATFORM_ID == 10) || (PLATFORM_ID == 88) // Core (0), Photon (6), P1 (8), Electron (10) or Redbear Duo (88)
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

  if(type == WS2812B || type == WS2812B_FAST) { // Same as WS2812 & WS2813, 800 KHz bitstream
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
  else if(type == WS2812B2 || type == WS2812B2_FAST) { // WS2812B with DWT timer
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
#elif (PLATFORM_ID == 12) || (PLATFORM_ID == 13) || (PLATFORM_ID == 14) // Argon (12), Boron (13), Xenon (14)
// [[[Begin of the Neopixel NRF52 EasyDMA implementation
//                                    by the Hackerspace San Salvador]]]
// This technique uses the PWM peripheral on the NRF52. The PWM uses the
// EasyDMA feature included on the chip. This technique loads the duty 
// cycle configuration for each cycle when the PWM is enabled. For this 
// to work we need to store a 16 bit configuration for each bit of the
// RGB(W) values in the pixel buffer.
// Comparator values for the PWM were hand picked and are guaranteed to
// be 100% organic to preserve freshness and high accuracy. Current 
// parameters are:
//   * PWM Clock: 16Mhz
//   * Minimum step time: 62.5ns
//   * Time for zero in high (T0H): 0.31ms
//   * Time for one in high (T1H): 0.75ms
//   * Cycle time:  1.25us
//   * Frequency: 800Khz
// For 400Khz we just double the calculated times.
// ---------- BEGIN Constants for the EasyDMA implementation -----------
// The PWM starts the duty cycle in LOW. To start with HIGH we
// need to set the 15th bit on each register.

// WS2812 (rev A) timing is 0.35 and 0.7us
//#define MAGIC_T0H               5UL | (0x8000) // 0.3125us
//#define MAGIC_T1H              12UL | (0x8000) // 0.75us

// WS2812B (rev B) timing is 0.4 and 0.8 us
#define MAGIC_T0H               6UL | (0x8000) // 0.375us
#define MAGIC_T1H              13UL | (0x8000) // 0.8125us

// WS2811 (400 khz) timing is 0.5 and 1.2
#define MAGIC_T0H_400KHz        8UL  | (0x8000) // 0.5us
#define MAGIC_T1H_400KHz        19UL | (0x8000) // 1.1875us

// For 400Khz, we double value of CTOPVAL
#define CTOPVAL                20UL            // 1.25us
#define CTOPVAL_400KHz         40UL            // 2.5us

// ---------- END Constants for the EasyDMA implementation -------------
// 
// If there is no device available an alternative cycle-counter
// implementation is tried.
// The nRF52832 runs with a fixed clock of 64Mhz. The alternative
// implementation is the same as the one used for the Teensy 3.0/1/2 but
// with the Nordic SDK HAL & registers syntax.
// The number of cycles was hand picked and is guaranteed to be 100% 
// organic to preserve freshness and high accuracy.
// ---------- BEGIN Constants for cycle counter implementation ---------
#define CYCLES_800_T0H  18  // ~0.36 uS
#define CYCLES_800_T1H  41  // ~0.76 uS
#define CYCLES_800      71  // ~1.25 uS

#define CYCLES_400_T0H  26  // ~0.50 uS
#define CYCLES_400_T1H  70  // ~1.26 uS
#define CYCLES_400      156 // ~2.50 uS
// ---------- END of Constants for cycle counter implementation --------

  // To support both the SoftDevice + Neopixels we use the EasyDMA
  // feature from the NRF25. However this technique implies to
  // generate a pattern and store it on the memory. The actual
  // memory used in bytes corresponds to the following formula:
  //              totalMem = numBytes*8*2+(2*2)
  // The two additional bytes at the end are needed to reset the
  // sequence.
  //
  // If there is not enough memory, we will fall back to cycle counter
  // using DWT
  uint32_t  pattern_size   = numBytes*8*sizeof(uint16_t)+2*sizeof(uint16_t);
  uint16_t* pixels_pattern = NULL;

  NRF_PWM_Type* pwm = NULL;

  // Try to find a free PWM device, which is not enabled
  // and has no connected pins
  NRF_PWM_Type* PWM[3] = {NRF_PWM0, NRF_PWM1, NRF_PWM2};
  for(int device = 0; device<3; device++) {
    if( (PWM[device]->ENABLE == 0)                            &&
        (PWM[device]->PSEL.OUT[0] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[1] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[2] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[3] & PWM_PSEL_OUT_CONNECT_Msk)
    ) {
      pwm = PWM[device];
      break;
    }
  }
  
  // only malloc if there is PWM device available
  if ( pwm != NULL ) {
    #ifdef ARDUINO_FEATHER52 // use thread-safe malloc
      pixels_pattern = (uint16_t *) rtos_malloc(pattern_size);
    #else
      pixels_pattern = (uint16_t *) malloc(pattern_size);
    #endif
  }

  // Use the identified device to choose the implementation
  // If a PWM device is available use DMA
  if( (pixels_pattern != NULL) && (pwm != NULL) ) {
    uint16_t pos = 0; // bit position

    for(uint16_t n=0; n<numBytes; n++) {
      uint8_t pix = pixels[n];

      for(uint8_t mask=0x80, i=0; mask>0; mask >>= 1, i++) {
        #ifdef NEO_KHZ400
        if( !is800KHz ) {
          pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H_400KHz : MAGIC_T0H_400KHz;
        }else
        #endif
        {
          pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H : MAGIC_T0H;
        }

        pos++;
      }
    }

    // Zero padding to indicate the end of que sequence
    pixels_pattern[++pos] = 0 | (0x8000); // Seq end
    pixels_pattern[++pos] = 0 | (0x8000); // Seq end

    // Set the wave mode to count UP
    pwm->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);

    // Set the PWM to use the 16MHz clock
    pwm->PRESCALER = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);

    // Setting of the maximum count
    // but keeping it on 16Mhz allows for more granularity just
    // in case someone wants to do more fine-tuning of the timing.
#ifdef NEO_KHZ400
    if( !is800KHz ) {
      pwm->COUNTERTOP = (CTOPVAL_400KHz << PWM_COUNTERTOP_COUNTERTOP_Pos);
    }else
#endif
    {
      pwm->COUNTERTOP = (CTOPVAL << PWM_COUNTERTOP_COUNTERTOP_Pos);
    }

    // Disable loops, we want the sequence to repeat only once
    pwm->LOOP = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);

    // On the "Common" setting the PWM uses the same pattern for the
    // for supported sequences. The pattern is stored on half-word
    // of 16bits
    pwm->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) |
                   (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    // Pointer to the memory storing the patter
    pwm->SEQ[0].PTR = (uint32_t)(pixels_pattern) << PWM_SEQ_PTR_PTR_Pos;

    // Calculation of the number of steps loaded from memory.
    pwm->SEQ[0].CNT = (pattern_size/sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos;

    // The following settings are ignored with the current config.
    pwm->SEQ[0].REFRESH  = 0;
    pwm->SEQ[0].ENDDELAY = 0;

    // The Neopixel implementation is a blocking algorithm. DMA
    // allows for non-blocking operation. To "simulate" a blocking
    // operation we enable the interruption for the end of sequence
    // and block the execution thread until the event flag is set by
    // the peripheral.
//    pwm->INTEN |= (PWM_INTEN_SEQEND0_Enabled<<PWM_INTEN_SEQEND0_Pos);

    // PSEL must be configured before enabling PWM
    pwm->PSEL.OUT[0] = NRF_GPIO_PIN_MAP(PIN_MAP2[pin].gpio_port, PIN_MAP2[pin].gpio_pin);

    // Enable the PWM
    pwm->ENABLE = 1;

    // After all of this and many hours of reading the documentation
    // we are ready to start the sequence...
    pwm->EVENTS_SEQEND[0]  = 0;
    pwm->TASKS_SEQSTART[0] = 1;

    // But we have to wait for the flag to be set.
    while(!pwm->EVENTS_SEQEND[0])
    {
      #ifdef ARDUINO_FEATHER52
      yield();
      #endif
    }

    // Before leave we clear the flag for the event.
    pwm->EVENTS_SEQEND[0] = 0;

    // We need to disable the device and disconnect
    // all the outputs before leave or the device will not
    // be selected on the next call.
    // TODO: Check if disabling the device causes performance issues.
    pwm->ENABLE = 0;

    pwm->PSEL.OUT[0] = 0xFFFFFFFFUL;

    #ifdef ARDUINO_FEATHER52  // use thread-safe free
      rtos_free(pixels_pattern);
    #else
      free(pixels_pattern);
    #endif
  }// End of DMA implementation
  // ---------------------------------------------------------------------
  else{
    // Fall back to DWT
    #ifdef ARDUINO_FEATHER52
      // Bluefruit Feather 52 uses freeRTOS
      // Critical Section is used since it does not block SoftDevice execution
      taskENTER_CRITICAL();
    #elif defined(NRF52_DISABLE_INT)
      // If you are using the Bluetooth SoftDevice we advise you to not disable
      // the interrupts. Disabling the interrupts even for short periods of time
      // causes the SoftDevice to stop working.
      // Disable the interrupts only in cases where you need high performance for
      // the LEDs and if you are not using the EasyDMA feature.
      __disable_irq();
    #endif

    uint32_t pinMask = 1UL << NRF_GPIO_PIN_MAP(PIN_MAP2[pin].gpio_port, PIN_MAP2[pin].gpio_pin);


    uint32_t CYCLES_X00     = CYCLES_800;
    uint32_t CYCLES_X00_T1H = CYCLES_800_T1H;
    uint32_t CYCLES_X00_T0H = CYCLES_800_T0H;

#ifdef NEO_KHZ400
    if( !is800KHz )
    {
      CYCLES_X00     = CYCLES_400;
      CYCLES_X00_T1H = CYCLES_400_T1H;
      CYCLES_X00_T0H = CYCLES_400_T0H;
    }
#endif

    // Enable DWT in debug core
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Tries to re-send the frame if is interrupted by the SoftDevice.
    while(1) {
      uint8_t *p = pixels;

      uint32_t cycStart = DWT->CYCCNT;
      uint32_t cyc = 0;

      for(uint16_t n=0; n<numBytes; n++) {
        uint8_t pix = *p++;

        for(uint8_t mask = 0x80; mask; mask >>= 1) {
          while(DWT->CYCCNT - cyc < CYCLES_X00);
          cyc  = DWT->CYCCNT;

          NRF_GPIO->OUTSET |= pinMask;

          if(pix & mask) {
            while(DWT->CYCCNT - cyc < CYCLES_X00_T1H);
          } else {
            while(DWT->CYCCNT - cyc < CYCLES_X00_T0H);
          }

          NRF_GPIO->OUTCLR |= pinMask;
        }
      }
      while(DWT->CYCCNT - cyc < CYCLES_X00);


      // If total time longer than 25%, resend the whole data.
      // Since we are likely to be interrupted by SoftDevice
      if ( (DWT->CYCCNT - cycStart) < ( 8*numBytes*((CYCLES_X00*5)/4) ) ) {
        break;
      }

      // re-send need 300us delay
      delayMicroseconds(300);
    }

    // Enable interrupts again
    #ifdef ARDUINO_FEATHER52
      taskEXIT_CRITICAL();
    #elif defined(NRF52_DISABLE_INT)
      __enable_irq();
    #endif
  }
// END of NRF52 implementation


#endif
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
    uint8_t *p = &pixels[n * (type==SK6812RGBW?4:3)];
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
    uint8_t *p = &pixels[n * (type==SK6812RGBW?4:3)];
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

  uint8_t *p = &pixels[n * (type==SK6812RGBW?4:3)];
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
