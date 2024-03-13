from dataclasses import dataclass
from typing import Self
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize
from py_core.serde.primitives import UInt8


@dataclass(frozen=True)
class ClusterId(DeriveDeserialize, DeriveSerialize):
    value: UInt8

    @property
    def id(self) -> int | None:
        return None if self.value.value == 0 else self.value.value

    @classmethod
    def no_cluster(cls) -> Self:
        return cls(UInt8(0))

    def __str__(self) -> str:
        return f"{self.id}"

    def is_no_cluster(self) -> bool:
        return self.value.value == 0
