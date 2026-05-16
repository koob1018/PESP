const els = {};
const rawLines = [];
const trendSamples = [];

const TAMPER_HOLD_MS = 15000;
const TREND_WINDOW_MS = 180000;
const MAX_RAW_LINES = 80;
const SENSITIVITY_PRESETS = { high: 600, medium: 1000, low: 1800 };
const RATE_PRESETS = { fast: 500, normal: 2000, slow: 5000 };
const TREND_DOMAINS = {
  temperature: { min: 0, max: 50, unit: "C" },
  humidity: { min: 0, max: 100, unit: "%" },
  light: { min: 0, max: 4095, unit: "" },
  force: { min: 0, max: 4095, unit: "" },
};

let configDirty = false;
let lastTrendSampleMs = null;
let latestState = null;
let calibration = {
  active: false,
  samples: [],
  timer: null,
  recommended: null,
  baseline: null,
  startedAt: null,
  lastSampleMs: null,
};

function $(id) {
  return document.getElementById(id);
}

function text(value, suffix = "") {
  if (value === null || value === undefined || Number.isNaN(value)) return "--";
  return `${value}${suffix}`;
}

function fixed(value, digits, suffix = "") {
  if (value === null || value === undefined || Number.isNaN(value)) return "--";
  return `${Number(value).toFixed(digits)}${suffix}`;
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function setStatus(connected, status) {
  els.statusPill.textContent = status || (connected ? "Connected" : "Disconnected");
  els.statusPill.classList.toggle("online", connected);
  els.statusPill.classList.toggle("offline", !connected);
}

function ageMs(timestamp) {
  if (!timestamp) return Infinity;
  return Date.now() - timestamp;
}

function isArmed(cfg) {
  return !cfg || cfg.interrupt !== false;
}

function deriveLightStatus(light) {
  if (light === null || light === undefined) return { label: "--", className: "" };
  if (light < 800) return { label: "Dark", className: "dark" };
  if (light > 3000) return { label: "Bright", className: "bright" };
  return { label: "Normal", className: "normal" };
}

function deriveCaseStatus(state) {
  const sensor = state.sensor || {};
  const cfg = (state.base && state.base.config) || {};
  const armed = isArmed(cfg);
  const recentTamper = armed && (
    sensor.event === true ||
    ageMs(state.interrupt && state.interrupt.last_interrupt_ms) <= TAMPER_HOLD_MS
  );

  if (recentTamper) {
    return {
      label: "Tamper Alert",
      className: "status-tamper",
      reason: "Touch or movement event detected by the sensor node.",
    };
  }
  if (!state.connected) {
    return {
      label: "Attention",
      className: "status-attention",
      reason: "Serial connection is not active.",
    };
  }
  if (!state.last_update_ms || ageMs(state.last_update_ms) > 7000) {
    return {
      label: "Attention",
      className: "status-attention",
      reason: "Waiting for fresh sensor data.",
    };
  }
  if (sensor.environment_ok === false) {
    return {
      label: "Attention",
      className: "status-attention",
      reason: "Environment sensor is unavailable.",
    };
  }
  return {
    label: "Protected",
    className: "status-protected",
    reason: "Environment stream is available and no armed tamper event is active.",
  };
}

function updateCaseStatus(state) {
  const status = deriveCaseStatus(state);
  els.caseStatusValue.textContent = status.label;
  els.caseStatusReason.textContent = status.reason;
  els.caseStatusCard.classList.remove("status-protected", "status-attention", "status-tamper");
  els.caseStatusCard.classList.add(status.className);

  const cfg = (state.base && state.base.config) || {};
  const sensor = state.sensor || {};
  els.protectionModeValue.textContent = isArmed(cfg) ? "Armed" : "Service";
  els.lastInterruptValue.textContent = (state.interrupt && state.interrupt.last_interrupt_text) || "--";
  els.tamperStateValue.textContent = sensor.event
    ? (isArmed(cfg) ? "Latched" : "Latched (service)")
    : "Idle";
}

function updateMetrics(state) {
  const sensor = state.sensor || {};
  const cfg = (state.base && state.base.config) || {};
  const light = deriveLightStatus(sensor.light);

  els.tempValue.textContent = fixed(sensor.temperature_c, 2, " C");
  els.humidityValue.textContent = fixed(sensor.humidity_pct, 2, "%");
  els.pressureValue.textContent = fixed(sensor.pressure_pa, 0, " Pa");
  els.gasValue.textContent = sensor.gas_status === "ok"
    ? text(sensor.gas_ohms, " ohm")
    : "--";
  els.gasStatus.textContent = sensor.gas_status === "ok"
    ? "VOC proxy available"
    : (sensor.environment_ok ? "warming or not ready" : "environment unavailable");
  els.environmentStatus.textContent = sensor.environment_ok
    ? "BME680 environment stream available"
    : "Environment sensor unavailable";

  els.lightValue.textContent = text(sensor.light);
  els.lightStatus.textContent = light.label;
  els.lightStatus.className = `state-chip ${light.className}`;

  els.forceValue.textContent = text(sensor.force);
  els.thresholdValue.textContent = text(sensor.threshold ?? cfg.force_threshold);
  els.trendTempNow.textContent = fixed(sensor.temperature_c, 1, " C");
  els.trendHumidityNow.textContent = fixed(sensor.humidity_pct, 1, "%");
  els.trendLightNow.textContent = text(sensor.light);
  els.trendForceNow.textContent = text(sensor.force);
}

function selectPresetForValue(value, presets) {
  for (const [name, presetValue] of Object.entries(presets)) {
    if (Number(value) === presetValue) return name;
  }
  return "custom";
}

function updateConfigControls(cfg) {
  if (configDirty) return;

  if (cfg.force_threshold !== null && cfg.force_threshold !== undefined &&
      document.activeElement !== els.thresholdInput) {
    els.thresholdInput.value = cfg.force_threshold;
    els.sensitivitySelect.value = selectPresetForValue(cfg.force_threshold, SENSITIVITY_PRESETS);
  }
  if (cfg.sampling_ms !== null && cfg.sampling_ms !== undefined &&
      document.activeElement !== els.samplingInput) {
    els.samplingInput.value = cfg.sampling_ms;
    els.rateSelect.value = selectPresetForValue(cfg.sampling_ms, RATE_PRESETS);
  }
  if (cfg.interrupt !== null && cfg.interrupt !== undefined &&
      document.activeElement !== els.protectionModeSelect) {
    els.protectionModeSelect.value = cfg.interrupt ? "armed" : "service";
  }
}

function updateDiagnostics(state) {
  const base = state.base || {};
  const cfg = base.config || {};
  const sensor = state.sensor || {};
  const readCounts = [
    `all ${base.read_all_count || 0}`,
    `force ${base.read_force_count || 0}`,
    `event ${base.read_event_count || 0}`,
    `clear ${base.clear_event_count || 0}`,
  ].join(" / ");

  els.diagConnection.textContent = state.connected
    ? `${state.port || "connected"} @ ${state.baud || 115200}`
    : state.status || "Disconnected";
  els.diagLastCommand.textContent = base.last_command || "--";
  els.diagReadCounts.textContent = readCounts;
  els.diagEnvironment.textContent = sensor.environment_ok ? "online" : "unavailable";
  els.diagGas.textContent = sensor.gas_status === "ok" ? "ready" : "not ready";
  els.diagConfigSync.textContent =
    `threshold ${text(cfg.force_threshold)}, sampling ${text(cfg.sampling_ms, " ms")}, ${isArmed(cfg) ? "armed" : "service"}`;
}

function renderAlarmLog(events) {
  const list = Array.isArray(events) ? events : [];
  if (!list.length) {
    els.alarmLog.innerHTML = '<p class="empty-log">No alarms in this session.</p>';
    return;
  }

  els.alarmLog.innerHTML = "";
  list.slice(0, 30).forEach((event) => {
    const item = document.createElement("div");
    item.className = `event-item ${event.kind || "info"}`;

    const time = document.createElement("div");
    time.className = "event-time";
    time.textContent = event.time || "--";

    const message = document.createElement("div");
    message.className = "event-text";
    message.textContent = event.text || "Event";

    item.append(time, message);
    els.alarmLog.appendChild(item);
  });
}

function addTrendSample(state) {
  const sensor = state.sensor || {};
  if (!state.last_update_ms || state.last_update_ms === lastTrendSampleMs) return;
  lastTrendSampleMs = state.last_update_ms;

  if ([sensor.temperature_c, sensor.humidity_pct, sensor.light, sensor.force]
      .every((value) => value === null || value === undefined)) {
    return;
  }

  trendSamples.push({
    ts: state.last_update_ms,
    temperature: sensor.temperature_c,
    humidity: sensor.humidity_pct,
    light: sensor.light,
    force: sensor.force,
  });

  const cutoff = state.last_update_ms - TREND_WINDOW_MS;
  while (trendSamples.length && trendSamples[0].ts < cutoff) trendSamples.shift();
  renderTrends();
}

function formatAxis(value, unit) {
  if (value >= 1000) return `${Math.round(value / 100) / 10}k${unit}`;
  return `${value}${unit}`;
}

function drawTrend(canvas, key, color, domain) {
  const ctx = canvas.getContext("2d");
  const rect = canvas.getBoundingClientRect();
  const width = Math.max(160, Math.floor(rect.width));
  const height = Math.max(120, Math.floor(rect.height));
  const ratio = window.devicePixelRatio || 1;
  canvas.width = Math.floor(width * ratio);
  canvas.height = Math.floor(height * ratio);
  ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
  ctx.clearRect(0, 0, width, height);

  const left = 34;
  const right = width - 8;
  const plotTop = 10;
  const plotBottom = height - 22;
  const midY = (plotTop + plotBottom) / 2;

  ctx.strokeStyle = "#d7e0e8";
  ctx.lineWidth = 1;
  [plotTop, midY, plotBottom].forEach((y) => {
    ctx.beginPath();
    ctx.moveTo(left, y);
    ctx.lineTo(right, y);
    ctx.stroke();
  });

  ctx.fillStyle = "#647283";
  ctx.font = "11px system-ui";
  ctx.textAlign = "left";
  ctx.textBaseline = "middle";
  ctx.fillText(formatAxis(domain.max, domain.unit), 0, plotTop);
  ctx.fillText(formatAxis(Math.round((domain.min + domain.max) / 2), domain.unit), 0, midY);
  ctx.fillText(formatAxis(domain.min, domain.unit), 0, plotBottom);

  ctx.strokeStyle = "#bdc9d4";
  ctx.beginPath();
  ctx.moveTo(left, plotTop);
  ctx.lineTo(left, plotBottom);
  ctx.stroke();

  const points = trendSamples
    .map((sample) => ({ ts: sample.ts, value: sample[key] }))
    .filter((point) => point.value !== null && point.value !== undefined && !Number.isNaN(point.value));

  if (points.length < 2) {
    ctx.fillStyle = "#637181";
    ctx.font = "12px system-ui";
    ctx.textAlign = "left";
    ctx.textBaseline = "alphabetic";
    ctx.fillText("waiting", left + 6, 26);
    return;
  }

  const minTs = trendSamples[0].ts;
  const maxTs = trendSamples[trendSamples.length - 1].ts;
  const xFor = (ts) => {
    if (maxTs === minTs) return right;
    return left + ((ts - minTs) / (maxTs - minTs)) * (right - left);
  };
  const yFor = (value) => {
    const clamped = clamp(value, domain.min, domain.max);
    return plotBottom - ((clamped - domain.min) / (domain.max - domain.min)) * (plotBottom - plotTop);
  };

  ctx.strokeStyle = color;
  ctx.lineWidth = 2;
  ctx.beginPath();
  points.forEach((point, index) => {
    const x = xFor(point.ts);
    const y = yFor(point.value);
    if (index === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();

  const latest = points[points.length - 1];
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(xFor(latest.ts), yFor(latest.value), 3, 0, Math.PI * 2);
  ctx.fill();
}

function renderTrends() {
  drawTrend(els.trendTemp, "temperature", "#ba2d24", TREND_DOMAINS.temperature);
  drawTrend(els.trendHumidity, "humidity", "#246a9c", TREND_DOMAINS.humidity);
  drawTrend(els.trendLight, "light", "#9b6200", TREND_DOMAINS.light);
  drawTrend(els.trendForce, "force", "#147c5b", TREND_DOMAINS.force);
}

function updateState(state) {
  latestState = state;
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

  const cfg = (state.base && state.base.config) || {};
  updateCaseStatus(state);
  updateMetrics(state);
  updateConfigControls(cfg);
  updateDiagnostics(state);
  renderAlarmLog(state.interrupt && state.interrupt.events);
  addTrendSample(state);
  collectCalibrationSample(state);

  if (Array.isArray(state.raw_lines) && state.raw_lines.length && rawLines.length === 0) {
    rawLines.push(...state.raw_lines.slice(-MAX_RAW_LINES));
    updateDebugSummary();
    renderLog();
  }
}

function appendLine(line) {
  rawLines.push(line);
  while (rawLines.length > MAX_RAW_LINES) rawLines.shift();
  updateDebugSummary();
  if (els.debugPanel && els.debugPanel.open) {
    renderLog();
  }
}

function renderLog() {
  if (!els.debugPanel || !els.debugPanel.open) {
    els.rawLog.textContent = "";
    return;
  }
  els.rawLog.textContent = rawLines.join("\n");
  els.rawLog.scrollTop = els.rawLog.scrollHeight;
}

function updateDebugSummary() {
  if (!els.debugSummary) return;
  const lineWord = rawLines.length === 1 ? "line" : "lines";
  els.debugSummary.textContent = `${rawLines.length} recent ${lineWord} cached`;
}

async function refreshPorts() {
  const response = await fetch("/api/ports");
  const payload = await response.json();
  const defaultPort = (payload.ports && payload.ports[0] && payload.ports[0].device) || "";
  const current = els.portSelect.value || defaultPort;
  els.portSelect.innerHTML = "";

  (payload.ports || []).forEach((port) => {
    const option = document.createElement("option");
    option.value = port.device;
    option.textContent = port.description ? `${port.device} - ${port.description}` : port.device;
    els.portSelect.appendChild(option);
  });

  if (current && !Array.from(els.portSelect.options).some((option) => option.value === current)) {
    const option = document.createElement("option");
    option.value = current;
    option.textContent = current;
    els.portSelect.appendChild(option);
  }
  if (current) {
    els.portSelect.value = current;
  }
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

function setConfigStatus(textValue, mode = "") {
  els.configStatus.textContent = textValue;
  els.configStatus.classList.toggle("ok", mode === "ok");
  els.configStatus.classList.toggle("error", mode === "error");
}

function markConfigDirty() {
  configDirty = true;
  setConfigStatus("editing");
}

function syncPresetControls() {
  const threshold = Number(els.thresholdInput.value);
  const sampling = Number(els.samplingInput.value);
  els.sensitivitySelect.value = selectPresetForValue(threshold, SENSITIVITY_PRESETS);
  els.rateSelect.value = selectPresetForValue(sampling, RATE_PRESETS);
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
    interrupt: els.protectionModeSelect.value === "armed",
  });
  setConfigStatus(payload.ok ? "applied" : "failed", payload.ok ? "ok" : "error");
  if (payload.ok) {
    configDirty = false;
    syncPresetControls();
  }
}

function collectCalibrationSample(state) {
  if (!calibration.active) return;
  const force = state.sensor && state.sensor.force;
  if (Number.isFinite(force) && state.last_update_ms !== calibration.lastSampleMs) {
    calibration.samples.push(force);
    calibration.lastSampleMs = state.last_update_ms;
  }
  const remaining = Math.max(0, 5 - Math.floor((Date.now() - calibration.startedAt) / 1000));
  els.calibrationStatus.textContent =
    `Sampling unloaded force for calibration, about ${remaining}s remaining.`;
}

function finishCalibration() {
  calibration.active = false;
  calibration.timer = null;

  if (!calibration.samples.length) {
    els.calibrationStatus.textContent = "Calibration failed: no force samples received.";
    els.applyRecommendedBtn.disabled = true;
    return;
  }

  const sorted = calibration.samples.slice().sort((a, b) => a - b);
  const mid = Math.floor(sorted.length / 2);
  const median = sorted.length % 2 === 0
    ? Math.round((sorted[mid - 1] + sorted[mid]) / 2)
    : sorted[mid];
  const recommended = clamp(median + 600, 300, 3500);

  calibration.baseline = median;
  calibration.recommended = recommended;
  els.baselineValue.textContent = median;
  els.recommendedThresholdValue.textContent = recommended;
  els.calibrationStatus.textContent =
    `Baseline captured from ${calibration.samples.length} samples. Review before applying.`;
  els.applyRecommendedBtn.disabled = false;
}

function startCalibration() {
  if (calibration.timer) clearTimeout(calibration.timer);
  calibration = {
    active: true,
    samples: [],
    timer: setTimeout(finishCalibration, 5000),
    recommended: null,
    baseline: null,
    startedAt: Date.now(),
    lastSampleMs: null,
  };
  els.baselineValue.textContent = "--";
  els.recommendedThresholdValue.textContent = "--";
  els.applyRecommendedBtn.disabled = true;
  els.calibrationStatus.textContent = "Sampling unloaded force for calibration, about 5s remaining.";
  if (latestState) collectCalibrationSample(latestState);
}

function applyRecommendation() {
  if (calibration.recommended === null || calibration.recommended === undefined) return;
  els.thresholdInput.value = calibration.recommended;
  els.sensitivitySelect.value = "custom";
  markConfigDirty();
  setConfigStatus("recommendation ready");
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

function bindConfigEvents() {
  els.sensitivitySelect.addEventListener("change", () => {
    const value = SENSITIVITY_PRESETS[els.sensitivitySelect.value];
    if (value !== undefined) els.thresholdInput.value = value;
    markConfigDirty();
  });
  els.rateSelect.addEventListener("change", () => {
    const value = RATE_PRESETS[els.rateSelect.value];
    if (value !== undefined) els.samplingInput.value = value;
    markConfigDirty();
  });
  els.protectionModeSelect.addEventListener("change", markConfigDirty);
  els.thresholdInput.addEventListener("input", () => {
    els.sensitivitySelect.value = "custom";
    markConfigDirty();
  });
  els.samplingInput.addEventListener("input", () => {
    els.rateSelect.value = "custom";
    markConfigDirty();
  });
  els.configForm.addEventListener("submit", applyConfig);
  els.readConfigBtn.addEventListener("click", async () => {
    setConfigStatus("reading");
    const payload = await postJson("/api/command", { command: "READ_CONFIG" });
    setConfigStatus(payload.ok ? "read requested" : "failed", payload.ok ? "ok" : "error");
    if (payload.ok) configDirty = false;
  });
  els.calibrateBtn.addEventListener("click", startCalibration);
  els.applyRecommendedBtn.addEventListener("click", applyRecommendation);
}

async function init() {
  [
    "portSelect", "baudInput", "connectBtn", "disconnectBtn", "statusPill",
    "lastUpdate", "caseStatusCard", "caseStatusValue", "caseStatusReason",
    "protectionModeValue", "lastInterruptValue", "tamperStateValue",
    "tempValue", "humidityValue", "pressureValue", "gasValue", "gasStatus",
    "environmentStatus", "lightValue", "lightStatus", "forceValue", "thresholdValue",
    "trendTemp", "trendHumidity", "trendLight", "trendForce", "trendTempNow",
    "trendHumidityNow", "trendLightNow", "trendForceNow", "configForm",
    "sensitivitySelect", "rateSelect", "protectionModeSelect", "thresholdInput",
    "samplingInput", "applyConfigBtn", "readConfigBtn", "calibrateBtn",
    "applyRecommendedBtn", "baselineValue", "recommendedThresholdValue",
    "calibrationStatus", "configStatus", "alarmLog", "diagConnection",
    "diagLastCommand", "diagReadCounts", "diagEnvironment", "diagGas",
    "diagConfigSync", "debugPanel", "debugSummary", "rawLog", "clearLogBtn",
  ].forEach((id) => {
    els[id] = $(id);
  });

  await refreshPorts();
  const initialState = await fetch("/api/state").then((response) => response.json());
  updateState(initialState);
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
    updateDebugSummary();
    renderLog();
  });
  els.debugPanel.addEventListener("toggle", renderLog);
  updateDebugSummary();
  bindConfigEvents();
  if (initialState.connected) {
    postJson("/api/command", { command: "READ_CONFIG" });
  }

  setInterval(refreshPorts, 10000);
  setInterval(async () => {
    updateState(await fetch("/api/state").then((response) => response.json()));
  }, 2500);
  window.addEventListener("resize", renderTrends);
}

init().catch((error) => {
  if (els.rawLog) appendLine(`[gui-error] ${error.message}`);
});
