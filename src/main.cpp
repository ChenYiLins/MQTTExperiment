#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <time.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "SSD1306.h"

#include "D:\\Projects\\Source\\ESP32\\Internal\\clientConstInfo.h"

WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);

#define ONE_WIRE_BUS 32
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

SSD1306 display(0x3c, 21, 22);

int displayContentMode = 0; // 0: temperature, 1: humidity
bool lastSwState = LOW;
bool swStableState = LOW;
unsigned long lastDebounceTimeMs = 0;
const unsigned long swDebounceDelayMs = 40;

unsigned long lastUploadTimeMs = 0;
const unsigned long uploadIntervalMs = 5000;
float latestTemperature = -99.0;
float latestHumidity = 60.0;

void onMessage(char *topic, byte *payload, unsigned int len)
{
  Serial.print("[Recive] topic=");
  Serial.print(topic);
  Serial.print("  msg=");
  for (unsigned int i = 0; i < len; i++)
    Serial.print((char)payload[i]);
  Serial.println();
}

void connectWiFi()
{
  Serial.print("[Info] Connect WiFi");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[Info] WiFi connected, IP: " + WiFi.localIP().toString());
}

void initialTime()
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

void connectMQTT()
{
  while (!mqttClient.connected())
  {
    Serial.print("[Info] Connect MQTT Broker...");

    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
    {
      Serial.println("[Info] MQTT connected!");
      if (mqttClient.subscribe(MQTT_SUB_TOPIC))
      {
        Serial.print("[Info] Subscribe topic: ");
        Serial.println(MQTT_SUB_TOPIC);
      }
      else
      {
        Serial.print("[Error] Subscribe failed: ");
        Serial.println(MQTT_SUB_TOPIC);
      }
    }
    else
    {
      Serial.print("[Error] MQTT failed, state=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void initialMQTTMessage()
{
  String msg = "Hello from ESP32, uptime=" + String(millis() / 1000) + "s";
  if (mqttClient.publish(MQTT_PUB_TOPIC, msg.c_str()))
  {
    Serial.print("[Publish] msg=");
    Serial.println(msg);
  }
  else
  {
    Serial.print("[Error] Publish failed: ");
    Serial.println(MQTT_PUB_TOPIC);
  }
}

float getTemperature()
{
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  if (temperature > 0.0)
  {
    return temperature;
  }
  else
  {
    return -99.0;
  }
}

void uploadTemperature(float temperature)
{
  JsonDocument doc;
  JsonArray services = doc["services"].to<JsonArray>();
  JsonObject service = services.add<JsonObject>();
  service["service_id"] = "MQTT_experimental_service";
  JsonObject props = service["properties"].to<JsonObject>();
  props["temperature"] = round(temperature * 100) / 100.0;

  String payload;
  serializeJson(doc, payload);

  if (mqttClient.publish(MQTT_REPORT_TOPIC, payload.c_str()))
  {
    Serial.print("[Report] Temperature reported: ");
    Serial.println(payload);
  }
  else
  {
    Serial.println("[Error] Temperature report failed!");
  }
}

void uploadHumidity(float humidity)
{
  JsonDocument doc;
  JsonArray services = doc["services"].to<JsonArray>();
  JsonObject service = services.add<JsonObject>();
  service["service_id"] = "MQTT_experimental_service";
  JsonObject props = service["properties"].to<JsonObject>();
  props["humidity"] = round(humidity * 100) / 100.0;

  String payload;
  serializeJson(doc, payload);

  if (mqttClient.publish(MQTT_REPORT_TOPIC, payload.c_str()))
  {
    Serial.print("[Report] Humidity reported: ");
    Serial.println(payload);
  }
  else
  {
    Serial.println("[Error] Humidity report failed!");
  }
}

void initialSSD1306()
{
  display.init();
  display.flipScreenVertically();
}

void displayTemperature(float temperature)
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Temp:");
  display.drawString(0, 20, String(temperature, 2) + " C");
  display.display();
}

void displayHumidity(float humidity)
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Humidity:");
  display.drawString(0, 20, String(humidity, 2) + " %");
  display.display();
}

void setup()
{
  Serial.begin(115200);

  connectWiFi();
  // initialTime();

  tlsClient.setCACert(MQTT_CA_CERT);
  // mqttClient.setBufferSize(1024);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(onMessage);
  mqttClient.setKeepAlive(120);

  connectMQTT();
  initialMQTTMessage();
  sensors.begin();
  initialSSD1306();

  pinMode(3, INPUT);
}

void loop()
{
  if (!mqttClient.connected())
  {
    connectMQTT();
  }
  mqttClient.loop();

  latestTemperature = getTemperature();
  latestHumidity = 60.0; // For demo, we just use a fake humidity value

  unsigned long now = millis();
  if (lastUploadTimeMs == 0 || (now - lastUploadTimeMs) >= uploadIntervalMs)
  {
    uploadTemperature(latestTemperature);
    uploadHumidity(latestHumidity);
    lastUploadTimeMs = now;
  }

  if (displayContentMode == 0)
  {
    displayTemperature(latestTemperature);
  }
  else if (displayContentMode == 1)
  {
    // For demo, we just show a fake humidity value
    displayHumidity(latestHumidity);
  }

  bool swReading = digitalRead(3);
  if (swReading != lastSwState)
  {
    lastDebounceTimeMs = now;
    lastSwState = swReading;
  }

  if ((now - lastDebounceTimeMs) > swDebounceDelayMs && swReading != swStableState)
  {
    swStableState = swReading;
    if (swStableState == HIGH)
    {
      displayContentMode = (displayContentMode + 1) % 2;
      Serial.println("[Info] Display mode changed to " + String(displayContentMode));
    }
  }

  delay(30);
}
