#include <stdio.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ParkingSensor.h>
#include <Adafruit_NeoPixel.h>

#ifndef WIFI_SSID
#define WIFI_SSID "changeme"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "changeme"
#endif
#ifndef WIFI_HOSTNAME
#define WIFI_HOSTNAME "changeme"
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
const char* hostname = WIFI_HOSTNAME;

#define PIN_TRIG 18 // SonicSensor Trigger
#define PIN_ECHO 19 // SonicSensor Echo

#define PIN_SIGNAL 25 // LED Signal
#define NUMPIXELS 10 // LED Count
#define MINBRIGHT 1 // LED Minimum Brightness
#define MAXBRIGHT 100 // LED Maximum Brightness

#define MAX_DISTANCE 200.0  // cm
#define ORANGE_DISTANCE 50.0
#define RED_DISTANCE 30.0
#define STOP_DISTANCE 20.0

// create the light strip object
Adafruit_NeoPixel Lightstrip(NUMPIXELS, PIN_SIGNAL, NEO_GRB + NEO_KHZ800);

// State for inactivity and animation
float lastDistance = -1;
unsigned long lastChangeTime = 0;
const float DISTANCE_MARGIN = 5.0; // cm
const unsigned long INACTIVITY_TIMEOUT = 16000; // 16 seconds in ms
bool lightsOn = true;

// Handle is the "main" loop logic that is executed.
void handle() {
  float distance = getDistanceCM([&]() {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    return pulseIn(PIN_ECHO, HIGH, 60000UL); // 60ms = ~1029cm
  });
  if (distance < 0) {
    Serial.println("no echo (out of range)");
    return;
  }

  unsigned long now = millis();

  // Animate parking bar based on distance
  static bool flashState = false;
  animateParkingBar(Lightstrip, distance, MAX_DISTANCE, STOP_DISTANCE, ORANGE_DISTANCE, RED_DISTANCE, flashState);

  if (lastDistance < 0 || fabs(distance - lastDistance) > DISTANCE_MARGIN) {
    lastChangeTime = now;
    lastDistance = distance;
    if (!lightsOn) {
      animateLightstripOn(Lightstrip, Lightstrip.Color(0, 0, 255)); // Blue sweep
      lightsOn = true;
    }
  } else if (lightsOn && (now - lastChangeTime > INACTIVITY_TIMEOUT)) {
    Lightstrip.clear();
    Lightstrip.show();
    lightsOn = false;
  }

  Serial.printf("distance: %f cm\n", distance);
}

void initWiFi() {
  Serial.print("\nConnecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(250);
  }
  
  Serial.print(" WiFi connected!");
  Serial.print("\nESP32 IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP32 HostName: ");
  Serial.println(WiFi.getHostname());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
}

// initOTA initializes the Arduino OTA over WiFi process.
void initOTA() {
  Serial.print("\nInitializing Arduino OTA process...");

  ArduinoOTA.onStart([]() {
    String type;
    switch (ArduinoOTA.getCommand()) {
    case U_FLASH:
      type = "sketch";
    default:
      type = "filesystem";
      // Note: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    }
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    switch (error) {
    case OTA_AUTH_ERROR:
      Serial.println("Auth Failed");
      break;
    
    case OTA_BEGIN_ERROR:
      Serial.println("Begin Failed");
      break;

    case OTA_CONNECT_ERROR:
      Serial.println("Connect Failed");
      break;

    case OTA_RECEIVE_ERROR:
      Serial.println("Receive Failed");
      break;

    case OTA_END_ERROR:
      Serial.println("End Failed");
      break;

    default:
      Serial.println("Unknown / Unhandled error");
    }
  });

  ArduinoOTA.begin();
  Serial.print(" Arduino OTA over WiFi process initalized!\n");
}

void initSonicSensor() {
  Serial.println("\nInitializing Ultrasonic sensor...");
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  Serial.print(" Ultrasonic sensor loaded and configured!\n");
}

void setup() {
  Serial.begin(115200);
  
  initSonicSensor();
  Lightstrip.begin();
  initWiFi();
  initOTA();

  Serial.println("Parking Sensor successfully initialized!");
}

void loop() {
  ArduinoOTA.handle(); // Handle OTA updates
  handle();
  delay(200);
}