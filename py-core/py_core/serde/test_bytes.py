import unittest

from py_core.serde.buffer import BufferReader, BufferWriter
from py_core.serde.bytes import FixedBytes, RemainingBytes


class FiveBytes(FixedBytes):
    @staticmethod
    def byte_len() -> int:
        return 5


value = bytearray([1, 2, 3, 4, 5])


class FixedBytesTest(unittest.TestCase):
    def test_deserialize(self):
        reader = BufferReader(value)
        five_bytes = FiveBytes.deserialize(reader)

        self.assertEqual(five_bytes.value, value)

    def test_serialize(self):
        writer = BufferWriter(5)
        five_bytes = FiveBytes(value)
        five_bytes.serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer, value)


class RemainingBytesTest(unittest.TestCase):
    def test_deserialize(self):
        reader = BufferReader(value)
        remaining_bytes = RemainingBytes.deserialize(reader)

        self.assertEqual(remaining_bytes.value, value)

    def test_serialize(self):
        writer = BufferWriter(5)
        remaining_bytes = RemainingBytes(value)
        remaining_bytes.serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer, value)
