#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define LED_COUNT 16

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.show();
}

void loop() { 
  strip.fill(strip.Color(255, 0, 0));
  strip.show();
  delay(1000);

  strip.fill(strip.Color(0, 255, 0));
  strip.show();
  delay(1000);

  strip.fill(strip.Color(0, 0, 255));
  strip.show();
  delay(1000);

  strip.clear();
  strip.show();
  delay(1000);
}
