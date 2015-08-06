#include "CX10.h"

CX10* transmitter;

void setup()                   
{
  Serial.begin(115200);
  Serial.println("Arduino alive");
  transmitter = new CX10();
  if (transmitter->healthy)
    Serial.println("XN297 alive");
  else
    Serial.println("XN297 is dead");
  
  transmitter->bind(0);
  Serial.println("found a craft!");
  
  // TODO:  auto-arm  (throttle from 0 -> 1000 -> 0 again)
}


uint16_t read16() {  // assumes MSB first.
  uint16_t result = Serial.read();
  result = (result << 8) + Serial.read();
}

void loop()
{
  transmitter->loop();
  
  if (Serial.available()) {
    transmitter->setAileron(0, read16());
    transmitter->setElevator(0, read16());
    transmitter->setThrottle(0, read16());
    transmitter->setRudder(0, read16());  
  }
};
