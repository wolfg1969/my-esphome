# CLAUDE.md

本仓库是家庭 ESPHome 配置集合。开始工作前先读 [AGENTS.md](AGENTS.md)；
调试 BL0906/UART 时加载项目 skill `bl0906-uart-debug`
（`.claude/skills/bl0906-uart-debug/SKILL.md`）。

## 环境

使用仓库内虚拟环境：

```bash
.venv/bin/esphome config  powermeter-belling-bl0906-community.yaml
.venv/bin/esphome compile powermeter-belling-bl0906-community.yaml
.venv/bin/esphome run     powermeter-belling-bl0906-community.yaml
.venv/bin/esphome logs    powermeter-belling-bl0906-community.yaml --device <ip>
```

## 硬性规则

- 修改 BL0906 驱动后必须用 `esphome compile` 编译通过，再让用户烧录。
- 驱动保持非阻塞和逐字节滑动窗口；不要恢复阻塞读，也不要“凑够 4 字节就信”。
- `GPIO25` 保持 `INPUT_PULLUP`；最终板在 RX/TX 上各加 10kΩ 外部上拉。
- 不输出、不提交 `common/secrets.yaml`。
- 市电危险：HLK-PM03 市电侧断电后再改线或测量。
- 未经用户明确要求不要 commit/push。
- 遇到 UART 异常先按 skill 的日志分类表判断，不要先假设是电路设计问题。

## 本项目已验证的行为

- 23 个寄存器一轮约 0.46s，60s 轮询正常。
- 同一固件在商用开发板正常；自制板曾因杜邦线松动，RX 只收到 TX 串扰。
- HA 发现异常时先查 mDNS、6053 端口和 `esphome logs`，再在 HA 里按 IP 手动添加。
