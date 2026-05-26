#pragma once

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include <MqttConfig.h>

class MqttService
{
public:
  MqttService();

  void begin(const MqttConfig &initialConfig);
  void applyConfig(const MqttConfig &newConfig);
  bool connect(uint8_t maxAttempts = 0);
  void ensureConnected();
  bool isConnected();
  void loop();

  void publishInitialMessage();
  void reportTemperature(float temperature);
  void reportHumidity(float humidity);
  void reportCity(String city);
  void reportIpAddress(String ipAddress);

private:
  void reportIntProperty(const char *propertyName, float value);
  void reportStringProperty(const char *propertyName, String value);

  MqttConfig config;
  WiFiClientSecure tlsClient;
  PubSubClient mqttClient;
};
