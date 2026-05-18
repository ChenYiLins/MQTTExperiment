#pragma once

#include <Arduino.h>
#include <stdint.h>

struct MqttConfig
{
  String caCert;
  String server;
  uint16_t port = 8883;
  String clientId;
  String user;
  String password;
  String publishTopic;
  String subscribeTopic;
  String reportTopic;
  unsigned long version = 0;
};

class MqttConfigStore
{
public:
  void begin();
  MqttConfig load();
  bool refreshFromServer(MqttConfig &config);
  void reportDeviceStatus(const MqttConfig &config, bool mqttConnected, const String &message);

private:
  MqttConfig defaultConfig() const;
  bool loadFromNvs(MqttConfig &config);
  void saveToNvs(const MqttConfig &config);
  bool parsePayload(const String &payload, MqttConfig &config);
};
