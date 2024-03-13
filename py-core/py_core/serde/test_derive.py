from dataclasses import dataclass
import unittest

from py_core.serde.buffer import BufferReader, BufferWriter
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize
from py_core.serde.primitives import UInt16, UInt8


@dataclass
class MyStruct(DeriveDeserialize, DeriveSerialize):
    u8: UInt8
    u16: UInt16


class TestDerive(unittest.TestCase):
    def test_deserialize(self):
        writer = BufferWriter(3)
        UInt8(0x12).serialize(writer)
        UInt16(0x3456).serialize(writer)

        reader = BufferReader(writer.unwrap_buffer())
        my = MyStruct.deserialize(reader)

        self.assertEqual(my.u8.value, 0x12)
        self.assertEqual(my.u16.value, 0x3456)

    def test_serialize(self):
        my = MyStruct(UInt8(0x12), UInt16(0x3456))
        writer = BufferWriter(3)
        my.serialize(writer)
        actual = writer.unwrap_buffer()

        expected_writer = BufferWriter(3)
        UInt8(0x12).serialize(expected_writer)
        UInt16(0x3456).serialize(expected_writer)
        expected = expected_writer.unwrap_buffer()

        self.assertEqual(actual, expected)
