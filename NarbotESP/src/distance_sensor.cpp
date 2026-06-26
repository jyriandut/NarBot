#include "distance_sensor.h"

#include <Arduino.h>

#ifdef FRONT_ESP
#include <Adafruit_VL53L0X.h>
#include <Wire.h>
#endif

namespace {

constexpr unsigned long DISTANCE_READ_INTERVAL_MS = 150;

DistanceSensorState state = {};
unsigned long lastAttemptAt = 0;

#ifdef FRONT_ESP
Adafruit_VL53L0X sensor;
constexpr int DISTANCE_SDA_PIN = D5;  // GPIO6, wired SDA
constexpr int DISTANCE_SCL_PIN = D4;  // GPIO5, wired SCL

void readDistance() {
  VL53L0X_RangingMeasurementData_t measurement = {};
  sensor.rangingTest(&measurement, false);

  state.rangeStatus = measurement.RangeStatus;
  state.valid = measurement.RangeStatus != 4;
  state.distanceMm = state.valid ? measurement.RangeMilliMeter : 0;
  state.lastReadAt = millis();
}
#endif

}  // namespace

void setupDistanceSensor() {
#ifdef FRONT_ESP
  Wire.begin(DISTANCE_SDA_PIN, DISTANCE_SCL_PIN);

  Serial.printf("Starting VL53L0X distance sensor on SDA D5/GPIO%d, SCL D4/GPIO%d\n", DISTANCE_SDA_PIN, DISTANCE_SCL_PIN);
  if (!sensor.begin(0x29, false, &Wire)) {
    Serial.println("VL53L0X init failed");
    state.ready = false;
    state.valid = false;
    return;
  }

  state.ready = true;
  Serial.println("VL53L0X init successful");
  readDistance();
#endif
}

void handleDistanceSensor() {
#ifdef FRONT_ESP
  if (!state.ready) {
    return;
  }

  unsigned long now = millis();
  if (now - lastAttemptAt < DISTANCE_READ_INTERVAL_MS) {
    return;
  }

  lastAttemptAt = now;
  readDistance();
#endif
}

DistanceSensorState getDistanceSensorState() {
  return state;
}
