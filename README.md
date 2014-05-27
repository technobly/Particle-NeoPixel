SparkCore-NeoPixel
==================

A library for manipulating NeoPixel RGB LEDs.
An Implementation of Adafruit's NeoPixel Library for the Spark Core
Library currently supports WS2812, WS2812B GRB 800kHz style pixels, strips and sticks!
WS2811 RGB 400kHz style pixels, strips and sticks!

Components Required
---
- A Neopixel digital RGB LED (get at [adafruit.com](adafruit.com))
- A Spark Shield Shield or breakout board to supply neopixel's with 5V (see store at [spark.io](spark.io))

Example Usage
---

See this [flashable, annotated example](firmware/examples/annotated-example.cpp) for details, or, in a nutshell:

    Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEOPIXEL_TYPE);
    void setup() {
      strip.begin();
      strip.show();
    }
    void loop() {
      // cool stuff
    }

Nuances
---

- Make sure get the # of pixels, pin number, type of pixels correct

- NeoPixels require 5V level inputs and the Spark Core only has 3.3V level outputs. Level shifting is
necessary, the Spark Shield Shield has the [TXB0108PWR](http://www.digikey.com/product-search/en?pv7=2&k=TXB0108PWR) 3.3V to 5V level shifter built in, alternatively you can wire up your own with a [SN74HCT245N](http://www.digikey.com/product-detail/en/SN74HCT245N/296-1612-5-ND/277258).


Building locally
---

If you are building locally, place the files here:

```
..\core-firmware\inc\Spark_NeoPixel.h
..\core-firmware\src\application.cpp (renamed from Spark_StrandTest.cpp)
..\core-firmware\src\Spark_NeoPixel.cpp
..\core-firmware\src\build.mk (optional, if you have your own make file going, just add the Spark_NeoPixel.cpp to it)
```

Useful Links
---

- NeoPixel Guide: https://learn.adafruit.com/adafruit-neopixel-uberguide
- Level Translator Breakout Board: https://www.adafruit.com/products/395


