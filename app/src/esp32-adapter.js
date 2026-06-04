"use strict";
const http = require("http");
const fs = require("fs");
const path = require("path");

const ESP32_HOST = "192.168.31.158";
const ESP32_PORT = 80;
const POLL_MS = 200;
const LOG_FILE = path.join(require("os").tmpdir(), "clawd-esp32-debug.log");

function debugLog(msg) {
  const line = `[${new Date().toISOString()}] ${msg}\n`;
  try { fs.appendFileSync(LOG_FILE, line); } catch (_) {}
  console.log(msg);
}

function createEsp32Adapter(deps = {}) {
  const {
    getPendingPermissions = () => [],
    resolvePermissionEntry,
    themeRuntime,
    settingsController,
    log = () => {},
  } = deps;

  let pollTimer = null;
  let running = false;
  let lastEncDelta = 0;
  let lastBtnPressed = false;
  let lastClawdState = "idle";

  const availableThemes = ["clawd", "calico", "cloudling"];
  let currentThemeIdx = 0;

  function espGet(path) {
    return new Promise((resolve, reject) => {
      const req = http.request({
        hostname: ESP32_HOST, port: ESP32_PORT, path,
        method: "GET", timeout: 3000,
      }, (res) => {
        let data = "";
        res.on("data", (c) => (data += c));
        res.on("end", () => resolve(data));
      });
      req.on("error", (err) => reject(err));
      req.on("timeout", () => { req.destroy(); reject(new Error("timeout")); });
      req.end();
    });
  }

  function espPost(path, body) {
    return new Promise((resolve, reject) => {
      const payload = JSON.stringify(body);
      const req = http.request({
        hostname: ESP32_HOST, port: ESP32_PORT, path,
        method: "POST", timeout: 3000,
        headers: { "Content-Type": "application/json", "Content-Length": Buffer.byteLength(payload) },
      }, (res) => { res.resume(); res.on("end", resolve); });
      req.on("error", (err) => reject(err));
      req.on("timeout", () => { req.destroy(); reject(new Error("timeout")); });
      req.write(payload);
      req.end();
    });
  }

  async function poll() {
    try {
      const raw = await espGet("/api/status");
      const status = JSON.parse(raw);
      const enc = status.enc1 || { delta: 0, pressed: false };

      // --- Encoder rotation → theme switch (dead zone: |d| >= 3) ---
      if (enc.delta !== lastEncDelta) {
        const d = enc.delta - lastEncDelta;
        if (Math.abs(d) >= 30) { // 最小刻度30
          if (d > 0) {
            currentThemeIdx = (currentThemeIdx + 1) % availableThemes.length;
          } else {
            currentThemeIdx = (currentThemeIdx - 1 + availableThemes.length) % availableThemes.length;
          }
          if (themeRuntime && themeRuntime.activateTheme) {
            themeRuntime.activateTheme(availableThemes[currentThemeIdx], "default", null);
            debugLog(`Theme → ${availableThemes[currentThemeIdx]} (d=${d})`);
          }
        }
        lastEncDelta = enc.delta;
      }

      // --- Button press → approve permission ---
      if (enc.pressed && !lastBtnPressed) {
        const pending = getPendingPermissions();
        debugLog(`BTN pressed! pending count=${pending ? pending.length : 0}`);
        if (pending && pending.length > 0) {
          const entry = pending[0];
          debugLog(`BTN approving: agentId=${entry.agentId} tool=${entry.toolName} id=${entry.id}`);
          try {
            if (resolvePermissionEntry) {
              resolvePermissionEntry(entry, "once");
              debugLog(`BTN resolvePermissionEntry called OK`);
            } else {
              debugLog(`BTN ERROR: resolvePermissionEntry is null`);
            }
          } catch(e) {
            debugLog(`BTN ERROR: ${e.message}`);
          }
        }
      }
      lastBtnPressed = enc.pressed;

    } catch (_err) {
      debugLog(`Poll error: ${_err.message}`);
    }
  }

  async function pushState(color) {
    try {
      await espPost("/api/led", { color });
    } catch (_err) { /* ignore */ }
  }

  function onClawdStateChange(newState) {
    if (newState === lastClawdState) return;
    lastClawdState = newState;
    switch (newState) {
      case "thinking": pushState("thinking"); break;
      case "working":
      case "juggling":  pushState("working"); break;
      case "sweeping":
      case "sleeping":  pushState("idle"); break;
      default:          pushState("idle"); break;
    }
  }

  function start() {
    if (running) return;
    running = true;
    pollTimer = setInterval(poll, POLL_MS);
    log(`[esp32] polling ${ESP32_HOST}:${ESP32_PORT} every ${POLL_MS}ms`);
  }

  function stop() {
    running = false;
    if (pollTimer) { clearInterval(pollTimer); pollTimer = null; }
  }

  return { start, stop, poll, onClawdStateChange, espGet, espPost };
}

module.exports = { createEsp32Adapter };
