#pragma once

#include <Arduino.h>
#include <SSD1306.h>

class DisplayService
{
public:
  DisplayService(uint8_t address, int sdaPin, int sclPin);

  void begin();
  void showTemperature(float temperature);
  void showHumidity(float humidity);

private:
  SSD1306 display;
};
