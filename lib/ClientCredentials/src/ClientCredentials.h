#pragma once

#include <stdint.h>

namespace ClientCredentials
{
const char *wifiSsid();
const char *wifiPassword();

const char *mqttCaCert();
const char *mqttServer();
uint16_t mqttPort();
const char *mqttClientId();
const char *mqttUser();
const char *mqttPassword();
const char *mqttPublishTopic();
const char *mqttSubscribeTopic();
const char *mqttReportTopic();
}
