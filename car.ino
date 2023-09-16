
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

#include "display.hpp"

#define THROTTLE_PIN A14
#define MIN_THROTTLE 1
#define MAX_THROTTLE 4.1

#define LIMITER_PIN A15
#define MIN_LIMITER 0.1
#define MAX_LIMITER 4.8

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

Display display(0x27);

int currentReadings = 0;
long reading = 0;

Sensor::Clock clock;
// Sensor::CounterSensor counter1;
// Sensor::CounterSensor counter2;
Sensor::Potentiometer limiter(LIMITER_PIN, true);
Sensor::Throttle throttle(THROTTLE_PIN, &throttleCallback);
Sensor::VoltageSensor batHigh(A0, 0.02713563);
Sensor::VoltageSensor batLow(A1, 0.01525241);
Sensor::CurrentSensor current(-0.34456141, 0.00685451);
Sensor::TemperatureSensor temperature(A11);
// Sensor::CPUMonitor* monitor;
//  AND CHARGE MONITOR
int sensorCount = 8;

const int time_per_reading = 100;
const int readings = 10;

Sensor::SensorManager manager(sensorCount, time_per_reading * readings);

Sensor::ChargeMonitor charge(&manager, &current, 36.0);

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

    if (display.status() == 0) {
        display.printError(message);
    }
}

void setup() {
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, HIGH);

    pinMode(PWM_PIN, OUTPUT);
    pinMode(MOTOR_ENABLE, OUTPUT);

    Serial.begin(115200);

    display.setup();
    
    // lcd.setCursor(0, 0);
    // lcd.write("aaa");

    // lcd.setCursor(0, 1);
    // lcd.write("bbb");

    // RAISE("whoops");
    
    clock.setup();
    clock.setReadRate(1000);
    manager.addSensor(&clock);

    // counter1.setReadRate(1000);
    // manager.addSensor(&counter1);

    // counter2.setReadRate(2000);
    // manager.addSensor(&counter2);

    limiter.setTickRate(10);
    limiter.setReadRate(50);
    manager.addSensor(&limiter);

    throttle.setTickRate(10);
    throttle.setReadRate(50);
    manager.addSensor(&throttle);

    batHigh.setReadRate(1000);
    manager.addSensor(&batHigh);

    batLow.setReadRate(1000);
    manager.addSensor(&batLow);

    temperature.setReadRate(1000);
    manager.addSensor(&temperature);

    // monitor = manager.getMonitor();
    // manager.addSensor(monitor);

    current.setReadRate(1000);
    current.setup();
    manager.addSensor(&current);

    charge.setReadRate(1000);
    manager.addSensor(&charge);

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
    double vl = manager.getLastRead(&batLow);
    double vh = manager.getLastRead(&batHigh) - vl;
    display.showVoltageL(vl);
    display.showVoltageH(vh);
    display.showCurrent(manager.getLastRead(&current));
    display.showCharge(manager.getLastRead(&charge));
    display.showTemperature(manager.getLastRead(&temperature));
    display.showClock(manager.getLastRead(&clock));

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
    double scaledVoltage = (voltage - MIN_THROTTLE) / (MAX_THROTTLE - MIN_THROTTLE);

    double limiterVoltage = manager.getLastRead(&limiter);
    double scaledLimiterVoltage = (limiterVoltage - MIN_LIMITER) / (MAX_LIMITER - MIN_LIMITER);

    if (scaledLimiterVoltage < 0) {
        scaledLimiterVoltage = 0;
    }
    else if (scaledLimiterVoltage > 1) {
        scaledLimiterVoltage = 1;
    }

    if (scaledVoltage < 0) {
        scaledVoltage = 0;
    }
    else if (scaledVoltage > scaledLimiterVoltage) {
        scaledVoltage = scaledLimiterVoltage;
    }

    display.showLimiter(scaledLimiterVoltage);
    display.showThrottle(scaledVoltage);

    int pwm_amount = scaledVoltage * 255;

    analogWrite(PWM_PIN, pwm_amount);
}

