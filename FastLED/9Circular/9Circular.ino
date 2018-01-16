/*
 * A FastLED example showing how to
 * map a virtual 2d matrix onto a circular led setup
 *
 * limitations: works so far only up to 255 leds
 *
 * written by Stefan Petrick 2016
 */

#include "FastLED.h"

#define DATA_PIN    7
#define CLOCK_PIN   14
#define BRIGHTNESS  255
#define LED_TYPE    APA102
#define COLOR_ORDER BGR
#define NUM_LEDS    144

CRGB leds[NUM_LEDS];

uint8_t x[NUM_LEDS];                              // arrays for the 2d coordinates of any led
uint8_t y[NUM_LEDS];

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER, DATA_RATE_MHZ(12)>(leds, NUM_LEDS);
  LEDS.setBrightness(BRIGHTNESS);

  for (uint16_t i = 0; i < NUM_LEDS; i++) {       // precalculate the lookup-tables:
    uint8_t angle = (i * 256) / NUM_LEDS;         // on which position on the circle is the led?
    x[i] = cos8( angle );                         // corrsponding x position in the matrix
    y[i] = sin8( angle );                         // corrsponding y position in the matrix
  }
}

void loop() {

  for (uint16_t i = 0; i < 5000; i++) {
    example_1();
    FastLED.show();
  }

  for (uint16_t i = 0; i < 5000; i++) {
    example_2();
    FastLED.show();
  }

  for (uint16_t i = 0; i < 5000; i++) {
    example_3();
    FastLED.show();
  }
}


// moves a noise up and down while slowly shifting to the side
void example_1() {

  uint8_t scale = 1000;                               // the "zoom factor" for the noise

  for (uint16_t i = 0; i < NUM_LEDS; i++) {

    uint16_t shift_x = beatsin8(17);                  // the x position of the noise field swings @ 17 bpm
    uint16_t shift_y = millis() / 100;                // the y position becomes slowly incremented

    uint32_t real_x = (x[i] + shift_x) * scale;       // calculate the coordinates within the noise field
    uint32_t real_y = (y[i] + shift_y) * scale;       // based on the precalculated positions

    uint8_t noise = inoise16(real_x, real_y, 4223) >> 8;           // get the noise data and scale it down

    uint8_t index = noise * 3;                        // map led color based on noise data
    uint8_t bri   = noise;

    CRGB color = CHSV( index, 255, bri);
    leds[i] = color;
  }
}

// just moving along one axis = "lavalamp effect"
void example_2() {

  uint8_t scale = 1000;                               // the "zoom factor" for the noise

  for (uint16_t i = 0; i < NUM_LEDS; i++) {

    uint16_t shift_x = millis() / 10;                 // x as a function of time
    uint16_t shift_y = 0;

    uint32_t real_x = (x[i] + shift_x) * scale;       // calculate the coordinates within the noise field
    uint32_t real_y = (y[i] + shift_y) * scale;       // based on the precalculated positions

    uint8_t noise = inoise16(real_x, real_y, 4223) >> 8;           // get the noise data and scale it down

    uint8_t index = noise * 3;                        // map led color based on noise data
    uint8_t bri   = noise;

    CRGB color = CHSV( index, 255, bri);
    leds[i] = color;
  }
}

// no x/y shifting but scrolling along z
void example_3() {

  uint8_t scale = 1000;                               // the "zoom factor" for the noise

  for (uint16_t i = 0; i < NUM_LEDS; i++) {

    uint16_t shift_x = 0;                             // no movement along x and y
    uint16_t shift_y = 0;


    uint32_t real_x = (x[i] + shift_x) * scale;       // calculate the coordinates within the noise field
    uint32_t real_y = (y[i] + shift_y) * scale;       // based on the precalculated positions
    
    uint32_t real_z = millis() * 20;                  // increment z linear

    uint8_t noise = inoise16(real_x, real_y, real_z) >> 8;           // get the noise data and scale it down

    uint8_t index = noise * 3;                        // map led color based on noise data
    uint8_t bri   = noise;

    CRGB color = CHSV( index, 255, bri);
    leds[i] = color;
  }
}
