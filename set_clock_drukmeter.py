#!/usr/bin/python

import serial
import time
import os
import sys
import platform

print "Python version : " + sys.version + "\n"

print "set_clock_drukmeter - Rev. 07-feb-2017" + "\n"
print "platform : " + platform.system()

if platform.system() == "Windows" :
 ser_port="COM9"
elif platform.system() == "Darwin" :
 ser_port="/dev/tty.usbmodem1411"

if len(sys.argv) == 2 :
 ser_port=sys.argv[1]

try:
 ser=serial.Serial(ser_port,9600,timeout=1)
except serial.serialutil.SerialException :
 print "No serial port / or port busy : " + ser_port
 sys.exit()

print "Serial port : " + ser_port + " is open"

print "Wait 15 seconds...."

time.sleep(15)

#get the unix timestamp

timestamp=int(time.time())

ser.write(str(timestamp) + "\n")

print "send : " + str(timestamp)

ser.close()
