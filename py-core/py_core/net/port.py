from dataclasses import dataclass
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize
from py_core.serde.primitives import UInt16


@dataclass(frozen=True)
class TunnelPort(DeriveDeserialize, DeriveSerialize):
    port: UInt16
