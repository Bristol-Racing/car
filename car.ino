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

// for LoRa module
#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 22
#define RFM95_RST 0
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

int16_t packetnum = 0;


bool sdStatus = false; //  Status of the SD card reader and radio

//  Called for any error messages that occur
void errorCallback(char* message) {
    Serial.println(message);//  Print the message to serial
//    //  Print the message to the text file if the SD card reader is ok
    if (sdStatus && txtFile) {
        txtFile.println(message);
        txtFile.flush();
        Serial.println("logged to text file");
    }
}

void lora_setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.println("Arduino LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

void lora_send(String msg) {
  int array_length = msg.length() + 1;
  char radiopacket[array_length];
  msg.toCharArray(radiopacket, array_length);
  
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server
  
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[array_length - 1] = 0;
  
  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, array_length);

  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply..."); delay(10);
  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
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

    // LoRa setup
    lora_setup();
}

void loop() {
  //  Spin the manager, so that it schedules and handles sensor readings
  manager.spin();
}

void reportCallback(double* results) {
    // Saves data to the sd card.
    //  Called by the sensor manager to pass readings back to the main program

    //  Print all the readings to serial and the SD card text file
    String msg = "";
    
    for (int i = 0; i < sensorCount; i++) {
        if (i > 0) {
            Serial.print(",");
            txtFile.print(",");
            msg += ",";
        }
        Serial.print(results[i]);
        txtFile.print(results[i]);
        msg += String(results[i]);
    }
    Serial.println();
    txtFile.println();
    txtFile.flush();
    
    lora_send(msg);
}

void sendLEDCommand(int ledNumber, int state) {
  Wire.beginTransmission(8); // Address of the Uno slave
  Wire.write(ledNumber);     // Send LED number
  Wire.write(state);         // Send state
  Wire.endTransmission();
}
