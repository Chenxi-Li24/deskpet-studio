// DeskPet ESP32-S3 — Minimal Test: WiFi + BLE Provisioning + LED + Web Server
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <WiFi.h>
#include "hw/pins.h"
#include "hw/input.h"
#include "wifi_manager.h"
#include "web_server.h"

// ── Stubs for excluded modules ─────────────────────────────────────────
// ble_bridge (NUS BLE) — not used in this test, use custom BLE instead
int  bleAvailable()           { return 0; }
int  bleRead()                { return -1; }
void bleWrite(const uint8_t*, int) {}
bool bleConnected()           { return false; }
bool bleSecure()              { return false; }
void bleClearBonds()          {}

// buddy / character — excluded, web_server references buddySetSpeciesIdx
void buddyInit()              {}
void buddyTick(uint8_t)       {}
void buddySetSpeciesIdx(uint8_t) {}
bool characterInit(const char*) { return false; }
void characterTick()          {}
void characterClose()         {}

// stats.h external refs
bool buddyMode    = true;
bool gifAvailable = false;
uint32_t _clkLastRead = 0;

// ── BLE UUIDs (custom, same as verified test) ──────────────────────────
#define SERVICE_UUID  "12345678-1234-1234-1234-123456789abc"
#define CHAR_INFO     "abcd0001-1234-1234-1234-123456789abc"
#define CHAR_WIFI_CFG "abcd0002-1234-1234-1234-123456789abc"

// ── LED ────────────────────────────────────────────────────────────────
static Adafruit_NeoPixel s_led(WS2812_NUM, PIN_WS2812, NEO_RGB + NEO_KHZ800);

static void ledSet(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness = 32) {
  s_led.setBrightness(brightness);
  s_led.fill(s_led.Color(r, g, b));
  s_led.show();
}

static void ledBreath(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t now = millis();
  float phase = (now % 2000) / 2000.0f;
  uint8_t bri = 4 + (uint8_t)((sin(phase * 2 * PI) + 1) * 10);
  ledSet(r, g, b, bri);
}

// ── BLE callbacks ──────────────────────────────────────────────────────
static bool s_bleConnected = false;

class ConnCallback : public BLEServerCallbacks {
  void onConnect(BLEServer*)    { s_bleConnected = true;  Serial.println("BLE: connected"); }
  void onDisconnect(BLEServer*) { s_bleConnected = false; Serial.println("BLE: disconnected"); }
};

class WifiCfgCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* chr) {
    std::string val = chr->getValue();
    if (val.empty()) return;

    // Parse {"ssid":"...","pass":"..."}
    const char* s = strstr(val.c_str(), "\"ssid\"");
    const char* p = strstr(val.c_str(), "\"pass\"");
    if (!s || !p) { Serial.println("WiFi: bad JSON"); return; }

    char ssid[33] = {0}, pass[65] = {0};

    s = strchr(s + 6, '"'); if (!s) return;
    s++;
    const char* e = strchr(s, '"'); if (!e) return;
    size_t len = e - s; if (len >= 32) len = 31;
    memcpy(ssid, s, len);

    p = strchr(p + 6, '"'); if (!p) return;
    p++;
    e = strchr(p, '"'); if (!e) return;
    len = e - p; if (len >= 64) len = 63;
    memcpy(pass, p, len);

    Serial.printf("WiFi: provisioning '%s' via BLE\n", ssid);
    wifiMgrConnect(ssid, pass);
  }
};

// ── Setup ──────────────────────────────────────────────────────────────
void setup() {
  // 1. Serial
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== DeskPet MinTest: WiFi+BLE+Web ===");

  // 2. LED init
  s_led.begin();
  ledSet(255, 0, 0, 32);  // Red = booting

  // 3. BLE custom service
  BLEDevice::init("DeskPet-Mini");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ConnCallback());

  BLEService* svc = server->createService(SERVICE_UUID);
  BLECharacteristic* info = svc->createCharacteristic(
    CHAR_INFO, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  info->addDescriptor(new BLE2902());
  info->setValue("{\"ssid\":\"...\",\"pass\":\"...\"}");

  BLECharacteristic* cfg = svc->createCharacteristic(
    CHAR_WIFI_CFG, BLECharacteristic::PROPERTY_WRITE);
  cfg->setCallbacks(new WifiCfgCallback());

  svc->start();
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();
  Serial.println("BLE: advertising 'DeskPet-Mini'");

  // 4. WiFi manager (auto-connect if NVS has creds)
  wifiMgrInit();

  // 5. Web server (async — serves REST API when WiFi connects)
  webServerInit();

  // 6. Input — EC11 encoder
  hwInputInit();

  // 7. Ready
  Serial.println("=== Ready ===");
  ledSet(0, 0, 255, 32);  // Blue = waiting for WiFi
}

// ── Loop ───────────────────────────────────────────────────────────────
void loop() {
  static uint32_t lastLed = 0;
  uint32_t now = millis();

  // 1. WiFi state machine
  wifiMgrTick();

  // 2. Input — poll encoder
  hwInputUpdate();

  // 3. LED update + Encoder print (500ms interval)
  if (now - lastLed > 500) {
    lastLed = now;

    // --- Encoder debug ---
    HwEnc enc = hwEnc1();
    if (enc.delta != 0 || enc.justPressed || enc.pressed) {
      Serial.printf("ENC1: delta=%+d btn=%d\n", enc.delta, enc.pressed);
    }

    // --- LED ---
    const char* cs = webClawdState();
    WifiMgrState st = wifiMgrState();
    if (st != WM_OK) {
      switch (st) {
        case WM_IDLE:         ledBreath(0, 0, 255);   break; // Blue breath = waiting
        case WM_AUTO_CONNECT:
        case WM_CONNECTING:   ledSet(255, 255, 0, 32); break; // Yellow = connecting
        case WM_FAIL:         ledBreath(255, 0, 0);    break; // Red breath = fail
      }
    } else if (strcmp(cs, "thinking") == 0) {
      ledBreath(255, 165, 0);    // Orange breath = thinking
    } else if (strcmp(cs, "working") == 0) {
      ledBreath(0, 255, 0);      // Green breath = working
    } else if (strcmp(cs, "juggling") == 0) {
      ledBreath(0, 255, 255);    // Cyan breath = juggling
    } else {
      ledBreath(0, 0, 255);      // Blue breath = idle
    }
    // Log state changes
    static WifiMgrState lastSt = WM_IDLE;
    if (st != lastSt) {
      if (st == WM_OK) webLog("WiFi connected: %s RSSI=%d", wifiMgrLocalIP().c_str(), wifiMgrRssi());
      if (st == WM_FAIL) webLog("WiFi connect failed");
      lastSt = st;
    }
  }

  delay(30);
}
