#!/usr/bin/python

import serial

ser = serial.Serial("/dev/pts/5", 115200);

ser.write("aaa");
