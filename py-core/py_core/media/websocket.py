import asyncio
from dataclasses import dataclass
from ipaddress import IPv4Address
import logging
from typing import Final, Self
import websockets
from websockets.client import WebSocketClientProtocol
from websockets.server import WebSocketServerProtocol, serve

from py_core.net.link.address import WebSocketAddress
from py_core.net.link.frame import LinkFrame, Protocol, ReceivedLinkFrame
from py_core.net.link.service import FrameHandler
from py_core.serde.buffer import BufferReader, BufferWriter
from py_core.serde.bytes import RemainingBytes
from py_core.serde.derive import DeriveDeserialize, DeriveSerialize

WEBSOCKET_PORT: Final[int] = 12346


@dataclass
class WebSocketFrame(DeriveDeserialize, DeriveSerialize):
    protocol: Protocol
    payload: RemainingBytes

    @classmethod
    def from_link_frame(cls, frame: LinkFrame) -> Self:
        return cls(frame.protocol, RemainingBytes(frame.payload))

    def into_received_link_frame(
        self, address: WebSocketAddress, disconnect_event: asyncio.Event
    ) -> ReceivedLinkFrame:
        return ReceivedLinkFrame(
            self.protocol, address, self.payload.value, disconnect_event
        )


@dataclass
class RawPacket:
    data: bytes
    address: WebSocketAddress
    disconnect_event: asyncio.Event


type WebSocketProtocol = WebSocketServerProtocol | WebSocketClientProtocol


class WebSocketHandler(FrameHandler):
    _server: serve
    _connections: dict[WebSocketAddress, WebSocketProtocol]
    _recv_queue: asyncio.Queue[RawPacket]

    async def _process_protocol(self, ws: WebSocketProtocol):
        host, port = ws.remote_address
        addresss = WebSocketAddress(IPv4Address(host), port)
        self._connections[addresss] = ws

        disconnect_event = asyncio.Event()

        async def remove_connection():
            await ws.wait_closed()
            del self._connections[addresss]
            disconnect_event.set()

        asyncio.create_task(remove_connection())

        async def recv():
            async for data in ws:
                if isinstance(data, bytes):
                    packet = RawPacket(data, addresss, disconnect_event)
                    await self._recv_queue.put(packet)
                else:
                    logging.warning(f"Received non-bytes data: {data}")

        asyncio.create_task(recv())

    def __init__(self):
        self._server = serve(self._process_protocol, "0.0.0.0", WEBSOCKET_PORT)

    async def connect(self, address: WebSocketAddress, use_ssl=False) -> None:
        scheme = "wss" if use_ssl else "ws"
        uri = f"{scheme}://{address.host}:{address.port}"
        async with websockets.connect(uri) as ws:
            asyncio.create_task(self._process_protocol(ws))

    async def recv(self) -> ReceivedLinkFrame:
        packet = await self._recv_queue.get()
        reader = BufferReader(packet.data)
        frame = WebSocketFrame.deserialize(reader)
        return frame.into_received_link_frame(packet.address, packet.disconnect_event)

    async def send(self, frame: LinkFrame) -> None:
        if not isinstance(frame.address, WebSocketAddress):
            raise ValueError(f"Invalid address: {frame.address}")

        connection = self._connections.get(frame.address)
        if connection is not None:
            ws_frame = WebSocketFrame.from_link_frame(frame)
            writer = BufferWriter(ws_frame.serialized_length())
            await connection.send(writer.unwrap_buffer())

    async def send_broadcast(self, protocol: Protocol, payload: bytes) -> None:
        for address in self._connections.keys():
            frame = LinkFrame(protocol, address, payload)
            await self.send(frame)

    async def close(self) -> None:
        self._server.ws_server.close()
        await self._server.ws_server.wait_closed()
