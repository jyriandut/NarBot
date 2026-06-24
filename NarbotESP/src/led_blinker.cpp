#include "led_blinker.h"

#include <Arduino.h>

namespace {

#ifdef LED_BUILTIN
constexpr int STATUS_LED_PIN = LED_BUILTIN;
#else
constexpr int STATUS_LED_PIN = 21;
#endif

unsigned long blinkIntervalMs = 1000;
unsigned long lastBlinkAt = 0;
bool ledOn = false;

void writeLed(bool on) {
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}

}  // namespace

void setupLedBlinker() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  writeLed(false);
  Serial.print("LED blinker started on pin ");
  Serial.println(STATUS_LED_PIN);
}

void handleLedBlinker() {
  if (millis() - lastBlinkAt < blinkIntervalMs) {
    return;
  }

  lastBlinkAt = millis();
  ledOn = !ledOn;
  writeLed(ledOn);
}

void setLedBlinkInterval(unsigned long intervalMs) {
  blinkIntervalMs = intervalMs;
}
