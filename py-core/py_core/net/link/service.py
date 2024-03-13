import asyncio
import logging
import typing

from py_core.net.link.address import Address, AddressType
from py_core.net.link.frame import (
    LINK_FRAME_MTU,
    LinkFrame,
    PayloadTooLargeError,
    Protocol,
    ReceivedLinkFrame,
)


class BroadcastNotSupportedError(Exception):
    pass


class FrameHandler(typing.Protocol):
    async def recv(self) -> ReceivedLinkFrame: ...
    async def send(self, frame: LinkFrame) -> None: ...
    async def send_broadcast(self, protocol: Protocol, payload: bytes) -> None: ...
    async def close(self) -> None: ...


class LinkSocket:
    _recv_queue: asyncio.Queue[LinkFrame]
    _protocol: Protocol
    _service: "LinkService"

    def __init__(
        self,
        protocol: Protocol,
        _recv_queue: asyncio.Queue[LinkFrame],
        service: "LinkService",
    ) -> None:
        self._protocol = protocol
        self._recv_queue = _recv_queue
        self._service = service

    async def recv(self) -> LinkFrame:
        return await self._recv_queue.get()

    async def send(self, remote: Address, payload: bytes) -> None:
        await self._service.send(LinkFrame(self._protocol, remote, payload))

    async def send_broadcast(self, type: AddressType, payload: bytes) -> None:
        await self._service.send_broadcast(type, self._protocol, payload)


class LinkService:
    _handlers: dict[AddressType, FrameHandler]
    _listeners: dict[Protocol, asyncio.Queue[LinkFrame]]

    def __init__(self) -> None:
        self._handlers = {}

    async def send(self, frame: LinkFrame) -> None:
        if len(frame.payload) > LINK_FRAME_MTU:
            raise PayloadTooLargeError(f"Payload too large: {len(frame.payload)}")

        handler = self._handlers.get(frame.address.type)
        if handler is None:
            logging.warning(f"No handler for address type {frame.address.type}")
        else:
            await handler.send(frame)

    async def send_broadcast(
        self, type: AddressType, protocol: Protocol, payload: bytes
    ) -> None:
        if len(payload) > LINK_FRAME_MTU:
            raise PayloadTooLargeError(f"Payload too large: {len(payload)}")

        handler = self._handlers.get(type)
        if handler is None:
            logging.warning(f"No handler for address type {type}")
        else:
            await handler.send_broadcast(protocol, payload)

    def add_handler(self, type: AddressType, handler: FrameHandler) -> None:
        if type in self._handlers:
            raise ValueError(f"Handler already exists for address type {type}")
        self._handlers[type] = handler

    def open(self, protocol: Protocol) -> LinkSocket:
        if protocol in self._listeners:
            raise ValueError(f"Listener already exists for protocol {protocol}")

        queue = asyncio.Queue[LinkFrame]()
        self._listeners[protocol] = queue
        return LinkSocket(protocol, queue, self)
