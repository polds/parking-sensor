#ifndef LIGHTSTRIP_ANIMATION_H
#define LIGHTSTRIP_ANIMATION_H

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Adafruit_NeoPixel.h>
void animateLightstripOn(Adafruit_NeoPixel &strip, uint32_t color = 0x0000FF, unsigned int width = 2, unsigned int delayMs = 75);
// New: Animate parking bar based on distance
void animateParkingBar(Adafruit_NeoPixel &strip, float distance, float maxDistance, float stopDistance, float orangeDistance, float redDistance, bool &flashState);
uint32_t getParkingBarColor(Adafruit_NeoPixel &strip, float distance, float orangeDistance, float redDistance);
#else
// Forward declaration for test/mocks
class Adafruit_NeoPixel;
typedef unsigned int uint32_t;
void animateLightstripOn(Adafruit_NeoPixel &strip, uint32_t color, unsigned int width, unsigned int delayMs);
void animateParkingBar(Adafruit_NeoPixel &strip, float distance, float maxDistance, float stopDistance, float orangeDistance, float redDistance, bool &flashState);
uint32_t getParkingBarColor(Adafruit_NeoPixel &strip, float distance, float orangeDistance, float redDistance);
#endif

#endif // LIGHTSTRIP_ANIMATION_H 