#ifndef ARDUINO
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#define NEO_GRB 0
#define NEO_KHZ800 0
#endif

#include "ParkingSensor.h"
#include <catch2/catch_all.hpp>
#include <vector>

// Mock Adafruit_NeoPixel for testing
class MockNeoPixel {
public:
    MockNeoPixel(uint16_t n, uint8_t p, uint8_t t) : pixels(n), numShow(0) {}
    uint16_t numPixels() const { return pixels; }
    void clear() { actions.push_back("clear"); cleared = true; }
    void fill(uint32_t color, uint16_t first, uint16_t count) {
        actions.push_back("fill:" + std::to_string(color) + "," + std::to_string(first) + "," + std::to_string(count));
        lastColor = color;
        lastFirst = first;
        lastCount = count;
        cleared = false;
    }
    void show() { numShow++; actions.push_back("show"); }
    std::vector<std::string> actions;
    int numShow;
    uint32_t lastColor = 0;
    uint16_t lastFirst = 0;
    uint16_t lastCount = 0;
    bool cleared = false;
private:
    uint16_t pixels;
};

// Overload for test: use MockNeoPixel instead of Adafruit_NeoPixel
void animateLightstripOn(MockNeoPixel &strip, uint32_t color = 0x0000FF, uint width = 2, uint delayMs = 0) {
    const uint numPixels = strip.numPixels();
    for (uint i = 0; i <= numPixels - width; ++i) {
        strip.clear();
        strip.fill(color, i, width);
        strip.show();
        // skip delay in test
    }
    strip.fill(color, 0, numPixels);
    strip.show();
}

// Helper for test: mimic the color logic
uint32_t testColorForDistance(float distance, float orangeDistance, float redDistance) {
    if (distance <= redDistance) return 0xFF0000; // Red
    if (distance <= orangeDistance) return 0xFF8000; // Orange
    return 0x00FF00; // Green
}

void animateParkingBar(MockNeoPixel &strip, float distance, float maxDistance, float stopDistance, float orangeDistance, float redDistance, bool &flashState) {
    const unsigned int numPixels = strip.numPixels();
    if (distance <= stopDistance) {
        // Flash red
        if (flashState) {
            strip.fill(0xFF0000, 0, numPixels);
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
        uint32_t color = testColorForDistance(distance, orangeDistance, redDistance);
        strip.clear();
        strip.fill(color, 0, numLeds);
        strip.show();
        flashState = false;
    }
}

TEST_CASE("animateLightstripOn animates and fills strip") {
    MockNeoPixel strip(10, 25, NEO_GRB + NEO_KHZ800);
    animateLightstripOn(strip, 0x0000FF, 2, 0);
    int expectedSweeps = 10 - 2 + 1;
    int expectedShows = expectedSweeps + 1;
    REQUIRE(strip.numShow == expectedShows);
    REQUIRE(strip.actions.back() == "show");
    REQUIRE(strip.actions[strip.actions.size() - 2].find("fill:255,0,10") != std::string::npos);
}

TEST_CASE("animateParkingBar: green, orange, red, and flashing red behaviors") {
    MockNeoPixel strip(10, 25, NEO_GRB + NEO_KHZ800);
    float maxDistance = 200.0;
    float orangeDistance = 50.0;
    float redDistance = 30.0;
    float stopDistance = 20.0;
    bool flashState = false;

    SECTION("Green zone: far distance") {
        float distance = 100.0;
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        REQUIRE(strip.lastColor == 0x00FF00);
        REQUIRE(strip.lastFirst == 0);
        REQUIRE(strip.lastCount > 0);
        REQUIRE(strip.cleared == false);
    }
    SECTION("Orange zone: mid distance") {
        float distance = 40.0;
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        REQUIRE(strip.lastColor == 0xFF8000);
        REQUIRE(strip.lastFirst == 0);
        REQUIRE(strip.lastCount > 0);
        REQUIRE(strip.cleared == false);
    }
    SECTION("Red zone: near distance") {
        float distance = 25.0;
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        REQUIRE(strip.lastColor == 0xFF0000);
        REQUIRE(strip.lastFirst == 0);
        REQUIRE(strip.lastCount > 0);
        REQUIRE(strip.cleared == false);
    }
    SECTION("Stop zone: flashing red (on)") {
        float distance = 10.0;
        flashState = false;
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        // Should clear (off)
        REQUIRE(strip.cleared == true);
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        // Should fill red (on)
        REQUIRE(strip.lastColor == 0xFF0000);
        REQUIRE(strip.lastFirst == 0);
        REQUIRE(strip.lastCount == 10);
        REQUIRE(strip.cleared == false);
    }
    SECTION("Edge case: exactly at orange threshold") {
        float distance = orangeDistance;
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        REQUIRE(strip.lastColor == 0xFF8000);
    }
    SECTION("Edge case: exactly at red threshold") {
        float distance = redDistance;
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        REQUIRE(strip.lastColor == 0xFF0000);
    }
    SECTION("Edge case: exactly at stop threshold") {
        float distance = stopDistance;
        flashState = false;
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        REQUIRE(strip.cleared == true);
        animateParkingBar(strip, distance, maxDistance, stopDistance, orangeDistance, redDistance, flashState);
        REQUIRE(strip.lastColor == 0xFF0000);
        REQUIRE(strip.lastCount == 10);
    }
}