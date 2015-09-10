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
  void loop();
  void setAileron(int slot, int value);
  void setElevator(int slot, int value);
  void setThrottle(int slot, int value);
  void setRudder(int slot, int value);
  void printAID(int slot) const;
  void printTXID(int slot) const;
  bool isHealthy() const { return healthy_; }
  uint8_t boundCraft() const { return nextSlot_; }
  void stopBinding() { bindAllowed_ = false; }
  
  static const size_t CRAFT = 4;

private:
  uint8_t _spi_read_address(uint8_t address);
  uint8_t _spi_read();
  void _spi_write_address(uint8_t address, uint8_t data);
  void _spi_write(uint8_t command);
  void Read_Packet();
  void Write_Packet(int slot, uint8_t init);
  bool bind_XN297(int slot);
  
  bool healthy_;
  bool bindAllowed_;
  uint32_t nextBind_;
  uint8_t nextSlot_;

  struct Craft {
    static const size_t CHANNELS = 6;

    uint8_t txid[4];  // transmitter ID
    uint8_t freq[4];  // frequency hopping table
    uint8_t aid[4];   // aircraft ID
    uint16_t Servo_data[CHANNELS];
    uint32_t nextPacket;
    uint8_t chan;
  } craft_[CRAFT];
};

#endif
