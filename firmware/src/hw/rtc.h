// src/hw/rtc.h — 软件时钟 (BLE 同步)

#pragma once
#include <stdint.h>

struct HwTime { uint8_t H,M,S, D,Mo,dow; uint16_t Y; };

bool hwRtcInit();
bool hwRtcRead(HwTime* t);
bool hwRtcWrite(const HwTime& t);
bool hwRtcSynced();
