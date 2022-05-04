//Include Libraries
#include <Wire.h>
#include "MAX30105.h" //PPG Sensor
#include "heartRate.h" //heart rate algorithm 
#include "SparkFun_Qwiic_KX13X.h"  //Accelerometer
#include <SparkFun_TMP117.h> //Temperature sensor
#include "RTClib.h" // Real time clock
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"//LCD
#define TFT_DC 9
#define TFT_CS 10

//Initialise sensors/ devices
QwiicKX134 Accelerometer; 
MAX30105 PPGsensor;
TMP117 TMPsensor;
RTC_DS3231 rtc;
outputData AccelData;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//PPG sensor variables
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;

//Accelerometer variables
float accelerationVector_f;
int threshold = 12; //minimum acceleration to be counted as a step
int steps, stepFlag, prevStep, stepInterval = 0;
float timer, prevTime, walkTimer, prevWalkTimer = 0; 
bool isWalking;

//Temp variable
float tempC = 0; 

//Time variable
long int prevTimeLCD = millis();
unsigned long PrevTimeUnix;
int loopcounter = 0;

void setup() {
  tft.begin();
  while(!Serial){
    delay(50);
  }
  Serial.begin(115200);
  Wire.setClock(400000);
  Wire.begin();

  //RTC setup
  if (! rtc.begin()) {
    Serial.println("RTC was not found. Freezing.");
    while (1);
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //Get system time
  DateTime now = rtc.now();
  PrevTimeUnix = now.unixtime();
  
  //Temp sensor setup
  if( !TMPsensor.begin()){
    Serial.println("TMP117 temperature sensor was not found. Freezing.");
    while(1);
  }

  //PPG sensor setup
  if (!PPGsensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 pulse sensor was not found. Freezing.");
    while (1);
  }
  //Setup to sense a nice looking saw tooth on the plotter
  byte ledBrightness = 0x1F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  PPGsensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor
  //particleSensor.setPulseAmplitudeRed(0);

  //Accelerometer setup
  if( !Accelerometer.begin() ){
    Serial.println("Could not communicate with the the KX13X. Freezing.");
    while(1);
  }
  if( !Accelerometer.initialize(DEFAULT_SETTINGS)){ // Loading default settings.
  Serial.println("KX134 accelerometer was not found. Freezing.");
  while(1);
  }
 
  LCDtext(); //write initial values to LCD

}

void loop() {
  
  //Temperature sensor
  // Data Ready is a flag for the conversion modes - in continous conversion the dataReady flag should always be high
  if (TMPsensor.dataReady() == true) // Function to make sure that there is data ready to be printed, only prints temperature values when data is ready
  {
    tempC = TMPsensor.readTempC();// read temperature in °C
  }


  //PPG Sensor
  long irValue = PPGsensor.getIR();

//Accelerometer
  AccelData = Accelerometer.getAccelData();
  
  accelerationVector_f = sqrt( ((AccelData.xData)*(AccelData.xData)) + ((AccelData.yData)*(AccelData.yData)) + ((AccelData.zData)*(AccelData.zData)) );
    int accelerationVector = accelerationVector_f *10;
  
  if(accelerationVector>threshold && stepFlag ==0){
    if(prevTime){
      timer = millis(); //record current time since program start
      stepInterval = timer-prevTime; //calculate difference in time between last step and current step
      //Serial.print("Step Interval: "); 
      //Serial.println(stepInterval); //display step interval on serial monitor
    }
    prevTime = millis(); //record time since program start
  
    if(300<stepInterval && stepInterval<700 ){ //check that step interval is within range for walking
         steps = steps +1; //increment step counter 
          prevWalkTimer = millis();
         //tft.setRotation(0);
         LCDtext();
  
    }
    stepFlag = 1;  //set the step flag to prevent continuous acceleration being counted as multiple steps
  }
  
    if(accelerationVector <threshold && stepFlag ==1){ //reset step flag if it is set and acceleration below threshold
      stepFlag = 0;
    }
    
  walkTimer = millis();
  if((walkTimer - prevWalkTimer < 2000) && (walkTimer > 2000)){
    isWalking = true;  
  }else{
    isWalking = false;
  }

//    Serial.print("Current time: ");
//    Serial.println(millis());
//    Serial.print("Prev time: ");
//    Serial.println(prevTimeLCD);

    if ((millis() - prevTimeLCD) > 1000){
      //LCDtext(); 
      prevTimeLCD = millis();  
    }

    DateTime now = rtc.now();
    if (now.unixtime() != PrevTimeUnix){
      Serial.print("*RTC Time:");Serial.println(now.unixtime());
      PrevTimeUnix = now.unixtime();
      //Serial.print("Loops:"); Serial.println(loopcounter); // counting the loops for debug purposes
      loopcounter = 0;
    } else{
      loopcounter = loopcounter + 1;
    }


    Serial.print("TIME:"); Serial.print(millis()); Serial.print("; ");
    Serial.print("STEPS:"); Serial.print(steps); Serial.print("; ");
    Serial.print("WALK:"); Serial.print(isWalking); Serial.print("; ");
    Serial.print("IR:"); Serial.print(irValue); Serial.print("; ");
    Serial.print("TMP_C:"); Serial.print(tempC); Serial.println("; ");
    
}

unsigned long LCDtext() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.print("Steps: ");
  tft.println(steps);
  tft.print("Walking?: ");
  if (isWalking)  tft.println("Y");
  if (!isWalking)  tft.println("N");

  return micros() - start;
}
