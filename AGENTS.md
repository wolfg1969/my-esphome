# AGENTS.md

本仓库是家庭环境使用的 ESPHome 配置集合，包含多个 ESP32/ESP8266 设备，
以及 `custom/bl0906_community` 下的自写 BL0906 多路电量表组件。

## 环境与常用命令

使用仓库内虚拟环境，不要依赖全局 Python：

```bash
.venv/bin/esphome config  powermeter-belling-bl0906-community.yaml
.venv/bin/esphome compile powermeter-belling-bl0906-community.yaml
.venv/bin/esphome run     powermeter-belling-bl0906-community.yaml
.venv/bin/esphome logs    powermeter-belling-bl0906-community.yaml --device <ip>
```

烧录前确认没有其他 `esphome` 进程占用 `/dev/ttyUSB0`。

## BL0906 硬件约定

- BL0906 板由 HLK-PM03 隔离 AC-DC 供电，**市电侧危险**；改线、测量、插拔前先断电。
- ESP32 与 BL0906 必须共地。
- 测试用 ESP32 开发板通过 USB 供电，不接 BL0906 板的 3.3V（除非最终板明确由 H1.2 供电）。
- 信号方向：
  - ESP32 `GPIO26` (TX) → BL0906 RX/SDI (H1.3)
  - ESP32 `GPIO25` (RX) ← BL0906 TX/SDO (H1.4)
- 不要用 `GPIO1/GPIO3`：它们是 UART0，被日志和 USB 串口占用。
- UART：19200 8N1。
- `GPIO25` 配置 `INPUT_PULLUP`；应用笔记要求 UART 模式 RX/TX 外部上拉，最终板建议各加 10kΩ 到 3.3V。
- 杜邦线必须插紧；线松时 RX 悬空，只会收到 TX 串扰，不会收到 BL0906 回复。

## BL0906 驱动架构（custom/bl0906_community）

- 非阻塞：`update()` 只启动一轮读取，`loop()` 状态机分步执行（IDLE/SEND/WAIT）。
- 每条寄存器：发送 `0x35 + 地址`，等待 4 字节 `L / M / H / SUM`。
- 校验和：`(地址 + L + M + H) ^ 0xFF`。
- 功率/总功率为有符号 24 位；电流/电压/电量为无符号 24 位。
- 只读取 YAML 中配置了传感器的寄存器。
- 关键参数：单条超时 150ms；正常帧间隔 2ms；失败重同步 60ms；每条最多重试 2 次。
- **不要恢复阻塞式 `read_array`**；不要在读完回复后立刻发下一条命令。
- 解析必须用逐字节滑动窗口，只接受校验和匹配的 4 字节帧；不要“凑够 4 字节就信”。
- 失败时先重试同一条寄存器，排空 RX 后再重发。

## 调试流程

BL0906/UART 问题先读 skill：`.claude/skills/bl0906-uart-debug/SKILL.md`。

1. 抓完整一轮日志（60s 周期），不要只看开头几行。
2. 按日志分类：
   - 完全没有 `0xXX = ...` → 读路径不通。
   - `Checksum mismatch` 与 `Timeout` 交替 → 回复迟到/错位。
   - `Timeout ... available=0` → 注意：消费型滑动窗口下不能证明没信号。
   - `rx_bytes=0` → 真的没收到字节。
   - `rx_bytes>0` 且 `last=` 是命令地址字节的移位 → TX 串扰，RX 没被 BL0906 驱动。
   - `last=00 FF ...` → 悬空/噪声。
3. 需要时加字节计数/最后字节诊断；每次发送前清空窗口。
4. 连接层验证：断电测通断、共地、3.3V；回环测试（短接 GPIO26/GPIO25，期望 `rx_bytes=2` 且 `last` 中出现 `35 XX`）。
5. 修改后先 `esphome compile` 通过，再让用户烧录验证。
6. 验证标准：所有配置寄存器各出现一次 `0xXX = ...`，无 W 警告。

## 正常空载读数参考

- 频率约 50Hz，电压约 220–230V，温度约 17–20°C。
- 无负载时 `Power_*` / `Energy_*` 约 0。
- 六路电流可能有 0.02–0.04A 的空载噪声底，不是故障。

## 其他 ESPHome 配置分析

范围仅限：`powermeter-pzem004t.yaml`、`powermeter.yaml`、
`scale_livingroom_ble.yaml`、`weatherstation*.yaml`。

### 共同点

- 都通过 `common/base.yaml` 复用 WiFi（`common/secrets.yaml`）、API、OTA、日志、
  WiFi 信息 text sensor 和重启按钮；`use_address` 已注释。
- `!secret` 按引用它的文件所在目录查找 `secrets.yaml`，不会自动回退到
  `common/secrets.yaml`。根目录文件和深层 include 里的 `!secret` 需要自己能找到
  对应的 `secrets.yaml`。
- `esphome config` 只做配置校验；lambda 里的 `id(...)` 要到编译期才解析，
  “config 通过”不等于“能编译”。
- `weatherstation/<room>/sensors/` 和 `.../automations/` 用**软链接**组合共享的
  `sensors/*.yaml`、`automations/*.yaml`。`find -type f` 看不到这些文件，检查时用
  `find -type l` 或 `ls -la`。

### powermeter-pzem004t.yaml

- ESP8266 `nodemcuv2` + Modbus RTU；UART0：GPIO1/GPIO3，9600。
- `logger.baud_rate: 0` 关闭串口日志，把 UART0 让给 Modbus。
- 寄存器：电压 0x0000（×0.1）、电流 0x0001（32 位，×0.001）、
  功率 0x0003（32 位，×0.1）、电量 0x0005（32 位，Wh）、
  频率 0x0007（×0.1）、功率因数 0x0008（×0.01）。
- 32 位电流/功率/电量使用 `U_DWORD_R`（字序反转）。
- 复位电量是 PZEM 私有帧 `01 42 a8 00`，`write_lambda` 返回 `false`
  以阻止默认 Modbus 帧。

### powermeter.yaml

- ESP8266 `nodemcuv2` + LT518 电表（Modbus RTU）；UART 使用
  GPIO15(TX)/GPIO13(RX)，4800，`logger.baud_rate: 0`。
- Modbus 地址 0x1，30s 更新。
- holding 寄存器：电压 0x0048、电流 0x0049、功率 0x004A、
  电量 0x004B（kWh）、功率因数 0x004C、CO2 0x004D；
  使用 `U_DWORD`，系数多为 0.0001（PF 为 0.001）。
- 复位电量是 LT518 私有帧 `01 4A 53 59 03 85 98`。
- 通过 MQTT 向 Domoticz 发布 `domoticz/in`（idx 94）；
  `mqtt_client` 来自 `common/mqtt.yaml`（`discovery: False`）。

### scale_livingroom_ble.yaml

- ESP32 `denky32`，ESP-IDF，BLE 小米体重秤 V2（`xiaomi_miscale`）。
- 两个用户区间：Yong 60–80kg，Yan 40–60kg。体重落在哪个区间，就把体重/阻抗
  写入对应用户的 template number/sensor；用 HA 时间记录最近测量时间。
- 设备名 `scale-ble-livingroom`。
- `mac_address: !secret scale_bt_mac` 在根目录文件里，ESPHome 会找根目录
  `secrets.yaml`；当前仓库没有根 `secrets.yaml`，`common/secrets.yaml` 也没有
  `scale_bt_mac`，所以 `esphome config` 会失败。
- 使用前需要补根目录 `secrets.yaml`（或把该 secret 放到能被找到的位置）。

### weatherstation*.yaml

四个文件共用 `weatherstation/base.yaml`：ESP8266 `nodemcuv2`，
`common/base.yaml` + MQTT + I2C（D1/D2），并每 60s 通过
`mqtt_client` 向 Domoticz 发布 `temp;hum;hum_stat;bar;0`。
`sensor`/`interval` 由 `!include_dir_list` 从各房间目录加载。

| 文件 | 平台/传感器 | Domoticz idx | 房间目录内容（软链接） |
|---|---|---|---|
| weatherstation_gongzuojian.yaml | ESP8266 + BMP280/DHT11 | 92 | work_room: bmp280 + dht11 + hum_stat；thermometer |
| weatherstation_livingroom.yaml | ESP8266 + BME280 | 95 | living_room: bme280 + hum_stat；thermometer |
| weatherstation_zhuwo.yaml | ESP8266 + BMP280/DHT11 + PMS5003 | 91/96/97 | bed_room: bmp280 + dht11 + hum_stat + pms5003；thermometer + air_quality |
| weatherstation_livingroom_ble.yaml | ESP32 lolin32_lite + BLE LYWSD02 | 90 | living_room_ble: 实际文件（非软链接） |

- 三个 ESP8266 配置通过软链接组合出完整传感器：
  - `work_room`：bmp280（temp/bar）+ dht11（hum）+ hum_stat + thermometer
  - `living_room`：bme280（temp/bar/hum）+ hum_stat + thermometer
  - `bed_room`：bmp280 + dht11 + hum_stat + pms5003 + thermometer + air_quality

  因此 `weatherstation/base.yaml` 的 60s lambda 所需的
  `${devicename}_${location_id}_temp/hum/hum_stat/bar` 都有定义。
  `weatherstation_zhuwo.yaml` 已实际 `esphome compile` 通过。
- `weatherstation_zhuwo.yaml` 额外定义 `uart: rx_pin D6, 9600` 和 D0 的
  `pms_set` 开关；`air_quality.yaml` 每 1800s 打开 PMS5003、等 20s 后发布
  PM2.5/PM10 到 idx 96/97，再关闭。
- `weatherstation_livingroom_ble.yaml` 使用
  `weatherstation/living_room_ble/sensors/xiaomi_lywsd02.yaml`（LYWSD02
  temp/hum/battery），60s 发布到 Domoticz idx 90。该 sensor 文件里的
  `!secret lywsd02_bt_mac` 需要同目录或根目录 `secrets.yaml`；当前都没有，
  所以 `esphome config` 失败。
- `weatherstation/bme280.yaml`、`weatherstation/bmp280_dht11.yaml` 是
  `weatherstation/` 下的旧副本；房间软链接指向根目录 `sensors/` 下的文件，
  不是这两个。

### 配置校验结果（2026-10-24）

| 文件 | `esphome config` | 说明 |
|---|---|---|
| powermeter-pzem004t.yaml | 通过 | 标注 not used |
| powermeter.yaml | 通过 | LT518 + MQTT/Domoticz |
| scale_livingroom_ble.yaml | 失败 | 缺根目录 `secrets.yaml` / `scale_bt_mac` |
| weatherstation_gongzuojian.yaml | 通过 | 软链接组合完整（bmp280 + dht11 + hum_stat） |
| weatherstation_livingroom.yaml | 通过 | 软链接组合完整（bme280 + hum_stat） |
| weatherstation_zhuwo.yaml | 通过 | 软链接组合完整；已编译成功 |
| weatherstation_livingroom_ble.yaml | 失败 | 缺 `lywsd02_bt_mac` 对应的 secrets |

## 其他约定

- `common/secrets.yaml` 含 WiFi 凭据，不要输出、不要提交内容。
- `common/base.yaml` 被所有设备共用，修改前确认影响范围。
- 未经用户明确要求不要 commit/push。
