/**
 * ESP32-C3 + MAX30102 — Live Web Monitor
 *
 * Opens a WiFi access point (or joins your network), serves a
 * real-time heart-rate + temperature dashboard over WebSocket.
 *
 * Libraries (install via Arduino Library Manager):
 *   1. SparkFun MAX3010x Pulse and Proximity Sensor Library
 *   2. ESPAsyncWebServer  (by Me-No-Dev)
 *   3. AsyncTCP           (by Me-No-Dev)
 *   4. ArduinoJson        (by Benoit Blanchon)
 *
 * Wiring:
 *   MAX30102 VIN → 3.3V | GND → GND | SDA → GPIO8 | SCL → GPIO9
 */

#include <Wire.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "MAX30105.h"
#include "heartRate.h"

// ── WiFi credentials ─────────────────────────────────────────────────────────
// Option A: Join your existing network
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// Option B: ESP32 creates its own Access Point (uncomment to use)
// #define USE_AP_MODE
// const char* AP_SSID     = "HeartMonitor";
// const char* AP_PASSWORD = "12345678";

// ── I2C + sensor config ──────────────────────────────────────────────────────
#define SDA_PIN 8
#define SCL_PIN 9
const long  IR_FINGER_THRESHOLD = 50000;
const byte  RATE_SIZE           = 4;

// ── Globals ──────────────────────────────────────────────────────────────────
MAX30105        particleSensor;
AsyncWebServer  server(80);
AsyncWebSocket  ws("/ws");

byte  rateBuffer[RATE_SIZE] = {0};
byte  rateIndex   = 0;
long  lastBeat    = 0;
float bpm         = 0;
int   bpmAvg      = 0;
float tempC       = 0;
bool  fingerOn    = false;

unsigned long lastSampleMs = 0;
unsigned long lastPushMs   = 0;
unsigned long lastTempMs   = 0;

// ── Embedded dashboard HTML ──────────────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Heart Monitor</title>
<link href="https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@300;500;700&display=swap" rel="stylesheet">
<style>
  :root {
    --bg:      #07090f;
    --panel:   #0d1117;
    --border:  #1a2535;
    --red:     #ff2d55;
    --red-dim: #7a1525;
    --teal:    #00e5c8;
    --amber:   #ffaa00;
    --text:    #c8d8e8;
    --muted:   #445566;
    --mono:    'Share Tech Mono', monospace;
    --sans:    'Rajdhani', sans-serif;
  }
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: var(--sans);
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 24px 16px 40px;
    gap: 20px;
  }

  /* scanline overlay */
  body::before {
    content: '';
    position: fixed; inset: 0;
    background: repeating-linear-gradient(
      0deg,
      transparent,
      transparent 2px,
      rgba(0,0,0,.18) 2px,
      rgba(0,0,0,.18) 4px
    );
    pointer-events: none;
    z-index: 100;
  }

  header {
    width: 100%; max-width: 860px;
    display: flex; align-items: center; justify-content: space-between;
    border-bottom: 1px solid var(--border);
    padding-bottom: 12px;
  }
  header h1 {
    font-family: var(--mono);
    font-size: 1rem;
    letter-spacing: .25em;
    color: var(--teal);
    text-transform: uppercase;
  }
  #status-dot {
    width: 10px; height: 10px;
    border-radius: 50%;
    background: var(--muted);
    box-shadow: none;
    transition: background .4s, box-shadow .4s;
  }
  #status-dot.connected {
    background: var(--teal);
    box-shadow: 0 0 10px var(--teal);
    animation: blink 2s infinite;
  }
  #status-dot.finger {
    background: var(--red);
    box-shadow: 0 0 14px var(--red);
  }
  @keyframes blink { 0%,100%{opacity:1} 50%{opacity:.4} }

  .grid {
    width: 100%; max-width: 860px;
    display: grid;
    grid-template-columns: 1fr 1fr;
    grid-template-rows: auto auto;
    gap: 16px;
  }

  .card {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 20px 24px;
    position: relative;
    overflow: hidden;
  }
  .card::after {
    content: '';
    position: absolute; top: 0; left: 0; right: 0;
    height: 2px;
    background: var(--border);
  }
  .card.accent-red::after  { background: var(--red); box-shadow: 0 0 8px var(--red); }
  .card.accent-teal::after { background: var(--teal); box-shadow: 0 0 8px var(--teal); }
  .card.accent-amber::after{ background: var(--amber); box-shadow: 0 0 8px var(--amber); }

  .card-label {
    font-family: var(--mono);
    font-size: .65rem;
    letter-spacing: .2em;
    color: var(--muted);
    text-transform: uppercase;
    margin-bottom: 10px;
  }

  /* ── BPM card ── */
  #bpm-card { grid-column: 1; grid-row: 1; }
  .bpm-row { display: flex; align-items: flex-end; gap: 10px; }
  #bpm-value {
    font-family: var(--mono);
    font-size: 5rem;
    line-height: 1;
    color: var(--red);
    text-shadow: 0 0 24px rgba(255,45,85,.6);
    transition: color .3s;
    min-width: 3ch;
  }
  #bpm-value.dim { color: var(--red-dim); text-shadow: none; }
  #bpm-value.pulse { animation: heartpulse .25s ease-out; }
  @keyframes heartpulse {
    0%  { transform: scale(1); }
    40% { transform: scale(1.08); }
    100%{ transform: scale(1); }
  }
  .bpm-unit { font-size: 1.2rem; color: var(--muted); margin-bottom: .6rem; }

  #avg-bpm {
    font-family: var(--mono);
    font-size: .85rem;
    color: var(--muted);
    margin-top: 6px;
  }
  #avg-bpm span { color: var(--text); }

  /* ── Temp card ── */
  #temp-card { grid-column: 2; grid-row: 1; }
  #temp-value {
    font-family: var(--mono);
    font-size: 3.8rem;
    color: var(--amber);
    text-shadow: 0 0 20px rgba(255,170,0,.5);
    line-height: 1;
    transition: color .3s;
  }
  #temp-value.dim { color: #554422; text-shadow: none; }
  .temp-unit { font-size: 1.2rem; color: var(--muted); }

  /* ── ECG chart card ── */
  #chart-card { grid-column: 1 / -1; grid-row: 2; }
  #ecg-canvas {
    width: 100%; height: 110px;
    display: block;
  }

  /* ── Finger status bar ── */
  #finger-bar {
    width: 100%; max-width: 860px;
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 10px 24px;
    font-family: var(--mono);
    font-size: .75rem;
    color: var(--muted);
    letter-spacing: .1em;
    display: flex;
    align-items: center;
    gap: 10px;
  }
  #finger-icon { font-size: 1.2rem; }
  #finger-text { flex: 1; }

  @media(max-width:520px){
    .grid { grid-template-columns: 1fr; }
    #temp-card  { grid-column: 1; grid-row: 2; }
    #chart-card { grid-column: 1; grid-row: 3; }
    #bpm-value  { font-size: 4rem; }
    #temp-value { font-size: 3rem; }
  }
</style>
</head>
<body>

<header>
  <h1>&#9829; VITAL MONITOR / ESP32-C3</h1>
  <div id="status-dot" title="WebSocket status"></div>
</header>

<div class="grid">
  <div class="card accent-red" id="bpm-card">
    <div class="card-label">Heart Rate</div>
    <div class="bpm-row">
      <div id="bpm-value" class="dim">--</div>
      <div class="bpm-unit">BPM</div>
    </div>
    <div id="avg-bpm">4-beat avg: <span id="avg-val">--</span> BPM</div>
  </div>

  <div class="card accent-amber" id="temp-card">
    <div class="card-label">Die Temperature</div>
    <div class="bpm-row">
      <div id="temp-value" class="dim">--.-</div>
      <div class="temp-unit">&deg;C</div>
    </div>
  </div>

  <div class="card accent-teal" id="chart-card">
    <div class="card-label">BPM History (last 60 s)</div>
    <canvas id="ecg-canvas"></canvas>
  </div>
</div>

<div id="finger-bar">
  <span id="finger-icon">&#128077;</span>
  <span id="finger-text">Waiting for sensor data...</span>
</div>

<script>
  // ── Canvas chart ──────────────────────────────────────────────────────────
  const canvas  = document.getElementById('ecg-canvas');
  const ctx     = canvas.getContext('2d');
  const HISTORY = 60;
  const bpmHist = new Array(HISTORY).fill(null);

  function resizeCanvas() {
    canvas.width  = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
  }
  resizeCanvas();
  window.addEventListener('resize', resizeCanvas);

  function drawChart() {
    const W = canvas.width, H = canvas.height;
    ctx.clearRect(0, 0, W, H);

    // Grid lines
    ctx.strokeStyle = 'rgba(26,37,53,.8)';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 4; i++) {
      const y = H * i / 4;
      ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(W, y); ctx.stroke();
    }

    const valid = bpmHist.filter(v => v !== null);
    if (valid.length < 2) return;

    const minV = Math.max(40,  Math.min(...valid) - 5);
    const maxV = Math.min(200, Math.max(...valid) + 5);
    const range = maxV - minV || 1;

    const toY = v => H - ((v - minV) / range) * H * .85 - H * .05;
    const toX = i => (i / (HISTORY - 1)) * W;

    // Gradient fill
    const grad = ctx.createLinearGradient(0, 0, 0, H);
    grad.addColorStop(0, 'rgba(0,229,200,.25)');
    grad.addColorStop(1, 'rgba(0,229,200,0)');

    ctx.beginPath();
    let first = true;
    bpmHist.forEach((v, i) => {
      if (v === null) { first = true; return; }
      if (first) { ctx.moveTo(toX(i), toY(v)); first = false; }
      else        ctx.lineTo(toX(i), toY(v));
    });
    ctx.lineTo(W, H); ctx.lineTo(0, H); ctx.closePath();
    ctx.fillStyle = grad;
    ctx.fill();

    // Line
    ctx.beginPath();
    first = true;
    bpmHist.forEach((v, i) => {
      if (v === null) { first = true; return; }
      if (first) { ctx.moveTo(toX(i), toY(v)); first = false; }
      else        ctx.lineTo(toX(i), toY(v));
    });
    ctx.strokeStyle = '#00e5c8';
    ctx.lineWidth = 2;
    ctx.shadowColor = '#00e5c8';
    ctx.shadowBlur  = 6;
    ctx.stroke();
    ctx.shadowBlur = 0;

    // Latest dot
    const lastIdx = bpmHist.map((v,i) => v!==null ? i : -1).reduce((a,b) => Math.max(a,b), -1);
    if (lastIdx >= 0) {
      const lv = bpmHist[lastIdx];
      ctx.beginPath();
      ctx.arc(toX(lastIdx), toY(lv), 4, 0, Math.PI * 2);
      ctx.fillStyle = '#00e5c8';
      ctx.shadowColor = '#00e5c8';
      ctx.shadowBlur = 10;
      ctx.fill();
      ctx.shadowBlur = 0;
    }
  }

  setInterval(drawChart, 500);

  // ── WebSocket ─────────────────────────────────────────────────────────────
  const dot       = document.getElementById('status-dot');
  const bpmEl     = document.getElementById('bpm-value');
  const avgEl     = document.getElementById('avg-val');
  const tempEl    = document.getElementById('temp-value');
  const fingerTxt = document.getElementById('finger-text');
  const fingerIco = document.getElementById('finger-icon');

  let ws, reconnectTimer;

  function connect() {
    ws = new WebSocket('ws://' + location.host + '/ws');

    ws.onopen = () => {
      dot.className = 'connected';
      fingerTxt.textContent = 'Connected — place finger on sensor.';
    };

    ws.onclose = () => {
      dot.className = '';
      fingerTxt.textContent = 'Disconnected — retrying in 3 s…';
      clearTimeout(reconnectTimer);
      reconnectTimer = setTimeout(connect, 3000);
    };

    ws.onmessage = e => {
      let d;
      try { d = JSON.parse(e.data); } catch { return; }

      if (!d.finger) {
        bpmEl.textContent = '--';  bpmEl.className = 'dim';
        avgEl.textContent = '--';
        tempEl.textContent = '--.-'; tempEl.className = 'dim';
        fingerTxt.textContent = 'No finger detected.';
        fingerIco.textContent = '✋';
        dot.className = 'connected';
        bpmHist.push(null);
        if (bpmHist.length > HISTORY) bpmHist.shift();
        return;
      }

      dot.className = 'finger';

      // BPM
      if (d.avg > 0) {
        bpmEl.textContent = d.bpm > 0 ? Math.round(d.bpm) : '--';
        bpmEl.className   = '';
        bpmEl.classList.add('pulse');
        setTimeout(() => bpmEl.classList.remove('pulse'), 300);
        avgEl.textContent = d.avg;
        bpmHist.push(d.avg);
      } else {
        bpmEl.textContent = '--'; bpmEl.className = 'dim';
        avgEl.textContent = '--';
        bpmHist.push(null);
      }
      if (bpmHist.length > HISTORY) bpmHist.shift();

      // Temperature
      if (d.temp > 0) {
        tempEl.textContent = d.temp.toFixed(2);
        tempEl.className   = '';
      }

      fingerTxt.textContent = 'Finger detected — measuring…';
      fingerIco.textContent = '☝️';
    };
  }

  connect();
</script>
</body>
</html>
)rawhtml";

// ── WebSocket event handler ──────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT)
    Serial.printf("[WS] Client #%u connected from %s\n",
                  client->id(), client->remoteIP().toString().c_str());
  else if (type == WS_EVT_DISCONNECT)
    Serial.printf("[WS] Client #%u disconnected\n", client->id());
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32-C3 Heart Web Monitor ===");

  // I2C + MAX30102
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("[ERROR] MAX30102 not found! Check wiring.");
    while (true) delay(1000);
  }
  particleSensor.setup(60, 4, 2, 400, 215, 16384);
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);
  tempC = particleSensor.readTemperature();
  Serial.println("[OK] MAX30102 ready.");

  // WiFi
#ifdef USE_AP_MODE
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("[WiFi] AP started  →  http://%s\n",
                WiFi.softAPIP().toString().c_str());
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
  Serial.printf("\n[WiFi] Connected  →  http://%s\n",
                WiFi.localIP().toString().c_str());
#endif

  // Web server – serve dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", INDEX_HTML);
  });

  // WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("[HTTP] Server started.");
}

// ── Loop ─────────────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Sample at ~50 Hz
  if (now - lastSampleMs >= 20) {
    lastSampleMs = now;
    long ir = particleSensor.getIR();
    fingerOn = (ir > IR_FINGER_THRESHOLD);

    if (fingerOn && checkForBeat(ir)) {
      long delta = now - lastBeat;
      lastBeat = now;
      float candidate = 60.0f / (delta / 1000.0f);
      if (candidate >= 20 && candidate <= 255) {
        bpm = candidate;
        rateBuffer[rateIndex++ % RATE_SIZE] = (byte)bpm;
        long s = 0;
        for (byte i = 0; i < RATE_SIZE; i++) s += rateBuffer[i];
        bpmAvg = s / RATE_SIZE;
      }
    }
    if (!fingerOn) {
      bpm = 0; bpmAvg = 0; lastBeat = 0;
      for (byte i = 0; i < RATE_SIZE; i++) rateBuffer[i] = 0;
      checkForBeat(0);
    }
  }

  // Temperature every 5 s
  if (now - lastTempMs >= 5000) {
    lastTempMs = now;
    tempC = particleSensor.readTemperature();
  }

  // Push JSON to all WS clients every second
  if (now - lastPushMs >= 1000) {
    lastPushMs = now;
    ws.cleanupClients();

    StaticJsonDocument<128> doc;
    doc["finger"] = fingerOn;
    doc["bpm"]    = (int)bpm;
    doc["avg"]    = bpmAvg;
    doc["temp"]   = serialized(String(tempC, 2));

    String out;
    serializeJson(doc, out);
    ws.textAll(out);

    Serial.printf("BPM: %d | Avg: %d | Temp: %.2f°C | Finger: %s\n",
                  (int)bpm, bpmAvg, tempC, fingerOn ? "YES" : "NO");
  }
}