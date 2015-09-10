#include "SPI.h"
#include "CX10.h"

CX10* transmitter;

struct {
  bool armed;
  byte state;
  uint32_t next;
} armed[CX10::CRAFT];

void setup()                   
{
  Serial.begin(9600);
  Serial.println("Arduino alive");
  delay(1000);
    Serial.println("Arduino alive again");
  transmitter = new CX10();
  if (transmitter->isHealthy())
    Serial.println("XN297 alive");
  else
    Serial.println("XN297 is dead");

  for ( ; ; ) {
    transmitter->loop();
    // I cannot reliably bind more than 2.
    // If its a CX-10A and a CX-10 it appears the 10A has to be switched on first.
    if (transmitter->boundCraft() > 0)
      break;
  }
  transmitter->stopBinding();
  for (uint32_t t = millis() + 3000; t > millis(); )
    transmitter->loop();
      
  Serial.print('Z');  // sync
  memset(armed, 0, sizeof armed);  
}

uint16_t read16()
{  // assumes MSB first.
  uint16_t result = Serial.read();
  result = (result << 8) + Serial.read();
  return result;
}

void loop()
{
  transmitter->loop();
  
  while (Serial.available() >= 10) {
    if (Serial.available() >= 15) {
      // Sadly the HardwareSerial library neither has any way to detect lost data
      // nor does it export RX_BUFFER_SIZE, which can be as low as 16.
      // SoftwareSerial interferes with radio timing causing glitches.
      while(1) Serial.print("Overflow");
    }
    uint16_t s = read16();  // craft number
 
    if (s < transmitter->boundCraft() && armed[s].armed) {
      transmitter->setAileron(s, read16());
      transmitter->setElevator(s, read16());
      transmitter->setThrottle(s, read16());
      transmitter->setRudder(s, read16());
    }
  }

  for (int s = 0; s < transmitter->boundCraft(); ++s) {
    // Arming is not needed, it turns out. Leave code in for now, but disabled.
    armed[s].armed = true;
    if (armed[s].armed)
      continue;
    if (armed[s].state == 0) {
      transmitter->setThrottle(s, 0);
      armed[s].state++;
      armed[s].next = millis() + 3000;
    } else if (millis() >= armed[s].next) {
      Serial.print(millis());
      Serial.print(' ');
      Serial.print(armed[s].state);
      Serial.print(' ');
      Serial.print(s);
      Serial.println("");
      if (armed[s].state == 1) {
        transmitter->setThrottle(s, 1000);
        armed[s].state++;
        armed[s].next = millis() + 100;
      } else if (armed[s].state == 2) {
        transmitter->setThrottle(s, 0);
        armed[s].state++;
        armed[s].next = millis() + 500;
      } else {
        armed[s].armed = true;
        Serial.print("Craft ");
        Serial.print(s);
        Serial.println(" armed");
      }
    }
  }
}

