from abc import ABC, abstractmethod
from typing import Self

from py_core.serde.primitives import UInt, UInt8

from .traits import Deserialize, Serde, Serialize


class Vector[T: Serde](ABC, Deserialize, Serialize):
    values: list[T]

    def __init__(self, values: list[T]) -> None:
        self.values = values

    @staticmethod
    @abstractmethod
    def element_type() -> type[T]: ...

    @staticmethod
    def length_type() -> type[UInt]:
        return UInt8

    @classmethod
    def deserialize(cls, reader) -> Self:
        length = cls.length_type().deserialize(reader)
        element_type = cls.element_type()
        return cls([element_type.deserialize(reader) for _ in range(length.value)])

    def serialize(self, writer) -> None:
        self.length_type()(len(self.values)).serialize(writer)
        for value in self.values:
            value.serialize(writer)

    def serialized_length(self) -> int:
        return self.length_type()(0).serialized_length() + sum(
            value.serialized_length() for value in self.values
        )
