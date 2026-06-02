// src/hw/imu.h — MPU6050 6-axis IMU

#pragma once

bool hwImuInit();
void hwImuAccel(float* ax, float* ay, float* az);  // 返回 g 值
