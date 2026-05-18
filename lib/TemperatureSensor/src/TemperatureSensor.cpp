#include "TemperatureSensor.h"

#include <AppConfig.h>

TemperatureSensor::TemperatureSensor(int oneWireBusPin)
    : oneWire(oneWireBusPin), sensors(&oneWire)
{
}

void TemperatureSensor::begin()
{
  sensors.begin();
}

float TemperatureSensor::readCelsius()
{
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  if (temperature > 0.0f)
  {
    return temperature;
  }

  return AppConfig::fallbackTemperature;
}
