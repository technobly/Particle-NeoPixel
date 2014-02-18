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
