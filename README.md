SparkCore-NeoPixel
==================

An Implementation of Adafruit's NeoPixel Library for the Spark Core

Currently supports:
WS2812, WS2812B GRB 800kHz style pixels, strips and sticks!
WS2811 RGB 400kHz style pixels, strips and sticks!

Grab the RAW version of each file and place into your web IDE as follows:
![image](http://i.imgur.com/SN2pRAc.png)

If you are building locally, place the files here:

```
..\core-firmware\inc\Spark_NeoPixel.h
..\core-firmware\src\application.cpp (renamed from Spark_StrandTest.cpp)
..\core-firmware\src\Spark_NeoPixel.cpp
..\core-firmware\src\build.mk (optional, if you have your own make file going, just add the Spark_NeoPixel.cpp to it)
```

PLEASE NOTE that the NeoPixels require 5V level inputs and the 
Spark Core only has 3.3V level outputs. Level shifting is 
necessary, but will require a fast device such as one of the following:

[SN74HCT245N](http://www.digikey.com/product-detail/en/SN74HCT245N/296-1612-5-ND/277258)

[TXB0108PWR](http://www.digikey.com/product-search/en?pv7=2&k=TXB0108PWR)

If you have a Spark Shield Shield, the TXB0108PWR 3.3V to 5V level shifter is built in.