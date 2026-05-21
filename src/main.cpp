#include <Arduino.h>

#include <AppConfig.h>
#include <DebouncedButton.h>
#include <DisplayService.h>
#include <HumidityService.h>
#include <MqttConfig.h>
#include <MqttService.h>
#include <TemperatureSensor.h>
#include <WiFiService.h>

enum class DisplayContentMode
{
  Temperature,
  Humidity
};

WiFiService wifiService;
HumidityService humidityService;
MqttConfigStore mqttConfigStore;
MqttConfig mqttConfig;
MqttService mqttService;
TemperatureSensor temperatureSensor(AppConfig::oneWireBusPin);
DisplayService display(AppConfig::displayAddress, AppConfig::displaySdaPin, AppConfig::displaySclPin);
DebouncedButton displayModeButton(AppConfig::switchPin, AppConfig::switchDebounceDelayMs);

DisplayContentMode displayContentMode = DisplayContentMode::Temperature;
unsigned long lastUploadTimeMs = 0;
unsigned long lastConfigRefreshTimeMs = 0;
unsigned long lastStatusReportTimeMs = 0;
float latestTemperature = AppConfig::fallbackTemperature;
float latestHumidity = AppConfig::fallbackHumidity;

void uploadTelemetry()
{
  mqttService.reportTemperature(latestTemperature);
  mqttService.reportHumidity(latestHumidity);
}

void refreshDisplay()
{
  if (displayContentMode == DisplayContentMode::Temperature)
  {
    display.showTemperature(latestTemperature);
    return;
  }
  else if(displayContentMode == DisplayContentMode::Humidity)
  {
    display.showHumidity(latestHumidity);
    return;
  }
}

void toggleDisplayMode()
{
  displayContentMode = displayContentMode == DisplayContentMode::Temperature ? DisplayContentMode::Humidity : DisplayContentMode::Temperature;
  Serial.println("[Info] Display mode changed to " + String(static_cast<int>(displayContentMode)));
}

void setup()
{
  Serial.begin(115200);

  wifiService.connect();
  // wifiService.syncTime();

  humidityService.begin();

  mqttConfigStore.begin();
  mqttConfig = mqttConfigStore.load();
  mqttConfigStore.refreshFromServer(mqttConfig);

  mqttService.begin(mqttConfig);
  mqttService.connect(3);
  mqttService.publishInitialMessage();
  mqttConfigStore.reportDeviceStatus(mqttConfig, mqttService.isConnected(), "Device connected");

  temperatureSensor.begin();
  display.begin();
  displayModeButton.begin(INPUT);
}

void loop()
{
  unsigned long now = millis();
  if (lastConfigRefreshTimeMs == 0 || (now - lastConfigRefreshTimeMs) >= AppConfig::configRefreshIntervalMs)
  {
    if (mqttConfigStore.refreshFromServer(mqttConfig))
    {
      mqttService.applyConfig(mqttConfig);
      mqttService.connect(3);
      mqttService.publishInitialMessage();
    }
    lastConfigRefreshTimeMs = now;
  }

  mqttService.ensureConnected();
  mqttService.loop();

  latestTemperature = temperatureSensor.readCelsius();
  latestHumidity = humidityService.getHumidity();
  if (latestHumidity < 0.0f)
  {
    latestHumidity = AppConfig::fallbackHumidity;
  }

  if (lastUploadTimeMs == 0 || (now - lastUploadTimeMs) >= mqttConfig.uploadIntervalMs)
  {
    uploadTelemetry();
    lastUploadTimeMs = now;
  }

  if (lastStatusReportTimeMs == 0 || (now - lastStatusReportTimeMs) >= AppConfig::statusReportIntervalMs)
  {
    mqttConfigStore.reportDeviceStatus(mqttConfig, mqttService.isConnected(), "Device heartbeat");
    lastStatusReportTimeMs = now;
  }

  refreshDisplay();

  if (displayModeButton.rose(now))
  {
    toggleDisplayMode();
  }
}
