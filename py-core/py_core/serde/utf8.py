from dataclasses import dataclass
from typing import Self

from py_core.serde.primitives import UInt, UInt8
from py_core.serde.traits import Deserialize, Serialize


@dataclass(frozen=True)
class Utf8(Deserialize, Serialize):
    value: str

    @staticmethod
    def length_type() -> type[UInt]:
        return UInt8

    @classmethod
    def deserialize(cls, reader) -> Self:
        length = cls.length_type().deserialize(reader)
        return cls(reader.read_bytes(length.value).decode("utf-8"))

    def serialize(self, writer) -> None:
        length = self.length_type()(len(self.value))
        length.serialize(writer)
        writer.write_bytes(self.value.encode("utf-8"))

    def serialized_length(self) -> int:
        length = self.length_type()(len(self.value))
        return length.serialized_length() + len(self.value.encode("utf-8"))
