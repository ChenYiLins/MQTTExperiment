#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>

class TemperatureSensor
{
public:
  explicit TemperatureSensor(int oneWireBusPin);

  void begin();
  float readCelsius();

private:
  OneWire oneWire;
  DallasTemperature sensors;
};
