/*
  cx10.h - Library for sending commands to a fleet of CX10 quadcopters
  
  TODO:  Actually only currently supports 1 craft, despite API 
         being designed for more.
*/
#ifndef CX10_h
#define CX10_h

#include "Arduino.h"


class CX10 {
public:
  CX10();
  loop();
  bind(int slot);
  setAileron(int slot, int value);
  setElevator(int slot, int value);
  setThrottle(int slot, int value);
  setRudder(int slot, int value);
}

#endif
