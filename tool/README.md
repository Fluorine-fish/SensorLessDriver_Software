# VESC_F4 USB BootLoader

已按飞书文档核对（2026-11-25）：

- [BootLoader 协议](https://ex4z7odckj5.feishu.cn/wiki/FAEtwhyHbiPeiDkFHQdcQNZLnGg)
- [STM32F405 Flash 分区](https://ex4z7odckj5.feishu.cn/wiki/TvmCwbQg0i1Uw0krKKKcdn12ntd)

文档示例是 STM32G431 USART IAP，本工程使用 STM32F405 USB FS CDC；保留帧格式、命令号、状态码、256字节块及8字节传输对齐。BootInfo magic 采用本机 SimpleBootLoader 实际工程及其 Python 上位机的 `0x434C4931`；文档代码片段里的 `0x424C4931` 与其日志不一致。

## 当前临时调试模式（2026-11-26）

`BOOT_DEBUG_DIRECT_APP` 当前默认 **ON**：正常复位在 HAL、PLL、GPIO 和 USB 初始化前，直接加载 `0x08008000` 的 MSP/Reset_Handler 进入 APP；不等待5秒，不读取 Sector 11，不校验 APP 向量、BootInfo 状态或 CRC。此时上电不会显示 BootLoader 双灯灯语；APP 自己管理灯语。

LetterShell `bootloader` 的软件复位邮箱仍有效：主动请求时进入 USB IAP 并双灯常亮。IAP 的下载帧校验和 VERIFY 完整性校验仍保留；本开关只放开启动/JUMP_APP 准入。旧 Sector 11 校准记录尚未迁移时，正常启动不会受影响，但 IAP 写入仍会报告旧日志格式错误。

需要直接调试 APP 时，继续使用 Ozone 加载 APP ELF；它会按 ELF 设置 SP/PC 并绕过 BootLoader。要验证正常启动，应使用硬件复位或断电重启，不手动覆盖 SP/PC。

恢复下文描述的正式启动校验和5秒窗口：

```sh
cmake --preset Debug -S BootLoader -DBOOT_DEBUG_DIRECT_APP=OFF
cmake --build BootLoader/build/Debug
```

随后补烧该 BootLoader；恢复前须先处理旧校准日志并建立有效 BootInfo。Release 的开关独立配置。调试期间重新打开用同一命令改为 `ON`。

## Flash 与构建

| 区域 | 地址（含末地址） | 大小 | 扇区 |
|---|---|---|---|
| BootLoader | `0x08000000–0x08007FFF` | 32 KB | 0–1 |
| APP | `0x08008000–0x080DFFFF` | 864 KB | 2–10 |
| 追加日志 | `0x080E0000–0x080FFFFF` | 128 KB | 11 |

APP 和 BootLoader 在主 SRAM 顶部共同保留8字节复位邮箱 `0x2001FFF8`，不在 `.bss` 清零，栈顶为 `0x2001FFF8`。邮箱只在软件复位且 magic/反码同时匹配时生效，BootLoader 读取后立即清除。主 SRAM 其余部分及64 KB CCM 的配置不变。

从仓库根目录执行：

```sh
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Debug -S BootLoader
cmake --build BootLoader/build/Debug
```

`Release` 同理。输出为 `build/Debug/VESC_F4.bin` 与 `BootLoader/build/Debug/BootLoader.bin`。BootLoader 的 Debug 也用 `-Os` 保证32 KB预算，并保留调试信息。链接器会拒绝分区溢出；APP 的 SystemInit 通过 CMake 编译定义把 VTOR 设置到 `0x08008000`。

首次通过 SWD 分别烧录 BootLoader 到 `0x08000000`，APP 到 `0x08008000`（ELF 自带地址）。不要再把 APP 的 bin 烧到 `0x08000000`。升级仅擦除 Sector 2–10，绝不擦除 BootLoader 或 Sector 11。

BootLoader 的 `.ioc`、HAL 模块、初始化函数和编译源列表均已关闭 ADC1/2/3、CAN1、TIM1/2；保留 HSE/PLL（USB 48 MHz）、SysTick、USB CDC、SWD 和 GPIO。PB0/PB1 两灯高电平常亮；PA8/9/10、PB13/14/15 六路功率管输入固定低电平。

## USB 升级

```sh
python3 -m venv tool/.venv
tool/.venv/bin/python -m pip install -r tool/requirements.txt
tool/.venv/bin/python tool/iap.py ports
```

APP 正常运行且 LetterShell 处于空闲提示符时，一条命令完成升级：

```sh
tool/.venv/bin/python tool/iap.py --port /dev/cu.usbmodemXXXX --enter-app upload build/Debug/VESC_F4.bin --version 1
```

Windows 把端口换为 `COM3` 等实际名称。脚本先验证 bin 向量，再发送 `bootloader\r`，随后按原设备 USB 序列号重连。APP 命令在 TIM2 中仅入队响应并提交请求；主循环禁用电机控制中断、清除 PWM 的 MOE、停止 ADC/TIM，等待 USB 发送完成（最多500 ms），断开 USB 并系统复位。BootLoader 收到邮箱请求后持续等待升级。

也可以手动在 LetterShell 输入 `bootloader`，关闭串口终端，再执行不带 `--enter-app` 的升级命令。烧录工具和串口终端不能同时占用同一端口。

正常上电从 USB 初始化完成后开始等待5秒；窗口内收到 `U/u` 就持续留在 IAP。无有效 APP、升级中或校验失败时自动留在 IAP。没有 USB 主机时也会按时启动有效 APP，不无限等待枚举。BootLoader 产品字符串为 `F4Driver BootLoader`，沿用 APP 的 VID/PID 和芯片 UID 序列号；USB 设备在 APP/BootLoader 切换时会重新枚举。

已在 BootLoader 状态时：

```sh
tool/.venv/bin/python tool/iap.py --port /dev/cu.usbmodemXXXX info
tool/.venv/bin/python tool/iap.py --port /dev/cu.usbmodemXXXX upload build/Debug/VESC_F4.bin --version 2
tool/.venv/bin/python tool/iap.py --port /dev/cu.usbmodemXXXX jump
```

升级为单 APP 区原地写入，没有备份镜像或回滚。掉电/断连后保留 UPDATING，重新执行 upload，从擦除开始重传。脚本对擦除等待30秒、写块等待8秒；F405 单 Bank Flash 擦写期间 USB ISR 会暂停，因此使用停等传输，不能连续灌包。

## 固件及协议格式

固件是链接到 `0x08008000` 的原始 `.bin`，开头为初始 MSP 和 Thumb Reset_Handler。CRC32 使用标准反射算法（多项式 `0xEDB88320`、初值及结果异或 `0xFFFFFFFF`），等同 `zlib.crc32`；仅计算 bin 实际长度，不包含尾部补齐。

所有多字节整数为小端：

```text
请求：5A CMD LEN16 PAYLOAD CRC16
响应：A5 CMD STATUS LEN16 PAYLOAD CRC16
```

CRC16 为 CCITT-FALSE（多项式 `0x1021`，初值 `0xFFFF`，不反转、不最终异或），覆盖 SOF 之后、CRC 之前的全部字节。

| CMD | 请求 payload | 响应/作用 |
|---|---|---|
| A1 GET_INFO | 空 | 32字节 BootInfo |
| A2 ERASE_APP | size、crc32、version，各uint32 | 先追加 UPDATING，再擦除 APP |
| A3 WRITE_BLOCK | offset uint32 + 8～256字节数据 | 按序写入；offset/长度8对齐，尾部补FF |
| A4 VERIFY | 空 | 检查完整长度、向量、CRC32，追加 VALID |
| A5 JUMP_APP | 空 | 再校验，等响应发送完成后跳转 APP |

状态码沿用 `00 OK / 01 BAD_FRAME / 02 BAD_LENGTH / 03 BAD_STATE / 04 FLASH_ERROR / 05 CRC_ERROR / 06 INVALID_APP / 07 UNKNOWN_COMMAND`。日志已满或损坏通过 FLASH_ERROR 返回。每次升级开始必须留出至少两个日志槽，否则在擦除 APP 前拒绝升级。

BootInfo 是8个uint32：`magic, state, app_addr, app_size, app_crc32, app_version, reserved0, reserved1`。magic=`0x434C4931`；state 为 UPDATING=`0x55504454` 或 VALID=`0x56414C44`；两个 reserved 均为 `0xFFFFFFFF`。A1 返回逻辑记录，不暴露 Flash 日志包装。

USB 分包/粘包不作为协议帧边界，帧内最长空闲时间1秒。重复已写且内容相同的块可再次确认，不重复编程；乱序块、跨区块、非FF尾部补齐和未完成下载的 VERIFY 均拒绝。断连/重新配置后清除解析与写入会话，必须重新 ERASE_APP。二进制响应中不夹杂文本日志。

首次调试器烧录：**只有 Sector 11 全空且向量合法时**允许无 BootInfo 启动。已有任何日志后必须存在有效 BootInfo 并通过整包CRC。调试器直接更换 APP 后，旧 BootInfo 不会自动失效，应再通过 IAP 更新以生成匹配记录；不要为方便调试直接擦除包含校准数据的 Sector 11。

## BspFlash 与后续电机参数 Shell

实现位于 `UserFile/Bsp/BspFlash.[ch]`，APP 和 BootLoader 共用同一份代码与格式：

- `BspFlash_ReadRecord(key, data, capacity, &length)`：读取该键最后一次已提交的值。
- `BspFlash_WriteRecord(key, data, length)`：追加一次记录，单条1～480字节。
- `BspFlash_GetStatus(&status)`：查询已用槽、尾部可用槽、损坏/未提交槽和全空状态。
- 键 `1` 保留给 BootInfo，`0x100` 用于电机参数；后续其他参数分配不同键。
- 返回值区分 `OK / NOT_FOUND / FULL / BAD_ARGUMENT / CORRUPT / IO_ERROR / BAD_CONTEXT`；不能把非OK当成保存成功。

Sector 11 共256个固定512字节槽，每槽格式：

```text
0x000 magic=0x314C4E4A
0x004 key
0x008 payload长度
0x00C 日志格式版本=1
0x010 payload CRC32
0x014 前20字节header CRC32
0x018 payload（最多480字节，其余FF）
0x1FC commit=0x434F4D54，最后写入
```

写前不擦除，逐字编程完成并回读校验后才写 commit；写完刷新 F4 Flash 缓存再读取。未提交槽保留、跳过，不重复使用。最新已提交数据 CRC 损坏不会回退旧值；未知旧格式拒绝写入，先维护。正常升级只增加两条 BootInfo，不触碰参数记录。写满时返回 FULL；没有在线自动回收、格式化或擦除命令。

后续电机参数命令请使用“修改 RAM 草稿 → 明确 save → 主循环落盘”的流程：

1. Shell 当前运行在 TIM2 ISR，只解析参数、做范围检查并复制保存请求；不能在回调里直接调用写 Flash。
2. 主循环先停电机 PWM，并阻止控制 ISR 再次启用输出，之后调用 WriteRecord。即使不擦扇区，单 Bank 编程也会暂停取指，不能边转电机边保存。
3. 存储专用参数结构（首字段加 schema_version），不要直接保存包含状态/指针的整个 Foc 运行结构；上电读取后检查长度、版本和物理范围。
4. 参数保存前应已通过一次 IAP 建立 VALID BootInfo。首次直接 SWD、Sector 11 为空的调试固件，如果先写参数但没有 BootInfo，下次启动会留在 BootLoader；可重新 IAP 恢复，参数不会被擦除。

旧 `BspFlash_Write(FLASH_Sector_11, ...)` 保留为 MOTOR_PARAMS 追加封装，最大480字节，其他扇区返回 false。旧 `BspFlash_Read` 仍是相对 Flash 起点的有界原始读取，**不会自动定位最新参数**，新代码必须使用 ReadRecord。

## 日志满后的维护

通过 SWD 备份 `0x080E0000` 起完整128 KB到 `sector11-backup.bin`，确认备份可读取后，离线检查/压缩：

```sh
python3 tool/journal.py sector11-backup.bin
python3 tool/journal.py sector11-backup.bin --compact sector11-compacted.bin
```

工具保留每个键的最新有效记录、保持 BootInfo 原状态，跳过未提交槽；遇到未知格式或最新有效记录损坏则拒绝整理，绝不伪造 VALID。输出路径必须不存在。保留原备份，再用 SWD **仅擦除 Sector 11**、回写完整整理镜像，并逐字节读取比对；不要执行整片擦除。设备在维护回写完成并验证之前保持复位/暂停状态。整理过程若掉电，使用保留的备份恢复。

## 验证

```sh
python3 -m unittest discover -s tool/tests -v
```

测试用本机 C 编译器直接编译生产 `BspFlash.c`、`IapUsb.c`，仅替换 Flash/USB 硬件端口；Python CLI 可完整驱动这套 C 协议模拟器。覆盖CRC标准向量、逐字写入中断、记录覆盖和损坏、空间耗尽、断连、帧拆分、越界/乱序/重复块、末块补齐、完整上传、升级不破坏参数及离线整理。

实机仍需验证：双灯常亮、六路功率管输入保持关闭、5秒启动、Shell复位后USB重连、接近864 KB镜像的整包写入、升级中拔线/断电恢复，以及跳转后 APP 的 USB/CAN/ADC/TIM 中断工作。主机测试不代替 USB 枚举、真实 Flash 时序和电机输出的板上验收。
