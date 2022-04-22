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

int isClockOn = 1;
int brightness = 100;
unsigned long lastIRValue = 0xFFFFFFFF;

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
    updateClock();
  } else {
    strip.clear();
    strip.show();
  }
  
  delay(100);
}

void updateClock() {
  dt = clock.getDateTime();
  uint32_t color = createColor(255, 0, 0);

  int h1 = dt.hour / 10;
  int h2 = dt.hour % 10;
  int m1 = dt.minute / 10;
  int m2 = dt.minute % 10;
  
  strip.clear();
  
  showDigit(h1, 0, color);
  showDigit(h2, 1, color);
  showDigit(m1, 2, color);
  showDigit(m2, 3, color);

  if (dt.second % 2 == 0) {
    showDots(color);
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

uint32_t createColor(int r, int g, int b) {
  return strip.Color((brightness*r/255), (brightness*g/255), (brightness*b/255));
}

void translateIR() {
  switch(results.value) {
    // REPEAT
    case 0xFFFFFFFF: 
      processIR(lastIRValue);
      break;
    default:
      processIR(results.value);
      lastIRValue = results.value;
  }
}

void processIR(unsigned long value) {
  switch(value) {
    // POWER
    case 0xFFA25D: 
      isClockOn = isClockOn ? 0 : 1; 
      break;
    // VOL+
    case 0xFF629D:
      brightness += 5;
      if (brightness > 255) {
        brightness = 255; 
      }
      break;
    // VOL-
    case 0xFFA857:
      brightness -= 5;
      if (brightness < 0) {
        brightness = 0; 
      }
      break;
  }  
}
