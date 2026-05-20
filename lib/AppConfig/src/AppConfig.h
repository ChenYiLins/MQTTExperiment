#pragma once

#include <stdint.h>

namespace AppConfig
{
constexpr int oneWireBusPin = 32;
constexpr int switchPin = 3;

constexpr uint8_t displayAddress = 0x3c;
constexpr int displaySdaPin = 21;
constexpr int displaySclPin = 22;

constexpr unsigned long uploadIntervalMs = 5000;
constexpr unsigned long switchDebounceDelayMs = 5;
constexpr unsigned long configRefreshIntervalMs = 10000;
constexpr unsigned long configHttpTimeoutMs = 3000;

constexpr float fallbackTemperature = -99.0f;
constexpr float fallbackHumidity = -1.0f;

constexpr const char *serviceId = "MQTT_experimental_service";

// Change the host to your computer's LAN IP before uploading this firmware.
constexpr const char *mqttConfigEndpoint = "http://172.20.10.13:5000/api/device-config";
constexpr const char *mqttConfigStatusEndpoint = "http://172.20.10.13:5000/api/device-status";
}
