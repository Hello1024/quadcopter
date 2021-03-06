import serial
import sys
import time

ser = serial.Serial('/dev/cu.usbserial-DC008KDX', 115200)

while True:
    x = ser.read()
    sys.stdout.write(x)
    sys.stdout.flush()
#    print ord(x), x
    if x == 'Z':
        break

print "Ready!"

def send(quad_num, throttle, rudder, elevator, aileron):
    echo()
    print "q = ", quad_num, "t =", throttle, "r =", rudder, "e =", elevator, "a =", aileron
    send16(quad_num)
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
        sys.stdout.flush()

def ack():
    while True:
        t = ser.read()
        sys.stdout.write(t)
        if t == '+':
            break

MAX = 1000
MIN = 0
MID = 500

# ARM (Don't - now auto-armed by the proxy)
#send(0, MIN, MID, MID, MID)
#time.sleep(.1)
#send(0, MAX, MID, MID, MID)
#time.sleep(.1)
#send(0, MIN, MID, MID, MID)
#time.sleep(.1)

# Wait for first craft to arm
t = time.time() + 2
while time.time() < t:
    echo()

MAXT = MAX/4

# Run
for t in range(MAXT):
    send(0, t, MID, MID, MID)
#    time.sleep(.01)

for t in range(MAXT, 0, -1):
    send(0, t, MID, MID, MID)
#    time.sleep(.01)
