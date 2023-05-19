/**
 * This is a minimal example, see extra-examples.cpp for a version
 * with more explantory documentation, example routines, how to
 * hook up your pixels and all of the pixel types that are supported.
 *
 * On Photon, Electron, P1, Core and Duo, any pin can be used for Neopixel.
 *
 * On the Argon, Boron and Xenon, only these pins can be used for Neopixel:
 * - D2, D3, A4, A5
 * - D4, D6, D7, D8
 * - A0, A1, A2, A3
 *
 * On Photon 2 / P2, only SPI(MOSI) or SPI1(D2) can be used for Neopixel,
 * and only PIXEL_TYPE's WS2812, WS2812B, WS2813 are supported.
 * - MISO (D12), SCK (D13) and SS (D8) can be used as GPIO when using SPI
 * - MISO1 (D3), SCK1 (D4) and SS1 (D5) can be used and GPIO when using SPI1
 * note: You may want to call System.disableFeature(FEATURE_ETHERNET_DETECTION);
 *       to disable automatic Ethernet driver detection, which will cause some
 *       glitches to SPI1 pins D2,D3,D4.
 *
 * In addition on the Argon/Boron/Xenon, only one pin per group can be used at a time.
 * So it's OK to have one Adafruit_NeoPixel instance on pin D2 and another one on pin
 * A2, but it's not possible to have one on pin A0 and another one on pin A1.
 */

#include "Particle.h"
#include "neopixel.h"

SYSTEM_MODE(AUTOMATIC);

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#if (PLATFORM_ID == 32)
// MOSI pin MO
#define PIXEL_PIN SPI
// MOSI pin D2
// #define PIXEL_PIN SPI1
#else // #if (PLATFORM_ID == 32)
#define PIXEL_PIN D3
#endif
#define PIXEL_COUNT 11
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// Prototypes for local build, ok to leave in for Build IDE
void rainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);

void setup()
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop()
{
  rainbow(20);
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
