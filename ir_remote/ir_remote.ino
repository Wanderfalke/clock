#include <Wire.h>
#include <IRremote.h>

#define I2C_ADDR 9
#define IR_PIN 12

IRrecv irrecv(IR_PIN);
decode_results results;
unsigned long lastValue = 0xFFFFFFFF;
byte currentCode = 0;

void setup() {
  Wire.begin(I2C_ADDR);
  Wire.onRequest(sendCurrentCode); 
  irrecv.enableIRIn();
}

void loop() {    
  if (irrecv.decode(&results)) {
    translateIR(); 
    delay(200);
    irrecv.resume();
  }  
}

void translateIR() {
  switch(results.value) {
    // REPEAT
    case 0xFFFFFFFF:
      if (results.value != 0xFF629D && results.value != 0xFFC23D && results.value != 0xFF906F) {
        processIR(lastValue);
      }
      break;
    default:
      processIR(results.value);
      lastValue = results.value;
  }
}

void processIR(unsigned long value) {
  switch(value) {
    // VOL+ / CH
    case 0xFF629D: setCurrentCode(1); break;
    // FAST FORWARD / PAUSE
    case 0xFFC23D: setCurrentCode(2); break;
    // UP / EQ
    case 0xFF906F: setCurrentCode(3); break;
    // FUNC|STOP / CH+
    case 0xFFE21D: setCurrentCode(4); break;
    // POWER / CH-
    case 0xFFA25D: setCurrentCode(5); break;
    // 1
    case 0xFF30CF: setCurrentCode(6); break;
    // 2
    case 0xFF18E7: setCurrentCode(7); break;
    // 3
    case 0xFF7A85: setCurrentCode(8); break;
    // 4
    case 0xFF10EF: setCurrentCode(9); break;
    // 5
    case 0xFF38C7: setCurrentCode(10); break;
    // 6
    case 0xFF5AA5: setCurrentCode(11); break;
    // 7
    case 0xFF42BD: setCurrentCode(12); break;
    // 8
    case 0xFF4AB5: setCurrentCode(13); break;
    // 9
    case 0xFF52AD: setCurrentCode(14); break;
  }  
}

void setCurrentCode(byte value) {
  currentCode = value;
}

void sendCurrentCode() {
  Wire.write(currentCode);
  setCurrentCode(0);
}
