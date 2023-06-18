
#include <SPI.h>
#include <RH_RF95.h>
#include <SD.h>
#include <Wire.h> 
#include <RTClib.h>
// #include <LiquidCrystal_I2C.h>
#include <hd44780.h>                       // main hd44780 header https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

#include "sensors/check.hpp"
#include "sensors/clock.hpp"
#include "sensors/testcounter.hpp"
#include "sensors/throttle.hpp"
#include "sensors/voltage.hpp"
#include "sensors/ADC_Current.hpp"
#include "sensors/sensorManager.hpp"

#define THROTTLE_PIN A14
#define MIN_THROTTLE 1
#define MAX_THROTTLE 4.1

// LiquidCrystal_I2C lcd(0x27,20,4); 
hd44780_I2Cexp lcd(0x27);

#define PWM_PIN 3
#define MOTOR_ENABLE 4

#define RFM95_CS  6
#define RFM95_INT 2
#define RFM95_RST 7

#define RF95_FREQ 434.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);
enum messageType : uint8_t {dataMessage, errorMessage};

#define SD_CS 14

const char filename[] = "log.txt";

File txtFile;

int currentReadings = 0;
long reading = 0;

// Sensor::Clock clock;
// Sensor::CounterSensor counter1;
// Sensor::CounterSensor counter2;
// Sensor::Throttle throttle(THROTTLE_PIN, &throttleCallback);
// Sensor::VoltageSensor batHigh(A0, 0.02713563);
// Sensor::VoltageSensor batLow(A1, 0.01525241);
Sensor::CurrentSensor current;
// Sensor::CPUMonitor* monitor;
int sensorCount = 1;

const int time_per_reading = 100;
const int readings = 10;

Sensor::SensorManager manager(sensorCount, time_per_reading * readings);

bool sdStatus = false;
bool loraStatus = false;
int lcdStatus = 1;
void errorCallback(char* message) {
    Serial.println(message);

    if (sdStatus && txtFile) {
        txtFile.println(message);
        txtFile.flush();
        Serial.println("logged to text file");
    }

    if (loraStatus) {
        size_t size = (strlen(message) + 1) * sizeof(char);
        uint8_t data[size + 1];
        data[0] = errorMessage;
        memcpy(&(data[1]), message, size);
        rf95.send(data, size + 1);
        rf95.waitPacketSent();
        Serial.println("lora transmitted");
    }

    if (lcdStatus == 0) {
        for (int i = 0; i < strlen(message); i += 20 * 4) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.write(&(message[i]), min(20 * 4, strlen(message) - i));
            delay(2000);
        }
    }
}

void setup() {
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);

    pinMode(PWM_PIN, OUTPUT);
    pinMode(MOTOR_ENABLE, OUTPUT);

    Serial.begin(115200);

    lcdStatus = lcd.begin(20, 4);
    CHECK(lcdStatus == 0, "LCD initialisation failed");
    lcd.backlight();
    lcd.lineWrap();
    
    // lcd.setCursor(0, 0);
    // lcd.write("aaa");

    // lcd.setCursor(0, 1);
    // lcd.write("bbb");

    // RAISE("whoops");
    
    // clock.setup();
    // clock.setReadRate(1000);
    // manager.addSensor(&clock);

    // counter1.setReadRate(1000);
    // manager.addSensor(&counter1);

    // counter2.setReadRate(2000);
    // manager.addSensor(&counter2);

    // throttle.setTickRate(10);
    // throttle.setReadRate(50);
    // manager.addSensor(&throttle);

    // batHigh.setReadRate(1000);
    // manager.addSensor(&batHigh);

    // batLow.setReadRate(1000);
    // manager.addSensor(&batLow);

    // monitor = manager.getMonitor();
    // manager.addSensor(monitor);

    current.setReadRate(1000);
    current.setup();
    manager.addSensor(&current);

    manager.setReadCallback(&readCallback);

    sdStatus = SD.begin(SD_CS);
    CHECK(sdStatus == true, "SD initialisation failed.");

    txtFile = SD.open(filename, O_READ | O_WRITE | O_CREAT);
    CHECK(txtFile, "Error opening log file.");

    delay(100);
    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);

    bool initStatus = rf95.init();
    CHECK(initStatus == true, "LoRa initialisation failed.");

    bool freqStatus = rf95.setFrequency(RF95_FREQ);
    CHECK(freqStatus == true, "LoRa frequency set failed.");

    loraStatus = initStatus && freqStatus;

    rf95.setTxPower(23, false);

    digitalWrite(MOTOR_ENABLE, LOW);

    HANDLE_ERRS(errorCallback);
}

void loop() {
    // Serial.println("B");
    manager.spin(5000);
    // Serial.println(manager.getLastRead(&counter1));
}

void readCallback(double* results) {
    size_t size = sensorCount * sizeof(double);
    uint8_t data[size + 1];
    data[0] = dataMessage;
    memcpy(&(data[1]), results, size);
    rf95.send(data, size + 1);
    rf95.waitPacketSent();

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

void throttleCallback(double voltage) {

    int pwm_amount = ((voltage - MIN_THROTTLE) / (MAX_THROTTLE - MIN_THROTTLE)) * 255;

    if (pwm_amount < 0) {
        pwm_amount = 0;
    }
    else if (pwm_amount > 255) {
        pwm_amount = 255;
    }
    
    Serial.println(pwm_amount);

    analogWrite(PWM_PIN, pwm_amount);
    
    // double results[sensorCount];

    // results[0] = voltage;
    // for (int i = 1; i < sensorCount; i++) {
    //     results[i] = manager.getLastRead(i);
    // }

    // rf95.send((uint8_t*)results, sensorCount * sizeof(double));
    // rf95.waitPacketSent();

    // for (int i = 0; i < sensorCount; i++) {
    //     if (i > 0) {
    //         Serial.print(",");
    //         txtFile.print(",");
    //     }
    //     Serial.print(results[i]);
    //     txtFile.print(results[i]);
    // }
    // Serial.println();
    // txtFile.println();
    // txtFile.flush();
}

