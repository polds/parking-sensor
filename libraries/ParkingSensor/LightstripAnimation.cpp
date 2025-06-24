#ifdef ARDUINO
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LightstripAnimation.h"

void animateLightstripOn(Adafruit_NeoPixel &strip, uint32_t color, unsigned int width, unsigned int delayMs) {
    const unsigned int numPixels = strip.numPixels();
    for (unsigned int i = 0; i <= numPixels - width; ++i) {
        strip.clear();
        strip.fill(color, i, width);
        strip.show();
        delay(delayMs);
    }
    strip.fill(color, 0, numPixels);
    strip.show();
}

uint32_t getParkingBarColor(Adafruit_NeoPixel &strip, float distance, float orangeDistance, float redDistance) {
    if (distance <= redDistance) {
        return strip.Color(255, 0, 0); // Red
    } else if (distance <= orangeDistance) {
        return strip.Color(255, 128, 0); // Orange
    } else {
        return strip.Color(0, 255, 0); // Green
    }
}

void animateParkingBar(Adafruit_NeoPixel &strip, float distance, float maxDistance, float stopDistance, float orangeDistance, float redDistance, bool &flashState) {
    const unsigned int numPixels = strip.numPixels();
    if (distance <= stopDistance) {
        // Flash red
        if (flashState) {
            strip.fill(strip.Color(255, 0, 0), 0, numPixels);
        } else {
            strip.clear();
        }
        strip.show();
        flashState = !flashState;
    } else {
        int numLeds = 0;
        if (distance < maxDistance && distance > stopDistance) {
            numLeds = (int)(((maxDistance - distance) / (maxDistance - stopDistance)) * numPixels);
        }
        if (numLeds < 0) numLeds = 0;
        if (numLeds > (int)numPixels) numLeds = numPixels;
        uint32_t color = getParkingBarColor(strip, distance, orangeDistance, redDistance);
        strip.clear();
        strip.fill(color, 0, numLeds);
        strip.show();
        flashState = false;
    }
}
#endif 