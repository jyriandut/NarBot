#include "obstacle_safety.h"

#include <Arduino.h>

#include "motor_driver.h"

namespace {

constexpr uint16_t STOP_DISTANCE_MM = 120;
constexpr unsigned long DISTANCE_STALE_MS = 750;

ObstacleSafetyState state = {};

bool isDrivingForward() {
  MotorDriverState motor = getMotorDriverState();
  return motor.leftDirection == MotorDirection::Forward && motor.rightDirection == MotorDirection::Forward;
}

bool hasFreshBlockedDistance() {
  return state.enabled &&
         state.frontReachable &&
         state.distanceValid &&
         millis() - state.lastCheckAt <= DISTANCE_STALE_MS &&
         state.distanceMm <= STOP_DISTANCE_MM;
}

}  // namespace

void setupObstacleSafety() {
#ifdef MAIN_ESP
  state.enabled = true;
  Serial.printf("Obstacle safety enabled from cached distance: stop forward drive at <= %u mm\n", STOP_DISTANCE_MM);
#endif
}

void handleObstacleSafety() {
#ifdef MAIN_ESP
  state.blocked = hasFreshBlockedDistance();
  if (state.blocked && isDrivingForward()) {
    Serial.printf("Obstacle stop: %u mm\n", state.distanceMm);
    stopMotors();
  }
#endif
}

bool isForwardBlocked() {
#ifdef MAIN_ESP
  return hasFreshBlockedDistance();
#else
  return false;
#endif
}

void updateFrontDistanceForSafety(bool valid, uint16_t distanceMm) {
#ifdef MAIN_ESP
  state.frontReachable = true;
  state.distanceValid = valid;
  state.distanceMm = distanceMm;
  state.lastCheckAt = millis();
  state.blocked = hasFreshBlockedDistance();
#endif
}

ObstacleSafetyState getObstacleSafetyState() {
  return state;
}
