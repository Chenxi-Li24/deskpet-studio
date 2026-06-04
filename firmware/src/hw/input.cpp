// src/hw/input.cpp — CST816T 触摸 + 2x EC11 编码器 → BtnA/BtnB 语义

#include "hw/input.h"
#include "hw/pins.h"
#include <Arduino.h>
#include <Wire.h>
// 测试模式禁用触摸库
#if 0
#include <Adafruit_CST8XX.h>
#endif

static HwBtn  s_btnA, s_btnB;
static HwEnc  s_enc1, s_enc2, s_enc1Web;
static HwTouch s_touch;

static volatile int8_t s_enc1Delta = 0;
static volatile int8_t s_enc2Delta = 0;

// ── CST816T ──
// 测试模式: CST816T 全部禁用
#if 0
static Adafruit_CST8XX s_cst;
static volatile bool   s_tpIrq = false;
static void IRAM_ATTR tpISR() { s_tpIrq = true; }
#endif

// ── 编码器 CLK 中断 ──
static void IRAM_ATTR enc1ISR() {
  s_enc1Delta += (digitalRead(PIN_ENC1_CLK) == digitalRead(PIN_ENC1_DT)) ? +1 : -1;
}
static void IRAM_ATTR enc2ISR() {
  s_enc2Delta += (digitalRead(PIN_ENC2_CLK) == digitalRead(PIN_ENC2_DT)) ? +1 : -1;
}

bool HwBtn::pressedFor(uint32_t ms) {
  return isPressed && (millis() - pressedAt) >= ms;
}

bool hwInputInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  // --- CST816T 触摸 (测试禁用) ---
#if 0
  pinMode(PIN_TP_RST, OUTPUT);
  digitalWrite(PIN_TP_RST, LOW);  delay(10);
  digitalWrite(PIN_TP_RST, HIGH); delay(50);

  if (!s_cst.begin()) {
    Serial.println("hwInput: CST816T begin failed (no touch)");
  } else {
    pinMode(PIN_TP_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_TP_INT), tpISR, FALLING);
    Serial.println("hwInput: CST816T OK");
  }
#endif

  // --- 编码器1 (VCC 复用按键检测) ---
  pinMode(PIN_ENC1_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC1_DT,  INPUT_PULLUP);
  // GPIO8 = VCC供电 + 按键复用: 平时OUTPUT HIGH给模块供电
  pinMode(PIN_ENC1_SW, OUTPUT);
  digitalWrite(PIN_ENC1_SW, HIGH);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC1_CLK), enc1ISR, CHANGE);

  // --- 编码器2 (暂不启用 — 只有一个 EC11) ---
#if 0
  pinMode(PIN_ENC2_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC2_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC2_SW,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_CLK), enc2ISR, CHANGE);
#endif

  Serial.println("hwInput: EC11 x1 OK");
  return true;
}

void hwInputUpdate() {
  uint32_t now = millis();

  // --- 触摸屏 (测试禁用) ---
#if 0
  if (s_tpIrq || s_touch.down) {
    s_tpIrq = false;
    if (s_cst.touched()) {
      s_touch.justPressed  = !s_touch.down;
      s_touch.justReleased = false;
      CST_TS_Point p = s_cst.getPoint();
      s_touch.x = p.x;
      s_touch.y = p.y;

      int dx = s_touch.x - (LCD_PHYS_W - HW_W) / 2;
      int dy = s_touch.y - (LCD_PHYS_H - HW_H) / 2;
      s_touch.x = constrain(dx, 0, HW_W - 1);
      s_touch.y = constrain(dy, 0, HW_H - 1);
      s_touch.down = true;
    } else {
      s_touch.justReleased = s_touch.down;
      s_touch.down         = false;
      s_touch.justPressed  = false;
    }
  }
#endif

  // --- 编码器1 SW → BtnA (VCC复用: analogRead, 简单可靠) ---
  static uint32_t lastBtnCheck = 0;
  static bool     cachedSw = false;
  if (now - lastBtnCheck > 200) {    // ~5Hz, 减少VCC中断
    lastBtnCheck = now;
    noInterrupts();
    pinMode(PIN_ENC1_SW, INPUT);         // 高阻输入, 电容保持VCC电荷
    interrupts();
    delay(3);                            // 3ms放电: 按下→~2500, 不按→~4000+
    noInterrupts();
    int v0 = analogRead(PIN_ENC1_SW);
    delay(1);
    int v1 = analogRead(PIN_ENC1_SW);
    delay(1);
    int v2 = analogRead(PIN_ENC1_SW);
    // 3次采样取中值, 过滤偶发跳变
    int v;
    if (v0 <= v1 && v1 <= v2) v = v1;
    else if (v2 <= v1 && v1 <= v0) v = v1;
    else if (v1 <= v0 && v0 <= v2) v = v0;
    else if (v2 <= v0 && v0 <= v1) v = v0;
    else v = v2;
    cachedSw = (v < 3500);               // 按下<3500, 松开>3500, 两侧余量充足
    Serial.printf("BTN: adc=%d pressed=%d\n", v, cachedSw);
    pinMode(PIN_ENC1_SW, OUTPUT);
    digitalWrite(PIN_ENC1_SW, HIGH);
    interrupts();
  }
  bool sw1 = cachedSw;
  s_btnA.wasPressed  = sw1 && !s_btnA.isPressed;
  s_btnA.wasReleased = !sw1 && s_btnA.isPressed;
  if (s_btnA.wasPressed) s_btnA.pressedAt = now;
  s_btnA.isPressed = sw1;

  // --- 编码器2 SW → BtnB (暂不启用) ---
#if 0
  bool sw2 = !digitalRead(PIN_ENC2_SW);
  s_btnB.wasPressed  = sw2 && !s_btnB.isPressed;
  s_btnB.wasReleased = !sw2 && s_btnB.isPressed;
  if (s_btnB.wasPressed) s_btnB.pressedAt = now;
  s_btnB.isPressed = sw2;
#endif

  // --- 旋转 delta ---
  noInterrupts();
  int8_t raw = s_enc1Delta; s_enc1Delta = 0;
  interrupts();
  s_enc1.delta = raw;
  // 死区: 按键时抑制 |delta|<3 忽略
  if (sw1)      s_enc1.delta = 0;
  else if (abs(raw) < 3) s_enc1.delta = 0;

  s_enc2.delta = 0;           // ENC2 暂不启用
  s_enc1.pressed = sw1; s_enc1.justPressed = s_btnA.wasPressed;
  s_enc2.pressed = false; s_enc2.justPressed = false;

  // Web accumulator — persists across hwInputUpdate() calls
  s_enc1Web.delta += s_enc1.delta;
  s_enc1Web.pressed = s_enc1.pressed;
  s_enc1Web.justPressed = s_enc1Web.justPressed || s_enc1.justPressed;
}

HwBtn&        hwBtnA()  { return s_btnA; }
HwBtn&        hwBtnB()  { return s_btnB; }
HwEnc         hwEnc1()  { return s_enc1; }
HwEnc         hwEnc2()  { return s_enc2; }
HwEnc         hwEnc1Web() {
  HwEnc ret = s_enc1Web;
  s_enc1Web.delta = 0;
  s_enc1Web.justPressed = false;
  return ret;
}
const HwTouch& hwTouch() { return s_touch; }
