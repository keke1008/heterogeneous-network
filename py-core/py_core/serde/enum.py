from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum, Flag
from typing import Self

from py_core.serde.primitives import UInt, UInt8
from py_core.serde.traits import Deserialize, Serialize


@dataclass(frozen=True)
class EnumSerde[T: Enum | Flag](ABC, Deserialize, Serialize):
    value: T

    @staticmethod
    @abstractmethod
    def enum_type() -> type[T]: ...

    @staticmethod
    def base_type() -> type[UInt]:
        return UInt8

    @classmethod
    def deserialize(cls, reader) -> Self:
        byte = reader.read_byte()
        try:
            return cls(cls.enum_type()(byte))
        except ValueError:
            reader.raise_invalid_value_error(cls.__name__, byte)

    def serialize(self, writer) -> None:
        self.base_type()(self.value.value).serialize(writer)

    def serialized_length(self) -> int:
        return self.base_type()(0).serialized_length()
