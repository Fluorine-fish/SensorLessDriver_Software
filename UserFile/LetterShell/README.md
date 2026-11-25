# USB Letter Shell 最小验证

本版本接通 USB VCP → Letter Shell → 文本响应，尚未注册电机控制命令。
上游使用 `UserLib/LetterShell` 中的纯 C 子模块，当前提交为
`03432e824ca889d961743b437bce3f837c47bb86`，本次未修改上游源码。

## 实板使用

烧录后连接 USB，打开设备对应的 CDC 串口。建议终端设置为 115200、8N1，
关闭本地回显；USB CDC 的波特率设置不决定 USB 传输速率。
支持 CR、LF、CRLF 行结束符，以及 Tab 补全、退格、上下键历史。
若打开终端时未显示提示符，按一次回车。

上方向键调出上一条已提交命令，连续按可继续向前查找（最多 5 条），
调出后可编辑并按回车执行；下方向键向较新的历史移动。
方向键兼容普通模式 `ESC [ A/B/C/D` 和应用模式 `ESC O A/B/C/D`。
Backspace 支持 `0x08`、`0x7F`，删除光标前一个字符；
独立 Delete 键的 `ESC [ 3 ~` 删除光标所在字符。
终端需使用逐字符发送并启用 ANSI/VT100 转义序列解释；关闭本地回显和本地行编辑。

### CoolTerm 设置

CoolTerm 并非完整的终端仿真器，需要单独启用按键与接收控制字符处理：

- `Options > Terminal`：选择 `Raw Mode`，关闭 `Local Echo`，开启
  `Emulate Special Keys`，关闭 `Replace TAB key with spaces`。
- `Options > Data Handling > Special Character Handling`：开启
  `Handle BS and DEL Characters`，关闭 `Convert Non-printable Characters`。
- 如果接收显示仍未处理退格，检查 `Receive` 中的
  `Direct RX update of text display` 并关闭；该优化可能干扰控制字符处理。

行尾删除时 Shell 返回 `08 20 08`（退格、空格、退格）。若 CoolTerm
仅将不可打印字符替换为点，会显示成 `. .`；这本身不表示点被写进命令。
可输入 `pingx`，按一次 Delete，再按回车，预期返回 `pong`。
CoolTerm 未开启特殊按键仿真时，上键可能发送 `0x1E`，而非 Shell 所需的 `ESC [ A`。
复杂的行中光标编辑建议使用 `screen` 或完整的 VT100 终端。

网页串口工具若使用终端组件，需要解释退格及光标控制字节；普通文本接收框
不能仅靠固件实现原地删除。若网页使用独立命令输入框，可在网页本地编辑整行，
点击发送时再发送命令和 `\r`，历史也由网页管理。

```text
VESC_F4:/$ ping
pong
VESC_F4:/$ version
VESC_F4 USB shell bring-up v1
VESC_F4:/$ help ping
```

`help` 可查看命令；`ping`、`version` 不接受参数，多余参数返回
`ERR usage: ...`。目前仍保留上游内置帮助、命令列表、终端编辑等功能。
没有导出电机变量，没有启用任意地址函数执行。

## 文件与调用关系

| 文件 | 职责 |
| --- | --- |
| `Shell_cfg_user.h` | Shell 功能、默认用户、历史与参数数量配置 |
| `ShellPort.c/.h` | 私有 Shell 实例、USB 读写适配、初始化与单字符轮询 |
| `ShellReply.c/.h` | 命令响应文本及 CRLF 输出 |
| `Commands/ShellSystemCmd.c` | `ping`、`version` 命令实现及注册 |
| `CMakeLists.txt` | 显式编译上游 C 源码及本项目适配文件 |

接口说明统一放在 `.c`，`.h` 仅保留声明等必要内容。
`ShellConfigCmd.c` 已接入编译，提供 `config_get_canid`、`config_set_canid`、
`config_get_can_status`；`ShellMotorCmd.c` 暂未接入编译。

1. `main()` 在启动 USB 和定时器前调用 `BspUsbVcp_Init()`、`ShellPort_Init()`。
2. CDC 配置、取消配置、接收和发送完成回调接入 `BspUsbVcp`。
3. `FocApp_UpdateLowFreqData()` 末尾调用 `ShellPort_Poll()`，启动校准期间也运行。
4. `main()` 的循环调用 `BspUsbVcp_Poll()`，负责 USB 发送提交、完成回收和恢复接收。
5. 链接脚本通过 `KEEP(*(shellCommand))` 保留命令表。

## 中断与容量约束

`FocApp_UpdateLowFreqData()` 当前运行于 TIM2 中断，抢占优先级为 1。
每次 Shell 轮询最多取一个字符，并要求 TX 队列完全空闲后才处理该字符。
命令在收到回车时同步执行，输出仅复制入队，不在中断中等待 USB。
主机停止读取时暂停消费后续输入，RX 队列接近满时通过 USB OUT NAK 背压。

`BspUsbVcp` 使用保存并恢复 BASEPRI 的临界区，屏蔽优先级 1 及以下紧急程度
的中断，保护主循环、TIM2 和 USB 回调共享的队列；优先级 0 的 FOC 中断仍可运行。
此设计依赖当前 `NVIC_PRIORITYGROUP_4` 分组；优先级 0 中断不得调用这些接口。

当前 RX 队列为 1024 字节、TX 队列为 2048 字节，Shell 每行缓冲为 128 字节，
保留 5 条历史。Shell 是 TX 唯一生产者；新增日志输出需重新安排容量和并发策略。
单次字符处理产生的全部输出必须能装入 TX 队列，超出时会截断，并在队列排空后报告
`ERR output too long`。USB 重新配置时清除旧命令和历史；挂起/恢复保留未完成输入。

后续命令仍放在本目录的 `Commands/` 中，通过上游导出宏注册。
耗时控制、等待、Flash 写入、大量格式化或长输出应提交请求给其他执行上下文，
不应直接在 TIM2 命令回调中执行。当前方案用于最小交互验证，尚未测量实际中断耗时。

## 已完成验证

- `cmake --preset Debug`、`cmake --build --preset Debug -j 4` 构建通过。
- ARM Cortex-M4 QEMU 中编译并运行真实 Shell、适配层及 BSP，USB 设备接口使用模拟实现。
- 验证 `ping`、`version`、帮助、未知命令、参数错误、CR/LF/CRLF、补全、退格、历史和连续输入。
- 验证多次重新配置、挂起恢复、主机暂停读取、BUSY/FAIL 重试、RX 背压及 BASEPRI 恢复。
- 上述用例观测到 TX 最大占用 428 字节；这不是对未来任意命令的输出上界证明。

尚未烧录实板；USB 枚举、实际串口收发、拔插及 FOC 实时性仍需实板验证。
