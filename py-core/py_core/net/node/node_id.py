from dataclasses import dataclass
from enum import Enum
from typing import Self
from py_core.net.link.address import Address, AddressType

from py_core.serde.enum import EnumSerde
from py_core.serde.traits import Deserialize, Serialize


class NodeIdTypeValue(Enum):
    # 特別な意味を持つID
    Broadcast = 0xFF

    # Address互換のID
    Serial = 0x01
    Uhf = 0x02
    Udp = 0x03
    WebSocket = 0x04
    Loopback = 0x7F


@dataclass(frozen=True)
class NodeIdType(EnumSerde[NodeIdTypeValue]):
    @staticmethod
    def enum_type():
        return NodeIdTypeValue

    @classmethod
    def from_address_type(cls, address_type: AddressType) -> Self:
        return cls(NodeIdTypeValue(address_type.value))

    _body_bytes_size = {
        NodeIdTypeValue.Broadcast: 0,
        NodeIdTypeValue.Serial: 1,
        NodeIdTypeValue.Uhf: 1,
        NodeIdTypeValue.Udp: 6,
        NodeIdTypeValue.WebSocket: 6,
        NodeIdTypeValue.Loopback: 0,
    }

    def body_bytes_size(self):
        return self._body_bytes_size[self.value]


@dataclass(frozen=True)
class NodeId(Deserialize, Serialize):
    type: NodeIdType
    body: bytes

    @classmethod
    def deserialize(cls, reader) -> Self:
        id_type = NodeIdType.deserialize(reader)
        body = reader.read_bytes(id_type.body_bytes_size())
        return cls(id_type, body)

    def serialize(self, writer) -> None:
        self.type.serialize(writer)
        writer.write_bytes(self.body)

    def serialized_length(self) -> int:
        return self.type.serialized_length() + len(self.body)

    @classmethod
    def from_address(cls, address: Address) -> Self:
        address_type = NodeIdType.from_address_type(address.type)
        return cls(address_type, bytes(address.get_body_as_bytes()))

    @classmethod
    def broadcast(cls) -> Self:
        return cls(NodeIdType(NodeIdTypeValue.Broadcast), b"")

    def is_broadcast(self) -> bool:
        return self.type.value == NodeIdTypeValue.Broadcast

    @classmethod
    def loopback(cls) -> Self:
        return cls(NodeIdType(NodeIdTypeValue.Loopback), b"")

    def is_loopback(self) -> bool:
        return self.type.value == NodeIdTypeValue.Loopback

    def __str__(self) -> str:
        return f"{self.type}({self.body})"
