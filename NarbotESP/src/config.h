#pragma once

#include <Arduino.h>
#include <IPAddress.h>

constexpr uint32_t SERIAL_BAUD = 115200;

constexpr const char *WIFI_SSID = "SMARS_MAIN";
constexpr const char *WIFI_PASSWORD = "smars1234";

const IPAddress MAIN_AP_IP(192, 168, 4, 1);
const IPAddress FRONT_STATIC_IP(192, 168, 4, 10);
const IPAddress WIFI_GATEWAY(192, 168, 4, 1);
const IPAddress WIFI_SUBNET(255, 255, 255, 0);

constexpr uint16_t WEB_SERVER_PORT = 80;
