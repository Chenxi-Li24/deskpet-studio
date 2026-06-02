// src/hw/pins.h — 物理桌宠引脚定义
// P169H002-CTP 1.69" 240x280 ST7789V + CST816T
// MPU6050 I2C / 亚博 ASR-TTS / 2x EC11 / 18650

#pragma once

// ── SPI 屏幕 (ST7789V 4-wire SPI Mode 3) ──
#define PIN_LCD_SCLK  12
#define PIN_LCD_MOSI  11
#define PIN_LCD_CS    10
#define PIN_LCD_DC    13
#define PIN_LCD_RST   14
#define PIN_LCD_BL    9

// ── I2C 总线 ──
#define PIN_I2C_SDA   4
#define PIN_I2C_SCL   5

// ── 触摸 CST816T I2C 0x15 ──
#define PIN_TP_RST    2
#define PIN_TP_INT    3

// ── 旋转编码器 EC11 ──
#define PIN_ENC1_CLK  6
#define PIN_ENC1_DT   7
#define PIN_ENC1_SW   8
#define PIN_ENC2_CLK  15
#define PIN_ENC2_DT   16
#define PIN_ENC2_SW   17

// ── WS2812 LED ──
#define PIN_LED       18

// ── PWM 蜂鸣器 ──
#define PIN_BEEP      21

// ── 电池 ADC (18650 分压 1:1) ──
#define PIN_VBAT_ADC  1

// ── 电源按键 ──
#define PIN_PWR_KEY   0

// ── AI 语音 UART 预留 ──
#define PIN_VOICE_RX  20
#define PIN_VOICE_TX  19

// ── 屏幕参数 ──
#define LCD_PHYS_W    240
#define LCD_PHYS_H    280
#define HW_W          135
#define HW_H          240
