from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Self

from py_core.serde.traits import Deserialize, Serialize


@dataclass
class Constant[T](ABC, Deserialize, Serialize):
    value: T

    @staticmethod
    @abstractmethod
    def constant_value() -> T: ...

    @classmethod
    def deserialize(cls, reader) -> Self:
        return cls(cls.constant_value())

    def serialize(self, writer) -> None:
        pass

    def serialized_length(self) -> int:
        return 0
