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
        color = state ? strip.Color(255, 255, 0) : strip.Color(0, 0, 0); // Yellow for blink
        strip.setPixelColor(0, color);
        strip.show();
        delay(50);
        strip.setPixelColor(0, strip.Color(255,0,255));
        strip.show();
        break;
      case 2:
        color = state ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0); // Red for fault
        strip.setPixelColor(1, color);
        break;
      case 3:
        color = state ? strip.Color(0, 255, 0) : strip.Color(0, 0, 0); // Green for switch
        strip.setPixelColor(2, color);
        break;
    }
    strip.show(); // Update strip to match
  }
}
