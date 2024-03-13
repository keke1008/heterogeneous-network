import asyncio
from dataclasses import dataclass
from enum import Enum
from typing import Final

from py_core.serde.enum import EnumSerde

from .address import Address


class ProtocolValue(Enum):
    NoProtocol = 0
    RoutingNeighbor = 1
    RoutingReactive = 2
    Rpc = 3
    Observer = 4
    Tunnel = 5


@dataclass(frozen=True)
class Protocol(EnumSerde[ProtocolValue]):
    @staticmethod
    def enum_type():
        return ProtocolValue


@dataclass
class LinkFrame:
    protocol: Protocol
    address: Address
    payload: bytes


@dataclass
class ReceivedLinkFrame(LinkFrame):
    media_disconnect_event: asyncio.Event


LINK_FRAME_MTU: Final[int] = 254


class PayloadTooLargeError(Exception):
    pass
