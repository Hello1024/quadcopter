#!/usr/bin/env qlua

-- Depending on your system, you might need to run this:
-- sudo  chmod o+rw /dev/ttyU*;


local S = require "syscall"


function write(f, a)
  -- If this assertion fails, it's a buffer overflow on the host side
  -- That likley means the arduino suffered a buffer overflow long ago.
  assert(f:write(string.char(a/256,a%256)) == 2)
end

function sendtoquad(quadnumber, ch1, ch2, ch3, ch4)
  write(f, quadnumber);
  write(f, ch1);
  write(f, ch2);
  write(f, ch3);
  write(f, ch4);
end

function open(device)
  local f = S.open(device, "rdwr, noctty, nonblock")
  local f_term = S.tcgetattr(f)
  f_term:makeraw()
  f_term.ispeed = 115200
  f_term.ospeed = 115200
  f:tcsetattr("now", f_term)

  return f

end


local f = open("/dev/ttyUSB0")

for j=1,250 do
  write(f, 0);
  write(f, 500);
  write(f, 500);
  write(f, (j%11)*100);
  write(f, 500);
  str = f:read(nil, 1024)
  if str then
    print(str)
  end
  os.execute("sleep 0.1")
end
