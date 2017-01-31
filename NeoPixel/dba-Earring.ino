#include <Adafruit_NeoPixel.h>

#define PIN 6
#define lowc 20

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  xmas();
  // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(lowc, 0, 0), 50); // Red
  colorWipe(strip.Color(0, lowc, 0), 50); // Green
  //alternate();
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  //rainbow(20);
  //rainbowCycle(20);
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

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
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

// Fill the dots one after the other with a color
void xmas() {
  // make the strip red
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(lowc, 0, 0));
      strip.show();
   }
  // green dot moving down
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      if (i > 0) {
        strip.setPixelColor(i-1, strip.Color(lowc, 0, 0));
      }  
      strip.setPixelColor(i, strip.Color(0, lowc, 0));
      strip.show();
      delay(100);
  }
  // green dot moving back
  for(uint16_t i=(strip.numPixels()-1); i>0; i--) {
      if (i < strip.numPixels()-1) {
        strip.setPixelColor(i+1, strip.Color(lowc, 0, 0));
      }  
      strip.setPixelColor(i, strip.Color(0, lowc, 0));
      strip.show();
      delay(100);
  }
 // blank the strip 
 for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
   }
 strip.show();

} // xmas

// Fill the dots one after the other with a color
void alternate() {
int i;

for(i=0; i<10; i++) {
  for(uint16_t i=0; i<strip.numPixels(); i+=2) {
      strip.setPixelColor(i, strip.Color(50, 0, 0));
   }
  for(uint16_t i=1; i<strip.numPixels(); i+=2) {
      strip.setPixelColor(i, strip.Color(0, 50, 0));
   }
 strip.show();
 delay(200);
  for(uint16_t i=0; i<strip.numPixels(); i+=2) {
      strip.setPixelColor(i, strip.Color(0, 50, 0));
   }
  for(uint16_t i=1; i<strip.numPixels(); i+=2) {
      strip.setPixelColor(i, strip.Color(50, 0, 0));
   }
 strip.show();
 delay(200);
}
}
