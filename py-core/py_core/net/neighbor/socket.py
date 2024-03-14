import asyncio
from dataclasses import dataclass

from py_core.net.link.service import LinkSocket
from py_core.net.neighbor.table import NeighborNode
from py_core.net.node.node_id import NodeId
from py_core.net.local import LocalNodeService
from .service import NeighborService


@dataclass
class ReceivedNeighborFrame:
    source: NodeId
    payload: bytes


class NeighborSocket:
    _link_socket: LinkSocket
    _do_delay: bool
    _local_node_service: LocalNodeService
    _neighbor_service: NeighborService
    _recv_queue: asyncio.Queue[ReceivedNeighborFrame]

    def __init__(
        self,
        link_socket: LinkSocket,
        do_delay: bool,
        local_node_service: LocalNodeService,
        neighbor_service: NeighborService,
    ) -> None:
        self._link_socket = link_socket
        self._do_delay = do_delay
        self._local_node_service = local_node_service
        self._neighbor_service = neighbor_service
        self._recv_queue = asyncio.Queue()

        async def recv():
            while True:
                frame = await self._link_socket.recv()
                source = self._neighbor_service.resolve_neighbor_from_address(
                    frame.address
                )
                if source is None:
                    continue

                async def put(source: NeighborNode):
                    await asyncio.sleep(source.link_cost.into_timedelta().seconds)
                    await self._recv_queue.put(
                        ReceivedNeighborFrame(source.node_id, frame.payload)
                    )

                asyncio.create_task(put(source))

        asyncio.create_task(recv())

    async def recv(self) -> ReceivedNeighborFrame:
        return await self._recv_queue.get()
