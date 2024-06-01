
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

//  The display isnt used
//#include "display.hpp"

//  Throttle and limiter arent used
// #define THROTTLE_PIN A14
// #define MIN_THROTTLE 1
// #define MAX_THROTTLE 4.1

// #define LIMITER_PIN A15
// #define MIN_LIMITER 0.1
// #define MAX_LIMITER 4.8

//  Motor control pins arent used
// #define PWM_PIN 3
// #define MOTOR_ENABLE 4

//  Radio pins
#define RFM95_CS  6
#define RFM95_INT 2
#define RFM95_RST 7

//  Radio frequency
#define RF95_FREQ 434.0

//  Setup radio
RH_RF95 rf95(RFM95_CS, RFM95_INT);

//  Message type for radio messages
enum messageType : uint8_t {dataMessage, errorMessage};

//  SD card reader pin
#define SD_CS 14

//  SD card filename
const char filename[] = "log.csv";

//  File written to the SD card
File txtFile;

//  Initialise the sensors with their calibration values
Sensor::Clock clock;
Sensor::PushButton pitBut(19);
Sensor::VoltageSensor batVolt(A12, 5.6440677966101);
Sensor::CurrentSensor current(A11, A14, 1, 1); // -0.34456141, 0.00685451
// Sensor::HallEffectSensor hallspeed(D18);
Sensor::TemperatureSensor motTemp(A8);
Sensor::TemperatureSensor pcbTemp(A3);

int sensorCount = 6;

//  tick and callback rates
const int time_per_tick = 100;
const int time_per_callback = 1000;

//  Initialise sensor manager with the number of sensors and callback rate
Sensor::SensorManager manager(sensorCount, time_per_callback);

//  Status of the SD card reader and radio
bool sdStatus = false;
bool loraStatus = false;

//  Called for any error messages that occur
void errorCallback(char* message) {
    //  Print the message to serial
    Serial.println(message);

    //  Print the message to the text file if the SD card reader is ok
    if (sdStatus && txtFile) {
        txtFile.println(message);
        txtFile.flush();
        Serial.println("logged to text file");
    }

    //  Send the message over radio if the radio is ok
    if (loraStatus) {
        //  Calculate the radio packet size
        size_t size = (strlen(message) + 1) * sizeof(char);
        //  Create an array for the packet
        uint8_t data[size + 1];

        //  First byte is the message type, an error message in this case
        data[0] = errorMessage;

        //  Copy the message into the rest of the packet
        memcpy(&(data[1]), message, size);

        //  Send the packet
        rf95.send(data, size + 1);
        rf95.waitPacketSent();

        Serial.println("lora transmitted");
    }
}

void setup() {
    //  I can't remember how the radio reset pin works, but it do
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);

    //  Start serial
    Serial.begin(115200);

    // Start I2C for LCD
    //Wire.begin();
    
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



    //  I can't remember how the radio reset pin works, but it do
    delay(100);
    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);

    //  Start the radio and check it started correctly
    bool initStatus = rf95.init();
    CHECK(initStatus == true, "LoRa initialisation failed.");

    //  Set the frequency and check it was set correctly
    bool freqStatus = rf95.setFrequency(RF95_FREQ);
    CHECK(freqStatus == true, "LoRa frequency set failed.");

    //  The radio works if both were ok
    loraStatus = initStatus && freqStatus;

    //  Set radio power
    rf95.setTxPower(23, false);

    //  Handle any errors that occured, by calling the error callback function
    HANDLE_ERRS(errorCallback);
}

void loop() {
  //  Spin the manager, so that it schedules and handles sensor readings
  manager.spin();
}

//  Called by the sensor manager to pass readings back to the main program
void reportCallback(double* results) {
    //  Calculate the radio packet size
    size_t size = sensorCount * sizeof(double);
    //  Create an array for the packet
    uint8_t data[size + 1];

    //  First byte is the message type, a data message in this case
    data[0] = dataMessage;

    //  Copy the message into the rest of the packet
    memcpy(&(data[1]), results, size);

    //  Send the packet
    rf95.send(data, size + 1);
    rf95.waitPacketSent();

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





