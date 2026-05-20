#pragma once

#include <Arduino.h>

class HumidityService
{
public:
  /// Resolve location once and prime the cache. Call after WiFi is connected.
  bool begin();

  /// Returns the latest cached humidity (%). Fetches from API automatically
  /// when cache is stale or uninitialised. Returns -1 on failure.
  float getHumidity();

  /// Human-readable city name resolved from IP, or "Unknown" if unresolved.
  String getCityName() const;

  /// True when a valid humidity value has been fetched at least once.
  bool isValid() const;

private:
  String cityName;
  float latitude = 0.0f;
  float longitude = 0.0f;
  float cachedHumidity = 0.0f;
  unsigned long lastFetchTimeMs = 0;
  bool locationResolved = false;
  bool humidityValid = false;

  static constexpr unsigned long kWeatherCacheIntervalMs = 600000; // 10 min

  bool resolveLocation();
  float fetchHumidityFromApi();
};
