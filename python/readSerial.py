#!/usr/bin/python

import serial

ser = serial.Serial('/dev/pts/4', 115200)

while 1:
	print ser.read()
