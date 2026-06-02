// src/hw/display.h — ST7789V 240x280 → 135x240 逻辑画布

#pragma once
#include <Arduino_GFX_Library.h>
#include "hw/pins.h"

bool              hwDisplayInit();
void              hwDisplayPush();
void              hwDisplayBrightness(uint8_t lvl_0_4);
void              hwDisplaySleep(bool off);
Arduino_Canvas*   hwCanvas();
