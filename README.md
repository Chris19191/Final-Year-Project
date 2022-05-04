# Final-Year-Project
Comprehensive Human Wellbeing Monitoring System

ArduinoSensorRead.ino 
This is the code that was run on the Sparkfun Redboard Plus to read the data from the heart rate, temperature and motion sensors.

NodeRedFlow.json 
This contains the code for the Node Red flow which was created to read theEmotiv Insight data and save it to a .csv file. This can be imported into Node Red by opening a new flow, going to the menu in the top right, selecting 'import', and then 'select a file to import'. 

FYP_ReadData.py 
This Python script was used to read in data from the Sparkfun Redboard via a serial port and Emotiv Insight headset via a .csv file. It then collated the data and saved it to a .csv file for future analysis.

downsample.m 
This is a short MATLAB script which was used to extract extract data at 10 second time intervals from the recorded PPG data.
