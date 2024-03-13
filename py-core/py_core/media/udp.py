from asyncio import DatagramTransport
import asyncio
from dataclasses import dataclass
from ipaddress import IPv4Address
from typing import Final, Self
from py_core.net.link.address import UdpAddress

from py_core.net.link.frame import LinkFrame, Protocol, ReceivedLinkFrame
from py_core.net.link.service import BroadcastNotSupportedError, FrameHandler
from py_core.serde.buffer import BufferReader, BufferWriter
from py_core.serde.bytes import RemainingBytes
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize


UDP_PORT: Final[int] = 12345


@dataclass
class UdpFrame(DeriveDeserialize, DeriveSerialize):
    protocol: Protocol
    payload: RemainingBytes

    @classmethod
    def from_link_frame(cls, frame: LinkFrame) -> Self:
        return cls(frame.protocol, RemainingBytes(frame.payload))

    def into_received_link_frame(
        self, address: UdpAddress, disconnect_event: asyncio.Event
    ) -> ReceivedLinkFrame:
        return ReceivedLinkFrame(
            self.protocol, address, self.payload.value, disconnect_event
        )


@dataclass
class RawPacket:
    data: bytes
    address: UdpAddress


class UdpHandler(FrameHandler):
    _server: DatagramTransport | None
    _recv_queue: asyncio.Queue[RawPacket]
    _disconnect_event: asyncio.Event

    class _Protocol(asyncio.DatagramProtocol):
        _transport: DatagramTransport
        _recv_queue: asyncio.Queue[RawPacket]

        def __init__(self, recv_queue: asyncio.Queue[RawPacket]) -> None:
            super().__init__()
            self._recv_queue = recv_queue

        def connection_made(self, transport: DatagramTransport) -> None:
            self._transport = transport

        def datagram_received(self, data: bytes, addr: tuple[str, int]) -> None:
            host, port = addr
            address = UdpAddress(IPv4Address(host), port)
            self._recv_queue.put_nowait(RawPacket(data, address))

        def connection_lost(self, exc: Exception | None) -> None:
            return super().connection_lost(exc)

    @classmethod
    async def create(cls) -> Self:
        self = cls()

        loop = asyncio.get_event_loop()
        transport, _ = await loop.create_datagram_endpoint(
            protocol_factory=lambda: self._Protocol(self._recv_queue),
            local_addr=("0.0.0.0", UDP_PORT),
        )

        self._server = transport
        return self

    async def recv(self) -> ReceivedLinkFrame:
        packet = await self._recv_queue.get()
        reader = BufferReader(packet.data)
        frame = UdpFrame.deserialize(reader)
        return frame.into_received_link_frame(packet.address, self._disconnect_event)

    async def send(self, frame: LinkFrame) -> None:
        udp_frame = UdpFrame.from_link_frame(frame)
        if not isinstance(frame.address, UdpAddress):
            raise TypeError(f"Invalid address type: {type(frame.address)}")

        if self._server is None:
            raise RuntimeError("Server not running")

        writer = BufferWriter(udp_frame.serialized_length())
        udp_frame.serialize(writer)
        address = (str(frame.address.host), frame.address.port)
        self._server.sendto(writer.unwrap_buffer(), address)

    async def send_broadcast(self, *_) -> None:
        raise BroadcastNotSupportedError()

    async def close(self) -> None:
        if self._server is not None:
            self._server.close()
            self._server = None
            self._disconnect_event.set()
