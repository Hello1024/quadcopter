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
  Serial.begin(115200);
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
    if (transmitter->boundCraft() > 2)
      break;
  }
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
  
  if (Serial.available() >= 8) {
    uint16_t a = read16();
    uint16_t e = read16();
    uint16_t t = read16();
    uint16_t r = read16();

    for (int s = 0; s < transmitter->boundCraft(); ++s) {
      if (armed[s].armed) {
        transmitter->setAileron(s, a);
        transmitter->setElevator(s, e);
        transmitter->setThrottle(s, t);
        transmitter->setRudder(s, r);
      }
    }

    Serial.print('+');  // ack
  }

  for (int s = 0; s < transmitter->boundCraft(); ++s) {
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

