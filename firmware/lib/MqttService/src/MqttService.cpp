#include "MqttService.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include <AppConfig.h>

namespace
{
void onMessage(char *topic, byte *payload, unsigned int len)
{
  Serial.print("[Recive] topic=");
  Serial.print(topic);
  Serial.print("  msg=");
  for (unsigned int i = 0; i < len; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
}

MqttService::MqttService() : mqttClient(tlsClient)
{
}

void MqttService::begin(const MqttConfig &initialConfig)
{
  mqttClient.setCallback(onMessage);
  mqttClient.setKeepAlive(120);
  applyConfig(initialConfig);
}

void MqttService::applyConfig(const MqttConfig &newConfig)
{
  if (mqttClient.connected())
  {
    mqttClient.disconnect();
  }

  tlsClient.stop();
  config = newConfig;
  if (config.caCert.length() > 0)
  {
    tlsClient.setCACert(config.caCert.c_str());
  }
  else
  {
    tlsClient.setInsecure();
  }

  mqttClient.setServer(config.server.c_str(), config.port);
}

bool MqttService::connect(uint8_t maxAttempts)
{
  uint8_t attempts = 0;
  while (!mqttClient.connected())
  {
    if (maxAttempts > 0 && attempts >= maxAttempts)
    {
      return false;
    }
    attempts++;

    Serial.print("[Info] Connect MQTT Broker...");

    if (mqttClient.connect(config.clientId.c_str(),
                           config.user.c_str(),
                           config.password.c_str()))
    {
      Serial.println("[Info] MQTT connected!");
      if (mqttClient.subscribe(config.subscribeTopic.c_str()))
      {
        Serial.print("[Info] Subscribe topic: ");
        Serial.println(config.subscribeTopic);
      }
      else
      {
        Serial.print("[Error] Subscribe failed: ");
        Serial.println(config.subscribeTopic);
      }
      return true;
    }
    else
    {
      Serial.print("[Error] MQTT failed, state=");
      Serial.println(mqttClient.state());
      if (maxAttempts == 0)
      {
        delay(5000);
      }
      else
      {
        delay(1000);
      }
    }
  }

  return true;
}

void MqttService::ensureConnected()
{
  if (!mqttClient.connected())
  {
    connect(1);
  }
}

bool MqttService::isConnected()
{
  return mqttClient.connected();
}

void MqttService::loop()
{
  mqttClient.loop();
}

void MqttService::publishInitialMessage()
{
  String msg = "Hello from ESP32, uptime=" + String(millis() / 1000) + "s";
  if (mqttClient.publish(config.publishTopic.c_str(), msg.c_str()))
  {
    Serial.print("[Publish] msg=");
    Serial.println(msg);
  }
  else
  {
    Serial.print("[Error] Publish failed: ");
    Serial.println(config.publishTopic);
  }
}

void MqttService::reportTemperature(float temperature)
{
  reportIntProperty("temperature", temperature);
}

void MqttService::reportHumidity(float humidity)
{
  reportIntProperty("humidity", humidity);
}

void MqttService::reportCity(String city)
{
  reportStringProperty("city", city);
}

void MqttService::reportIpAddress(String ipAddress)
{
  reportStringProperty("ip_address", ipAddress);
}

void MqttService::reportIntProperty(const char *propertyName, float value)
{
  JsonDocument doc;
  JsonArray services = doc["services"].to<JsonArray>();
  JsonObject service = services.add<JsonObject>();
  service["service_id"] = AppConfig::serviceId;
  JsonObject props = service["properties"].to<JsonObject>();
  props[propertyName] = round(value * 100) / 100.0;

  String payload;
  serializeJson(doc, payload);

  if (mqttClient.publish(config.reportTopic.c_str(), payload.c_str()))
  {
    Serial.print("[Report] ");
    Serial.print(propertyName);
    Serial.print(" reported: ");
    Serial.println(payload);
  }
  else
  {
    Serial.print("[Error] ");
    Serial.print(propertyName);
    Serial.println(" report failed!");
  }
}

void MqttService::reportStringProperty(const char *propertyName, String value)
{
  JsonDocument doc;
  JsonArray services = doc["services"].to<JsonArray>();
  JsonObject service = services.add<JsonObject>();
  service["service_id"] = AppConfig::serviceId;
  JsonObject props = service["properties"].to<JsonObject>();
  props[propertyName] = value;

  String payload;
  serializeJson(doc, payload);

  if (mqttClient.publish(config.reportTopic.c_str(), payload.c_str()))
  {
    Serial.print("[Report] ");
    Serial.print(propertyName);
    Serial.print(" reported: ");
    Serial.println(payload);
  }
  else
  {
    Serial.print("[Error] ");
    Serial.print(propertyName);
    Serial.println(" report failed!");
  }
}
