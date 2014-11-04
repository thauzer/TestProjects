#!/usr/bin/python3

import os, re, sys, string
import threading
import time

#grettings = sys.args[1]

menuOptions = ("Send character: ", "Druga izbira", "Tretja izbira", "Izhod")

class SerialReader(threading.Thread):
	self.readCharacter = ""
	
	def __init__(self, portName, justSomething)
		self.portName = portName
		self.justSomething = justSomething
		threading.Thread.__init__(self)
	
	def run (self):
		print("run method")
		time.sleep(0.001)
	
def readSerialPort(threadName)
	print("Reading from Serial port")

def main():
	print("Welcome to the led blinker")
	loopRun = 1
	choice = 0
	while loopRun == 1:
		os.system("clear")
		choice = 0
		for izbira in range(len(menuOptions)):
			print(izbira+1,". ", menuOptions[izbira])
		
		strChoice = input("Your choice: ")
		if strChoice.isnumeric():
			choice = int(strChoice)
		
		while choice:
			if choice == 1:
				sendChar = input("Input character to send: ")
				if sendChar:
					print("Sending: --> ", sendChar[0], " <--")
				else:
					print("Nothing sent")
				break
			if choice == 2:
				print("You have choosen the second option")
				break
			if choice == 3:
				print("You have choosen the third option")
				break
			if choice == len(menuOptions):
				loopRun = 0
				print("Exiting program")
				break
			print("Invalid option")
			break
		
#		os.system("pause")
		input("Press ENTER to continue")


if __name__ == "__main__":
	main()
