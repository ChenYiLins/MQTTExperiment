#include <Arduino.h>

#include <AppConfig.h>
#include <DebouncedButton.h>
#include <DisplayService.h>
#include <MqttService.h>
#include <TemperatureSensor.h>
#include <WiFiService.h>

enum class DisplayContentMode
{
  Temperature,
  Humidity
};

WiFiService wifiService;
MqttService mqttService;
TemperatureSensor temperatureSensor(AppConfig::oneWireBusPin);
DisplayService display(AppConfig::displayAddress, AppConfig::displaySdaPin, AppConfig::displaySclPin);
DebouncedButton displayModeButton(AppConfig::switchPin, AppConfig::switchDebounceDelayMs);

DisplayContentMode displayContentMode = DisplayContentMode::Temperature;
unsigned long lastUploadTimeMs = 0;
float latestTemperature = AppConfig::fallbackTemperature;
float latestHumidity = AppConfig::demoHumidity;

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

  mqttService.begin();
  mqttService.connect();
  mqttService.publishInitialMessage();

  temperatureSensor.begin();
  display.begin();
  displayModeButton.begin(INPUT);
}

void loop()
{
  mqttService.ensureConnected();
  mqttService.loop();

  latestTemperature = temperatureSensor.readCelsius();
  latestHumidity = AppConfig::demoHumidity;

  unsigned long now = millis();
  if (lastUploadTimeMs == 0 || (now - lastUploadTimeMs) >= AppConfig::uploadIntervalMs)
  {
    uploadTelemetry();
    lastUploadTimeMs = now;
  }

  refreshDisplay();

  if (displayModeButton.rose(now))
  {
    toggleDisplayMode();
  }
}
