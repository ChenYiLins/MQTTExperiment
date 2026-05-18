#include "MqttConfig.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>

#include <AppConfig.h>
#include <ClientCredentials.h>

namespace
{
constexpr const char *preferencesNamespace = "mqtt_cfg";

Preferences preferences;

String readString(const char *key, const String &fallback)
{
  return preferences.getString(key, fallback);
}
}

void MqttConfigStore::begin()
{
  preferences.begin(preferencesNamespace, false);
}

MqttConfig MqttConfigStore::load()
{
  MqttConfig config = defaultConfig();
  loadFromNvs(config);
  return config;
}

bool MqttConfigStore::refreshFromServer(MqttConfig &config)
{
  HTTPClient http;
  http.setTimeout(AppConfig::configHttpTimeoutMs);

  if (!http.begin(AppConfig::mqttConfigEndpoint))
  {
    reportDeviceStatus(config, false, "Invalid config endpoint");
    return false;
  }

  int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK)
  {
    String message = "HTTP status " + String(statusCode);
    http.end();
    reportDeviceStatus(config, false, message);
    return false;
  }

  String payload = http.getString();
  http.end();

  MqttConfig fetchedConfig = config;
  if (!parsePayload(payload, fetchedConfig))
  {
    reportDeviceStatus(config, false, "Invalid config payload");
    return false;
  }

  if (fetchedConfig.version == config.version)
  {
    return false;
  }

  config = fetchedConfig;
  saveToNvs(config);
  reportDeviceStatus(config, false, "Config refreshed");
  return true;
}

MqttConfig MqttConfigStore::defaultConfig() const
{
  MqttConfig config;
  config.caCert = ClientCredentials::mqttCaCert();
  config.server = ClientCredentials::mqttServer();
  config.port = ClientCredentials::mqttPort();
  config.clientId = ClientCredentials::mqttClientId();
  config.user = ClientCredentials::mqttUser();
  config.password = ClientCredentials::mqttPassword();
  config.publishTopic = ClientCredentials::mqttPublishTopic();
  config.subscribeTopic = ClientCredentials::mqttSubscribeTopic();
  config.reportTopic = ClientCredentials::mqttReportTopic();
  config.uploadIntervalMs = AppConfig::uploadIntervalMs;
  config.version = 0;
  return config;
}

bool MqttConfigStore::loadFromNvs(MqttConfig &config)
{
  if (!preferences.isKey("server"))
  {
    return false;
  }

  config.caCert = readString("ca", config.caCert);
  config.server = readString("server", config.server);
  config.port = preferences.getUShort("port", config.port);
  config.clientId = readString("client_id", config.clientId);
  config.user = readString("user", config.user);
  config.password = readString("password", config.password);
  config.publishTopic = readString("pub_topic", config.publishTopic);
  config.subscribeTopic = readString("sub_topic", config.subscribeTopic);
  config.reportTopic = readString("report_topic", config.reportTopic);
  config.uploadIntervalMs = preferences.getULong("upload_ms", config.uploadIntervalMs);
  config.version = preferences.getULong("version", config.version);
  return true;
}

void MqttConfigStore::saveToNvs(const MqttConfig &config)
{
  preferences.putString("ca", config.caCert);
  preferences.putString("server", config.server);
  preferences.putUShort("port", config.port);
  preferences.putString("client_id", config.clientId);
  preferences.putString("user", config.user);
  preferences.putString("password", config.password);
  preferences.putString("pub_topic", config.publishTopic);
  preferences.putString("sub_topic", config.subscribeTopic);
  preferences.putString("report_topic", config.reportTopic);
  preferences.putULong("upload_ms", config.uploadIntervalMs);
  preferences.putULong("version", config.version);
}

bool MqttConfigStore::parsePayload(const String &payload, MqttConfig &config)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error)
  {
    return false;
  }

  const char *server = doc["server"] | "";
  uint16_t port = doc["port"] | 0;
  const char *clientId = doc["client_id"] | "";
  const char *publishTopic = doc["publish_topic"] | "";
  const char *subscribeTopic = doc["subscribe_topic"] | "";
  const char *reportTopic = doc["report_topic"] | "";
  unsigned long uploadIntervalMs = doc["upload_interval_ms"] | 0;

  if (strlen(server) == 0 || port == 0 || strlen(clientId) == 0 ||
      strlen(publishTopic) == 0 || strlen(subscribeTopic) == 0 ||
      strlen(reportTopic) == 0 || uploadIntervalMs == 0)
  {
    return false;
  }

  config.caCert = doc["ca_cert"] | "";
  config.server = server;
  config.port = port;
  config.clientId = clientId;
  config.user = doc["user"] | "";
  config.password = doc["password"] | "";
  config.publishTopic = publishTopic;
  config.subscribeTopic = subscribeTopic;
  config.reportTopic = reportTopic;
  config.uploadIntervalMs = uploadIntervalMs;
  config.version = doc["version"] | 0;
  return true;
}

void MqttConfigStore::reportDeviceStatus(const MqttConfig &config, bool mqttConnected, const String &message)
{
  Serial.print("[Info] ");
  Serial.println(message);

  HTTPClient http;
  http.setTimeout(AppConfig::configHttpTimeoutMs);
  if (!http.begin(AppConfig::mqttConfigStatusEndpoint))
  {
    return;
  }

  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["device_connected"] = WiFi.status() == WL_CONNECTED;
  doc["mqtt_connected"] = mqttConnected;
  doc["message"] = message;
  doc["uptime_ms"] = millis();
  doc["wifi_ssid"] = WiFi.SSID();
  doc["wifi_ip"] = WiFi.localIP().toString();
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["mac_address"] = WiFi.macAddress();
  doc["mqtt_server"] = config.server;
  doc["mqtt_client_id"] = config.clientId;
  doc["upload_interval_ms"] = config.uploadIntervalMs;

  String payload;
  serializeJson(doc, payload);
  http.POST(payload);
  http.end();
}
