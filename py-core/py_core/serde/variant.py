from abc import ABC, abstractmethod
from typing import Self

from py_core.serde.buffer import Reader
from .traits import Deserialize, Serde, Serialize


class Variant[T: Serde](ABC, Deserialize, Serialize):
    value: T

    def __init__(self, value: T) -> None:
        self.value = value

    @staticmethod
    @abstractmethod
    def variant_types() -> list[type[T]] | dict[int, type[T]]: ...

    @classmethod
    def deserialize(cls, reader: Reader) -> Self:
        index = reader.read_byte()
        types = cls.variant_types()
        match types:
            case list():
                index -= 1
                if index < 0 or index >= len(types):
                    reader.raise_invalid_value_error("Variant", index)
                return cls(types[index].deserialize(reader))
            case dict():
                if index not in types:
                    reader.raise_invalid_value_error("Variant", index)
                return cls(types[index].deserialize(reader))

    def serialize(self, writer) -> None:
        types = self.variant_types()
        match types:
            case list():
                index = types.index(type(self.value)) + 1
                writer.write_byte(index)
                self.value.serialize(writer)
            case dict():
                index = next(k for k, v in types.items() if v == type(self.value))
                writer.write_byte(index)
                self.value.serialize(writer)

    def serialized_length(self) -> int:
        return 1 + self.value.serialized_length()
