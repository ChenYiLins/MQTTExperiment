#include "DisplayService.h"

DisplayService::DisplayService(uint8_t address, int sdaPin, int sclPin)
    : display(address, sdaPin, sclPin)
{
}

void DisplayService::begin()
{
  display.init();
  display.flipScreenVertically();
}

void DisplayService::showTemperature(float temperature)
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Temp:");
  display.drawString(0, 20, String(temperature, 2) + " C");
  display.display();
}

void DisplayService::showHumidity(float humidity)
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Humidity:");
  display.drawString(0, 20, String(humidity, 2) + " %");
  display.display();
}
