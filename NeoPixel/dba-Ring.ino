#include <Adafruit_NeoPixel.h>

#define PIN 6  // the digital pin the data line is connected to

// Modifed NeoPixel sample for the holiday craft project

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

#define low_red strip.Color(3, 0, 0)
#define low_green strip.Color(0, 3, 0)
#define med_red strip.Color(20, 0, 0)
#define med_green strip.Color(0, 20, 0)
#define hi_red strip.Color(40, 0, 0)
#define hi_green strip.Color(0, 40, 0)

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  //Serial.begin(9600);
}

void loop() {
  uint16_t i, j;
  int num_steps = 100;
if (1) {
  fade_up(100, 30, 30, 0, 0);
  fade_up(100, 30, 0, 30, 0);
  fade_up(100, 30, 30, 0, 0);
  fade_up(100, 30, 0, 30, 0);
  
  colorWipe(med_red, 100); // Red
  colorWipe(med_green, 100); // Green
  colorWipe(med_red, 100); // Red
  colorWipe(med_green, 100); // Green
  
  for (j=0; j<2; j++) {
    //for(i=0; i<strip.numPixels()+3; i++) {
    //  if (i<strip.numPixels()) strip.setPixelColor(i, hi_red);
    //  if ((i-1 >= 0) && (i-1 < strip.numPixels())) strip.setPixelColor(i-1, med_red);
    //  if ((i-2 >= 0) && (i-2 < strip.numPixels())) strip.setPixelColor(i-2, low_red);
    //  if ((i-3 >= 0) && (i-3 < strip.numPixels())) strip.setPixelColor(i-3, med_green);
    //  strip.show();
    //  delay(100);
    //}
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, hi_red);
      if ((i-1) >= 0) { 
        strip.setPixelColor(i-1, med_red);
      } else {
        strip.setPixelColor(strip.numPixels()-1, med_red);
      }
      if ((i-2) >= 0) { 
        strip.setPixelColor(i-2, med_red);
      } else {
        strip.setPixelColor(strip.numPixels()-2, med_red);
      }
      if ((i-3) >= 0) strip.setPixelColor(i-3, med_green);
      strip.show();
      delay(100);
    }
    strip.setPixelColor(strip.numPixels()-1, med_red);
    strip.setPixelColor(strip.numPixels()-2, low_red);
    strip.setPixelColor(strip.numPixels()-3, med_green);
    strip.show();
    delay(100);

    strip.setPixelColor(strip.numPixels()-1, low_red);
    strip.setPixelColor(strip.numPixels()-2, med_green);
    strip.show();
    delay(100);

    strip.setPixelColor(strip.numPixels()-1, med_green);
    strip.show();
    delay(100);
  }  

  colorWipe(strip.Color(80, 80, 80), 100);
  colorWipe(strip.Color(0, 0, 30), 100);

  soft_twinkles(1000, 10, 8, 7, 1);
}
  smooth();
} // loop

void smooth() {
  int num_steps = 10000;
  float step_size = 0.008;
  uint16_t i, k;
  float x_pos = 0.0;
  float width = 2.3;
  uint8_t val;
  float dist;
  
  for (k=0; k<num_steps; k++) {
    for(i=0; i<strip.numPixels(); i++) {
      dist = x_pos - float(i); // separating this out since abs does not do well with functions inside
      val = 255.0 * max(0, width - abs(dist)) / width;
      strip.setPixelColor(i, val, 0, 0);
    }
    strip.show();
    //delay(20);
    x_pos += step_size;
    if (x_pos > strip.numPixels()) x_pos = 0.0;
  }

  
} // smooth

// soft_twinkles - a soft colored random twinkle pattern
// loops controls how many times to run through the twinkle loop
// odds is the one in n chance of a black one to light up
// the rgb increment ones are the color val to add for each loop up and down  8, 7, 1 is good for soft white
void soft_twinkles(uint16_t num_twinkles, uint16_t odds, uint8_t inc_r, uint8_t inc_g, uint8_t inc_b) {
  uint8_t R, G, B;
  uint16_t i, k;
  uint32_t c;
  uint8_t direction[strip.numPixels()];  // 0 = still, 1 = brighter, 2 = dimmer
  
  // init the direction array and the strip
  for(i=0; i<strip.numPixels(); i++) {
    direction[i] = 0;
    strip.setPixelColor(i, 0, 0, 0);
  }
  
  for (k=0; k<num_twinkles; k++) {
    // loop through the strip, incrementing and decrementing the color as needed
    for(i=0; i<strip.numPixels(); i++) {
      if (direction[i] == 0) continue; // skip pixel if it is not currently twinkling
      c = strip.getPixelColor(i);
      R = (c >> 16);
      G = (c >>  8);
      B = (c      );
      if (direction[i] == 2) { // dimming
        R = max(0, R - inc_r);
        G = max(0, G - inc_g);
        B = max(0, B - inc_b);
        //if (i == 1) {
        //  Serial.println("Dimming");
        //}
        strip.setPixelColor(i, R, G, B);
        if ((R == 0) && (G == 0) && (B == 0)) direction[i] = 0;
      } else { // brightening: direction[i] == 1
        R = min(255, R + inc_r);
        G = min(255, G + inc_g);
        B = min(255, B + inc_b);
        /*if (i == 1) {
        Serial.print("i: ");
        Serial.print(i);
        Serial.print(", R: ");
        Serial.print(R);
        Serial.print(", G: ");
        Serial.print(G);
        Serial.print(", B: ");
        Serial.println(B);
        } */
        strip.setPixelColor(i, R, G, B);
        if ((R == 255) || (G == 255) || (B == 255)) direction[i] = 2;
      }
    }
  
    // add a pixel if the odds hit
    if (random(odds) == 1) {
      i = random(strip.numPixels());
      if (!strip.getPixelColor(i)) {
        strip.setPixelColor(i, inc_r, inc_g, inc_b);
        direction[i] = 1;
      }
    }

  strip.show();
  delay(20);
  }
} // settwinkles


// fade_up - fade up to the given color
void fade_up(int num_steps, int wait, int R, int G, int B) {
   uint16_t i, j;
   
   for (i=0; i<num_steps; i++) {
      for(j=0; j<strip.numPixels(); j++) {
         strip.setPixelColor(j, strip.Color(R * i / num_steps, G * i / num_steps, B * i / num_steps));
      }  
   strip.show();
   delay(wait);
   }  
} // fade_up


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}


