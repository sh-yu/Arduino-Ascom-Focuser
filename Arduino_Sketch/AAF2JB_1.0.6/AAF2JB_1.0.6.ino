//----------------------------------------------------------------------------------------------------
// Include necessary libraries
//----------------------------------------------------------------------------------------------------
#include <OneWire.h>                       // DS18B20 temp sensor
#include <DallasTemperature.h>             // DS18B20 temp sensor  
#include <EEPROM.h>                        // EEPROM Library
//#include <U8glib.h>                        // For i2c OLED
#include <U8g2lib.h>
#include <BME280I2C.h>
#include <EnvironmentCalculations.h>
#include <Wire.h>

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
// User-configurable values. Only change values if you know what you are doing.
//---------------------------------------------------------------------------------------------------
//----------------
// PIN OUT
//----------------
const int motorPins[4] = {8, 9, 10, 4};    // Declare pins to drive motor control board
const int oneWireBus    = 6;               // DS18B20 DATA connected to Digital Pin 6
const int highPowerPin1 = 3;               // General purpose High Power pin 1 (jD-IOBoard V1.0)
const int highPowerPin2 = 2;               // General purpose High Power pin 2 (jD-IOBoard V1.0)
const int buttonPin     = A3;              // Pin assigned to read the keypad
const int DHTPin        = 16;              // DHT11 DATA connected to Digital Pin 16(analog pin A3)
//----------------
// Stepper Motor
//----------------
const int motorStepsPerAscomStep  = 8;      // Motor steps per Ascom movement Unit (default = 8)
const int defaultStepDelay        = 10000;  // Default motor step delay (uS)(failsafe operation)(default = 16000)
const int lowSpeedStepDelay       = 10000;  // Motor step delay for Lo speed (uS)(default = 16000)
const int highSpeedStepDelay      = 2000;   // Motor step delay for Hi speed (uS)(default = 2000)
const int speedThreshold          = 50;     // motor speed Hi if steps to go are higher than this(default = 200)
const int defaultStartPosition    = 5000;   // Default Start Position if not set by Innnn# command(default = 5000)

//----------------
// Temperature
//----------------
const int tempResolution  = 10;              // 1-wire temperature sensor resolution 9=9bit(0.50C), 10=10bit(0.25C), 11=11bit(0.125C), 12=12bit(0.0625C)
//BME280
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
float temp(NAN), hum(NAN), pres(NAN), dewPoint(NAN);
//---------------------------
// EEPROM storage parameters
//---------------------------
const int EE_LOC_POS              = 0;       // Location of position (2 bytes)
const int EE_LOC_PSTAT            = 2;       // Location of Position Status (1 Byte)
const int POS_VALID               = 55;      // Stored position valid if this value otherwise invalid                               // Stored position valid if this value otherwise invalid

//----------------------------------------------------------------------------------------------------------------------

//---------------- OLED -----------------
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);
char str[10];
bool flag = 0;
//---------------------------------------
//--------------- BUTTONS ---------------
const int numButtons              = 5;
const int button[5]               = { 1, 2, 3, 4, 5 };
const int buttonThresholdLow[5]   = { 875, 814, 763, 677, 506 };
const int buttonThresholdHigh[5]  = { 887, 824, 773, 687, 516 };
int focusStepSize = 1;
//---------------------------------------

//----------------------------------------------------------------------------------------------------------------------
// Variables
//----------------------------------------------------------------------------------------------------------------------

const String programName = "Arduino Focuser";
const String programVersion = "1.0.6";



int           motorSpeed = defaultStepDelay;              // current delay for motor step speed (uS)
DeviceAddress tempSensor;                                 // Temperature sensor
double        currentTemperature;                         // current temperature
boolean       tempSensorPresent = false;                  // Is there a temperature sensor installed?
int           step = 0;                                   // current motor step position
boolean       outputActive = true;                        // Is motor energised? Initialised to true so it gets cleared on startup

// Default initial positions if not set using the Innnn# command by Ascom Driver
unsigned int  currentPosition = defaultStartPosition;     // current position
unsigned int  targetPosition  = defaultStartPosition;     // target position
unsigned int  lastSavedPosition;                          // last position saved to EEPROM

// Initialise the temp sensor
OneWire oneWire(oneWireBus);                              // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);                      // Pass our oneWire reference to Dallas Temperature.

// Lookup table to drive motor control board
const int stepPattern[8] = {B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001};

// For ASCOM connection
String inputString = "";                                   // string to hold incoming data
boolean stringComplete = false;                            // whether the string is complete
boolean isMoving = false;                                  // is the motor currently moving

//=============================================================================================================
// Setup
//=============================================================================================================

void setup() {
  u8g2.begin();
  u8g2.setFlipMode(0);
  Wire.begin();
  //-----------------------------------------------------------------------------------------------------------
  // Setup IO Pins
  //-----------------------------------------------------------------------------------------------------------

  // Motor Pins
  for (int i = 0; i < 4; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], 0);                       // Ensure all motor coils de-energised
  }
  outputActive = false;


  //-----------------------------------------------------------------------------------------------------------
  // Check SetUp switches And initialization
  //-----------------------------------------------------------------------------------------------------------
  inputString.reserve(200);                          // reserve 200 bytes for the ASCOM driver inputString
  Serial.begin(9600);                              // Initialize USB serial for ASCOM interface
  //-----------------------------------------------------------------------------------------------------------
  // Position initialisation
  //-----------------------------------------------------------------------------------------------------------
  if (storedPositionValid()) {                       // Check if EEPROM position is valid
    currentPosition = restorePosition();             // Use position from EEPROM if it is valid
  } else {
    currentPosition = defaultStartPosition;        // If invalid use the default position
  }
  lastSavedPosition = currentPosition;
  targetPosition = currentPosition;
  //-----------------------------------------------------------------------------------------------------------
  // OneWire Libary initialisation
  //-----------------------------------------------------------------------------------------------------------
  oneWire.reset_search();                            // Reset search
  oneWire.search(tempSensor);                        // Search for temp sensor and assign address to tempSensor
  //-----------------------------------------------------------------------------------------------------------
  // DallasTemperature Library initialisation
  //-----------------------------------------------------------------------------------------------------------
  sensors.begin();                                   // Initialise 1-wire bus
  //-----------------------------------------------------------------------------------------------------------
  // Temperature sensor initialisation
  //-----------------------------------------------------------------------------------------------------------
  if (sensors.getDeviceCount() == 0) {
    tempSensorPresent = false;                       // temperature sensor not installed
  } else {
    tempSensorPresent = true;                               // temperature sensor installed - set it up and get initial value
    sensors.setResolution(tempSensor, tempResolution);     // Set the resolution
    sensors.requestTemperatures();                          // Get the Temperatures
    currentTemperature = getTemperature();                  // Save current temperature
  }
  //===========
  // BME 280
  //===========
  while(!bme.begin())
    {
    u8g2.firstPage();
      do {
        bootingBme280();
      } while ( u8g2.nextPage() );
    delay(1000);
    }

  /*switch(bme.chipModel())
    {
     case BME280::ChipModel_BME280:
       //Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       //Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       //Serial.println("Found UNKNOWN sensor! Error!");
       break;
    }*/

  //-----------------------------------------------------------------------------------------------------------
}
//=============================================================================================================
// Main Loop
//=============================================================================================================

void loop() {

  if (stringComplete) {
    doCommand(inputString);
  }
  doMovement();// If any motor movement required do it

  if (!isMoving) {
    flag = 0;
    u8g2.firstPage();
    do {
      drawDataFrames();
      draw();
    } while ( u8g2.nextPage() );
  } else {
    if (flag == 0) {
      flag = 1;
      u8g2.firstPage();
      do {
        drawDataFrames();
        draw();
      } while ( u8g2.nextPage() );
    }
  }
  if (!isMoving) {
    ProcessButtons();
  }

  if (!isMoving) {
    currentTemperature = getTemperature();
    getAmbientData();
  }
}
//=============================================================================================================


