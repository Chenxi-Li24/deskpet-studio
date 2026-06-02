// src/hw/imu.cpp — MPU6050, Adafruit 库返回 m/s² → 转为 g

#include "hw/imu.h"
#include "hw/pins.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>

static Adafruit_MPU6050 s_mpu;

bool hwImuInit() {
  if (!s_mpu.begin()) {
    Serial.println("hwImu: MPU6050 begin failed");
    return false;
  }
  s_mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  Serial.println("hwImu: MPU6050 OK");
  return true;
}

void hwImuAccel(float* ax, float* ay, float* az) {
  sensors_event_t a, g, t;
  s_mpu.getEvent(&a, &g, &t);
  *ax = a.acceleration.x / 9.81f;
  *ay = a.acceleration.y / 9.81f;
  *az = a.acceleration.z / 9.81f;
}
