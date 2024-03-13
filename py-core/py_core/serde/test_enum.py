from enum import Flag
import unittest
from py_core.serde.enum import EnumSerde

from py_core.serde.buffer import BufferReader, BufferWriter


class Color(Flag):
    RED = 1
    GREEN = 2
    BLUE = 4


class ColorSerde(EnumSerde[Color]):
    @staticmethod
    def enum_type():
        return Color


class TestBitflags(unittest.TestCase):
    def test_deserialize(self):
        value = Color.RED | Color.GREEN

        reader = BufferReader(bytearray([value.value]))
        bitflags = ColorSerde.deserialize(reader)

        self.assertEqual(bitflags.value, value)

    def test_serialize(self):
        value = Color.RED | Color.GREEN

        writer = BufferWriter(1)
        ColorSerde(value).serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer, bytearray([value.value]))
