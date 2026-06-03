#pragma once
// DeskPet ESP-NOW Bridge — receives button events from 2.4G mini-keyboard
// Forwards commands via BLE to PC, sends ACK back to keyboard with deskpet state.
// Completely optional: deskpet works fine without keyboard, keyboard works
// fine via its USB receiver. When both are on, they enhance each other.

#include <cstdint>
#include "espnow_protocol.h"

// Callback: called when a keyboard button event is received.
// Return the BLE command string to send to PC, or nullptr to ignore.
typedef const char* (*EspNowButtonHandler)(ButtonId btn, uint8_t batteryMv);

// Init ESP-NOW (call once in setup()). Returns true on success.
bool espNowInit(EspNowButtonHandler handler);

// State setters — called by main loop to keep ACK info current
void espNowSetState(DeskpetState state);
void espNowSetPermCount(uint8_t count);
