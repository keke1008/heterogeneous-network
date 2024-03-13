import unittest
from py_core.serde.buffer import BufferReader, BufferWriter
from py_core.serde.primitives import UInt16, UInt8
from py_core.serde.variant import Variant


class ListVariant(Variant[UInt8 | UInt16]):
    @staticmethod
    def variant_types():
        return [UInt8, UInt16]


class TestListVariant(unittest.TestCase):
    def test_serde_first_param(self):
        writer = BufferWriter(2)
        ListVariant(UInt8(0x12)).serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer[0], 1)

        reader = BufferReader(buffer)
        v = ListVariant.deserialize(reader)
        self.assertEqual(v.value, UInt8(0x12))

    def test_serde_second_param(self):
        writer = BufferWriter(3)
        ListVariant(UInt16(0x3456)).serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer[0], 2)

        reader = BufferReader(buffer)
        v = ListVariant.deserialize(reader)
        self.assertEqual(v.value, UInt16(0x3456))


class DictVariant(Variant[UInt8 | ListVariant]):
    @staticmethod
    def variant_types():
        return {2: UInt8, 4: ListVariant}


class TestDictVariant(unittest.TestCase):
    def test_serde_first_param(self):
        writer = BufferWriter(2)
        DictVariant(UInt8(0x12)).serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer[0], 2)

        reader = BufferReader(buffer)
        v = DictVariant.deserialize(reader)
        self.assertEqual(v.value, UInt8(0x12))

    def test_serde_second_param(self):
        writer = BufferWriter(4)
        DictVariant(ListVariant(UInt16(0x3456))).serialize(writer)

        buffer = writer.unwrap_buffer()
        self.assertEqual(buffer[0], 4)

        reader = BufferReader(buffer)
        v = DictVariant.deserialize(reader)
        self.assertIsInstance(v.value, ListVariant)
        self.assertEqual(v.value.value, UInt16(0x3456))
