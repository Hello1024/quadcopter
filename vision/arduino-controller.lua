#!/usr/bin/env qlua

-- sudo  chmod o+rw /dev/ttyU*; sudo stty -F /dev/ttyU* 115200



function write(f, a)
  f:write(string.char(a/256,a%256))
  f:flush()
end


os.execute("stty -F /dev/ttyU* 115200 raw")

f = assert(io.open("/dev/ttyUSB0", "w"))
os.execute("sleep 4")
for j=1,250 do
   f:write(string.char(255))
   write(f, 500);
   write(f, 500);
   write(f, (j%11)*100);
   write(f, 500);

  os.execute("sleep 0.1")
end
