// src/hw/hw.cpp — 硬件初始化 (顺序敏感)

#include "hw/hw.h"
#include <Arduino.h>
#include <Wire.h>

static void die(const char* what) {
  Serial.printf("hwInit FAIL: %s\n", what);
  while (1) delay(1000);
}

void hwInit() {
  Serial.begin(115200);
  delay(200);  // 缩短延时, 加快启动
  Serial.println("\n=== DeskPet ESP32-S3 boot ===");

  if (!hwDisplayInit())  die("display");

  Serial.println("hwInit OK");
}
