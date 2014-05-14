/*-------------------------------------------------------------------------
  Spark Core library to control WS2811/WS2812 based RGB
  LED devices such as Adafruit NeoPixel strips.
  Currently handles 800 KHz and 400kHz bitstream on Spark Core, 
  WS2812, WS2812B and WS2811.
  
  PLEASE NOTE that the NeoPixels require 5V level inputs 
  and the Spark Core only has 3.3V level outputs. Level shifting is
  necessary, but will require a fast device such as one of the following:

  [SN74HCT245N] 
  http://www.digikey.com/product-detail/en/SN74HCT245N/296-1612-5-ND/277258

  [TXB0108PWR] 
  http://www.digikey.com/product-search/en?pv7=2&k=TXB0108PWR

  If you have a Spark Shield Shield, the TXB0108PWR 3.3V to 5V level 
  shifter is built in.

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
  Modified to work with Spark Core by Technobly.
  Contributions by PJRC and other members of the open source community.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!
  --------------------------------------------------------------------*/

/* ======================= Includes ================================= */

#include "application.h"

#include "neopixel/neopixel.h"

/* ======================= Prototype Defs =========================== */

void colorAll(uint32_t c, uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void rainbow(uint8_t wait);
void rainbowCycle(uint8_t wait);
uint32_t Wheel(byte WheelPos);

/* ======================= Spark_StrandTest.cpp ===================== */

#define PIN D2

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
//               note: if not specified, D2 is selected for you.
// Parameter 3 = pixel type [ WS2812, WS2812B, WS2811 ]
//               note: if not specified, WS2812B is selected for you.
//               note: RGB order is automatically applied to WS2811,
//                     WS2812/WS2812B is GRB order.
//
// 800 KHz bitstream 800 KHz bitstream (most NeoPixel products ...
//                         ... WS2812 (6-pin part)/WS2812B (4-pin part) )
//
// 400 KHz bitstream (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, PIN, WS2812B);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Some example procedures showing how to display to the pixels:
  // Do not run more than one of these at a time, or the b/g tasks 
  // will be blocked.
  //--------------------------------------------------------------
  //strip.setPixelColor(0, strip.Color(255, 0, 255));
  //strip.show();
  
  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  
  rainbow(20);
  
  //rainbowCycle(20);
  
  //colorAll(strip.Color(0, 255, 255), 50); // Magenta
}

// Set all pixels in the strip to a solid color, then wait (ms)
void colorAll(uint32_t c, uint8_t wait) {
  uint16_t i;
  
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
  delay(wait);
}

// Fill the dots one after the other with a color, wait (ms) after each one
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
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

// Slightly different, this makes the rainbow equally distributed throughout, then wait (ms)
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) { // 1 cycle of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
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
