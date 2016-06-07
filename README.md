Particle-NeoPixel
=================

A library for manipulating NeoPixel RGB LEDs for the Spark Core, Particle Photon, P1 and Electron.
Also now supports the [RedBear Duo](https://github.com/redbear/Duo).
Implementation based on Adafruit's NeoPixel Library.

Supported Pixel Types
---
- 800 KHz and 400kHz bitstream WS2812, WS2812B and WS2811
- 800 KHz bitstream SK6812RGBW (NeoPixel RGBW pixel strips)
  - (use 'SK6812RGBW' as PIXEL_TYPE)

Also supports these less common pixels
---
- Radio Shack Tri-Color LED Strip with TM1803 controller 400kHz bitstream.
- TM1829 pixels, many [details here.](https://community.particle.io/t/neopixel-library-for-tm1829-controller-resolved/5363)
- Some functions from the [MessageTorch library](https://github.com/plan44/messagetorch/blob/master/messagetorch.cpp#L58-L134) have been added.
- SK6812MINI "NeoPixel Mini" (use 'WS2812B' as PIXEL_TYPE)

Components Required
---
- A Neopixel digital RGB LED (get at [adafruit.com](https://www.adafruit.com))
- or a Radio Shack Tri-Color LED Strip (get at [radioshack.com](https://www.radioshack.com))
- A Particle Shield Shield or breakout board to supply neopixel's with 5V (see store at [particle.io](https://www.particle.io))

Example Usage
---

See this [flashable, rainbow example](firmware/examples/a-rainbow.cpp) for details, or, in a nutshell:

```cpp
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);
void setup() {
  strip.begin();
  strip.show();
}
void loop() {
  // change your pixel colors and call strip.show() again
}
```

More Detailed Example Usage
---

```cpp
// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_COUNT 10
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B

// Parameter 1 = number of pixels in strip
//               note: for some stripes like those with the TM1829, you
//                     need to count the number of segments, i.e. the
//                     number of controllers in your stripe, not the number
//                     of individual LEDs!
// Parameter 2 = pin number (most are valid)
//               note: if not specified, D2 is selected for you.
// Parameter 3 = pixel type [ WS2812, WS2812B, WS2812B2, WS2811,
//                             TM1803, TM1829, SK6812RGBW ]
//               note: if not specified, WS2812B is selected for you.
//               note: RGB order is automatically applied to WS2811,
//                     WS2812/WS2812B/WS2812B2/TM1803 is GRB order.
//
// 800 KHz bitstream 800 KHz bitstream (most NeoPixel products
//               WS2812 (6-pin part)/WS2812B (4-pin part)/SK6812RGBW (RGB+W) )
//
// 400 KHz bitstream (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//                   (Radio Shack Tri-Color LED Strip - TM1803 driver
//                    NOTE: RS Tri-Color LED's are grouped in sets of 3)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

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
  // Do not run more than 15 seconds of these, or the b/g tasks
  // will be blocked.
  //--------------------------------------------------------------

  // Note, these are not built into the library, check the example
  // files for these helper functions
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  colorWipe(strip.Color(0, 0, 255), 50); // Blue

  // SK6812RGBW type strips can do RGB+W
  // Here we only turn on the WHITE LEDs
  colorWipe(strip.Color(0, 0, 0, 255), 50); // White

  rainbow(20);
}
```

Nuances
---

- Make sure get the # of pixels, pin number, type of pixels correct
- NeoPixels require 5V logic level inputs and the Spark Core and Photon only have 3.3V logic level digital outputs. Level shifting from 3.3V to 5V is
necessary, the Particle Shield Shield has the [TXB0108PWR](http://www.digikey.com/product-search/en?pv7=2&k=TXB0108PWR) 3.3V to 5V level shifter built in (but has been known to oscillate at 50MHz with wire length longer than 6"), alternatively you can wire up your own with a [SN74HCT245N](http://www.digikey.com/product-detail/en/SN74HCT245N/296-1612-5-ND/277258), or [SN74HCT125N](http://www.digikey.com/product-detail/en/SN74HCT125N/296-8386-5-ND/376860). These are rock solid.
- Don't use `getPixelColor()` to move pixel data around when you are also using `setBrightness()`.  When the brightness is set, all `setPixelColor()` calls will end up scaling colors to dim them before they are stored in memory.  When using `getPixelColor()` the stored dimmed color is rescaled back up to the original color.  However, due to some loss of precision with the math, it is not possible to recreate this color data perfectly.  This is especially true with low brightness values.  If you `get` and `set` color data repeatedly with a dimmed pixel, it will eventually continue to decrease in value until it is equal to zero.
- When changing the brightness, always call `setPixelColor()` first with fresh un-dimmed color data, then call `setBrightness()`, and finally `show()`.


Building locally
---

If you are building locally, place the files here:

```

../firmware/user/neo/extra-examples.cpp
../firmware/user/neo/neopixel.h
../firmware/user/neo/neopixel.cpp
```

Compile and program via DFU over USB with:

```
cd firmware/modules
make clean all PLATFORM=photon APP=neo -s program-dfu
make clean all PLATFORM=P1 APP=neo -s program-dfu
make clean all PLATFORM=electron APP=neo -s program-dfu
make clean all PLATFORM=core APP=neo -s program-dfu
```

Compile and flash OTA with the CLI locally:

```
git clone https://github.com/technobly/SparkCore-NeoPixel.git
cd SparkCore-NeoPixel/firmware
// make sure to edit the example to use #include "neopixel.h"
particle flash <device-name> neopixel.cpp neopixel.h examples/rgbw-strandtest.cpp
```

Useful Links
---

- NeoPixel Guide: https://learn.adafruit.com/adafruit-neopixel-uberguide
- Quad Level Shifter IC: [SN74ACHT125N](https://www.adafruit.com/product/1787) (Adafruit)
- Quad Level Shifter IC: [SN74HCT125N](http://www.digikey.com/product-detail/en/SN74HCT125N/296-8386-5-ND/376860) (Digikey)
- Quad Level Shifter IC: [SN74AHCT125N](http://www.digikey.com/product-detail/en/SN74AHCT125N/296-4655-5-ND/375798) (Digikey)
