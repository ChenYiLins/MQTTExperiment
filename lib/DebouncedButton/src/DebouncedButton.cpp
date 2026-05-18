#include "DebouncedButton.h"

DebouncedButton::DebouncedButton(int pin, unsigned long debounceDelayMs)
    : pin(pin),
      debounceDelayMs(debounceDelayMs),
      lastReading(LOW),
      stableState(LOW),
      lastDebounceTimeMs(0)
{
}

void DebouncedButton::begin(uint8_t mode)
{
  pinMode(pin, mode);
  lastReading = digitalRead(pin);
  stableState = lastReading;
}

bool DebouncedButton::rose(unsigned long nowMs)
{
  bool reading = digitalRead(pin);
  if (reading != lastReading)
  {
    lastDebounceTimeMs = nowMs;
    lastReading = reading;
  }

  if ((nowMs - lastDebounceTimeMs) > debounceDelayMs && reading != stableState)
  {
    stableState = reading;
    return stableState == HIGH;
  }

  return false;
}
