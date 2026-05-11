const els = {};
const rawLines = [];

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

function setInterruptLamp(interrupt) {
  const latched = interrupt && interrupt.state === "latched";
  const recent = !latched && interrupt && interrupt.last_latched_ms &&
    (Date.now() - interrupt.last_latched_ms < 5000);
  const active = latched || recent;
  els.interruptLamp.classList.toggle("latched", active);
  els.interruptLamp.classList.toggle("idle", !active);
  els.interruptText.textContent = latched ? "interrupt" : (recent ? "recent" : "idle");
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
  els.tempValue.textContent = fixed(sensor.temperature_c, 2, " C");
  els.humidityValue.textContent = fixed(sensor.humidity_pct, 2, "%");
  els.pressureValue.textContent = fixed(sensor.pressure_pa, 0, " Pa");
  els.lightValue.textContent = text(sensor.light);
  els.forceValue.textContent = text(sensor.force);
  els.gasValue.textContent = sensor.gas_status === "ok"
    ? text(sensor.gas_ohms, " ohm")
    : (sensor.gas_status === "err" ? "not ready" : "--");

  setInterruptLamp(state.interrupt || {});

  if (Array.isArray(state.raw_lines) && state.raw_lines.length && rawLines.length === 0) {
    rawLines.push(...state.raw_lines.slice(-180));
    renderLog();
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
    "gasValue", "forceValue", "interruptLamp", "interruptText", "rawLog", "clearLogBtn",
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

  setInterval(refreshPorts, 10000);
  setInterval(async () => {
    updateState(await fetch("/api/state").then((response) => response.json()));
  }, 2500);
}

init().catch((error) => {
  if (els.rawLog) appendLine(`[gui-error] ${error.message}`);
});
