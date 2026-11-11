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
- 板上 12P CT 接口 CN1（`DB2ERC-2.54-12P`）的通道顺序与固件通道号**相反**：
  ch1=11/12、ch2=9/10、ch3=7/8、ch4=5/6、ch5=3/4、ch6=1/2。
  每通道输入：CN1 脚 → 5.1Ω 到 GND（burden）+ 1kΩ 串联到 BL0906 的 IP/IN。
- `GPIO25` 配置 `INPUT_PULLUP`；应用笔记要求 UART 模式 RX/TX 外部上拉，最终板建议各加 10kΩ 到 3.3V。
- 杜邦线必须插紧；线松时 RX 悬空，只会收到 TX 串扰，不会收到 BL0906 回复。

## 电流互感器（CT）接入

- CT 测的是窗口内所有导体电流的矢量和，**一次只能套一根线**。
- 火线或零线都可以；**不能火线、零线一起套**。正常负载下两者电流大小相等、方向相反，
  一起套会互相抵消，读数接近 0（此时读到的是漏电流）。
- 不要套地线，也不要套整根护套线（里面 L+N 一起）。
- 推荐套火线：零线可能被多个回路共用，套零线会测到其他回路的电流。
- 只能套零线时：确认该零线只属于这一路；功率方向可能反，装好后检查 `Power_N` 符号。
- CT 有 P1/P2 或圆点极性标记；一次电流从 P1 流向 P2 时二次侧输出正方向。
- `Power_N` 为负时翻转 CT，或交换二次侧两根线；板载 CT 只能在软件里取反，
  能量累计要小心。
- `Current_N` 是有效值，不受方向影响；方向只影响功率/电量。
- 一次线只穿一圈；多绕一圈读数按圈数放大，需要重新标定。
- 开合式 CT 必须完全闭合、卡紧、导线居中；缝隙会让读数偏低。
- CT 二次侧不能开路：带电时二次侧必须接着 burden 电阻；可拆 CT 先确认板上
  burden 已接好再套线。
- 六路 CT 优先各套各自回路的火线，不要六路套同一根。
- 市电危险：接线、改线前断电。

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
- 无负载时 `Power_1..6` 可能为 0，但 `Power_sum` 可能显示 1–2W：单路 WATT 寄存器受
  `WA_CREEP`（默认 0x04C，本项目 CT 参数下约 0.89W）防潜动清零，而总功率寄存器 0x2C
  汇总的是清零前的亚阈值噪声。`Power_sum` **不是**六路显示的算术和；能量寄存器仍为 0，
  不会虚计电量。例：空载 0x2C=0x15=21，按 `PREF×16` 换算约 1.96W。
- 空载电流明显高于 0.02–0.04A 时，先查该路 5.1Ω burden 是否装配。缺 burden 时输入浮空，
  接 CT 后可能读到 127A 级的假电流（见“现场状态与已知偏差”）。

## 现场状态与已知偏差（2026-10-29）

- ch1–ch5 已接 CT；ch6 的 R21/R22（5.1Ω burden）未装配，输入已在 CN1 侧短接到 GND 作停用处理。
  以后启用 ch6：先拆短接、补 R21/R22，再接 CT；短接状态下误接 CT 不会伤芯片，但读数恒为 0。
- 缺 burden 的通道接 CT 时输入近似开路：曾出现 `Current_6` 127A、`Power_6` -15558W，
  `Total_Power` 被 `WATT_SUM` 带到 +15558W。短接后 `Current_6` 0.033A，与其他空载通道一致；
  R23/R24（1k）复测仍为 1k，芯片输入未受损。
- 未启用软件校准：`setup()` 里的 `Bias_correction` / `gain_correction` 仍全部注释。
- 与商用插座电量计对比：功率稳定偏高约 **3.7%**（968 vs 927–939W；1629 vs 1570W），
  电压准确（220.9 vs 221.5V，-0.27%），两种负载比例一致，倾向 CT 比值容差，
  暂不校准。`Energy_*` 会同比偏高约 3.7%，与电费对账时注意。
- 以后校准需要可信基准（万用表 + 钳形表，或已知阻值负载），分别校 `WATTGN`（功率/电量）
  和 `RMSGN`（电流）。当前是读数偏高、增益为负，现有 `gain_correction()` 的
  `data.h = 0xFF` 恰好适用于负增益。

## BL0906 电流校正

### 先判断要不要校

- 出厂增益误差 <1%；外围器件 1% 时系统误差 <2%。明显超出这个范围才考虑校正。
- 先排除误判：
  - 参考表在 0.15A 左右小电流下的准确度；LED 灯泡电流非正弦，非真有效值的表会读偏。
  - 空载偏置：该通道空载有 0.02–0.04A 底数，负载读数里包含它。
  - 对比 `Power_N`：功率也同比例偏低更像 CT/增益问题；只有电流偏低更像参考表或波形问题。
- 建议用 1–2kW 纯阻性负载（电热丝/白炽灯）复核；阻性负载下差 <2% 就
  不要按 LED 灯泡的数据校正。

### 寄存器

| 寄存器 | 位宽/符号 | 地址（通道 1–6） | 公式 |
|---|---|---|---|
| `RMSGN[N]` | 16 位有符号 | 0x6D/0x6E/0x6F/0x70/0x73/0x74 | `I_corr = I_raw × (1 + RMSGN/65536)` |
| `RMSOS[N]` | 24 位有符号 | 0x78/0x79/0x7A/0x7B/0x7E/0x7F | `I_corr² = I_raw² + RMSOS×256` |
| `WATTGN[N]` | 16 位有符号 | 0xB7/0xB8/0xB9/0xBA/0xBD/0xBE | 功率增益，单独校 |

- 写入前 `0x9E=0x5555`，写完 `0x9E=0x0000`；`setup()` 已包含这两步。
- 电流校正只影响 `Current_N`；`Power_N`/`Energy_N` 走 `WATTGN`，要单独校。
- 功率可用同样的标准负载法：`gain_correction(BL0906_WATTGN_1, P_raw, P_ref, BL0906_Kp)`；
  阻性负载可参考 `P=V×I`，LED 有功率因数，不能直接用 `V×I`。

### 步骤

1. 确认负载接在哪一路（看哪个 `Current_N` 明显上升）。
2. 关负载，记录该通道稳定空载 `I0`；确认 `RMSGN`/`RMSOS` 为 0
   （默认值；写过就先写 0 或断电重启）。
3. 开负载，记录 BL0906 `I_raw` 和参考表 `I_ref`。
4. 计算：

   ```
   I1 = sqrt(I_raw² − I0²)
   RMSGN = (I_ref/I1 − 1) × 65536
   ```

   例：`I0=0` → `RMSGN=9089 (0x2381)`；`I0=0.0196` → `9865 (0x2689)`；
   `I0=0.0365` → `11887 (0x2E6F)`。必须用**该通道**的 `I0`。
5. 在 `setup()` 的 `USR_WRPROT_Witable` 和 `USR_WRPROT_Onlyread` 之间启用：

   ```cpp
   Bias_correction(BL0906_RMSOS_1, I0, 0);
   gain_correction(BL0906_RMSGN_1, I1, I_ref, BL0906_ki);
   ```

6. 编译烧录；空载应接近 0，带载应接近 `I_ref`；再用 >1A 的第二个点复核线性。
7. 如果空载还有一点底数，按复测值微调一次 `RMSOS`。

### 代码注意

- `gain_correction()` 现在无条件写 `data.h = 0xFF`。读数偏低时 `RMSGN` 为正，
  若芯片按 24 位有符号解释会变成负数；应改成
  `data.h = (value < 0) ? 0xFF : 0x00;`。
- `Bias_correction()` 在 `value >= 0` 时没有给 `data.h` 赋值；建议统一为
  `data.h = (value >> 16) & 0xFF;`。
- `Bias_correction()` 里的 `ESP_LOGW` 会让启动日志出现 W；启用时改成
  `ESP_LOGI`/`ESP_LOGD`。
- 启用后校正值由 `setup()` 每次上电写入；改了 `BL0906_IREF` 或 CT/burden 后要重新标定。

## 电量实体迁移到新设备（HA 统计校准）

换 ESP32（MAC 变）、换 BL0906 或旧设备退役后，新实体在 HA 里是全新的：
`state_class: total_increasing` 会把新实体第一次出现的读数当作零点，长期统计的
`sum` 从 0 开始，旧表的累计量不会自动带过来。只要仪表盘总量连续，用 HA 的
统计校准（Adjust sum），不要给原始计数器加固定 offset。

### 前提与事实

- 电量读数是 BL0906 芯片内的 `CF_N_CNT` / `CF_SUM_CNT`，不是旧固件保存的值。
  同一块 BL0906 不断电，换 ESP32/重烧固件读到的还是同一个累计值；芯片断电则清零。
- `CF_CNT` 是 24 位无符号：单路满量程约 3739.85 kWh，`Energy_sum` 约 59837.65 kWh。
  单路一天 10 kWh 时约一年回绕一次。回绕和掉电清零在数值上不可区分。
- 新旧驱动的换算系数一致（内置 `BL0906_CF` 与社区 `BL0906_CF_SUM_REF` 同式），
  同芯片切换固件不会出现量级跳变。
- 社区组件 `custom/bl0906_community/sensor.py` 的 `Energy_*` 默认
  `state_class: total`；YAML 里的每个 `Energy_*`/`Energy_sum` 必须显式写
  `total_increasing`，不要删。`total` 会把下降记成负增量，污染累计。
- HA 的 `total_increasing`：新实体第一次记录时以当时读数为零点，`sum` 从 0 开始；
  下降超过 10% 视为换表/清零（零点按 0 计，`sum` 继续累加），下降 ≤10% 按负增量
  处理（`sum` 会往回掉）。因此原始计数器的掉电/回绕可以由 HA 兜住，但**不要**用
  常量 offset 的模板传感器直接做 Energy 源——复位时会制造假增量或负增量。
- 同一块 ESP32 + 逐字相同的实体名时，HA 的 entity unique_id 不变（ESPHome 用
  MAC + 域 + object_id），会直接复用旧实体，这种情况不需要下面的校准。

### 操作

1. 旧设备先停更，并从 Energy Dashboard 移除它的源，避免切换期双计。
2. 记录旧实体**最新长期统计行的 `sum`**（不是 `state`；掉电/回绕过的话 state 会比
   sum 小），并记下切换时间 T。
3. 新实体上线后等第一条长期统计（最多约一个整点）。
4. HA → 设置 → 开发者工具 → 统计，找到新实体，点右侧 **Adjust sum**。
5. 时间选 T（或新实体第一条统计的时间），把该点的累计 `sum` 调到旧表的 `sum`：
   对话框填增量时填 `+旧sum`，填目标值时直接填旧 `sum`。
6. Energy Dashboard 只保留新实体作为该回路的源。

脚本化可用 `recorder.adjust_statistics` 服务（字段按 HA 版本：
`statistic_id`/`entity_id` + `start_time` + `adjustment`）。

### 验证

- 新实体最新 `sum − 旧sum` ≈ 切换后的实际用电量。
- 人为重启/断电 BL0906 一次，`sum` 不应下降；`state` 归零或回绕是正常的。
- 分路 `Energy_1..4` 如果要继承，各自单独 adjust；不要假设分路之和等于总表
  （`CF_SUM_CNT` 是芯片内部汇总，不是 ESPHome 侧把四路相加）。

### 注意

- 操作前备份 `home-assistant_v2.db`；不要用 SQL 改单行，官方 adjust 会同步后续行。
- 只调一次；新旧设备有重叠记录时选对 T，否则重叠期双计。
- adjust 按数据库存储单位计算，能量是 kWh，不做换算。
- 旧实体 unique_id 若是旧版 ESPHome 的“设备名开头”格式，新固件会生成新实体；
  这正是本方案的适用场景，不要试图靠同名让 HA 复用。

## 其他 ESPHome 配置分析

范围仅限：`powermeter-pzem004t.yaml`、`retired/powermeter.yaml`（已退役）、
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

### retired/powermeter.yaml（已退役）

- 已移至 `retired/`，不再烧录，仅作参考。文件里的 `!include common/base.yaml`、
  `!include common/mqtt.yaml` 是相对路径，移动后无法解析；重新启用前需先移回
  根目录（或改写成 `../common/...`）。
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
| retired/powermeter.yaml | 通过 | LT518 + MQTT/Domoticz；已退役，结果为移动前校验 |
| scale_livingroom_ble.yaml | 失败 | 缺根目录 `secrets.yaml` / `scale_bt_mac` |
| weatherstation_gongzuojian.yaml | 通过 | 软链接组合完整（bmp280 + dht11 + hum_stat） |
| weatherstation_livingroom.yaml | 通过 | 软链接组合完整（bme280 + hum_stat） |
| weatherstation_zhuwo.yaml | 通过 | 软链接组合完整；已编译成功 |
| weatherstation_livingroom_ble.yaml | 失败 | 缺 `lywsd02_bt_mac` 对应的 secrets |

## 其他约定

- `common/secrets.yaml` 含 WiFi 凭据，不要输出、不要提交内容。
- `common/base.yaml` 被所有设备共用，修改前确认影响范围。
- 未经用户明确要求不要 commit/push。
