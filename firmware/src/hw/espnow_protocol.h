#pragma once
// Desk Keyboard + DeskPet — ESP-NOW bidirectional protocol
// Keyboard broadcasts button events; DeskPet replies with ACK+state.
// USB receiver also catches broadcasts for standalone keyboard use.

#include <cstdint>

// ── Button IDs (keyboard → everyone) ───────────────────────────────────
enum ButtonId : uint8_t {
  BTN_YES         = 0,   // Allow permission
  BTN_NO          = 1,   // Deny permission
  BTN_CORRECT     = 2,   // quick command "correct"
  BTN_PLAIN_LANG  = 3,   // quick command "plain_language"
  BTN_PLAN_FIRST  = 4,   // quick command "plan_first"
  BTN_COUNT       = 5,
};

// ── Deskpet state (deskpet → keyboard ACK) ─────────────────────────────
enum DeskpetState : uint8_t {
  DP_OFFLINE      = 0,   // no deskpet nearby
  DP_IDLE         = 1,   // Claude idle
  DP_WORKING      = 2,   // Claude working/thinking
  DP_PERMISSION   = 3,   // permission bubble active — YES/NO needed!
  DP_SLEEP        = 4,   // do-not-disturb / sleeping
};

// ── Messages ────────────────────────────────────────────────────────────

// Keyboard → broadcast (2 bytes, minimal air time)
struct __attribute__((packed)) ButtonEvent {
  uint8_t button_id;   // ButtonId
  uint8_t battery_mv;  // battery voltage in 10mV steps (0 = unknown)
};

// Deskpet → keyboard (ACK, 3 bytes)
struct __attribute__((packed)) AckEvent {
  uint8_t button_id;      // echo of the button that was pressed
  uint8_t deskpet_state;  // DeskpetState enum
  uint8_t perm_count;     // number of pending permissions (0 = none)
};

// Broadcast MAC — reaches USB receiver AND deskpet simultaneously
static constexpr uint8_t ESP_NOW_BROADCAST[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
