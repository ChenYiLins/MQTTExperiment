#include "WiFiService.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <ClientCredentials.h>

void WiFiService::connect()
{
  Serial.print("[Info] Connect WiFi");
  WiFi.begin(ClientCredentials::wifiSsid(), ClientCredentials::wifiPassword());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[Info] WiFi connected, IP: " + WiFi.localIP().toString());
}

void WiFiService::syncTime()
{
  configTime(0, 0, "pool.ntp.org");
  Serial.print("[Info] Sync time");
  time_t now = time(nullptr);
  while (now < 1000000000)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\n[Info] Time synced");
}
