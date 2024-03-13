from abc import ABC

from .traits import Deserialize, Serialize
from .buffer import Reader, Writer


class DeriveDeserialize(ABC, Deserialize):
    @classmethod
    def deserialize[T](cls: type[T], reader: Reader) -> T:
        kwargs = {
            name: field.deserialize(reader)
            for name, field in cls.__annotations__.items()
        }
        return cls(**kwargs)


class DeriveSerialize(ABC, Serialize):
    def serialize(self, writer: Writer) -> None:
        for name in self.__annotations__.keys():
            getattr(self, name).serialize(writer)

    def serialized_length(self) -> int:
        return sum(
            getattr(self, name).serialized_length()
            for name in self.__annotations__.keys()
        )
