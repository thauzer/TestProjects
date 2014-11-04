#!/usr/bin/env python

"""
A simple echo server
"""

import socket

host = '192.168.0.42'
port = 50123
backlog = 5
size = 1024*1024
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host,port))
s.listen(backlog)
while 1:
    client, address = s.accept()
    data = client.recv(size)
    if data:
        print data
    client.close() 
