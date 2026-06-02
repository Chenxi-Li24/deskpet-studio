// src/hw/input.cpp — CST816T 触摸 + 2x EC11 编码器 → BtnA/BtnB 语义

#include "hw/input.h"
#include "hw/pins.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_CST8xx.h>

static HwBtn  s_btnA, s_btnB;
static HwEnc  s_enc1, s_enc2;
static HwTouch s_touch;

static volatile int8_t s_enc1Delta = 0;
static volatile int8_t s_enc2Delta = 0;

// ── CST816T ──
static Adafruit_CST8xx s_cst;
static volatile bool   s_tpIrq = false;
static void IRAM_ATTR tpISR() { s_tpIrq = true; }

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

  // --- CST816T 触摸 ---
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

  // --- 编码器1 ---
  pinMode(PIN_ENC1_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC1_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC1_SW,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC1_CLK), enc1ISR, CHANGE);

  // --- 编码器2 ---
  pinMode(PIN_ENC2_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC2_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC2_SW,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_CLK), enc2ISR, CHANGE);

  Serial.println("hwInput: EC11 x2 OK");
  return true;
}

void hwInputUpdate() {
  uint32_t now = millis();

  // --- 触摸屏 ---
  if (s_tpIrq || s_touch.down) {
    s_tpIrq = false;
    if (s_cst.touched()) {
      s_touch.justPressed  = !s_touch.down;
      s_touch.justReleased = false;
      s_cst.getPoint(&s_touch.x, &s_touch.y, nullptr, nullptr);

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

  // --- 编码器1 SW → BtnA ---
  bool sw1 = !digitalRead(PIN_ENC1_SW);
  s_btnA.wasPressed  = sw1 && !s_btnA.isPressed;
  s_btnA.wasReleased = !sw1 && s_btnA.isPressed;
  if (s_btnA.wasPressed) s_btnA.pressedAt = now;
  s_btnA.isPressed = sw1;

  // --- 编码器2 SW → BtnB ---
  bool sw2 = !digitalRead(PIN_ENC2_SW);
  s_btnB.wasPressed  = sw2 && !s_btnB.isPressed;
  s_btnB.wasReleased = !sw2 && s_btnB.isPressed;
  if (s_btnB.wasPressed) s_btnB.pressedAt = now;
  s_btnB.isPressed = sw2;

  // --- 旋转 delta ---
  noInterrupts();
  s_enc1.delta = s_enc1Delta; s_enc1Delta = 0;
  s_enc2.delta = s_enc2Delta; s_enc2Delta = 0;
  interrupts();
  s_enc1.pressed = sw1; s_enc1.justPressed = s_btnA.wasPressed;
  s_enc2.pressed = sw2; s_enc2.justPressed = s_btnB.wasPressed;
}

HwBtn&        hwBtnA()  { return s_btnA; }
HwBtn&        hwBtnB()  { return s_btnB; }
HwEnc         hwEnc1()  { return s_enc1; }
HwEnc         hwEnc2()  { return s_enc2; }
const HwTouch& hwTouch() { return s_touch; }
