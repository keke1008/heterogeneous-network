import unittest

from .buffer import (
    BufferReader,
    BufferWriter,
    UnwrapUnfinishedBufferException,
    NotEnoughBytesException,
    NotEnoughSpaceException,
)


class TestBufferWriter(unittest.TestCase):
    def test_write_byte(self):
        buffer_writer = BufferWriter(1)
        buffer_writer.write_byte(65)
        self.assertEqual(buffer_writer.unwrap_buffer(), bytearray([65]))

    def test_write_bytes(self):
        buffer_writer = BufferWriter(5)
        buffer_writer.write_bytes(b"hello")
        self.assertEqual(
            buffer_writer.unwrap_buffer(), bytearray([104, 101, 108, 108, 111])
        )

    def test_write_bytes_not_enough(self):
        buffer_writer = BufferWriter(5)
        with self.assertRaises(NotEnoughSpaceException):
            buffer_writer.write_bytes(b"hello world")

    def test_unwrap_buffer(self):
        buffer_writer = BufferWriter(5)
        buffer_writer.write_byte(65)
        with self.assertRaises(UnwrapUnfinishedBufferException):
            buffer_writer.unwrap_buffer()


class TestBufferReader(unittest.TestCase):
    def test_read_byte(self):
        buffer = bytearray([1, 2, 3, 4, 5])
        reader = BufferReader(buffer)

        self.assertEqual(reader.read_byte(), 1)
        self.assertEqual(reader.read_byte(), 2)

    def test_read_bytes(self):
        buffer = bytearray([1, 2, 3, 4, 5])
        reader = BufferReader(buffer)

        self.assertEqual(reader.read_bytes(3), bytes([1, 2, 3]))
        self.assertEqual(reader.read_bytes(2), bytes([4, 5]))

    def test_read_bytes_not_enough(self):
        buffer = bytearray([1, 2, 3, 4, 5])
        reader = BufferReader(buffer)

        with self.assertRaises(NotEnoughBytesException):
            reader.read_bytes(6)

    def test_read_remaining(self):
        buffer = bytearray([1, 2, 3, 4, 5])
        reader = BufferReader(buffer)
        self.assertEqual(reader.read_remaining(), bytes([1, 2, 3, 4, 5]))

    def test_remaining_length(self):
        buffer = bytearray([1, 2, 3, 4, 5])
        reader = BufferReader(buffer)

        self.assertEqual(reader.remaining_length(), 5)

        reader.read_byte()
        self.assertEqual(reader.remaining_length(), 4)

    def test_sub_reader(self):
        buffer = bytearray([1, 2, 3, 4, 5])
        reader = BufferReader(buffer)
        reader.read_byte()

        sub_reader = reader.sub_reader()
        self.assertEqual(sub_reader.remaining_length(), 4)
