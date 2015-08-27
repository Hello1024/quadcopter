#include "SPI.h"
#include "CX10.h"

CX10* transmitter;

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
  
  transmitter->bind(0);
  Serial.println("found a craft!");
  transmitter->printTXID(0);
  Serial.print('+');
  transmitter->printAID(0);
  Serial.print('\n');

  transmitter->bind(1);
  Serial.println("found a craft!");
  transmitter->printTXID(1);
  Serial.print('+');
  transmitter->printAID(1);
  Serial.print('\n');

  Serial.print('Z');  // sync
  
  // TODO:  auto-arm  (throttle from 0 -> 1000 -> 0 again)
}

uint16_t read16()
{  // assumes MSB first.
  uint16_t result = Serial.read();
  result = (result << 8) + Serial.read();
  return result;
}

void loop()
{  
  transmitter->loop(0);
  transmitter->loop(1);

  if (Serial.available() >= 8) {
    uint16_t a = read16();
    uint16_t e = read16();
    uint16_t t = read16();
    uint16_t r = read16();

    transmitter->setAileron(0, a);
    transmitter->setElevator(0, e);
    transmitter->setThrottle(0, t);
    transmitter->setRudder(0, r);

    transmitter->setAileron(1, a);
    transmitter->setElevator(1, e);
    transmitter->setThrottle(1, t);
    transmitter->setRudder(1, r);

    Serial.print('+');  // ack
  }
};
