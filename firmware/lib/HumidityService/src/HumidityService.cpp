#include "HumidityService.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool HumidityService::begin()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[HumiditySvc] WiFi not connected – skip initialisation");
    return false;
  }

  if (resolveLocation())
  {
    float h = fetchHumidityFromApi();
    if (h >= 0.0f)
    {
      cachedHumidity = h;
      humidityValid = true;
      lastFetchTimeMs = millis();
      Serial.printf("[HumiditySvc] initial humidity = %.1f %%\n", cachedHumidity);
    }
  }

  return humidityValid;
}

float HumidityService::getHumidity()
{
  if (!locationResolved)
  {
    Serial.println("[HumiditySvc] location unresolved – returning -1");
    return -1.0f;
  }

  unsigned long now = millis();
  if (!humidityValid || (now - lastFetchTimeMs) >= kWeatherCacheIntervalMs)
  {
    float h = fetchHumidityFromApi();
    if (h >= 0.0f)
    {
      cachedHumidity = h;
      humidityValid = true;
      lastFetchTimeMs = now;
      Serial.printf("[HumiditySvc] updated humidity = %.1f %%\n", cachedHumidity);
    }
    else
    {
      Serial.println("[HumiditySvc] API call failed – returning stale value");
    }
  }

  return cachedHumidity;
}

String HumidityService::getCityName() const
{
  return cityName.length() > 0 ? cityName : String("Unknown");
}

String HumidityService::getIpAddress() const
{
  return ipAddress.length() > 0 ? ipAddress : String("Unknown");
}

bool HumidityService::isValid() const
{
  return humidityValid;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool HumidityService::resolveLocation()
{
  // ip-api.com free tier – no API key required for non-commercial use.
  // Returns JSON including city, lat, lon based on the caller's public IP.
  constexpr const char *kIpApiEndpoint = "http://ip-api.com/json/";

  HTTPClient http;
  http.setTimeout(5000);

  if (!http.begin(kIpApiEndpoint))
  {
    Serial.println("[HumiditySvc] ip-api begin() failed");
    return false;
  }

  int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK)
  {
    Serial.printf("[HumiditySvc] ip-api HTTP %d\n", statusCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.printf("[HumiditySvc] ip-api response: %s\n", payload.c_str());

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error)
  {
    Serial.printf("[HumiditySvc] ip-api JSON parse error: %s\n", error.c_str());
    return false;
  }

  const char *status = doc["status"] | "";
  if (strcmp(status, "success") != 0)
  {
    Serial.printf("[HumiditySvc] ip-api status: %s\n", status);
    return false;
  }

  const char *city = doc["city"] | "";
  float lat = doc["lat"] | 0.0f;
  float lon = doc["lon"] | 0.0f;

  const char *ip = doc["query"] | "";

  if (strlen(city) == 0 || (lat == 0.0f && lon == 0.0f))
  {
    Serial.println("[HumiditySvc] ip-api returned empty city/lat/lon");
    return false;
  }

  cityName = city;
  ipAddress = ip;
  latitude = lat;
  longitude = lon;
  locationResolved = true;

  Serial.printf("[HumiditySvc] location resolved: %s (%.4f, %.4f)\n",
                cityName.c_str(), latitude, longitude);
  return true;
}

float HumidityService::fetchHumidityFromApi()
{
  if (!locationResolved)
  {
    return -1.0f;
  }

  // Open-Meteo: free weather API (no API key required)
  // Documentation: https://open-meteo.com/en/docs
  String url = "http://api.open-meteo.com/v1/forecast?latitude=";
  url += String(latitude, 4);
  url += "&longitude=";
  url += String(longitude, 4);
  url += "&current=relative_humidity_2m";

  HTTPClient http;
  http.setTimeout(5000);

  if (!http.begin(url))
  {
    Serial.println("[HumiditySvc] Open-Meteo begin() failed");
    return -1.0f;
  }

  int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK)
  {
    Serial.printf("[HumiditySvc] Open-Meteo HTTP %d\n", statusCode);
    http.end();
    return -1.0f;
  }

  String payload = http.getString();
  http.end();

  Serial.printf("[HumiditySvc] Open-Meteo response: %s\n", payload.c_str());

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error)
  {
    Serial.printf("[HumiditySvc] Open-Meteo JSON parse error: %s\n", error.c_str());
    return -1.0f;
  }

  float humidity = doc["current"]["relative_humidity_2m"] | -1.0f;
  if (humidity < 0.0f)
  {
    Serial.println("[HumiditySvc] humidity field missing in response");
  }

  return humidity;
}
