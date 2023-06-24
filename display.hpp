
#ifndef CAR_DISPLAY_H
#define CAR_DISPLAY_H

#include "sensors/check.hpp"
#include <hd44780.h>                       // main hd44780 header https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

class Display {
private:
    hd44780_I2Cexp lcd;
    int lcdStatus;
public:
    Display(int id): lcd(id) {};
    ~Display();

    void setup();

    int status();

    void printError(char* message);

    void showLimiter(double lim);
    void showThrottle(double thr);
    void showVoltageL(double v);
    void showVoltageH(double v);
    void showCurrent(double c);
    void showCharge(double c);
    void showTemperature(double t);
    void showClock(double c);
};

Display::~Display() {

}

void Display::setup() {
    lcdStatus = lcd.begin(20, 4);
    CHECK(lcdStatus == 0, "LCD initialisation failed");
    lcd.backlight();
    lcd.lineWrap();
}

int Display::status() {
    return lcdStatus;
}

void Display::printError(char* message) {
    for (int i = 0; i < strlen(message); i += 20 * 4) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.write(&(message[i]), min(20 * 4, strlen(message) - i));
        delay(2000);
    }
}

void Display::showLimiter(double lim) {
    int limPercent = lim * 100;
    char out[8];
    sprintf(out, "lim:%4d", limPercent);
    lcd.setCursor(1, 0);
    lcd.write(out, 8);
}

void Display::showThrottle(double thr) {
    int thrPercent = thr * 100;
    char out[8];
    sprintf(out, "thr:%4d", thrPercent);
    lcd.setCursor(1, 1);
    lcd.write(out, 8);
}

void Display::showVoltageL(double v) {
    char out[8];
    char num[4];
    dtostrf(v, 4, 1, num);
    sprintf(out, "VL:%sV", num);
    lcd.setCursor(11, 1);
    lcd.write(out, 8);
}

void Display::showVoltageH(double v) {
    char out[8];
    char num[4];
    dtostrf(v, 4, 1, num);
    sprintf(out, "VH:%sV", num);
    lcd.setCursor(11, 0);
    lcd.write(out, 8);
}

void Display::showCurrent(double c) {
    char out[8];
    char num[4];
    dtostrf(c, 4, 1, num);
    sprintf(out, "Cu:%sA", num);
    lcd.setCursor(11, 2);
    lcd.write(out, 8);
}

void Display::showCharge(double c) {
    char out[9];
    char num[4];
    dtostrf(c, 4, 1, num);
    sprintf(out, "Ch:%sAh", num);
    lcd.setCursor(11, 3);
    lcd.write(out, 9);
}

void Display::showTemperature(double t) {
    char out[9];
    char num[4];
    dtostrf(t, 4, 1, num);
    sprintf(out, "tmp:%sC", num);
    lcd.setCursor(1, 2);
    lcd.write(out, 9);
}

void Display::showClock(double c) {
    int seconds = c;
    int minutes = seconds / 60;
    seconds -= minutes * 60;

    char out[5];
    sprintf(out, "%02d:%02d", minutes, seconds);
    lcd.setCursor(2, 3);
    lcd.write(out, 5);
}

#endif