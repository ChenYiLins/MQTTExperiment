#pragma once

#include <Arduino.h>

class DebouncedButton
{
public:
  DebouncedButton(int pin, unsigned long debounceDelayMs);

  void begin(uint8_t mode = INPUT);
  bool rose(unsigned long nowMs);

private:
  int pin;
  unsigned long debounceDelayMs;
  bool lastReading;
  bool stableState;
  unsigned long lastDebounceTimeMs;
};
