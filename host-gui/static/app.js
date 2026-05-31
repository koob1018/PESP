const els = {};
const rawLines = [];
const trendSamples = [];

const LIGHT_RAW_MAX = 65535;
const TAMPER_HOLD_MS = 15000;
const TREND_WINDOW_MS = 180000;
const MAX_RAW_LINES = 80;
const FORCE_BASELINE_IDLE_MAX = 200;
const FORCE_BASELINE_SAMPLE_COUNT = 8;
const RATE_PRESETS = { fast: 500, normal: 2000, slow: 5000 };
const METRIC_VALUE_IDS = [
  "tempValue",
  "humidityValue",
  "lightValue",
  "forceValue",
  "pressureValue",
  "gasValue",
];
const TREND_DOMAINS = {
  temperature: { min: 0, max: 50, unit: "C" },
  humidity: { min: 0, max: 100, unit: "%" },
  light: { min: 0, max: LIGHT_RAW_MAX, unit: "" },
  force: { min: 0, max: 4095, unit: "" },
};
const ALARM_LABELS = {
  FORCE: "Force Alert",
  LIGHT: "Light Alert",
  TEMP: "Temperature Alert",
  HUMIDITY: "Humidity Alert",
};

let configDirty = false;
let configSendTimer = null;
let lastTrendSampleMs = null;
let forceBaseline = null;
let lastForceBaselineSampleMs = null;
const forceBaselineSamples = [];

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

function medianRounded(values) {
  const sorted = values.slice().sort((a, b) => a - b);
  const mid = Math.floor(sorted.length / 2);
  if (sorted.length % 2 === 1) return sorted[mid];
  return Math.round((sorted[mid - 1] + sorted[mid]) / 2);
}

function updateForceBaseline(rawForce, timestamp) {
  if (!Number.isFinite(rawForce) || rawForce > FORCE_BASELINE_IDLE_MAX) return;
  if (timestamp && timestamp === lastForceBaselineSampleMs) return;
  if (forceBaselineSamples.length >= FORCE_BASELINE_SAMPLE_COUNT) return;

  lastForceBaselineSampleMs = timestamp || Date.now();
  forceBaselineSamples.push(rawForce);
  forceBaseline = medianRounded(forceBaselineSamples);
}

function displayForce(rawForce) {
  if (!Number.isFinite(rawForce)) return rawForce;
  return Math.max(0, rawForce - (forceBaseline || 0));
}

function displayForceThreshold(rawThreshold) {
  if (!Number.isFinite(rawThreshold)) return rawThreshold;
  return Math.max(0, rawThreshold - (forceBaseline || 0));
}

function setStatus(connected, status) {
  els.statusPill.textContent = status || (connected ? "Online" : "Offline");
  els.statusPill.classList.toggle("online", connected);
  els.statusPill.classList.toggle("offline", !connected);
}

function ageMs(timestamp) {
  if (!timestamp) return Infinity;
  return Date.now() - timestamp;
}

function isArmed(cfg) {
  return !cfg || (cfg.interrupt !== false && cfg.service !== true);
}

function addAlarm(alarms, source, label, detail) {
  if (!source || alarms.some((alarm) => alarm.source === source)) return;
  alarms.push({ source, label: label || ALARM_LABELS[source] || "Alarm Alert", detail });
}

function deriveActiveAlarms(state) {
  const sensor = state.sensor || {};
  const interrupt = state.interrupt || {};
  const cfg = (state.base && state.base.config) || {};
  const alarms = [];

  if (!isArmed(cfg)) return alarms;

  const recentInterrupt = ageMs(interrupt.last_interrupt_ms) <= TAMPER_HOLD_MS;
  if (sensor.event === true || recentInterrupt) {
    addAlarm(
      alarms,
      interrupt.alarm_source || "FORCE",
      interrupt.alarm_label || ALARM_LABELS.FORCE,
      interrupt.alarm_detail || interrupt.last_event_text || "Latched event",
    );
  }

  const forceThreshold = sensor.threshold ?? cfg.force_threshold;
  const force = displayForce(sensor.force);
  const threshold = displayForceThreshold(forceThreshold);
  if (Number.isFinite(force) && Number.isFinite(threshold) && force >= threshold) {
    addAlarm(alarms, "FORCE", ALARM_LABELS.FORCE, `Force ${force} >= ${threshold}`);
  }
  if (Number.isFinite(sensor.light) && Number.isFinite(cfg.light_high) &&
      sensor.light >= cfg.light_high) {
    addAlarm(alarms, "LIGHT", ALARM_LABELS.LIGHT, `Light ${sensor.light} >= ${cfg.light_high}`);
  }
  if (Number.isFinite(sensor.temperature_c) && Number.isFinite(cfg.temp_high) &&
      sensor.temperature_c >= cfg.temp_high) {
    addAlarm(
      alarms,
      "TEMP",
      ALARM_LABELS.TEMP,
      `Temperature ${Number(sensor.temperature_c).toFixed(2)} >= ${Number(cfg.temp_high).toFixed(2)}`,
    );
  }
  if (Number.isFinite(sensor.humidity_pct) && Number.isFinite(cfg.humidity_high) &&
      sensor.humidity_pct >= cfg.humidity_high) {
    addAlarm(
      alarms,
      "HUMIDITY",
      ALARM_LABELS.HUMIDITY,
      `Humidity ${Number(sensor.humidity_pct).toFixed(2)} >= ${Number(cfg.humidity_high).toFixed(2)}`,
    );
  }

  return alarms;
}

function deriveCaseStatus(state) {
  const sensor = state.sensor || {};
  const cfg = (state.base && state.base.config) || {};
  const armed = isArmed(cfg);
  const activeAlarms = deriveActiveAlarms(state);

  if (activeAlarms.length) {
    return {
      label: activeAlarms.length === 1 ? activeAlarms[0].label : "Multiple Alerts",
      className: "status-tamper",
      reason: activeAlarms.length === 1
        ? activeAlarms[0].detail
        : `${activeAlarms.length} active alarms`,
      alarms: activeAlarms,
    };
  }
  if (!state.connected) {
    return {
      label: "Attention",
      className: "status-attention",
      reason: "Serial offline",
      alarms: [],
    };
  }
  if (!state.last_update_ms || ageMs(state.last_update_ms) > 7000) {
    return {
      label: "Attention",
      className: "status-attention",
      reason: "Data stale",
      alarms: [],
    };
  }
  if (sensor.environment_ok === false) {
    return {
      label: "Attention",
      className: "status-attention",
      reason: "Env offline",
      alarms: [],
    };
  }
  if (!armed) {
    return {
      label: "Service",
      className: "status-service",
      reason: "Alarms muted",
      alarms: [],
    };
  }
  return {
    label: "Protected",
    className: "status-protected",
    reason: "Nominal",
    alarms: [],
  };
}

function updateCaseStatus(state) {
  const status = deriveCaseStatus(state);
  els.caseStatusValue.textContent = status.label;
  els.caseStatusReason.textContent = status.reason;
  els.caseStatusCard.classList.remove("status-protected", "status-attention", "status-tamper", "status-service");
  els.caseStatusCard.classList.add(status.className);

  const cfg = (state.base && state.base.config) || {};
  els.protectionModeValue.textContent = isArmed(cfg) ? "Armed" : "Service";
  els.lastInterruptValue.textContent = (state.interrupt && state.interrupt.last_interrupt_text) || "--";
  renderActiveAlarms(status.alarms || []);
}

function renderActiveAlarms(alarms) {
  els.activeAlarmList.innerHTML = "";
  if (!alarms.length) {
    els.activeAlarmList.hidden = true;
    return;
  }

  alarms.forEach((alarm) => {
    const item = document.createElement("div");
    item.className = "active-alarm";

    const label = document.createElement("strong");
    label.textContent = alarm.label;

    const detail = document.createElement("span");
    detail.textContent = alarm.detail || "";

    item.append(label, detail);
    els.activeAlarmList.appendChild(item);
  });
  els.activeAlarmList.hidden = false;
}

function updateMetrics(state) {
  const sensor = state.sensor || {};

  els.tempValue.textContent = fixed(sensor.temperature_c, 2, " C");
  els.humidityValue.textContent = fixed(sensor.humidity_pct, 2, "%");
  els.pressureValue.textContent = fixed(sensor.pressure_pa, 0, " Pa");
  els.gasValue.textContent = sensor.gas_status === "ok"
    ? text(sensor.gas_ohms, " ohm")
    : "--";
  els.environmentStatus.textContent = sensor.environment_ok
    ? "Env online"
    : "Env offline";

  els.lightValue.textContent = text(sensor.light);

  const force = displayForce(sensor.force);
  els.forceValue.textContent = text(force);
  els.trendTempNow.textContent = fixed(sensor.temperature_c, 1, " C");
  els.trendHumidityNow.textContent = fixed(sensor.humidity_pct, 1, "%");
  els.trendLightNow.textContent = text(sensor.light);
  els.trendForceNow.textContent = text(force);
  requestAnimationFrame(fitMetricValues);
}

function fitMetricValues() {
  METRIC_VALUE_IDS.forEach((id) => {
    const el = els[id];
    if (!el || !el.clientWidth) return;

    el.style.fontSize = "";
    let size = Number.parseFloat(getComputedStyle(el).fontSize);
    while (el.scrollWidth > el.clientWidth && size > 18) {
      size -= 1;
      el.style.fontSize = `${size}px`;
    }
  });
}

function selectPresetForValue(value, presets) {
  for (const [name, presetValue] of Object.entries(presets)) {
    if (Number(value) === presetValue) return name;
  }
  return null;
}

function syncRatePreset(value) {
  const preset = selectPresetForValue(value, RATE_PRESETS);
  if (preset) els.rateSelect.value = preset;
}

function updateConfigControls(cfg) {
  if (cfg.force_threshold !== null && cfg.force_threshold !== undefined &&
      document.activeElement !== els.thresholdInput) {
    els.thresholdInput.value = cfg.force_threshold;
  }
  if (cfg.sampling_ms !== null && cfg.sampling_ms !== undefined &&
      document.activeElement !== els.samplingInput) {
    els.samplingInput.value = cfg.sampling_ms;
    syncRatePreset(cfg.sampling_ms);
  }
  if (cfg.light_high !== null && cfg.light_high !== undefined &&
      document.activeElement !== els.lightHighInput) {
    els.lightHighInput.value = cfg.light_high;
  }
  if (cfg.temp_high !== null && cfg.temp_high !== undefined &&
      document.activeElement !== els.tempHighInput) {
    els.tempHighInput.value = Number(cfg.temp_high).toFixed(2);
  }
  if (cfg.humidity_high !== null && cfg.humidity_high !== undefined &&
      document.activeElement !== els.humidityHighInput) {
    els.humidityHighInput.value = Number(cfg.humidity_high).toFixed(2);
  }
  if ((cfg.interrupt !== null && cfg.interrupt !== undefined ||
       cfg.service !== null && cfg.service !== undefined) &&
      document.activeElement !== els.protectionModeSelect) {
    els.protectionModeSelect.value = isArmed(cfg) ? "armed" : "service";
  }
}

function renderAlarmLog(events) {
  const list = Array.isArray(events) ? events : [];
  if (!list.length) {
    els.alarmLog.innerHTML = '<p class="empty-log">No alarms</p>';
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
    force: displayForce(sensor.force),
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
  updateForceBaseline(state.sensor && state.sensor.force, state.last_update_ms);
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
    ? `Updated ${state.last_update_text}`
    : "No data";

  const cfg = (state.base && state.base.config) || {};
  updateCaseStatus(state);
  updateMetrics(state);
  updateConfigControls(cfg);
  renderAlarmLog(state.interrupt && state.interrupt.events);
  addTrendSample(state);

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
  els.debugSummary.textContent = `${rawLines.length} ${lineWord}`;
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

function clearPendingConfigSend() {
  if (configSendTimer) {
    clearTimeout(configSendTimer);
    configSendTimer = null;
  }
}

function syncPresetControls() {
  const sampling = Number(els.samplingInput.value);
  syncRatePreset(sampling);
}

async function sendConfigPatch(payload, okText = "saved") {
  clearPendingConfigSend();
  setConfigStatus("saving");
  const response = await postJson("/api/config", payload);
  setConfigStatus(response.ok ? okText : "failed", response.ok ? "ok" : "error");
  if (response.ok) {
    configDirty = false;
    syncPresetControls();
  }
  return response;
}

function sendIntegerField(input, key, min, max, label, delayMs = 450) {
  const value = Number(input.value);
  markConfigDirty();
  if (!Number.isInteger(value) || value < min || value > max) {
    setConfigStatus(`${label} ${min}-${max}`, "error");
    return;
  }
  clearPendingConfigSend();
  configSendTimer = setTimeout(() => {
    sendConfigPatch({ [key]: value });
  }, delayMs);
}

function sendFloatField(input, key, min, max, label, delayMs = 450) {
  const value = Number(input.value);
  markConfigDirty();
  if (!Number.isFinite(value) || value < min || value > max) {
    setConfigStatus(`${label} ${min}-${max}`, "error");
    return;
  }
  clearPendingConfigSend();
  configSendTimer = setTimeout(() => {
    sendConfigPatch({ [key]: value });
  }, delayMs);
}

async function applyProtectionMode() {
  const armed = els.protectionModeSelect.value === "armed";

  els.protectionModeSelect.disabled = true;
  setConfigStatus(armed ? "arming" : "service");
  try {
    await sendConfigPatch({ interrupt: armed }, armed ? "armed" : "service");
  } finally {
    els.protectionModeSelect.disabled = false;
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

function bindConfigEvents() {
  els.rateSelect.addEventListener("change", () => {
    const value = RATE_PRESETS[els.rateSelect.value];
    if (value !== undefined) els.samplingInput.value = value;
    if (value !== undefined) sendConfigPatch({ sampling_ms: value });
  });
  els.protectionModeSelect.addEventListener("change", applyProtectionMode);
  els.thresholdInput.addEventListener("input", () => {
    sendIntegerField(els.thresholdInput, "force_threshold", 0, 4095, "fsr");
  });
  els.samplingInput.addEventListener("input", () => {
    sendIntegerField(els.samplingInput, "sampling_ms", 100, 60000, "sampling rate");
  });
  els.lightHighInput.addEventListener("input", () => {
    sendIntegerField(els.lightHighInput, "light_high", 0, LIGHT_RAW_MAX, "light");
  });
  els.tempHighInput.addEventListener("input", () => {
    sendFloatField(els.tempHighInput, "temp_high", -40, 85, "temp");
  });
  els.humidityHighInput.addEventListener("input", () => {
    sendFloatField(els.humidityHighInput, "humidity_high", 0, 100, "humidity");
  });
  els.configForm.addEventListener("submit", (event) => event.preventDefault());
  els.readConfigBtn.addEventListener("click", async () => {
    setConfigStatus("reading config");
    const payload = await postJson("/api/command", { command: "READ_CONFIG" });
    setConfigStatus(payload.ok ? "config synced" : "sync failed", payload.ok ? "ok" : "error");
    if (payload.ok) configDirty = false;
  });
}

async function init() {
  [
    "portSelect", "baudInput", "connectBtn", "disconnectBtn", "statusPill",
    "lastUpdate", "caseStatusCard", "caseStatusValue", "caseStatusReason",
    "activeAlarmList", "protectionModeValue", "lastInterruptValue",
    "tempValue", "humidityValue", "pressureValue", "gasValue",
    "environmentStatus", "lightValue", "forceValue",
    "trendTemp", "trendHumidity", "trendLight", "trendForce", "trendTempNow",
    "trendHumidityNow", "trendLightNow", "trendForceNow", "configForm",
    "rateSelect", "protectionModeSelect", "thresholdInput",
    "samplingInput", "lightHighInput", "tempHighInput", "humidityHighInput",
    "readConfigBtn", "configStatus", "alarmLog", "debugPanel", "debugSummary",
    "rawLog", "clearLogBtn",
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

  setInterval(() => {
    refreshPorts().catch(() => setStatus(false, "Offline"));
  }, 10000);
  setInterval(async () => {
    try {
      updateState(await fetch("/api/state").then((response) => response.json()));
    } catch (_error) {
      setStatus(false, "Offline");
    }
  }, 2500);
  window.addEventListener("resize", () => {
    renderTrends();
    requestAnimationFrame(fitMetricValues);
  });
}

init().catch((error) => {
  if (els.rawLog) appendLine(`[gui-error] ${error.message}`);
});
