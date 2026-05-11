#!/usr/bin/env python3
"""Local browser GUI for the PESP two-board demo.

The app reads the sensor-node USB serial console and exposes a small web UI.
It intentionally uses only Python stdlib plus pyserial so it can run on the
coursework machine without a frontend build step.
"""

from __future__ import annotations

import argparse
import json
import queue
import re
import threading
import time
from collections import deque
from dataclasses import dataclass, field
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import urlparse

import serial
from serial.tools import list_ports


ROOT = Path(__file__).resolve().parent
STATIC_DIR = ROOT / "static"

SAMPLE_ENV_RE = re.compile(
    r"^\[sample\] force=(?P<force>\d+) event=(?P<event>[01]) "
    r"thresh=(?P<threshold>\d+) temp=(?P<temp>-?\d+(?:\.\d+)?)C "
    r"hum=(?P<humidity>-?\d+(?:\.\d+)?)% "
    r"press=(?P<pressure>-?\d+(?:\.\d+)?)Pa "
    r"gas=(?P<gas>\d+ohm|ERR) light=(?P<light>\d+|ERR)$"
)
SAMPLE_ERR_RE = re.compile(
    r"^\[sample\] force=(?P<force>\d+) event=(?P<event>[01]) "
    r"thresh=(?P<threshold>\d+) env=ERR light=(?P<light>\d+|ERR)$"
)
LINK_RE = re.compile(r"^\[link-rx\] type=(?P<type>0x[0-9a-fA-F]+) cmd=(?P<cmd>.+)$")
CONFIG_RE = re.compile(r"^\[config\] (?P<key>[a-z_]+)=(?P<value>\d+)$")
EVENT_LATCH_RE = re.compile(r"^\[event\] latched force=(?P<force>\d+) threshold=(?P<threshold>\d+)$")
I2C_RE = re.compile(r"^\[i2c\] addr=(?P<addr>0x[0-9a-fA-F]+)$")
BME_RE = re.compile(r"^\[bme\] addr=(?P<addr>0x[0-9a-fA-F]+) (?P<result>.+)$")


def now_ms() -> int:
    return int(time.time() * 1000)


def iso_now() -> str:
    return time.strftime("%H:%M:%S")


def parse_number(text: str) -> int | float | None:
    if text == "ERR":
        return None
    if "." in text:
        return float(text)
    return int(text)


@dataclass
class DashboardState:
    connected: bool = False
    port: str | None = None
    baud: int = 115200
    status: str = "Disconnected"
    last_update_ms: int | None = None
    last_update_text: str = "never"
    raw_lines: deque[str] = field(default_factory=lambda: deque(maxlen=240))
    sensor: dict[str, Any] = field(default_factory=lambda: {
        "force": None,
        "threshold": None,
        "event": None,
        "temperature_c": None,
        "humidity_pct": None,
        "pressure_pa": None,
        "gas_ohms": None,
        "gas_status": "unknown",
        "light": None,
        "environment_ok": False,
    })
    base: dict[str, Any] = field(default_factory=lambda: {
        "last_command": None,
        "last_command_type": None,
        "last_command_ms": None,
        "polling": False,
        "read_all_count": 0,
        "read_force_count": 0,
        "read_event_count": 0,
        "clear_event_count": 0,
        "config": {
            "force_threshold": 1000,
            "sampling_ms": 2000,
            "interrupt": True,
        },
    })
    interrupt: dict[str, Any] = field(default_factory=lambda: {
        "state": "idle",
        "last_force": None,
        "threshold": None,
        "last_latched_ms": None,
        "last_event_text": None,
        "events": [],
    })
    diagnostics: dict[str, Any] = field(default_factory=lambda: {
        "i2c_devices": [],
        "bme": {},
    })

    def snapshot(self) -> dict[str, Any]:
        return {
            "connected": self.connected,
            "port": self.port,
            "baud": self.baud,
            "status": self.status,
            "last_update_ms": self.last_update_ms,
            "last_update_text": self.last_update_text,
            "raw_lines": list(self.raw_lines),
            "sensor": self.sensor,
            "base": self.base,
            "interrupt": self.interrupt,
            "diagnostics": self.diagnostics,
        }


class AppModel:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.state = DashboardState()
        self.subscribers: list[queue.Queue[dict[str, Any]]] = []

    def subscribe(self) -> queue.Queue[dict[str, Any]]:
        q: queue.Queue[dict[str, Any]] = queue.Queue(maxsize=32)
        with self.lock:
            self.subscribers.append(q)
            snapshot = self.state.snapshot()
        q.put({"kind": "state", "state": snapshot})
        return q

    def unsubscribe(self, q: queue.Queue[dict[str, Any]]) -> None:
        with self.lock:
            if q in self.subscribers:
                self.subscribers.remove(q)

    def publish(self, event: dict[str, Any]) -> None:
        stale: list[queue.Queue[dict[str, Any]]] = []
        with self.lock:
            targets = list(self.subscribers)
        for q in targets:
            try:
                q.put_nowait(event)
            except queue.Full:
                stale.append(q)
        for q in stale:
            self.unsubscribe(q)

    def set_connection(self, connected: bool, status: str, port: str | None = None, baud: int | None = None) -> None:
        with self.lock:
            self.state.connected = connected
            self.state.status = status
            if port is not None:
                self.state.port = port
            if baud is not None:
                self.state.baud = baud
            snapshot = self.state.snapshot()
        self.publish({"kind": "state", "state": snapshot})

    def add_event_log(self, text: str) -> None:
        events = self.state.interrupt["events"]
        events.insert(0, {"time": iso_now(), "text": text})
        del events[30:]

    def apply_line(self, line: str) -> None:
        line = line.strip()
        if not line:
            return

        with self.lock:
            self.state.raw_lines.append(line)
            self.state.last_update_ms = now_ms()
            self.state.last_update_text = iso_now()
            self._parse_locked(line)
            snapshot = self.state.snapshot()

        self.publish({"kind": "line", "line": line, "state": snapshot})

    def _parse_locked(self, line: str) -> None:
        env_match = SAMPLE_ENV_RE.match(line)
        if env_match:
            groups = env_match.groupdict()
            gas = groups["gas"]
            self.state.sensor.update({
                "force": int(groups["force"]),
                "threshold": int(groups["threshold"]),
                "event": bool(int(groups["event"])),
                "temperature_c": float(groups["temp"]),
                "humidity_pct": float(groups["humidity"]),
                "pressure_pa": float(groups["pressure"]),
                "gas_ohms": int(gas[:-3]) if gas.endswith("ohm") else None,
                "gas_status": "ok" if gas.endswith("ohm") else "err",
                "light": parse_number(groups["light"]),
                "environment_ok": True,
            })
            self.state.base["config"]["force_threshold"] = int(groups["threshold"])
            self.state.interrupt["state"] = "latched" if self.state.sensor["event"] else "idle"
            if self.state.sensor["event"]:
                self.state.interrupt["last_latched_ms"] = self.state.last_update_ms
            return

        err_match = SAMPLE_ERR_RE.match(line)
        if err_match:
            groups = err_match.groupdict()
            self.state.sensor.update({
                "force": int(groups["force"]),
                "threshold": int(groups["threshold"]),
                "event": bool(int(groups["event"])),
                "temperature_c": None,
                "humidity_pct": None,
                "pressure_pa": None,
                "gas_ohms": None,
                "gas_status": "err",
                "light": parse_number(groups["light"]),
                "environment_ok": False,
            })
            self.state.base["config"]["force_threshold"] = int(groups["threshold"])
            self.state.interrupt["state"] = "latched" if self.state.sensor["event"] else "idle"
            if self.state.sensor["event"]:
                self.state.interrupt["last_latched_ms"] = self.state.last_update_ms
            return

        link_match = LINK_RE.match(line)
        if link_match:
            command = link_match.group("cmd")
            self.state.base["last_command"] = command
            self.state.base["last_command_type"] = link_match.group("type")
            self.state.base["last_command_ms"] = now_ms()
            self.state.base["polling"] = True
            if command == "READ_ALL":
                self.state.base["read_all_count"] += 1
            elif command == "READ_FORCE":
                self.state.base["read_force_count"] += 1
            elif command == "READ_EVENT":
                self.state.base["read_event_count"] += 1
                self.state.interrupt["last_event_text"] = "Base requested READ_EVENT"
                self.add_event_log("base requested READ_EVENT")
            elif command == "CLEAR_EVENT":
                self.state.base["clear_event_count"] += 1
                self.state.interrupt["last_event_text"] = "Base sent CLEAR_EVENT"
                self.add_event_log("base sent CLEAR_EVENT")
            elif command.startswith("SET_FORCE_THRESHOLD="):
                self.state.base["config"]["force_threshold"] = int(command.split("=", 1)[1])
            elif command.startswith("SET_SAMPLING_RATE="):
                self.state.base["config"]["sampling_ms"] = int(command.split("=", 1)[1])
            elif command.startswith("ENABLE_INTERRUPT="):
                self.state.base["config"]["interrupt"] = command.endswith("=1")
            return

        config_match = CONFIG_RE.match(line)
        if config_match:
            key = config_match.group("key")
            value = int(config_match.group("value"))
            if key == "force_threshold":
                self.state.base["config"]["force_threshold"] = value
                self.state.sensor["threshold"] = value
            elif key == "sampling_ms":
                self.state.base["config"]["sampling_ms"] = value
            elif key == "interrupt":
                self.state.base["config"]["interrupt"] = bool(value)
            return

        latch_match = EVENT_LATCH_RE.match(line)
        if latch_match:
            force = int(latch_match.group("force"))
            threshold = int(latch_match.group("threshold"))
            self.state.interrupt.update({
                "state": "latched",
                "last_force": force,
                "threshold": threshold,
                "last_latched_ms": self.state.last_update_ms,
                "last_event_text": f"Latched force={force}, threshold={threshold}",
            })
            self.add_event_log(f"latched force={force}, threshold={threshold}")
            return

        if line == "[event] cleared":
            self.state.interrupt["state"] = "idle"
            self.state.interrupt["last_event_text"] = "Cleared"
            self.add_event_log("cleared")
            return

        i2c_match = I2C_RE.match(line)
        if i2c_match:
            addr = i2c_match.group("addr")
            devices = self.state.diagnostics["i2c_devices"]
            if addr not in devices:
                devices.append(addr)
                devices.sort()
            return

        if line == "[i2c] scan start":
            self.state.diagnostics["i2c_devices"] = []
            return

        bme_match = BME_RE.match(line)
        if bme_match:
            self.state.diagnostics["bme"][bme_match.group("addr")] = bme_match.group("result")


class SerialMonitor:
    def __init__(self, model: AppModel) -> None:
        self.model = model
        self.thread: threading.Thread | None = None
        self.stop_event = threading.Event()
        self.serial: serial.Serial | None = None
        self.write_lock = threading.Lock()

    def start(self, port: str, baud: int) -> None:
        self.stop()
        self.stop_event.clear()
        self.thread = threading.Thread(target=self._run, args=(port, baud), daemon=True)
        self.thread.start()

    def stop(self) -> None:
        self.stop_event.set()
        with self.write_lock:
            if self.serial is not None:
                try:
                    self.serial.close()
                except serial.SerialException:
                    pass
                self.serial = None
        if self.thread is not None and self.thread.is_alive():
            self.thread.join(timeout=1.5)
        self.thread = None
        self.model.set_connection(False, "Disconnected")

    def write_command(self, command: str) -> None:
        text = command.strip()
        if not text:
            return
        with self.write_lock:
            if self.serial is None or not self.serial.is_open:
                raise RuntimeError("serial port is not connected")
            self.serial.write((text + "\n").encode("utf-8"))
            self.serial.flush()
        self.model.apply_line(f"[host-tx] {text}")

    def _run(self, port: str, baud: int) -> None:
        try:
            ser = serial.Serial(port, baud, timeout=0.2, write_timeout=1)
            ser.reset_input_buffer()
            with self.write_lock:
                self.serial = ser
            self.model.set_connection(True, "Connected", port, baud)
        except serial.SerialException as exc:
            self.model.set_connection(False, f"Serial error: {exc}", port, baud)
            return

        buf = bytearray()
        try:
            while not self.stop_event.is_set():
                try:
                    data = ser.read(256)
                except serial.SerialException as exc:
                    self.model.set_connection(False, f"Serial error: {exc}", port, baud)
                    return
                if not data:
                    continue
                for byte in data:
                    if byte in (10, 13):
                        if buf:
                            self.model.apply_line(buf.decode("utf-8", errors="replace"))
                            buf.clear()
                    elif len(buf) < 512:
                        buf.append(byte)
                    else:
                        self.model.apply_line(buf.decode("utf-8", errors="replace"))
                        buf.clear()
        finally:
            with self.write_lock:
                try:
                    ser.close()
                except serial.SerialException:
                    pass
                if self.serial is ser:
                    self.serial = None
            if not self.stop_event.is_set():
                self.model.set_connection(False, "Disconnected", port, baud)


MODEL = AppModel()
MONITOR = SerialMonitor(MODEL)


class GuiHandler(BaseHTTPRequestHandler):
    server_version = "PESPHostGUI/1.0"

    def log_message(self, fmt: str, *args: Any) -> None:
        print("[gui]", fmt % args)

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path == "/":
            self._serve_file(STATIC_DIR / "index.html", "text/html; charset=utf-8")
        elif path == "/api/state":
            self._send_json(MODEL.state.snapshot())
        elif path == "/api/ports":
            ports = [
                {
                    "device": port.device,
                    "description": port.description,
                    "hwid": port.hwid,
                }
                for port in list_ports.comports()
            ]
            self._send_json({"ports": ports})
        elif path == "/events":
            self._serve_events()
        elif path.startswith("/static/"):
            self._serve_static(path)
        else:
            self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:
        path = urlparse(self.path).path
        try:
            payload = self._read_json()
            if path == "/api/connect":
                port = str(payload.get("port") or "")
                baud = int(payload.get("baud") or 115200)
                if not port:
                    self._send_json({"ok": False, "error": "port is required"}, HTTPStatus.BAD_REQUEST)
                    return
                MONITOR.start(port, baud)
                self._send_json({"ok": True})
            elif path == "/api/disconnect":
                MONITOR.stop()
                self._send_json({"ok": True})
            elif path == "/api/command":
                command = str(payload.get("command") or "")
                try:
                    MONITOR.write_command(command)
                    self._send_json({"ok": True})
                except RuntimeError as exc:
                    self._send_json({"ok": False, "error": str(exc)}, HTTPStatus.BAD_REQUEST)
            else:
                self.send_error(HTTPStatus.NOT_FOUND)
        except Exception as exc:  # noqa: BLE001 - HTTP boundary
            self._send_json({"ok": False, "error": str(exc)}, HTTPStatus.BAD_REQUEST)

    def _read_json(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        if length == 0:
            return {}
        raw = self.rfile.read(length)
        return json.loads(raw.decode("utf-8"))

    def _serve_static(self, path: str) -> None:
        rel = path.removeprefix("/static/")
        target = (STATIC_DIR / rel).resolve()
        if not str(target).startswith(str(STATIC_DIR.resolve())) or not target.is_file():
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        content_type = "text/plain; charset=utf-8"
        if target.suffix == ".css":
            content_type = "text/css; charset=utf-8"
        elif target.suffix == ".js":
            content_type = "text/javascript; charset=utf-8"
        self._serve_file(target, content_type)

    def _serve_file(self, target: Path, content_type: str) -> None:
        data = target.read_bytes()
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _send_json(self, payload: dict[str, Any], status: HTTPStatus = HTTPStatus.OK) -> None:
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _serve_events(self) -> None:
        q = MODEL.subscribe()
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "keep-alive")
        self.end_headers()
        try:
            while True:
                try:
                    event = q.get(timeout=15)
                    data = json.dumps(event, ensure_ascii=False)
                    self.wfile.write(f"data: {data}\n\n".encode("utf-8"))
                except queue.Empty:
                    self.wfile.write(b": keepalive\n\n")
                self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass
        finally:
            MODEL.unsubscribe(q)


def main() -> None:
    parser = argparse.ArgumentParser(description="PESP host GUI")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--serial", default="/dev/cu.usbmodem1101")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--no-autoconnect", action="store_true")
    args = parser.parse_args()

    if not args.no_autoconnect:
        MONITOR.start(args.serial, args.baud)

    httpd = ThreadingHTTPServer((args.host, args.port), GuiHandler)
    print(f"PESP host GUI: http://{args.host}:{args.port}")
    if not args.no_autoconnect:
        print(f"Serial autoconnect: {args.serial} @ {args.baud}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        MONITOR.stop()
        httpd.server_close()


if __name__ == "__main__":
    main()
