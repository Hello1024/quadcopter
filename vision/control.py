import serial
import sys
import struct
import numpy as np

class SerialConnection:

  def __init__(self):
    print "Serial Connecting"
    self.ser = serial.Serial('/dev/ttyUSB0', 9600)
    while True:
      a = self.ser.read(1);
      sys.stdout.write(a)
      if a=="Z":
        break
    print "\nSerial Connected"

  def update_quad(self, quadno, controls):
    """Set the throttle etc. controls on a quad to a given value.
    
    Controls are Aileron, Elevator, Throttle, Rudder in that order.
    
    Controls vary (-1.0 1.0).  Throttle can't be negative."""
    assert len(controls) == 4
    
    def send_packed(val):
      self.ser.write(struct.pack( ">H", val ))

    def send_control(val):
      send_packed(int(np.clip(val, 0, 999)))

    send_packed(quadno)    
    send_control((controls[0]+1.0)*500)
    send_control((controls[1]+1.0)*500)
    send_control((controls[2])*1000)
    send_control((controls[3]+1.0)*500)

