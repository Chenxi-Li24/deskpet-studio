// src/hw/input.h — CST816T 触摸 + EC11 编码器 → 按键语义

#pragma once
#include <stdint.h>

struct HwBtn {
  bool     isPressed, wasPressed, wasReleased;
  uint32_t pressedAt;
  bool     pressedFor(uint32_t ms);
};

struct HwEnc {
  int8_t delta;       // 本次采样旋转步数 +CW/-CCW
  bool   pressed, justPressed;
};

struct HwTouch {
  bool    down;
  int16_t x, y;
  bool    justPressed, justReleased;
};

bool          hwInputInit();
void          hwInputUpdate();
HwBtn&        hwBtnA();      // Enc1 SW: 确认/批准
HwBtn&        hwBtnB();      // Enc2 SW: 返回/拒绝
HwEnc         hwEnc1();      // Enc1 旋转: 导航
HwEnc         hwEnc2();      // Enc2 旋转: 翻页
const HwTouch& hwTouch();    // 触摸屏
