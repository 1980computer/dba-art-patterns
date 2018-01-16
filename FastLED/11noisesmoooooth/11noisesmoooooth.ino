/*
  A FastLED matrix example:
  A simplex noise field fully modulated and controlled by itself
  written by
  Stefan Petrick 2017
  Do with it whatever you like and show your results to the FastLED community
  https://plus.google.com/communities/109127054924227823508
*/

#include "FastLED.h"

// matrix size
uint8_t Width  = 16;
uint8_t Height = 16;
uint8_t CentreX =  (Width / 2) - 1;
uint8_t CentreY = (Height / 2) - 1;

// NUM_LEDS = Width * Height
#define NUM_LEDS      256
#define BRIGHTNESS    255
CRGB leds[NUM_LEDS];
CRGB buffer1[NUM_LEDS];
CRGB buffer2[NUM_LEDS];

// parameters and buffer for the noise array
#define NUM_LAYERS 2
uint32_t x[NUM_LAYERS];
uint32_t y[NUM_LAYERS];
uint32_t z[NUM_LAYERS];
uint32_t scale_x[NUM_LAYERS];
uint32_t scale_y[NUM_LAYERS];
uint16_t noise[NUM_LAYERS][16][16];

// colortables
uint8_t a[1024];
uint8_t b[1024];
uint8_t c[1024];

//control parameters
uint8_t ctrl[6];

void setup() {

  setup_tables();
  Serial.begin(115200);
  // Adjust this for you own setup. Use the hardware SPI pins if possible.
  // On Teensy 3.1/3.2 the pins are 11 & 13
  // Details here: https://github.com/FastLED/FastLED/wiki/SPI-Hardware-or-Bit-banging
  // In case you see flickering / glitching leds, reduce the data rate to 12 MHZ or less
  LEDS.addLeds<APA102, 11, 13, BGR, DATA_RATE_MHZ(12)>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(DISABLE_DITHER);
  randomSeed(analogRead(5));

  x[0] = random(60000);
  y[0] = random(60000);
  z[0] = random(60000);
  x[1] = random(60000);
  y[1] = random(60000);
  z[1] = random(60000);
}

void loop() {

  //show_palette();

  noise_noise2();
  
  //monitor();
  
  show_fps();
}

void setup_tables() {
  /*
    for (uint16_t i = 0; i < 1024; i++) {
      a[i] = sin8(i/4) ;
      b[i] = 0;
      c[i] = cubicwave8( i/2) ;
    }
  */
  for (uint16_t i = 256; i < 768; i++) {
    a[i] = triwave8(127 + (i / 2)) ;
    //b[i] = 0;
    //c[i] = triwave8(127 + (i / 2)) ;
  }
}

// whatch the serial plotter
void monitor() {
  EVERY_N_MILLIS(50) {
    Serial.print(ctrl[0]);
    Serial.print(" ");
    Serial.print(ctrl[1]);
    Serial.print(" ");
    Serial.print(ctrl[2]);
    Serial.print(" ");
    Serial.print(ctrl[3]);
    Serial.print(" ");
    Serial.print(ctrl[4]);
    Serial.print(" ");
    Serial.print(ctrl[5]);
    Serial.print(" ");
    Serial.println(noise[0][0][0]);
  }
}

// check the "palette"
void show_palette() {

  for (uint8_t y = 0; y < Height; y++) {
    for (uint8_t x = 0; x < Width; x++) {
      leds[XY(x, y)] = CRGB( a[ ((x * 16) + y) * 4], b[ ((x * 16) + y) * 4], c[ ((x * 16) + y) * 4]);
    }
  }
  adjust_gamma();
  FastLED.show();
}

// check the Serial Monitor to see how many fps you get
void show_fps() {
  EVERY_N_MILLIS(100) {
    Serial.println(LEDS.getFPS());
  }
}

// this finds the right index within a serpentine matrix
uint16_t XY( uint8_t x, uint8_t y) {
  uint16_t i;
  if ( y & 0x01) {
    uint8_t reverseX = (Width - 1) - x;
    i = (y * Width) + reverseX;
  } else {
    i = (y * Width) + x;
  }
  return i;
}

/*
  // for a line by line matrix it should be
  uint16_t XY( uint8_t x, uint8_t y)
  {
  uint16_t i;
  i = (y * Width) + x;
  return i;
  }
*/

// cheap correction with gamma 2.0
void adjust_gamma()
{
  // minimal brightness you want to allow
  // make sure to have the global brightnes on maximum and run no other color correction
  // a minimum of min = 1 might work fine for you and allow more contrast
  uint8_t min = 3;
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i].r = dim8_video(leds[i].r);
    leds[i].g = dim8_video(leds[i].g);
    leds[i].b = dim8_video(leds[i].b);

    if (leds[i].r < min) leds[i].r = min;
    if (leds[i].g < min) leds[i].g = min;
    if (leds[i].b < min) leds[i].b = min;
  }
}

// highly experimental
// note that the noise values are in the range of 0-1023 when using them
void noise_noise2() {

  // LAYER ONE
  //top left
  ctrl[0] = (ctrl[0] + noise[0][0][0] + noise[0][1][0] + noise [0][0][1] + noise[0][1][1]) / 20;
  //top right
  ctrl[1] = (ctrl[1] + noise[0][Width - 1][0] + noise[0][Width - 2][0] + noise [0][Width - 1][1] + noise[0][Width - 2][1]) / 20;
  //down left
  ctrl[2] = (ctrl[2] + noise[0][0][Height - 1] + noise[0][0][Height - 2] + noise [0][1][Height - 1] + noise[0][1][Height - 2]) / 20;
  //middle left
  ctrl[3] = (ctrl[3] + noise[0][0][CentreY] + noise[0][1][CentreY] + noise [0][0][CentreY + 1] + noise[0][1][CentreY + 1]) / 20;
  //center
  ctrl[4] = (ctrl[4] + noise[0][Width - 1][CentreY] + noise[0][Width - 2][CentreY] + noise [0][Width - 1][CentreY + 1] + noise[0][Width - 2][CentreY + 1]) / 20;
  ctrl[5] = (ctrl[5] + ctrl[0] + ctrl[1]) / 12;

  x[0] = x[0] + (ctrl[0]) - 127;
  y[0] = y[0] + (ctrl[1]) - 127;
  z[0] += 1 + (ctrl[2] / 4);
  scale_x[0] = 8000 + (ctrl[3] * 16);
  scale_y[0] = 8000 + (ctrl[4] * 16);

  //calculate the noise data
  uint8_t layer = 0;
  for (uint8_t i = 0; i < Width; i++) {
    uint32_t ioffset = scale_x[layer] * (i - CentreX);
    for (uint8_t j = 0; j < Height; j++) {
      uint32_t joffset = scale_y[layer] * (j - CentreY);
      uint16_t data = inoise16(x[layer] + ioffset, y[layer] + joffset, z[layer]);
      if (data < 11000) data = 11000;
      if (data > 51000) data = 51000;
      data = data - 11000;
      data = data / 41;
      noise[layer][i][j] = (noise[layer][i][j] + data) / 2;
    }
  }

  //map the colors
  //here the red layer
  for (uint8_t y = 0; y < Height; y++) {
    for (uint8_t x = 0; x < Width; x++) {
      uint16_t i = noise[0][x][y];
      buffer1[XY(x, y)] = CRGB(a[i], 0, 0);
    }
  }

  // LAYER TWO
  //top left
  ctrl[0] = (ctrl[0] + noise[1][0][0] + noise[1][1][0] + noise [1][0][1] + noise[1][1][1]) / 20;
  //top right
  ctrl[1] = (ctrl[1] + noise[1][Width - 1][0] + noise[1][Width - 2][0] + noise [1][Width - 1][1] + noise[1][Width - 2][1]) / 20;
  //down left
  ctrl[2] = (ctrl[2] + noise[1][0][Height - 1] + noise[1][0][Height - 2] + noise [1][1][Height - 1] + noise[1][1][Height - 2]) / 20;
  //middle left
  ctrl[3] = (ctrl[3] + noise[1][0][CentreY] + noise[1][1][CentreY] + noise [1][0][CentreY + 1] + noise[1][1][CentreY + 1]) / 20;
  //center
  ctrl[4] = (ctrl[4] + noise[1][Width - 1][CentreY] + noise[1][Width - 2][CentreY] + noise [1][Width - 1][CentreY + 1] + noise[1][Width - 2][CentreY + 1]) / 20;
  ctrl[5] = (ctrl[5] + ctrl[0] + ctrl[1]) / 12;

  x[1] = x[1] + (ctrl[0]) - 127;
  y[1] = y[1] + (ctrl[1]) - 127;
  z[1] += 1 + (ctrl[2] / 4);
  scale_x[1] = 8000 + (ctrl[3] * 16);
  scale_y[1] = 8000 + (ctrl[4] * 16);

  //calculate the noise data
  layer = 1;
  for (uint8_t i = 0; i < Width; i++) {
    uint32_t ioffset = scale_x[layer] * (i - CentreX);
    for (uint8_t j = 0; j < Height; j++) {
      uint32_t joffset = scale_y[layer] * (j - CentreY);
      uint16_t data = inoise16(x[layer] + ioffset, y[layer] + joffset, z[layer]);
      if (data < 11000) data = 11000;
      if (data > 51000) data = 51000;
      data = data - 11000;
      data = data / 41;
      noise[layer][i][j] = (noise[layer][i][j] + data) / 2;
    }
  }

  //map the colors
  //here the blue layer
  for (uint8_t y = 0; y < Height; y++) {
    for (uint8_t x = 0; x < Width; x++) {
      uint16_t i = noise[1][x][y];
      buffer2[XY(x, y)] = CRGB(0, 0, a[i]);
    }
  }

  // blend
  //for (uint16_t i = 0; i < NUM_LEDS; i++) {leds[i] = buffer1[i] + buffer2[i];}
  for (uint8_t y = 0; y < Height; y++) {
    for (uint8_t x = 0; x < Width; x++) {
      leds[XY(x, y)] = blend(buffer1[XY(x, y)], buffer2[XY(x, y)], noise[1][y][x] / 4);
      // you could also just add them:
      // leds[XY(x, y)] = buffer1[XY(x, y)] + buffer2[XY(x, y)];
    }
  }

  //make it looking nice
  adjust_gamma();

  //and show it!
  FastLED.show();
}
