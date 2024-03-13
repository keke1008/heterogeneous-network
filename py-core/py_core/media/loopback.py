import asyncio

from py_core.net.link.address import LoopbackAddress
from py_core.net.link.frame import LinkFrame, Protocol, ReceivedLinkFrame
from py_core.net.link.service import FrameHandler


class LoopbackHandler(FrameHandler):
    _queue: asyncio.Queue[LinkFrame]
    _disconnect_event: asyncio.Event

    def __init__(self) -> None:
        self._queue = asyncio.Queue()

    async def send(self, frame: LinkFrame) -> None:
        await self._queue.put(frame)

    async def recv(self) -> ReceivedLinkFrame:
        frame = await self._queue.get()
        return ReceivedLinkFrame(
            frame.protocol,
            frame.address,
            frame.payload,
            self._disconnect_event,
        )

    async def send_broadcast(self, protocol: Protocol, payload: bytes) -> None:
        return await self.send(LinkFrame(protocol, LoopbackAddress(), payload))

    async def close(self) -> None:
        self._disconnect_event.set()
