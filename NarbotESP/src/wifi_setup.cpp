#include "wifi_setup.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

namespace {

#ifdef FRONT_ESP
unsigned long lastReconnectAttempt = 0;
constexpr unsigned long reconnectIntervalMs = 5000;

void startFrontWifiConnection() {
  Serial.print("Connecting to Wi-Fi AP: ");
  Serial.println(WIFI_SSID);
  WiFi.disconnect(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
#endif

}  // namespace

bool setupWifi() {
#ifdef MAIN_ESP
  WiFi.mode(WIFI_AP);

  if (!WiFi.softAPConfig(MAIN_AP_IP, MAIN_AP_IP, WIFI_SUBNET)) {
    Serial.println("Failed to configure AP IP");
    return false;
  }

  if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("Failed to start Wi-Fi AP");
    return false;
  }

  Serial.println("Wi-Fi AP started");
  Serial.print("AP SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  return true;
#elif defined(FRONT_ESP)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (!WiFi.config(FRONT_STATIC_IP, WIFI_GATEWAY, WIFI_SUBNET)) {
    Serial.println("Failed to configure static station IP; continuing with DHCP");
  }

  startFrontWifiConnection();

  constexpr unsigned long connectTimeoutMs = 20000;
  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < connectTimeoutMs) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi connection failed; will keep retrying");
    lastReconnectAttempt = millis();
    return false;
  }

  Serial.println("Wi-Fi connected");
  Serial.print("Assigned IP address: ");
  Serial.println(WiFi.localIP());
  return true;
#else
#error "Define MAIN_ESP or FRONT_ESP in platformio.ini"
#endif
}

void handleWifi() {
#ifdef FRONT_ESP
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastReconnectAttempt < reconnectIntervalMs) {
    return;
  }

  lastReconnectAttempt = millis();
  Serial.println("Wi-Fi disconnected; retrying");
  startFrontWifiConnection();
#endif
}
