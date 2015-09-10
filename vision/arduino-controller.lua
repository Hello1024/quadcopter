#!/usr/bin/env qlua

-- Depending on your system, you might need to run this:
-- sudo  chmod o+rw /dev/ttyU*;


QuadConnection = {}
QuadConnection.__index = QuadConnection

function QuadConnection.create(device)
  local this = {}
  setmetatable(this, QuadConnection)
  
  this.f = S.open(device, "rdwr, noctty, nonblock")
  local f_term = S.tcgetattr(f)
  f_term:makeraw()
  f_term.c_cflag = S.c.CFLAG["CS8,CREAD,CLOCAL,CSTOPB"]
  f_term.ispeed = 9600
  f_term.ospeed = 9600
  this.f:tcsetattr("now", f_term)

  -- Opening it resets the hardware.  Now wait for device to indicate
  -- readyness.
  while true do
    str = this.f:read(nil, 1024)
    if str then
      print(str)
    end
    -- Possible race bug here, Z may not appear alone, or might appear
    -- in other startup info.
    if str == "Z" then
      break
    end
  end
  return this
end

function QuadConnection:SendToQuad(quadnumber, ch1, ch2, ch3, ch4)
  write(self.f, quadnumber);
  write(self.f, ch1);
  write(self.f, ch2);
  write(self.f, ch3);
  write(self.f, ch4);
  
  -- Could read and report a partial string...   Oh well.
  str = self.f:read(nil, 1024)
  if str then
    print("Arduino says: " .. str:match( "^%s*(.-)%s*$" ))
  end
end


local S = require "syscall"


function write(fd, a)
  -- If this assertion fails, it's a buffer overflow on the host side
  -- That likley means the arduino suffered a buffer overflow long ago.
  assert(fd:write(string.char(a/256,a%256)) == 2)
end

--local fd = open("/dev/ttyUSB0")

--for j=1,250 do
--  sendtoquad(fd, 0, 500, 500, (j%11)*100, 500)
  
  

--  os.execute("sleep 0.1")
--end
