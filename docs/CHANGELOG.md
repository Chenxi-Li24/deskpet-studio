# DeskPet Studio — 开发日志

## 2026-06-05

### EC11 旋转编码器集成
- GPIO1=CLK, GPIO2=DT, GPIO8=VCC(SW复用)
- 旋转中断 (CHANGE)，硬件死区 |delta|<3 过滤噪声
- Web 累加器 `hwEnc1Web()` 防止 500ms 轮询丢失 delta
- VCC 复用按键检测：INPUT 模式 + 3ms 延时 + 3 次采样中值滤波 + 阈值 3500
  - 待解决：电容放电曲线不稳定，需确认模块原理图

### Clawd on Desk ↔ ESP32 双向集成
- `src/esp32-adapter.js`：200ms 轮询 `/api/status`
- 旋转编码器切换主题（阈值 30，约 7-8 detent）
- 按下编码器批准权限请求
- Clawd 状态推送到 ESP32 LED (`POST /api/led`)

### Web 面板优化
- `data/web/index.html` IoT 仪表盘：电池/WiFi/Heap/Uptime/Encoder/Clawd 状态
- 500ms 轮询，cache-busting URL

### 电池检测修复
- 浮空 ADC 检测：波动 >800 或 raw < 200 → pct=-1 (USB 供电)
- ADC 参考电压修正为 3.1V (ESP32-S3)

### UI 优化
- `design-tokens.css`：46 个共享 CSS 自定义属性，品牌色 #d97757
- 毛玻璃效果：bubble、settings、dashboard、session-hud
- 暗色主题 IoT 仪表盘

### 配置
- 自动 compact：800K tokens 阈值
- `.pre-commit-config.yaml` 添加

