//The clock with 4x7-segment indicator with a shift register and neopixel rgb 24 led ring @ arduino nano

#include <Wire.h>
#include <DS3232RTC.h>
#include <Time.h>

// The 4 7-segment indicator with the shift register
const byte digit_pin[4]   = { 14, 15, 16, 17 };
const byte dataPIN_14  = 7;
const byte latchPIN_12 = 8;
const byte clockPIN_11 = 9;

// Neopixel ring
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
const byte NEOPIXEL = 10;                    // Pin of Neopixel Ring
const byte RingSize = 24;
const byte HZ = 10;                          // Redraw neopixel ring frequency

const byte BTN_MENU_PIN = 2;
const byte BTN_INCR_PIN = 3;

void print_time(byte Hour, byte Minute, byte dot, byte DisplayMask = 0xf);
void flashLabels(uint16_t S, int r, int g, int b, byte pos = 255);
void SwitchOffRing(void);
uint32_t Wheel(byte WheelPos);
void showSeconds(uint16_t S, uint32_t Color, bool erase);
void show1Seconds(uint16_t S, uint32_t Color, bool erase);
void show2Seconds(uint16_t S, uint32_t Color, bool erase);
void show3Seconds(uint16_t S, uint32_t Color, bool keep, bool erase);
void show4Seconds(uint16_t S, uint32_t Color, bool erase);
void show5Seconds(uint16_t S, uint32_t Color, bool erase);
void showRainbow(uint16_t S, bool erase);
void show1Rainbow(uint16_t S, bool erase);

//------------------------------------------ class BUTTON ------------------------------------------------------
class BUTTON {
  public:
    BUTTON(byte ButtonPIN, uint16_t timeout_ms = 3000) {
      buttonPIN = ButtonPIN; 
      pt = tickTime = 0; 
      overPress = timeout_ms;
    }
    void init(void) { pinMode(buttonPIN, INPUT_PULLUP); }
    void setTimeout(uint16_t timeout_ms = 3000) { overPress = timeout_ms; }
    byte buttonCheck(void);
    byte intButtonStatus(void) { byte m = mode; mode = 0; return m; }
    bool buttonTick(void);
    void buttonCnangeINTR(void);
  private:
    const uint16_t shortPress = 900;           // If the button was pressed less that this timeout, we assume the short button press
    const uint16_t tickTimeout = 200;          // Period of button tick, while tha button is pressed
    const byte bounce = 50;                    // Bouncing timeout (ms)  
    uint16_t overPress;                        // Maxumum time in ms the button can be pressed
    volatile byte mode;                        // The button mode: 0 - not presses, 1 - short press, 2 - long press
    volatile uint32_t pt;                      // Time in ms when the button was pressed (press time)
    volatile uint32_t tickTime;                // The time in ms when the button Tick was set
    byte buttonPIN;                            // The pin number connected to the button
};

byte BUTTON::buttonCheck(void) {               // Check the button state, called each time in the main loop

  mode = 0;
  bool keyUp = digitalRead(buttonPIN);         // Read the current state of the button
  uint32_t now_t = millis();
  if (!keyUp) {                                // The button is pressed
    if ((pt == 0) || (now_t - pt > overPress)) pt = now_t;
  } else {
    if (pt == 0) return 0;
    if ((now_t - pt) < bounce) return 0;
    if ((now_t - pt) > shortPress)             // Long press
      mode = 2;
    else
      mode = 1;
    pt = 0;
  } 
  return mode;
}

bool BUTTON::buttonTick(void) {                // When the button pressed for a while, generate periodical ticks

  bool keyUp = digitalRead(buttonPIN);         // Read the current state of the button
  uint32_t now_t = millis();
  if (!keyUp && (now_t - pt > shortPress)) {   // The button have been pressed for a while
    if (now_t - tickTime > tickTimeout) {
       tickTime = now_t;
       return (pt != 0);
    }
  } else {
    if (pt == 0) return false;
    tickTime = 0;
  } 
  return false;
}

void BUTTON::buttonCnangeINTR(void) {          // Interrupt function, called when the button status changed
  
  bool keyUp = digitalRead(buttonPIN);
  unsigned long now_t = millis();
  if (!keyUp) {                                // The button has been pressed
    if ((pt == 0) || (now_t - pt > overPress)) pt = now_t; 
  } else {
    if ((now_t - pt) < bounce) return;
    if (pt > 0) {
      if ((now_t - pt) < shortPress) mode = 1; // short press
        else mode = 2;                         // long press
      pt = 0;
    }
  }
}

// -------------------------------------- class adjustClock ------------------------------------------- 
class adjustClock {
  public:
    adjustClock() {
      adjusted = 0;
      incr = autoAdjusted = 0;
    }
    void init(void);                                  // Load adjustment parameters from the clock SRAM
    void adjustRTC(void);                             // adjust RTC. This routine should run periodicaly
    void saveManualAdjustment(long delta);            // save parameters after manual clock adjustment
  private:
    time_t adjusted;                                  // the last time the clock was udjusted manually
    time_t nextAutoAdjust;                            // the time when the clock should be automatically adjusted
    long incr;                                        // adjust increment (decrement) in seconds per 30 days
    long autoAdjusted;                                // the number of seconds the time was adjusted automatically since last update
    void saveData(void);
    void loadData(void);
    void long2Byte(byte *p, long data);
    long byte2Long(const byte *p);
    void calculateAutoAdjustTime(void);
    const long thirtyDays = 2592000;                  // Second in 30 days. Clock incremant adjust calculated in seconds per month
};

void adjustClock::init(void) {
  adjusted = 0;
  incr = autoAdjusted = 0;
  loadData();
  calculateAutoAdjustTime();
}

void adjustClock::adjustRTC(void) {
  if (incr == 0) return;
  time_t nowT = now();
  if (nowT < nextAutoAdjust) return;
  long e = nowT - adjusted;                            // The seconds from time when the clock was adjusted
  if (e < 0) {
    calculateAutoAdjustTime();
    return;
  }
  e *= incr;
  long adj = (long)(e/thirtyDays);                     // normalyze to the seconds in 30 days
  if (adj != autoAdjusted) {
    adj -= autoAdjusted;
    nowT = now() + adj;
    RTC.set(nowT);
    autoAdjusted += adj;
    saveData();
  }
  calculateAutoAdjustTime();
}

void adjustClock::saveManualAdjustment(long delta) {
  time_t nowT = now();
  incr = 0;
  if (adjusted) {                                       // If the clock was previously setup
    long delta_time = nowT - adjusted;                  // The period from last adjustment
    if (delta_time < 43200) {                           // The clock was adjusted less than 12 hours ago
      incr = 0;                                         // The clock month adjustment limited to 1 minute
      if (delta > 0) incr = 60;
      if (delta < 0) incr = -60;
    } else {
      delta += autoAdjusted;                            // since last manual adjustment, the clock was automatically updated
      delta *= thirtyDays;                          
      incr   = delta / delta_time;                      // Number of seconds the clock should be adjusted in 30 days
    }
  }
  adjusted     = nowT;
  autoAdjusted = 0;
  saveData();
}

void adjustClock::long2Byte(byte *p, long data) {
  for (byte i = 0; i < 4; ++i) {
    p[i] = data & 0xff;
    data >>= 8;
  }
}

long adjustClock::byte2Long(const byte *p) {
  long data = 0;
  for (char i = 3; i >= 0; --i) {
    data <<= 8;
    data |= p[i];
  }
  return data;
}

void adjustClock::saveData(void) {
 byte buff[16];

  long2Byte(buff, adjusted);
  long2Byte(&buff[4], incr);
  long2Byte(&buff[8], autoAdjusted);

  long crc = 0;
  for (byte i = 0; i < 12; ++i) {
    crc <<= 2;
    crc += buff[i];
  }
  long2Byte(&buff[12], crc);

  RTC.writeRTC(SRAM_START_ADDR, buff, 16);
}

void adjustClock::loadData(void) {
 byte buff[16];

  byte ans = RTC.readRTC(SRAM_START_ADDR, buff, 16);

  long crc = 0;
  for (byte i = 0; i < 12; ++i) {
    crc <<= 2;
    crc += buff[i];
  }
  long crc_written = byte2Long(&buff[12]);
  if (crc_written == crc) {
    adjusted     = byte2Long(buff);
    incr         = byte2Long(&buff[4]);
    autoAdjusted = byte2Long(&buff[8]);
  }
}

void adjustClock::calculateAutoAdjustTime(void) {
  // The clock should be automatically adjusted at 2:00 am
  time_t nowT = RTC.get();
  long sec = nowT % 86400;        // Seconds since midnight
  long delta = 7200L - sec;       // difference between actual time and 2:00 am
  long last_time = 0;
  if (autoAdjusted < 0) 
    last_time = autoAdjusted * (-1);
  if (delta <= last_time) {       // The clock should be adjusted tomorrow
    delta += 86400;
  }
  nextAutoAdjust = nowT + delta;
}

//------------------------------------------ class SCREEN ------------------------------------------------------
class SCREEN {
  public:
    SCREEN* next;
    SCREEN* nextL;

    SCREEN() {
      next = nextL = 0;
    }
    virtual void init(void) { }
    virtual void show(void) { }
    virtual SCREEN* menu(void) {if (this->next != 0) return this->next; else return this; }
    virtual SCREEN* menu_long(void) { if (this->nextL != 0) return this->nextL; else return this; }
    virtual void inc(void) { }
    virtual void inc_long(void) { }
};


class clockSCREEN: public SCREEN {
  public:
    clockSCREEN() : SCREEN() {
      H = M = S = 0;
      mS = HZ;
      disp_mode = 2; ring_mode = 0;
      ShowDot = true;
      ShowRing = true;
      NightMode = true;
      next_sec = disp_mode_change = ring_mode_change = 0;
      ring_mode = 0;
    }
    virtual SCREEN* menu(void) {
      ++disp_mode;
      if (disp_mode > 2) disp_mode = 0;
      return this;
    }
    virtual SCREEN* menu_long(void) {
      if (nextL != 0) {
        nextL->init();
        return nextL;
      } else return this;
    }
    virtual void init(void);
    virtual void show(void);
    virtual void inc(void) { NightMode = !NightMode; if (!NightMode) ShowRing = true; }
  private:
    unsigned long next_sec = 0;
    unsigned long disp_mode_change;
    unsigned long ring_mode_change;
    const byte deltaMS = (1000 / HZ);
    const byte morning = 9;
    const byte night   = 21; 
    byte mS;
    byte disp_mode;               // Display info: 0 - clock, 1 - day&month, 2 - year
    byte ring_mode;               // How to show the ring this time
    bool NightMode;
    bool ShowRing;
    bool ShowDot;
    byte H, M, S;
    time_t nowRTC;
};

void clockSCREEN::init(void) {
  nowRTC = RTC.get();
  setTime(nowRTC);
  S = second();
  M = minute();
  H = hour();
  ring_mode = random(9);
  ring_mode_change = millis() + ((unsigned long)random(120, 600) * 1000);
}

void clockSCREEN::show(void) {

  unsigned long nowMS = millis();
  
  if (nowMS >= next_sec) {            // Calculate seconds
    next_sec = nowMS + deltaMS;       // Several times per second

    if (NightMode) {
      if (ShowRing && ((H >= night) || (H <= morning))) {
        SwitchOffRing();
        ShowRing = false;
      }
      if (!ShowRing && (H > morning) && (H < night)) ShowRing = true;
    } else {
      if (!ShowRing) ShowRing = true;
    }
    
    ++mS;
    if (mS >= HZ) {
      mS = 0;
      ++S;
      ShowDot = !ShowDot;
      if (S >= 60) {
        nowRTC = RTC.get();
        setTime(nowRTC);
        S = second();
        M = minute();
        H = hour();
      }
    }
  }
  
  // Sometimes show date
  if (nowMS >= disp_mode_change) {
    if (disp_mode != 0)
      ++disp_mode;
    else
      if ((S > 5) && (S < 50)) ++disp_mode;
    if (disp_mode > 1) disp_mode = 0;
    if (disp_mode > 0) disp_mode_change = nowMS + 3000;
    else disp_mode_change = nowMS + ((unsigned long)random(20, 30) * 1000);
  }
  
  byte Century = year() / 100;
  byte y = year() % 100;
  switch (disp_mode) {
    case 1:                      // Show the date (day, month)
      print_time(day(), month(), 2);
      break;

    case 2:                      // Year
      print_time(Century, y, 4);
      break;
      
    default:                      // Hour and Minute
      if (ShowDot) 
        print_time(H, M, 2);
      else
        print_time(H, M, 4);
      break;
  }
  
  if (ShowRing) {
    byte loop = H * 60 + M;
    if (M % 2) loop += 86;
    if ((S == 0) && nowMS >= ring_mode_change) {
      byte next_mode = random(8);
      if (next_mode >= ring_mode) {
        next_mode ++;
      }
      ring_mode = next_mode;
      ring_mode_change = nowMS + ((unsigned long)random(120, 600) * 1000);
    }
    bool last_leap = (nowMS + 5000) > ring_mode_change;
    switch (ring_mode) {
      case 1:
        show1Seconds((uint16_t)S*HZ + mS, Wheel(loop), last_leap);
        break;
      case 2:
        show2Seconds((uint16_t)S*HZ + mS, Wheel(loop), last_leap);
        break;
      case 3:
        show3Seconds((uint16_t)S*HZ + mS, Wheel(loop), false, last_leap);
        break;
      case 4:
        show3Seconds((uint16_t)S*HZ + mS, Wheel(loop), true, last_leap);
        break;
      case 5:
        showRainbow((uint16_t)S*HZ + mS, last_leap);
        break;
      case 6:
        show1Rainbow((uint16_t)S*HZ + mS, last_leap);
        break;
      case 7:
        show4Seconds((uint16_t)S*HZ + mS, Wheel(loop), last_leap);
        break;
      case 8:
        show5Seconds((uint16_t)S*HZ + mS, Wheel(loop), last_leap);
        break;
      default:
        showSeconds((uint16_t)S*HZ + mS, Wheel(loop), last_leap);
        break;
    }
  } else {              // If the Ring is not working
    delay(10);
  }
}


class clockSetupSCREEN: public SCREEN {
  public:
    clockSetupSCREEN(adjustClock* AC) : SCREEN() {
      next_sec = 0;
      EditEntity = 0;
      ShowDigit = true;
      pAc = AC;
    }
    virtual SCREEN* menu(void) {
      ++EditEntity;
      if (EditEntity >= 5) EditEntity = 0;
      return this;
    }
    virtual SCREEN* menu_long(void);
    virtual void inc(void);
    virtual void init(void);
    virtual void show(void);
  private:
    const uint16_t timeout = 10000;
    unsigned long next_sec;
    unsigned long edit_timeout;
    bool ShowDigit;
    byte EditEntity;                          // 0 - Hour, 1 - Minute, 2 - Day, 3 - Month, 4 - Year
    tmElements_t tm;                          // time to be set-up
    adjustClock* pAc;                         // The pointer to the adjustClock instance;
};

void clockSetupSCREEN::init(void) {
  RTC.read(tm);
  EditEntity = 0;
  next_sec = millis();
}

SCREEN* clockSetupSCREEN::menu_long(void) {
  if (nextL != 0) {
    tm.Second = 0;
    time_t old_time = RTC.get();
    time_t new_time = makeTime(tm);
    long difference = 0;
    if (new_time >= old_time) {
      difference = new_time - old_time;
    } else {
      difference = old_time - new_time;
      difference *= -1;
    }
    RTC.write(tm);
    pAc->saveManualAdjustment(difference);
    nextL->init();
    return nextL;
  } else return this;
}


void clockSetupSCREEN::show(void) {
  
  unsigned long nowMS = millis();
  
  if (nowMS >= next_sec) {
    next_sec = nowMS + 200;               // Blink with digit
    ShowDigit = !ShowDigit;
  }
  
  byte DisplayMask = 0b1111;
  byte Cent = (tm.Year + 1970) / 100;
  byte Year = (tm.Year + 70) % 100;
  switch (EditEntity) {
   case 0:                                // Hour
     if (!ShowDigit) DisplayMask &= 0b0011;
     print_time(tm.Hour, tm.Minute, 2, DisplayMask);
     break;

   case 1:                                // Minute
     if (!ShowDigit) DisplayMask &= 0b1100;
     print_time(tm.Hour, tm.Minute, 2, DisplayMask);
     break;

   case 2:                                // Day
     if (!ShowDigit) DisplayMask &= 0b0011;
     print_time(tm.Day, tm.Month, 2, DisplayMask);
     break;

   case 3:                                // Month
     if (!ShowDigit) DisplayMask &= 0b1100;
     print_time(tm.Day, tm.Month, 2, DisplayMask);
     break;

   case 4:                                // Year
     if (!ShowDigit) DisplayMask &= 0b1100;
     print_time(Cent, Year, 4, DisplayMask);
     break;
  }
}

void clockSetupSCREEN::inc(void) {
 static byte days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  char Year = tm.Year - 30;
  if (Year < 0) Year = 0;
  switch(EditEntity) {
    case 0:                                // Hour
      ++tm.Hour; if (tm.Hour > 23) tm.Hour = 0;
      break;

    case 1:                                // Minute
      ++tm.Minute; if (tm.Minute > 59) tm.Minute = 0;
      break;

    case 2:                                // Day
      days[1] = 28;
      if (((tm.Year+1970) / 4) == 0) days[1] = 29;       // Leap Year 
      ++tm.Day;
      if (tm.Day > days[tm.Month-1]) tm.Day = 1;
      break;

    case 3:                                // Month
      ++tm.Month;
      if (tm.Month > 12) tm.Month = 1;
      break;

    case 4:                                // Year
      ++Year;
      if (Year > 99) Year = 0;
      break;
  }
  tm.Year = Year + 30;
}
// ================================ End of all class definitions ====================================== 

// Class instances
Adafruit_NeoPixel strip = Adafruit_NeoPixel(RingSize, NEOPIXEL, NEO_GRB + NEO_KHZ800);
adjustClock adjClock;

clockSCREEN      mainClock;
clockSetupSCREEN setupClock(&adjClock);

BUTTON menuButton(BTN_MENU_PIN);
BUTTON incrButton(BTN_INCR_PIN);

SCREEN* pScreen = &mainClock;


void setup() {                
  // initialize shift register PINS
  pinMode(dataPIN_14, OUTPUT);
  pinMode(latchPIN_12, OUTPUT);
  pinMode(clockPIN_11, OUTPUT);

  // initialize the digit pin as an output.
  for (int i = 0; i < 4; ++i) {
    pinMode(digit_pin[i], OUTPUT);
    digitalWrite(digit_pin[i], HIGH);
  }

  time_t nowRTC = RTC.get();
  randomSeed(nowRTC);
  mainClock.init();
  adjClock.init();
  
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  menuButton.init();
  incrButton.init();
  incrButton.setTimeout(10000);
  attachInterrupt(digitalPinToInterrupt(BTN_MENU_PIN), menuButtonChange, CHANGE);
  
  mainClock.nextL  = &setupClock;
  setupClock.nextL = &mainClock;

  strip.begin();
  delay(500);
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'
}

// Interrupts functions just call the corresponding method
void menuButtonChange(void) {
  menuButton.buttonCnangeINTR();
}

//================================================================================
// The Main LOOP                                                                  
//================================================================================
void loop() {
 static unsigned long return_to_main = 0;
 static bool RingOff = false;

  bool buttonPressed = false;

   if ((pScreen != &mainClock) && millis() > return_to_main) {
     return_to_main = 0;
     pScreen = &mainClock;
   }

  // Check status of the main button, that supports interrupts
  if (byte Mode = menuButton.intButtonStatus()) {
    buttonPressed = true;
    if (Mode == 1) pScreen = pScreen->menu();
    if (Mode == 2) pScreen = pScreen->menu_long();
  }
  // Check status of the increment button
  if (byte Mode = incrButton.buttonCheck()) {
    buttonPressed = true;
    if (Mode == 1) pScreen->inc();
    if (Mode == 2) pScreen->inc_long();
  }  

  if (pScreen == &setupClock)
    if (incrButton.buttonTick()) pScreen->inc();
    
  if (pScreen != &mainClock) {
      if (!RingOff) {
        SwitchOffRing();
        RingOff = true;
      }
      if (buttonPressed == true) return_to_main = millis() + 30000;
  } else
    RingOff = false;

   pScreen->show();
   adjClock.adjustRTC();
}


//Low-level functions

void SwitchOffRing(void) {
  for (byte i = 0; i < strip.numPixels(); ++i) strip.setPixelColor(i, 0);
  strip.show();
}

// 4 indicators (0h, 3h, 6h, 9h) is flashing every 5 seconds
void flashLabels(uint16_t S, int r, int g, int b, byte pos) {
 static byte br_lev[] = { 127, 64, 50, 40, 30, 25, 30, 40, 50, 64 };
 const uint16_t tick = 5 * HZ;                   // ticks in 5 seconds

  byte secPart  =  S % tick; 
  byte divider = tick / 10;                      // Normalize time interval to the array size
  byte pos1 = secPart / divider;                 // index in the brightness array 
  byte k = secPart % divider;
  byte pos2 = pos1 + 1; if (pos2 >= 10) pos2 = 0;
  //int delta = ((br_lev[pos2] - br_lev[pos1]) * k) / divider;
  //byte br = br_lev[pos1] + delta;
  byte br = map(k, 0, divider, br_lev[pos1], br_lev[pos2]);
  r *= br; r >>= 7;
  g *= br; g >>= 7;
  b *= br; b >>= 7;
  uint32_t white = strip.Color(r, g, b);
  for (byte label = 0; label < 24; label += 6) { 
    if (label == 0 || label != pos) strip.setPixelColor(label, white);
  }
}

void FadeRing(uint32_t Color, byte shift) {
  byte b = Color;
  byte g = Color >> 8;
  byte r = Color >> 16;
  r >>= shift;
  g >>= shift;
  b >>= shift;
  uint32_t Color_faded = strip.Color(r, g, b);
  for (byte i = 0; i < RingSize; ++i) strip.setPixelColor(i, Color_faded);
}

// Fill ring as seconds past, new dot every 2.5 sec
void showSeconds(uint16_t S, uint32_t Color, bool erase) {
 const uint16_t Last8Ticks = 60 * HZ - 8;
 const byte onedot = ((uint16_t)HZ * 60) / RingSize;
 static unsigned int old_S    = 0;
 static byte old_pos = 0;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;
 
  if (old_S == S) return;
  old_S = S;
  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  if (S < RingSize) strip.setPixelColor(S, 0);
  if (erase && (S > Last8Ticks)) {
    S -= Last8Ticks;
    FadeRing(Color, S);
    flashLabels(S, r, g, b);
  } else {
    byte pos = (uint32_t)S * RingSize / ((uint32_t)HZ * 60);
    if (pos != old_pos) {
      strip.setPixelColor(pos, Color);
      old_pos = pos;
    }
    flashLabels(S, r, g, b, pos);
  }
  
  strip.show();  
}

// each dot is coming counterclockwize from 12 o'clock
void show1Seconds(uint16_t S, uint32_t Color, bool erase) {
 const byte onedot = ((uint16_t)HZ * 60) / RingSize;
 const uint16_t Last8Ticks = 60 * HZ - 8;
 static unsigned int old_S    = 0;
 static byte runPos = 255;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;

  if (old_S == S) return;
  old_S = S;

  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  if (erase && (S > Last8Ticks)) {
    S -= Last8Ticks;
    FadeRing(Color, S);
    flashLabels(S, r, g, b);
  } else {
    byte pos = (uint32_t)S * RingSize / ((uint32_t)HZ * 60);

    byte runSecond = S % onedot;
    byte newRunPos = RingSize - RingSize * runSecond / onedot;
    if ((newRunPos < RingSize) && newRunPos > pos) {
      if (newRunPos != runPos) {
        strip.setPixelColor(newRunPos, Color);
        if (runPos < RingSize) strip.setPixelColor(runPos, 0);
        runPos = newRunPos;
      } 
    }
    strip.setPixelColor(pos, Color);
    flashLabels(S, r, g, b, runPos);
  }
  strip.show();  
}

// each dot is coming counterclockwize from last position
void show2Seconds(uint16_t S, uint32_t Color, bool erase) {
 const byte onedot = ((uint16_t)HZ * 60) / RingSize;
 const uint16_t LastTick = 60 * HZ - 1;
 static unsigned int old_S    = 0;
 static byte runPos = 255;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;

  if (old_S == S) return;
  old_S = S;

  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  byte pos = (uint32_t)S * RingSize / ((uint32_t)HZ * 60);
 
  byte runSecond = S % onedot;
  char newRunPos = pos - RingSize * runSecond / onedot;
  if (newRunPos < 0) newRunPos += RingSize; 
  if ((newRunPos < RingSize) && newRunPos != pos) {
    if (newRunPos != runPos) {
      strip.setPixelColor(newRunPos, Color);
      if (runPos < RingSize) strip.setPixelColor(runPos, 0);
      runPos = newRunPos;
    } 
  }
  strip.setPixelColor(pos, Color);
 if (erase && (S >= LastTick)) strip.setPixelColor(RingSize-1, 0);
 

  flashLabels(S, r, g, b, pos);
  strip.show();  
}

// Rise the dot slowly
void show3Seconds(uint16_t S, uint32_t Color, bool keep, bool erase) {
 const byte onedot = ((uint16_t)HZ * 60) / RingSize;
 const uint16_t Last8Ticks = 60 * HZ - 8;
 static unsigned int old_S    = 0;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;

  if (old_S == S) return;
  old_S = S;

  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  if (S < RingSize) strip.setPixelColor(S, 0);
    
  byte pos = (uint32_t)S * RingSize / ((uint32_t)HZ * 60);

  byte b1 = Color;
  byte g1 = Color >> 8;
  byte r1 = Color >> 16;
  byte runSecond = S % onedot;
  byte shift = (runSecond << 3) / onedot;
  r1 >>= (7 - shift);
  g1 >>= (7 - shift);
  b1 >>= (7 - shift);
  strip.setPixelColor(pos, strip.Color(r1, g1, b1));

  if (!keep) {
    byte fadePos = RingSize-1;
    if (pos >= 1) fadePos = pos - 1;
    uint32_t fadeColor = strip.getPixelColor(fadePos);
    b1 = fadeColor;
    g1 = fadeColor >> 8;
    r1 = fadeColor >> 16;
    if (shift > 3) {
      r1 >>= 1;
      g1 >>= 1;
      b1 >>= 1;
    } else if (shift == 7) strip.setPixelColor(fadePos, 0);
    strip.setPixelColor(fadePos, strip.Color(r1, g1, b1));
  }

  if (erase && (S >  Last8Ticks)) { 
    if (keep) FadeRing(Color, S);
    if (!keep && (shift == 7)) strip.setPixelColor(RingSize-1, 0);
  }

  flashLabels(S, r, g, b, pos);
  strip.show();  
}

// run the sector clockwise
void show4Seconds(uint16_t S, uint32_t Color, bool erase) {
 const byte onedot = ((uint16_t)HZ * 60) / RingSize;
 const uint16_t Last8Ticks = 60 * HZ - 8;
 static unsigned int old_S    = 0;
 static byte firstPos = 255;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;

  if (old_S == S) return;
  old_S = S;

  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  if (erase && (S > Last8Ticks)) {
    S -= Last8Ticks;
    FadeRing(Color, S);
    flashLabels(S, r, g, b);
  } else {
    byte length = (uint32_t)S * RingSize / ((uint32_t)HZ * 60) + 1;
    byte runSecond = S % onedot;
    byte newFirstPos = RingSize * runSecond / onedot;
    char lastPos = newFirstPos - length;
    if (lastPos < 0) lastPos += RingSize;
    if (newFirstPos < RingSize) {
      if (newFirstPos != firstPos) {
        strip.setPixelColor(newFirstPos, Color);
        if (lastPos >= 0) strip.setPixelColor(lastPos, 0);
        firstPos = newFirstPos;
      } 
    }
    flashLabels(S, r, g, b, firstPos);
  }
  strip.show();  
}

// swing the sector
void show5Seconds(uint16_t S, uint32_t Color, bool erase) {
 const byte onedot = ((uint16_t)HZ * 60) / RingSize;
 const uint16_t Last8Ticks = 60 * HZ - 8;
 static unsigned int old_S    = 0;
 static byte firstPos = 255;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;

  if (old_S == S) return;
  old_S = S;

  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  if (erase && (S > Last8Ticks)) {
    S -= Last8Ticks;
    FadeRing(Color, S);
    flashLabels(S, r, g, b);
  } else {
    byte length = (uint32_t)S * RingSize / ((uint32_t)HZ * 60) + 1;
    byte runSecond = S % onedot;
    byte newFirstPos;
    char lastPos;
    if ((length % 2) == 0) {                      // even
      newFirstPos = RingSize - RingSize * runSecond / onedot;
      lastPos = newFirstPos + length;
      if (lastPos >= RingSize) lastPos = -1;
    } else {                                      // odd
      newFirstPos = RingSize * runSecond / onedot;
      lastPos = newFirstPos - length;
      if (lastPos < 0) lastPos = -1;
    }
    if ((newFirstPos < RingSize) && (newFirstPos >= 0)) {
      if (newFirstPos != firstPos) {
        strip.setPixelColor(newFirstPos, Color);
        if ((lastPos >= 0) && (lastPos < RingSize))
          strip.setPixelColor(lastPos, 0);
        firstPos = newFirstPos;
      } 
    }
    flashLabels(S, r, g, b, firstPos);
  }
  strip.show();  
}

// Fill ring wth the rainbow as seconds past
void showRainbow(uint16_t S, bool erase) {
 const uint16_t LastRSTicks = 60 * HZ - RingSize;
 static unsigned int old_S    = 0;
 static byte old_pos = 0;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;
 
  if (old_S == S) return;
  old_S = S;
  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  if (S < RingSize) strip.setPixelColor(S, 0);
  if (erase && (S > LastRSTicks)) {
    S -= LastRSTicks;
    strip.setPixelColor(S, 0);
  } else {
    for(uint16_t i = 1; i <= RingSize; ++i) {
      strip.setPixelColor(RingSize - i, Wheel((i+S) & 255));
    }
    flashLabels(S, r, g, b);
  }
  
  strip.show();  
}

// Fill ring wth the rainbow as seconds past
void show1Rainbow(uint16_t S, bool erase) {
 const uint16_t LastRSTicks = 60 * HZ - RingSize;
 static unsigned int old_S    = 0;
 static byte old_pos = 0;
 static byte r = 128;
 static byte g = 128;
 static byte b = 128;
 
  if (old_S == S) return;
  old_S = S;
  if (S == 0) {
     r = random(32);
     g = random(32);
     b = random(32);
     r <<= 2; g <<= 2; b <<= 2;
  }

  if (S < RingSize) strip.setPixelColor(S, 0);
  if (erase && (S > LastRSTicks)) {
    S -= LastRSTicks;
    strip.setPixelColor(S, 0);
  } else {
    for(uint16_t i = 1; i <= RingSize; ++i) {
      strip.setPixelColor(RingSize - i, Wheel(((i * 256 / RingSize) + S) & 255));
    }
    flashLabels(S, r, g, b);
  }
  
  strip.show();  
}

void print_time(byte Hour, byte Minute, byte dot, byte DisplayMask) {
 static byte decimal[] = {
  0b11111100, 0b01100000, 0b11011010, 0b11110010, 0b01100110, 
  0b10110110, 0b10111110, 0b11100000, 0b11111110, 0b11110110 };
  byte digit;

  byte mask = 1;
  for (byte d = 0; d < 4; ++d) {        // Print out digit from right to left
    if (d < 2) {                        // Minutes
      digit = Minute % 10;
      Minute /= 10;
    } else {                            // Hours
      digit = Hour % 10;
      Hour /= 10;
    }
    byte symbol = decimal[digit];
    if (dot == d) symbol |= 1;
    if (DisplayMask & mask) {
      digitalWrite(latchPIN_12, LOW);
      shiftOut(dataPIN_14, clockPIN_11, LSBFIRST, symbol);    
      digitalWrite(latchPIN_12, HIGH);
    
      digitalWrite(digit_pin[d], LOW);
      delayMicroseconds(50);
      digitalWrite(digit_pin[d], HIGH);
    }
    mask <<= 1;
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  const byte br = 5;
  byte c1 = 255 - WheelPos * 3; c1 /= br;
  byte c2 = WheelPos * 3;       c2 /= br;
  if(WheelPos < 85) {
    return strip.Color(c1, 0, c2);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, c2, c1);
  }
  WheelPos -= 170;
  return strip.Color(c2, c1, 0);
}
