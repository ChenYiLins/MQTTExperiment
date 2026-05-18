// #include <Arduino.h>
// #include <WiFi.h>
// #include <WiFiClientSecure.h>

// void setup()
// {
//     Serial.begin(115200);

//     Serial.printf("Connecting to %s ...\n", WIFI_SSID);
//     WiFi.begin(WIFI_SSID, WIFI_PASS);
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("\nWiFi OK");

//     WiFiClientSecure tlsClient;
//     tlsClient.setInsecure();

//     Serial.printf("Connecting to %s:%d via TLS...\n", MQTT_SERVER, MQTT_PORT);

//     if (!tlsClient.connect(MQTT_SERVER, MQTT_PORT))
//     {
//         Serial.println("TLS connect FAILED");
//         return;
//     }

//     Serial.println("TLS OK");
//     tlsClient.stop();
// }

// void loop() {}