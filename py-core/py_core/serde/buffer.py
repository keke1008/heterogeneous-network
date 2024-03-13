from typing import Never, Protocol


class NotEnoughSpaceException(Exception):
    pass


class UnwrapUnfinishedBufferException(Exception):
    pass


class Writer(Protocol):
    def write_byte(self, value: int) -> None: ...

    def write_bytes(self, data: bytes) -> None: ...


class BufferWriter(Writer):
    _buffer: bytearray
    _index: int

    def __init__(self, length: int = 0):
        self._buffer = bytearray(length)
        self._index = 0

    def _assert_enough_space(self, n: int) -> None:
        if self._index + n > len(self._buffer):
            raise NotEnoughSpaceException()

    def write_byte(self, value: int) -> None:
        self._assert_enough_space(1)
        self._buffer[self._index] = value
        self._index += 1

    def write_bytes(self, data: bytes) -> None:
        self._assert_enough_space(len(data))
        self._buffer[self._index : self._index + len(data)] = data
        self._index += len(data)

    def unwrap_buffer(self) -> bytearray:
        if self._index != len(self._buffer):
            raise UnwrapUnfinishedBufferException()
        return self._buffer


class NotEnoughBytesException(Exception):
    buffer: bytes
    expected: int
    actual: int

    def __init__(self, buffer: bytes, expected: int, actual: int):
        self.buffer = buffer
        self.expected = expected
        self.actual = actual
        super().__init__(f"Expected {expected} bytes, but got {actual}")


class InvalidValueException(Exception):
    buffer: bytes
    name: str
    value: object

    def __init__(self, buffer: bytes, name: str, value: object):
        self.buffer = buffer
        self.name = name
        self.value = value
        super().__init__(f"Invalid value for {name}: {value}")


class Reader(Protocol):
    def read_byte(self) -> int: ...

    def read_bytes(self, n: int) -> bytes: ...

    def read_remaining(self) -> bytes: ...

    def raise_invalid_value_error(self, name: str, value: object) -> Never: ...


class BufferReader(Reader):
    _buffer: bytes
    _index: int

    def __init__(self, buffer: bytes):
        self._buffer = buffer
        self._index = 0

    def _assert_enough_bytes(self, n: int) -> None:
        if self._index + n > len(self._buffer):
            raise NotEnoughBytesException(
                self._buffer, n, len(self._buffer) - self._index
            )

    def read_byte(self) -> int:
        self._assert_enough_bytes(1)
        value = self._buffer[self._index]
        self._index += 1
        return value

    def read_bytes(self, n: int) -> bytes:
        self._assert_enough_bytes(n)
        value = self._buffer[self._index : self._index + n]
        self._index += n
        return value

    def read_remaining(self) -> bytes:
        value = self._buffer[self._index :]
        self._index = len(self._buffer)
        return value

    def raise_invalid_value_error(self, name: str, value: object) -> Never:
        raise InvalidValueException(self._buffer, name, value)

    def remaining_length(self) -> int:
        return len(self._buffer) - self._index

    def sub_reader(self) -> "BufferReader":
        return BufferReader(self._buffer[self._index :])
