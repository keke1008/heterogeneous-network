import unittest
from py_core.serde.buffer import BufferReader, BufferWriter

from py_core.serde.primitives import UInt8
from py_core.serde.vector import Vector


class UInt8Vector(Vector[UInt8]):
    @staticmethod
    def element_type():
        return UInt8


class TestVector(unittest.TestCase):
    def test_deserialize(self):
        reader = BufferReader(bytearray([2, 0x12, 0x34]))
        v = UInt8Vector.deserialize(reader)
        self.assertEqual(v.values, [UInt8(0x12), UInt8(0x34)])

    def test_serialize(self):
        v = UInt8Vector([UInt8(0x12), UInt8(0x34)])
        writer = BufferWriter(3)
        v.serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer, bytearray([2, 0x12, 0x34]))
