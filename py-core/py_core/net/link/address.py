from dataclasses import dataclass
from enum import Enum
from ipaddress import IPv4Address
from typing import Literal, Self

from py_core.serde.buffer import BufferWriter
from py_core.serde.primitives import UInt16
from py_core.serde.traits import Serde
from py_core.serde.variant import Variant


class AddressType(Enum):
    Loopback = 0x7F
    Serial = 0x01
    Uhf = 0x02
    Udp = 0x03
    WebSocket = 0x04


@dataclass(frozen=True)
class LoopbackAddress(Serde):
    type: Literal[AddressType.Loopback] = AddressType.Loopback

    def __str__(self) -> str:
        return f"{self.type.name}()"

    def get_body_as_bytes(self) -> bytes:
        return b""

    @classmethod
    def deserialize(cls, *_):
        return cls()

    def serialize(self, *_):
        pass

    def serialized_length(self) -> int:
        return 0


@dataclass(frozen=True)
class SingleByteAddress(Serde):
    value: int

    def __str__(self) -> str:
        return f"{self.value}"

    def get_body_as_bytes(self) -> bytes:
        writer = BufferWriter(1)
        writer.write_byte(self.value)
        return writer.unwrap_buffer()

    @classmethod
    def deserialize(cls, reader):
        return cls(reader.read_byte())

    def serialize(self, writer):
        writer.write_byte(self.value)

    def serialized_length(self) -> int:
        return 1


@dataclass(frozen=True)
class SerialAddress(SingleByteAddress):
    type: Literal[AddressType.Serial] = AddressType.Serial

    def __str__(self) -> str:
        return f"{self.type.name}({super().__str__()})"


@dataclass(frozen=True)
class UhfAddress(SingleByteAddress):
    type: Literal[AddressType.Uhf] = AddressType.Uhf

    def __str__(self) -> str:
        return f"{self.type.name}({super().__str__()})"


@dataclass(frozen=True)
class IPv4AndPortAddress(Serde):
    host: IPv4Address
    port: int

    @classmethod
    def from_string(cls, address: str) -> Self:
        ip, port = address.split(":")
        return cls(IPv4Address(ip), int(port))

    def __str__(self) -> str:
        return f"{self.host}:{self.port}"

    def get_body_as_bytes(self) -> bytes:
        writer = BufferWriter(6)
        writer.write_bytes(self.host.packed)
        UInt16(self.port).serialize(writer)
        return writer.unwrap_buffer()

    @classmethod
    def deserialize(cls, reader):
        host = IPv4Address(reader.read_bytes(4))
        port = UInt16.deserialize(reader)
        return cls(host, port.value)

    def serialize(self, writer):
        writer.write_bytes(self.host.packed)
        UInt16(self.port).serialize(writer)

    def serialized_length(self) -> int:
        return 6


@dataclass(frozen=True)
class UdpAddress(IPv4AndPortAddress):
    type: Literal[AddressType.Udp] = AddressType.Udp

    def __str__(self) -> str:
        return f"{self.type.name}({super().__str__()})"


@dataclass(frozen=True)
class WebSocketAddress(IPv4AndPortAddress):
    type: Literal[AddressType.WebSocket] = AddressType.WebSocket

    def __str__(self) -> str:
        return f"{self.type.name}({super().__str__()})"


type AddressTypes = LoopbackAddress | SerialAddress | UhfAddress | UdpAddress | WebSocketAddress


@dataclass(frozen=True)
class Address(Variant[AddressTypes]):
    @staticmethod
    def variant_types():
        return {
            AddressType.Loopback.value: LoopbackAddress,
            AddressType.Serial.value: SerialAddress,
            AddressType.Uhf.value: UhfAddress,
            AddressType.Udp.value: UdpAddress,
            AddressType.WebSocket.value: WebSocketAddress,
        }

    @property
    def type(self) -> AddressType:
        return self.value.type

    def get_body_as_bytes(self) -> bytes:
        return self.value.get_body_as_bytes()

    def __str__(self) -> str:
        return f"{AddressType(self.type).name}({self.value})"
