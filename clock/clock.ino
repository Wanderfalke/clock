#include <Adafruit_NeoPixel.h>
#include <DS3231.h>
#include <Wire.h>

#define LED_PIN    6
#define LED_COUNT 209
#define I2C_ADDR 9

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

DS3231 clock;
RTCDateTime dt;

struct Color {
  unsigned int r;
  unsigned int g;
  unsigned int b;
};

int loopCounter = 0;
int isClockOn = 1;
int isInverted = 0;
int isRainbow = 0;
unsigned int brightness = 150;
uint32_t black = strip.Color(0, 0, 0);
Color currentColor = {255, 0, 0};
long rainbowHue = 0;

// Positions of seconds dots.
int dots[] = {85, 123};  

// Positions of clock digits.
int positions[4][15] = {
  {74, 73, 72, 77, 78, 79, 112, 111, 110, 115, 116, 117, 150, 149, 148},
  {70, 69, 68, 81, 82, 83, 108, 107, 106, 119, 120, 121, 146, 145, 144},
  {64, 63, 62, 87, 88, 89, 102, 101, 100, 125, 126, 127, 140, 139, 138},
  {60, 59, 58, 91, 92, 93, 98, 97, 96, 129, 130, 131, 136, 135, 134}
};

// Layouts of digits (0-9).
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
  Serial.println("Initialize");
 
  // Initialize I2C communication.
  Wire.begin();

  // Initialize RTC.
  clock.begin();
  clock.setDateTime(__DATE__, __TIME__); 
  updateClock();

  // Initialize pixel strip.
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
}

void loop() {
  loopCounter += 1;
  
  if (isClockOn) {
    updateDisplay();
  }

  if (loopCounter % 10 == 0) {
    updateClock();
    receiveKey();
  }
  
  delay(20);
}

void updateClock() {
  dt = clock.getDateTime();
//  Serial.print(dt.hour);
//  Serial.print(":");
//  Serial.print(dt.minute);
//  Serial.print(":");
//  Serial.println(dt.second);
}

void updateDisplay() {
  if(isRainbow) updateRainbow();

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

void receiveKey() {
  Wire.requestFrom(I2C_ADDR, 1);
  
  byte response = 0;
  while (Wire.available()) {
      response = Wire.read();
  } 

  if (response > 0) {
    Serial.print("Key: ");
    Serial.println(response);

    processKey(response);
  }
}

void processKey(byte value) {
  switch(value) {
    // VOL+ / CH
    case 1:
      isClockOn = !isClockOn; 
      if (!isClockOn) {
        strip.clear();
        strip.show();
      }
      break;
    // FAST FORWARD / PAUSE
    case 2:
      isInverted = !isInverted; break;
    // UP / EQ
    case 3:
      isRainbow = !isRainbow; break;
    // FUNC|STOP / CH+
    case 4:
      changeValue(&brightness, 10); break;
    // POWER / CH-
    case 5: 
      changeValue(&brightness, -10); break;
    // 1
    case 6:
      currentColor = {255, 0, 0}; break;
    // 2
    case 7: 
      currentColor = {0, 255, 0}; break;
    // 3
    case 8: 
      currentColor = {0, 0, 255}; break;
    // 4
    case 9:
      changeValue(&currentColor.r, 10); break;
    // 5
    case 10: 
      changeValue(&currentColor.g, 10); break;
    // 6
    case 11: 
      changeValue(&currentColor.b, 10); break;
    // 7
    case 12: 
      changeValue(&currentColor.r, -10); break;
    // 8
    case 13: 
      changeValue(&currentColor.g, -10); break;
    // 9
    case 14: 
      changeValue(&currentColor.b, -10); break;
  }  
}
