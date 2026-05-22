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
constexpr unsigned long statusReportIntervalMs = 5000;
constexpr unsigned long configDiscoveryTimeoutMs = 1500;
constexpr unsigned long configDiscoveryCacheMs = 30000;
constexpr unsigned long configHttpTimeoutMs = 3000;

constexpr float fallbackTemperature = -99.0f;
constexpr float fallbackHumidity = -1.0f;

constexpr const char *serviceId = "MQTT_experimental_service";
constexpr uint16_t mqttConfigDiscoveryPort = 4210;
constexpr const char *mqttConfigDiscoveryRequest = "MQTT_CONFIG_DISCOVERY";
constexpr const char *mqttConfigDiscoveryResponsePrefix = "MQTT_CONFIG_SERVER ";
constexpr const char *mqttConfigPath = "/api/device-config";
constexpr const char *mqttConfigStatusPath = "/api/device-status";
}
