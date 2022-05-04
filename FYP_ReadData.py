# Import modules
import serial.tools.list_ports
import serial
import time
import csv
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import find_peaks
from datetime import datetime

from serial import Serial

# Setup variables
IR_Arr = np.zeros(200)  # Setup empty array for IR values
time_Arr = np.zeros(200)  # Setup empty array for time stamps
counter = 0  # Initialise counter variable
starttime = datetime.now().strftime("%Y-%m-%d_%I-%M-%S")  # Define start time
global peak_pos
arduino_time_flag = 0  # Flag to allow the first arduino timestamp to be logged


ports = serial.tools.list_ports.comports()  # get port list
serialInst = serial.Serial()

portList = []  # define variable for list of available ports
BPM = 0  # initialise beats per minute variable
HRV = 0  # initialise heart rate variability variable

for onePort in ports:
	portList.append(str(onePort))
	print(str(onePort))

val = input("Select Port: COM")  # get port number from user

for x in range(0, len(portList)):
	if portList[x].startswith("COM" + str(val)):
		portVar = "COM" + str(val)
		print(portList[x])

ser = serial.Serial(portVar, 115200)  # setup port
ser.flushInput()  # instruct serial port to clear queue

with open("%s.csv" % starttime, "a") as fOut:  # create csv file
	writer = csv.writer(fOut, delimiter=",")
	writer.writerow(["Arduino Time", "Steps", "Walk State", "IR", "BPM", "HRV", "Temp", "Relax", "Stress"])

while True:
	unix_time = time.time()  # Get system UNIX time
	# Read in data from serial port
	data = ser.readline(1000)  # read serial port
	decodeData = data.decode("utf-8").strip()  # decode string using utf-8 and strip a newline from it

	# Checking that the RTC connected to the arduino is keeping in sync
	# Check if the line decoded is a timestamp (denoted by *) and that it is the first occurrence
	if (decodeData[0] == '*') & (arduino_time_flag == 0):
		arduino_unix_time = int(decodeData[10:len(decodeData)])  # read in time and convert to integer
		arduino_unix_time = arduino_unix_time - 3600  # adjust for BST
		initial_timediff = abs(unix_time - arduino_unix_time)  # Calculate time difference
		arduino_time_flag = 1  # Set flag to show that initial time difference has been calculated
	# Check that the line is a timestamp
	elif (decodeData[0] == '*') & (arduino_time_flag == 1):
		arduino_unix_time = int(decodeData[10:len(decodeData)])  # read in time and convert to integer
		arduino_unix_time = arduino_unix_time - 3600  # adjust for BST
		# compare time difference between arduino UNIX time and system UNIX time against original time difference
		if abs(unix_time - arduino_unix_time) > (initial_timediff + 5):
			print("Arduino time has drifted by >5 seconds!")  # Print message warning that time has drifted
	# Read in sensor data
	else:
		dataList = decodeData.split(";")  # Split data string into a list of strings using ; as delimiter
		arduino_time = dataList[0]  # Assign first item in list to variable 'time'
		arduino_time = int(arduino_time[5:len(arduino_time)])  # extract numerical part from string and convert to int
		steps = dataList[1]
		steps = int(steps[7:len(steps)])
		walkState = dataList[2]
		walkState = int(walkState[6:len(walkState)])
		IR = dataList[3]
		IR = int(IR[4:len(IR)])
		temp = dataList[4]
		temp = float(temp[7:len(temp)])

		# open file containing data from EEG headset
		with open("EEGdata.csv") as fIn:
			lines = fIn.read().splitlines()  # split the file into a list on newlines
			last_line = lines[-1]  # take the 2nd to last line
			EEG_data = last_line.split(",")  # split line using , as delimiter
			EEG_time = int(EEG_data[2])  # Assign 3rd item in list to variable 'EEG_time'
			if (unix_time * 1000) - EEG_time < 10000:
				relax = EEG_data[0]  # Assign 1st item in list to variable 'relax'
				stress = EEG_data[1]  # Assign 2nd item in list to variable 'stress'
			else:
				relax = "NO DATA"
				stress = "NO DATA"


		if IR < 50000:
			print("No finger detected!")

		# Fill arrays
		if counter < 200:
			IR_Arr[counter] = IR  # add IR values to IR array
			time_Arr[counter] = arduino_time  # add timestamp values to timestamp array
		else:
			IR_Arr = np.roll(IR_Arr, -1)  # shift data in array
			IR_Arr[199] = IR  # add new value to first element

			time_Arr = np.roll(time_Arr, -1)
			time_Arr[199] = arduino_time

		# if time_Arr[99]:
		if counter > 250:
			peaks = find_peaks(IR_Arr, height=10000, threshold=None, prominence=500)
			height = peaks[1]['peak_heights']  # list of the heights of the peaks
			peak_pos = time_Arr[peaks[0]]  # list of the peaks positions

			# plot data
			if counter == 251:
				fig = plt.figure()
				ax = fig.subplots()
				ax.plot(time_Arr, IR_Arr)
				ax.scatter(peak_pos, height, color='r', s=15, marker='D', label='Maxima')
				ax.legend()
				ax.grid()
				plt.show()

			# IBI and BPM calculation
			IBI_Arr = np.zeros(len(peak_pos)-1)
			for i, x in enumerate(peak_pos):
				if i == 0:
					prevValue = x
				else:
					IBI_Arr[i-1] = x - prevValue
					prevValue = x
			BPM_sum = 0
			# Sum all values in array
			for x in IBI_Arr:
				BPM_sum = BPM_sum + x
			AvgIBI = BPM_sum / (len(IBI_Arr))  # divide by total elements in array
			BPM = round(60 * 1000/AvgIBI)  # convert average IBI to HR and round
			print("BPM: ", end='')
			print(BPM)

			# HRV calculation
			# Differences of successive beat intervals
			IBI_Arr_diff = np.zeros(len(IBI_Arr)-1)
			for i, x in enumerate(IBI_Arr):
				if i == 0:
					prevValue = x
				else:
					IBI_Arr_diff[i-1] = abs(prevValue - x)
					prevValue = x
			# Square each element in IBI differences array
			for i, x in enumerate(IBI_Arr_diff):
				IBI_Arr_diff[i] = IBI_Arr_diff[i] ** 2
			HRV_sum = 0
			for x in IBI_Arr_diff:
				HRV_sum = HRV_sum + x
			Avg = HRV_sum / (len(IBI_Arr_diff))
			HRV = round(Avg ** 0.5)
			print("HRV: ", end='')
			print(HRV)

		counter += 1

		# Open file to save data into
		with open("%s.csv" % starttime, "a", newline='') as fOut:  # create csv file
			writer = csv.writer(fOut, delimiter=",")
			writer.writerow([arduino_time, steps, walkState, IR, BPM, HRV, temp, relax, stress])  # write collected data to file
