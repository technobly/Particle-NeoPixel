/*-------------------------------------------------------------------------
  Spark Core library to control WS2812 based RGB
  LED devices such as Adafruit NeoPixel strips.
  Currently handles 800 KHz bitstream on Spark Core, 
  with LEDs wired for GRB color order.

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
  Modified to work with Spark Core by Technobly.
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

class Adafruit_NeoPixel {

 public:

  // Constructor: number of LEDs, pin number, LED type
  Adafruit_NeoPixel(uint16_t n, uint8_t p=6);
  ~Adafruit_NeoPixel();

  void
    begin(void),
    show(void),
    setPin(uint8_t p),
    setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b),
    setPixelColor(uint16_t n, uint32_t c),
    setBrightness(uint8_t);
  uint8_t
   *getPixels() const;
  uint16_t
    numPixels(void) const;
  static uint32_t
    Color(uint8_t r, uint8_t g, uint8_t b);
  uint32_t
    getPixelColor(uint16_t n) const;

 private:

  const uint16_t
    numLEDs,       // Number of RGB LEDs in strip
    numBytes;      // Size of 'pixels' buffer below
  uint8_t
    pin,           // Output pin number
    brightness,
   *pixels;        // Holds LED color values (3 bytes each)
  uint32_t
    endTime;       // Latch timing reference
};

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

Adafruit_NeoPixel::Adafruit_NeoPixel(uint16_t n, uint8_t p) : numLEDs(n), numBytes(n), pin(p), pixels(NULL)
{
  if((pixels = (uint8_t *)malloc(numBytes))) {
    memset(pixels, 0, numBytes);
  }
}

Adafruit_NeoPixel::~Adafruit_NeoPixel() {
  if(pixels) free(pixels);
  pinMode(pin, INPUT);
}

void Adafruit_NeoPixel::begin(void) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void Adafruit_NeoPixel::show(void) {
  if(!pixels) return;

  // Data latch = 50+ microsecond pause in the output stream.  Rather than
  // put a delay at the end of the function, the ending time is noted and
  // the function will simply hold off (if needed) on issuing the
  // subsequent round of data until the latch time has elapsed.  This
  // allows the mainline code to start generating the next frame of data
  // rather than stalling for the latch.
  while((micros() - endTime) < 50L);
  // endTime is a private member (rather than global var) so that multiple
  // instances on different pins can be quickly issued in succession (each
  // instance doesn't delay the next).

  noInterrupts(); // Need 100% focus on instruction timing

  volatile uint32_t 
    c,    // 24-bit pixel color
    mask; // 8-bit mask
  volatile uint16_t i = numBytes; // Output loop counter
  volatile uint8_t
    j,              // 8-bit inner loop counter
   *ptr = pixels,   // Pointer to next byte
    g,              // Current green byte value
    r,              // Current red byte value
    b;              // Current blue byte value
  
  while(i) { // While bytes left...
    mask = 0x1000000; // reset the mask, start 1 higher than
    i--; // decrement bytes remaining
    g = *ptr++;   // Next green byte value
    r = *ptr++;   // Next red byte value
    b = *ptr++;   // Next blue byte value
    c = ((uint32_t)g << 16) | ((uint32_t)r <<  8) | b; // Pack the next 3 bytes to keep timing tight
    for (j=0; j<24; j++) { // iterate through 24-bits of next pixel, MSB to LSB.
      if (c & (mask >>= 1)) { // mask shifts first, then & with c
        // 700ns HIGH (meas. 694ns)
        PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin; // HIGH
        asm volatile(
          "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" 
          "nop" "\n\t" "nop" "\n\t"
          ::: "r0", "cc", "memory");
        // 600ns LOW (meas. 598ns)
        PIN_MAP[pin].gpio_peripheral->BRR = PIN_MAP[pin].gpio_pin; // LOW
      } else {
        // 350ns HIGH (meas. 360ns)
        PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin; // HIGH
        asm volatile(
          "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t"
          ::: "r0", "cc", "memory");
        // 800ns LOW (meas. 792ns)
        PIN_MAP[pin].gpio_peripheral->BRR = PIN_MAP[pin].gpio_pin; // LOW
        asm volatile(
          "mov r0, r0" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          "nop" "\n\t" "nop" "\n\t" "nop" "\n\t" "nop" "\n\t"
          ::: "r0", "cc", "memory");
      }
    }
  } // end while(i)

  interrupts();
  endTime = micros(); // Save EOD time for latch on next call
}

// Set the output pin number
void Adafruit_NeoPixel::setPin(uint8_t p) {
  pinMode(pin, INPUT);
  pin = p;
  pinMode(p, OUTPUT);
  digitalWrite(p, LOW);
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
    *p++ = g;
    *p++ = r;
    *p = b;
  }
}

// Set pixel color from 'packed' 32-bit RGB color:
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
    uint8_t *p = &pixels[n * 3];
    *p++ = g;
    *p++ = r;
    *p = b;
  }
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
uint32_t Adafruit_NeoPixel::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

// Query color from previously-set pixel (returns packed 32-bit RGB value)
uint32_t Adafruit_NeoPixel::getPixelColor(uint16_t n) const {

  if(n < numLEDs) {
    uint16_t ofs = n * 3;
    return (uint32_t)(pixels[ofs + 2]) |
      ((uint32_t)(pixels[ofs    ]) <<  8) |
      ((uint32_t)(pixels[ofs + 1]) << 16);
  }

  return 0; // Pixel # is out of bounds
}

uint8_t *Adafruit_NeoPixel::getPixels(void) const {
  return pixels;
}

uint16_t Adafruit_NeoPixel::numPixels(void) const {
  return numLEDs;
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

/* ======================= SparkPixel.cpp ======================= */
#define PIN D0

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Pixels are wired for GRB bitstream
// 800 KHz bitstream (e.g. High Density LED strip) - WS2812 (6-pin part)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(3, PIN);

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Some example procedures showing how to display to the pixels:
  // Do not run more than one of these at a time, or the b/g tasks will be blocked.
  //-------------------------------------------------------------------------------
  //strip.setPixelColor(0, strip.Color(255, 0, 255));
  //strip.show();
  
  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  
  //rainbow(20);
  
  rainbowCycle(20);
}

// Fill the dots one after the other with a color
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

// Slightly different, this makes the rainbow equally distributed throughout
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