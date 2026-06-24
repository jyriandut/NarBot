#include <Arduino.h>

#include "camera_setup.h"
#include "config.h"
#include "led_blinker.h"
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
  setupWifi();
  setupCamera();
  setupWebServer();
}

void loop() {
  handleLedBlinker();
  handleWifi();
  handleWebServer();
}
