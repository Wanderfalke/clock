#include <Adafruit_NeoPixel.h>
#include <DS3231.h>
#include <Wire.h>

#define LED_PIN 6
#define LED_COUNT 209
#define I2C_ADDR 9

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
DS3231 clock;

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

RTCDateTime dt;
uint8_t loopCounter = 0;
uint8_t isClockOn = 1;
uint8_t isInverted = 0;
uint8_t isRainbow = 0;
uint8_t brightness = 220;
uint32_t currentColor = strip.Color(0, 0, 0);
uint32_t black = strip.Color(0, 0, 0);
uint32_t rainbowHue = 0;
Color currentRGB = {255, 0, 0};

// Postitionen der Sekunden-Punkte
uint8_t dots[] = {85, 123};  

// Positionen der Ziffern
uint8_t positions[4][15] = {
  {74, 73, 72, 77, 78, 79, 112, 111, 110, 115, 116, 117, 150, 149, 148},
  {70, 69, 68, 81, 82, 83, 108, 107, 106, 119, 120, 121, 146, 145, 144},
  {64, 63, 62, 87, 88, 89, 102, 101, 100, 125, 126, 127, 140, 139, 138},
  {60, 59, 58, 91, 92, 93, 98, 97, 96, 129, 130, 131, 136, 135, 134}
};

// Layouts der Ziffern 0-9
uint8_t digits[10][15] = {
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
  // Initialisiere I2C-Kommunikation
  Wire.begin();

  // Initialisiere Echtzeituhr
  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__); 
  updateTime();

  // Initialisiere LEDs
  strip.begin();
  strip.show();
  //strip.setBrightness(brightness);

  playIntro();
}

void loop() {
  loopCounter += 1;
  
  if (isClockOn) {
    updateDisplay();
  }

  // Aktualisiere Uhrzeit und checke Fernbedienung alle 10 Iterationen (~200ms)
  if (loopCounter % 10 == 0) {
    updateTime();
    receiveKey();
  }
  
  delay(20);
}

void playIntro() {
  strip.clear();
  updateCurrentColor();

  introLoop(currentColor);
  introLoop(black);
}

void introLoop(uint32_t color) {
  for(uint8_t j = 0; j < 19; j++) {
    for(uint8_t i = 0; i < 11; i++) {
      strip.setPixelColor(i * 19 + j, color);
    }

    strip.show();
    delay(30);
  }
}

void updateTime() {
  dt = clock.getDateTime();
}

void updateDisplay() {
  if(isRainbow) updateRainbow();
  if(!isRainbow) updateCurrentColor();

  strip.clear();

  for(uint8_t i = 0; i < strip.numPixels(); i++) {
    setPixel(i, isInverted);
  }
  
  uint8_t h1 = dt.hour / 10;
  uint8_t h2 = dt.hour % 10;
  uint8_t m1 = dt.minute / 10;
  uint8_t m2 = dt.minute % 10;
  
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

void showDigit(uint8_t digit, uint8_t pos) {
  for(uint8_t i = 0; i < 15; i++) {
    if (digits[digit][i] == 1) {
      setPixel(positions[pos][i], !isInverted);
    }
  }
}

void setPixel(uint8_t pixel, uint8_t visible) {
  uint32_t pixelColor = black;
  if (visible && !isRainbow) pixelColor = currentColor;
  if (visible && isRainbow) pixelColor = calculateRainbowColor(pixel);

  strip.setPixelColor(pixel, pixelColor);
}

void updateCurrentColor() {
  currentColor = strip.Color((brightness*currentRGB.r/255), (brightness*currentRGB.g/255), (brightness*currentRGB.b/255));  
}

void changeValue(uint8_t *var, int8_t delta) {
  int16_t newValue = *var + delta;

  if (newValue < 0) {
    *var = 0;
  } else if (newValue > 255){
    *var = 255;
  } else {
    *var = newValue;
  }
}

void updateRainbow() {
  rainbowHue += 256;
  if (rainbowHue >= 5*65536) {
    rainbowHue = 0;
  }
}

uint32_t calculateRainbowColor(uint8_t pixel) {
  uint16_t pixelHue = rainbowHue + (pixel * 65536L / (strip.numPixels() * 2)); 

  return strip.gamma32(strip.ColorHSV(pixelHue, 255, brightness));
}

void receiveKey() {
  Wire.requestFrom(I2C_ADDR, 1);
  
  uint8_t response = 0;
  while (Wire.available()) {
      response = Wire.read();
  } 

  if (response > 0) {
    processKey(response);
  }
}

void processKey(uint8_t value) {
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
      changeValue(&brightness, 10); 
      break;
    // POWER / CH-
    case 5: 
      changeValue(&brightness, -10); 
      break;
    // 1
    case 6:
      currentRGB = {255, 0, 0}; break;
    // 2
    case 7: 
      currentRGB = {0, 255, 0}; break;
    // 3
    case 8: 
      currentRGB = {0, 0, 255}; break;
    // 4
    case 9:
      changeValue(&currentRGB.r, 10); break;
    // 5
    case 10: 
      changeValue(&currentRGB.g, 10); break;
    // 6
    case 11: 
      changeValue(&currentRGB.b, 10); break;
    // 7
    case 12: 
      changeValue(&currentRGB.r, -10); break;
    // 8
    case 13: 
      changeValue(&currentRGB.g, -10); break;
    // 9
    case 14: 
      changeValue(&currentRGB.b, -10); break;
    // 0
    case 15:
      currentRGB = {random(0, 256), random(0, 256), random(0, 256)};
  }  
}
