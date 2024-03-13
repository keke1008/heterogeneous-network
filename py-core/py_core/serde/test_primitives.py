import unittest

from .buffer import BufferReader, BufferWriter, InvalidValueException
from .primitives import Boolean, UInt8, UInt16, UInt32, UInt64


class TestBoolean(unittest.TestCase):
    cases = {
        True: bytearray([1]),
        False: bytearray([0]),
    }

    def test_deserialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                reader = BufferReader(expected)
                boolean = Boolean.deserialize(reader)
                self.assertEqual(boolean, value)

    def test_deserialize_invalid(self):
        with self.assertRaises(InvalidValueException):
            reader = BufferReader(bytearray([2]))
            Boolean.deserialize(reader)

    def test_serialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                writer = BufferWriter(1)
                Boolean(value).serialize(writer)
                self.assertEqual(writer.unwrap_buffer(), expected)


class TestUInt8(unittest.TestCase):
    cases = {
        0: bytearray([0]),
        1: bytearray([1]),
        0xFF: bytearray([0xFF]),
    }

    def test_deserialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                reader = BufferReader(expected)
                uint8 = UInt8.deserialize(reader)
                self.assertEqual(uint8.value, value)

    def test_serialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                writer = BufferWriter(1)
                UInt8(value).serialize(writer)
                self.assertEqual(writer.unwrap_buffer(), expected)


class TestUInt16(unittest.TestCase):
    cases = {
        0: bytearray([0, 0]),
        1: bytearray([1, 0]),
        0x1234: bytearray([0x34, 0x12]),
        0xFFFF: bytearray([0xFF, 0xFF]),
    }

    def test_deserialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                reader = BufferReader(expected)
                uint16 = UInt16.deserialize(reader)
                self.assertEqual(uint16.value, value)

    def test_serialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                writer = BufferWriter(2)
                UInt16(value).serialize(writer)
                self.assertEqual(writer.unwrap_buffer(), expected)


class TestUInt32(unittest.TestCase):
    cases = {
        0: bytearray([0, 0, 0, 0]),
        1: bytearray([1, 0, 0, 0]),
        0x12345678: bytearray([0x78, 0x56, 0x34, 0x12]),
        0xFFFFFFFF: bytearray([0xFF, 0xFF, 0xFF, 0xFF]),
    }

    def test_deserialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                reader = BufferReader(expected)
                uint32 = UInt32.deserialize(reader)
                self.assertEqual(uint32.value, value)

    def test_serialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                writer = BufferWriter(4)
                UInt32(value).serialize(writer)
                self.assertEqual(writer.unwrap_buffer(), expected)


class TestUInt64(unittest.TestCase):
    cases = {
        0: bytearray([0, 0, 0, 0, 0, 0, 0, 0]),
        1: bytearray([1, 0, 0, 0, 0, 0, 0, 0]),
        0x123456789ABCDEF0: bytearray([0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12]),
        0xFFFFFFFFFFFFFFFF: bytearray([0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]),
    }

    def test_deserialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                reader = BufferReader(expected)
                uint64 = UInt64.deserialize(reader)
                self.assertEqual(uint64.value, value)

    def test_serialize(self):
        for value, expected in self.cases.items():
            with self.subTest(value=value):
                writer = BufferWriter(8)
                UInt64(value).serialize(writer)
                self.assertEqual(writer.unwrap_buffer(), expected)
