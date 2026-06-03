// DeskPet ESP-NOW Bridge — implementation
// Listens for ESP-NOW broadcast from 2.4G mini-keyboard.
// Sends ACK back with current deskpet state for keyboard LED feedback.
// Runs alongside existing BLE; ESP32 handles WiFi + BLE coexistence.

#include "espnow_bridge.h"
#include <WiFi.h>
#include <esp_now.h>

static EspNowButtonHandler g_handler = nullptr;
static DeskpetState g_state = DP_IDLE;
static uint8_t g_permCount = 0;

// ── ESP-NOW receive callback ───────────────────────────────────────────
static void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len != sizeof(ButtonEvent)) return;
  const ButtonEvent *evt = (const ButtonEvent*)data;
  if (evt->button_id >= BTN_COUNT) return;

  // Forward to main handler (sends BLE command to PC)
  if (g_handler) g_handler((ButtonId)evt->button_id, evt->battery_mv);

  // Build ACK with current deskpet state
  AckEvent ack;
  ack.button_id = evt->button_id;
  ack.deskpet_state = g_state;
  ack.perm_count = g_permCount;

  // Send ACK back to the keyboard sender
  if (mac_addr) {
    esp_now_send(mac_addr, (uint8_t*)&ack, sizeof(ack));
  }
}

// ── Init ───────────────────────────────────────────────────────────────
bool espNowInit(EspNowButtonHandler handler) {
  g_handler = handler;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, ESP_NOW_BROADCAST, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer); // ignore error — peer may already exist
  return true;
}

// ── State helpers ──────────────────────────────────────────────────────
void espNowSetState(DeskpetState state) { g_state = state; }
void espNowSetPermCount(uint8_t count)  { g_permCount = count; }
