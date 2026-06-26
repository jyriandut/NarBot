#pragma once

#include <Arduino.h>

struct ColorSensorState {
  bool ready;
  bool available;
  uint8_t id;
  uint16_t clear;
  uint16_t red;
  uint16_t green;
  uint16_t blue;
  uint8_t displayRed;
  uint8_t displayGreen;
  uint8_t displayBlue;
  bool targetDetected;
  unsigned long lastReadAt;
};

void setupColorSensor();
void handleColorSensor();
ColorSensorState getColorSensorState();
