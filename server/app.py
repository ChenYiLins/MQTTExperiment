from __future__ import annotations

import json
import socket
import threading
import time
from pathlib import Path
from typing import Any

from flask import Flask, jsonify, redirect, render_template, request, url_for

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
CONFIG_PATH = DATA_DIR / "mqtt_config.json"
STATUS_PATH = DATA_DIR / "device_status.json"
SERVER_PORT = 5000
DISCOVERY_PORT = 4210
DISCOVERY_REQUEST = "MQTT_CONFIG_DISCOVERY"
DISCOVERY_RESPONSE_PREFIX = "MQTT_CONFIG_SERVER "

DEFAULT_CONFIG: dict[str, Any] = {
    "version": 1,
    "server": "",
    "port": 8883,
    "client_id": "esp32-mqtt-experiment",
    "user": "",
    "password": "",
    "upload_interval_ms": 5000,
    "publish_topic": "",
    "subscribe_topic": "",
    "report_topic": "",
    "ca_cert": "",
}

app = Flask(__name__)
app.config["TEMPLATES_AUTO_RELOAD"] = True
discovery_thread_started = False


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


def discovery_response() -> bytes:
    return f"{DISCOVERY_RESPONSE_PREFIX}http://{local_ip()}:{SERVER_PORT}".encode("utf-8")


def discovery_worker() -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", DISCOVERY_PORT))
    while True:
        try:
            payload, addr = sock.recvfrom(256)
        except OSError:
            continue
        if payload.decode("utf-8", errors="ignore").strip() != DISCOVERY_REQUEST:
            continue
        try:
            sock.sendto(discovery_response(), addr)
        except OSError:
            continue


def ensure_discovery_thread() -> None:
    global discovery_thread_started
    if discovery_thread_started:
        return
    thread = threading.Thread(target=discovery_worker, name="config-discovery", daemon=True)
    thread.start()
    discovery_thread_started = True


def read_json(path: Path, fallback: dict[str, Any]) -> dict[str, Any]:
    if not path.exists():
        return fallback.copy()
    try:
        data = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError):
        return fallback.copy()
    merged = fallback.copy()
    merged.update(data)
    return merged


def write_json(path: Path, data: dict[str, Any]) -> None:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def mark_device_seen(remote_addr: str | None, updates: dict[str, Any] | None = None) -> dict[str, Any]:
    status = read_json(STATUS_PATH, {})
    status["last_seen"] = int(time.time())
    status["remote_addr"] = remote_addr or ""
    status["device_connected"] = True
    if updates:
        status.update(updates)
    write_json(STATUS_PATH, status)
    return status


def mqtt_config() -> dict[str, Any]:
    return read_json(CONFIG_PATH, DEFAULT_CONFIG)


def status_defaults() -> dict[str, Any]:
    return {
        "last_seen": 0,
        "remote_addr": "",
        "device_connected": False,
        "mqtt_connected": False,
        "message": "",
        "uptime_ms": 0,
        "wifi_ssid": "",
        "wifi_ip": "",
        "wifi_rssi": 0,
        "mac_address": "",
        "mqtt_server": "",
        "mqtt_client_id": "",
        "upload_interval_ms": 0,
    }


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
            "upload_interval_ms": int(data.get("upload_interval_ms") or 0),
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
    upload_interval_ms = int(config.get("upload_interval_ms") or 0)
    if upload_interval_ms < 1000:
        errors.append("Upload interval must be at least 1000 ms")
    return errors


def device_status() -> dict[str, Any]:
    status = read_json(STATUS_PATH, status_defaults())
    last_seen = int(status.get("last_seen") or 0)
    now = int(time.time())
    offline_timeout_sec = 60
    age_sec = now - last_seen if last_seen else None
    online = bool(last_seen and age_sec is not None and age_sec <= offline_timeout_sec)
    status.setdefault("device_connected", False)
    status.setdefault("mqtt_connected", False)
    status.setdefault("message", "")
    status["last_seen_age_sec"] = age_sec
    status["online"] = online
    if last_seen:
        status["last_seen_text"] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(last_seen))
    else:
        status["last_seen_text"] = "-"
    if not online and not status["message"]:
        status["message"] = "设备已离线"
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
    form_data = request.form.to_dict()
    if any(form_data.values()):
        save_mqtt_config(form_data)
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
    mark_device_seen(request.remote_addr)
    errors = validate_config(config)
    if errors:
        return jsonify({"ok": False, "errors": errors}), 400
    return jsonify(config)


@app.post("/api/device-status")
def api_device_status():
    payload = request.get_json(silent=True) or {}
    status = {
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
        "upload_interval_ms": int(payload.get("upload_interval_ms") or 0),
    }
    mark_device_seen(request.remote_addr, status)
    return jsonify({"ok": True})


@app.get("/api/device-status")
def api_get_device_status():
    return jsonify(device_status())


if __name__ == "__main__":
    ensure_discovery_thread()
    app.run(host="0.0.0.0", port=SERVER_PORT, debug=False, use_reloader=False)
