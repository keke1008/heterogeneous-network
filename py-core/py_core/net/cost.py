from dataclasses import dataclass
import datetime

from py_core.serde.derive import DeriveDeserialize, DeriveSerialize
from py_core.serde.primitives import UInt16


@dataclass(order=True, frozen=True)
class Cost(DeriveDeserialize, DeriveSerialize):
    cost: UInt16

    def __str__(self) -> str:
        return f"{self.cost}"

    def __add__(self, other: "Cost") -> "Cost":
        return Cost(UInt16(self.cost.value + other.cost.value))

    def into_timedelta(self) -> datetime.timedelta:
        return datetime.timedelta(milliseconds=self.cost.value)
