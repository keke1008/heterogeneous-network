from typing import Protocol

from .buffer import Reader, Writer


class Deserialize(Protocol):
    @classmethod
    def deserialize(cls: type, reader: Reader) -> object: ...


class Serialize(Protocol):
    def serialize(self, writer: Writer) -> None: ...
    def serialized_length(self) -> int: ...


class Serde(Deserialize, Serialize, Protocol):
    pass
