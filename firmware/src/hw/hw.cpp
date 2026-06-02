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
  delay(500);
  Serial.println("\n=== DeskPet ESP32-S3 boot ===");

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  if (!hwDisplayInit())  die("display");
  if (!hwInputInit())    die("input");
  if (!hwImuInit())      die("imu");
  if (!hwPowerInit())    die("power");
  if (!hwAudioInit())    die("audio");
  if (!hwRtcInit())      die("rtc");

  Serial.println("hwInit OK");
}
