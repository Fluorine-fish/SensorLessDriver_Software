#!/usr/bin/env python3
"""检查或压缩 SWD 导出的 Sector 11 日志；仅操作本地文件，不连接/擦除设备。"""
import argparse
from pathlib import Path
import struct
import sys
import zlib

SIZE = 128 * 1024
SLOT = 512
MAGIC = 0x314C4E4A
COMMIT = 0x434F4D54


def read_records(image):
    if len(image) != SIZE:
        raise ValueError("输入必须是 0x080E0000 开始的完整 128KB Sector 11 备份")
    latest = {}
    damaged = 0
    used = 0
    for offset in range(0, SIZE, SLOT):
        raw = image[offset:offset + SLOT]
        if raw == b"\xff" * SLOT:
            continue
        used += 1
        magic, key, length, fmt, payload_crc, header_crc = struct.unpack_from("<6I", raw)
        if magic != MAGIC:
            raise ValueError(f"0x{offset:X} 含未知数据；拒绝自动整理，保留原备份")
        if int.from_bytes(raw[-4:], "little") != COMMIT:
            damaged += 1
            continue
        if not (fmt == 1 and 0 < key < 0xFFFFFFFF and 0 < length <= 480 and
                zlib.crc32(raw[:20]) & 0xFFFFFFFF == header_crc):
            raise ValueError(f"0x{offset:X} 已提交记录头损坏；拒绝回退旧版本")
        # 最后已提交值覆盖旧值，即使 payload 损坏也不能悄悄回退。
        latest[key] = (raw, payload_crc, length)
    records = {}
    for key, (raw, crc, length) in latest.items():
        payload = raw[24:24 + length]
        if zlib.crc32(payload) & 0xFFFFFFFF != crc:
            raise ValueError(f"key=0x{key:X} 最新值 CRC 错误；拒绝整理")
        records[key] = (raw, payload)
    return records, used, damaged


def compact(image):
    records, _, _ = read_records(image)
    if 1 not in records and image != b"\xff" * SIZE:
        raise ValueError("非空日志没有有效 BootInfo，拒绝整理成允许首次启动的空日志")
    # 保留原始 BootInfo（包括 UPDATING），绝不由维护工具伪造 VALID。
    payload = b"".join(raw for raw, _ in records.values())
    return payload + b"\xff" * (SIZE - len(payload))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("backup", type=Path)
    parser.add_argument("--compact", type=Path, help="输出完整128KB整理镜像；不覆盖已有文件")
    args = parser.parse_args()
    image = args.backup.read_bytes()
    records, used, damaged = read_records(image)
    print(f"已用槽={used}/256，未提交槽={damaged}，有效键数={len(records)}")
    for key, (_, value) in records.items():
        print(f"key=0x{key:X}，长度={len(value)}，CRC32=0x{zlib.crc32(value) & 0xFFFFFFFF:08X}")
    if args.compact:
        packed = compact(image)
        with args.compact.open("xb") as output:
            output.write(packed)
        print(f"已生成 {args.compact}；原始备份必须保留，SWD 回写后逐字节校验。")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError) as error:
        print(f"错误：{error}", file=sys.stderr)
        sys.exit(1)
