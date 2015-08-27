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
  void loop(int slot);
  void bind(int slot);
  void setAileron(int slot, int value);
  void setElevator(int slot, int value);
  void setThrottle(int slot, int value);
  void setRudder(int slot, int value);
  void printAID(int slot);
  void printTXID(int slot);
  bool isHealthy() const { return healthy_; }
  
private:
  uint8_t _spi_read_address(uint8_t address);
  uint8_t _spi_read();
  void _spi_write_address(uint8_t address, uint8_t data);
  void _spi_write(uint8_t command);
  void Read_Packet();
  void Write_Packet(int slot, uint8_t init);
  void bind_XN297(int slot);
  
  bool healthy_;

  static const size_t CRAFT = 4;
  struct Craft {
    static const size_t CHANNELS = 6;

    uint8_t txid[4];  // transmitter ID
    uint8_t freq[4];  // frequency hopping table
    uint8_t aid[4];   // aircraft ID
    uint16_t Servo_data[CHANNELS];
  } craft_[CRAFT];
};

#endif
