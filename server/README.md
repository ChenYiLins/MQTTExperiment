# ESP32 MQTT Config Server

This Flask server provides a small web page and JSON API for updating the ESP32 MQTT settings without rebuilding firmware.

## Run

```powershell
cd D:\Projects\Source\ESP32\MQTTExperiment\server
python -m pip install -r requirements.txt
python .\app.py
```

Open:

```text
http://172.20.10.13:5000/
```

The ESP32 fetches:

```text
http://172.20.10.13:5000/api/device-config
```

If your computer's LAN IP changes, update these constants in `lib/AppConfig/src/AppConfig.h` and upload the firmware once:

```cpp
AppConfig::mqttConfigEndpoint
AppConfig::mqttConfigStatusEndpoint
```

After that, MQTT parameters can be changed from the web page. Saving the form increments `version`; the ESP32 polls every 10 seconds, stores the new config in NVS, disconnects, and reconnects MQTT with the latest settings.
