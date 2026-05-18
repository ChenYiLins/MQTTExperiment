#include "ClientCredentials.h"

#include "D:\\Projects\\Source\\ESP32\\Internal\\clientConstInfo.h"

namespace ClientCredentials
{
const char *wifiSsid()
{
  return SSID;
}

const char *wifiPassword()
{
  return PASSWORD;
}

const char *mqttCaCert()
{
  return MQTT_CA_CERT;
}

const char *mqttServer()
{
  return MQTT_SERVER;
}

uint16_t mqttPort()
{
  return MQTT_PORT;
}

const char *mqttClientId()
{
  return MQTT_CLIENT_ID;
}

const char *mqttUser()
{
  return MQTT_USER;
}

const char *mqttPassword()
{
  return MQTT_PASSWORD;
}

const char *mqttPublishTopic()
{
  return MQTT_PUB_TOPIC;
}

const char *mqttSubscribeTopic()
{
  return MQTT_SUB_TOPIC;
}

const char *mqttReportTopic()
{
  return MQTT_REPORT_TOPIC;
}
}
