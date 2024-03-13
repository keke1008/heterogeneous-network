from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Self

from py_core.serde.traits import Deserialize, Serialize


@dataclass
class FixedBytes(ABC, Deserialize, Serialize):
    value: bytes

    @staticmethod
    @abstractmethod
    def byte_len() -> int: ...

    @classmethod
    def deserialize(cls, reader) -> Self:
        return cls(reader.read_bytes(cls.byte_len()))

    def serialize(self, writer) -> None:
        writer.write_bytes(self.value)

    def serialized_length(self) -> int:
        return len(self.value)


@dataclass
class RemainingBytes(Deserialize, Serialize):
    value: bytes

    @classmethod
    def deserialize(cls, reader) -> Self:
        return cls(reader.read_remaining())

    def serialize(self, writer) -> None:
        writer.write_bytes(self.value)

    def serialized_length(self) -> int:
        return len(self.value)
