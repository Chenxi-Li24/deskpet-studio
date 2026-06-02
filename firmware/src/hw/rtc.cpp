// src/hw/rtc.cpp — 软件时钟, BLE 桥接同步

#include "hw/rtc.h"
#include <time.h>

static time_t   s_epoch  = 0;
static uint32_t s_syncMs = 0;
static bool     s_synced = false;

bool hwRtcInit() { return true; }

bool hwRtcWrite(const HwTime& t) {
  struct tm lt = {};
  lt.tm_sec = t.S; lt.tm_min = t.M; lt.tm_hour = t.H;
  lt.tm_mday = t.D; lt.tm_mon = t.Mo - 1; lt.tm_year = t.Y - 1900;
  s_epoch = mktime(&lt);
  if (s_epoch == (time_t)-1) return false;
  s_syncMs = millis();
  s_synced = true;
  return true;
}

bool hwRtcRead(HwTime* t) {
  if (!s_synced) { *t = {}; return false; }
  uint32_t elapsed = (millis() - s_syncMs) / 1000;
  time_t now = s_epoch + (time_t)elapsed;
  struct tm lt;
  localtime_r(&now, &lt);
  t->S = lt.tm_sec; t->M = lt.tm_min; t->H = lt.tm_hour;
  t->D = lt.tm_mday; t->Mo = lt.tm_mon + 1;
  t->Y = lt.tm_year + 1900; t->dow = lt.tm_wday;
  return true;
}

bool hwRtcSynced() { return s_synced; }
