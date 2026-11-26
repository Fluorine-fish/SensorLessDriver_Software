#!/usr/bin/env python3
"""VESC_F4 USB VCP 升级 CLI；协议兼容 SimpleBootLoader。"""
from __future__ import annotations

import argparse
import binascii
from pathlib import Path
import struct
import sys
import time
import zlib

APP_ADDR = 0x08008000
APP_SIZE = 864 * 1024
BOOT_MAGIC = 0x434C4931
STATUS = {0: "OK", 1: "BAD_FRAME", 2: "BAD_LENGTH", 3: "BAD_STATE",
          4: "FLASH_ERROR（也可能是 Sector 11 已满/损坏）", 5: "CRC_ERROR",
          6: "INVALID_APP", 7: "UNKNOWN_COMMAND"}


def crc16(data: bytes) -> int:
    return binascii.crc_hqx(data, 0xFFFF)


def make_request(command: int, payload: bytes = b"") -> bytes:
    if len(payload) > 260:
        raise ValueError("请求 payload 超过 260 字节")
    body = bytes([command]) + struct.pack("<H", len(payload)) + payload
    return b"\x5a" + body + struct.pack("<H", crc16(body))


def validate_firmware(data: bytes) -> None:
    if not 8 <= len(data) <= APP_SIZE:
        raise ValueError("固件须为 8～884736 字节的原始 APP .bin")
    stack, reset = struct.unpack_from("<II", data)
    valid_stack = (0x20000000 < stack <= 0x2001FFF8 or
                   0x10000000 < stack <= 0x10010000) and stack % 8 == 0
    if not valid_stack or not reset & 1 or not APP_ADDR + 8 <= (reset & ~1) < APP_ADDR + len(data):
        raise ValueError("向量表不合法：须使用链接至 0x08008000 的 APP，不能发送 ELF/HEX/BootLoader")


class ResponseParser:
    def __init__(self):
        self.buffer = bytearray()

    def feed(self, data: bytes):
        self.buffer.extend(data)
        frames = []
        while self.buffer:
            if self.buffer[0] != 0xA5:
                del self.buffer[0]
                continue
            if len(self.buffer) < 5:
                break
            length = int.from_bytes(self.buffer[3:5], "little")
            if length > 32:
                del self.buffer[0]
                continue
            total = 7 + length
            if len(self.buffer) < total:
                break
            raw = bytes(self.buffer[:total])
            if crc16(raw[1:-2]) != int.from_bytes(raw[-2:], "little"):
                del self.buffer[0]
                continue
            frames.append((raw[1], raw[2], raw[5:-2]))
            del self.buffer[:total]
        return frames


class Client:
    def __init__(self, port):
        self.port = port
        self.parser = ResponseParser()

    def request(self, command, payload=b"", timeout=5.0):
        packet = make_request(command, payload)
        if self.port.write(packet) != len(packet):
            raise RuntimeError("USB 请求未完整发送；请重新连接并从头升级")
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for cmd, status, body in self.parser.feed(self.port.read(64)):
                if cmd != command:
                    continue
                if status:
                    raise RuntimeError(f"命令 0x{cmd:02X}: {STATUS.get(status, status)}")
                expected = 32 if command == 0xA1 else 0
                if len(body) != expected:
                    raise RuntimeError(f"响应长度错误：期望 {expected}，实际 {len(body)}")
                return body
        raise TimeoutError(f"命令 0x{command:02X} 超时；保持 BootLoader，重新连接后从头下载")

    def info(self, timeout=5.0):
        fields = struct.unpack("<8I", self.request(0xA1, timeout=timeout))
        if fields[0] != 0xFFFFFFFF and (fields[0] != BOOT_MAGIC or fields[2] != APP_ADDR):
            raise RuntimeError("BootInfo 的 magic 或 APP 地址与 VESC_F4 不一致")
        return fields

    def upload(self, data, version):
        validate_firmware(data)
        self.info()
        crc = zlib.crc32(data) & 0xFFFFFFFF
        print(f"固件 {len(data)} 字节，CRC32=0x{crc:08X}，版本={version}")
        # STM32F405 单 Bank 擦除时不能执行 Flash 中的 USB ISR；停等并留出擦除时间。
        self.request(0xA2, struct.pack("<III", len(data), crc, version), timeout=30.0)
        padded = data + b"\xff" * (-len(data) % 8)
        total = (len(padded) + 255) // 256
        for number, offset in enumerate(range(0, len(padded), 256), 1):
            block = padded[offset:offset + 256]
            self.request(0xA3, struct.pack("<I", offset) + block, timeout=8.0)
            if number == total or number % 32 == 0:
                print(f"写入 {number}/{total}")
        self.request(0xA4, timeout=15.0)
        info = self.info()
        if info[1:6] != (0x56414C44, APP_ADDR, len(data), crc, version):
            raise RuntimeError("VERIFY 后 BootInfo 与本次固件不一致，拒绝跳转")
        self.request(0xA5, timeout=15.0)
        print("校验通过，设备已确认跳转 APP；USB 将重新枚举。")


def open_boot(args, serial, list_ports):
    selected = next((p for p in list_ports.comports() if p.device == args.port), None)
    if args.enter_app:
        with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=2) as port:
            if port.write(b"bootloader\r") != len(b"bootloader\r"):
                raise RuntimeError("bootloader 命令未完整发送")
            # 关闭旧端口，避免操作系统继续保留已失效的 USB 会话。
            time.sleep(0.3)
    deadline = time.monotonic() + 15.0
    last_error = None
    while time.monotonic() < deadline:
        devices = list(list_ports.comports())
        if args.enter_app and selected and selected.serial_number:
            matches = [p.device for p in devices if p.serial_number == selected.serial_number
                       and p.vid == selected.vid and p.pid == selected.pid]
        else:
            matches = [p.device for p in devices if p.device == args.port]
        if len(matches) > 1:
            raise RuntimeError("同一序列号匹配多个端口，请断开其他设备")
        if matches:
            port = None
            try:
                port = serial.Serial(matches[0], 115200, timeout=0.1, write_timeout=2)
                # 两种入口均可重复 U；BootLoader 协议空闲时会忽略它。
                port.write(b"U")
                client = Client(port)
                client.info(timeout=min(1.0, max(0.1, deadline - time.monotonic())))
                return client
            except (serial.SerialException, TimeoutError) as exc:
                last_error = exc
                if port:
                    port.close()
            except Exception:
                if port:
                    port.close()
                raise
        time.sleep(0.1)
    raise TimeoutError(f"15秒内未连接到 BootLoader，请复位并在5秒内连接：{last_error}")


def version_number(value):
    result = int(value, 0)
    if not 0 <= result <= 0xFFFFFFFF:
        raise argparse.ArgumentTypeError("版本号须为 uint32")
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="串口名，如 /dev/cu.usbmodem... 或 COM3")
    parser.add_argument("--enter-app", action="store_true", help="从 APP 的空闲 LetterShell 发送 bootloader 并重连")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("ports", help="列出串口")
    commands.add_parser("info", help="读取 BootInfo")
    commands.add_parser("jump", help="校验并启动现有 APP")
    upload = commands.add_parser("upload", help="擦除、写入、校验并启动 APP")
    upload.add_argument("bin", type=Path)
    upload.add_argument("--version", type=version_number, default=1)
    args = parser.parse_args(argv)
    if args.command != "ports" and not args.port:
        parser.error("需要 --port")
    # 在连接设备、触发复位之前检查固件。
    firmware = None
    if args.command == "upload":
        firmware = args.bin.read_bytes()
        validate_firmware(firmware)
    try:
        import serial
        from serial.tools import list_ports
    except ImportError as exc:
        raise RuntimeError("请先执行 python3 -m pip install -r tool/requirements.txt") from exc
    if args.command == "ports":
        for port in list_ports.comports():
            print(f"{port.device}\t{port.description}\tserial={port.serial_number}")
        return 0
    client = open_boot(args, serial, list_ports)
    try:
        if args.command == "upload":
            client.upload(firmware, args.version)
        elif args.command == "jump":
            client.request(0xA5, timeout=15)
            print("设备已确认跳转 APP")
        else:
            values = client.info()
            for name, value in zip(("magic", "state", "app_addr", "app_size", "app_crc32",
                                    "app_version", "reserved0", "reserved1"), values):
                print(f"{name:12s}: {value} (0x{value:08X})")
    finally:
        client.port.close()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, TimeoutError) as error:
        print(f"错误：{error}", file=sys.stderr)
        sys.exit(1)
