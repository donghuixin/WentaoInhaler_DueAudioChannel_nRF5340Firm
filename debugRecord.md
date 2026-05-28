# debugRecord：MLX90642 检测不到（OpenEarable v2 / nRF5340）

## 背景与目标

- **现象**：I2C 扫描能看到部分设备（如 ADAU1860），但 **MLX90642（热成像 32×24）始终 NACK**，`0x66/0x33` 都扫不到。
- **目标**：让 MLX90642 在 OpenEarable v2 上稳定上电、可被扫描/驱动，并明确“扫不到”的真实根因（软件 / 电源 / 电平转换 / 走线）。

## 关键硬件拓扑（用户确认）

- MLX90642 的 SDA/SCL 并非直连 MCU：中间有 **PCA9306DCUR 双向 I2C 电平转换器**。
- **PCA9306 EN 与 VREF2 连接**（常见做法：EN 与 VREF2 短接后一起上拉）。
- 系统同时存在电源管理链路：
  - BQ25120A 输出（VOUTLS）→ 作为 3V3 轨来源的一部分
  - 用户新增/使用 **SGM2036 3.3V LDO** 给 MLX90642 / PCA9306 高侧供电（VREF2 域）

## 数据手册核对要点

### MLX90642（Melexis）

- **默认 I2C 地址**：`0x66`；若 EEPROM 中 SA 字节为 `0x00`，设备会响应 **`0x33`**（手册 Note）。
- **POR / 首帧有效数据时间**：
  - \(T_{valid\_data} = 10ms + 70ms + RT\)
  - RT 与刷新率有关，2Hz 最坏 RT=500ms → 最坏约 **580ms**
- **I2C 输入阈值默认是 VDD 参考（3.3V）**：
  - 默认 VIH≈0.7×VDD≈2.31V（若想 1.8V I2C 需配置 EEPROM `0x11FC` 的阈值参考位）

### PCA9306（TI）

- PCA9306 **不是“主动驱动高电平”**，I2C 高电平来自两侧各自的上拉电阻。
- **最关键条款（Power Supply Recommendations / Correct Setup）**：
  - **VREF2 必须通过高阻电阻（典型 200kΩ）连接到 VDPU（3.3V 轨）**
  - EN 与 VREF2 需要短接，并通过同一个高阻上拉到 VDPU
  - 若 VREF2 直接短接到 3.3V（没有 200kΩ），会导致 **过流/不可靠/甚至损坏 pass FET**，表现为 NMOS 半导通、SDA2/SCL2 静态电压异常（常见 ≈2.5V）

## 软件侧（Zephyr / OpenEarable）关键改动与诊断增强

### 1) PMIC（BQ25120A）初始化修复与诊断

- **问题线索**：早期日志出现 BQ25120A 配置失败（I2C ret=-5），导致上电时序与 3V3 轨状态不确定。
- **处理**：
  - 将 `bq25120a_safe_init.c` 加入构建（此前未参编译导致“修复代码实际没进固件”）。
  - 重写 `power_sequence.c`：在合适的 `SYS_INIT` 时序中调用 safe-init，输出 PMIC 寄存器 dump、写入回读验证等诊断日志。
  - 将扫描前 settle 延迟从 5ms 增大到 250ms，并在 MLX90642 driver 内加入更长的 POR 等待（最终按手册 worst case 将等待调整到 ~600ms）。

### 2) MLX90642 driver 增强（更精确的失败定位）

- 增加：
  - I2C controller ready 检查
  - POR delay
  - ACK 探测重试 + `0x33` fallback
  - 进度寄存器（`0x3C10`）读探测重试
  - FW / DeviceID / Ta 读取日志（用于“活着但读失败”与“完全不在总线”区分）

### 3) Web UI / BLE HW Status 日志链路

- 引入 `boot_diag` 环形缓冲，将关键启动/扫描诊断通过 **Hardware Status GATT** 特征值输出，网页可直接查看：
  - PMIC safe-init 过程
  - 三条 I2C bus scan 结果
  - PCA9306 高侧专项探测与 verdict
- 为避免关键信息被截断，将 `BOOT_DIAG_MAX_MSG_LEN` 从 88 增加到 144，并把 verdict 拆成多条短日志。

### 4) PCA9306 高侧专项诊断（决定性定位）

在扫描中额外探测 IIC1 上“高侧 3.3V 域设备”（MLX90642 / MLX90632 / MAXM86161）：

- **观测到**：
  - ADAU1860（1.8V 侧）能 ACK
  - 但 3 个高侧 3.3V 设备全部 NACK（`0x66/0x33/0x3A/0x62` 全失败）
- **结论**：
  - 这不可能是“3 个芯片都坏了”
  - 明确是 **PCA9306 高侧（VREF2/EN/上拉/器件接法）不工作**

## 硬件测量与最终根因

### 关键实测

- VREF1 = 1.8V（正常）
- VREF2 = 3.3V（用户直接短接到 3.3V 轨）
- EN = 3.3V（与 VREF2 同节点）
- SDA2/SCL2 有上拉到 3.3V，但 **静态电压稳定在 ~2.5V（异常）**

### 根因（与 TI PCA9306 手册完全吻合）

- **错误接法**：VREF2（pin7）直接短接到 3.3V（VDPU），未串联 200kΩ 高阻电阻。
- 造成：
  - PCA9306 内部参考/charge pump 机制失效/偏置电流路径异常
  - pass NMOS 处于半导通，导致 SDA2/SCL2 静态被“分压/钳位”到约 2.5V
  - 3.3V 域设备无法看到可靠的 I2C HIGH/边沿 → 全部 NACK

### 建议修复（硬件）

- 按 TI 推荐接法改线：
  - **VREF2 与 EN 短接**
  - 该节点通过 **200kΩ** 上拉到 VDPU(3.3V)
  - 可选：VREF2 节点加 **100pF** 到 GND 滤波
- 预期修复后：
  - SDA2/SCL2 静态回到 3.3V
  - IIC1 扫描能看到 `0x66` ACK

## 备选方案：3.3V 供电 + 1.8V I2C（不经电平转换）

MLX90642 支持 **3.3V 供电 + 1.8V I2C**，但需要先把 EEPROM `0x11FC` 配成 1.8V 阈值参考（非出厂默认）。可用外部单片机（3.3V I2C）先配置好，再连接到 nRF5340 的 1.8V 总线使用。

## 当前状态

- 软件侧：PMIC 初始化、延迟、扫描诊断、MLX driver 探测链路均已完善并能稳定复现“高侧不导通”的判据。
- 硬件侧：依据测量与 TI 手册，**PCA9306 VREF2 接法（缺 200kΩ）为主要根因**；修复该接法应能恢复高侧 I2C。

