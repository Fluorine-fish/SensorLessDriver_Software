"""实际 C 存储/协议代码 + Python CLI 的主机测试，无需开发板或 pyserial。"""
import contextlib
import ctypes as C
import io
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tool"))
import iap
import journal


class FlashStatus(C.Structure):
    _fields_ = [("used_slots", C.c_uint32), ("free_slots", C.c_uint32),
                ("damaged_slots", C.c_uint32), ("blank", C.c_bool)]


class FirmwareTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        lib = Path(cls.temp.name) / "sim.so"
        subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-Itool/tests/native", "-IUserFile/Bsp", "-IBootLoader/UserFile",
                        "tool/tests/native/simulator.c", "-o", str(lib)], cwd=ROOT, check=True)
        cls.sim = C.CDLL(str(lib))
        cls.sim.BspFlash_WriteRecord.argtypes = [C.c_uint32, C.c_void_p, C.c_uint32]
        cls.sim.BspFlash_ReadRecord.argtypes = [C.c_uint32, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32)]
        cls.sim.BspFlash_Crc32.argtypes = [C.c_void_p, C.c_uint32]
        cls.sim.BspFlash_Crc32.restype = C.c_uint32
        cls.sim.SimReceive.argtypes = [C.c_void_p, C.c_uint32]

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        self.sim.SimReset(1)

    def write(self, key, data):
        return self.sim.BspFlash_WriteRecord(key, data, len(data))

    def read(self, key, capacity=480):
        out = C.create_string_buffer(capacity)
        length = C.c_uint32()
        result = self.sim.BspFlash_ReadRecord(key, out, capacity, C.byref(length))
        return result, out.raw[:length.value]

    def export(self):
        out = C.create_string_buffer(128 * 1024)
        self.sim.SimExport(out)
        return out.raw

    def status(self):
        status = FlashStatus()
        self.assertEqual(self.sim.BspFlash_GetStatus(C.byref(status)), 0)
        return status

    def raw(self, packet):
        self.sim.SimReceive(packet, len(packet))
        out = C.create_string_buffer(4096)
        size = self.sim.SimRead(out)
        return out.raw[:size]

    def request(self, command, payload=b"", status=0):
        frames = iap.ResponseParser().feed(self.raw(iap.make_request(command, payload)))
        self.assertEqual(len(frames), 1)
        self.assertEqual(frames[0][:2], (command, status))
        return frames[0][2]

    @staticmethod
    def firmware(size=1027):
        return struct.pack("<II", 0x2001FFF8, 0x08008009) + bytes((i % 251 for i in range(size - 8)))

    def begin(self, data=None):
        data = data or self.firmware()
        self.request(0xA2, struct.pack("<III", len(data), zlib.crc32(data), 7))
        return data

    def test_crc_golden(self):
        self.assertEqual(iap.crc16(b"123456789"), 0x29B1)
        self.assertEqual(self.sim.BspFlash_Crc32(b"123456789", 9), 0xCBF43926)
        self.assertEqual(iap.make_request(0xA1), bytes.fromhex("5aa100003046"))

    def test_latest_key_and_short_read(self):
        self.assertEqual(self.read(0x100)[0], 1)
        self.assertTrue(self.status().blank)
        self.assertEqual(self.write(0x100, b"old"), 0)
        self.assertEqual(self.write(1, b"boot-info"), 0)
        self.assertEqual(self.write(0x100, b"new"), 0)
        self.assertEqual(self.read(0x100), (0, b"new"))
        self.assertEqual(self.read(1), (0, b"boot-info"))
        self.assertEqual(self.read(1, 2)[0], 3)
        self.assertEqual(self.status().free_slots, 253)

    def test_append_power_cut_at_every_word(self):
        value = bytes(range(64))
        # 24字节头 + 64字节payload + 最后提交字，共23次编程。
        for cut in range(24):
            with self.subTest(cut=cut):
                self.sim.SimReset(1)
                self.assertEqual(self.write(0x100, b"previous"), 0)
                self.sim.SimPowerCut(cut)
                result = self.write(0x100, value)
                self.sim.SimReset(0)
                expected = value if cut >= 23 else b"previous"
                self.assertEqual(self.read(0x100), (0, expected))
                self.assertEqual(result, 0 if cut >= 23 else 5)
                self.assertEqual(self.write(0x101, b"after reset"), 0)
                self.assertEqual(self.read(0x100), (0, expected))

    def test_corrupt_latest_does_not_fall_back(self):
        self.write(0x100, b"first")
        self.write(0x100, b"second")
        self.sim.SimCorrupt(512 + 24)
        self.assertEqual(self.read(0x100)[0], 4)
        with self.assertRaises(ValueError):
            journal.compact(self.export())

    def test_limits_context_and_full(self):
        self.assertEqual(self.write(0, b"x"), 3)
        self.assertEqual(self.write(1, b""), 3)
        self.assertEqual(self.write(1, b"x" * 481), 3)
        self.sim.SimIrq(1)
        self.assertEqual(self.write(0x100, b"x"), 6)
        self.sim.SimIrq(0)
        for _ in range(255):
            self.assertEqual(self.write(0x100, b"x"), 0)
        image = self.export()
        self.request(0xA2, struct.pack("<III", 16, 0, 1), status=4)
        self.assertEqual(self.sim.SimErases(), 0)
        self.assertEqual(self.export(), image)
        self.assertEqual(self.write(0x100, b"last"), 0)
        self.assertEqual(self.write(0x100, b"overflow"), 2)
        self.assertEqual(self.read(0x100), (0, b"last"))

    def test_foreign_data_rejected(self):
        self.sim.SimCorrupt(0)
        self.assertEqual(self.write(0x100, b"x"), 4)
        self.assertEqual(self.read(0x100)[0], 4)

    def test_fragmented_protocol_and_crc(self):
        packet = iap.make_request(0xA1)
        for byte in packet[:-1]:
            self.assertEqual(self.raw(bytes([byte])), b"")
        frames = iap.ResponseParser().feed(self.raw(packet[-1:]))
        self.assertEqual(frames, [(0xA1, 0, b"\xff" * 32)])
        bad = packet[:-1] + bytes([packet[-1] ^ 1])
        frames = iap.ResponseParser().feed(self.raw(bad))
        self.assertEqual(frames[0][:2], (0xA1, 5))
        self.request(0xA1)
        self.request(0xFE, status=7)
        self.request(0xA1, b"x", status=2)
        self.request(0xA4, status=3)
        self.request(0xA5, status=6)

    def test_timeout_oversize_and_reconnect(self):
        self.assertEqual(self.raw(b"\x5a\xa1"), b"")
        self.sim.SimAdvance(1001)
        response = self.raw(b"")
        self.assertEqual(iap.ResponseParser().feed(response)[0][:2], (0xA1, 1))
        frames = iap.ResponseParser().feed(self.raw(b"\x5a\xa3\xff\xff"))
        self.assertEqual(frames[0][:2], (0xA3, 2))
        self.begin()
        self.sim.SimReconnect()
        self.request(0xA3, struct.pack("<I", 0) + b"\xff" * 8, status=3)
        self.request(0xA5, status=6)

    def test_write_order_duplicates_and_padding(self):
        data = self.begin(self.firmware(17))
        self.request(0xA3, struct.pack("<I", 8) + data[8:16], status=3)
        self.request(0xA3, struct.pack("<I", 0xFFFFFFF8) + b"\xff" * 8, status=2)
        self.request(0xA3, struct.pack("<I", 0) + data[:8])
        self.request(0xA3, struct.pack("<I", 0) + data[:8])
        self.request(0xA3, struct.pack("<I", 0) + b"\x00" * 8, status=3)
        self.request(0xA4, status=3)
        self.request(0xA3, struct.pack("<I", 8) + data[8:] + b"\x00" * 7, status=2)
        self.request(0xA3, struct.pack("<I", 8) + data[8:] + b"\xff" * 7)
        self.request(0xA4)
        self.request(0xA5)
        self.assertEqual(self.sim.SimJumps(), 1)

    def test_crc_failure_and_erase_failure(self):
        data = self.firmware(16)
        self.request(0xA2, struct.pack("<III", len(data), zlib.crc32(data) ^ 1, 1))
        self.request(0xA3, struct.pack("<I", 0) + data)
        self.request(0xA4, status=5)
        self.request(0xA5, status=6)
        self.sim.SimFlashError(1)
        self.request(0xA2, struct.pack("<III", len(data), 0, 1), status=4)
        self.request(0xA3, struct.pack("<I", 0) + data, status=3)
        info = struct.unpack("<8I", self.request(0xA1))
        self.assertEqual(info[1], 0x55504454)

    def test_boot_wait_and_trigger(self):
        self.sim.SimWaitBoot()
        self.sim.SimAdvance(4999)
        self.assertEqual(self.sim.SimJumps(), 0)
        self.raw(b"u")
        self.sim.SimAdvance(10000)
        self.assertEqual(self.sim.SimJumps(), 0)
        self.request(0xA1)
        self.sim.SimWaitBoot()
        self.sim.SimAdvance(5000)
        self.assertEqual(self.sim.SimJumps(), 1)

    def test_full_upload_host_to_real_c_protocol_preserves_params(self):
        self.assertEqual(self.write(0x100, b"motor calibration v1"), 0)
        test = self
        class Port:
            pending = bytearray()
            def write(self, data):
                self.pending.extend(test.raw(data))
                return len(data)
            def read(self, size):
                data = bytes(self.pending[:size])
                del self.pending[:size]
                return data
        with contextlib.redirect_stdout(io.StringIO()):
            iap.Client(Port()).upload(self.firmware(), 7)
        self.assertEqual(self.sim.SimJumps(), 1)
        self.assertEqual(self.read(0x100), (0, b"motor calibration v1"))
        packed = journal.compact(self.export())
        self.sim.SimImport(packed)
        self.assertEqual(self.status().free_slots, 254)
        self.assertEqual(self.read(0x100), (0, b"motor calibration v1"))
        self.assertTrue(self.sim.BootLoader_IsBootable())

    def test_updating_survives_reset_and_compaction(self):
        self.begin()
        self.sim.SimReset(0)
        self.request(0xA4, status=3)
        self.request(0xA5, status=6)
        self.sim.SimImport(journal.compact(self.export()))
        self.request(0xA5, status=6)
        self.assertEqual(struct.unpack("<8I", self.request(0xA1))[1], 0x55504454)

    def test_bad_images_and_response_resync(self):
        for data in [b"", b"\xff" * 40, b"\x7fELF" * 10,
                     struct.pack("<II", 0x20020000, 0x08000009) + b"\xff" * 40]:
            with self.assertRaises(ValueError):
                iap.validate_firmware(data)
        iap.validate_firmware(self.firmware())
        good = self.raw(iap.make_request(0xA1))
        bad = good[:-1] + bytes([good[-1] ^ 1])
        parser = iap.ResponseParser()
        output = []
        for byte in b"junk" + bad + good:
            output.extend(parser.feed(bytes([byte])))
        self.assertEqual(output, [(0xA1, 0, b"\xff" * 32)])


if __name__ == "__main__":
    unittest.main()
