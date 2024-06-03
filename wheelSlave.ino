#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#define PIN            6     // Pin where NeoPixel is connected
#define NUMPIXELS      3     // Number of NeoPixels in the strip

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Wire.begin(8); // Join I2C bus with address #8
  Wire.onReceive(receiveEvent); // Register event handler
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Main loop does nothing, all action is in the receiveEvent function
}

void receiveEvent(int howMany) {
  while (Wire.available() >= 2) {
    int ledNumber = Wire.read(); // Receive LED number
    int state = Wire.read();     // Receive state

    uint32_t color;
    switch (ledNumber) {
      case 1:
        switch (state) {
          case 0: // Off state
            color = strip.Color(0,0,0);
            break;
          case 1: // SD logging state
            color = strip.Color(0,0,255);
            break;
          case 2: // Fault injection state
            color = strip.Color(255,0,0);
            break;
          case 8: // Startup state
            color = strip.Color(128,0,128);
            break;
        }
        strip.setPixelColor(0, color);
        strip.show();
        break;
      case 2:
        switch (state) {
          case 0: // Off state
            color = strip.Color(0,0,0);
            break;
          case 1: // Good temp state
            color = strip.Color(0,255,0);
            break;
          case 2: // Medium temp state
            color = strip.Color(255,191,0);
            break;
          case 3: // Bad temp state
            color = strip.Color(255,0,0);
            break;
          case 8: // Startup state
            color = strip.Color(128,0,128);
            break;
        }
        strip.setPixelColor(1, color);
        strip.show();
        break;
      case 3:
        switch (state) {
          case 0: // Off state
            color = strip.Color(0,0,0);
            break;
          case 1: // Button on
            color = strip.Color(0,255,0);
            break;
          case 2: // Fault injection state
            color = strip.Color(255,0,0);
            break;
          case 8: // Startup state
            color = strip.Color(128,0,128);
            break;
        }
        strip.setPixelColor(2, color);
        strip.show();
        break;
    }
    strip.show(); // Update strip to match
  }
}
