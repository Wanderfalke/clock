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
int isRainbow = 0;
int brightness = 255;
uint32_t black = strip.Color(0, 0, 0);
unsigned long lastIRValue = 0xFFFFFFFF;
Color currentColor = {255, 0, 0};
long rainbowHue = 0;

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
  Serial.begin(9600);
  
  clock.begin(); // Initialize DS3231
  // Send sketch compiling time to Arduino
  clock.setDateTime(__DATE__, __TIME__); 

  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show(); // Turn OFF all pixels ASAP
  strip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)

  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  if (irrecv.decode(&results)) {
    translateIR();
    irrecv.resume();
  }

  // have we received an IR signal?
  if (isClockOn) {
    updateClock();
    if(isRainbow) updateRainbow();
  } else {
    strip.clear();
    strip.show();
  }
  
  delay(100);
}

void updateClock() {
  dt = clock.getDateTime();

  int h1 = dt.hour / 10;
  int h2 = dt.hour % 10;
  int m1 = dt.minute / 10;
  int m2 = dt.minute % 10;
  
  strip.clear();

  for(int i = 0; i < strip.numPixels(); i++) {
    setPixel(i, isInverted);
  }
  
  showDigit(h1, 0);
  showDigit(h2, 1);
  showDigit(m1, 2);
  showDigit(m2, 3);

  if (dt.second % 2 == 0) {
    setPixel(dots[0], !isInverted);
    setPixel(dots[1], !isInverted);
  }
  
  strip.show();
}

void showDigit(int digit, int pos) {
  for(int i = 0; i < 15; i++) {
    if (digits[digit][i] == 1) {
      setPixel(positions[pos][i], !isInverted);
    }
  }
}

void setPixel(int pixel, int visible) {
  uint32_t pixelColor = black;
  if (visible && !isRainbow) pixelColor = createColor(currentColor);
  if (visible && isRainbow) pixelColor = calculateRainbowColor(pixel);

  strip.setPixelColor(pixel, pixelColor);
}

uint32_t createColor(Color color) {
  return strip.Color((brightness*color.r/255), (brightness*color.g/255), (brightness*color.b/255));
}

void translateIR() {
  switch(results.value) {
    // REPEAT
    case 0xFFFFFFFF:
      if (results.value != 0xFF629D && results.value != 0xFFC23D && results.value != 0xFF906F) {
        processIR(lastIRValue);
      }
      break;
    default:
      processIR(results.value);
      lastIRValue = results.value;
  }
}

void processIR(unsigned long value) {
  Serial.println("processIR");
  Serial.println(value);
  Serial.println(0xFF906F);

  switch(value) {
    // VOL+ / CH
    case 0xFF629D:
      isClockOn = !isClockOn; 
      break;
    // FAST FORWARD / PAUSE
    case 0xFFC23D:
      isInverted = !isInverted; 
      break;
    // UP / EQ
    case 0xFF906F:
      isRainbow = !isRainbow;
      Serial.println("Rainbow");
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

void updateRainbow() {
  rainbowHue += 256;
  if (rainbowHue >= 5*65536) {
    rainbowHue = 0;
  }
}

uint32_t calculateRainbowColor(int pixel) {
  int pixelHue = rainbowHue + (pixel * 65536L / strip.numPixels()); 

  return strip.gamma32(strip.ColorHSV(pixelHue, 255, brightness));
}
