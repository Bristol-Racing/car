/****************************************************************************************************************************************************
*                                                                                                                                                   *                                                                                                         *
*   Car main script
*                                                                                                                                                   *
*   
*
****************************************************************************************************************************************************/

#include <SD.h>
#include <Wire.h> 
#include <RTClib.h>// Real time clock libary
#include "sensors/check.hpp"// custom error handling
#include "sensors/clock.hpp"
#include "sensors/voltage.hpp"
#include "sensors/ADC_Current.hpp"
#include "sensors/temperature.hpp"
#include "sensors/pushButton.hpp"
#include "sensors/sensorManager.hpp"

#define SD_CS 14  //  SD card reader pin
const char filename[] = "log.csv";//  SD card filename
File txtFile;     //  File written to the SD card

//  Initialise the sensors with their calibration values
int sensorCount = 6;
const int time_per_tick = 100;      //  tick rate milli
const int time_per_callback = 1000; // callback rates milli
Sensor::SensorManager manager(sensorCount, time_per_callback);//  Initialise sensor manager with the number of sensors and callback rate
Sensor::Clock clock;
Sensor::PushButton pitBut(19);
Sensor::VoltageSensor batVolt(A12, 5.6440677966101);
Sensor::CurrentSensor current(A11, A14, 1, 1);
Sensor::TemperatureSensor motTemp(A8);
Sensor::TemperatureSensor pcbTemp(A3);

bool sdStatus = false; //  Status of the SD card reader and radio

//  Called for any error messages that occur
void errorCallback(char* message) {
    Serial.println(message);//  Print the message to serial
    //  Print the message to the text file if the SD card reader is ok
    if (sdStatus && txtFile) {
        txtFile.println(message);
        txtFile.flush();
        Serial.println("logged to text file");
    }
}

void setup() {
    Serial.begin(115200);
    clock.setup();                  //  Set the clock up and add it to the sensor manager
    clock.setReportRate(1000);
    manager.addSensor(&clock);

    pitBut.setReportRate(1000);     //  Add the pit confirm push button to the manager
    manager.addSensor(&pitBut);

    batVolt.setReportRate(1000);    //  Add the high (24ish V) battery voltage sensor to the manager
    manager.addSensor(&batVolt);

    current.setReportRate(1000);    //  Add the current sensor to the manager
    current.setup();
    manager.addSensor(&current);

    motTemp.setReportRate(1000);    //  Add the motor temperature sensor to the manager
    manager.addSensor(&motTemp);

    pcbTemp.setReportRate(1000);    //  Add the PCB temperature sensor to the manager
    manager.addSensor(&pcbTemp); 

    manager.setReportCallback(&reportCallback); //  Set the report callback
    manager.setSendLEDCommand(&sendLEDCommand); // Set the LED command callback

    //  Start the SD card reader and check it started correctly
    sdStatus = SD.begin(SD_CS);
    CHECK(sdStatus == true, "SD initialisation failed.");

    //  Open a text file and check it opened correctly
    txtFile = SD.open(filename, O_READ | O_WRITE | O_CREAT | O_APPEND);
    CHECK(txtFile, "Error opening log file.");

    // Add headers to text file
    if(txtFile) {
      txtFile.println("clock,pitBut,voltage,current,motTemp,pcbTemp");
    }
    delay(100);
    HANDLE_ERRS(errorCallback);//  Handle any errors that occured, by calling the error callback function

    // LED test initialisation
    sendLEDCommand(1,8);
    sendLEDCommand(2,8);
    sendLEDCommand(3,8);
}

void loop() {
  //  Spin the manager, so that it schedules and handles sensor readings
  manager.spin();
}

void reportCallback(double* results) {
    // Saves data to the sd card.
    //  Called by the sensor manager to pass readings back to the main program

    //  Print all the readings to serial and the SD card text file
    for (int i = 0; i < sensorCount; i++) {
        if (i > 0) {
            Serial.print(",");
            txtFile.print(",");
        }
        Serial.print(results[i]);
        txtFile.print(results[i]);
    }
    Serial.println();
    txtFile.println();
    txtFile.flush();
}

void sendLEDCommand(int ledNumber, int state) {
  Wire.beginTransmission(8); // Address of the Uno slave
  Wire.write(ledNumber);     // Send LED number
  Wire.write(state);         // Send state
  Wire.endTransmission();
}



