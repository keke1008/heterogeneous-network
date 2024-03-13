from abc import ABC, abstractmethod
from typing import Self
from dataclasses import dataclass

from .buffer import Reader
from .traits import Deserialize, Serialize


@dataclass
class Boolean(Deserialize, Serialize):
    value: bool

    @classmethod
    def deserialize(cls, reader) -> bool:
        match reader.read_byte():
            case 0:
                return False
            case 1:
                return True
            case other:
                return reader.raise_invalid_value_error("Boolean", other)

    def serialize(self, writer) -> None:
        writer.write_byte(1 if self.value else 0)

    def serialized_length(self) -> int:
        return 1


def _deserialize_uint(reader: Reader, bits: int) -> int:
    data = reader.read_bytes(bits // 8)
    value = 0
    for i, byte in enumerate(data):
        value |= byte << (i * 8)
    return value


def _serialize_uint(writer, value: int, bits: int) -> None:
    data = bytearray(bits // 8)
    for i in range(bits // 8):
        data[i] = (value >> (i * 8)) & 0xFF
    writer.write_bytes(data)


@dataclass
class UInt(ABC, Deserialize, Serialize):
    value: int

    @staticmethod
    @abstractmethod
    def bits() -> int: ...

    def __init__(self, value: int) -> None:
        self.value = value

    @classmethod
    def deserialize(cls, reader) -> Self:
        return cls(_deserialize_uint(reader, cls.bits()))

    def serialize(self, writer) -> None:
        _serialize_uint(writer, self.value, self.__class__.bits())

    def serialized_length(self) -> int:
        return self.__class__.bits() // 8


class UInt8(UInt):
    @staticmethod
    def bits() -> int:
        return 8


class UInt16(UInt):
    @staticmethod
    def bits() -> int:
        return 16


class UInt32(UInt):
    @staticmethod
    def bits() -> int:
        return 32


class UInt64(UInt):
    @staticmethod
    def bits() -> int:
        return 64
