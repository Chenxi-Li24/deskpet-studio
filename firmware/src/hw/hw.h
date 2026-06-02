// src/hw/hw.h — 硬件抽象层总入口

#pragma once
#include "hw/display.h"
#include "hw/input.h"
#include "hw/power.h"
#include "hw/imu.h"
#include "hw/rtc.h"
#include "hw/audio.h"
#include "hw/pins.h"

void hwInit();
