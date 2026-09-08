# Panther X2（黑豹X2）NPU + LoRa(SX1302) 驱动 Bring-up 记录

> Rockchip RK3566 / Armbian 6.1.57-rk35xx-hiasia 上，成功驱动 **RK3566 NPU** 与板载 **Semtech SX1302 LoRa 集中器**，并找到/读取了板载 **SHT2x 温湿度传感器** 与 **ATECC608**。

## 1. 设备与环境

| 项 | 值 |
| --- | --- |
| 设备 | Panther X2 / 黑豹X2（Helium 矿机版），Rockchip RK3566, 4GB RAM, eMMC+TF |
| 固件 | **`Armbian_24.8.0_rockchip_panther-x2_bullseye_6.1.57_server_2024.07.09.img`** |
| 系统 | Armbian-unofficial 24.8.0-trunk bullseye, 内核 `6.1.57-rk35xx-hiasia` |
| DT | `rk3566-panther-x2.dtb`（`/boot/dtb/rockchip/`），boot 方式 `fdtfile=rockchip/rk3566-panther-x2.dtb` |
| SSH | root@192.168.2.120 |

> 本仓库所有改动均基于上述 `.img` 固件验证/记录。

## 2. 成果摘要

### 2.1 NPU（RK3566, 0.8 TOPS）— 可用 ✅
- 内核已内建 `rknpu` 驱动（v0.9.6），以 DRM 方式注册为 `/dev/dri/renderD129`（**注意：没有传统 `/dev/rknpu` 节点，官方运行时依然可用**）。
- 装入与驱动匹配的官方运行时：`librknnrt.so`（RKNN **2.3.2**）到 `/usr/lib`。
- 实测 mobilenet_v1 推理：`rknn_run` 成功，**16.3 ms**，输出正常。

### 2.2 LoRa 集中器（Semtech SX1302）— 可用 ✅
- 硬件位置：**SPI3（fe640000）的 m0 引脚组 / CS0** → `/dev/spidev3.0`。
  - ⚠️ 出厂 Armbian 的 DT 把 spi3 配成了 **m1** 引脚组，集中器完全不响应（读回 0xFF）；切到 **m0** 后正常。
- 复位/供电 GPIO：**RESET=120（GPIO3_24），POWER_EN=129（GPIO4_1）**，上电后需给模块供电再复位。
- 射频方案识别为 **SX1250**（HAL 需用 `-r 1250`，不能用 1257）。
- HAL：官方 `Lora-net/sx1302_hal` 2.x 在启动时会**强依赖 I2C 温度传感器（stts751）**，本机没有 → 用专为该机修改的 fork **`onicolaos/pantherx2_sx1302_hal`**（可容忍无 stts751、支持 SHT2x）。
- 验证结果（`lora_pkt_fwd`）：
  ```
  INFO: [main] concentrator started, packet can now be received
  INFO: concentrator EUI: 0x0016c001f15574b1
  # RF packets received by concentrator: 0
  ```

### 2.3 板载传感器 — 可用 ✅
- **SHT2x 温湿度**：挂在 **i2c-2 / 0x40**（i2c2 控制器，**m1 引脚组 GPIO4_12/13**）。⚠️ 传感器在 LoRa 模块上，**必须先给模块上电（GPIO129=1）才能扫到**。实测 37.3°C / 43.7%RH。
- **ATECC608 加密芯片**：i2c-1 / 0x60。

## 3. 目录结构

```
.
├── README.md              本文档
├── npu/
│   ├── npu_probe.c        NPU 推理探针源码（加载模型→init→推理→打印版本/耗时）
│   ├── npu_probe          aarch64 编译产物
│   ├── rknn_api.h         RKNN2 官方头文件（编译 npu_probe 需要）
│   └── mobilenet_v1.rknn  RK3566/RK3568 测试模型（来自 rknn-toolkit2 v2.3.2）
├── lora/
│   ├── global_conf.json   lora_pkt_fwd 配置（sx1250·EU868 示例，spidev=/dev/spidev3.0）
│   ├── lora_pkt_fwd       packet forwarder（fork 编译）
│   ├── chip_id            chip_id（fork 编译；带 -r 1250 使用）
│   ├── reset_lgw.sh       HAL 使用的复位脚本（GPIO120/129）
│   ├── reset_good.sh      复位脚本纯文件备份
│   └── sht_read.py        SHT2x 温湿度读取脚本（i2c-2 / 0x40）
├── system/
│   ├── led-fix.sh           LED 灯行为开机自启脚本（eth/wifi 跟随链路、status 心跳）
│   └── led-fix.service      对应 systemd 单元
└── dtb/
    ├── dtb-original-st7789v-m1.dtb    出厂原始 dtb（备份，spi3=m1 + st7789v 屏）
    ├── dtb-spi3-m0-cs0-spidev.dtb     ① spi3→m0 + spidev@0（LoRa 可访问）
    ├── dtb-final-spi3-m0+i2c2-m1.dtb  ② 最终：① + i2c2→m1 启用（SHT2x 可用）
    ├── patch_spi3_m0cs0.py            dtb 补丁脚本（decompile→改 dts→dtc）
    └── patch_enable_i2c2_m1.py        i2c2 使能 + m1 引脚补丁脚本
```

## 3.5 LED 灯行为修正（板载 4 颗灯）

出厂 DTB 的 `gpio-leds` 定义了 4 颗灯，其中 **led-eth / led-wifi 默认 `default-state = "on"`（被强制常亮）**，导致"网线没插网口灯也亮、WiFi 没连灯也亮"。

| 面板灯 | 出厂行为 | 修正后行为 | 触发器 |
| --- | --- | --- | --- |
| led-pwr（电源） | 常亮 | 常亮 | none |
| led-status（状态） | 规律呼吸 | 规律呼吸（系统正常指示） | `heartbeat` |
| led-eth（网口） | **强制常亮** | **插网线亮、拔线灭** | `netdev` + eth0 |
| led-wifi（WiFi） | **强制常亮** | **连上 AP 亮、断开灭** | `netdev` + wlan0 |

> 注：系统里的 `mmc0::` 是 eMMC 控制器的活动指示，**未接任何实体 GPIO**，面板上没有独立的"硬盘灯"。

修正通过开机自启脚本实现（`/usr/local/sbin/led-fix.sh` + systemd 服务 `led-fix.service`），**重启自动生效**，无需改 dtb。

```bash
# 安装/启用（设备上）
cp system/led-fix.sh /usr/local/sbin/led-fix.sh && chmod +x /usr/local/sbin/led-fix.sh
cp system/led-fix.service /etc/systemd/system/
systemctl daemon-reload && systemctl enable --now led-fix.service
```

## 4. 复现步骤

### 4.1 NPU
```bash
# 1) 安装运行时（与内核驱动 0.9.6 匹配）
#    librknnrt.so 取自 airockchip/rknn-toolkit2 v2.3.2:
#    rknpu2/runtime/Linux/librknn_api/aarch64/librknnrt.so
cp librknnrt.so /usr/lib/ && ldconfig

# 2) 编译探针（需 rknn_api.h）
gcc -O2 -o npu_probe npu_probe.c -I. -L/usr/lib -lrknnrt

# 3) 实测推理
./npu_probe mobilenet_v1.rknn
# 预期：rknn_init=0, sdk 2.3.2 / driver 0.9.6, rknn_run elapsed≈16ms
```

### 4.2 LoRa —— 定制 dtb（改引导文件，可回退）
对设备树做两处修改并重编译（备份在 `/root/dtb-backup/`）：
1. `spi@fe640000`：pinctrl 由 m1(`<0xda 0xdb>`) 改为 m0(`<0xdc 0x2a3>`)；删除 `st7789v@0`，加入：
   ```dts
   spidev@0 { compatible = "rockchip,spidev"; reg = <0x00>; spi-max-frequency = <8000000>; };
   ```
2. `i2c@fe5b0000`(i2c2)：`status="okay"`，pinctrl-0 由 m0(`<0xca>`) 改为 m1(`<0x231>`)（SHT2x 挂在 m1）。
```bash
dtc -I dtb -O dts rk3566-panther-x2.dtb > p.dts   # decompile
# ...编辑（见 dtb/patch_*.py）...
dtc -I dts -O dtb -o rk3566-panther-x2.dtb p.dts
cp /boot/dtb/rockchip/rk3566-panther-x2.dtb /root/dtb-backup/   # 先备份
cp 新dtb /boot/dtb/rockchip/rk3566-panther-x2.dtb && reboot
```

### 4.3 LoRa —— 编译 HAL（fork）+ 运行
```bash
git clone https://github.com/onicolaos/pantherx2_sx1302_hal
cd pantherx2_sx1302_hal
make -C libtools && make -C libloragw libloragw.a && make -C util_chip_id && make -C packet_forwarder

# 复位脚本放二进制同目录（RESET=120 / POWER_EN=129）
cp tools/reset_lgw.sh util_chip_id/ && cp tools/reset_lgw.sh packet_forwarder/

# 1) 读芯片/EUI（射频用 sx1250）
cd util_chip_id && ./chip_id -d /dev/spidev3.0 -r 1250
# 预期：INFO: concentrator EUI: 0x0016c001f15574b1

# 2) 跑 packet forwarder（config: com_path=/dev/spidev3.0）
cd ../packet_forwarder && ./lora_pkt_fwd -c global_conf.json
# 预期：INFO: [main] concentrator started, packet can now be received
```

> `global_conf.json` 内的 radio/频点来自 `global_conf.json.sx1250.EU868` 示例；
> 若需正式接入 LoRaWAN，请按你所在地区替换为对应的 region 配置并填写 server_address。

### 4.4 温湿度
```bash
# 先给 LoRa 模块上电
echo 129 > /sys/class/gpio/export; echo out > /sys/class/gpio/gpio129/direction; echo 1 > /sys/class/gpio/gpio129/value
# 读 SHT2x（i2c-2 / 0x40）
python3 sht_read.py
# Temperature: 37.28 C / Humidity: 43.71 %RH
```

## 5. 踩坑要点（备忘）
- spi3 出厂配 m1 引脚 → SX1302 完全不应答（0xFF）；**改 m0**。
- HAL 2.x 默认假设 sx1250 之外的射频 → 校准失败；此机按 **SX1250** 处理。
- HAL 启动要求 stts751 I2C 温控，板上没有 → 必须用 `onicolaos/pantherx2_sx1302_hal` fork（或自行 patch 掉该硬依赖）。
- SHT2x 在 LoRa 模块上，**模块断电时 i2cdetect 扫不到且 i2c 超时**——先上电。
- 本机无 `/dev/rknpu`，仅 `/dev/dri/renderD129`；用官方 librknnrt 2.3.x 实测 NPU 推理正常。
- 该机在编译等高负载下偶发自动重启（电源余量问题），长任务建议降低 make 并发。

## 6. 备份与还原
- 设备上已存：`/root/dtb-backup/`（`.orig` / `.m0cs0` / `.m0cs0-i2c2m1`）
- 还原出厂：
  ```bash
  cp /root/dtb-backup/rk3566-panther-x2.dtb.orig /boot/dtb/rockchip/rk3566-panther-x2.dtb
  ```
- 工具目录（设备上）：`/root/gw/`（lora_pkt_fwd / chip_id / reset / global_conf）、`/root/npu-test/`、`/root/sht_read.py`、HAL 源码 `/root/pantherx2_sx1302_hal-master/`

## 7. 参考
- Armbian 设备树: `https://github.com/armbian/build` (RK3566 panther-x2)
- 官方 SX1302 HAL: `https://github.com/Lora-net/sx1302_hal`
- Panther X2 专用 HAL fork: `https://github.com/onicolaos/pantherx2_sx1302_hal`
- RKNN 运行时: `https://github.com/airockchip/rknn-toolkit2` (v2.3.2, librknnrt for RK356x)
- 相关讨论: `ophub/amlogic-s9xxx-armbian` issues #1319 / #2154；Lora-net/sx1302_hal issues #66/#67
- Helium/Panther X2 介绍: `https://github.com/yjdwbj/panther-x2`
