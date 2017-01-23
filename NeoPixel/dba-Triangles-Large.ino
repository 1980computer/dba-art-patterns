#include <Adafruit_NeoPixel.h>

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);

// # of LED strips in the system - can be any contiguous subsets
#define NUM_STRIPS 3
int strip_thread_spacing[NUM_STRIPS];
int strip_start[NUM_STRIPS];
int strip_end[NUM_STRIPS];

// Connecions between strips - allows threads to move from one strip to another
#define NUM_STRIP_CONNECTIONS 2
int strip_connection_start[NUM_STRIP_CONNECTIONS];  // value is a strip number
int strip_connection_end[NUM_STRIP_CONNECTIONS];    // value is a strip number

// max # of threads - depends on the strip size and how many threads per strip there might be
// it should be a bit more than the NUM_STRIPS x the max number of threads per strip that might be on at the same time
// the debug code will show the max thread #
#define MAX_THREADS 40
int thread_pos[MAX_THREADS];
int thread_strip[MAX_THREADS];
boolean thread_active[MAX_THREADS];
int max_thread = 0;

int loop_count = 0;   // just an internal counter to know when to start new threads
int width = 5;        // width of color bar - should match the code in the draw fucntion
int wait = 40;        // ms between loops - speed of movement

boolean debug = false;


// ********************************************************
// setup
// ********************************************************
void setup() {
  int i;
  
  if (debug) Serial.begin(9600);      // open the serial port at 9600 bps:    

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  // do one of these for each strip
  // the led numbers that start and end each strip + the timing
  // do one of these for each strip
  strip_start[0] = 0;
  strip_end[0] = 39;
  strip_thread_spacing[0] = 100;  // the strip timing - the number is how many loops until a new thread starts
                                 // 0 = do not start threads on that strip
  strip_start[1] = 40;
  strip_end[1] = 79;
  strip_thread_spacing[1] = 0;

  strip_start[2] = 80;
  strip_end[2] = 150;
  strip_thread_spacing[2] = 0;
  
  // optional strip connections - array indexes have to match NUM_STRIP_CONNECTIONS
  strip_connection_start[0] = 0;  // value is a strip number
  strip_connection_end[0] = 1;   // value is a strip number
  strip_connection_start[1] = 0;  // value is a strip number
  strip_connection_end[1] = 2;   // value is a strip number
  //strip_connection_start[2] = 0;  // value is a strip number
  //strip_connection_end[2] = 2;   // value is a strip number

  // mark all the thread active markers as inactive
  for (i=0; i<MAX_THREADS; i++) {
    thread_active[i] = false;
  }
} // setup


// ********************************************************
// loop
// ********************************************************
void loop() {
  int i, j;
  int num_new_connections = 0;
  int new_connection_index[NUM_STRIP_CONNECTIONS];
   
  if (debug) Serial.print(loop_count);  
  if (debug) Serial.print(" .. ");  
  
  // wipe the strip
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }

  // add new threads if we are at the right interval for that strip
  for (i=0; i<NUM_STRIPS; i++) {
    if ((loop_count % strip_thread_spacing[i]) == 0) new_thread(i);
  }
  
  // increment the thread positions and draw them
  for (i=0; i<=max_thread; i++) {
    if (thread_active[i]) {
      
      // if at the end of a strip and there is a connection, then save the connection
      // we will start a new one on the new strip, but leave this thread to finish on the current strip
      if (thread_pos[i] == strip_end[thread_strip[i]]) {
        for (j=0; j<NUM_STRIP_CONNECTIONS; j++) {
          if (thread_strip[i] == strip_connection_start[j]) new_connection_index[num_new_connections++] = j;
        }
      }
      
      // draw the thread if it is not at the end (plus the thread width)
      if (thread_pos[i] < (strip_end[thread_strip[i]] + width)) {
        draw_thread(i);
      } else {
        // if at the end, mark it as inactive
        if (debug) Serial.println();
        if (debug) Serial.print("Freeing thread: ");  
        if (debug) Serial.println(i);  
        thread_active[i] = false;
      }
      thread_pos[i]++;
    } // thread_active
  }

  // add new threads for any connections
  for (i=0; i<num_new_connections; i++) {
    new_thread(strip_connection_end[new_connection_index[i]]);
  }
  
  strip.show();
  delay(wait);
  loop_count++;
  
} // loop


// ********************************************************
// new_thread - add a thread to the list
// ********************************************************
void new_thread(int strip_num) {
  int i;
  
  // find the next available inactive thread
  for (i=0; i<MAX_THREADS; i++) {
    if (!thread_active[i]) break;
  }

  // assign the strip info to that new thread  
  if (debug) Serial.println();
  if (debug) Serial.print("New Thread - strip_num: ");  
  if (i < MAX_THREADS) {
    thread_active[i] = true;
    thread_strip[i] = strip_num;
    thread_pos[i] = strip_start[strip_num];
    if (debug) Serial.print(strip_num);  
    if (debug) Serial.print(" , start: ");  
    if (debug) Serial.print(strip_start[strip_num]);  
    if (debug) Serial.print(" , thread_num: ");  
    if (debug) Serial.println(i);  
   }
   
   // find the largest active thread to minimize looping later
  for (i=(MAX_THREADS-1); i>=0; i--) {
    if (thread_active[i]) break;
  }
  max_thread = i;
  if (debug) Serial.print("Max Thread: ");  
  if (debug) Serial.println(max_thread);  
} // new_thread 9/9/15 CFS


// ********************************************************
// draw_thread - draw the thread
// ********************************************************
void draw_thread(int thread_num) {
    int t_pos = thread_pos[thread_num];
    int s_start = strip_start[thread_strip[thread_num]];
    int s_end = strip_end[thread_strip[thread_num]];

    // these are the colors of each pixel ina  thread - the number of these whoulf match the width variable
    // the -0 one is the head of the thread
    
    if (((t_pos - 4) >= s_start) && ((t_pos - 4) <= s_end)) strip.setPixelColor(t_pos - 4, 0, 0, 16);
    if (((t_pos - 3) >= s_start) && ((t_pos - 3) <= s_end)) strip.setPixelColor(t_pos - 3, 0, 0, 32);
    if (((t_pos - 2) >= s_start) && ((t_pos - 2) <= s_end)) strip.setPixelColor(t_pos - 2, 0, 0, 64);
    if (((t_pos - 1) >= s_start) && ((t_pos - 1) <= s_end)) strip.setPixelColor(t_pos - 1, 0, 0, 128);
    if (((t_pos - 0) >= s_start) && ((t_pos - 0) <= s_end)) strip.setPixelColor(t_pos - 0, 0, 0, 255);
    
    /*
    if (thread_strip[thread_num] == 0) {
      if (((t_pos - 4) >= s_start) && ((t_pos - 4) <= s_end)) strip.setPixelColor(t_pos - 4, 0, 16, 0);
      if (((t_pos - 3) >= s_start) && ((t_pos - 3) <= s_end)) strip.setPixelColor(t_pos - 3, 0, 32, 0);
      if (((t_pos - 2) >= s_start) && ((t_pos - 2) <= s_end)) strip.setPixelColor(t_pos - 2, 0, 64, 0);
      if (((t_pos - 1) >= s_start) && ((t_pos - 1) <= s_end)) strip.setPixelColor(t_pos - 1, 0, 128, 0);
      if (((t_pos - 0) >= s_start) && ((t_pos - 0) <= s_end)) strip.setPixelColor(t_pos - 0, 0, 255, 0);
    } else if (thread_strip[thread_num] == 1) {
      if (((t_pos - 4) >= s_start) && ((t_pos - 4) <= s_end)) strip.setPixelColor(t_pos - 4, 0, 0, 16);
      if (((t_pos - 3) >= s_start) && ((t_pos - 3) <= s_end)) strip.setPixelColor(t_pos - 3, 0, 0, 32);
      if (((t_pos - 2) >= s_start) && ((t_pos - 2) <= s_end)) strip.setPixelColor(t_pos - 2, 0, 0, 64);
      if (((t_pos - 1) >= s_start) && ((t_pos - 1) <= s_end)) strip.setPixelColor(t_pos - 1, 0, 0, 128);
      if (((t_pos - 0) >= s_start) && ((t_pos - 0) <= s_end)) strip.setPixelColor(t_pos - 0, 0, 0, 255);
    } else {
      if (((t_pos - 4) >= s_start) && ((t_pos - 4) <= s_end)) strip.setPixelColor(t_pos - 4, 16, 0, 0);
      if (((t_pos - 3) >= s_start) && ((t_pos - 3) <= s_end)) strip.setPixelColor(t_pos - 3, 32, 0, 0);
      if (((t_pos - 2) >= s_start) && ((t_pos - 2) <= s_end)) strip.setPixelColor(t_pos - 2, 64, 0, 0);
      if (((t_pos - 1) >= s_start) && ((t_pos - 1) <= s_end)) strip.setPixelColor(t_pos - 1, 128, 0, 0);
      if (((t_pos - 0) >= s_start) && ((t_pos - 0) <= s_end)) strip.setPixelColor(t_pos - 0, 255, 0, 0);
    }
    */
} // draw_thread  9/9/15 CFS









  // Some example procedures showing how to display to the pixels:
  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  //rainbow(20);
  //rainbowCycle(20);
  

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

