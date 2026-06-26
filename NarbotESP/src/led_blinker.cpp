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
bool blinkingEnabled = true;

void writeLed(bool on) {
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
  ledOn = on;
}

}  // namespace

void setupLedBlinker() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  writeLed(false);
  Serial.print("LED blinker started on pin ");
  Serial.println(STATUS_LED_PIN);
}

void handleLedBlinker() {
  if (!blinkingEnabled) {
    return;
  }

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

void setLedSolid(bool on) {
  blinkingEnabled = false;
  writeLed(on);
}

void setLedBlinking() {
  blinkingEnabled = true;
  lastBlinkAt = millis();
}
