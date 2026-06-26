#include "color_sensor.h"

#include <Arduino.h>
#include <Wire.h>

#include "motor_driver.h"

namespace {

#ifdef MAIN_ESP
constexpr int COLOR_SDA_PIN = D0;  // GPIO1
constexpr int COLOR_SCL_PIN = D1;  // GPIO2
#endif

constexpr uint8_t COLOR_SENSOR_ADDR = 0x29;
constexpr unsigned long COLOR_READ_INTERVAL_MS = 250;

constexpr uint8_t TCS_COMMAND_BIT = 0x80;
constexpr uint8_t TCS_ENABLE = 0x00;
constexpr uint8_t TCS_ATIME = 0x01;
constexpr uint8_t TCS_CONTROL = 0x0F;
constexpr uint8_t TCS_ID = 0x12;
constexpr uint8_t TCS_STATUS = 0x13;
constexpr uint8_t TCS_CDATAL = 0x14;
constexpr uint8_t TCS_RDATAL = 0x16;
constexpr uint8_t TCS_GDATAL = 0x18;
constexpr uint8_t TCS_BDATAL = 0x1A;
constexpr uint8_t TCS_ENABLE_PON = 0x01;
constexpr uint8_t TCS_ENABLE_AEN = 0x02;
constexpr uint8_t TCS_STATUS_AVALID = 0x01;
constexpr uint8_t TCS_INTEGRATIONTIME_154MS = 0xC0;
constexpr uint8_t TCS_GAIN_4X = 0x01;
constexpr uint8_t TARGET_RED = 74;
constexpr uint8_t TARGET_GREEN = 70;
constexpr uint8_t TARGET_BLUE = 122;
constexpr uint8_t TARGET_TOLERANCE = 5;

ColorSensorState state = {};
unsigned long lastAttemptAt = 0;

bool writeColorRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(COLOR_SENSOR_ADDR);
  Wire.write(TCS_COMMAND_BIT | reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readColorRegister(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(COLOR_SENSOR_ADDR);
  Wire.write(TCS_COMMAND_BIT | reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(COLOR_SENSOR_ADDR, (uint8_t)1) != 1) {
    return false;
  }

  value = Wire.read();
  return true;
}

bool readColorRegister16(uint8_t reg, uint16_t &value) {
  uint8_t low = 0;
  uint8_t high = 0;

  if (!readColorRegister(reg, low) || !readColorRegister(reg + 1, high)) {
    return false;
  }

  value = ((uint16_t)high << 8) | low;
  return true;
}

void normalizeColorReading() {
  if (state.clear == 0) {
    state.displayRed = 0;
    state.displayGreen = 0;
    state.displayBlue = 0;
    return;
  }

  uint32_t red = (uint32_t)state.red * 256 / state.clear;
  uint32_t green = (uint32_t)state.green * 256 / state.clear;
  uint32_t blue = (uint32_t)state.blue * 256 / state.clear;

  state.displayRed = min<uint32_t>(red, 255);
  state.displayGreen = min<uint32_t>(green, 255);
  state.displayBlue = min<uint32_t>(blue, 255);
}

bool isWithinTarget(uint8_t value, uint8_t target) {
  int delta = abs((int)value - (int)target);
  return delta <= TARGET_TOLERANCE;
}

void updateTargetDetection() {
  state.targetDetected = state.available &&
                         isWithinTarget(state.displayRed, TARGET_RED) &&
                         isWithinTarget(state.displayGreen, TARGET_GREEN) &&
                         isWithinTarget(state.displayBlue, TARGET_BLUE);
}

bool isDriving() {
  MotorDriverState motor = getMotorDriverState();
  return motor.leftDirection != MotorDirection::Stopped || motor.rightDirection != MotorDirection::Stopped;
}

bool readColorSensor() {
  uint8_t status = 0;
  if (!readColorRegister(TCS_STATUS, status) || (status & TCS_STATUS_AVALID) == 0) {
    state.available = false;
    state.targetDetected = false;
    return false;
  }

  bool ok = readColorRegister16(TCS_CDATAL, state.clear);
  ok = ok && readColorRegister16(TCS_RDATAL, state.red);
  ok = ok && readColorRegister16(TCS_GDATAL, state.green);
  ok = ok && readColorRegister16(TCS_BDATAL, state.blue);

  state.available = ok;
  state.lastReadAt = millis();
  if (ok) {
    normalizeColorReading();
  }
  updateTargetDetection();

  return ok;
}

}  // namespace

void setupColorSensor() {
#ifdef MAIN_ESP
  Wire.begin(COLOR_SDA_PIN, COLOR_SCL_PIN);
  Wire.setTimeOut(20);

  Serial.printf("Starting color sensor on SDA D0/GPIO%d, SCL D1/GPIO%d\n", COLOR_SDA_PIN, COLOR_SCL_PIN);
  if (!readColorRegister(TCS_ID, state.id)) {
    state.ready = false;
    Serial.println("Color sensor not found at I2C address 0x29");
    return;
  }

  state.ready = writeColorRegister(TCS_ENABLE, TCS_ENABLE_PON);
  delay(3);
  state.ready = state.ready && writeColorRegister(TCS_ATIME, TCS_INTEGRATIONTIME_154MS);
  state.ready = state.ready && writeColorRegister(TCS_CONTROL, TCS_GAIN_4X);
  state.ready = state.ready && writeColorRegister(TCS_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN);

  if (!state.ready) {
    Serial.println("Color sensor init failed");
    return;
  }

  Serial.print("Color sensor ready, ID 0x");
  Serial.println(state.id, HEX);
#endif
}

void handleColorSensor() {
#ifdef MAIN_ESP
  if (!state.ready) {
    return;
  }

  unsigned long now = millis();
  if (now - lastAttemptAt < COLOR_READ_INTERVAL_MS) {
    return;
  }

  lastAttemptAt = now;
  if (readColorSensor() && state.targetDetected && isDriving()) {
    Serial.printf("Color stop: RGB %u,%u,%u matched target %u,%u,%u +/- %u\n",
                  state.displayRed,
                  state.displayGreen,
                  state.displayBlue,
                  TARGET_RED,
                  TARGET_GREEN,
                  TARGET_BLUE,
                  TARGET_TOLERANCE);
    stopMotors();
  }
#endif
}

ColorSensorState getColorSensorState() {
  return state;
}
