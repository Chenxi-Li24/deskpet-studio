# 🐱 DeskPet Studio

> **AI 桌宠工作室** — 从 ESP32 固件到桌面陪伴应用，一站式 AI 桌宠开发仓库。

**DeskPet Studio** is a monorepo for building your own AI desktop pet companion. It spans the full stack: ESP32-S3 firmware with a custom hardware abstraction layer, an Electron desktop companion app with multi-AI-tool integration, and comprehensive planning / reference documentation.

---

## 📁 仓库结构

```
deskpet-studio/
├── firmware/       # ESP32-S3 物理桌宠固件 (PlatformIO / Arduino)
├── app/            # Electron 桌面宠物应用 (Clawd on Desk)
├── reference/      # Anthropic 官方 BLE 桌宠参考实现 (M5StickC)
└── docs/           # 项目方案、硬件选型、物料清单
```

### 🔧 `firmware/` — 物理桌宠固件

自定义 ESP32-S3 固件，支持多种宠物角色动画，通过 BLE 与 PC 端通信。

| 特性 | 详情 |
|------|------|
| **MCU** | ESP32-S3-WROOM-1 (N16R8, 16MB Flash) |
| **显示屏** | ST7789 1.69" 240×280 IPS (P169H002-CTP) |
| **触摸** | CST816T I2C 电容触控 |
| **IMU** | MPU6050 (I2C, 3.3V) |
| **输入** | EC11 旋转编码器 ×2 |
| **灯光** | WS2812-2020 RGB LED ×2 |
| **音频** | 蜂鸣器 + CI1302 语音识别模块 |
| **通信** | BLE (Nordic UART Service) |
| **框架** | PlatformIO + Arduino |
| **角色** | 20+ 宠物 (猫/狗/龙/鸭/鹅/猫头鹰/企鹅/兔/龟等) |

### 🖥️ `app/` — 桌面伴侣应用

Electron 桌面宠物应用，支持多种 AI 编码工具集成 (Claude Code, Cursor, Copilot, Codex, Gemini 等)，通过悬浮动画窗口 + 气泡反馈 + 音效实现实时陪伴体验。

| 特性 | 详情 |
|------|------|
| **框架** | Electron + Node.js |
| **主题** | Calico / Clawd / Cloudling |
| **集成** | Claude Code / Cursor / Copilot / Codex / Gemini / Kimi / Kiro |
| **角色** | 3 个动画主题 + 模板系统 |
| **音效** | 互动音效反馈 |

[→ app/README.md](app/README.md)

### 📖 `reference/` — 官方参考实现

Anthropic 官方 `claude-desktop-buddy` 参考实现，基于 M5StickC Plus (ESP32)，含完整 BLE 协议规范 (`REFERENCE.md`)。

| 特性 | 详情 |
|------|------|
| **来源** | [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy) |
| **平台** | M5StickC Plus (ESP32) |
| **协议** | BLE JSON Lines over Nordic UART |
| **角色** | Bufo (社区青蛙表情包) |
| **许可** | MIT |

[→ reference/README.md](reference/README.md)

### 📐 `docs/` — 项目方案与文档

- `PLAN.md` — 完整项目执行方案（硬件选型、BOM、驱动规划、外壳设计）
- 代码规范配置 (`.commitlintrc.json`, `.clang-format`)

---

## 🚀 快速开始

### 前置要求

- **固件**: [PlatformIO IDE](https://platformio.org/) 或 VS Code + PlatformIO 插件
- **上位机**: [Node.js](https://nodejs.org/) ≥18 + npm
- **硬件**: ESP32-S3 开发板 + ST7789 屏幕 + MPU6050（可选）

### 固件编译 & 烧录

```bash
cd firmware
platformio run --target upload
```

### 桌面应用启动

```bash
cd app
npm install
npm start
```

---

## 🎯 系统架构

```
┌─────────────────────────┐         BLE (JSON Lines)        ┌──────────────────────┐
│   DeskPet App (PC)      │ ◄─────────────────────────────► │   物理桌宠 (ESP32)    │
│   Electron + Node.js    │   角色切换 / 状态同步 / 通知      │   ST7789 + MPU6050   │
│   AI Tool Integration   │                                  │   20+ 宠物角色动画     │
└─────────────────────────┘                                  └──────────────────────┘
```

---

## 📄 许可证

本项目各部分遵循其原始许可证：
- `firmware/` — MIT License
- `app/` — 参见 `app/LICENSE`
- `reference/` — MIT License (Anthropic, PBC 2026)

详见各子目录的 LICENSE 文件。

---

## 🙏 致谢

- [Anthropic](https://anthropic.com) — Claude Desktop Buddy 参考设计
- [Bufo](https://bufo.zone) — 社区青蛙表情包 (reference/characters/bufo/)
- 所有参与开发的 AI 工具和社区

---

<p align="center"><i>Made with ❤️ by Chenxi Li (keyan) · 2026</i></p>
