# 物理桌宠 - 完整执行方案

## 项目目标

做一个 ESP32-S3 物理桌宠，通过 BLE 连接 Claude Desktop，在硬件屏幕上显示宠物动画，支持摇晃互动和物理按键审批权限请求。

```
┌──────────────────────┐    BLE (Nordic UART)    ┌─────────────────────┐
│  Claude Desktop (PC) │ ◄─────────────────────► │  你的物理桌宠设备    │
│  Developer Mode      │   JSON Lines over BLE   │  ESP32-S3 + 屏幕    │
└──────────────────────┘                         └─────────────────────┘
```

---

## Phase 1 - 硬件选型与面包板验证

### 1.1 物料清单 (BOM)

| 物料 | 型号建议 | 数量 | 单价(¥) | 备注 |
|------|---------|------|---------|------|
| MCU 模块 | ESP32-S3-WROOM-1 N16R8 | 1 | ~25 | 16MB Flash, 8MB PSRAM |
| 屏幕 | ST7789 1.54" 240×240 IPS | 1 | ~15 | 8P 排线, SPI |
| IMU | GY-521 MPU6050 模块 | 1 | ~4 | I2C, 3.3V |
| 旋转编码器 | EC11 模块 (带板) | 2 | ~2 | CLK/DT/SW/+/GND |
| 电池 | 18650 锂电池 2600mAh | 1 | ~12 | |
| 充电模块 | TP4056 Type-C | 1 | ~2 | 带 DW01 保护 |
| 升压模块 | MT3608 | 1 | ~3 | 3.7V→5V |
| LDO | AMS1117-3.3 | 1 | ~1 | 5V→3.3V |
| RGB LED | WS2812-2020 | 2 | ~1 | 状态指示 |
| 蜂鸣器 | 无源蜂鸣器 | 1 | ~1 | PWM 驱动 |
| 面包板 | 830 孔 | 1 | ~5 | |
| 杜邦线 | 公母各一盒 | 2 | ~6 | |
| 电阻 | 100KΩ ×2 | 2 | ~0 | 电池分压 |

**总预算: ~80¥**

### 1.2 面包板引脚规划

```
ESP32-S3 引脚分配:

SPI 屏幕 (ST7789):
  SCLK = GPIO12    MOSI = GPIO11
  CS   = GPIO10    DC   = GPIO13
  RST  = GPIO14    BL   = GPIO9  (PWM 背光)

I2C 总线:
  SDA  = GPIO4     SCL  = GPIO5
  └── MPU6050 (0x68)

旋转编码器:
  Enc1: CLK=GPIO6  DT=GPIO7   SW=GPIO8   (主导航 + 确认)
  Enc2: CLK=GPIO15 DT=GPIO16  SW=GPIO17  (翻页 + 返回)

其他:
  LED  = GPIO18  (WS2812 Data)
  BEEP = GPIO21  (PWM 蜂鸣器)
  VBAT = GPIO1   (ADC 电池分压)
  PWR  = GPIO0   (电源按键, 短按唤醒/长按关机)
```

### 1.3 搭建顺序

```
Day 1 - 最小系统:
  1. ESP32-S3 USB 烧录 Blink 验证
  2. 接 ST7789, 跑 TFT_eSPI 示例
  3. 接 MPU6050, I2C Scanner + 读数

Day 2 - 外设:
  4. 接编码器, 写测试程序验证旋转/按下
  5. 接 WS2812, PWM 蜂鸣器
  6. 接电池+TP4056+升压, 验证 ADC 电压读取

Day 3 - BLE:
  7. 烧录固件, 验证 BLE 广告
  8. Claude Desktop → Developer Mode → Hardware Buddy 连接测试
```

---

## Phase 2 - 固件: 硬件抽象层 (hw/)

参考 `esp32s3-amoled-ref/src/hw/` 的设计模式，为你的硬件写一套 hw/ 驱动。

### 2.1 项目结构

```
deskpet-firmware/
├── platformio.ini
├── no_ota_8mb.csv
├── src/
│   ├── main.cpp            # 主逻辑 (从原版移植, 少量修改)
│   ├── data.h              # JSON 解析 (不改)
│   ├── stats.h             # NVS 统计 (不改)
│   ├── xfer.h              # 文件推送 (不改)
│   ├── buddy.cpp/h         # ASCII 宠物 (不改)
│   ├── character.cpp/h     # GIF 角色 (不改)
│   ├── ble_bridge.cpp/h    # BLE NUS (不改)
│   ├── buddies/            # 18 种宠物
│   └── hw/                 # ★ 硬件抽象层 (新写)
│       ├── hw.h / hw.cpp   # 统一初始化
│       ├── pins.h          # 引脚定义
│       ├── display.h/cpp   # ST7789 + Canvas
│       ├── imu.h/cpp       # MPU6050
│       ├── input.h/cpp     # 编码器映射
│       ├── power.h/cpp     # 电池检测
│       ├── audio.h/cpp     # PWM 蜂鸣器
│       └── rtc.h/cpp       # 软件时钟
```

### 2.2 各模块接口

```cpp
// ====== display.h ======
constexpr int HW_W = 135, HW_H = 240;   // 逻辑分辨率

bool hwDisplayInit();                          // 初始化 SPI + ST7789
void hwDisplayPush();                          // canvas → 物理屏
void hwDisplayBrightness(uint8_t lvl);        // 0..4 亮度
void hwDisplaySleep(bool off);                 // 关屏/开屏
Arduino_Canvas* hwCanvas();                    // 获取画布指针

// 物理屏 240×240, 逻辑区 135×240 居中:
// [53px 黑边 | 135px 内容 | 52px 黑边]
// hwDisplayPush 自动处理偏移

// ====== imu.h ======
bool hwImuInit();
void hwImuAccel(float* ax, float* ay, float* az); // 返回 g 值

// ====== input.h ======
struct HwBtn {
  bool isPressed, wasPressed, wasReleased;
  uint32_t pressedAt;
  bool pressedFor(uint32_t ms);
};
struct HwEnc { int8_t delta; bool pressed, justPressed; };

bool hwInputInit();
void hwInputUpdate();
HwBtn&    hwBtnA();   // Enc1-SW: 确认/批准
HwBtn&    hwBtnB();   // Enc2-SW: 返回/拒绝
HwEnc     hwEnc1();   // Enc1 旋转: 菜单导航
HwEnc     hwEnc2();   // Enc2 旋转: 翻页/滚动

// ====== power.h ======
struct HwBattery { int mV, pct; bool usb; };
bool      hwPowerInit();
HwBattery hwBattery();
void      hwPowerOff();

// ====== audio.h ======
bool hwAudioInit();
void hwBeep(uint16_t freq, uint16_t durMs);

// ====== rtc.h ======
struct HwTime { uint8_t H,M,S,D,Mo,dow; uint16_t Y; };
bool hwRtcInit();
bool hwRtcRead(HwTime* t);
bool hwRtcWrite(const HwTime& t);
bool hwRtcSynced();
```

### 2.3 关键实现: input.cpp — 编码器 → 按键语义

```cpp
// 核心思路: 把编码器的物理操作映射为 original main.cpp 期望的按钮事件
//
// 编码器1 (主控):
//   旋转 CW  → 注入短 BtnA pulse (菜单选择下移 / 页面切换)
//   旋转 CCW → 注入短 BtnA pulse (菜单选择上移)
//   按下 SW  → 真正的 hwBtnA().wasPressed (确认/批准/长按菜单)
//
// 编码器2 (副控):
//   旋转    → 注入短 BtnB pulse (翻页 / 滚动)
//   按下 SW → 真正的 hwBtnB().wasPressed (返回/拒绝)

// 编码器读取用中断:
volatile int8_t enc1Delta, enc2Delta;
void IRAM_ATTR enc1_isr() {
  enc1Delta += (digitalRead(PIN_ENC1_CLK) == digitalRead(PIN_ENC1_DT)) ? +1 : -1;
}
// 在 hwInputUpdate() 中消费 delta, 每积累4步 → 注入一个 button event
```

### 2.4 关键实现: imu.cpp — MPU6050

```cpp
#include <Adafruit_MPU6050.h>
// Adafruit 库返回 m/s² → 除以 9.81 转为 g 值 (匹配原版 face-down 判断逻辑)
void hwImuAccel(float* ax, float* ay, float* az) {
  sensors_event_t a, g, t;
  s_mpu.getEvent(&a, &g, &t);
  *ax = a.acceleration.x / 9.81f;
  *ay = a.acceleration.y / 9.81f;
  *az = a.acceleration.z / 9.81f;
}
```

### 2.5 关键实现: display.cpp — ST7789 240×240 居中

```cpp
// 物理屏 240×240, Canvas 135×240
// hwDisplayPush(): 计算偏移 → 居中 blit 到物理屏
void hwDisplayPush() {
  const int OFF_X = (240 - HW_W) / 2;  // = 52
  uint16_t* src = (uint16_t*)s_canvas->getFramebuffer();
  s_gfx->draw16bitRGBBitmap(OFF_X, 0, src, HW_W, HW_H);
  // attention 红条画在顶部物理屏坐标
  if (alertOn) s_gfx->fillRoundRect(70, 4, 100, 6, 3, RED);
}
```

---

## Phase 3 - 固件: main.cpp 适配

### 3.1 改动清单

把原版 `claude-deskpet-esp32s3/src/main.cpp` 里所有 `M5.xxx` 调用替换为对应的 `hw/` 接口:

| 原版调用 | 替换为 | 改动类型 |
|---------|--------|---------|
| `#include <M5StickCPlus.h>` | `#include "hw/hw.h"` | 头文件 |
| `M5.begin()` | `hwInit()` | setup |
| `TFT_eSprite spr(&M5.Lcd)` | `Arduino_Canvas* c = hwCanvas()` | 全局变量 |
| `spr.createSprite(W,H)` | 删掉 (hwDisplayInit 已创建) | setup |
| `spr.fillSprite(c)` | `c->fillScreen(c)` | 全部替换 |
| `spr.pushSprite(0,0)` | `hwDisplayPush()` | loop 尾 |
| `spr.setCursor / print / drawXxx` | `c->setCursor / print / drawXxx` | 全部替换 |
| `M5.Imu.getAccelData(&ax,&ay,&az)` | `hwImuAccel(&ax,&ay,&az)` | loop |
| `M5.BtnA.isPressed()` | `hwBtnA().isPressed` | 按键 |
| `M5.BtnA.wasReleased()` | `hwBtnA().wasReleased` | 按键 |
| `M5.BtnA.pressedFor(600)` | `hwBtnA().pressedFor(600)` | 长按 |
| `M5.BtnB.wasPressed()` | `hwBtnB().wasPressed` | 按键 |
| `M5.Beep.tone(f,d)` | `hwBeep(f,d)` | 蜂鸣 |
| `M5.Axp.GetBatVoltage()` | `hwBattery().mV / 1000.0f` | 电池 |
| `M5.Axp.ScreenBreath(v)` | `hwDisplayBrightness(lvl)` | 亮度 |
| `M5.Axp.SetLDO2(on)` | `hwDisplaySleep(!on)` | 屏幕 |
| `M5.Axp.PowerOff()` | `hwPowerOff()` | 关机 |
| `M5.Rtc.GetTime/Date()` | `hwRtcRead(&t)` | 时钟 |
| `M5.Axp.GetBtnPress()` | `digitalRead(PIN_PWR)` | 电源键 |
| `M5.Axp.GetVBusVoltage()` | `hwBattery().usb` | USB 检测 |
| `M5.update()` / `M5.Beep.update()` | `hwInputUpdate()` | loop |
| `digitalWrite(LED_PIN, x)` | WS2812 API | LED |
| `M5.Lcd.setRotation(r)` | c->setRotation(r) | 旋转 |

### 3.2 编码器增强

原版用 BtnA 短按循环切换页面 (Normal → Pet → Info → Normal)。现在用编码器1旋转替代:

```cpp
// 在 main.cpp loop() 中:
static int8_t encNav = 0;
encNav += hwEnc1().delta / 4;  // 每4步 = 1 click = 切换一次
if (encNav >= 4) { displayMode = (displayMode+1) % DISP_COUNT; encNav -= 4; }
if (encNav <= -4) { displayMode = (displayMode==0?DISP_COUNT-1:displayMode-1); encNav += 4; }

// 菜单内: 编码器旋转替代 BtnA 递增
if (menuOpen) {
  menuSel = (menuSel + hwEnc1().delta/4 + MENU_N) % MENU_N;
}

// 编码器2旋转替代 BtnB 翻页/滚动
if (displayMode == DISP_INFO) {
  infoPage = (infoPage + hwEnc2().delta/4 + INFO_PAGES) % INFO_PAGES;
}
```

---

## Phase 4 - PCB 设计

### 4.1 电源树

```
USB-C (5V) ──→ TP4056 ──→ 18650 (充电管理)
                  │
18650 (+) ──→ 开关 ──→ MT3608 (升压5V) ──→ AMS1117-3.3 ──→ ESP32-S3 + 外设
                                         └──→ 屏幕背光 (5V)
```

### 4.2 关键电路

- **电池分压**: VBAT+ → 100K(1%) → ADC → 100K(1%) → GND
- **编码器去抖**: CLK/DT 各接 100nF 对地 + 10K 上拉
- **WS2812**: VDD 旁路 100nF, DIN 串 100Ω
- **屏幕背光**: GPIO9 → NPN S8050 基极(串1K) → 集电极接 BL → 发射极接地

### 4.3 推荐流程

- 工具: **立创 EDA** (免费, 一键下单)
- 尺寸: 双层板, 50×35mm 以内
- 打样: 立创 10×10cm 内 5元/5片, 约5天到货
- ESP32-S3 天线区域不铺铜, 留 15mm 净空

---

## Phase 5 - 外壳 (3D 打印)

- 建模: Fusion 360 (教育版) / FreeCAD
- 屏幕窗口 → 透明亚克力片
- 编码器旋钮突出面板
- 底部: 18650 电池仓 + Type-C 开口 + 开关开口
- 侧边: 蜂鸣器出音孔

---

## Phase 6 - 组装调试

### 调试命令

```bash
cd deskpet-firmware
pio run                    # 编译
pio run -t upload          # 烧录
pio device monitor -b 115200  # 串口监视
pio run -t erase && pio run -t upload  # 完全擦除重烧
```

### 检查清单

- [ ] USB 上电, 串口有输出
- [ ] 屏幕显示 "Hello!" 启动画面
- [ ] 手机 nRF Connect 能扫描到 `Claude-XXXX`
- [ ] Claude Desktop Hardware Buddy 能连接
- [ ] 摇晃设备 → 屏幕显示 dizzy 动画
- [ ] 翻转设备 (面朝下) → 进入 nap 模式, 屏幕变暗
- [ ] 编码器旋转/按下均正常响应
- [ ] 权限请求时 LED 闪烁, 蜂鸣器提示
- [ ] 电池供电正常运行

---

## Phase 7 - 后续增强

1. **AI 语音模块** - 等确认型号后加 UART 驱动, 实现语音播报权限请求
2. **自定义角色** - 做自己的 GIF 角色包, 通过 BLE 推送到设备
3. **无线充电** - 外壳底部加 Qi 接收线圈

---

## 时间估算

| Phase | 内容 | 时间 |
|-------|------|------|
| 1 | 面包板验证 | 3 天 |
| 2 | hw/ 层代码 | 2 天 |
| 3 | main.cpp 适配 | 1.5 天 |
| 4 | PCB 设计 | 2 天 (+5天等板) |
| 5 | 3D 外壳 | 2 天 |
| 6 | 组装调试 | 1.5 天 |

**总计: ~2 周**

---

## 现在需要确认

1. **屏幕型号?** → 推荐 ST7789 1.54" 240×240 (¥15), 或用 GC9A01 圆屏 (¥20)
2. **ESP32-S3 形态?** → 裸模块 (需焊转接板) 还是已有开发板?
3. **AI 语音模块型号?** → SU-03T / ASRPRO / DFPlayer / 其他?
4. **要不要直接开始面包板?** → 我可以立即帮你写 hw/ 层全部代码

---

## 进度日志

### 2026-06-03 / 06-04 — Web 控制面板最小测试 ✅

**完成内容：**
- [x] WiFi 状态机 (`wifi_manager.h/.cpp`) — 自动连接、重试(3次/15s超时/30s冷却)、异步扫描
- [x] NVS 持久化 WiFi 凭证 (`stats.h` — `w_ssid`, `w_pass` key)
- [x] Async Web 服务器 (ESPAsyncWebServer 3.x, port 80)
- [x] 8 个 REST API: `/api/status`(GET), `/api/display`, `/api/brightness`, `/api/sound`, `/api/name`, `/api/species`, `/api/wifi`, `/api/beep`(POST), `/api/log`(GET)
- [x] mDNS (`deskpet.local`) + CORS 全放通
- [x] 环形日志缓冲区 (100行, 128B/行) → `/api/log`
- [x] BLE 配网命令 `wifi_set` (`xfer.h`)
- [x] WS2812 LED 状态指示 (蓝=等待, 黄=连接中, 绿=OK, 红=失败)
- [x] Web UI 占位页 (`data/web/index.html` → LittleFS)
- [x] 最小化编译 — `build_src_filter` 排除 buddies/character/IMU/ESP-NOW/encoders
- [x] 推送 GitHub (`deskpet-studio`, commit `4c89e01`)

**验证通过：**
- `curl 192.168.31.158/api/status` → 200 JSON ✅
- `curl -X POST .../api/beep -d '{"freq":440,"dur":200}'` → 蜂鸣器响 ✅
- LED 绿色 (WiFi OK) ✅
- 烧录: RAM 24.4% (80KB), Flash 23.4% (1.47MB) ✅

**遇到的问题：**

| 问题 | 原因 | 解决 |
|------|------|------|
| PSRAM init failed → 无限重启 | 实际硬件无 PSRAM，但编译标记了 `BOARD_HAS_PSRAM` | 从 `platformio.ini` 移除 `-DBOARD_HAS_PSRAM` 和 `board_build.psram_type` |
| WiFi+BLE 共存 crash (`abort()`) | `WiFi.setSleep(false)` 禁用了 ESP-IDF 必需的 modem sleep | 删除 `WiFi.setSleep(false)`，让 ESP-IDF 自动管理 |
| ESPAsyncWebServer 库找不到 | 注册表名不同 | 改用 `esp32async/ESPAsyncWebServer @ ^3.7.0` |
| `ArJsonRequestHandler` 未定义 | 3.x API 变化，旧回调签名废弃 | 使用 `server.on(path, method, callback)` + `AsyncCallbackJsonWebHandler` 新 API |
| JSON 反斜杠转义失败 | Bash heredoc / Write 工具 / Python -c 多次消费反斜杠 | 用十六进制常量 `0x5C`(\\)、`0x22`(") 代替字面量 |
| GitHub HTTPS 被阻断 | GFW 封锁 443 端口 | 改用 SSH (端口22) + ED25519 密钥 |
| mDNS `deskpet.local` Windows 无法解析 | Windows 无原生 Bonjour 支持 | 使用 IP 直连 `192.168.31.158` |
| 硬编码 WiFi 密码推上 GitHub | 默认凭证写死在代码中 | amend commit + force push 清除历史 |

**关键认知：**
- **NVS = Flash**，断电数据不丢；ESP32 Preferences 库写的是 Flash 分区
- **WiFi + BLE 共存**时，ESP-IDF 强制要求 WiFi modem sleep 开启；关闭会导致 `abort()`
- ESPAsyncWebServer 3.x JSON body 回调签名为 `void(AsyncWebServerRequest*, JsonVariant&)`
- Windows 环境下工具链对反斜杠的转义处理不一致，用十六进制常量更可靠

**下一步 (按顺序逐步加回模块)：**
1. `hw/input.cpp` — 编码器输入
2. `hw/imu.cpp` — MPU6050 摇晃检测
3. `hw/espnow_bridge.cpp` — ESP-NOW 键盘桥接
4. `ble_bridge.cpp` — BLE NUS (Claude Desktop 连接)
5. `character.cpp` — GIF 角色动画
6. `buddies/` — 18 种 ASCII 宠物
7. 完整 Web UI 仪表盘
