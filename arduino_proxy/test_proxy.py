import serial
import sys
import time

ser = serial.Serial('/dev/cu.usbserial-DC008KDX', 115200)

while True:
    x = ser.read()
    sys.stdout.write(x)
#    print ord(x), x
    if x == 'Z':
        break

print "Ready!"

def send(throttle, rudder, elevator, aileron):
    echo()
    print "t =", throttle, "r =", rudder, "e =", elevator, "a =", aileron
    send16(aileron)
    send16(elevator)
    send16(throttle)
    send16(rudder)
    ack()

def send16(n):
    send8((n >> 8) & 0xff)
    send8(n & 0xff)

def send8(n):
    ser.write(chr(n))

def echo():
    while ser.inWaiting():
        t = ser.read()
        sys.stdout.write(t)

def ack():
    while True:
        t = ser.read()
        sys.stdout.write(t)
        if t == '+':
            break

MAX = 1000
MIN = 0
MID = 500

# ARM
send(MIN, MID, MID, MID)
time.sleep(.1)
send(MAX, MID, MID, MID)
time.sleep(.1)
send(MIN, MID, MID, MID)
time.sleep(.1)

MAXT = MAX/4

# Run
for t in range(MAXT):
    send(t, MID, MID, MID)
#    time.sleep(.01)

for t in range(MAXT, 0, -1):
    send(t, MID, MID, MID)
#    time.sleep(.01)
