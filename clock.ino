#include <Adafruit_NeoPixel.h>
#include <DS3231.h>
#include "IRremote.h"

#define LED_PIN    6
#define LED_COUNT 209

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
DS3231 clock;
// remote on pin 11
IRrecv irrecv(11); 

decode_results results; 
RTCDateTime dt;

struct Color {
  int r;
  int g;
  int b;
};

int isClockOn = 1;
int isInverted = 0;
int brightness = 100;
uint32_t black = strip.Color(0, 0, 0);
unsigned long lastIRValue = 0xFFFFFFFF;
Color currentColor = {255, 0, 0};

int dots[] = {85, 123};  

int positions[4][15] = {
  {74, 73, 72, 77, 78, 79, 112, 111, 110, 115, 116, 117, 150, 149, 148},
  {70, 69, 68, 81, 82, 83, 108, 107, 106, 119, 120, 121, 146, 145, 144},
  {64, 63, 62, 87, 88, 89, 102, 101, 100, 125, 126, 127, 140, 139, 138},
  {60, 59, 58, 91, 92, 93, 98, 97, 96, 129, 130, 131, 136, 135, 134}
};

int digits[10][15] = {
  {1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1},  
  {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},  
  {1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1},  
  {1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1},  
  {0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1},  
  {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1},  
  {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1},  
  {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1},  
  {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},  
  {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1}  
};

void setup() {
  clock.begin(); // Initialize DS3231
  // Send sketch compiling time to Arduino
  clock.setDateTime(__DATE__, __TIME__); 

  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show(); // Turn OFF all pixels ASAP
  strip.setBrightness(brightness); // Set BRIGHTNESS to about 1/5 (max = 255)

  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  // have we received an IR signal?
  if (irrecv.decode(&results)) {
    translateIR(); 
    irrecv.resume();
  }

  if (isClockOn) {
    uint32_t color = createColor(currentColor);

    if (isInverted) {
      updateClock(black, color);
    } else {
      updateClock(color, black);
    }
  } else {
    strip.clear();
    strip.show();
  }
  
  delay(100);
}

void updateClock(uint32_t foregroundColor, uint32_t backgroundColor) {
  dt = clock.getDateTime();

  int h1 = dt.hour / 10;
  int h2 = dt.hour % 10;
  int m1 = dt.minute / 10;
  int m2 = dt.minute % 10;
  
  strip.clear();

  for(int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, backgroundColor);
  }
  
  showDigit(h1, 0, foregroundColor);
  showDigit(h2, 1, foregroundColor);
  showDigit(m1, 2, foregroundColor);
  showDigit(m2, 3, foregroundColor);

  if (dt.second % 2 == 0) {
    showDots(foregroundColor);
  }
  
  strip.show();
}

void showDigit(int digit, int pos, uint32_t color) {
  for(int i = 0; i < 15; i++) {
    if (digits[digit][i] == 1) {
      strip.setPixelColor(positions[pos][i], color);
    }
  }
}

void showDots(uint32_t color) {
  strip.setPixelColor(dots[0], color);
  strip.setPixelColor(dots[1], color);
}

uint32_t createColor(Color color) {
  return strip.Color((brightness*color.r/255), (brightness*color.g/255), (brightness*color.b/255));
}

void translateIR() {
  switch(results.value) {
    // REPEAT
    case 0xFFFFFFFF:
      if (results.value != 0xFF629D && results.value != 0xFFC23D) {
        processIR(lastIRValue);
      }
      break;
    default:
      processIR(results.value);
      lastIRValue = results.value;
  }
}

void processIR(unsigned long value) {
  switch(value) {
    // VOL+ / CH
    case 0xFF629D:
      isClockOn = isClockOn ? 0 : 1; 
      break;
    // FAST FORWARD / PAUSE
    case 0xFFC23D:
      isInverted = isInverted ? 0 : 1; 
      break;
    // FUNC|STOP / CH+
    case 0xFFE21D:
      changeValue(&brightness, 5);
      break;
    // POWER / CH-
    case 0xFFA25D: 
      changeValue(&brightness, -5);
      break;
    // 1
    case 0xFF30CF:
      currentColor = {255, 0, 0}; 
      break;
    // 2
    case 0xFF18E7: 
      currentColor = {0, 255, 0}; 
      break;
    // 3
    case 0xFF7A85: 
      currentColor = {0, 0, 255}; 
      break;
    // 4
    case 0xFF10EF:
      changeValue(&currentColor.r, 10); 
      break;
    // 5
    case 0xFF38C7: 
      changeValue(&currentColor.g, 10); 
      break;
    // 6
    case 0xFF5AA5: 
      changeValue(&currentColor.b, 10); 
      break;
    // 7
    case 0xFF42BD: 
      changeValue(&currentColor.r, -10); 
      break;
    // 8
    case 0xFF4AB5: 
      changeValue(&currentColor.g, -10); 
      break;
    // 9
    case 0xFF52AD: 
      changeValue(&currentColor.b, -10); 
      break;
  }  
}

void changeValue(int *var, int delta) {
  *var = *var + delta;
  if (*var < 0) {
    *var = 0;
  } else if (*var > 255){
    *var = 255;
  }
}
