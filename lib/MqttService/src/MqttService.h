#pragma once

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

class MqttService
{
public:
  MqttService();

  void begin();
  void connect();
  void ensureConnected();
  void loop();

  void publishInitialMessage();
  void reportTemperature(float temperature);
  void reportHumidity(float humidity);

private:
  void reportProperty(const char *propertyName, float value);

  WiFiClientSecure tlsClient;
  PubSubClient mqttClient;
};
