#pragma once

#include <Arduino.h>

struct ObstacleSafetyState {
  bool enabled;
  bool frontReachable;
  bool distanceValid;
  bool blocked;
  uint16_t distanceMm;
  unsigned long lastCheckAt;
};

void setupObstacleSafety();
void handleObstacleSafety();
bool isForwardBlocked();
void updateFrontDistanceForSafety(bool valid, uint16_t distanceMm);
ObstacleSafetyState getObstacleSafetyState();
