const els = {};
const rawLines = [];
let configDirty = false;

function $(id) {
  return document.getElementById(id);
}

function text(value, suffix = "") {
  if (value === null || value === undefined) return "--";
  return `${value}${suffix}`;
}

function fixed(value, digits, suffix = "") {
  if (value === null || value === undefined || Number.isNaN(value)) return "--";
  return `${Number(value).toFixed(digits)}${suffix}`;
}

function setStatus(connected, status) {
  els.statusPill.textContent = status || (connected ? "Connected" : "Disconnected");
  els.statusPill.classList.toggle("online", connected);
  els.statusPill.classList.toggle("offline", !connected);
}

function updateState(state) {
  setStatus(state.connected, state.status);

  if (state.port) {
    const hasPort = Array.from(els.portSelect.options).some((option) => option.value === state.port);
    if (!hasPort) {
      const option = document.createElement("option");
      option.value = state.port;
      option.textContent = state.port;
      els.portSelect.appendChild(option);
    }
    els.portSelect.value = state.port;
  }
  els.baudInput.value = state.baud || 115200;
  els.lastUpdate.textContent = state.last_update_text
    ? `last update ${state.last_update_text}`
    : "waiting for serial data";

  const sensor = state.sensor || {};
  const cfg = (state.base && state.base.config) || {};
  els.tempValue.textContent = fixed(sensor.temperature_c, 2, " C");
  els.humidityValue.textContent = fixed(sensor.humidity_pct, 2, "%");
  els.pressureValue.textContent = fixed(sensor.pressure_pa, 0, " Pa");
  els.lightValue.textContent = text(sensor.light);
  els.forceValue.textContent = text(sensor.force);
  els.gasValue.textContent = sensor.gas_status === "ok"
    ? text(sensor.gas_ohms, " ohm")
    : (sensor.gas_status === "err" ? "not ready" : "--");

  els.lastInterruptValue.textContent = (state.interrupt && state.interrupt.last_interrupt_text) || "--";
  updateConfigControls(cfg);

  if (Array.isArray(state.raw_lines) && state.raw_lines.length && rawLines.length === 0) {
    rawLines.push(...state.raw_lines.slice(-180));
    renderLog();
  }
}

function updateConfigControls(cfg) {
  if (configDirty) return;

  if (cfg.force_threshold !== null && cfg.force_threshold !== undefined &&
      document.activeElement !== els.thresholdInput) {
    els.thresholdInput.value = cfg.force_threshold;
  }
  if (cfg.sampling_ms !== null && cfg.sampling_ms !== undefined &&
      document.activeElement !== els.samplingInput) {
    els.samplingInput.value = cfg.sampling_ms;
  }
  if (cfg.interrupt !== null && cfg.interrupt !== undefined &&
      document.activeElement !== els.interruptToggle) {
    els.interruptToggle.checked = Boolean(cfg.interrupt);
  }
}

function appendLine(line) {
  rawLines.push(line);
  while (rawLines.length > 300) rawLines.shift();
  renderLog();
}

function renderLog() {
  els.rawLog.textContent = rawLines.join("\n");
  els.rawLog.scrollTop = els.rawLog.scrollHeight;
}

async function refreshPorts() {
  const response = await fetch("/api/ports");
  const payload = await response.json();
  const current = els.portSelect.value || "/dev/cu.usbmodem1101";
  els.portSelect.innerHTML = "";

  (payload.ports || []).forEach((port) => {
    const option = document.createElement("option");
    option.value = port.device;
    option.textContent = port.description ? `${port.device} - ${port.description}` : port.device;
    els.portSelect.appendChild(option);
  });

  if (!Array.from(els.portSelect.options).some((option) => option.value === current)) {
    const option = document.createElement("option");
    option.value = current;
    option.textContent = current;
    els.portSelect.appendChild(option);
  }
  els.portSelect.value = current;
}

async function postJson(path, body) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  const payload = await response.json();
  if (!payload.ok) appendLine(`[gui-error] ${payload.error || "request failed"}`);
  return payload;
}

function setConfigStatus(text, mode = "") {
  els.configStatus.textContent = text;
  els.configStatus.classList.toggle("ok", mode === "ok");
  els.configStatus.classList.toggle("error", mode === "error");
}

async function applyConfig(event) {
  event.preventDefault();

  const forceThreshold = Number(els.thresholdInput.value);
  const samplingMs = Number(els.samplingInput.value);
  if (!Number.isInteger(forceThreshold) || forceThreshold < 0 || forceThreshold > 4095) {
    setConfigStatus("threshold 0-4095", "error");
    return;
  }
  if (!Number.isInteger(samplingMs) || samplingMs < 100 || samplingMs > 60000) {
    setConfigStatus("sampling 100-60000", "error");
    return;
  }

  setConfigStatus("applying");
  const payload = await postJson("/api/config", {
    force_threshold: forceThreshold,
    sampling_ms: samplingMs,
    interrupt: els.interruptToggle.checked,
  });
  setConfigStatus(payload.ok ? "applied" : "failed", payload.ok ? "ok" : "error");
  if (payload.ok) {
    setTimeout(() => {
      configDirty = false;
    }, 1800);
  }
}

function connectEvents() {
  const source = new EventSource("/events");
  source.onmessage = (message) => {
    const event = JSON.parse(message.data);
    if (event.kind === "line" && event.line) appendLine(event.line);
    if (event.state) updateState(event.state);
  };
  source.onerror = () => setStatus(false, "GUI stream reconnecting");
}

async function init() {
  [
    "portSelect", "baudInput", "connectBtn", "disconnectBtn", "statusPill",
    "lastUpdate", "tempValue", "humidityValue", "pressureValue", "lightValue",
    "gasValue", "forceValue", "lastInterruptValue", "rawLog", "clearLogBtn",
    "configForm", "thresholdInput", "samplingInput", "interruptToggle", "configStatus",
  ].forEach((id) => {
    els[id] = $(id);
  });

  await refreshPorts();
  updateState(await fetch("/api/state").then((response) => response.json()));
  connectEvents();

  els.connectBtn.addEventListener("click", () => {
    postJson("/api/connect", {
      port: els.portSelect.value,
      baud: Number(els.baudInput.value || 115200),
    });
  });
  els.disconnectBtn.addEventListener("click", () => postJson("/api/disconnect", {}));
  els.clearLogBtn.addEventListener("click", () => {
    rawLines.length = 0;
    renderLog();
  });
  [els.thresholdInput, els.samplingInput, els.interruptToggle].forEach((control) => {
    control.addEventListener("input", () => {
      configDirty = true;
      setConfigStatus("editing");
    });
    control.addEventListener("change", () => {
      configDirty = true;
      setConfigStatus("editing");
    });
  });
  els.configForm.addEventListener("submit", applyConfig);
  postJson("/api/command", { command: "READ_CONFIG" });

  setInterval(refreshPorts, 10000);
  setInterval(async () => {
    updateState(await fetch("/api/state").then((response) => response.json()));
  }, 2500);
}

init().catch((error) => {
  if (els.rawLog) appendLine(`[gui-error] ${error.message}`);
});
