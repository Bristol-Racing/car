// Car main script
// 

#include <SPI.h>
#include <RH_RF95.h>
#include <SD.h>
#include <Wire.h> 
#include <RTClib.h>
#include "sensors/check.hpp"
#include "sensors/clock.hpp"
#include "sensors/testcounter.hpp"
#include "sensors/throttle.hpp"
#include "sensors/potentiometer.hpp"
#include "sensors/voltage.hpp"
#include "sensors/ADC_Current.hpp"
#include "sensors/charge.hpp"
#include "sensors/temperature.hpp"
#include "sensors/sensorManager.hpp"
#include "sensors/pushButton.hpp"

#define SD_CS 14  //  SD card reader pin
const char filename[] = "log.csv";//  SD card filename
File txtFile;//  File written to the SD card

//  Initialise the sensors with their calibration values
int sensorCount = 6;
Sensor::Clock clock;
Sensor::PushButton pitBut(19);
Sensor::VoltageSensor batVolt(A12, 5.6440677966101);
Sensor::CurrentSensor current(A11, A14, 1, 1); // -0.34456141, 0.00685451
Sensor::TemperatureSensor motTemp(A8);
Sensor::TemperatureSensor pcbTemp(A3);

//  tick and callback rates
const int time_per_tick = 100;
const int time_per_callback = 1000;

//  Initialise sensor manager with the number of sensors and callback rate
Sensor::SensorManager manager(sensorCount, time_per_callback);

//  Status of the SD card reader and radio
bool sdStatus = false;

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
    
    //  Set the clock up and add it to the sensor manager
    clock.setup();
    clock.setReportRate(1000);
    manager.addSensor(&clock);

    //  Add the pit confirm push button to the manager
    pitBut.setReportRate(1000);
    manager.addSensor(&pitBut);

    //  Add the high (24ish V) battery voltage sensor to the manager
    batVolt.setReportRate(1000);
    manager.addSensor(&batVolt);

    //  Add the current sensor to the manager
    current.setReportRate(1000);
    current.setup();
    manager.addSensor(&current);

    //  Add the motor temperature sensor to the manager
    motTemp.setReportRate(1000);
    manager.addSensor(&motTemp);

    //  Add the PCB temperature sensor to the manager
    pcbTemp.setReportRate(1000);
    manager.addSensor(&pcbTemp);

    //  Set the report callback
    manager.setReportCallback(&reportCallback);

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