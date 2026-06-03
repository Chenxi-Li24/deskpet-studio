// DeskPet ESP32-S3 - BLE WiFi Provisioning Test
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <WiFi.h>
#include "hw/pins.h"

// 板载 WS2812 LED (GPIO21)
static Adafruit_NeoPixel s_led(WS2812_NUM, PIN_WS2812, NEO_RGB + NEO_KHZ800);

// BLE 状态
static bool s_bleConnected = false;

// WiFi 状态
enum WifiState { WF_IDLE, WF_CONNECTING, WF_OK, WF_FAIL };
static WifiState s_wifiState = WF_IDLE;
static uint32_t  s_wifiFailTime = 0;

// BLE UUIDs
#define SERVICE_UUID  "12345678-1234-1234-1234-123456789abc"
#define CHAR_INFO     "abcd0001-1234-1234-1234-123456789abc"
#define CHAR_WIFI_CFG "abcd0002-1234-1234-1234-123456789abc"

// ── 简单 JSON 解析 (不用 ArduinoJson, 省依赖) ──
static bool parseWifiJson(const char* json, char* ssid, char* pass, size_t max) {
  const char* s = strstr(json, "\"ssid\"");
  const char* p = strstr(json, "\"pass\"");
  if (!s || !p) return false;

  // 提取 ssid
  s = strchr(s + 6, '"'); if (!s) return false;
  s++;
  const char* e = strchr(s, '"'); if (!e) return false;
  size_t len = e - s;
  if (len >= max) len = max - 1;
  memcpy(ssid, s, len); ssid[len] = 0;

  // 提取 pass
  p = strchr(p + 6, '"'); if (!p) return false;
  p++;
  e = strchr(p, '"'); if (!e) return false;
  len = e - p;
  if (len >= max) len = max - 1;
  memcpy(pass, p, len); pass[len] = 0;

  return true;
}

// ── BLE 连接回调 ──
class ConnCallback : public BLEServerCallbacks {
  void onConnect(BLEServer*) {
    s_bleConnected = true;
    Serial.println("BLE: connected");
  }
  void onDisconnect(BLEServer*) {
    s_bleConnected = false;
    Serial.println("BLE: disconnected");
  }
};

// ── WiFi 配网 Characteristic 回调 ──
class WifiCfgCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* chr) {
    std::string val = chr->getValue();
    if (val.empty()) return;

    char ssid[33] = {0}, pass[65] = {0};
    if (!parseWifiJson(val.c_str(), ssid, pass, sizeof(ssid))) {
      Serial.printf("WiFi: bad JSON: %s\n", val.c_str());
      return;
    }

    Serial.printf("WiFi: connecting to '%s'...\n", ssid);
    s_wifiState = WF_CONNECTING;
    WiFi.begin(ssid, pass);
  }
};

static WifiCfgCallback* s_wifiCb = nullptr;  // 保持引用防止被回收

void setup() {
  s_led.begin();
  s_led.setBrightness(32);
  s_led.fill(s_led.Color(255, 0, 0));  // 🔴 启动
  s_led.show();

  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== DeskPet BLE WiFi Provisioning ===");
  Serial.printf("Chip: %s rev%d\n", ESP.getChipModel(), ESP.getChipRevision());

  // ── BLE Setup ──
  BLEDevice::init("DeskPet-Test");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ConnCallback());

  BLEService* svc = server->createService(SERVICE_UUID);

  // Info Characteristic (可读, 显示状态)
  BLECharacteristic* info = svc->createCharacteristic(
    CHAR_INFO, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  info->addDescriptor(new BLE2902());
  info->setValue("Send JSON to abcd0002: {\"ssid\":\"xxx\",\"pass\":\"yyy\"}");

  // WiFi Config Characteristic (可写, 接收配网JSON)
  BLECharacteristic* cfg = svc->createCharacteristic(
    CHAR_WIFI_CFG, BLECharacteristic::PROPERTY_WRITE);
  s_wifiCb = new WifiCfgCallback();
  cfg->setCallbacks(s_wifiCb);

  svc->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();

  WiFi.mode(WIFI_STA);
  Serial.println("BLE: advertising. Write JSON to abcd0002 to configure WiFi");
}

void loop() {
  uint32_t now = millis();

  // ── WiFi 状态机 ──
  if (s_wifiState == WF_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      s_wifiState = WF_OK;
      Serial.printf("\nWiFi CONNECTED!\n  IP: %s\n  RSSI: %d dBm\n\n",
        WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
      s_wifiState = WF_FAIL;
      s_wifiFailTime = now;
      Serial.println("WiFi: connect FAILED!");
    }
  }
  // 失败 5 秒后回到待机状态
  if (s_wifiState == WF_FAIL && now - s_wifiFailTime > 5000) {
    s_wifiState = WF_IDLE;
    WiFi.disconnect();
  }

  // ── LED 指示 ──
  float phase = (now % 2000) / 2000.0f;
  uint8_t bri = 4 + (uint8_t)((sin(phase * 2 * PI) + 1) * 10);

  s_led.setBrightness(bri);
  switch (s_wifiState) {
    case WF_IDLE:
      s_led.fill(s_led.Color(0, 0, 255));    // 🔵 蓝色呼吸 = 等待配网
      break;
    case WF_CONNECTING:
      s_led.setBrightness(32);
      s_led.fill(s_led.Color(255, 255, 0));  // 🟡 黄色常亮 = 连接中
      break;
    case WF_OK:
      s_led.fill(s_led.Color(0, 255, 0));    // 🟢 绿色呼吸 = 已连接
      break;
    case WF_FAIL:
      s_led.fill(s_led.Color(255, 0, 0));    // 🔴 红色呼吸 = 失败
      break;
  }
  s_led.show();

  delay(30);
}
