#pragma once

#include <Arduino.h>

struct DistanceSensorState {
  bool ready;
  bool valid;
  uint16_t distanceMm;
  uint8_t rangeStatus;
  unsigned long lastReadAt;
};

void setupDistanceSensor();
void handleDistanceSensor();
DistanceSensorState getDistanceSensorState();
