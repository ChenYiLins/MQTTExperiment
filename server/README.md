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

The server also listens for discovery requests on UDP 4210. The ESP32 now broadcasts
`MQTT_CONFIG_DISCOVERY` on the LAN, receives the current server address, and then fetches:

```text
http://<current-server-ip>:5000/api/device-config
```

This means you no longer need to update a hardcoded LAN IP in firmware every time your computer gets a new address. As long as the ESP32 and this server stay on the same LAN and UDP broadcast is allowed, the device will rediscover the server automatically.

After that, MQTT parameters can be changed from the web page. Saving the form increments `version`; the ESP32 polls every 10 seconds, stores the new config in NVS, disconnects, and reconnects MQTT with the latest settings.
