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

constexpr float fallbackTemperature = -99.0f;
constexpr float demoHumidity = 60.0f;

constexpr const char *serviceId = "MQTT_experimental_service";
}
