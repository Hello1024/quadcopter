#include "CX10.h"

CX10* transmitter;

void setup()                   
{
  Serial.begin(115200);
  Serial.println("hello");
  transmitter = new CX10();
  transmitter->bind(0);
}


uint16_t read16() {  // assumes MSB first.
  uint16_t result = Serial.read();
  result = (result << 8) + Serial.read();
}

void loop()
{
  transmitter->loop();
  
  if (Serial.available() >= 8) {
    transmitter->setAileron(0, read16());
    transmitter->setElevator(0, read16());
    transmitter->setThrottle(0, read16());
    transmitter->setRudder(0, read16());  
  }
};
