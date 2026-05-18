from __future__ import annotations

import json
import socket
import time
from pathlib import Path
from typing import Any

from flask import Flask, jsonify, redirect, render_template, request, url_for

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
CONFIG_PATH = DATA_DIR / "mqtt_config.json"
STATUS_PATH = DATA_DIR / "device_status.json"

DEFAULT_CONFIG: dict[str, Any] = {
    "version": 1,
    "server": "",
    "port": 8883,
    "client_id": "esp32-mqtt-experiment",
    "user": "",
    "password": "",
    "publish_topic": "",
    "subscribe_topic": "",
    "report_topic": "",
    "ca_cert": "",
}

app = Flask(__name__)
app.config["TEMPLATES_AUTO_RELOAD"] = True


@app.after_request
def add_no_cache_headers(response):
    response.headers["Cache-Control"] = "no-store, no-cache, must-revalidate, max-age=0"
    response.headers["Pragma"] = "no-cache"
    response.headers["Expires"] = "0"
    return response


def local_ip() -> str:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect(("8.8.8.8", 80))
        return sock.getsockname()[0]
    except OSError:
        return "127.0.0.1"
    finally:
        sock.close()


def read_json(path: Path, fallback: dict[str, Any]) -> dict[str, Any]:
    if not path.exists():
        return fallback.copy()
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return fallback.copy()
    merged = fallback.copy()
    merged.update(data)
    return merged


def write_json(path: Path, data: dict[str, Any]) -> None:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def mqtt_config() -> dict[str, Any]:
    return read_json(CONFIG_PATH, DEFAULT_CONFIG)


def save_mqtt_config(data: dict[str, Any]) -> dict[str, Any]:
    current = mqtt_config()
    config = DEFAULT_CONFIG.copy()
    config.update(
        {
            "version": int(current.get("version", 0)) + 1,
            "server": str(data.get("server", "")).strip(),
            "port": int(data.get("port") or 0),
            "client_id": str(data.get("client_id", "")).strip(),
            "user": str(data.get("user", "")).strip(),
            "password": str(data.get("password", "")),
            "publish_topic": str(data.get("publish_topic", "")).strip(),
            "subscribe_topic": str(data.get("subscribe_topic", "")).strip(),
            "report_topic": str(data.get("report_topic", "")).strip(),
            "ca_cert": str(data.get("ca_cert", "")).strip(),
        }
    )
    write_json(CONFIG_PATH, config)
    return config


def validate_config(config: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    for key, label in {
        "server": "MQTT server",
        "client_id": "Client ID",
        "publish_topic": "Publish topic",
        "subscribe_topic": "Subscribe topic",
        "report_topic": "Report topic",
    }.items():
        if not str(config.get(key, "")).strip():
            errors.append(f"{label} is required")
    port = int(config.get("port") or 0)
    if port < 1 or port > 65535:
        errors.append("Port must be between 1 and 65535")
    return errors


def device_status() -> dict[str, Any]:
    status = read_json(STATUS_PATH, {})
    last_seen = int(status.get("last_seen") or 0)
    status["online"] = bool(last_seen and int(time.time()) - last_seen <= 30)
    return status


@app.get("/")
def index():
    return render_template(
        "index.html",
        config=mqtt_config(),
        status=device_status(),
        host_ip=local_ip(),
    )


@app.post("/")
def save_from_form():
    config = save_mqtt_config(request.form.to_dict())
    errors = validate_config(config)
    if errors:
        return render_template(
            "index.html",
            config=config,
            status=device_status(),
            host_ip=local_ip(),
            errors=errors,
        ), 400
    return redirect(url_for("index"))


@app.post("/refresh")
def refresh_version():
    config = mqtt_config()
    config["version"] = int(config.get("version", 0)) + 1
    write_json(CONFIG_PATH, config)
    return redirect(url_for("index"))


@app.get("/api/config")
def api_config():
    return jsonify(mqtt_config())


@app.post("/api/config")
def api_save_config():
    payload = request.get_json(silent=True) or {}
    config = save_mqtt_config(payload)
    errors = validate_config(config)
    if errors:
        return jsonify({"ok": False, "errors": errors, "config": config}), 400
    return jsonify({"ok": True, "config": config})


@app.get("/api/device-config")
def api_device_config():
    config = mqtt_config()
    errors = validate_config(config)
    if errors:
        return jsonify({"ok": False, "errors": errors}), 400
    return jsonify(config)


@app.post("/api/device-status")
def api_device_status():
    payload = request.get_json(silent=True) or {}
    status = {
        "last_seen": int(time.time()),
        "remote_addr": request.remote_addr,
        "device_connected": bool(payload.get("device_connected")),
        "mqtt_connected": bool(payload.get("mqtt_connected")),
        "message": str(payload.get("message", "")),
        "uptime_ms": int(payload.get("uptime_ms") or 0),
        "wifi_ssid": str(payload.get("wifi_ssid", "")),
        "wifi_ip": str(payload.get("wifi_ip", "")),
        "wifi_rssi": int(payload.get("wifi_rssi") or 0),
        "mac_address": str(payload.get("mac_address", "")),
        "mqtt_server": str(payload.get("mqtt_server", "")),
        "mqtt_client_id": str(payload.get("mqtt_client_id", "")),
    }
    write_json(STATUS_PATH, status)
    return jsonify({"ok": True})


@app.get("/api/device-status")
def api_get_device_status():
    return jsonify(device_status())


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False)
