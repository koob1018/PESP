#!/usr/bin/env python3
"""Local browser GUI for the PESP two-board demo.

The app connects to the base-station USB serial endpoint. The base station
talks to the sensor node over the board-to-board UART protocol.
"""

from __future__ import annotations

import argparse
import json
import math
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
LIGHT_RAW_MAX = 65535

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
BASE_TX_RE = re.compile(r"^\[tx\] (?P<cmd>[A-Z_]+)(?:=(?P<payload>.*))?$")
BASE_DATA_RE = re.compile(r"^\[data\] (?P<payload>.+)$")
BASE_FORCE_RE = re.compile(r"^\[force\] (?P<payload>.+)$")
BASE_EVENT_RE = re.compile(r"^\[event\] (?P<payload>EVENT=.*)$")
BASE_CONFIG_RE = re.compile(r"^\[config\] (?P<payload>(?:OK,)?[A-Z_]+=.*)$")
CONFIG_RE = re.compile(r"^\[config\] (?P<key>[a-z_]+)=(?P<value>-?\d+(?:\.\d+)?)$")
EVENT_LATCH_RE = re.compile(r"^\[event\] latched force=(?P<force>\d+) threshold=(?P<threshold>\d+)$")
EVENT_SOURCE_RE = re.compile(
    r"^\[event\] latched source=(?P<source>[A-Z_]+) "
    r"(?P<key>light|temp|hum)=(?P<value>-?\d+(?:\.\d+)?) "
    r"threshold=(?P<threshold>-?\d+(?:\.\d+)?)$"
)
SERVICE_RE = re.compile(r"^\[service\] mode=(?P<mode>[01])$")
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


def parse_float_value(text: str) -> float | None:
    try:
        value = float(text)
    except (TypeError, ValueError):
        return None
    return value if math.isfinite(value) else None


def parse_int_value(text: str) -> int | None:
    value = parse_float_value(text)
    if value is None:
        return None
    return int(value)


def parse_payload_fields(payload: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for part in payload.split(","):
        if part == "OK" or "=" not in part:
            continue
        key, value = part.split("=", 1)
        fields[key.strip().upper()] = value.strip()
    return fields


def alarm_label_for_source(source: str | None) -> str:
    return {
        "FORCE": "Force Alert",
        "LIGHT": "Light Alert",
        "TEMP": "Temperature Alert",
        "HUMIDITY": "Humidity Alert",
    }.get(source or "", "Alarm Alert")


def alarm_name_for_source(source: str | None) -> str:
    return {
        "FORCE": "Force",
        "LIGHT": "Light",
        "TEMP": "Temperature",
        "HUMIDITY": "Humidity",
    }.get(source or "", "Alarm")


def detect_default_serial_port() -> str:
    ports = list(list_ports.comports())
    for port in ports:
        haystack = f"{port.device} {port.description} {port.hwid}".lower()
        if "case guardian" in haystack or "base station" in haystack:
            return port.device
    for port in ports:
        haystack = f"{port.device} {port.description} {port.hwid}".lower()
        if "usbmodem" in haystack or "pico" in haystack:
            return port.device
    return ports[0].device if ports else ""


@dataclass
class DashboardState:
    connected: bool = False
    port: str | None = None
    baud: int = 115200
    status: str = "Offline"
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
            "service": False,
            "light_high": 12000,
            "temp_high": 30.0,
            "humidity_high": 70.0,
        },
    })
    interrupt: dict[str, Any] = field(default_factory=lambda: {
        "state": "idle",
        "last_force": None,
        "threshold": None,
        "last_latched_ms": None,
        "last_interrupt_ms": None,
        "last_interrupt_text": None,
        "last_event_text": None,
        "alarm_source": None,
        "alarm_label": None,
        "alarm_detail": None,
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

    def add_event_log(
        self,
        text: str,
        kind: str = "info",
        force: int | None = None,
        threshold: int | None = None,
        armed_only: bool = False,
    ) -> None:
        config = self.state.base["config"]
        if armed_only and (not config.get("interrupt", True) or config.get("service", False)):
            return
        events = self.state.interrupt["events"]
        events.insert(0, {
            "time": iso_now(),
            "time_ms": now_ms(),
            "kind": kind,
            "text": text,
            "force": force,
            "threshold": threshold,
        })
        del events[30:]

    def record_interrupt_time(self) -> None:
        config = self.state.base["config"]
        if config.get("interrupt", True) and not config.get("service", False):
            self.state.interrupt["last_interrupt_ms"] = self.state.last_update_ms
            self.state.interrupt["last_interrupt_text"] = self.state.last_update_text

    def clear_alarm_state_locked(self) -> None:
        self.state.sensor["event"] = False
        self.state.interrupt.update({
            "state": "idle",
            "last_force": None,
            "threshold": None,
            "last_latched_ms": None,
            "last_interrupt_ms": None,
            "last_interrupt_text": None,
            "last_event_text": None,
            "alarm_source": None,
            "alarm_label": None,
            "alarm_detail": None,
        })
        self.state.interrupt["events"] = []

    def _record_base_command_locked(self, command: str, command_type: str | None = None) -> None:
        self.state.base["last_command"] = command
        self.state.base["last_command_type"] = command_type
        self.state.base["last_command_ms"] = now_ms()
        self.state.base["polling"] = True
        if command == "READ_ALL":
            self.state.base["read_all_count"] += 1
        elif command == "READ_FORCE":
            self.state.base["read_force_count"] += 1
        elif command == "READ_EVENT":
            self.state.base["read_event_count"] += 1
            self.state.interrupt["last_event_text"] = "Base requested READ_EVENT"
            self.record_interrupt_time()
            self.add_event_log("Base READ_EVENT", "driver", armed_only=True)
        elif command == "CLEAR_EVENT":
            self.state.base["clear_event_count"] += 1
            self.state.interrupt["last_event_text"] = "Base sent CLEAR_EVENT"
            self.add_event_log("Cleared by base", "clear", armed_only=True)
        elif command.startswith("SET_FORCE_THRESHOLD="):
            value = parse_int_value(command.split("=", 1)[1])
            if value is not None:
                self.state.base["config"]["force_threshold"] = value
        elif command.startswith("SET_SAMPLING_RATE="):
            value = parse_int_value(command.split("=", 1)[1])
            if value is not None:
                self.state.base["config"]["sampling_ms"] = value
        elif command.startswith("ENABLE_INTERRUPT="):
            self.state.base["config"]["interrupt"] = command.endswith("=1")
        elif command.startswith("SERVICE_MODE="):
            enabled = command.endswith("=1")
            self.state.base["config"]["service"] = enabled
            self.state.base["config"]["interrupt"] = not enabled
            if enabled:
                self.clear_alarm_state_locked()

    def _apply_config_fields_locked(self, fields: dict[str, str]) -> None:
        config = self.state.base["config"]
        if "FORCE_THRESHOLD" in fields:
            value = parse_int_value(fields["FORCE_THRESHOLD"])
            if value is not None:
                config["force_threshold"] = value
                self.state.sensor["threshold"] = value
        if "SAMPLING_MS" in fields:
            value = parse_int_value(fields["SAMPLING_MS"])
            if value is not None:
                config["sampling_ms"] = value
        if "INTERRUPT" in fields:
            config["interrupt"] = fields["INTERRUPT"] not in {"0", "false", "False"}
        if "SERVICE" in fields:
            service_enabled = fields["SERVICE"] not in {"0", "false", "False"}
            config["service"] = service_enabled
            if service_enabled:
                self.clear_alarm_state_locked()
        if "LIGHT_HIGH" in fields:
            value = parse_int_value(fields["LIGHT_HIGH"])
            if value is not None:
                config["light_high"] = value
        if "TEMP_HIGH" in fields:
            value = parse_float_value(fields["TEMP_HIGH"])
            if value is not None:
                config["temp_high"] = value
        if "HUMIDITY_HIGH" in fields:
            value = parse_float_value(fields["HUMIDITY_HIGH"])
            if value is not None:
                config["humidity_high"] = value

    def _apply_reading_fields_locked(self, fields: dict[str, str]) -> None:
        sensor = self.state.sensor
        if "FORCE" in fields and fields["FORCE"] != "ERR":
            value = parse_int_value(fields["FORCE"])
            if value is not None:
                sensor["force"] = value
        if "EVENT" in fields:
            sensor["event"] = fields["EVENT"] not in {"0", "false", "False"}
            self.state.interrupt["state"] = "latched" if sensor["event"] else "idle"
            if sensor["event"]:
                self.state.interrupt["last_latched_ms"] = self.state.last_update_ms
                self.record_interrupt_time()
        if "TEMP" in fields:
            sensor["temperature_c"] = None if fields["TEMP"] == "ERR" else parse_float_value(fields["TEMP"])
        if "HUM" in fields:
            sensor["humidity_pct"] = None if fields["HUM"] == "ERR" else parse_float_value(fields["HUM"])
        if "PRESS" in fields:
            value = parse_float_value(fields["PRESS"])
            sensor["pressure_pa"] = None if fields["PRESS"] == "ERR" or value is None else value * 100.0
        if "GAS" in fields:
            value = parse_int_value(fields["GAS"])
            sensor["gas_ohms"] = None if fields["GAS"] == "ERR" or value is None else value
            sensor["gas_status"] = "err" if fields["GAS"] == "ERR" or value is None else "ok"
        if "LIGHT" in fields:
            try:
                sensor["light"] = parse_number(fields["LIGHT"])
            except ValueError:
                sensor["light"] = None
        if {"TEMP", "HUM", "PRESS"} & set(fields):
            sensor["environment_ok"] = (
                sensor["temperature_c"] is not None
                and sensor["humidity_pct"] is not None
                and sensor["pressure_pa"] is not None
            )
        self._apply_config_fields_locked(fields)

    def _apply_base_event_locked(self, fields: dict[str, str]) -> None:
        self._apply_reading_fields_locked(fields)
        if fields.get("EVENT") not in {"1", "true", "True"}:
            return

        source = fields.get("SOURCE", "FORCE")
        config = self.state.base["config"]
        if source == "LIGHT":
            value = fields.get("EVENT_LIGHT") or fields.get("LIGHT") or "--"
            threshold = str(config.get("light_high", "--"))
        elif source == "TEMP":
            value = fields.get("EVT_T_C") or fields.get("TEMP") or "--"
            threshold = f"{float(config.get('temp_high', 0.0)):.2f}"
        elif source == "HUMIDITY":
            value = fields.get("EVT_H_PCT") or fields.get("HUM") or "--"
            threshold = f"{float(config.get('humidity_high', 0.0)):.2f}"
        else:
            source = "FORCE"
            value = fields.get("EVENT_FORCE") or fields.get("FORCE") or "--"
            threshold = fields.get("THRESH") or str(config.get("force_threshold", "--"))

        detail = f"{alarm_name_for_source(source)} {value} >= {threshold}"
        self.state.interrupt.update({
            "state": "latched",
            "last_force": int(fields["FORCE"]) if fields.get("FORCE", "").isdigit() else None,
            "threshold": int(threshold) if threshold.isdigit() else None,
            "last_latched_ms": self.state.last_update_ms,
            "last_event_text": detail,
            "alarm_source": source,
            "alarm_label": alarm_label_for_source(source),
            "alarm_detail": detail,
        })
        self.record_interrupt_time()
        self.add_event_log(
            detail,
            "tamper" if source == "FORCE" else "environment",
            force=int(value) if value.isdigit() else None,
            threshold=int(threshold) if threshold.isdigit() else None,
            armed_only=True,
        )

    def apply_local_mode(self, armed: bool) -> None:
        with self.lock:
            self.state.base["config"]["interrupt"] = armed
            self.state.base["config"]["service"] = not armed
            if not armed:
                self.clear_alarm_state_locked()
            snapshot = self.state.snapshot()
        self.publish({"kind": "state", "state": snapshot})

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
                self.record_interrupt_time()
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
                self.record_interrupt_time()
            return

        link_match = LINK_RE.match(line)
        if link_match:
            command = link_match.group("cmd")
            self._record_base_command_locked(command, link_match.group("type"))
            return

        base_tx_match = BASE_TX_RE.match(line)
        if base_tx_match:
            command = base_tx_match.group("cmd")
            payload = base_tx_match.group("payload")
            if payload is not None:
                command = f"{command}={payload}"
            self._record_base_command_locked(command)
            return

        data_match = BASE_DATA_RE.match(line)
        if data_match:
            self._apply_reading_fields_locked(parse_payload_fields(data_match.group("payload")))
            return

        force_match = BASE_FORCE_RE.match(line)
        if force_match:
            self._apply_reading_fields_locked(parse_payload_fields(force_match.group("payload")))
            return

        base_event_match = BASE_EVENT_RE.match(line)
        if base_event_match:
            self._apply_base_event_locked(parse_payload_fields(base_event_match.group("payload")))
            return

        base_config_match = BASE_CONFIG_RE.match(line)
        if base_config_match:
            self._apply_config_fields_locked(parse_payload_fields(base_config_match.group("payload")))
            return

        config_match = CONFIG_RE.match(line)
        if config_match:
            key = config_match.group("key")
            text_value = config_match.group("value")
            if key == "force_threshold":
                value = int(text_value)
                self.state.base["config"]["force_threshold"] = value
                self.state.sensor["threshold"] = value
            elif key == "sampling_ms":
                self.state.base["config"]["sampling_ms"] = int(text_value)
            elif key == "interrupt":
                self.state.base["config"]["interrupt"] = bool(int(text_value))
            elif key == "service":
                service_enabled = bool(int(text_value))
                self.state.base["config"]["service"] = service_enabled
                if service_enabled:
                    self.clear_alarm_state_locked()
            elif key == "light_high":
                self.state.base["config"]["light_high"] = int(text_value)
            elif key == "temp_high":
                self.state.base["config"]["temp_high"] = float(text_value)
            elif key == "humidity_high":
                self.state.base["config"]["humidity_high"] = float(text_value)
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
                "alarm_source": "FORCE",
                "alarm_label": alarm_label_for_source("FORCE"),
                "alarm_detail": f"Force {force} >= {threshold}",
            })
            self.record_interrupt_time()
            self.add_event_log(
                f"Force {force} >= {threshold}",
                "tamper",
                force=force,
                threshold=threshold,
                armed_only=True,
            )
            return

        source_match = EVENT_SOURCE_RE.match(line)
        if source_match:
            source = source_match.group("source")
            key = source_match.group("key")
            value = source_match.group("value")
            threshold = source_match.group("threshold")
            label = alarm_label_for_source(source)
            detail = f"{alarm_name_for_source(source)} {value} >= {threshold}"
            self.state.interrupt.update({
                "state": "latched",
                "last_latched_ms": self.state.last_update_ms,
                "last_event_text": f"{source} high: {key}={value}, threshold={threshold}",
                "alarm_source": source,
                "alarm_label": label,
                "alarm_detail": detail,
            })
            self.record_interrupt_time()
            self.add_event_log(
                detail,
                "environment" if source in {"TEMP", "HUMIDITY"} else "light",
                armed_only=True,
            )
            return

        service_match = SERVICE_RE.match(line)
        if service_match:
            enabled = service_match.group("mode") == "1"
            self.state.base["config"]["service"] = enabled
            if enabled:
                self.clear_alarm_state_locked()
            return

        if line == "[event] cleared":
            self.state.interrupt["state"] = "idle"
            self.state.interrupt["last_event_text"] = "Cleared"
            self.add_event_log("Cleared", "clear", armed_only=True)
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
        self.model.set_connection(False, "Offline")

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
                self.model.set_connection(False, "Offline", port, baud)


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
            elif path == "/api/config":
                commands = self._build_config_commands(payload)
                if not commands:
                    self._send_json({"ok": False, "error": "no configuration fields provided"}, HTTPStatus.BAD_REQUEST)
                    return
                try:
                    for command in commands:
                        MONITOR.write_command(command)
                    if "interrupt" in payload:
                        MODEL.apply_local_mode(bool(payload["interrupt"]))
                    self._send_json({"ok": True, "commands": commands})
                except RuntimeError as exc:
                    self._send_json({"ok": False, "error": str(exc)}, HTTPStatus.BAD_REQUEST)
            else:
                self.send_error(HTTPStatus.NOT_FOUND)
        except Exception as exc:  # noqa: BLE001 - HTTP boundary
            self._send_json({"ok": False, "error": str(exc)}, HTTPStatus.BAD_REQUEST)

    def _build_config_commands(self, payload: dict[str, Any]) -> list[str]:
        commands: list[str] = []

        if "force_threshold" in payload:
            threshold = int(payload["force_threshold"])
            if threshold < 0 or threshold > 4095:
                raise ValueError("force threshold must be between 0 and 4095")
            commands.append(f"SET_FORCE_THRESHOLD={threshold}")

        if "sampling_ms" in payload:
            sampling_ms = int(payload["sampling_ms"])
            if sampling_ms < 100 or sampling_ms > 60000:
                raise ValueError("sampling interval must be between 100 and 60000 ms")
            commands.append(f"SET_SAMPLING_RATE={sampling_ms}")

        if "light_high" in payload:
            light_high = int(payload["light_high"])
            if light_high < 0 or light_high > LIGHT_RAW_MAX:
                raise ValueError(f"light threshold must be between 0 and {LIGHT_RAW_MAX}")
            commands.append(f"SET_LIGHT_HIGH={light_high}")

        if "temp_high" in payload:
            temp_high = float(payload["temp_high"])
            if not math.isfinite(temp_high) or temp_high < -40.0 or temp_high > 85.0:
                raise ValueError("temperature threshold must be between -40 and 85 C")
            commands.append(f"SET_TEMP_HIGH={temp_high:.2f}")

        if "humidity_high" in payload:
            humidity_high = float(payload["humidity_high"])
            if not math.isfinite(humidity_high) or humidity_high < 0.0 or humidity_high > 100.0:
                raise ValueError("humidity threshold must be between 0 and 100%")
            commands.append(f"SET_HUMIDITY_HIGH={humidity_high:.2f}")

        if "interrupt" in payload:
            interrupt = payload["interrupt"]
            if not isinstance(interrupt, bool):
                raise ValueError("interrupt must be true or false")
            commands.append(f"SERVICE_MODE={0 if interrupt else 1}")
            commands.append(f"ENABLE_INTERRUPT={1 if interrupt else 0}")

        if commands:
            commands.append("READ_CONFIG")
        return commands

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
    parser.add_argument("--serial", default="")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--no-autoconnect", action="store_true")
    args = parser.parse_args()

    serial_port = args.serial or detect_default_serial_port()

    if not args.no_autoconnect and serial_port:
        MONITOR.start(serial_port, args.baud)

    httpd = ThreadingHTTPServer((args.host, args.port), GuiHandler)
    print(f"PESP host GUI: http://{args.host}:{args.port}")
    if not args.no_autoconnect and serial_port:
        print(f"Serial autoconnect: {serial_port} @ {args.baud}")
    elif not args.no_autoconnect:
        print("Serial autoconnect skipped: no serial ports found")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        MONITOR.stop()
        httpd.server_close()


if __name__ == "__main__":
    main()
