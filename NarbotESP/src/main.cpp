#include <Arduino.h>

#include "camera_setup.h"
#include "config.h"
#include "color_sensor.h"
#include "distance_sensor.h"
#include "led_blinker.h"
#include "motor_driver.h"
#include "obstacle_safety.h"
#include "web_server.h"
#include "wifi_setup.h"

namespace {

void logBootInfo() {
  Serial.println();
#ifdef MAIN_ESP
  Serial.println("Boot mode: MAIN_ESP");
#elif defined(FRONT_ESP)
  Serial.println("Boot mode: FRONT_ESP");
#else
#error "Define MAIN_ESP or FRONT_ESP in platformio.ini"
#endif
}

}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);

  unsigned long serialStart = millis();
  while (!Serial && millis() - serialStart < 3000) {
    delay(10);
  }

  logBootInfo();
  setupLedBlinker();
  setupMotorDriver();
  setupWifi();
  setupCamera();
  setupColorSensor();
  setupDistanceSensor();
  setupObstacleSafety();
  setupWebServer();
}

void loop() {
  handleWebServer();
  handleLedBlinker();
  handleMotorDriver();
  handleColorSensor();
  handleDistanceSensor();
  handleObstacleSafety();
  handleWifi();
}
